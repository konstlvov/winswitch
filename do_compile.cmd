rem todo : create makefile

set PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x86;C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x86;%PATH%
rem set PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x64;C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64

rem set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um
set INCLUDE=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt

rem set LIB=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\lib;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\um\x86
set LIB=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\lib

if not exist Debug md Debug
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt generic.cpp
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt lang.cpp
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt main.cpp
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt TaskSwitchXP.cpp
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt tscontrol.cpp
rc.exe /d "_VC80_UPGRADE=0x0710" /d "_UNICODE" /d "UNICODE" /fo"Debug/TaskSwitchXP.res" .\TaskSwitchXP.rc
