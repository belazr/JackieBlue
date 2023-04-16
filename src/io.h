#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>

// Handles console output and user input.

namespace io {

	// options for user selection
	enum action { EXIT = 0, LOAD_LIB, MAN_MAP, UNLINK, CHANGE_TARGETS, MAX_ACTION };
	enum launchMethod { CREATE_THREAD = 1, HIJACK_THREAD, SET_WINDOWS_HOOK, HOOK_BEGIN_PAINT, QUEUE_USER_APC, MAX_LAUNCH_METHOD };
	enum handleCreation { OPEN_PROCESS = 1, DUPLICATE_HANDLE, MAX_HANDLE_CREATION };

	// Sets the console output handle to std out.
	void init();

	// Prints the header.
	void printHeader();


	// Prints the target info.
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
	void printTargetInfo(std::string procName, std::string dllName, std::string dllDir);
	
	// Prints the main menu.
	// 
	// Parameters:
	// 
	// [in] curAction:
	// Action currently selected. Number is printed in brackets: " [2] label".
	void printMainMenu(action action);

	// Prints the sub menu to select the handle creation.
	// 
	// Parameters:
	// 
	// [in] curAction:
	// Action currently selected.
	// 
	// [in] curHandleCreation:
	// Handle creation currently selected. Number is printed in brackets: " [2] label".
	void printHandleCreationMenu(action curAction, handleCreation curHandleCreation);

	// Prints the sub menu to select the launch method.
	// 
	// Parameters:
	// 
	// [in] curAction:
	// Action currently selected.
	// 
	// [in] curLaunchMethod:
	// Launch method currently selected. Number is printed in brackets: " [2] label".
	void printLaunchMethodMenu(action curAction, launchMethod curLaunchMethod);

	// Lets the user select the action to be executed.
	// 
	// Parameters:
	// 
	// [in/out] pAction:
	// Action currently selected. Only overwritten for valid user input. For invalid input it keeps its value.
	void selectAction(action* pAction);

	// Lets the user select the handle creation.
	// 
	// Parameters:
	// 
	// [in/out] pHandleCreation:
	// Handle creation currently selected. Only overwritten for valid user input. For invalid input it keeps its value.
	void selectHandleCreation(handleCreation* pHandleCreation);

	// Lets the user select the launch method of the code to execute the injection.
	// 
	// Parameters:
	// 
	// [in/out] pSelect:
	// Launch method currently selected. Only overwritten for valid user input. For invalid input it keeps its value.
	void selectLaunchMethod(launchMethod* pLaunchMethod);

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

	// Clears the logging section and prints a seperator.
	void initLog();

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


	// Prints the result message for an action.
	// 
	// Parameters:
	//
	// [in] curAction:
	// The executed action
	// [in] curLaunchMethod:
	// The launch method used for the action if applicable.
	void printInjectionResult(action curAction, launchMethod curLaunchMethod, bool success);

	
	// Formats a pointer to a string with format 0xXXXXXXXX...
	//
	// Parameters:
	//
	// [in] ptr:
	// Pointer that should be formatted for output.
	//
	// Return:
	// The pointer value as a string of format  0xXXXXXXXX...
	std::string formatPointer(uintptr_t ptr);

}