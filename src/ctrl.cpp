#include "ctrl.h"
#include "io.h"
#include "loadLibraryA.h"
#include "manualMapping.h"
#include "unlinkDll.h"

namespace ctrl {

	static constexpr DWORD PROCESS_REQUIRED_ACCESS = (PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_CREATE_THREAD);

	static void takeAction(io::Action action, io::LaunchMethod launchMethod, io::HandleCreation handleCreation, const std::string& procName, const std::string& dllName, const std::string& dllDir);

	void run(std::string procName, std::string dllName, std::string dllDir) {
		io::init();
		io::printHeader();

		if (procName.empty() || dllName.empty() || dllDir.empty()) {
			io::selectTargets(procName, dllName, dllDir);
		}

		// defaults
		io::Action curAction = io::Action::LOAD_LIB;
		io::HandleCreation curHandleCreation = io::HandleCreation::OPEN_PROCESS;
		io::LaunchMethod curLaunchMethod = io::LaunchMethod::CREATE_THREAD;

		while (curAction != io::Action::EXIT) {
			io::printHeader();
			io::printTargetInfo(procName, dllName, dllDir);
			io::printMainMenu(curAction);
			curAction = io::selectEnum(curAction);

			switch (curAction) {
			case io::Action::LOAD_LIB:
			case io::Action::MAN_MAP:
				io::printLaunchMethodMenu(curAction, curLaunchMethod);
				curLaunchMethod = io::selectEnum(curLaunchMethod);
			case io::Action::UNLINK:
				io::printHandleCreationMenu(curAction, curHandleCreation);
				curHandleCreation = io::selectEnum(curHandleCreation);
				break;
			case io::Action::CHANGE_TARGETS:
				io::selectTargets(procName, dllName, dllDir);
				break;
			default:
				break;
			}

			if (curAction == io::Action::CHANGE_TARGETS) continue;

			io::initLog();
			takeAction(curAction, curLaunchMethod, curHandleCreation, procName, dllName, dllDir);
		}

		return;
	}


	static HANDLE getProcessHandle(const std::string& procName, io::HandleCreation curHandleCreation);
	static bool getIsWow64(HANDLE hProc, BOOL& isWow64);
	static void takeInjectionAction(io::Action action, io::LaunchMethod launchMethod, HANDLE hProc, BOOL isWow64, const std::string& dllName, const std::string& dllDir);
	static void takeUnlinkAction(HANDLE hProc, BOOL isWow64, const std::string& dllName);

	static void takeAction(io::Action action, io::LaunchMethod launchMethod, io::HandleCreation handleCreation, const std::string& procName, const std::string& dllName, const std::string& dllDir) {
		const HANDLE hProc = getProcessHandle(procName, handleCreation);

		if (!hProc) return;

		BOOL isWow64 = FALSE;

		if (!getIsWow64(hProc, isWow64)) {
			CloseHandle(hProc);

			return;
		}

		#ifndef _WIN64

		if (!isWow64) {
			io::printPlainError("Cannot interact with x64 process. Please use the x64 binary.");
			CloseHandle(hProc);

			return;
		}

		#endif // !_WIN64

		switch (action) {
		case io::Action::LOAD_LIB:
		case io::Action::MAN_MAP:
			takeInjectionAction(action, launchMethod, hProc, isWow64, dllName, dllDir);
			break;
		case io::Action::UNLINK:
			takeUnlinkAction(hProc, isWow64, dllName);
			break;
		default:
			break;
		}

		return;
	}
	
	
	static bool findProcIds(const std::string& procName, hax::Vector<DWORD>& procIds);

	static HANDLE getProcessHandle(const std::string& procName, io::HandleCreation curHandleCreation) {
		hax::Vector<DWORD> procIds{};

		if (!findProcIds(procName, procIds)) return nullptr;

		size_t targetProcIdIndex = 0;

		if (procIds.size() > 1) {
			io::printProcessIdMenu(procIds);
			targetProcIdIndex = io::selectProcessIdIndex(targetProcIdIndex);
		}

		const DWORD procId = procIds[targetProcIdIndex];

		io::printInfo("Selected process with ID: " + std::to_string(procId) + ".");

		HANDLE hProc = nullptr;

		switch (curHandleCreation) {
		case io::HandleCreation::OPEN_PROCESS:
			hProc = OpenProcess(PROCESS_REQUIRED_ACCESS, FALSE, procId);
			break;
		case io::HandleCreation::DUPLICATE_HANDLE:
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


	static bool findProcIds(const std::string& procName, hax::Vector<DWORD>& procIds) {
		io::printInfo("Looking for processes with name: '" + procName + "'...");

		size_t size = 0u;

		// search for the process for five seconds
		for (int i = 0; i < 50; i++) {

			if (hax::proc::getProcessIds(procName.c_str(), nullptr, &size) && size) break;

			Sleep(100);
		}

		if (!size) {
			io::printWinError("Could not find any processes.");

			return false;
		}

		procIds.resize(size);

		if (!hax::proc::getProcessIds(procName.c_str(), procIds.data(), &size) || !size) {
			io::printWinError("Failed to get process IDs.");

			return false;
		}

		io::printInfo(std::to_string(size) + " porcess(es) found.");

		return true;
	}


	static bool getIsWow64(HANDLE hProc, BOOL& isWow64) {

		if (!IsWow64Process(hProc, &isWow64)) {
			io::printWinError("Failed to get architecture of target process.");

			return false;
		}

		return true;
	}


	static hax::launch::tLaunchFunc getLaunchFunction(io::LaunchMethod launchMethod, BOOL isWow64);

	static void takeInjectionAction(io::Action curAction, io::LaunchMethod curLaunchMethod, HANDLE hProc, BOOL isWow64, const std::string& dllName, const std::string& dllDir) {
		const hax::launch::tLaunchFunc pLaunchFunc = getLaunchFunction(curLaunchMethod, isWow64);

		if (!pLaunchFunc) return;

		const std::string dllFullPath = (dllDir + "\\" + dllName);
		bool success = false;

		switch (curAction) {
		case io::Action::LOAD_LIB:
			success = loadLib::inject(hProc, isWow64, dllFullPath.c_str(), pLaunchFunc);
			break;
		case io::Action::MAN_MAP:
			success = manMap::inject(hProc, isWow64, dllFullPath.c_str(), pLaunchFunc);
			break;
		default:
			break;
		}

		io::printInjectionResult(curAction, curLaunchMethod, success);

		return;
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


	static void takeUnlinkAction(HANDLE hProc, BOOL isWow64, const std::string& dllName) {
		const bool success = ldr::unlinkModule(hProc, isWow64, dllName.c_str());
		io::printUnlinkResult(success);

		return;
	}

}