# JackieBlue
JackieBlue is a basic DLL injector made with the EasyWinHax library.

It supports the injection methods:
- LoadLibraryA
- Manual mapping via shell code

and the following methods to launch the injection code in the target process:
- Create thread
- Hijack thread
- Set windows hook
- Hook NtUserBeginPaint
- Queue user APC

For further details regarding the launch methods see the included header files of the EasyWinHax library.

It is also able to unlink a module entry from a processes loader data table, so it is harder to detect the injected DLL when using LoadLibrary injections.
For manual mapping no loader data table entry is created anyway.

Further it is able to duplicate a handle to the target process from another process instead of creating a new process handle.
This option is available for both injection methods and module unlinking.

The x64 binary is able to handle x86 as well as x64 targets.

This project was created for personal use, learning purposes and fun.
# Screenshot
![alt text](https://github.com/belazr/JackieBlue/blob/master/res/screenshot.png?raw=true)

## Requirements
- Windows 10/11 64bit
- Visual Studio
- C++ 14 compliant compiler
- Windows SDK
- EasyWinHax library (included)

Build tested with:
- Windows 11 64bit
- Visual Studio 18
- MSVC v145
- Windows 11 SDK (10.0.26200)

Usage tested with:
- Windows 11 64bit

## Build
Open the solution file (JackieBlue.sln) with Visual Studio and run the desired builds from there.
By default an executable with static runtime library linkage (/MT and /MTd) is built, so it is completely protable.

## Usage
The x64 build supports both x86 as well as x64 targets.

Launch the binary and enter the process name of the target process, the DLL file name you want to inject and the directory where the DLL is located.

Select an action by entering the item number from to menu. Press enter.
If no number is entered the action marked in brackets is selected.

If a DLL injection (LoadLibraryA or manual mapping) was selected, the menu to select the launch method is displayed.
Select a launch method by entering the item number from the menu. Press enter.
If no number is entered the launch method marked in brackets is selected.
"Set windows hook" and "Hook NtUserBeginPaint" will not work on console applications.
"Set windows hook" will only work if injector and target process architectures match.

Now the menu to select how to create a handle to the target process is shown.
Select a creation method by entering the item number from the menu. Press enter.

If there are multiple processes with the same name, another menu to select the target process ID is displayed.
Again, select the process ID by entering the item number from the menu. Press enter.

Now the selected action is performed on the selected process and the status is displayed in the logging section.

The binary can also be launched from the command line setting the target, DLL and path like this:

```
JackieBlue.exe process.exe payload.dll c:\path
```

### Proxy
In some cases the injector cannot obtain a process handle with the access rights needed to inject a DLL, while other processes running on the system can.
Those processes can act as proxies: build the injector as a DLL using the Project "Proxy".
When "Proxy.dll" is injected into a proxy process, it spawns a full instance of the injector inside that process.
This instance can be used the same way as the executable.

## Known issues
Resizing the console window to a width smaller than the header breaks the menu.
Restarting the program is required for proper display.

## TODOs
- Add more injection methods
- Use virtual console sequences for printing (maybe)
- GUI (some day)

## References
- https://github.com/belazr/EasyWinHax/
- https://guidedhacking.com/
- https://learn.microsoft.com/en-us/windows/win32/api/
