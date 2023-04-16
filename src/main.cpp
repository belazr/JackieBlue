#include "io.h"
#include "manualMapping.h"
#include "loadLibraryA.h"
#include "unlinkDll.h"

#define PROCESS_REQUIRED_ACCESS (PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_CREATE_THREAD)

static void takeInjectionAction(io::action curAction, io::launchMethod curLaunchMethod, io::handleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName, const std::string* pDllDir);
static void takeUnlinkAction(io::handleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName);

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

	// defaults
	io::action curAction = io::action::LOAD_LIB;
	io::handleCreation curHandleCreation = io::handleCreation::OPEN_PROCESS;
	io::launchMethod curLaunchMethod = io::launchMethod::CREATE_THREAD;

	while (curAction != io::action::EXIT) {
		io::printHeader();
		io::printTargetInfo(procName, dllName, dllDir);
		io::printMainMenu(curAction);
		io::selectAction(&curAction);

		switch (curAction) {
		case io::action::LOAD_LIB:
		case io::action::MAN_MAP:
			io::printLaunchMethodMenu(curAction, curLaunchMethod);
			io::selectLaunchMethod(&curLaunchMethod);
		case io::action::UNLINK:
			io::printHandleCreationMenu(curAction, curHandleCreation);
			io::selectHandleCreation(&curHandleCreation);
			io::initLog();
			break;
		case io::action::CHANGE_TARGETS:
			io::selectTargets(&procName, &dllName, &dllDir);
			break;
		default:
			break;
		}

		switch (curAction) {
		case io::action::LOAD_LIB:
		case io::action::MAN_MAP:
			takeInjectionAction(curAction, curLaunchMethod, curHandleCreation, &procName, &dllName, &dllDir);
			break;
		case io::action::UNLINK:
			takeUnlinkAction(curHandleCreation, &procName, &dllName);
			break;
		default:
			break;
		}

	}

	return 0;
}


static HANDLE getProcessHandle(const std::string* pProcName, io::handleCreation curHandleCreation);
static launch::tLaunchFunc getLaunchFunction(io::launchMethod launchMethod, BOOL isWow64);

static void takeInjectionAction(io::action curAction, io::launchMethod curLaunchMethod, io::handleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName, const std::string* pDllDir) {
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
		hProc = OpenProcess(PROCESS_REQUIRED_ACCESS, FALSE, procId);
		break;
	case io::DUPLICATE_HANDLE:
		hProc = proc::getDuplicateProcessHandle(PROCESS_REQUIRED_ACCESS, FALSE, procId);
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