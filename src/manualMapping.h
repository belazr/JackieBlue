#pragma once
#include <Windows.h>

namespace manMap {

	// Performs a dll injection by manually mapping the module to the target process.
	// Relocation, fixing the import address table and tls callback execution is done by shell code injected into the target process and started by CreateRemoteThread.
	// The shell code still calls Loadlibrary and GetProcAddress to fix the IAT.
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