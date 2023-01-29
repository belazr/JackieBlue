#include "io.h"
#include <iostream>

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
#define LOG_SEP "------------------------- LOG -------------------------"

namespace io {

	// labels for the actions
	std::unordered_map<action, std::string> actionLabels{
		{ action::LOAD_LIB, "LoadLibraryA injection"},
		{ action::MAN_MAP, "Manual mapping injection"},
		{ action::UNLINK, "Unlink dll"},
		{ action::CHANGE_TARGETS, "Change targets"},
		{ action::EXIT, "Exit"}
	};

	// handle to current console
	HANDLE hConsole;

	// cursor positions relevant for printing static console output
	COORD cursorAfterHeader;
	COORD cursorAfterTargets;
	COORD cursorAfterSelect;
	COORD cursorAfterLog;


	void setConsoleHandle() {
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		return;
	}


	static COORD getCursorPosition();

	void printHeader() {
		std::cout << HEADER << std::endl;

		CONSOLE_SCREEN_BUFFER_INFO screen;
		GetConsoleScreenBufferInfo(hConsole, &screen);
		DWORD written = 0;

		for (SHORT line = 0; line < screen.dwSize.Y; line++) {
			// sets the color of first word of the header to purple
			FillConsoleOutputAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_RED, 54, { 0, line }, &written);
			// sets the color of second word of the header to blue
			FillConsoleOutputAttribute(hConsole, FOREGROUND_BLUE, 34, { 54, line }, &written);
		}

		cursorAfterHeader = getCursorPosition();

		return;
	}
	

	static std::string getActionMenuEntry(action act, bool isSelected);
	static void printMenuItem(std::string item);

	void printMenu(std::string procName, std::string dllName, std::string dllDir, action select) {
		// print target info section
		printMenuItem(PROCESS_LABEL + procName);
		printMenuItem(DLL_LABEL + dllName);
		printMenuItem(PATH_LABEL + dllDir);
		
		cursorAfterTargets = getCursorPosition();

		std::cout << std::endl;

		// print options section
		printMenuItem("Select action:");

		// print all options starting at the first non exit option
		for (int i = action::EXIT + 1; i < action::ACTION_COUNT; i++) {
			printMenuItem(getActionMenuEntry(static_cast<action>(i), i == select));
		}

		// print the exit option as last entry
		printMenuItem(getActionMenuEntry(action::EXIT, false));
		std::cout << std::endl;

		return;
	}


	static void clearConsole(COORD from, COORD to);

	void clearMenu() {
		clearConsole(cursorAfterHeader, cursorAfterSelect);

		return;
	}


	void selectAction(action* pSelect) {
		std::string strInput;
		std::getline(std::cin, strInput);
		std::cout << std::endl;

		cursorAfterSelect = io::getCursorPosition();

		// if input is empty or not a number return without setting new selected action
		if (strInput == "" || strInput.find_first_not_of("1234567890") != std::string::npos) {

			return;
		}

		int intInput = std::stoi(strInput);

		// if input is higher option than available return without setting new selected action
		if (intInput >= action::ACTION_COUNT) {

			return;
		}

		*pSelect = static_cast<action>(intInput);

		return;
	}


	static void getTargetInput(std::string info, std::string* pTargetInfo);

	void selectTargets(std::string* pProcName, std::string* pDllName, std::string* pDllDir) {
		COORD curCursorPos = getCursorPosition();
		SetConsoleCursorPosition(hConsole, cursorAfterHeader);
		// clears target info section
		clearConsole(cursorAfterHeader, cursorAfterTargets);

		getTargetInput(PROCESS_LABEL, pProcName);
		getTargetInput(DLL_LABEL, pDllName);
		getTargetInput(PATH_LABEL, pDllDir);

		SetConsoleCursorPosition(hConsole, curCursorPos);

		return;
	}


	void printLogSeparator() {
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


	void printPlainError(std::string msg) {
		COORD start = getCursorPosition();

		std::cout << ERROR_PREFIX << msg;

		COORD end = getCursorPosition();

		// sets the color of the error message to red
		DWORD written = 0;
		FillConsoleOutputAttribute(hConsole, FOREGROUND_RED, end.X - start.X, start, &written);

		std::cout << std::endl;

		// keep track of the cursor position after the logging section
		cursorAfterLog = getCursorPosition();

		return;
	}


	void printInfo(std::string msg) {
		std::cout << INFO_PREFIX << msg << std::endl;

		// keep track of the cursor position after the logging section
		cursorAfterLog = getCursorPosition();

		return;
	}


	void clearLog() {
		clearConsole(cursorAfterSelect, cursorAfterLog);

		return;
	}


	static COORD getCursorPosition() {
		CONSOLE_SCREEN_BUFFER_INFO screen{};
		GetConsoleScreenBufferInfo(hConsole, &screen);

		return screen.dwCursorPosition;
	}


	static void clearConsole(COORD from, COORD to) {
		CONSOLE_SCREEN_BUFFER_INFO screen{};
		GetConsoleScreenBufferInfo(hConsole, &screen);

		DWORD start = from.Y * screen.dwSize.X + from.X;
		DWORD end = to.Y * screen.dwSize.X + to.X;

		if (start > end) return;

		DWORD toFill = end - start;
		DWORD written = 0;
		FillConsoleOutputCharacterA(hConsole, ' ', toFill, from, &written);

		SetConsoleCursorPosition(hConsole, from);

		return;
	}


	static std::string getActionMenuEntry(action act, bool isSelected) {
		std::string strNum = std::to_string(act);
		std::string padding(4 - strNum.length(), ' ');
		std::string actionString = padding + strNum + "  " + actionLabels.at(act);

		// number gets put in brackets [] if the action is selected
		if (isSelected) {
			actionString.replace(3 - strNum.length(), 1, "[");
			actionString.replace(4, 1, "]");
		}

		return actionString;
	}

	static void printMenuItem(std::string msg) {
		std::cout << MENU_PREFIX << msg << std::endl;

		return;
	}

	
	static void getTargetInput(std::string label, std::string* pTargetInfo) {
		COORD cursorCurrent = getCursorPosition();

		// gets the targer info from user input until set
		// keeps value if already set and user input is empty
		do {
			SetConsoleCursorPosition(hConsole, cursorCurrent);

			std::string input;
			std::cout << MENU_PREFIX << label;
			std::getline(std::cin, input);

			// current value is overwritten if users input is not empty
			if (!input.empty()) {
				*pTargetInfo = input;
			}
		} while (pTargetInfo->empty());

		SetConsoleCursorPosition(hConsole, cursorCurrent);
		printMenuItem(label + *pTargetInfo);

		return;
	}

}