#include "manualMapping.h"
#include "io.h"

namespace manMap {

	static bool writeMappedDll(HANDLE hProc, BYTE* pImageBase, const IMAGE_NT_HEADERS* pNtHeaders, BYTE* pDllBytes, BOOL isWow64);
	static BYTE* writeShellCode(HANDLE hProc, BOOL isWow64);

	bool inject(HANDLE hProc, const char* dllPath, hax::launch::tLaunchFunc pLaunchFunc) {
		BOOL isWow64 = FALSE;
		IsWow64Process(hProc, &isWow64);
		
		#ifndef _WIN64
		
		if (!isWow64) {
			io::printPlainError("Can not unlink dll from x64 process. Please use the x64 binary.");
			
			return false;
		}

		#endif // !_WIN64

		hax::FileLoader dllLoader = hax::FileLoader(dllPath);
		
		if (dllLoader.getErrno()) {
			io::printFileError("Failed to open file.", dllLoader.getErrno());
			
			return false;
		}
		
		if (!dllLoader.readBytes() || dllLoader.getErrno()) {
			io::printFileError("Failed to write file to memory.", dllLoader.getErrno());
			
			return false;
		}

		BYTE* const pDllBytes = dllLoader.getBytes();
		hax::proc::PeHeaders headers{};
		
		if (!hax::proc::in::getPeHeaders(reinterpret_cast<HMODULE>(pDllBytes), &headers)) {
			io::printPlainError("File format is not PE.");
			
			return false;
		}

		const IMAGE_NT_HEADERS* pNtHeaders = nullptr;
		BYTE* pPrefBase = nullptr;
		size_t imgSize = 0;
		
		// get prefered dll image base and size
		if (!isWow64 && headers.pOptHeader64) {
			
			#ifdef _WIN64

			pNtHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(headers.pNtHeaders64);
			pPrefBase = reinterpret_cast<BYTE*>(headers.pOptHeader64->ImageBase);
			imgSize = headers.pOptHeader64->SizeOfImage;
			
			#endif // _WIN64

		}
		else if (isWow64 && headers.pOptHeader32) {
			pNtHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(headers.pNtHeaders32);
			pPrefBase = reinterpret_cast<BYTE*>(static_cast<uintptr_t>(headers.pOptHeader32->ImageBase));
			imgSize = headers.pOptHeader32->SizeOfImage;
		}
		else {
			io::printPlainError("Process and DLL architecture not compatible.");
			
			return false;
		}

		// try to allocate memory at prefered image base
		BYTE* pImageBase = static_cast<BYTE*>(VirtualAllocEx(hProc, pPrefBase, imgSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		
		if (!pImageBase) {
			// try to allocate memory anywhere in the target process
			// module needs to be relocated after mapping in this case
			pImageBase = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, imgSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
			
			if (!pImageBase) {
				io::printWinError("Failed to allocate memory for module in target process.");
				return false;
			}

		}

		if (!writeMappedDll(hProc, pImageBase, pNtHeaders, pDllBytes, isWow64)) {
			VirtualFreeEx(hProc, pImageBase, 0, MEM_RELEASE);
			
			return false;
		}

		BYTE* pShellCode = writeShellCode(hProc, isWow64);
		
		if (!pShellCode) {
			VirtualFreeEx(hProc, pImageBase, 0, MEM_RELEASE);
			
			return false;
		}

		uintptr_t pModBase = 0;

		// parameters were written to the dll image base because the pe header of the dll is not needed anyway
		if (!pLaunchFunc(hProc, reinterpret_cast<hax::launch::tLaunchableFunc>(pShellCode), pImageBase, &pModBase)) {
			io::printWinError("Failed to launch code execution in target process.");
			VirtualFreeEx(hProc, pImageBase, 0, MEM_RELEASE);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		if (!pModBase) {
			io::printPlainError("Failed to load module.");
			VirtualFreeEx(hProc, pImageBase, 0, MEM_RELEASE);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

		io::printInfo("Module loaded at: " + io::formatPointer(pModBase));

		return true;
	}


	typedef HMODULE(WINAPI* tLoadLibraryA)(const char* dllPath);
	typedef FARPROC(WINAPI* tGetProcAddress)(HMODULE hModule, const char* funcName);

	// parameters for manual mapping shell code
	typedef struct ManMapFuncs {
		tLoadLibraryA	pLoadLibraryA;
		tGetProcAddress pGetProcAddress;
	} ManMapFuncs;

	static bool getManMapFuncs(HANDLE hProc, ManMapFuncs* pManMapFuncs);
	static void writeShellCodeParameters(BYTE* pBase, ManMapFuncs data, BOOL isWow64);
	static bool mapSections(HANDLE hProc, BYTE* pImageBase, const IMAGE_NT_HEADERS* pNtHeaders, BYTE* pDllBytes);

	static bool writeMappedDll(HANDLE hProc, BYTE* pImageBase, const IMAGE_NT_HEADERS* pNtHeaders, BYTE* pDllBytes, BOOL isWow64) {
		ManMapFuncs manMapFuncs{};
		
		if (!getManMapFuncs(hProc, &manMapFuncs)) return false;

		// write parameters of the shell code to the dll image base because pe header is not needed and to save addition memory allocation and writes to the target
		writeShellCodeParameters(pDllBytes, manMapFuncs, isWow64);

		// write header section
		if (!WriteProcessMemory(hProc, pImageBase, pDllBytes, 0x1000, nullptr)) {
			io::printWinError("Failed to write module header to target process.");
			
			return false;
		}

		// map remaining sections
		if (!mapSections(hProc, pImageBase, pNtHeaders, pDllBytes)) return false;

		return true;
	}


	static bool getManMapFuncs(HANDLE hProc, ManMapFuncs* pManMapFuncs) {
		HMODULE hKernel32 = hax::proc::ex::getModuleHandle(hProc, "kernel32.dll");
		
		if (!hKernel32) {
			io::printPlainError("Failed to retrieve kernel32.dll module handle from target process.");
			
			return false;
		}

		pManMapFuncs->pLoadLibraryA = reinterpret_cast<tLoadLibraryA>(hax::proc::ex::getProcAddress(hProc, hKernel32, "LoadLibraryA"));

		if (!pManMapFuncs->pLoadLibraryA) {
			io::printPlainError("Failed to retrieve LoadLibraryA address from target process.");
			
			return false;
		}

		pManMapFuncs->pGetProcAddress = reinterpret_cast<tGetProcAddress>(hax::proc::ex::getProcAddress(hProc, hKernel32, "GetProcAddress"));

		if (!pManMapFuncs->pGetProcAddress) {
			io::printPlainError("Failed to retrieve GetProcAddress address from target process.");
			
			return false;
		}

		return true;
	}


	static void writeShellCodeParameters(BYTE* pBase, ManMapFuncs data, BOOL isWow64) {
		
		if (isWow64) {
			memcpy_s(pBase, sizeof(UINT32), &data.pLoadLibraryA, sizeof(UINT32));
			memcpy_s(pBase + sizeof(UINT32), sizeof(UINT32), &data.pGetProcAddress, sizeof(UINT32));
			// sets flag to signal shell code execution to zero
			memset(pBase + 2 * sizeof(UINT32), 0, sizeof(BYTE));
		}
		else {
			
			#ifdef _WIN64

			memcpy_s(pBase, sizeof(data), &data, sizeof(data));
			// sets flag to signal shell code execution to zero
			memset(pBase + sizeof(data), 0, sizeof(BYTE));
			
			#endif // _WIN64
		
		}

	}


	static bool mapSections(HANDLE hProc, BYTE* pImageBase, const IMAGE_NT_HEADERS* pNtHeaders, BYTE* pDllBytes) {
		const IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);
		
		for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++, pSectionHeader++) {
			
			if (pSectionHeader->SizeOfRawData) {
				
				if (!WriteProcessMemory(hProc, pImageBase + pSectionHeader->VirtualAddress, pDllBytes + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr)) {
					io::printWinError("Failed to write section " + std::to_string(i) + " to target process.");
					
					return false;
				}

			}

		}

		return true;
	}


	static const BYTE shellX64[]{ 0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x6c, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xec, 0x20, 0x48, 0x8b, 0xf1, 0x48, 0x85, 0xc9, 0x0f, 0x84, 0x8d, 0x01, 0x00, 0x00, 0x4c, 0x63, 0x69, 0x3c, 0x4c, 0x8b, 0xd9, 0x4c, 0x03, 0xe9, 0x4d, 0x2b, 0x5d, 0x30, 0x0f, 0x84, 0x7c, 0x00, 0x00, 0x00, 0x49, 0x8b, 0x8d, 0xb0, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xc1, 0x48, 0xc1, 0xe8, 0x20, 0x85, 0xc0, 0x0f, 0x84, 0x63, 0x01, 0x00, 0x00, 0x44, 0x8b, 0xd1, 0x4c, 0x03, 0xd6, 0x41, 0x83, 0x3a, 0x00, 0x74, 0x5a, 0xbb, 0x00, 0xf0, 0x00, 0x00, 0xbf, 0x00, 0xa0, 0x00, 0x00, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x8b, 0x4a, 0x04, 0x4d, 0x8d, 0x42, 0x08, 0x49, 0x83, 0xe9, 0x08, 0x49, 0xd1, 0xe9, 0x74, 0x2a, 0x41, 0x0f, 0xb7, 0x10, 0x0f, 0xb7, 0xc2, 0x66, 0x23, 0xc3, 0x66, 0x3b, 0xc7, 0x75, 0x11, 0x41, 0x8b, 0x0a, 0x81, 0xe2, 0xff, 0x0f, 0x00, 0x00, 0x48, 0x8d, 0x04, 0x16, 0x4c, 0x01, 0x1c, 0x01, 0x49, 0x83, 0xc0, 0x02, 0x49, 0x83, 0xe9, 0x01, 0x75, 0xd6, 0x41, 0x8b, 0x42, 0x04, 0x4c, 0x03, 0xd0, 0x41, 0x83, 0x3a, 0x00, 0x75, 0xb8, 0x49, 0x8b, 0x8d, 0x90, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xc1, 0x48, 0xc1, 0xe8, 0x20, 0x85, 0xc0, 0x0f, 0x84, 0x97, 0x00, 0x00, 0x00, 0x4c, 0x8b, 0x26, 0x4c, 0x8b, 0x7e, 0x08, 0x4d, 0x85, 0xe4, 0x0f, 0x84, 0xd7, 0x00, 0x00, 0x00, 0x4d, 0x85, 0xff, 0x0f, 0x84, 0xce, 0x00, 0x00, 0x00, 0x44, 0x8b, 0xf1, 0x4c, 0x03, 0xf6, 0x41, 0x83, 0x7e, 0x0c, 0x00, 0x74, 0x71, 0x41, 0x8b, 0x4e, 0x0c, 0x48, 0x03, 0xce, 0x41, 0xff, 0xd4, 0x48, 0x8b, 0xe8, 0x48, 0x85, 0xc0, 0x0f, 0x84, 0xab, 0x00, 0x00, 0x00, 0x45, 0x8b, 0x06, 0x41, 0x8b, 0x56, 0x10, 0x49, 0x8d, 0x0c, 0x30, 0x48, 0x85, 0xc9, 0x48, 0x8d, 0x1c, 0x32, 0x41, 0x0f, 0x45, 0xd0, 0x48, 0x8b, 0x0c, 0x32, 0x48, 0x8d, 0x3c, 0x32, 0x48, 0x85, 0xc9, 0x74, 0x2d, 0x79, 0x05, 0x0f, 0xb7, 0xd1, 0xeb, 0x07, 0x48, 0x8d, 0x56, 0x02, 0x48, 0x03, 0xd1, 0x48, 0x8b, 0xcd, 0x41, 0xff, 0xd7, 0x48, 0x89, 0x03, 0x48, 0x85, 0xc0, 0x74, 0x6c, 0x48, 0x8b, 0x4f, 0x08, 0x48, 0x83, 0xc7, 0x08, 0x48, 0x83, 0xc3, 0x08, 0x48, 0x85, 0xc9, 0x75, 0xd3, 0x49, 0x83, 0xc6, 0x14, 0x41, 0x83, 0x7e, 0x0c, 0x00, 0x75, 0x8f, 0x49, 0x8b, 0x8d, 0xd0, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xc1, 0x48, 0xc1, 0xe8, 0x20, 0x85, 0xc0, 0x74, 0x26, 0x8b, 0xc1, 0x48, 0x8b, 0x5c, 0x30, 0x18, 0x48, 0x85, 0xdb, 0x74, 0x1a, 0x48, 0x8b, 0x03, 0x48, 0x85, 0xc0, 0x74, 0x12, 0x45, 0x33, 0xc0, 0x48, 0x8b, 0xce, 0x41, 0x8d, 0x50, 0x01, 0xff, 0xd0, 0x48, 0x83, 0xc3, 0x08, 0x75, 0xe6, 0x41, 0x8b, 0x45, 0x28, 0x45, 0x33, 0xc0, 0x48, 0x03, 0xc6, 0x48, 0x8b, 0xce, 0x41, 0x8d, 0x50, 0x01, 0xff, 0xd0, 0x48, 0x8b, 0xc6, 0xeb, 0x02, 0x33, 0xc0, 0x48, 0x8b, 0x5c, 0x24, 0x50, 0x48, 0x8b, 0x6c, 0x24, 0x58, 0x48, 0x8b, 0x74, 0x24, 0x60, 0x48, 0x83, 0xc4, 0x20, 0x41, 0x5f, 0x41, 0x5e, 0x41, 0x5d, 0x41, 0x5c, 0x5f, 0xc3 };

	static const BYTE shellX86[]{ 0x55, 0x8b, 0xec, 0x83, 0xec, 0x10, 0x53, 0x56, 0x57, 0x8b, 0x7d, 0x08, 0x85, 0xff, 0x0f, 0x84, 0x5d, 0x01, 0x00, 0x00, 0x8b, 0x57, 0x3c, 0x8b, 0xc7, 0x03, 0xd7, 0x89, 0x55, 0x08, 0x2b, 0x42, 0x34, 0x89, 0x45, 0xf4, 0x74, 0x65, 0x83, 0xba, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x82, 0xa0, 0x00, 0x00, 0x00, 0x0f, 0x84, 0x38, 0x01, 0x00, 0x00, 0x83, 0x3c, 0x38, 0x00, 0x8d, 0x1c, 0x38, 0x74, 0x49, 0x8b, 0x73, 0x04, 0x8d, 0x43, 0x04, 0x83, 0xee, 0x08, 0x89, 0x45, 0xf8, 0xd1, 0xee, 0x8d, 0x53, 0x08, 0x74, 0x2c, 0x0f, 0xb7, 0x02, 0x8b, 0xc8, 0x81, 0xe1, 0x00, 0xf0, 0x00, 0x00, 0x81, 0xf9, 0x00, 0x30, 0x00, 0x00, 0x75, 0x0e, 0x8b, 0x4d, 0xf4, 0x25, 0xff, 0x0f, 0x00, 0x00, 0x03, 0xc7, 0x03, 0x03, 0x01, 0x08, 0x83, 0xc2, 0x02, 0x83, 0xee, 0x01, 0x75, 0xd7, 0x8b, 0x45, 0xf8, 0x03, 0x18, 0x83, 0x3b, 0x00, 0x75, 0xba, 0x8b, 0x55, 0x08, 0x83, 0xba, 0x84, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x8a, 0x80, 0x00, 0x00, 0x00, 0x0f, 0x84, 0x90, 0x00, 0x00, 0x00, 0x8b, 0x17, 0x8b, 0x47, 0x04, 0x89, 0x55, 0xf0, 0x89, 0x45, 0xf8, 0x85, 0xd2, 0x0f, 0x84, 0xc0, 0x00, 0x00, 0x00, 0x85, 0xc0, 0x0f, 0x84, 0xb8, 0x00, 0x00, 0x00, 0x83, 0x7c, 0x39, 0x0c, 0x00, 0x8d, 0x1c, 0x39, 0x89, 0x5d, 0xf4, 0x74, 0x65, 0x8b, 0x43, 0x0c, 0x03, 0xc7, 0x50, 0xff, 0xd2, 0x89, 0x45, 0xfc, 0x85, 0xc0, 0x0f, 0x84, 0x98, 0x00, 0x00, 0x00, 0x8b, 0x0b, 0x8b, 0x53, 0x10, 0x8d, 0x04, 0x39, 0x85, 0xc0, 0x8d, 0x34, 0x3a, 0x0f, 0x45, 0xd1, 0x8b, 0x04, 0x3a, 0x8d, 0x1c, 0x3a, 0x85, 0xc0, 0x74, 0x26, 0x79, 0x05, 0x0f, 0xb7, 0xc0, 0xeb, 0x05, 0x83, 0xc0, 0x02, 0x03, 0xc7, 0x50, 0xff, 0x75, 0xfc, 0xff, 0x55, 0xf8, 0x89, 0x06, 0x85, 0xc0, 0x74, 0x65, 0x8b, 0x43, 0x04, 0x83, 0xc3, 0x04, 0x83, 0xc6, 0x04, 0x85, 0xc0, 0x75, 0xda, 0x8b, 0x5d, 0xf4, 0x8b, 0x55, 0xf0, 0x83, 0xc3, 0x14, 0x89, 0x5d, 0xf4, 0x83, 0x7b, 0x0c, 0x00, 0x75, 0x9b, 0x8b, 0x55, 0x08, 0x83, 0xba, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x8b, 0xb2, 0xc0, 0x00, 0x00, 0x00, 0x74, 0x1d, 0x8b, 0x74, 0x3e, 0x0c, 0x85, 0xf6, 0x74, 0x15, 0x8b, 0x06, 0x85, 0xc0, 0x74, 0x0c, 0x6a, 0x00, 0x6a, 0x01, 0x57, 0xff, 0xd0, 0x83, 0xc6, 0x04, 0x75, 0xee, 0x8b, 0x55, 0x08, 0x8b, 0x4a, 0x28, 0x6a, 0x00, 0x6a, 0x01, 0x57, 0x03, 0xcf, 0xff, 0xd1, 0x8b, 0xc7, 0x5f, 0x5e, 0x5b, 0x8b, 0xe5, 0x5d, 0xc2, 0x04, 0x00 };

	static BYTE* writeShellCode(HANDLE hProc, BOOL isWow64) {
		BYTE* pShellCode = nullptr;
		
		if (isWow64) {
			pShellCode = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(shellX86), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		}
		else {
			
			#ifdef _WIN64

			pShellCode = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(shellX64), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
			
			#endif // _WIN64

		}

		if (!pShellCode) {
			io::printWinError("Failed to allocate memory for shell code in target process.");

			return nullptr;
		}

		if (isWow64) {
			
			if (!WriteProcessMemory(hProc, pShellCode, shellX86, sizeof(shellX86), nullptr)) {
				io::printWinError("Failed to write shell code to target process.");
				return nullptr;
			}

		}
		else {
			
			#ifdef _WIN64
			
			if (!WriteProcessMemory(hProc, pShellCode, shellX64, sizeof(shellX64), nullptr)) {
				io::printWinError("Failed to write shell code to target process.");
				
				return nullptr;
			}

			#endif // _WIN64

		}

		return pShellCode;
	}


	// manuall mapping shell code C++ implementation for documentation and maintenance purposes
	// injector uses the precompiled byte arrays at the top of the file so the x64 compilation is able to inject into x86 targets and injection works in debug mode
	// DO NOT COMPILE IN DEBUG MODE
	#define COMPILE_SHELL_CODE FALSE
	
	#if COMPILE_SHELL_CODE

	#ifdef _WIN64

	#define HAS_RELOC_FLAG(relativeInfo) ((relativeInfo >> 0xC) == IMAGE_REL_BASED_DIR64)
	
	#else
	
	#define HAS_RELOC_FLAG(relativeInfo) ((relativeInfo >> 0xC) == IMAGE_REL_BASED_HIGHLOW)
	
	#endif // _WIN64

	typedef HINSTANCE(WINAPI* tDllEntryPoint)(void* hDll, DWORD dwReason, void* pReserved);

	__declspec(noinline) void* WINAPI shell(BYTE* pImageBase) {
		
		if (!pImageBase) return nullptr;

		const IMAGE_NT_HEADERS* const pNtHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(pImageBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pImageBase)->e_lfanew);
		const IMAGE_OPTIONAL_HEADER* const pOptHeader = &pNtHeaders->OptionalHeader;

		const ptrdiff_t locationDelta = reinterpret_cast<ptrdiff_t>(pImageBase - pOptHeader->ImageBase);
		
		// relocate module if not loaded at prefered image base
		if (locationDelta) {
			const IMAGE_DATA_DIRECTORY relocEntry = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
			
			if (!relocEntry.Size) return nullptr;

			const IMAGE_BASE_RELOCATION* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pImageBase + relocEntry.VirtualAddress);

			while (pRelocData->VirtualAddress) {
				const size_t entryCount = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
				const WORD* pRelativeInfo = reinterpret_cast<const WORD*>(&pRelocData[1]);

				for (size_t i = 0; i < entryCount; i++, pRelativeInfo++) {
					
					if (HAS_RELOC_FLAG(*pRelativeInfo)) {
						uintptr_t* const pPatch = reinterpret_cast<uintptr_t*>(pImageBase + pRelocData->VirtualAddress + (*pRelativeInfo & 0xFFF));
						*pPatch += locationDelta;
					}

				}

				pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(pRelocData) + pRelocData->SizeOfBlock);
			}

		}

		const IMAGE_DATA_DIRECTORY importEntry = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		
		// fix IAT
		if (importEntry.Size) {
			// functions to fix IAT were written to the image base since header is not needed
			const ManMapFuncs* const pFuncs = reinterpret_cast<ManMapFuncs*>(pImageBase);
			
			const tLoadLibraryA _LoadLibraryA = pFuncs->pLoadLibraryA;
			const tGetProcAddress _GetProcAddress = pFuncs->pGetProcAddress;

			if (!_LoadLibraryA || !_GetProcAddress) return nullptr;

			const IMAGE_IMPORT_DESCRIPTOR* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pImageBase + importEntry.VirtualAddress);
			
			while (pImportDescr->Name) {
				const char* const modName = reinterpret_cast<const char*>(pImageBase + pImportDescr->Name);
				const HINSTANCE hMod = _LoadLibraryA(modName);

				if (!hMod) return nullptr;

				uintptr_t* pOriginalThunk = reinterpret_cast<uintptr_t*>(pImageBase + pImportDescr->OriginalFirstThunk);
				uintptr_t* pThunk = reinterpret_cast<uintptr_t*>(pImageBase + pImportDescr->FirstThunk);

				if (!pOriginalThunk) {
					pOriginalThunk = pThunk;
				}

				while (*pOriginalThunk) {
					
					if (IMAGE_SNAP_BY_ORDINAL(*pOriginalThunk)) {
						*pThunk = reinterpret_cast<uintptr_t>(_GetProcAddress(hMod, reinterpret_cast<const char*>(IMAGE_ORDINAL(*pOriginalThunk))));
					}
					else {
						IMAGE_IMPORT_BY_NAME* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pImageBase + (*pOriginalThunk));
						*pThunk = reinterpret_cast<uintptr_t>(_GetProcAddress(hMod, pImport->Name));
					}

					if (!(*pThunk)) return nullptr;
					
					pOriginalThunk++;
					pThunk++;
				}

				pImportDescr++;
			}

		}

		const IMAGE_DATA_DIRECTORY tlsEntry = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
		
		// execute tls callbacks
		if (tlsEntry.Size) {
			const IMAGE_TLS_DIRECTORY* const pTls = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pImageBase + tlsEntry.VirtualAddress);
			const PIMAGE_TLS_CALLBACK* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTls->AddressOfCallBacks);
			
			while (pCallback && *pCallback) {
				(*pCallback)(pImageBase, DLL_PROCESS_ATTACH, nullptr);
				pCallback++;
			}

		}

		const tDllEntryPoint _DllMain = reinterpret_cast<tDllEntryPoint>(pImageBase + pOptHeader->AddressOfEntryPoint);
		// execute DllMain
		_DllMain(pImageBase, DLL_PROCESS_ATTACH, nullptr);
		
		return pImageBase;
	}

	#endif

}