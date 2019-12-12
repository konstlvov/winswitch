// stdafx.h

#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#ifndef _UNICODE
#define _UNICODE
#endif // _UNICODE

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif // _WIN32_WINNT

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif // _WIN32_IE


#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include <uxtheme.h>
#include <tmschema.h>
#pragma comment(lib, "uxtheme.lib")

#include <shlobj.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <crtdbg.h>

#include <string>
#include <vector>
#include <algorithm>
//#pragma comment(linker, "/NODEFAULTLIB:libcmt.lib")
//#pragma warning(disable: 4127)
using namespace std;
typedef basic_string<WCHAR> stringW;


#include <C:\Program Files\HTML Help Workshop\Include\HtmlHelp.h>
#pragma comment(lib, "C:\\Program Files\\HTML Help Workshop\\Lib\\HtmlHelp.lib")

// 
#define chSTR2(x)			#x
#define chSTR(x)			chSTR2(x)
#define FIX_LATER(desc)		message(__FILE__ "(" chSTR(__LINE__) "):" #desc)

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define __WTIME__ WIDEN(__TIME__)
#define __WDATE__ WIDEN(__DATE__)
#define __WTIMESTAMP__ WIDEN(__TIMESTAMP__)


#define VALUE_INRANGE(val, beg, end)	((val >= beg) && (val <= end))
#define SIZEOF_ARRAY(ar)				(sizeof(ar) / sizeof((ar)[0]))
#define ABS(n)							((n >= 0) ? (n) : (-n))
#define MIN(x,y)						((x<y)?(x):(y))
#define MAX(x,y)						((x>y)?(x):(y))
