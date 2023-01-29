#pragma once
#include <Windows.h>

namespace ldr {

	// Unlinks a module from the loader data table of a process.
	//
	// Parameter:
	// 
	// [in] hProc:
	// Handle to the target process.
	// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_READ and PROCESS_VM_WRITE access rights
	// 
	// [in] modName:
	// Name of the module which loader data table entry should be unlinked.
	// 
	// Return:
	// True on success, false on failure
	bool unlinkModule(HANDLE hProc, const char* modName);

}
