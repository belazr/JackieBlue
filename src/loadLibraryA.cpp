#include "loadLibraryA.h"
#include "io.h"
#include <hax.h>

namespace loadLib {
	
	bool inject(HANDLE hProc, const char* dllPath) {
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
		proc::PeHeaders headers{};

		if (!proc::in::getPeHeaders(reinterpret_cast<HMODULE>(pDllBytes), &headers)) {
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
			io::printWinError("Failed to write to target process.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);
			
			return false;
		}

		HMODULE hKernel32 = proc::ex::getModuleHandle(hProc, "kernel32.dll");
		
		if (!hKernel32) {
			io::printWinError("Failed to get kernel32.dll module handle.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);
			
			return false;
		}

		const FARPROC _LoadLibraryA = proc::ex::getProcAddress(hProc, hKernel32, "LoadLibraryA");
		
		if (!_LoadLibraryA) {
			io::printWinError("LoadLibraryA address not found.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);
			
			return false;
		}

		const HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(_LoadLibraryA), pDllPath, 0, nullptr);
		
		if (!hThread) {
			io::printWinError("Thread creation failed.");
			VirtualFreeEx(hProc, pDllPath, 0, MEM_RELEASE);
			
			return false;
		}

		CloseHandle(hThread);

		return true;
	}

}
