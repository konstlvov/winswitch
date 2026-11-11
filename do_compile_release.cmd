rem release build : run from repository root, then run do_link_release.cmd
rem keep in sync with TaskSwitchXP.vcxproj Release|Win32

set PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x86;C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x86;%PATH%

set INCLUDE=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt

set LIB=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\lib

if not exist Release md Release
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /Gz /W4 /nologo /c /TP /errorReport:prompt /Fo"Release\\" /Fd"Release\vc143.pdb" generic.cpp
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /Gz /W4 /nologo /c /TP /errorReport:prompt /Fo"Release\\" /Fd"Release\vc143.pdb" lang.cpp
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /Gz /W4 /nologo /c /TP /errorReport:prompt /Fo"Release\\" /Fd"Release\vc143.pdb" main.cpp
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /Gz /W4 /nologo /c /TP /errorReport:prompt /Fo"Release\\" /Fd"Release\vc143.pdb" TaskSwitchXP.cpp
cl.exe /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /EHsc /MT /GS /Gz /W4 /nologo /c /TP /errorReport:prompt /Fo"Release\\" /Fd"Release\vc143.pdb" tscontrol.cpp
rc.exe /d "_VC80_UPGRADE=0x0710" /d "_UNICODE" /d "UNICODE" /fo"Release/TaskSwitchXP.res" .\TaskSwitchXP.rc
