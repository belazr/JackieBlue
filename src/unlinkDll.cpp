#include "unlinkDll.h"
#include "io.h"
#include <hax.h>

namespace ldr {

    template <typename LDTE>
    static bool unlinkLdrEntry(HANDLE hProc, const LDTE* pLdrEntry);
    
    bool unlinkModule(HANDLE hProc, const char* dllName) {
        BOOL isWow64 = FALSE;
        IsWow64Process(hProc, &isWow64);

        if (isWow64) {
            const LDR_DATA_TABLE_ENTRY32* const pLdrEntry32 = proc::ex::getLdrDataTableEntry32Address(hProc, dllName);

            return unlinkLdrEntry(hProc, pLdrEntry32);
        }
        else {

            #ifdef _WIN64

            const LDR_DATA_TABLE_ENTRY64* const pLdrEntry64 = proc::ex::getLdrDataTableEntry64Address(hProc, dllName);

            return unlinkLdrEntry(hProc, pLdrEntry64);

            #else

            io::printPlainError("Can not unlink dll from x64 process. Please use the x64 binary.");

            return false;

            #endif // _WIN64

        }

    }


    template <typename LDTE>
    static bool unlinkLdrEntry(HANDLE hProc, const LDTE* pLdrEntry) {

        if (!pLdrEntry) {
            io::printWinError("Loader data table entry not found.");

                return false;
        }
        else {
            io::printInfo("Dll found in target process.");
        }
        
        LDTE ldrEntry{};

        if (!ReadProcessMemory(hProc, pLdrEntry, &ldrEntry, sizeof(LDTE), nullptr)) {
            io::printWinError("Could not read loader data table entry from target.");

            return false;
        }

        if (!mem::ex::unlinkListEntry(hProc, ldrEntry.InMemoryOrderLinks)) {
            io::printPlainError("Could not unlink InMemoryOrder links.");
            
            return false;
        }

        if (!mem::ex::unlinkListEntry(hProc, ldrEntry.InLoadOrderLinks)) {
            io::printPlainError("Could not unlink InLoadOrder links.");

            return false;
        }

        if (!mem::ex::unlinkListEntry(hProc, ldrEntry.InInitializationOrderLinks)) {
            io::printPlainError("Could not unlink InInitializationOrder links.");

            return false;
        }

        return true;
    }

}