#pragma once
#include <hax.h>

namespace manMap {

	// Performs a DLL injection by manually mapping the module into the target process.
	// Relocation, fixing the import address table and tls callback execution is done by shell code injected into the target process.
	// The shell code still calls Loadlibrary and GetProcAddress to fix the IAT.
	// 
	// Parameters:
	// 
	// [in] hProc:
	// Handle to the target process.
	// Needs at least PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
	// 
	// [in] dllPath:
	// Path of the DLL file to inject.
	// 
	// [in] pLaunchFunc:
	// Function that should launch code execution of the shell code in the target process.
	// 
	// Return:
	// True on success, false on failure
	bool inject(HANDLE hProc, const char* dllPath, launch::tLaunchFunc pLaunchFunc);

}