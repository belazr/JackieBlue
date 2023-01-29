#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>

// Handles console output and user input.

namespace io {

	// All possible actions that can be selected by the user.
	enum action { EXIT, LOAD_LIB, MAN_MAP, UNLINK, CHANGE_TARGETS, ACTION_COUNT };

	// The labels for the actions.
	extern std::unordered_map<action, std::string> actionLabels;

	// Sets the handle of the console to be written to.
	void setConsoleHandle();

	// Prints the header.
	void printHeader();
	
	// Prints the Menu.
	// 
	// Parameters:
	// 
	// [in] procName:
	// Image name of the selected process.
	// 
	// [in] dllName:
	// Image name of the selected DLL.
	// 
	// [in] dllDir:
	// DLL parent directory.
	// 
	// [in] select:
	// Action currently selected. Number is printed in brackets: " [2] label".
	void printMenu(std::string procName, std::string dllName, std::string dllDir, action select);
	
	// Clears the menu.
	void clearMenu();

	// Lets the user select an action to be executed.
	// 
	// Parameters:
	// 
	// [in/out] pSelect:
	// Action currently selected. On valid user input this is the new action selected. For invalid input it keeps the value.
	void selectAction(action* pSelect);

	// Lets the user select the targets.
	// 
	// Parameters:
	// 
	// [in/out] pProcName:
	// Process image name currently selected. On user input this is the new process image name entered.
	// If the user presses enter without entering anythin it keeps the value.
	// 
	// [in/out] pDllName:
	// Dll image name currently selected. On user input this is the new dll image name entered.
	// If the user presses enter without entering anythin it keeps the value.
	// 
	// [in/out] pDllDir:
	// DLL parent directory currently selected. On user input this is the new DLL parent directory entered.
	// If the user presses enter without entering anythin it keeps the value.
	void selectTargets(std::string* pProcName, std::string* pDllName, std::string* pDllDir);

	// Prints  seperator for the logging section.
	void printLogSeparator();

	// Prints an error message in red to the logging section including the last Win32-API error retrieved by GetLastError.
	// 
	// Parameters:
	// [in] msg: Error message to be printed.
	void printWinError(std::string msg);

	// Prints an error message in red to the logging section including the last error of the hax::FileLoader class parsed as string.
	// 
	// Parameters:
	// 
	// [in] msg:
	// Error message to be printed.
	// 
	// [in] errNo:
	// The error number retrieved by hax::FileLoader::getError()
	void printFileError(std::string msg, errno_t errNo);

	// Prints an error message in red to the logging section.
	// 
	// Parameters:
	// 
	// [in] msg:
	// Error message to be printed.
	void printPlainError(std::string msg);

	// Prints an info message in white to the logging section.
	// 
	// Parameters:
	// 
	// [in] msg:
	// Info message to be printed.
	void printInfo(std::string msg);

	// Clears the logging section.
	void clearLog();
}