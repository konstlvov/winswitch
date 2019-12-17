// stdafx.h


//#define _WIN32_WINNT 0x0501 // this worked for WinXP
#define _WIN32_WINNT 0x0600 // this requires Windows Vista minimum
#define _WIN32_IE 0x0600

#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#ifndef _UNICODE
#define _UNICODE
#endif // _UNICODE

#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include <stdio.h>

#pragma warning(disable:4725) // __asm fdiv [mem]

#include <crtdbg.h>


#define chSTR2(x)			#x
#define chSTR(x)			chSTR2(x)
#define FIX_LATER(desc)		message(__FILE__ "(" chSTR(__LINE__) "):" #desc)

#define ABS(x)				((x<0)?(-x):(x))
#define MIN(x,y)			((x<y)?(x):(y))
#define MAX(x,y)			((x>y)?(x):(y))
#define SIZEOF_ARRAY(p)		(sizeof(p)/sizeof(p[0]))
