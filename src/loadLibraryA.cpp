#include "loadLibraryA.h"
#include "io.h"

namespace loadLib {
	
	bool inject(HANDLE hProc, BOOL isWow64, const char* dllPath, hax::launch::tLaunchFunc pLaunchFunc) {
		hax::FileMapper dllMapper(dllPath);
		const DWORD err = dllMapper.map();

		if (err != ERROR_SUCCESS) {
			io::printWinError("Failed to map DLL.", err);

			return false;
		}

		const BYTE* const pImage = dllMapper.data();
		hax::proc::PeHeaders headers{};

		if (!hax::proc::in::getPeHeaders(reinterpret_cast<HMODULE>(const_cast<BYTE*>(pImage)), &headers)) {
			io::printPlainError("File format is not PE.");

			return false;
		}

		if (isWow64 && headers.pOptHeader64 || !isWow64 && headers.pOptHeader32) {
			io::printPlainError("DLL and target process architecture do not match.");

			return false;
		}

		void* const pDllPath = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		
		if (!pDllPath) {
			io::printWinError("Failed to allocate memory for DLL path in target process.");
			
			return false;
		}

		if (!WriteProcessMemory(hProc, pDllPath, dllPath, strlen(dllPath) + 1, nullptr)) {
			io::printWinError("Failed to write DLL path to target process.");
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
		const hax::launch::Status status = pLaunchFunc(hProc, reinterpret_cast<hax::launch::tLaunchableFunc>(_LoadLibraryA), pDllPath, &pModBase);

		if (status != hax::launch::Status::SUCCESS) {
			io::printLaunchError(status);
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
