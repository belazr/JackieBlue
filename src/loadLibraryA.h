#pragma once
#include <hax.h>

namespace loadLib {

	// Performs a DLL injection via calling LoadLibraryA in the context of the target process.
	// 
	// Parameters:
	// 
	// [in] hProc:
	// Handle to the target process.
	// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, PROCESS_VM_READ access rights and the additional access rights required by the launch function.
	// 
	// [in] dllPath:
	// Path of the DLL file to inject.
	// 
	// [in] pLaunchFunc:
	// Function that should launch code execution of LoadLibraryA in the target process.
	// 
	// Return:
	// True on success, false on failure
	bool inject(HANDLE hProc, const char* dllPath, launch::tLaunchFunc pLaunchFunc);

}