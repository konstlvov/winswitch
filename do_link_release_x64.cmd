rem release x64 link : run after do_compile_release_x64.cmd
rem keep in sync with TaskSwitchXP.vcxproj Release|x64

set PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x64;%PATH%

set LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64

if not exist x64\Release md x64\Release
cd x64\Release
link /NOLOGO /OUT:"TaskSwitchXP.exe" /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /DYNAMICBASE:NO /MACHINE:X64 /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed /OPT:REF /OPT:ICF /ERRORREPORT:PROMPT kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib TaskSwitchXP.obj generic.obj lang.obj main.obj tscontrol.obj TaskSwitchXP.res
