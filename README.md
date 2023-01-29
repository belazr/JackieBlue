# JackieBlue
JackieBlue is a basic DLL injector made with the EasyWinHax library.
It supports LoadLibrary injections and manual mapping via shell code with CreateRemoteThread as the launch method.
It is also able to unlink a module entry from a processes loader data table so it is harder to detect the injected DLL when using LoadLibrary injections. For manual mapping no loader data table entry is created anyway.
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
- Visual Studio 17
- MSVC v143
- Windows 11 SDK (10.0.22621)

Usage tested with:
- Windows 10 64bit
- Windows 11 64bit

## Build
Open the solution file (JackieBlue.sln) with Visual Studio and run the desired builds from there.
By default an executable with static runtime library linkage (/MT and /MTd) is built so it is completely protable.

## Usage
The x64 build supports both x86 as well as x64 targets.
Launch the binary and enter the process name of the target process, the DLL file name you want to inject and the directory where the DLL is located. Select an injection method by entering the item numbers from to menu. Press enter.
If no number is entered the method marked in brackets is executed.
The binary can also be launched from the command line setting the target, DLL and path like this:
"JackieBlue.exe process.exe payload.dll c:\path"
An icon for a shortcut is located in the "res" folder.

## Known issues
Resizing the console window to a width smaller than the header breaks the menu.
Restart is required for proper display.

## TODOs
- Add more injection methods
- Add more launch methods
- Use virtual console sequences for printing (maybe)
- GUI (some day)

## References
- https://github.com/belazr/EasyWinHax/
- https://guidedhacking.com/
- https://learn.microsoft.com/en-us/windows/win32/api/