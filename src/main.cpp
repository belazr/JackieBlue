#include "io.h"
#include "manualMapping.h"
#include "loadLibraryA.h"
#include "unlinkDll.h"

#define PROCESS_REQUIRED_ACCESS (PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_CREATE_THREAD)

static void takeInjectionAction(io::Action curAction, io::LaunchMethod curLaunchMethod, io::HandleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName, const std::string* pDllDir);
static void takeUnlinkAction(io::HandleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName);

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
	io::Action curAction = io::Action::LOAD_LIB;
	io::HandleCreation curHandleCreation = io::HandleCreation::OPEN_PROCESS;
	io::LaunchMethod curLaunchMethod = io::LaunchMethod::CREATE_THREAD;

	while (curAction != io::Action::EXIT) {
		io::printHeader();
		io::printTargetInfo(procName, dllName, dllDir);
		io::printMainMenu(curAction);
		io::selectAction(&curAction);

		switch (curAction) {
		case io::Action::LOAD_LIB:
		case io::Action::MAN_MAP:
			io::printLaunchMethodMenu(curAction, curLaunchMethod);
			io::selectLaunchMethod(&curLaunchMethod);
		case io::Action::UNLINK:
			io::printHandleCreationMenu(curAction, curHandleCreation);
			io::selectHandleCreation(&curHandleCreation);
			io::initLog();
			break;
		case io::Action::CHANGE_TARGETS:
			io::selectTargets(&procName, &dllName, &dllDir);
			break;
		default:
			break;
		}

		switch (curAction) {
		case io::Action::LOAD_LIB:
		case io::Action::MAN_MAP:
			takeInjectionAction(curAction, curLaunchMethod, curHandleCreation, &procName, &dllName, &dllDir);
			break;
		case io::Action::UNLINK:
			takeUnlinkAction(curHandleCreation, &procName, &dllName);
			break;
		default:
			break;
		}

	}

	return 0;
}


static HANDLE getProcessHandle(const std::string* pProcName, io::HandleCreation curHandleCreation);
static hax::launch::tLaunchFunc getLaunchFunction(io::LaunchMethod launchMethod, BOOL isWow64);

static void takeInjectionAction(io::Action curAction, io::LaunchMethod curLaunchMethod, io::HandleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName, const std::string* pDllDir) {
	const HANDLE hProc = getProcessHandle(pProcName, curHandleCreation);

	if (!hProc) return;

	BOOL isWow64 = FALSE;
	IsWow64Process(hProc, &isWow64);

	const hax::launch::tLaunchFunc pLaunchFunc = getLaunchFunction(curLaunchMethod, isWow64);

	if (!pLaunchFunc) {
		CloseHandle(hProc);

		return;
	}

	const std::string dllFullPath = (*pDllDir + "\\" + *pDllName);
	bool success = false;

	switch (curAction) {
	case io::Action::LOAD_LIB:
		success = loadLib::inject(hProc, dllFullPath.c_str(), pLaunchFunc);
		break;
	case io::Action::MAN_MAP:
		success = manMap::inject(hProc, dllFullPath.c_str(), pLaunchFunc);
		break;
	default:
		break;
	}

	CloseHandle(hProc);

	io::printInjectionResult(curAction, curLaunchMethod, success);

	return;
}


static bool findProcIds(const std::string* pProcName, std::vector<DWORD>& procIds);

static HANDLE getProcessHandle(const std::string* pProcName, io::HandleCreation curHandleCreation) {
	std::vector<DWORD> procIds{};

	if (!findProcIds(pProcName, procIds)) return nullptr;

	size_t targetProcIdIndex = 0;
	
	if (procIds.size() > 1) {
		io::printProcessIdMenu(procIds);
		io::selectProcessIdIndex(&targetProcIdIndex);
	}

	const DWORD procId = procIds.at(targetProcIdIndex);

	io::printInfo("Injecting into process with ID: " + std::to_string(procId) + ".");

	HANDLE hProc = nullptr;
	
	switch (curHandleCreation) {
	case io::OPEN_PROCESS:
		hProc = OpenProcess(PROCESS_REQUIRED_ACCESS, FALSE, procId);
		break;
	case io::DUPLICATE_HANDLE:
		hProc = hax::proc::getDuplicateProcessHandle(PROCESS_REQUIRED_ACCESS, FALSE, procId);
		break;
	default:
		break;
	}

	if (!hProc || hProc == INVALID_HANDLE_VALUE) {
		io::printWinError("Failed to get process handle.");

		return nullptr;
	}

	return hProc;
}


static bool findProcIds(const std::string* pProcName, std::vector<DWORD>& procIds) {
	io::printInfo("Looking for processes with name: '" + *pProcName + "'...");

	size_t size = 0u;

	// search for the process for five seconds
	for (int i = 0; i < 50; i++) {

		if (hax::proc::getProcessIds(pProcName->c_str(), nullptr, &size) && size) break;

		Sleep(100);
	}

	if (!size) {
		io::printWinError("Could not find any processes.");

		return false;
	}

	DWORD* pIds = new DWORD[size]{};

	if (!pIds) {
		io::printPlainError("Failed to allocate memory.");

		return false;
	}

	size_t tmp = size;

	if (!hax::proc::getProcessIds(pProcName->c_str(), pIds, &tmp) || !tmp) {
		delete[] pIds;
		io::printWinError("Failed to get process IDs.");

		return false;
	}

	size_t processCount = 0;

	for (size_t i = 0u; i < size; i++) {		
		
		if (pIds[i]) {
			procIds.emplace_back(pIds[i]);
			processCount++;
		}

	}

	io::printInfo(std::to_string(processCount) + " porcess(es) found.");

	delete[] pIds;

	return true;
}


static hax::launch::tLaunchFunc getLaunchFunction(io::LaunchMethod launchMethod, BOOL isWow64) {
	hax::launch::tLaunchFunc pLaunchFunc = nullptr;

	switch (launchMethod) {
	case io::LaunchMethod::CREATE_THREAD:
		pLaunchFunc = hax::launch::createThread;
		break;
	case io::LaunchMethod::HIJACK_THREAD:
		pLaunchFunc = hax::launch::hijackThread;
		break;
	case io::LaunchMethod::SET_WINDOWS_HOOK:
		
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

		pLaunchFunc = hax::launch::setWindowsHook;
		break;
	case io::LaunchMethod::HOOK_BEGIN_PAINT:
		pLaunchFunc = hax::launch::hookBeginPaint;
		break;
	case io::LaunchMethod::QUEUE_USER_APC:
		pLaunchFunc = hax::launch::queueUserApc;
		break;
	}

	return pLaunchFunc;
}


static void takeUnlinkAction(io::HandleCreation curHandleCreation, const std::string* pProcName, const std::string* pDllName) {
	const HANDLE hProc = getProcessHandle(pProcName, curHandleCreation);

	if (!hProc) return;

	const bool success = ldr::unlinkModule(hProc, pDllName->c_str());

	io::printUnlinkResult(success);

	CloseHandle(hProc);

	return;
}