#pragma once
#include <Windows.h>

// Functions to launch code execution in an external target process.
// x64 compilations of these functions (except setWindowsHook) can launch code in x64 and x86 targets.
// x86 compilations of these functions can only launch code in x86 targets.
// setWindowsHook can only launch code in targets with matching architechure.

namespace launch {

	typedef void* (WINAPI* tLaunchFunc)(void* pArg);

	// Launches code execution by creating a thread in the target process via CreateRemoteThread.
	// Waits for the thread and retrives the return value. Can retrive 8 byte return values for x64 targets (unlike GetExitCodeThread).
	// 
	// Parameters:
	// 
	// [in] hProc:
	// Handle to the process where the code should be launched.
	// Needs at least PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
	// 
	// [in] pFunc:
	// Pointer to the code that should be executed by the new thread. Assumes the calling conventions:
	// x86: __stdcall void* pFunc(void* pArg)
	// x64: __fastcall void* pFunc(void* pArg)
	// Additional arguments can be passed in a struct pointed to by pArg.
	// To call api functions with different calling conventions shell code can be used as a wrapper.
	// 
	// [in] pArg:
	// Argument of the function called by the thread.
	// 
	// [out] pRet:
	// Pointer for the return value of the function called by the thread.
	// 
	// Return:
	// True on success or false on failure.
	bool createRemoteThread(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet);

	// Launches code execution by hijacking an existing thread of the target process.
	// Suspends the thread, switches it's context and resumes it executing the desired code.
	// Waits for successfull execution and retrives the return value.
	// 
	// Parameters:
	// 
	// [in] hProc:
	// Handle to the process where the code should be launched.
	// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
	// 
	// [in] pFunc:
	// Pointer to the code that should be executed by the hijacked thread. Assumes the calling conventions:
	// x86: __stdcall void* pFunc(void* pArg)
	// x64: __fastcall void* pFunc(void* pArg)
	// Additional arguments can be passed in a struct pointed to by pArg.
	// To call api functions with different calling conventions shell code can be used as a wrapper.
	// 
	// [in] pArg:
	// Argument of the function called by the thread.
	// 
	// [out] pRet:
	// Pointer for the return value of the function called by the thread.
	// 
	// Return:
	// True on success or false on failure.
	bool hijackThread(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet);

	// Launches code execution by setting a windows hook.
	// Hooks the window procedure to process messages for a window of the target process.
	// Waits for successfull execution, retrives the return value and unhooks the window procedure.
	// In any case the function to launch is only executed once.
	// Caller and target architechture have to match.
	// Can not be used on console application targets.
	// 
	// Parameters:
	// 
	// [in] hProc:
	// Handle to the process where the code should be launched.
	// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
	// 
	// [in] pFunc:
	// Pointer to the code that should be executed by the hijacked thread. Assumes the calling conventions:
	// x86: __stdcall void* pFunc(void* pArg)
	// x64: __fastcall void* pFunc(void* pArg)
	// Additional arguments can be passed in a struct pointed to by pArg.
	// To call api functions with different calling conventions shell code can be used as a wrapper.
	// 
	// [in] pArg:
	// Argument of the function called by the hook.
	// 
	// [out] pRet:
	// Pointer for the return value of the function called by the hook.
	// 
	// Return:
	// True on success or false on failure.
	bool setWindowsHook(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet);

}
