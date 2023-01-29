#include "manualMapping.h"
#include "io.h"
#include <hax.h>

namespace manMap {

	typedef HINSTANCE(WINAPI* tLoadLibraryA)(const char* lpLibFilename);
	typedef FARPROC(WINAPI* tGetProcAddress)(HINSTANCE hModule, const char* lpProcName);

	// parameters of the shell code
	typedef struct ManMapFuncs {
		tLoadLibraryA	pLoadLibraryA;
		tGetProcAddress pGetProcAddress;
	} ManMapFuncs;

	// shell code precompiled in x86 to be compatible with both x64 as well as x86 targets for x64 injector
	const BYTE x86shell[]{ 0x55, 0x8b, 0xec, 0x83, 0xec, 0x14, 0x53, 0x56, 0x57, 0x8b, 0x7d, 0x08, 0x85, 0xff, 0x0f, 0x84, 0x3e, 0x01, 0x00, 0x00, 0x8b, 0x4f, 0x3c, 0x8b, 0x47, 0x04, 0x03, 0xcf, 0x8b, 0x17, 0x89, 0x45, 0xfc, 0x89, 0x55, 0xf4, 0x8b, 0x41, 0x28, 0x03, 0xc7, 0x89, 0x4d, 0xf0, 0x89, 0x45, 0xec, 0x8b, 0xc7, 0x2b, 0x41, 0x34, 0x89, 0x45, 0x08, 0x74, 0x6f, 0x83, 0xb9, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x81, 0xa0, 0x00, 0x00, 0x00, 0x0f, 0x84, 0x06, 0x01, 0x00, 0x00, 0x83, 0x3c, 0x38, 0x00, 0x8d, 0x1c, 0x38, 0x74, 0x53, 0x8b, 0x73, 0x04, 0x8d, 0x43, 0x04, 0x83, 0xee, 0x08, 0x89, 0x45, 0xf8, 0xd1, 0xee, 0x8d, 0x53, 0x08, 0x74, 0x33, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xb7, 0x02, 0x8b, 0xc8, 0x81, 0xe1, 0x00, 0xf0, 0x00, 0x00, 0x81, 0xf9, 0x00, 0x30, 0x00, 0x00, 0x75, 0x0d, 0x8b, 0x4d, 0x08, 0x25, 0xff, 0x0f, 0x00, 0x00, 0x03, 0x03, 0x01, 0x0c, 0x38, 0x83, 0xc2, 0x02, 0x83, 0xee, 0x01, 0x75, 0xd8, 0x8b, 0x45, 0xf8, 0x03, 0x18, 0x83, 0x3b, 0x00, 0x75, 0xb3, 0x8b, 0x4d, 0xf0, 0x8b, 0x55, 0xf4, 0x83, 0xb9, 0x84, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x81, 0x80, 0x00, 0x00, 0x00, 0x74, 0x66, 0x83, 0x7c, 0x38, 0x0c, 0x00, 0x8d, 0x1c, 0x38, 0x89, 0x5d, 0xf8, 0x74, 0x59, 0x8b, 0x43, 0x0c, 0x03, 0xc7, 0x50, 0xff, 0xd2, 0x8b, 0x13, 0x8b, 0x73, 0x10, 0x89, 0x45, 0x08, 0x8d, 0x0c, 0x3a, 0x85, 0xc9, 0x8d, 0x1c, 0x3e, 0x0f, 0x45, 0xf2, 0x03, 0xf7, 0x8b, 0x06, 0x85, 0xc0, 0x74, 0x21, 0x79, 0x05, 0x0f, 0xb7, 0xc0, 0xeb, 0x05, 0x83, 0xc0, 0x02, 0x03, 0xc7, 0x50, 0xff, 0x75, 0x08, 0xff, 0x55, 0xfc, 0x83, 0xc6, 0x04, 0x89, 0x03, 0x83, 0xc3, 0x04, 0x8b, 0x06, 0x85, 0xc0, 0x75, 0xdf, 0x8b, 0x5d, 0xf8, 0x8b, 0x55, 0xf4, 0x83, 0xc3, 0x14, 0x89, 0x5d, 0xf8, 0x83, 0x7b, 0x0c, 0x00, 0x75, 0xaa, 0x8b, 0x4d, 0xf0, 0x83, 0xb9, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x8b, 0xb1, 0xc0, 0x00, 0x00, 0x00, 0x74, 0x1a, 0x8b, 0x74, 0x3e, 0x0c, 0x85, 0xf6, 0x74, 0x12, 0x8b, 0x06, 0x85, 0xc0, 0x74, 0x0c, 0x6a, 0x00, 0x6a, 0x01, 0x57, 0xff, 0xd0, 0x83, 0xc6, 0x04, 0x75, 0xee, 0x6a, 0x00, 0x6a, 0x01, 0x57, 0xff, 0x55, 0xec, 0xc6, 0x47, 0x08, 0x01, 0x5f, 0x5e, 0x5b, 0x8b, 0xe5, 0x5d, 0xc2, 0x04, 0x00 };

	static bool writeMappedDll(HANDLE hProc, BYTE* pImageBase, const IMAGE_NT_HEADERS* pNtHeaders, BYTE* pDllBytes, BOOL isWow64);
	static BYTE* writeShellCode(HANDLE hProc, BOOL isWow64);
	static bool checkShellcodeFlag(HANDLE hProc, BYTE* pImageBase, BOOL isWow64);
	static void __stdcall shell(BYTE* pImageBase);

	bool inject(HANDLE hProc, const char* dllPath) {
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
		proc::PeHeaders headers{};
		
		if (!proc::in::getPeHeaders(reinterpret_cast<HMODULE>(pDllBytes), &headers)) {
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
				io::printWinError("Failed to allocate memory for DLL in target.");
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

		// starts thread with shell code
		// parameters were writen to the dll image base because the pe header of the dll is not needed anyway
		HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellCode), pImageBase, 0, nullptr);
		
		if (!hThread) {
			io::printWinError("Thread creation failed.");
			VirtualFreeEx(hProc, pImageBase, 0, MEM_RELEASE);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);
			
			return false;
		}

		CloseHandle(hThread);
		
		// wait until the shell code has set the flag to signal success
		if (!checkShellcodeFlag(hProc, pImageBase, isWow64)) {
			io::printPlainError("Shell code failed.");
			VirtualFreeEx(hProc, pImageBase, 0, MEM_RELEASE);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);
		
		return true;
	}


	static bool getManMapFuncs(HANDLE hProc, ManMapFuncs* pManMapFuncs);
	static void writeShellCodeParameters(BYTE* pBase, ManMapFuncs data, BOOL isWow64);
	static bool mapSections(HANDLE hProc, BYTE* pImageBase, const IMAGE_NT_HEADERS* pNtHeaders, BYTE* pDllBytes);

	static bool writeMappedDll(HANDLE hProc, BYTE* pImageBase, const IMAGE_NT_HEADERS* pNtHeaders, BYTE* pDllBytes, BOOL isWow64) {
		ManMapFuncs manMapFuncs{};
		
		if (!getManMapFuncs(hProc, &manMapFuncs)) return false;

		// writes parameters of the shell code to the dll image base because pe header is not needed and to save addition memory allocation and writes to the target
		writeShellCodeParameters(pDllBytes, manMapFuncs, isWow64);

		// write header section
		if (!WriteProcessMemory(hProc, pImageBase, pDllBytes, 0x1000, nullptr)) {
			io::printWinError("Failed to write header to target.");
			
			return false;
		}

		// maps remaining sections
		if (!mapSections(hProc, pImageBase, pNtHeaders, pDllBytes)) return false;

		return true;
	}


	static bool getManMapFuncs(HANDLE hProc, ManMapFuncs* pManMapFuncs) {
		HMODULE hKernel32 = proc::ex::getModuleHandle(hProc, "kernel32.dll");
		
		if (!hKernel32) {
			io::printPlainError("Failed to get kernel32.dll module handle.");
			
			return false;
		}

		pManMapFuncs->pLoadLibraryA = reinterpret_cast<tLoadLibraryA>(proc::ex::getProcAddress(hProc, hKernel32, "LoadLibraryA"));

		if (!pManMapFuncs->pLoadLibraryA) {
			io::printPlainError("LoadLibraryA address not found.");
			
			return false;
		}

		pManMapFuncs->pGetProcAddress = reinterpret_cast<tGetProcAddress>(proc::ex::getProcAddress(hProc, hKernel32, "GetProcAddress"));

		if (!pManMapFuncs->pGetProcAddress) {
			io::printPlainError("GetProcAddress address not found.");
			
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
					io::printWinError("Failed to write section " + std::to_string(i) + " to target.");
					
					return false;
				}

			}

		}

		return true;
	}


	static BYTE* writeShellCode(HANDLE hProc, BOOL isWow64) {
		BYTE* pShellCode = nullptr;
		
		if (isWow64) {
			pShellCode = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(x86shell), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		}
		else {
			
			#ifdef _WIN64
			pShellCode = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, 0x500, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
			#endif // _WIN64

		}

		if (!pShellCode) {
			io::printWinError("Shell code allocation failed.");

			return nullptr;
		}

		if (isWow64) {
			
			if (!WriteProcessMemory(hProc, pShellCode, x86shell, sizeof(x86shell), nullptr)) {
				io::printWinError("Writing shell code to target failed.");
				return nullptr;
			}

		}
		else {
			
			#ifdef _WIN64
			
			if (!WriteProcessMemory(hProc, pShellCode, shell, 0x500, nullptr)) {
				io::printWinError("Writing shell code to target failed.");
				
				return nullptr;
			}

			#endif // _WIN64

		}

		return pShellCode;
	}

	static bool checkShellcodeFlag(HANDLE hProc, BYTE* pImageBase, BOOL isWow64) {
		ptrdiff_t flagOffset = 0x0;
		
		if (isWow64) {
			flagOffset = 2 * sizeof(UINT32);
		}
		else {
			
			#ifdef _WIN64
			flagOffset = sizeof(ManMapFuncs);
			#endif // _WIN64

		}

		bool flag = false;

		for (int i = 0; i < 100; i++) {
			ReadProcessMemory(hProc, pImageBase + flagOffset, &flag, sizeof(bool), nullptr);
			Sleep(10);
			
			if (flag) break;
		
		}

		return flag;
	}

	#ifdef _WIN64
	#define HAS_RELOC_FLAG(relativeInfo) ((relativeInfo >> 0xC) == IMAGE_REL_BASED_DIR64)
	#else
	#define HAS_RELOC_FLAG(relativeInfo) ((relativeInfo >> 0xC) == IMAGE_REL_BASED_HIGHLOW)
	#endif // _WIN64

	// only compile in x64 because the precompiled shell code for x86 (x86shell) is already in the file
	#ifdef _WIN64
	typedef HINSTANCE(WINAPI* tDllEntryPoint)(void* hDll, DWORD dwReason, void* pReserved);

	__declspec(noinline) void __stdcall shell(BYTE* pImageBase) {
		
		if (!pImageBase) {
			
			return;
		}

		const IMAGE_NT_HEADERS* const pNtHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(pImageBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pImageBase)->e_lfanew);
		const IMAGE_OPTIONAL_HEADER* const pOptHeader = &pNtHeaders->OptionalHeader;

		const ptrdiff_t locationDelta = reinterpret_cast<ptrdiff_t>(pImageBase - pOptHeader->ImageBase);
		
		// relocates dll if not loaded at prefered image base
		if (locationDelta) {
			const IMAGE_DATA_DIRECTORY relocEntry = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
			
			if (!relocEntry.Size) {
				return;
			}

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
		
		// fixes IAT
		if (importEntry.Size) {
			// functions to fix IAT were writen to the image base
			const ManMapFuncs* const pFuncs = reinterpret_cast<ManMapFuncs*>(pImageBase);
			
			const tLoadLibraryA _LoadLibraryA = pFuncs->pLoadLibraryA;
			const tGetProcAddress _GetProcAddress = pFuncs->pGetProcAddress;

			const IMAGE_IMPORT_DESCRIPTOR* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pImageBase + importEntry.VirtualAddress);
			
			while (pImportDescr->Name) {
				const char* const modName = reinterpret_cast<const char*>(pImageBase + pImportDescr->Name);
				const HINSTANCE hDll = _LoadLibraryA(modName);

				uintptr_t* pOriginalThunk = reinterpret_cast<uintptr_t*>(pImageBase + pImportDescr->OriginalFirstThunk);
				uintptr_t* pThunk = reinterpret_cast<uintptr_t*>(pImageBase + pImportDescr->FirstThunk);

				if (!pOriginalThunk) {
					pOriginalThunk = pThunk;
				}

				while (*pOriginalThunk) {
					
					if (IMAGE_SNAP_BY_ORDINAL(*pOriginalThunk)) {
						*pThunk = reinterpret_cast<uintptr_t>(_GetProcAddress(hDll, reinterpret_cast<const char*>(IMAGE_ORDINAL(*pOriginalThunk))));
					}
					else {
						IMAGE_IMPORT_BY_NAME* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pImageBase + (*pOriginalThunk));
						*pThunk = reinterpret_cast<uintptr_t>(_GetProcAddress(hDll, pImport->Name));
					}
					
					pOriginalThunk++;
					pThunk++;
				}

				pImportDescr++;

			}

		}

		const IMAGE_DATA_DIRECTORY tlsEntry = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
		
		// executes tls callbacks
		if (tlsEntry.Size) {
			const IMAGE_TLS_DIRECTORY* const pTls = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pImageBase + tlsEntry.VirtualAddress);
			const PIMAGE_TLS_CALLBACK* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTls->AddressOfCallBacks);
			
			while (pCallback && *pCallback) {
				(*pCallback)(pImageBase, DLL_PROCESS_ATTACH, nullptr);
				pCallback++;
			}

		}

		const tDllEntryPoint _DllMain = reinterpret_cast<tDllEntryPoint>(pImageBase + pOptHeader->AddressOfEntryPoint);
		// executes DllMain
		_DllMain(pImageBase, DLL_PROCESS_ATTACH, nullptr);
		
		// sets flag to signal success
		*(pImageBase + sizeof(ManMapFuncs)) = 0x1;
	}
	#endif // _WIN64

}