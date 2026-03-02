rem todo : create makefile

set PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x86;%PATH%
rem set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um
rem set INCLUDE=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt

set LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x86;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\lib\x86;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x86

if not exist Debug md Debug
cd Debug
link /VERBOSE:LIB /OUT:"TaskSwitchXP.exe" /INCREMENTAL /DEBUG /PDB:"TaskSwitchXP.pdb" /MAP /SUBSYSTEM:WINDOWS /DYNAMICBASE:NO /MACHINE:X86 /ERRORREPORT:PROMPT kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib TaskSwitchXP.obj generic.obj lang.obj main.obj tscontrol.obj TaskSwitchXP.res
