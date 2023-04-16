#include "io.h"
#include "manualMapping.h"
#include "loadLibraryA.h"
#include "unlinkDll.h"

static void takeAction(io::action select, std::string* pProcName, std::string* pDllName, std::string* pDllDir);

int main(int argc, const char* argv[]) {
	std::string procName;
	std::string dllName;
	std::string dllDir;

	io::init();
	io::printHeader();

	if (argc < 4) {
		io::selectTargets(&procName, &dllName, &dllDir);
	}
	else {
		procName = argv[1];
		dllName = argv[2];
		dllDir = argv[3];
	}

	// LoadLibrary injection as default
	io::action curAction = io::action::LOAD_LIB;

	while (curAction != io::action::EXIT) {
		io::printHeader();
		io::printTargetInfo(procName, dllName, dllDir);
		io::printMainMenu(curAction);
		io::selectAction(&curAction);

		takeAction(curAction, &procName, &dllName, &dllDir);
	}

	return 0;
}

static void takeProcessAction(io::action curAction, const std::string* pProcName, const std::string* pDllName, const std::string* pDllDir);

static void takeAction(io::action curAction, std::string* pProcName, std::string* pDllName, std::string* pDllDir) {

	switch (curAction) {
	case io::action::LOAD_LIB:
	case io::action::MAN_MAP:
	case io::action::UNLINK:
		takeProcessAction(curAction, pProcName, pDllName, pDllDir);
		break;
	case io::action::CHANGE_TARGETS:
		io::selectTargets(pProcName, pDllName, pDllDir);
		break;
	case io::action::EXIT:
	default:
		break;
	}
	
	return;
}


static void takeInjectionAction(io::action curAction, io::handleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName, const std::string* pDllDir);
static void takeUnlinkAction(io::handleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName);

static void takeProcessAction(io::action curAction, const std::string* pProcName, const std::string* pDllName, const std::string* pDllDir) {
	// Open process as default handle creation
	static io::handleCreation curHandleCreation = io::handleCreation::OPEN_PROCESS;

	io::printHandleCreationMenu(curAction, curHandleCreation);
	io::selectHandleCreation(&curHandleCreation);

	switch (curAction) {
	case io::action::LOAD_LIB:
	case io::action::MAN_MAP:
		takeInjectionAction(curAction, curHandleCreation, pProcName, pDllName, pDllDir);
		break;
	case io::action::UNLINK:
		takeUnlinkAction(curHandleCreation, pProcName, pDllName);
		break;
	default:
		break;
	}

	return;
}


static HANDLE getProcessHandle(const std::string* pProcName, io::handleCreation curHandleCreation);
static launch::tLaunchFunc getLaunchFunction(io::launchMethod launchMethod, BOOL isWow64);

static void takeInjectionAction(io::action curAction, io::handleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName, const std::string* pDllDir) {
	// create thread as default launch method
	static io::launchMethod curLaunchMethod = io::launchMethod::CREATE_THREAD;

	io::printLaunchMethodMenu(curAction, curLaunchMethod);
	io::selectLaunchMethod(&curLaunchMethod);
	
	io::initLog();

	const HANDLE hProc = getProcessHandle(pProcName, curHandleCreation);

	if (!hProc) return;

	BOOL isWow64 = FALSE;
	IsWow64Process(hProc, &isWow64);

	const launch::tLaunchFunc pLaunchFunc = getLaunchFunction(curLaunchMethod, isWow64);

	if (!pLaunchFunc) {
		CloseHandle(hProc);

		return;
	}

	const std::string dllFullPath = (*pDllDir + "\\" + *pDllName);
	bool success = false;

	switch (curAction) {
	case io::action::LOAD_LIB:
		success = loadLib::inject(hProc, dllFullPath.c_str(), pLaunchFunc);
		break;
	case io::action::MAN_MAP:
		success = manMap::inject(hProc, dllFullPath.c_str(), pLaunchFunc);
		break;
	default:
		break;
	}

	CloseHandle(hProc);

	io::printInjectionResult(curAction, curLaunchMethod, success);

	return;
}


static DWORD searchProcId(const std::string* pProcName);

static HANDLE getProcessHandle(const std::string* pProcName, io::handleCreation curHandleCreation) {
	const DWORD procId = searchProcId(pProcName);

	if (!procId) return nullptr;

	HANDLE hProc = nullptr;
	switch (curHandleCreation) {
	case io::OPEN_PROCESS:
		hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
		break;
	case io::DUPLICATE_HANDLE:
		hProc = proc::getDuplicateProcessHandle(PROCESS_ALL_ACCESS, procId);
		break;
	default:
		break;
	}

	if (!hProc || hProc == INVALID_HANDLE_VALUE) {
		io::printWinError("Failed to get process handle.");

		// common error when trying to access a process run by higher priviliged user
		if (GetLastError() == ERROR_ACCESS_DENIED) {
			io::printPlainError("Try running as administrator.");
		}

		return nullptr;
	}

	return hProc;
}


static DWORD searchProcId(const std::string* pProcName) {
	io::printInfo("Looking for process '" + *pProcName + "'...");
	DWORD procId = 0;

	// search for the process for five seconds
	for (int i = 0; i < 50; i++) {
		procId = proc::getProcessId(pProcName->c_str());

		if (procId) break;

		Sleep(100);
	}

	if (!procId) {
		io::printWinError("Could not find target process.");

		return 0;
	}

	io::printInfo("Found target process.");
	io::printInfo("Process ID: " + std::to_string(procId));

	return procId;
}


static launch::tLaunchFunc getLaunchFunction(io::launchMethod launchMethod, BOOL isWow64) {
	launch::tLaunchFunc pLaunchFunc = nullptr;

	switch (launchMethod) {
	case io::launchMethod::CREATE_THREAD:
		pLaunchFunc = launch::createThread;
		break;
	case io::launchMethod::HIJACK_THREAD:
		pLaunchFunc = launch::hijackThread;
		break;
	case io::launchMethod::SET_WINDOWS_HOOK:
		
		#ifdef _WIN64

		if (isWow64) {
			io::printPlainError("\"Set windows hook\" on x86 targets only possible from x86 binary.");
			
			return nullptr;
		}
		
		#else

		if (!isWow64) {
			io::printPlainError("\"Set windows hook\" on x64 targets only possible from x64 binary.");

			return nullptr;
		}

		#endif // _WIN64

		pLaunchFunc = launch::setWindowsHook;
		break;
	case io::launchMethod::HOOK_BEGIN_PAINT:
		pLaunchFunc = launch::hookBeginPaint;
		break;
	case io::launchMethod::QUEUE_USER_APC:
		pLaunchFunc = launch::queueUserApc;
		break;
	}

	return pLaunchFunc;
}


static void takeUnlinkAction(io::handleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName) {
	io::initLog();

	const HANDLE hProc = getProcessHandle(pProcName, curHandleCreation);

	if (!hProc) return;

	std::string resultString = "Unlinking of module";

	if (ldr::unlinkModule(hProc, pDllName->c_str())) {
		resultString += " succeeded.";
		io::printInfo(resultString);
	}
	else {
		resultString += " failed.";
		io::printPlainError(resultString);
	}

	CloseHandle(hProc);

	return;
}