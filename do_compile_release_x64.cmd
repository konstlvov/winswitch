rem release x64 build : run from repository root, then run do_link_release_x64.cmd
rem keep in sync with TaskSwitchXP.vcxproj Release|x64

set PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x64;C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64;%PATH%

set INCLUDE=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt

set LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64

if not exist x64\Release md x64\Release
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /W4 /nologo /c /TP /errorReport:prompt /Fo"x64\Release\\" /Fd"x64\Release\vc143.pdb" generic.cpp
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /W4 /nologo /c /TP /errorReport:prompt /Fo"x64\Release\\" /Fd"x64\Release\vc143.pdb" lang.cpp
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /W4 /nologo /c /TP /errorReport:prompt /Fo"x64\Release\\" /Fd"x64\Release\vc143.pdb" main.cpp
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /W4 /nologo /c /TP /errorReport:prompt /Fo"x64\Release\\" /Fd"x64\Release\vc143.pdb" TaskSwitchXP.cpp
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /W4 /nologo /c /TP /errorReport:prompt /Fo"x64\Release\\" /Fd"x64\Release\vc143.pdb" tscontrol.cpp
rc.exe /d "_VC80_UPGRADE=0x0710" /d "_UNICODE" /d "UNICODE" /fo"x64\Release\TaskSwitchXP.res" .\TaskSwitchXP.rc
