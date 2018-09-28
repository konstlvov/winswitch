rem INCLUDE=E:\Program Files\Microsoft Visual Studio 8\VC\include;E:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Include
rem LIB=E:\Program Files\Microsoft Visual Studio 8\VC\lib;E:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Lib
rem todo : create makefile
if not exist Debug md Debug
cd Debug
link /VERBOSE:LIB /OUT:"TaskSwitchXP.exe" /INCREMENTAL /DEBUG /PDB:"TaskSwitchXP.pdb" /MAP /SUBSYSTEM:WINDOWS /DYNAMICBASE:NO /MACHINE:X86 /ERRORREPORT:PROMPT kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib TaskSwitchXP.obj generic.obj lang.obj main.obj tscontrol.obj TaskSwitchXP.res
