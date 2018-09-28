rem INCLUDE=E:\Program Files\Microsoft Visual Studio 8\VC\include;E:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Include
rem LIB=E:\Program Files\Microsoft Visual Studio 8\VC\lib;E:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Lib
rem todo : create makefile
if not exist Debug md Debug
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt generic.cpp
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt lang.cpp
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt main.cpp
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt TaskSwitchXP.cpp
cl.exe /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_VC80_UPGRADE=0x0710" /D "_UNICODE" /D "UNICODE" /Gm /EHsc /RTC1 /MTd /Fo"Debug\\" /Fd"Debug\vc80.pdb" /W3 /nologo /c /Wp64 /ZI /TP /errorReport:prompt tscontrol.cpp
rc.exe /d "_VC80_UPGRADE=0x0710" /d "_UNICODE" /d "UNICODE" /fo"Debug/TaskSwitchXP.res" .\TaskSwitchXP.rc
