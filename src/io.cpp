#include "io.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#define HEADER "\
    /$$$$$                     /$$       /$$           /$$$$$$$  /$$                    \n\
   |__  $$                    | $$      |__/          | $$__  $$| $$                    \n\
      | $$  /$$$$$$   /$$$$$$$| $$   /$$ /$$  /$$$$$$ | $$  \\ $$| $$ /$$   /$$  /$$$$$$ \n\
      | $$ |____  $$ /$$_____/| $$  /$$/| $$ /$$__  $$| $$$$$$$ | $$| $$  | $$ /$$__  $$\n\
 /$$  | $$  /$$$$$$$| $$      | $$$$$$/ | $$| $$$$$$$$| $$__  $$| $$| $$  | $$| $$$$$$$$\n\
| $$  | $$ /$$__  $$| $$      | $$_  $$ | $$| $$_____/| $$  \\ $$| $$| $$  | $$| $$_____/\n\
|  $$$$$$/|  $$$$$$$|  $$$$$$$| $$ \\  $$| $$|  $$$$$$$| $$$$$$$/| $$|  $$$$$$/|  $$$$$$$\n\
 \\______/  \\_______/ \\_______/|__/  \\__/|__/ \\_______/|_______/ |__/ \\______/  \\_______/\n\
"

#define PROCESS_LABEL	"Process:\t\t"
#define DLL_LABEL		"DLL:\t\t\t"
#define PATH_LABEL		"DLL Directory:\t"

#define MENU_PREFIX "+ "
#define ERROR_PREFIX "# "
#define INFO_PREFIX "> "
#define SUCCESS_PREFIX "! "
#define LOG_SEP "------------------------- LOG -------------------------"

namespace io {

	// labels for actions
	static const std::unordered_map<action, std::string> actionLabels{
		{ action::LOAD_LIB, "LoadLibraryA injection"},
		{ action::MAN_MAP, "Manual mapping"},
		{ action::UNLINK, "Unlink DLL"},
		{ action::CHANGE_TARGETS, "Change targets"},
		{ action::EXIT, "Exit"}
	};

	// labels for launchMethods
	static const std::unordered_map<launchMethod, std::string> launchMethodLabels{
		{ launchMethod::CREATE_THREAD, "Create thread"},
		{ launchMethod::HIJACK_THREAD, "Hijack thread"},
		{ launchMethod::SET_WINDOWS_HOOK, "Set windows hook"},
		{ launchMethod::HOOK_BEGIN_PAINT, "Hook NtUserBeginPaint"},
		{ launchMethod::QUEUE_USER_APC, "Queue user APC"}
	};

	// labels for handleCreations
	static const std::unordered_map<handleCreation, std::string> handleCreationLabels{
		{ handleCreation::OPEN_PROCESS, "Call OpenProcess"},
		{ handleCreation::DUPLICATE_HANDLE, "Duplicate handle"}
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
	static void printMenuItem(std::string item);
	
	void printTargetInfo(std::string procName, std::string dllName, std::string dllDir) {
		clearConsole(cursorAfterHeader, cursorAfterTargetInfo);

		printMenuItem(PROCESS_LABEL + procName);
		printMenuItem(DLL_LABEL + dllName);
		printMenuItem(PATH_LABEL + dllDir);
		std::cout << std::endl;

		cursorAfterTargetInfo = getCursorPosition();

		return;
	}
	
	
	template<typename Enum>
	static std::string getMenuEntryString(Enum curEntry, const std::unordered_map<Enum, std::string>* pMap, bool isSelected);

	void printMainMenu(action curAction) {
		clearConsole(cursorAfterTargetInfo, cursorAfterSelect);

		printMenuItem("Select action:");

		// print all options starting at the first non exit option
		for (int i = action::EXIT + 1; i < action::MAX_ACTION; i++) {
			printMenuItem(getMenuEntryString(static_cast<action>(i), &actionLabels, i == curAction));
		}

		// print the exit option as last entry
		printMenuItem(getMenuEntryString(action::EXIT, &actionLabels, false));
		std::cout << std::endl;

		return;
	}


	void printHandleCreationMenu(action curAction, handleCreation curHandleCreation) {
		clearConsole(cursorAfterTargetInfo, cursorAfterSelect);

		printMenuItem("Select handle creation (" + actionLabels.at(curAction) + "):");

		for (int i = handleCreation::OPEN_PROCESS; i < handleCreation::MAX_HANDLE_CREATION; i++) {
			printMenuItem(getMenuEntryString(static_cast<handleCreation>(i), &handleCreationLabels, i == curHandleCreation));
		}

		std::cout << std::endl;

		return;
	}


	void printLaunchMethodMenu(action curAction, launchMethod curLaunchMethod) {
		clearConsole(cursorAfterTargetInfo, cursorAfterSelect);

		printMenuItem("Select launch method (" + actionLabels.at(curAction) + "):");

		for (int i = launchMethod::CREATE_THREAD; i < launchMethod::MAX_LAUNCH_METHOD; i++) {
			printMenuItem(getMenuEntryString(static_cast<launchMethod>(i), &launchMethodLabels, i == curLaunchMethod));
		}

		std::cout << std::endl;

		return;
	}


	static int getIntInput();
	
	void selectAction(action* pAction) {
		const int input = getIntInput();

		if (input < 0 || input >= action::MAX_ACTION) return;

		*pAction = static_cast<action>(input);

		return;
	}


	void selectHandleCreation(handleCreation* pHandleCreation) {
		const int input = getIntInput();

		if (input < 0 || input >= handleCreation::MAX_HANDLE_CREATION) return;

		*pHandleCreation = static_cast<handleCreation>(input);

		return;
	}


	void selectLaunchMethod(launchMethod* pLaunchMethod) {
		const int input = getIntInput();

		if (input < 0 || input >= launchMethod::MAX_LAUNCH_METHOD) return;

		*pLaunchMethod = static_cast<launchMethod>(input);

		return;
	}


	static void getTargetInput(std::string info, std::string* pTargetInfo);

	void selectTargets(std::string* pProcName, std::string* pDllName, std::string* pDllDir) {
		clearConsole(cursorAfterHeader, cursorAfterTargetInfo);

		getTargetInput(PROCESS_LABEL, pProcName);
		getTargetInput(DLL_LABEL, pDllName);
		getTargetInput(PATH_LABEL, pDllDir);

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


	void printWinError(std::string msg) {
		std::string winError = " Error code: " + std::to_string(GetLastError());
		printPlainError(msg + winError);

		return;
	}


	void printFileError(std::string msg, errno_t errNo) {
		char errStr[0x100]{};
		strerror_s(errStr, 0x100, errNo);

		std::string fileError = " ";
		fileError.append(errStr);
		printPlainError(msg + fileError);

		return;
	}


	static void printLogText(std::string msg, const char* prefix, WORD attribute);

	void printPlainError(std::string msg) {
		printLogText(msg, ERROR_PREFIX, FOREGROUND_RED);

		return;
	}


	void printInfo(std::string msg) {
		printLogText(msg, INFO_PREFIX, 0);

		return;
	}


	void printSuccess(std::string msg) {
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


	void printInjectionResult(action curAction, launchMethod curLaunchMethod, bool success) {
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
	static std::string getMenuEntryString(Enum curEntry, const std::unordered_map<Enum, std::string>* pMap, bool isSelected) {
		std::string strNum = std::to_string(curEntry);
		std::string padding(4 - strNum.length(), ' ');
		std::string entryString = padding + strNum + "  " + pMap->at(curEntry);

		// number gets put in brackets [] if the action is selected
		if (isSelected) {
			entryString.replace(3 - strNum.length(), 1, "[");
			entryString.replace(4, 1, "]");
		}

		return entryString;
	}


	static void printMenuItem(std::string msg) {
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

	
	static void getTargetInput(std::string label, std::string* pTargetInfo) {
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
				*pTargetInfo = input;
			}
		} while (pTargetInfo->empty());

		SetConsoleCursorPosition(hStdOut, cursorCur);
		printMenuItem(label + *pTargetInfo);

		return;
	}


	static void printLogText(std::string msg, const char* prefix, WORD attribute) {
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