#pragma once
#include <hax.h>
#include <Windows.h>
#include <unordered_map>
#include <string>

// Handles console output and user input.

namespace io {

	// options for user selection
	typedef enum class Action { EXIT = 0, LOAD_LIB, MAN_MAP, UNLINK, CHANGE_TARGETS, MAX_VAL }Action;
	typedef enum class LaunchMethod { CREATE_THREAD = 1, HIJACK_THREAD, SET_WINDOWS_HOOK, HOOK_BEGIN_PAINT, QUEUE_USER_APC, MAX_VAL }LaunchMethod;
	typedef enum class HandleCreation { OPEN_PROCESS = 1, DUPLICATE_HANDLE, MAX_VAL }HandleCreation;

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
	void printTargetInfo(const std::string& procName, const std::string& dllName, const std::string& dllDir);
	
	// Prints the main menu.
	// 
	// Parameters:
	// 
	// [in] curAction:
	// Action currently selected. Number is printed in brackets: " [2] label".
	void printMainMenu(Action action);

	// Prints the sub menu to select the launch method.
	// 
	// Parameters:
	// 
	// [in] curAction:
	// Action currently selected.
	// 
	// [in] curLaunchMethod:
	// Launch method currently selected. Number is printed in brackets: " [2] label".
	void printLaunchMethodMenu(Action curAction, LaunchMethod curLaunchMethod);

	// Prints the sub menu to select the handle creation.
	// 
	// Parameters:
	// 
	// [in] curAction:
	// Action currently selected.
	// 
	// [in] curHandleCreation:
	// Handle creation currently selected. Number is printed in brackets: " [2] label".
	void printHandleCreationMenu(Action curAction, HandleCreation curHandleCreation);

	// Prints the sub menu to select the process ID.
	// 
	// Parameters:
	// 
	// [in] procIds:
	// List with all process IDs that are offered as a selection.
	void printProcessIdMenu(const std::vector<DWORD>& procIds);

	// Lets the user select the action to be executed.
	// 
	// Parameters:
	// 
	// [in] current:
	// Enum currently selected.
	//
	// Return:
	// Enum selected by user or current enum for invalid input.
	template<typename Enum>
	Enum selectEnum(Enum current);

	template Action selectEnum<Action>(Action current);
	template LaunchMethod selectEnum<LaunchMethod>(LaunchMethod current);
	template HandleCreation selectEnum<HandleCreation>(HandleCreation current);

	// Lets the user select the target process ID.
	// 
	// Parameters:
	// 
	// [in] current:
	// Process ID currently selected.
	//
	// Return:
	// Process ID selected by user or current process ID creation for invalid input.
	size_t selectProcessIdIndex(size_t current);

	// Lets the user select the targets.
	// 
	// Parameters:
	// 
	// [in/out] procName:
	// Process image name currently selected. On user input this is the new process image name entered.
	// If the user presses enter without entering anythin it keeps the value.
	// 
	// [in/out] dllName:
	// Dll image name currently selected. On user input this is the new dll image name entered.
	// If the user presses enter without entering anythin it keeps the value.
	// 
	// [in/out] dllDir:
	// DLL parent directory currently selected. On user input this is the new DLL parent directory entered.
	// If the user presses enter without entering anythin it keeps the value.
	void selectTargets(std::string& pProcName, std::string& pDllName, std::string& pDllDir);

	// Clears the logging section and prints a seperator.
	void initLog();

	// Prints an error message in red to the logging section including the last Win32-API error retrieved by GetLastError.
	// 
	// Parameters:
	// [in] msg: Error message to be printed.
	//
	// [in] winError: The WinError code that should be printed. If ERROR_SUCCESS is passed the function calls GetLastError() itself.
	void printWinError(const std::string& msg, DWORD winError = ERROR_SUCCESS);

	// Prints the code launch error message in red to the logging section including the status code returned by a launch function.
	// 
	// Parameters:
	// 
	// [in] status:
	// The status returned by a launch function.
	void printLaunchError(hax::launch::Status status);

	// Prints an error message in red to the logging section.
	// 
	// Parameters:
	// 
	// [in] msg:
	// Error message to be printed.
	void printPlainError(const std::string& msg);

	// Prints an info message in white to the logging section.
	// 
	// Parameters:
	// 
	// [in] msg:
	// Info message to be printed.
	void printInfo(const std::string& msg);

	// Prints a success message in green to the logging section.
	// 
	// Parameters:
	// 
	// [in] msg:
	// Success message to be printed.
	void printSuccess(const std::string& msg);


	// Prints the result message for an injection action.
	// 
	// Parameters:
	//
	// [in] curAction:
	// The executed injection action.
	// [in] curLaunchMethod:
	// The launch method used for the injection.
	// [in]
	// Indicates wether the injection was successful.
	void printInjectionResult(Action curAction, LaunchMethod curLaunchMethod, bool success);


	// Prints the result message for an unliking action.
	// 
	// Parameters:
	//
	// [in]
	// Indicates wether the unlinking was successful.
	void printUnlinkResult(bool success);

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