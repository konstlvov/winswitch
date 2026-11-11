rem release link : run after do_compile_release.cmd
rem keep in sync with TaskSwitchXP.vcxproj Release|Win32

set PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x86;%PATH%

set LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x86;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\lib\x86;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x86

if not exist Release md Release
cd Release
link /NOLOGO /OUT:"TaskSwitchXP.exe" /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /DYNAMICBASE:NO /MACHINE:X86 /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed /OPT:REF /OPT:ICF /SAFESEH /ERRORREPORT:PROMPT kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib TaskSwitchXP.obj generic.obj lang.obj main.obj tscontrol.obj TaskSwitchXP.res
