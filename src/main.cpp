#include "manualMapping.h"
#include "loadLibraryA.h"
#include "unlinkDll.h"
#include "io.h"
#include <hax.h>

static void takeAction(io::action select, std::string* pProcName, std::string* pDllName, std::string* pDllDir);

int main(int argc, const char* argv[]) {
	io::setConsoleHandle();
	io::printHeader();
	
	std::string procName;
	std::string dllName;
	std::string dllDir;

	if (argc < 4) {
		io::selectTargets(&procName, &dllName, &dllDir);
	}
	else {
		procName = argv[1];
		dllName = argv[2];
		dllDir = argv[3];
	}

	// LoadLibrary injection as default
	io::action select = io::action::LOAD_LIB;

	while (select != io::action::EXIT) {
		io::printMenu(procName, dllName, dllDir, select);
		io::selectAction(&select);

		takeAction(select, &procName, &dllName, &dllDir);

		io::clearMenu();
	}

	return 0;
}


static void takeDllAction(io::action select, std::string pProcName, std::string pDllName, std::string pDllDir);

static void takeAction(io::action select, std::string* pProcName, std::string* pDllName, std::string* pDllDir) {
	
	switch (select) {
	case io::action::CHANGE_TARGETS:
		io::selectTargets(pProcName, pDllName, pDllDir);
		break;
	case io::action::EXIT:
		return;
	default:
		takeDllAction(select, *pProcName, *pDllName, *pDllDir);
	}
	
	return;
}


static DWORD searchProcId(std::string procName);

static void takeDllAction(io::action select, std::string procName, std::string dllName, std::string dllDir) {
	io::clearLog();
	io::printLogSeparator();

	const DWORD procId = searchProcId(procName);

	if (!procId) return;

	const HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);

	if (!hProc || hProc == INVALID_HANDLE_VALUE) {
		io::printWinError("Failed to get process handle.");

		// common error when trying to access a process run by higher priviliged user
		if (GetLastError() == ERROR_ACCESS_DENIED) {
			io::printPlainError("Try running as administrator.");
		}

		return;
	}

	std::string dllFullPath = (dllDir + "\\" + dllName);
	bool successful = false;

	switch (select) {
	case io::action::LOAD_LIB:
		successful = loadLib::inject(hProc, dllFullPath.c_str());
		break;
	case io::action::MAN_MAP:
		successful = manMap::inject(hProc, dllFullPath.c_str());
		break;
	case io::action::UNLINK:
		successful = ldr::unlinkModule(hProc, dllName.c_str());
		break;
	default:
		break;
	}

	CloseHandle(hProc);

	if (successful) {
		io::printInfo(io::actionLabels.at(select) + " succeeded.");
	}
	else {
		io::printPlainError(io::actionLabels.at(select) + " failed.");
	}

	return;
}


static DWORD searchProcId(std::string procName) {
	io::printInfo("Looking for process '" + procName + "'...");
	DWORD procId = 0;

	// search for the process for five seconds
	for (int i = 0; i < 50; i++) {
		procId = proc::ex::getProcId(procName.c_str());

		if (procId) break;

		Sleep(100);
	}

	if (!procId) {
		io::printWinError("Target process not found.");

		return 0;
	}

	io::printInfo("Target process found.");
	io::printInfo("Process ID: " + std::to_string(procId));

	return procId;
}