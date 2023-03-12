#pragma once
#include <Windows.h>

namespace loadLib {

	// Performs a dll injection by creating a remote thread in the target process that calls LoadLibrary to load the module.
	// 
	// Parameters:
	// 
	// [in] hProc:
	// Handle to the target process.
	// Needs at least PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
	// 
	// Return:
	// True on success, false on failure
	bool inject(HANDLE hProc, const char* dllPath);

}