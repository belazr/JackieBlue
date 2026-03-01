#include "io.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <unordered_map>

namespace io {
	
	static constexpr char HEADER[]{
		"    /$$$$$                     /$$       /$$           /$$$$$$$  /$$                    \n"
		"   |__  $$                    | $$      |__/          | $$__  $$| $$                    \n"
		"      | $$  /$$$$$$   /$$$$$$$| $$   /$$ /$$  /$$$$$$ | $$  \\ $$| $$ /$$   /$$  /$$$$$$ \n"
		"      | $$ |____  $$ /$$_____/| $$  /$$/| $$ /$$__  $$| $$$$$$$ | $$| $$  | $$ /$$__  $$\n"
		" /$$  | $$  /$$$$$$$| $$      | $$$$$$/ | $$| $$$$$$$$| $$__  $$| $$| $$  | $$| $$$$$$$$\n"
		"| $$  | $$ /$$__  $$| $$      | $$_  $$ | $$| $$_____/| $$  \\ $$| $$| $$  | $$| $$_____/\n"
		"|  $$$$$$/|  $$$$$$$|  $$$$$$$| $$ \\  $$| $$|  $$$$$$$| $$$$$$$/| $$|  $$$$$$/|  $$$$$$$\n"
		" \\______/  \\_______/ \\_______/|__/  \\__/|__/ \\_______/|_______/ |__/ \\______/  \\_______/\n"
	};
	static constexpr char PROCESS_LABEL[]{ "Process:\t\t" };
	static constexpr char DLL_LABEL[]{ "DLL:\t\t\t" };
	static constexpr char PATH_LABEL[]{ "DLL Directory:\t" };

	static constexpr char MENU_PREFIX[]{ "+ " };
	static constexpr char ERROR_PREFIX[]{ "# " };
	static constexpr char INFO_PREFIX[]{ "> " };
	static constexpr char SUCCESS_PREFIX[]{ "! " };
	static constexpr char LOG_SEP[]{ "------------------------- LOG -------------------------" };

	// labels for actions
	static const std::unordered_map<Action, std::string> actionLabels{
		{ Action::LOAD_LIB, "LoadLibraryA injection"},
		{ Action::MAN_MAP, "Manual mapping"},
		{ Action::UNLINK, "Unlink DLL"},
		{ Action::CHANGE_TARGETS, "Change targets"},
		{ Action::EXIT, "Exit"}
	};

	// labels for launchMethods
	static const std::unordered_map<LaunchMethod, std::string> launchMethodLabels{
		{ LaunchMethod::CREATE_THREAD, "Create thread"},
		{ LaunchMethod::HIJACK_THREAD, "Hijack thread"},
		{ LaunchMethod::SET_WINDOWS_HOOK, "Set windows hook"},
		{ LaunchMethod::HOOK_BEGIN_PAINT, "Hook NtUserBeginPaint"},
		{ LaunchMethod::QUEUE_USER_APC, "Queue user APC"}
	};

	// labels for handleCreations
	static const std::unordered_map<HandleCreation, std::string> handleCreationLabels{
		{ HandleCreation::OPEN_PROCESS, "Call OpenProcess"},
		{ HandleCreation::DUPLICATE_HANDLE, "Duplicate handle"}
	};

	static HANDLE hStdOut;

	// cursor positions relevant for printing static console output
	static COORD cursorAfterHeader;
	static COORD cursorAfterTargetInfo;
	static COORD cursorAfterSelect;
	static COORD cursorAfterLog;


	void init() {
		hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

		return;
	}


	static COORD getCursorPosition();

	void printHeader() {
		SetConsoleCursorPosition(hStdOut, { 0, 0 });
		std::cout << HEADER << std::endl;

		DWORD written = 0;

		for (SHORT line = 0; line < 8; line++) {
			// sets the color of first word of the header to purple
			FillConsoleOutputAttribute(hStdOut, FOREGROUND_BLUE | FOREGROUND_RED, 54, { 0, line }, &written);
			// sets the color of second word of the header to blue
			FillConsoleOutputAttribute(hStdOut, FOREGROUND_BLUE, 34, { 54, line }, &written);
		}

		cursorAfterHeader = getCursorPosition();

		return;
	}


	static void clearConsole(COORD from, COORD to);
	static void printMenuItem(const std::string& item);
	
	void printTargetInfo(const std::string& procName, const std::string& dllName, const std::string& dllDir) {
		clearConsole(cursorAfterHeader, cursorAfterTargetInfo);

		printMenuItem(PROCESS_LABEL + procName);
		printMenuItem(DLL_LABEL + dllName);
		printMenuItem(PATH_LABEL + dllDir);
		std::cout << std::endl;

		cursorAfterTargetInfo = getCursorPosition();

		return;
	}
	
	
	template<typename Enum>
	static std::string getMenuEntryString(Enum curEntry, const std::unordered_map<Enum, std::string>& map, bool isSelected);

	void printMainMenu(Action curAction) {
		clearConsole(cursorAfterTargetInfo, cursorAfterSelect);

		printMenuItem("Select action:");

		// print all options starting at the first non exit option
		for (int i = static_cast<int>(Action::EXIT) + 1; i < static_cast<int>(Action::MAX_VAL); i++) {
			printMenuItem(getMenuEntryString(static_cast<Action>(i), actionLabels, i == static_cast<int>(curAction)));
		}

		// print the exit option as last entry
		printMenuItem(getMenuEntryString(Action::EXIT, actionLabels, false));
		std::cout << std::endl;

		return;
	}


	void printLaunchMethodMenu(Action curAction, LaunchMethod curLaunchMethod) {
		clearConsole(cursorAfterTargetInfo, cursorAfterSelect);

		printMenuItem("Select launch method (" + actionLabels.at(curAction) + "):");

		for (int i = static_cast<int>(LaunchMethod::CREATE_THREAD); i < static_cast<int>(LaunchMethod::MAX_VAL); i++) {
			printMenuItem(getMenuEntryString(static_cast<LaunchMethod>(i), launchMethodLabels, i == static_cast<int>(curLaunchMethod)));
		}

		std::cout << std::endl;

		return;
	}


	void printHandleCreationMenu(Action curAction, HandleCreation curHandleCreation) {
		clearConsole(cursorAfterTargetInfo, cursorAfterSelect);

		printMenuItem("Select handle creation (" + actionLabels.at(curAction) + "):");

		for (int i = static_cast<int>(HandleCreation::OPEN_PROCESS); i < static_cast<int>(HandleCreation::MAX_VAL); i++) {
			printMenuItem(getMenuEntryString(static_cast<HandleCreation>(i), handleCreationLabels, i == static_cast<int>(curHandleCreation)));
		}

		std::cout << std::endl;

		return;
	}


	void printProcessIdMenu(const hax::Vector<DWORD>& procIds) {
		clearConsole(cursorAfterTargetInfo, cursorAfterSelect);

		printMenuItem("Select process ID:");

		for (size_t i = 0; i < procIds.size(); i++) {
			std::string menuItem{};

			if (i == 0) {
				menuItem += "[";
			}
			else {
				menuItem += " ";
			}

			menuItem += std::to_string(i + 1);

			if (i == 0) {
				menuItem += "]";
			}
			else {
				menuItem += " ";
			}

			menuItem += " " + std::to_string(procIds[i]);

			printMenuItem(menuItem);
		}

		std::cout << std::endl;

		return;
	}


	static int getIntInput();

	template<typename Enum>
	Enum selectEnum(Enum current) {
		const int input = getIntInput();

		if (input < 0 || input >= static_cast<int>(Enum::MAX_VAL)) return current;

		return static_cast<Enum>(input);
	}


	size_t selectProcessIdIndex(size_t current) {
		const int input = getIntInput();

		if (input < 1) return current;

		SetConsoleCursorPosition(hStdOut, cursorAfterLog);

		return static_cast<size_t>(input) - 1;
	}


	static void getTargetInput(const std::string& info, std::string& targetInfo);

	void selectTargets(std::string& procName, std::string& dllName, std::string& dllDir) {
		clearConsole(cursorAfterHeader, cursorAfterTargetInfo);

		getTargetInput(PROCESS_LABEL, procName);
		getTargetInput(DLL_LABEL, dllName);
		getTargetInput(PATH_LABEL, dllDir);

		return;
	}


	void initLog() {
		clearConsole(cursorAfterSelect, cursorAfterLog);
		SetConsoleCursorPosition(hStdOut, cursorAfterSelect);

		std::cout
			<< LOG_SEP << std::endl
			<< std::endl;

		return;
	}


	void printWinError(const std::string& msg, DWORD winError) {
		
		if (winError == ERROR_SUCCESS) {
			winError = GetLastError();
		}
		
		std::string winErrorStr = " Error code: " + std::to_string(winError);
		printPlainError(msg + winErrorStr);

		// common error when trying to access a process run by higher priviliged user
		if (winError == ERROR_ACCESS_DENIED) {
			io::printPlainError("Try running as administrator.");
		}

		return;
	}


	void printLaunchError(hax::launch::Status status){
		std::string launchErrorMsg = "Failed to launch code execution in target process with status: ";
		launchErrorMsg.append(std::to_string(status) + ".");
		printPlainError(launchErrorMsg);

		return;
	}


	static void printLogText(const std::string&, const char* prefix, WORD attribute);

	void printPlainError(const std::string& msg) {
		printLogText(msg, ERROR_PREFIX, FOREGROUND_RED);

		return;
	}


	void printInfo(const std::string& msg) {
		printLogText(msg, INFO_PREFIX, 0);

		return;
	}


	void printSuccess(const std::string& msg) {
		printLogText(msg, SUCCESS_PREFIX, FOREGROUND_GREEN);

		return;
	}


	std::string formatPointer(uintptr_t ptr) {
		std::stringstream stream;
		stream
			<< "0x"
			<< std::setfill('0') << std::setw(sizeof(uintptr_t) * 2)
			<< std::hex << ptr;

		return stream.str();;
	}


	void printInjectionResult(Action curAction, LaunchMethod curLaunchMethod, bool success) {
		std::string resultString = "\"" + actionLabels.at(curAction) + "\"";
		resultString += " with launch method: \"" + launchMethodLabels.at(curLaunchMethod) + "\"";
		
		if (success) {
			resultString += " succeeded.";
			io::printSuccess(resultString);
		}
		else {
			resultString += " failed.";
			io::printPlainError(resultString);
		}

		return;
	}


	void printUnlinkResult(bool success) {
		std::string resultString = "Unlinking of module";

		if (success) {
			resultString += " succeeded.";
			io::printSuccess(resultString);
		}
		else {
			resultString += " failed.";
			io::printPlainError(resultString);
		}

		return;
	}


	static COORD getCursorPosition() {
		CONSOLE_SCREEN_BUFFER_INFO screen{};
		GetConsoleScreenBufferInfo(hStdOut, &screen);

		return screen.dwCursorPosition;
	}


	static void clearConsole(COORD from, COORD to) {
		CONSOLE_SCREEN_BUFFER_INFO screen{};
		GetConsoleScreenBufferInfo(hStdOut, &screen);

		DWORD start = from.Y * screen.dwSize.X + from.X;
		DWORD end = to.Y * screen.dwSize.X + to.X;

		if (start > end) return;

		DWORD toFill = start < end ? end - start : 0;
		DWORD written = 0;
		FillConsoleOutputCharacterA(hStdOut, ' ', toFill, from, &written);

		SetConsoleCursorPosition(hStdOut, from);

		return;
	}


	template<typename Enum>
	static std::string getMenuEntryString(Enum curEntry, const std::unordered_map<Enum, std::string>& map, bool isSelected) {
		std::string strNum = std::to_string(static_cast<int>(curEntry));
		std::string padding(4 - strNum.length(), ' ');
		std::string entryString = padding + strNum + "  " + map.at(curEntry);

		// number gets put in brackets [] if the action is selected
		if (isSelected) {
			entryString.replace(3 - strNum.length(), 1, "[");
			entryString.replace(4, 1, "]");
		}

		return entryString;
	}


	static void printMenuItem(const std::string& msg) {
		std::cout << MENU_PREFIX << msg << std::endl;

		return;
	}


	static int getIntInput() {
		std::string strInput;
		std::getline(std::cin, strInput);
		std::cout << std::endl;

		const COORD cursorCur = getCursorPosition();

		if (cursorCur.Y > cursorAfterSelect.Y) {
			cursorAfterSelect = cursorCur;
		}

		// if input is empty or not a number return without setting new selected action
		if (strInput == "" || strInput.find_first_not_of("1234567890") != std::string::npos) {

			return -1;
		}

		return std::stoi(strInput);
	}

	
	static void getTargetInput(const std::string& label, std::string& targetInfo) {
		COORD cursorCur = getCursorPosition();

		// gets the targer input from user input until set
		// keeps value if already set and user input is empty
		do {
			SetConsoleCursorPosition(hStdOut, cursorCur);

			std::string input;
			std::cout << MENU_PREFIX << label;
			std::getline(std::cin, input);

			// current value is overwritten if users input is not empty
			if (!input.empty()) {
				targetInfo = input;
			}

		} while (targetInfo.empty());

		SetConsoleCursorPosition(hStdOut, cursorCur);
		printMenuItem(label + targetInfo);

		return;
	}


	static void printLogText(const std::string& msg, const char* prefix, WORD attribute) {
		COORD start = getCursorPosition();

		std::cout << prefix << msg;

		COORD end = getCursorPosition();

		if (attribute) {
			DWORD written = 0;
			FillConsoleOutputAttribute(hStdOut, attribute, end.X - start.X, start, &written);
		}
		
		std::cout << std::endl;

		// keep track of the cursor position after the logging section
		cursorAfterLog = getCursorPosition();

		return;
	}

}