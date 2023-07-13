#include "loadLibraryA.h"
#include "io.h"

namespace loadLib {
	
	bool inject(HANDLE hProc, const char* dllPath, hax::launch::tLaunchFunc pLaunchFunc) {
		BOOL isWow64 = FALSE;
		IsWow64Process(hProc, &isWow64);
		
		#ifndef _WIN64

		if (!isWow64) {
			io::printPlainError("Can not inject into x64 process. Please use the x64 binary.");
			return false;
		}
		
		#endif // !_WIN64

		// loads dll to memory to check compatibility
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

		if (isWow64 && headers.pOptHeader64 || !isWow64 && headers.pOptHeader32) {
			io::printPlainError("Process and DLL architecture not compatible.");

			return false;
		}

		void* const pDllPath = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		
		if (!pDllPath) {
			io::printWinError("Failed to allocate memory in target process.");
			
			return false;
		}

		// writes dll path to target process including terminating null character
		if (!WriteProcessMemory(hProc, pDllPath, dllPath, strlen(dllPath) + 1, nullptr)) {
			io::printWinError("Failed to write dll path to target process.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);
			
			return false;
		}

		const HMODULE hKernel32 = hax::proc::ex::getModuleHandle(hProc, "kernel32.dll");
		
		if (!hKernel32) {
			io::printWinError("Failed to retrieve kernel32.dll module handle from target process.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);
			
			return false;
		}

		const FARPROC _LoadLibraryA = hax::proc::ex::getProcAddress(hProc, hKernel32, "LoadLibraryA");
		
		if (!_LoadLibraryA) {
			io::printWinError("Failed to retrieve LoadLibraryA address from target process.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);
			
			return false;
		}

		uintptr_t pModBase = 0;

		if (!pLaunchFunc(hProc, reinterpret_cast<hax::launch::tLaunchableFunc>(_LoadLibraryA), pDllPath, &pModBase)) {
			io::printWinError("Failed to launch code execution in target process.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);
			
			return false;
		}

		if (!pModBase) {
			io::printPlainError("Failed to load module.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);

			return false;
		}
		
		VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);

		io::printInfo("Module loaded at: " + io::formatPointer(pModBase));

		return true;
	}

}
