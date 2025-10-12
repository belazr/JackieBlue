#include "..\..\src\ctrl.h"
#include <Windows.h>

static HMODULE hModule;

static DWORD WINAPI runThread() {
	const BOOL wasConsoleAllocated = AllocConsole();

	FILE* fileOut = nullptr;

	if (freopen_s(&fileOut, "CONOUT$", "w", stdout) || !fileOut) {

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	FILE* fileIn = nullptr;

	if (freopen_s(&fileIn, "CONIN$", "r", stdin) || !fileIn) {

		FreeLibraryAndExitThread(hModule, 0ul);
	}
	
	std::string procName;
	std::string dllDir;
	std::string dllName;

	ctrl::run(procName, dllDir, dllName);

	fclose(fileIn);
	fclose(fileOut);

	if (wasConsoleAllocated) {
		FreeConsole();
	}

	FreeLibraryAndExitThread(hModule, 0ul);
}


BOOL APIENTRY DllMain(HMODULE hMod, DWORD reasonForCall, LPVOID) {

	if (reasonForCall != DLL_PROCESS_ATTACH) {

		return TRUE;
	}

	DisableThreadLibraryCalls(hMod);
	hModule = hMod;
	const HANDLE hThread = CreateThread(nullptr, 0u, reinterpret_cast<LPTHREAD_START_ROUTINE>(runThread), nullptr, 0ul, nullptr);

	if (!hThread) {

		return TRUE;
	}

	CloseHandle(hThread);

	return TRUE;
}