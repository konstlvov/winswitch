// TaskSwitchXP.cpp

#include "stdafx.h"
#include <tlhelp32.h>
#include "main.h"
#include "lang.h"
#include "generic.h"
#include "tscontrol.h"
#include "TaskSwitchXP.h"
#include "resource.h"
#include "WinBase.h"


#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "version.lib")

//-----------------------------------------------------------
WCHAR tmp[4096] = L"";

WCHAR g_szTypeBuffer[MAX_PATH] = L"";
HTHEME g_htheme					= NULL;
HWND g_hwndTs					= NULL;
HBITMAP g_hbitmapTs				= NULL;
HBITMAP g_hbitmapList			= NULL;
HHOOK g_hhookKb					= NULL;
HANDLE g_hheapWnd				= NULL;
PTASKINFO g_pTs					= NULL;
int g_nTs						= 0;
int g_nFirstTs					= 0;
int g_nCurTs					= -1;
int g_nSelTs					= 0;
DWORD g_dwFlags					= DEFAULT_FLAGS;
DWORD g_dwFlagsEx				= DEFAULT_FLAGSEX;
DWORD g_dwFlagsPv				= DEFAULT_FLAGSPV;
DWORD g_dwFlagsList				= DEFAULT_FLAGSLIST;
DWORD g_dwFlagsPane				= DEFAULT_FLAGSPANE;
DWORD g_dwFadeIn				= DEFAULT_FADEIN;
DWORD g_dwFadeOut				= DEFAULT_FADEOUT;

HFONT g_hfontCap				= NULL;
COLORREF g_crCapText			= DEFAULT_CAPTEXTCOLOR;
DWORD g_dwCapShadow				= DEFAULT_CAPSHADOW;

HFONT g_hfontPane				= NULL;
COLORREF g_crPaneText			= DEFAULT_PANETEXTCOLOR;
DWORD g_dwPaneShadow			= DEFAULT_PANESHADOW;

HFONT g_hfontList				= NULL;
//DWORD g_dwListShadow			= DEFAULT_LISTSHADOW;

int g_nIconsX					= DEFAULT_ICONSX;
int g_nIconsY					= DEFAULT_ICONSY;
DWORD g_dwPvDelay				= DEFAULT_PVDELAY;
DWORD g_dwShowDelay				= DEFAULT_SHOWDELAY;
HWND g_hwndFrgnd				= NULL;
DWORD g_idThreadAttachTo		= 0;
HWND g_hwndInfo					= NULL;
HWND g_hwndShell				= NULL;
HWND g_hwndTaskbar				= NULL;

UINT g_uTrayMenu				= DEFAULT_TRAYMENU;
UINT g_uTrayConfig				= DEFAULT_TRAYCONFIG;
UINT g_uTrayNext				= DEFAULT_TRAYNEXT;
UINT g_uTrayPrev				= DEFAULT_TRAYPREV;


PTSEXCLUSION g_pExcl			= NULL;
int g_nExcl						= 0;

COLORREF g_crText				= DEFAULT_TEXTCOLOR;
COLORREF g_crSelText			= DEFAULT_SELTEXTCOLOR;
COLORREF g_crSel				= DEFAULT_SELCOLOR;
COLORREF g_crMarkText			= DEFAULT_MARKTEXTCOLOR;
COLORREF g_crMark				= DEFAULT_MARKCOLOR;
COLORREF g_crSelMarkText		= DEFAULT_SELMARKTEXTCOLOR;
COLORREF g_crSelMark			= DEFAULT_SELMARKCOLOR;

RECT g_rcTs						= { 0 };
RECT g_rcCap					= { 0 };
RECT g_rcList					= { 0 };
RECT g_rcPv						= { 0 };
RECT g_rcPvEx					= { 0 };
RECT g_rcPane					= { 0 };
RECT g_rcScreen					= { 0 };

WALLPAPER g_Wallpaper			= { -1, -1, { 0, 0 }, { 0, 0, 0, 0 }, L"" };
HBITMAP g_hbitmapWpEx			= NULL;

struct PVDATA {
	HWND hwndOwner;
	HWND phWndPv[MAX_WNDPV];
	int nWndPv;

	DWORD dwFlags;
	HICON hIcon;
};

PVDATA g_pvData = {
	NULL, { NULL }, 0, 0, NULL
};

HBITMAP g_hbitmapPv				= NULL;
HANDLE g_phPv[2]				= { NULL, NULL };
HANDLE &g_hevtPv				= g_phPv[0];
HANDLE &g_hmtxPv				= g_phPv[1];
HANDLE g_hThreadPv				= NULL;
HANDLE g_hThreadTs				= NULL;

#define WM_PVREADY				(WM_USER + 2005)

HBITMAP DrawTaskPreview(const PVDATA* pvData);

DWORD WINAPI PreviewThread(LPVOID) {
	while (WaitForMultipleObjects(2, g_phPv, TRUE, INFINITE) == WAIT_OBJECT_0) {
		if (g_hbitmapPv)
			DeleteObject(g_hbitmapPv);
		g_hbitmapPv = DrawTaskPreview(&g_pvData);
		ReleaseMutex(g_hmtxPv);
		PostMessage(g_hwndTs, WM_PVREADY, 0, 0);
	}
	return(0);
}

BOOL InitPreviewThread() {
    if (g_hThreadPv)
		return(TRUE);
	g_hevtPv = CreateEvent(NULL, FALSE, FALSE, NULL);
	g_hmtxPv = CreateMutex(NULL, FALSE, NULL);

	g_hThreadPv = CreateThread(NULL, 0, PreviewThread, NULL, CREATE_SUSPENDED, NULL);
	if (!g_hThreadPv) 
		return(FALSE);
	
	SetThreadPriority(g_hThreadPv, THREAD_PRIORITY_BELOW_NORMAL);
	ResumeThread(g_hThreadPv);

	return(TRUE);
}

void DestroyPreviewThread() {
	if (g_hThreadPv) {
#pragma FIX_LATER(correctly terminate preview thread)
		TerminateThread(g_hThreadPv, 0);
		g_hThreadPv = NULL;
	}
	if (g_hevtPv) {
		CloseHandle(g_hevtPv);
		g_hevtPv = NULL;
	}
	if (g_hmtxPv) {
		CloseHandle(g_hmtxPv);
		g_hmtxPv = NULL;
	}	
}

//-----------------------------------------------------------

LRESULT CALLBACK AltTabKeyboardProc(int, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelMouseProc(int, WPARAM, LPARAM);
LRESULT CALLBACK TaskSwitchWndProc(HWND, UINT, WPARAM, LPARAM);
//LRESULT CALLBACK TsBkWndProc(HWND, UINT, WPARAM, LPARAM);
void DrawTaskListItem(HDC, int, const RECT*, BOOL);
void DrawItemSelection(HDC hdc, const RECT* prcItem, COLORREF);
BOOL UpdateInfoTip(int);
void DestroyTsBackground();
int _NextTask(int, int, PBOOL);
int MyEnumDesktopWindows();
BOOL UpdateWallpaper();
void KillMsTaskSwitcher();
int CompareExcl(const PVOID, const PVOID);
void FastSwitch(HWND);

//klvov:
void ProcessTypeBuffer(HWND);

//-----------------------------------------------------------
//-----------------------------------------------------------

BOOL InitLanguage(BOOL fFirst) {

	HKEY hkey;
	DWORD cbData, tmp;

	HKEY hkeyRoot = HKEY_CURRENT_USER;
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
		cbData = sizeof(DWORD);
		if (!RegQueryValueEx(hkey, RS_ALLUSERS, 0, NULL, (PBYTE)&tmp, &cbData) && tmp == 1)
			hkeyRoot = HKEY_LOCAL_MACHINE;
		RegCloseKey(hkey);
	}

	BOOL fSuccess = TRUE;	
	if (!RegOpenKeyEx(hkeyRoot, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
		WCHAR szBuff[MAX_DATALEN];
		cbData = MAX_DATALEN * sizeof(WCHAR);
		if (!RegQueryValueEx(hkey, RS_LANGFILE, 0, NULL, (PBYTE)szBuff, &cbData)) {
			if (fFirst) {
				fSuccess = LoadLangFile(szBuff);
			} else UpdateLangFile(szBuff);
		} else LoadLangFile(NULL);
		RegCloseKey(hkey);
	} else 
		LoadLangFile(NULL);
	return(fSuccess);
}

//-----------------------------------------------------------------

void CheckDefaultColors() {
	
	if (g_dwFlagsEx & TSFEX_DEFCAPCOLORS) {
		g_crCapText = GetSysColor(COLOR_CAPTIONTEXT);
		g_dwCapShadow &= 0xff000000;
		g_dwCapShadow |= (~g_crCapText & 0xffffff);
		/*if (IsThemeActive())
			g_dwCapShadow &= ~TSFCS_NODEEP;
		else g_dwCapShadow |= TSFCS_NODEEP;*/
	}

	if (g_dwFlagsEx & TSFEX_DEFPANECOLORS) {
		g_crPaneText = GetSysColor(COLOR_CAPTIONTEXT);
		g_dwPaneShadow &= 0xff000000;
		g_dwPaneShadow |= (~g_crPaneText & 0xffffff);
		/*if (IsThemeActive()) {
			g_crPaneText = g_crCapText;
			g_dwPaneShadow &= ~TSFCS_NODEEP;
		} else {
			g_crPaneText = GetSysColor(COLOR_GRAYTEXT);
			g_dwPaneShadow |= TSFCS_NODEEP;
		}*/
	}

	if (g_dwFlagsEx & TSFEX_DEFLISTCOLORS) {
		g_crText = GetSysColor(COLOR_MENUTEXT);
		g_crSelText = GetSysColor(COLOR_HIGHLIGHTTEXT);
		g_crSel = GetSysColor(COLOR_MENUHILIGHT);
		g_crMarkText = GetSysColor(COLOR_HIGHLIGHTTEXT);
		g_crMark = GetSysColor(COLOR_HIGHLIGHT);
		g_crSelMarkText = GetSysColor(COLOR_HIGHLIGHTTEXT);
		g_crSelMark = GetSysColor(COLOR_HOTLIGHT);
	}
}

// return FALSE - if theme changed
BOOL CheckColorTheme() {

	WCHAR szCurTheme[MAX_PATH], szTheme[MAX_PATH] = L"";
	WCHAR szCurColors[32], szColors[32] = L"";
	if (GetCurrentThemeName(szCurTheme, SIZEOF_ARRAY(szCurTheme), 
		szCurColors, SIZEOF_ARRAY(szCurColors), NULL, 0) != S_OK) {
		szCurTheme[0] = L'\0';
		szCurColors[0] = L'\0';
	}

	HKEY hkey;
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
		DWORD cbData = MAX_PATH * sizeof(WCHAR);
		if (RegQueryValueEx(hkey, RS_THEMEFILE, 0, NULL, (PBYTE)szTheme, &cbData))
			szTheme[0] = L'\0';
		cbData = 32 * sizeof(WCHAR);
		if (RegQueryValueEx(hkey, RS_COLORTHEME, 0, NULL, (PBYTE)szColors, &cbData))
			szColors[0] = L'\0';
		RegCloseKey(hkey);
	}
	if (lstrcmpi(szCurTheme, szTheme) || lstrcmpi(szCurColors, szColors))
		g_dwFlagsEx |= TSEX_DEFCOLORSMASK;

	CheckDefaultColors();

	return(FALSE);
}

//-----------------------------------------------------------------

BOOL LoadSettings() {

	DestroySettings();

	DWORD dwCapFontAttr = DEFAULT_CAPFONTATTR, 
		dwPaneFontAttr = DEFAULT_PANEFONTATTR,
		dwListFontAttr = DEFAULT_LISTFONTATTR, cbData;
	WCHAR szCapFontName[32] = DEFAULT_CAPFONTNAME, 
		szPaneFontName[32] = DEFAULT_PANEFONTNAME,
		szBuff[32] = DEFAULT_LISTFONTNAME;
	HKEY hkey = NULL;
	g_dwFlags = DEFAULT_FLAGS;
	g_dwFlagsEx = DEFAULT_FLAGSEX;
	g_dwFlagsPv = DEFAULT_FLAGSPV;
	g_dwFlagsList = DEFAULT_FLAGSLIST;
	g_dwFlagsPane = DEFAULT_FLAGSPANE;
	g_crCapText = DEFAULT_CAPTEXTCOLOR;
	g_dwCapShadow = DEFAULT_CAPSHADOW;
	g_crPaneText = DEFAULT_PANETEXTCOLOR;
	g_dwPaneShadow = DEFAULT_PANESHADOW;
	g_dwPvDelay = DEFAULT_PVDELAY;
	g_dwFadeIn = DEFAULT_FADEIN;
	g_dwFadeOut = DEFAULT_FADEOUT;
	g_dwShowDelay = DEFAULT_SHOWDELAY;
	DWORD tmp;

	g_uTrayMenu = DEFAULT_TRAYMENU;
	g_uTrayConfig = DEFAULT_TRAYCONFIG;
	g_uTrayNext = DEFAULT_TRAYNEXT;
	g_uTrayPrev = DEFAULT_TRAYPREV;

	g_rcPv.right = DEFAULT_PVWIDTH;
		
	g_rcPane.bottom = DEFAULT_PANEHEIGHT;	
	g_rcList.right = DEFAULT_LISTWIDTH;
	g_rcList.bottom = DEFAULT_LISTHEIGHT;

	// list colors
	g_crText = DEFAULT_TEXTCOLOR;
	g_crSelText = DEFAULT_SELTEXTCOLOR;
	g_crSel = DEFAULT_SELCOLOR;
	g_crMarkText = DEFAULT_MARKTEXTCOLOR;
	g_crMark = DEFAULT_MARKCOLOR;
	g_crSelMarkText = DEFAULT_SELMARKTEXTCOLOR;
	g_crSelMark = DEFAULT_SELMARKCOLOR;
	
	InitLanguage(FALSE);

	HKEY hkeyRoot = HKEY_CURRENT_USER;
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
		cbData = sizeof(DWORD);
		if (!RegQueryValueEx(hkey, RS_ALLUSERS, 0, NULL, (PBYTE)&tmp, &cbData) && tmp == 1)
			hkeyRoot = HKEY_LOCAL_MACHINE;
		RegCloseKey(hkey);
	}

	if (!RegOpenKeyEx(hkeyRoot, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_FLAGS, 0, NULL, (PBYTE)&g_dwFlags, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_FLAGSEX, 0, NULL, (PBYTE)&g_dwFlagsEx, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_FLAGSPV, 0, NULL, (PBYTE)&g_dwFlagsPv, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_FLAGSLIST, 0, NULL, (PBYTE)&g_dwFlagsList, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_FLAGSPANE, 0, NULL, (PBYTE)&g_dwFlagsPane, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_FADEIN, 0, NULL, (PBYTE)&g_dwFadeIn, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_FADEOUT, 0, NULL, (PBYTE)&g_dwFadeOut, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_CAPFONTATTR, 0, NULL, (PBYTE)&dwCapFontAttr, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_CAPTEXTCOLOR, 0, NULL, (PBYTE)&g_crCapText, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_CAPSHADOW, 0, NULL, (PBYTE)&g_dwCapShadow, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_PANEFONTATTR, 0, NULL, (PBYTE)&dwPaneFontAttr, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_PANETEXTCOLOR, 0, NULL, (PBYTE)&g_crPaneText, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_PANESHADOW, 0, NULL, (PBYTE)&g_dwPaneShadow, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_LISTFONTATTR, 0, NULL, (PBYTE)&dwListFontAttr, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_PVDELAY, 0, NULL, (PBYTE)&g_dwPvDelay, &cbData);
	    cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_PVWIDTH, 0, NULL, (PBYTE)&g_rcPv.right, &cbData);
	    cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_LISTWIDTH, 0, NULL, (PBYTE)&g_rcList.right, &cbData);
	    cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_LISTHEIGHT, 0, NULL, (PBYTE)&g_rcList.bottom, &cbData);
	    cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_SHOWDELAY, 0, NULL, (PBYTE)&g_dwShowDelay, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_TEXTCOLOR, 0, NULL, (PBYTE)&g_crText, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_SELTEXTCOLOR, 0, NULL, (PBYTE)&g_crSelText, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_SELCOLOR, 0, NULL, (PBYTE)&g_crSel, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_MARKTEXTCOLOR, 0, NULL, (PBYTE)&g_crMarkText, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_MARKCOLOR, 0, NULL, (PBYTE)&g_crMark, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_SELMARKTEXTCOLOR, 0, NULL, (PBYTE)&g_crSelMarkText, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_SELMARKCOLOR, 0, NULL, (PBYTE)&g_crSelMark, &cbData);

		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_TRAYMENU, 0, NULL, (PBYTE)&g_uTrayMenu, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_TRAYCONFIG, 0, NULL, (PBYTE)&g_uTrayConfig, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_TRAYAPPLIST, 0, NULL, (PBYTE)&g_uTrayNext, &cbData);
		cbData = sizeof(DWORD);
		RegQueryValueEx(hkey, RS_TRAYINSTLIST, 0, NULL, (PBYTE)&g_uTrayPrev, &cbData);	    

		// hotkeys
		if (!RegQueryValueEx(hkey, RS_EXITHK, 0, NULL, (PBYTE)&tmp, &cbData) && LOWORD(tmp) != 0) {
			if (!RegisterHotKey(g_hwndMain, IDH_EXIT, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp))))
				HotKeyError(IDS_ERR_EXITHK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp)));
		}
		if (!RegQueryValueEx(hkey, RS_SHOWHIDEHK, 0, NULL, (PBYTE)&tmp, &cbData) && LOWORD(tmp) != 0) {
			if (!RegisterHotKey(g_hwndMain, IDH_SHOWHIDE, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp))))
				HotKeyError(IDS_ERR_SHOWHIDEHK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp)));
		}
		if (!RegQueryValueEx(hkey, RS_CONFIGHK, 0, NULL, (PBYTE)&tmp, &cbData) && LOWORD(tmp) != 0) {
			if (!RegisterHotKey(g_hwndMain, IDH_CONFIG, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp))))
				HotKeyError(IDS_ERR_CONFIGHK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp)));
		}
		if (!RegQueryValueEx(hkey, RS_APPLISTHK, 0, NULL, (PBYTE)&tmp, &cbData) && LOWORD(tmp) != 0) {
			if (!RegisterHotKey(g_hwndMain, IDH_STNEXTTASK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp))))
				HotKeyError(IDS_ERR_STNEXTTASKHK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp)));
		}
		if (!RegQueryValueEx(hkey, RS_INSTLISTHK, 0, NULL, (PBYTE)&tmp, &cbData) && LOWORD(tmp) != 0) {
			if (!RegisterHotKey(g_hwndMain, IDH_STINEXTTASK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp))))
				HotKeyError(IDS_ERR_STINEXTTASKHK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp)));
		}
		if (!RegQueryValueEx(hkey, RS_MINIMIZETRAYHK, 0, NULL, (PBYTE)&tmp, &cbData) && LOWORD(tmp) != 0) {
			if (!RegisterHotKey(g_hwndMain, IDH_MINIMIZETRAY, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp))))
				HotKeyError(IDS_ERR_MINIMIZETRAYHK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp)));
		}
		if (!RegQueryValueEx(hkey, RS_RESTORETRAYHK, 0, NULL, (PBYTE)&tmp, &cbData) && LOWORD(tmp) != 0) {
			if (!RegisterHotKey(g_hwndMain, IDH_RESTORETRAY, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp))))
				HotKeyError(IDS_ERR_RESTORETRAYHK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp)));
		}		
		//klvov: trying to register hotkey to display alternate task switching window 
		// 
		if (!RegQueryValueEx(hkey, RS_ALTAPPLISTHK, 0, NULL, (PBYTE)&tmp, &cbData) && LOWORD(tmp) != 0) {
			if (!RegisterHotKey(g_hwndMain, IDH_ALTAPPLIST, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp))))
				HotKeyError(IDS_ERR_ALTAPPLISTHK, HIBYTE(LOWORD(tmp)), LOBYTE(LOWORD(tmp)));
		}		
		
		//~klvov

		HKEY hkeyExcl;
		if (RegOpenKeyEx(hkey, RS_EXCLUSIONS_SUBKEY, 0, KEY_READ, &hkeyExcl))
			hkeyExcl = NULL;

		TSEXCLUSION tse;
		cbData = sizeof(DWORD);
		if (hkeyExcl) {
			if (!RegQueryValueEx(hkeyExcl, RS_EXCLCOUNT, 0, NULL, (PBYTE)&tmp, &cbData) && tmp > 0) {

				if (tmp > MAX_EXCLUSIONS)
					tmp = MAX_EXCLUSIONS;
				g_pExcl = (PTSEXCLUSION)HeapAlloc(GetProcessHeap(), 
					HEAP_ZERO_MEMORY, tmp * sizeof(TSEXCLUSION));
				// if (!g_pExcl) error
#pragma FIX_LATER(memory allocation errors)

				for (g_nExcl = 0; g_nExcl < (int)tmp; g_nExcl++) {

					wsprintf(szBuff, L"Flags%u", g_nExcl);
					cbData = sizeof(DWORD);
					if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, 
						(PBYTE)&(g_pExcl[g_nExcl].dwFlags), &cbData)) break;
					if (g_pExcl[g_nExcl].dwFlags & TSEXCLF_DISABLED || 
						!(g_pExcl[g_nExcl].dwFlags & TSEXCLF_BYMASK)) continue;

					g_pExcl[g_nExcl].szExeName[0] = L'\0';
					if (g_pExcl[g_nExcl].dwFlags & TSEXCLF_BYPROCESS) {
						wsprintf(szBuff, L"ExeName%u", g_nExcl);
						cbData = MAX_FILENAME * sizeof(WCHAR);
						if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, 
							(PBYTE)&g_pExcl[g_nExcl].szExeName, &cbData))
							tse.szExeName[0] = L'\0';
					}
					g_pExcl[g_nExcl].szClassName[0] = L'\0';
					if (g_pExcl[g_nExcl].dwFlags & TSEXCLF_BYCLASS) { 
						wsprintf(szBuff, L"ClassName%u", g_nExcl);
						cbData = MAX_CLASSNAME * sizeof(WCHAR);
						if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, 
							(PBYTE)&g_pExcl[g_nExcl].szClassName, &cbData))
							tse.szClassName[0] = L'\0';
					}
					g_pExcl[g_nExcl].szCaption[0] = L'\0';
					if (g_pExcl[g_nExcl].dwFlags & TSEXCLF_BYCAPTION) { 
						wsprintf(szBuff, L"Caption%u", g_nExcl);
						cbData = MAX_CAPTION * sizeof(WCHAR);
						if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, 
							(PBYTE)&g_pExcl[g_nExcl].szCaption, &cbData))
							tse.szCaption[0] = L'\0';
					}
				}
				if (g_nExcl < (int)tmp) {
					if (!g_nExcl) {
						HeapFree(GetProcessHeap(), 0, g_pExcl);
						g_pExcl = NULL;
					} else {
						g_pExcl = (PTSEXCLUSION)HeapReAlloc(GetProcessHeap(), 
							0, g_pExcl, g_nExcl * sizeof(TSEXCLUSION));
						// if (!g_pExcl) error
					}
				}
				if (g_pExcl)
					MyInsertionSort(g_pExcl, g_nExcl, sizeof(TSEXCLUSION), CompareExcl);
			}
			RegCloseKey(hkeyExcl);
		}
		cbData = LF_FACESIZE * sizeof(WCHAR);
		RegQueryValueEx(hkey, RS_CAPFONTNAME, 0, NULL, (PBYTE)szCapFontName, &cbData);
		cbData = LF_FACESIZE * sizeof(WCHAR);
		RegQueryValueEx(hkey, RS_PANEFONTNAME, 0, NULL, (PBYTE)szPaneFontName, &cbData);
		cbData = LF_FACESIZE * sizeof(WCHAR);
		RegQueryValueEx(hkey, RS_LISTFONTNAME, 0, NULL, (PBYTE)szBuff, &cbData);
	    RegCloseKey(hkey);
	}

	// default colors
	if (CheckColorTheme())
		CheckDefaultColors();

	if (g_rcPv.right < MIN_PVWIDTH || g_rcPv.right > MAX_PVWIDTH)
		g_rcPv.right = DEFAULT_PVWIDTH;
	g_rcPane.bottom = HIWORD(g_dwFlagsPane);
	if (g_rcPane.bottom < MIN_PANEHEIGHT || g_rcPane.bottom > MAX_PANEHEIGHT)
		g_rcPane.bottom = DEFAULT_PANEHEIGHT;

	if (g_dwShowDelay < MIN_SHOWDELAY || g_dwShowDelay > MAX_SHOWDELAY)
		g_dwShowDelay = DEFAULT_SHOWDELAY;
	
	// priority
	SetPriorityClass(GetCurrentProcess(), (g_dwFlags & TSF_HIGHPRIORITY) 
		? HIGH_PRIORITY_CLASS : ((g_dwFlags & TSF_ABOVEPRIORITY) 
		? ABOVE_NORMAL_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS));

	// list width
	if (g_rcList.right < MIN_LISTWIDTH || g_rcList.right > MAX_LISTWIDTH)
		g_rcList.right = DEFAULT_LISTWIDTH;

	if (g_dwFlagsList & TSFL_VERTICALLIST)
		g_nIconsX = 1;
	else if (g_dwFlagsList & TSFL_SMALLICONS)
		g_nIconsX = g_rcList.right / (CX_SMICON + 2 * (XMARGIN_SMSEL + 1));
	else g_nIconsX = g_rcList.right / (CX_ICON + 2 * (XMARGIN_SEL + 1));
	g_rcList.right += 2 * XMARGIN_LIST;

	// list height
	if (g_rcList.bottom < MIN_LISTHEIGHT || g_rcList.bottom > MAX_LISTHEIGHT)
		g_rcList.bottom = DEFAULT_LISTHEIGHT;

	if (g_dwFlagsList & TSFL_SMALLICONS)
		g_nIconsY = g_rcList.bottom / (CY_SMICON + 2 * (YMARGIN_SMSEL + 1));
	else g_nIconsY = g_rcList.bottom / (CY_ICON + 2 * (YMARGIN_SEL + 1));
	g_rcList.bottom += 2 * YMARGIN_LIST;

	// calc rect's
	g_rcTs.left = g_rcTs.top = 0;
	g_rcCap.left = g_rcCap.top = 0;
	g_rcPane.left = 0;

	// left-right pane order
	if (g_dwFlagsEx & TSFEX_LEFTRIGHT) {
		g_rcList.left = 0;
		g_rcPv.left = g_rcList.right;
		g_rcTs.right = g_rcPv.right = g_rcPv.left + 2 * XMARGIN_PREVIEW + g_rcPv.right;
	} else {
		g_rcPv.left = 0;
		g_rcList.left = g_rcPv.right = g_rcPv.left + 2 * XMARGIN_PREVIEW + g_rcPv.right;
		g_rcTs.right = g_rcList.right = g_rcList.left + g_rcList.right;
	}
	g_rcCap.right = g_rcPane.right = g_rcTs.right;
	g_rcPv.top = g_rcList.top = g_rcCap.bottom = \
		(g_dwFlagsEx & TSFEX_REMOVECAPTION) ? 0 : ((g_dwFlagsEx & TSFEX_CAPSMALLICON) 
		? (CY_SMICON + 2 * YMARGIN_SMICON) : (CY_ICON + 2 * YMARGIN_ICON));
	g_rcPane.top = g_rcPv.bottom = g_rcList.bottom = g_rcList.top + g_rcList.bottom;
	g_rcTs.bottom = g_rcPane.bottom = (g_dwFlagsEx & TSFEX_REMOVEPANE) ? 
		g_rcPane.top : (g_rcPane.top + g_rcPane.bottom);

	// font
	HDC hdcScreen = GetDC(NULL);
	cbData = GetDeviceCaps(hdcScreen, LOGPIXELSY);
	ReleaseDC(NULL, hdcScreen);
	if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
		g_hfontCap = CreateFont(-MulDiv(dwCapFontAttr & TSFCF_SIZEMASK, cbData, 72), 
			0, 0, 0, (dwCapFontAttr & TSFCF_BOLD) ? FW_BOLD : FW_NORMAL, 
			(dwCapFontAttr & TSFCF_ITALIC) ? TRUE : FALSE, 0, 0, DEFAULT_CHARSET, 
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
			DEFAULT_PITCH | FF_DONTCARE, szCapFontName);
	}
	if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
		g_hfontPane = CreateFont(-MulDiv(dwPaneFontAttr & TSFCF_SIZEMASK, cbData, 72), 
			0, 0, 0, (dwPaneFontAttr & TSFCF_BOLD) ? FW_BOLD : FW_NORMAL, 
			(dwPaneFontAttr & TSFCF_ITALIC) ? TRUE : FALSE, 0, 0, DEFAULT_CHARSET, 
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
			DEFAULT_PITCH | FF_DONTCARE, szPaneFontName);
	}
	if (g_dwFlagsList & TSFL_VERTICALLIST) {
		g_hfontList = CreateFont(-MulDiv(dwListFontAttr & TSFCF_SIZEMASK, cbData, 72), 
			0, 0, 0, (dwListFontAttr & TSFCF_BOLD) ? FW_BOLD : FW_NORMAL, 
			(dwListFontAttr & TSFCF_ITALIC) ? TRUE : FALSE, 0, 0, DEFAULT_CHARSET, 
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
			DEFAULT_PITCH | FF_DONTCARE, szBuff);
	}
    
	/*if (!g_hfontCap) { // get system caption font
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 
			sizeof( NONCLIENTMETRICS), (LPVOID)&ncm, 0);
		g_hfontCap = CreateFontIndirect(&ncm.lfCaptionFont);
	}
	if (!g_hfontCap)
		return(FALSE);*/

	// preview delay
	if (g_dwPvDelay < MIN_PVDELAY || g_dwPvDelay > MAX_PVDELAY) {
		g_dwPvDelay = DEFAULT_PVDELAY;
		/*SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, (LPVOID)(DWORD_PTR)&g_dwPvDelay, 0);
		g_dwPvDelay = (g_dwPvDelay + 1) * 50;*/
	}

	if (g_dwFadeIn & TSFBL_ENABLED && !g_dwShowDelay)
		g_dwShowDelay = 100;

	UpdateWallpaper();

	// register TaskSwitchXP window class
	_ASSERT(g_hwndTs == NULL);
	UnregisterClass(g_szTaskSwitchWnd, g_hinstExe);
	//UnregisterClass(g_szTsBkClass, g_hinstExe);

	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= (g_dwFlags & TSF_DROPSHADOW) 
		? CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW | CS_DBLCLKS : CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)TaskSwitchWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= g_hinstExe;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= g_szTaskSwitchWnd;
	wcex.hIconSm		= NULL;
	if (!RegisterClassEx(&wcex))
		return(FALSE);

	/*wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)TsBkWndProc;
	wcex.lpszClassName	= g_szTsBkClass;
	if (!RegisterClassEx(&wcex))
		return(FALSE);*/

	if (g_dwFlags & TSF_CHECKMSTS)
		KillMsTaskSwitcher();

	ReplaceAltTab(g_dwFlags);

	EnableExtMouse(g_dwFlags & TSF_EXTMOUSE, g_dwFlags & TSF_WHEELTAB);
	ShowTrayIcon(g_dwFlags & TSF_SHOWTRAYICON);
	if (g_dwFlags & TSF_RELOADICONS)
		SetTimer(g_hwndMain, TIMER_RELOADICONS, RELOADICONS_DELAY, NULL);

	return(TRUE);
}


void DestroySettings() {

	if (g_hwndTs)
		DestroyWindow(g_hwndTs);

	ReplaceAltTab(TSF_NOREPLACEALTTAB);
	EnableExtMouse(FALSE, FALSE);

	UnregisterHotKey(g_hwndMain, IDH_STNEXTTASK);
	UnregisterHotKey(g_hwndMain, IDH_STINEXTTASK);
	UnregisterHotKey(g_hwndMain, IDH_EXIT);
	UnregisterHotKey(g_hwndMain, IDH_SHOWHIDE);
	UnregisterHotKey(g_hwndMain, IDH_CONFIG);
	UnregisterHotKey(g_hwndMain, IDH_MINIMIZETRAY);
	UnregisterHotKey(g_hwndMain, IDH_RESTORETRAY);

	g_nExcl = 0;
	if (g_pExcl) {
		HeapFree(GetProcessHeap(), 0, g_pExcl);
		g_pExcl = NULL;
	}
	if (g_hfontCap) {
		DeleteObject(g_hfontCap);
		g_hfontCap = NULL;
	}
	if (g_hfontPane) {
		DeleteObject(g_hfontPane);
		g_hfontPane = NULL;
	}
	if (g_hfontList) {
		DeleteObject(g_hfontList);
		g_hfontList = NULL;
	}
	if (g_hbitmapWpEx) {
		DeleteObject(g_hbitmapWpEx);
		g_hbitmapWpEx = NULL;
	}

	g_Wallpaper.rcWallpaper.left = g_Wallpaper.rcWallpaper.top = \
		g_Wallpaper.rcWallpaper.right = g_Wallpaper.rcWallpaper.bottom = 0;

	KillTimer(g_hwndMain, TIMER_RELOADICONS);	
	ShowTrayIcon(FALSE);	
}

//-----------------------------------------------------------

void ReplaceAltTab(DWORD dwFlags) {

	if (g_hhookKb) {
		UnhookWindowsHookEx(g_hhookKb);
		g_hhookKb = NULL;
	}
	UnregisterHotKey(g_hwndMain, IDH_NEXTTASK);
	UnregisterHotKey(g_hwndMain, IDH_PREVTASK);
	UnregisterHotKey(g_hwndMain, IDH_WINNEXTTASK);
	UnregisterHotKey(g_hwndMain, IDH_WINPREVTASK);
	UnregisterHotKey(g_hwndMain, IDH_INSTNEXT);
	UnregisterHotKey(g_hwndMain, IDH_INSTPREV);

	g_dwFlags |= TSF_NOREPLACEALTTAB;
	g_dwFlags &= ~TSF_INSTSWITCHER;
	if (dwFlags & TSF_HOOKALTTAB)
		g_dwFlags |= TSF_HOOKALTTAB;
	else g_dwFlags &= ~TSF_HOOKALTTAB;

	if (!(dwFlags & TSF_NOREPLACEALTTAB)) {
		if (dwFlags & TSF_HOOKALTTAB) {
			g_hhookKb = SetWindowsHookEx(WH_KEYBOARD_LL, 
				(HOOKPROC)AltTabKeyboardProc, g_hinstExe, NULL);
			if (!g_hhookKb) {
				g_dwFlags &= ~TSF_HOOKALTTAB;
				ReportError(IDS_ERR_KBHOOK);
			} else {
				g_dwFlags &= ~TSF_NOREPLACEALTTAB;
				g_dwFlags |= TSF_HOOKALTTAB;
			}
		} else {
			if (!RegisterHotKey(g_hwndMain, IDH_NEXTTASK, MOD_ALT, VK_TAB) ||
				!RegisterHotKey(g_hwndMain, IDH_PREVTASK, MOD_ALT | MOD_SHIFT, VK_TAB)) {
				ReportError(IDS_ERR_ALTTAB);
			} else {
				g_dwFlags &= ~TSF_NOREPLACEALTTAB;
				RegisterHotKey(g_hwndMain, IDH_WINNEXTTASK, MOD_ALT | MOD_WIN, VK_TAB);
				RegisterHotKey(g_hwndMain, IDH_WINPREVTASK, MOD_ALT | MOD_WIN | MOD_SHIFT, VK_TAB);
			}
		}
	}
	if (dwFlags & TSF_INSTSWITCHER) {
		g_dwFlags |= TSF_INSTSWITCHER;
		if (!(dwFlags & TSF_HOOKALTTAB)) {
			if (!RegisterHotKey(g_hwndMain, IDH_INSTNEXT, MOD_CONTROL | MOD_ALT, VK_TAB) ||
				!RegisterHotKey(g_hwndMain, IDH_INSTPREV, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_TAB)) {
				g_dwFlags &= ~TSF_INSTSWITCHER;
				ReportError(IDS_ERR_INSTSWITCHER);
			}
		} else if (!g_hhookKb) {			
			g_hhookKb = SetWindowsHookEx(WH_KEYBOARD_LL, 
				(HOOKPROC)AltTabKeyboardProc, g_hinstExe, NULL);
			if (!g_hhookKb) {
				g_dwFlags &= ~(TSF_INSTSWITCHER | TSF_HOOKALTTAB);
				ReportError(IDS_ERR_KBHOOK);
			} else {
				g_dwFlags |= TSF_HOOKALTTAB;
			}
		}
	}
}


void KillMsTaskSwitcher() {
	HWND hwndMsTs = FindWindowEx(HWND_MESSAGE, NULL, L"TaskSwitch", L"");
	if (hwndMsTs) {
		if (ConfirmMessage(IDS_CONFIRM_MSTS, MB_ICONEXCLAMATION)) {
			DWORD dwProcessId;
			GetWindowThreadProcessId(hwndMsTs, &dwProcessId);
			MyTerminateProcess(dwProcessId);			
		}
		Sleep(200);
	}	
}

//-----------------------------------------------------------

LRESULT CALLBACK AltTabKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {

	static DWORD s_vkSkip = 0;

	LRESULT lRet = 0;

	if (nCode == HC_ACTION) {
		PKBDLLHOOKSTRUCT pkbhs = (PKBDLLHOOKSTRUCT)lParam;

		if (wParam == WM_SYSKEYDOWN || wParam == WM_KEYDOWN) {
			if (pkbhs->vkCode == VK_TAB) {
				if ((pkbhs->flags & LLKHF_ALTDOWN || 
					(GetAsyncKeyState(VK_MENU) & 0x8000)) /*&& 
					!(GetAsyncKeyState(VK_LWIN) & 0x8000) && 
					!(GetAsyncKeyState(VK_RWIN) & 0x8000)*/) {

					if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
						if (g_dwFlags & TSF_INSTSWITCHER) {
							// Ctrl+Alt+Tab & Ctrl+Alt+Shift+Tab
							UINT uParam = (GetAsyncKeyState(VK_SHIFT) & 0x8000) 
								? IDH_INSTPREV : IDH_INSTNEXT;
							if (g_hwndTs)
								PostMessage(g_hwndTs, WM_TASKSWITCH, uParam, 0);
							else PostMessage(g_hwndMain, WM_HOTKEY, uParam, 0);								
							s_vkSkip = VK_TAB;
							lRet = 1;
						}
					} else if (!(g_dwFlags & TSF_NOREPLACEALTTAB)) {
						// Alt+Tab & Alt+Shift+Tab
						UINT uParam = (GetAsyncKeyState(VK_SHIFT) & 0x8000) 
							? IDH_PREVTASK : IDH_NEXTTASK;
						if (g_hwndTs)
							PostMessage(g_hwndTs, WM_TASKSWITCH, uParam, 0);
						else PostMessage(g_hwndMain, WM_HOTKEY, uParam, 0);
						s_vkSkip = VK_TAB;
						lRet = 1;
					}
				 }
			} else if (pkbhs->vkCode == VK_ESCAPE) {
				 if (g_hwndTs && !(g_dwFlagsList & TSFL_MENUACTIVE)) {
					 PostMessage(g_hwndTs, WM_TASKSWITCH, IDH_ESCAPE, 0);
					 s_vkSkip = VK_ESCAPE;
					 lRet = 1;
				 }
			 }
		 } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {

			switch (pkbhs->vkCode) {

				case VK_MENU:
				case VK_LMENU:
				case VK_RMENU:
					if (g_hwndTs && !(g_dwFlags & TSF_STICKYALTTAB) && \
						!(GetAsyncKeyState(VK_MENU) & 0x8000)) {
						PostMessage(g_hwndTs, WM_TASKSWITCH, IDH_SWITCH, 0);
					}
					break;

				default:
					if (s_vkSkip && pkbhs->vkCode == s_vkSkip) {
						s_vkSkip = 0;
						lRet = 1;
					}
					break;
			}
		}
	}
	return(lRet ? lRet : CallNextHookEx(g_hhookKb, nCode, wParam, lParam));
}


LRESULT CALLBACK AltKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {

	if (g_hheapWnd) {
		if (nCode == HC_ACTION) {
			PKBDLLHOOKSTRUCT pkbhs = (PKBDLLHOOKSTRUCT)lParam;
			if (pkbhs->vkCode == VK_LMENU || pkbhs->vkCode == VK_RMENU) {
				if (g_hwndTs && !(GetAsyncKeyState(VK_MENU) & 0x8000)) {
					PostMessage(g_hwndTs, WM_TASKSWITCH, IDH_SWITCH, 0);
				}
			} else if (pkbhs->vkCode == VK_ESCAPE) {
				if (g_hwndTs && !(g_dwFlagsList & TSFL_MENUACTIVE)) {
					PostMessage(g_hwndTs, WM_TASKSWITCH, IDH_ESCAPE, 0);
					return(1);
				}
			}
		}
		return(CallNextHookEx(g_hhookKb, nCode, wParam, lParam));
	}

    // unhook
	LRESULT lRet = CallNextHookEx(g_hhookKb, nCode, wParam, lParam);
	UnhookWindowsHookEx(g_hhookKb);
	g_hhookKb = NULL;
	return(lRet);
}

//-----------------------------------------------------------

HDESK g_hDesk = NULL;

DWORD WINAPI TaskSwitchThread(LPVOID pvParam) {

	if (g_hDesk) {
		CloseDesktop(g_hDesk);
		g_hDesk = NULL;
	}

	g_hDesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
	//_RPT1(_CRT_WARN, "hDesk = %X\n", g_hDesk);
	if (g_hDesk) {
		SetThreadDesktop(g_hDesk);
		//CloseDesktop(g_hDesk);
	}

	UINT uParam = (UINT)(UINT_PTR)pvParam;

	g_hwndFrgnd = GetForegroundWindow();
	if (uParam != IDH_STNEXTTASK && uParam != IDH_STINEXTTASK) {
		if (g_dwFlags & TSF_HOOKALTTAB) {
			g_idThreadAttachTo = g_hwndFrgnd ? GetWindowThreadProcessId(g_hwndFrgnd, NULL) : 0;
			if (g_idThreadAttachTo)
				AttachThreadInput(GetCurrentThreadId(), g_idThreadAttachTo, TRUE);
		} else if (!(g_dwFlags & TSF_STICKYALTTAB)) {
			_ASSERT(!g_hhookKb);
			g_hhookKb = SetWindowsHookEx(WH_KEYBOARD_LL, 
				(HOOKPROC)AltKeyboardProc, g_hinstExe, NULL);
			//if (!g_hhookKb) return(FALSE);
		}
	}
	g_hwndShell = GetShellWindow(); // for service messages
	g_hwndTaskbar = FindWindow(L"Shell_TrayWnd", L"");

	//SetForegroundWindow(g_hwndMain);
	
	MyEnumDesktopWindows();
	if (g_nTs <= 0 || (g_nTs < 2 && (uParam >= IDH_NEXTTASK && uParam <= IDH_WINPREVTASK))) {
		HWND hwndPv = NULL;
		if (g_nTs == 1)
			CheckTask(0, &hwndPv);
		FastSwitch(hwndPv);
		return(0);
	}

	int nCurTs;

	if (g_hwndFrgnd) {
		HWND hwndTmp = g_hwndFrgnd, hwndOwner;
		do {
			hwndOwner = hwndTmp;
			hwndTmp = GetWindow(hwndTmp, GW_OWNER);
		} while (hwndTmp && hwndTmp != g_hwndShell);

		// trick
		hwndTmp = hwndOwner;
		DWORD dwStyleEx = GetWindowLongPtr(hwndOwner, GWL_EXSTYLE),
			dwStyleEx2 = (g_hwndFrgnd != hwndOwner) ? GetWindowLongPtr(g_hwndFrgnd, GWL_EXSTYLE) : dwStyleEx;
		if (hwndOwner != g_hwndFrgnd && dwStyleEx2 & WS_EX_APPWINDOW)
			hwndOwner = g_hwndFrgnd;

		nCurTs = 0;
		while (nCurTs < g_nTs && g_pTs[nCurTs].hwndOwner != hwndOwner)
			nCurTs++;
		_RPT3(_CRT_WARN, "hwndOwner=%X, hwndFrgnd=%x, %d\n", hwndOwner, g_hwndFrgnd, nCurTs);
		// trick
		if (nCurTs >= g_nTs && hwndTmp != hwndOwner) {
			hwndOwner = hwndTmp;
			nCurTs = 0;
			while (nCurTs < g_nTs && g_pTs[nCurTs].hwndOwner != hwndOwner)
				nCurTs++;
		}
		if (nCurTs < g_nTs)
			g_nCurTs = nCurTs;
		else {
			g_nCurTs = (uParam == IDH_INSTNEXT || uParam == IDH_INSTPREV || 
				(uParam >= IDH_STINEXTTASK && uParam <= IDH_STITRAYNEXT)) ? 0 : (g_nTs - 1);
		}
	} else {
		g_nCurTs = g_nCurTs = (uParam == IDH_INSTNEXT || uParam == IDH_INSTPREV || 
			(uParam >= IDH_STINEXTTASK && uParam <= IDH_STITRAYNEXT)) ? 0 : (g_nTs - 1);
	}

	if (uParam == IDH_INSTNEXT || uParam == IDH_INSTPREV || 
		uParam == IDH_STINEXTTASK || uParam == IDH_STITRAYNEXT) { // instances switcher

		PWSTR pszInstance = g_pTs[g_nCurTs].pszExePath;
		if (pszInstance) {
			nCurTs = 0;
			while (nCurTs < g_nTs) {
				if (pszInstance != g_pTs[nCurTs].pszExePath && 
					lstrcmpi(pszInstance, g_pTs[nCurTs].pszExePath))
					DeleteTsItem(nCurTs);
				else nCurTs++;
			}
		}
	}

	if (!(g_dwFlags & TSF_STICKYALTTAB)) {
		g_nCurTs = _NextTask(g_nCurTs, (uParam == IDH_PREVTASK || 
			uParam == IDH_WINPREVTASK || uParam == IDH_INSTPREV) ? -1 : 1, NULL);
	}
	if (g_nTs <= 0) {
		FastSwitch(g_hwndFrgnd);
		return(1);
	}

	/*HWND hwndBk = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | \
		WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_LAYERED, 
		g_szTsBkClass, g_szWindowName, WS_POPUP, 
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 
		g_hwndMain, NULL, g_hinstExe, NULL);
	if (hwndBk)
		ShowWindow(hwndBk, SW_SHOW);*/


	HWND hwndTs = CreateWindowEx( WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		g_szTaskSwitchWnd, g_szWindowName, WS_POPUP, 0, 0, 0, 0, 
		NULL, NULL, g_hinstExe, (LPVOID)(UINT_PTR)uParam);
	if (!hwndTs) {
		FastSwitch(NULL);
		return(1);
	}

	if (g_dwShowDelay && uParam == IDH_NEXTTASK) {
		SetWindowPos(hwndTs, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		SetTimer(hwndTs, TIMER_TSXPSHOW, g_dwShowDelay, NULL);
	} else {
		SetWindowPos(hwndTs, HWND_TOPMOST, 0, 0, 0, 0, 
			SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
	SetForegroundWindow(hwndTs);

	RedrawWindow(g_hwndTaskbar, NULL, NULL, // black taskbar
		RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN);

	//klvov: initialization code for IDH_ALTAPPLIST mode
	if (uParam == IDH_ALTAPPLIST){
		//g_dwFlagsList |= TSFL_ALTMODE;
		if (hwndTs) 
		{
			SortTaskListByModuleName(hwndTs);
			SelectTask(hwndTs, 0, FALSE);
			lstrcpy(g_szTypeBuffer, L"");
		}
	}
	else {
		//g_dwFlagsList &= ~TSFL_ALTMODE;
	}
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// free resources
	FastSwitch(NULL);
	return(0);
}

BOOL ShowTaskSwitchWnd(UINT uParam =IDH_NEXTTASK) {
	//klvov: not sure whether this should be placed here or not
	//well, at this time I'm placing initialization code for  IDH_ALTAPPLIST
	//mode here
	if (uParam == IDH_ALTAPPLIST)	{
		g_dwFlagsList |= (/*TSFL_TEMPSTICKY |*/ TSFL_ALTMODE);
		//klvov: if user changed his mind and wants to select other task,
		//he can just press assigned key combination for the second time.
		lstrcpy(g_szTypeBuffer, L"");
		if (g_hwndTs) ProcessTypeBuffer(g_hwndTs);
	}
	else{
		g_dwFlagsList &= ~(/*TSFL_TEMPSTICKY | */TSFL_ALTMODE);
	}
	if (g_hThreadTs && WaitForSingleObject(g_hThreadTs, 0) != WAIT_OBJECT_0) {
		if (g_hwndTs)
			PostMessage(g_hwndTs, WM_TASKSWITCH, (WPARAM)uParam, 0);
	} else {
		if (g_hThreadTs) {
			CloseHandle(g_hThreadTs);
			g_hThreadTs = NULL;
		}
		if (WaitForSingleObject(g_hmtxPv, 0) != WAIT_OBJECT_0)
			return(FALSE);
		if (g_hbitmapPv) {
			DeleteObject(g_hbitmapPv);
			g_hbitmapPv = NULL;
		}
		ReleaseMutex(g_hmtxPv);

		g_hThreadTs = CreateThread(NULL, 0, TaskSwitchThread, 
			(LPVOID)(UINT_PTR)uParam, CREATE_SUSPENDED, NULL);
		if (!g_hThreadTs) 
			return(FALSE);

		//HWND hwndShell = GetDesktopWindow();
		//_RPT1(_CRT_WARN, "hwndShell = %X\n", hwndShell);
/*
		HWND hwndFrgnd = GetForegroundWindow();
		_RPT1(_CRT_WARN, "hwndFrgnd = %X\n", hwndFrgnd);
		if (hwndFrgnd) {
			DWORD dwFrgndId = GetWindowThreadProcessId(hwndFrgnd, NULL);
			HDESK hDesk = GetThreadDesktop(dwFrgndId);
			_RPT1(_CRT_WARN, "hDesk = %X\n", hDesk);
			if (hDesk) {
				//SetThreadDesktop(hDesk);
				CloseDesktop(hDesk);
			}
		}
*/
		/*HDESK hDesk = OpenInputDesktop(0, FALSE, GENERIC_ALL);
		if (hDesk) {
			_RPT1(_CRT_WARN, "hDesk = %X\n", hDesk);
			SetThreadDesktop(hDesk);
			CloseDesktop(hDesk);
		}*/
		//SetThreadPriority(g_hThreadTs, THREAD_PRIORITY_BELOW_NORMAL);
		ResumeThread(g_hThreadTs);

	}
	return(TRUE);
}

//-----------------------------------------------------------
//-----------------------------------------------------------

void DestroyTs() {

	if (g_pTs) {
		for (int i = 0; i < g_nTs; i++) {
			if (g_pTs[i].hbitmapPv)
				DeleteObject(g_pTs[i].hbitmapPv);
		}
	}
	if (g_hheapWnd) {
		HeapDestroy(g_hheapWnd);
		g_hheapWnd = NULL;
	}
	g_pTs = NULL;
	g_nTs = 0;	
}

void DestroyAltHook() {
	if (g_dwFlags & TSF_HOOKALTTAB) {
		if (g_idThreadAttachTo) {
			AttachThreadInput(GetCurrentThreadId(), g_idThreadAttachTo, FALSE);
			g_idThreadAttachTo = 0;
		}
	} else if (g_hhookKb) {
		UnhookWindowsHookEx(g_hhookKb);
		g_hhookKb = NULL;
	}
}

void FastSwitch(HWND hwndPv) {
	DestroyAltHook();
	DestroyTs();
	g_hwndFrgnd = NULL;
	if (hwndPv)
		MySwitchToThisWindow(hwndPv);
	SetTimer(g_hwndMain, TIMER_CLOSEDESK, 50, NULL);
}

//-----------------------------------------------------------

int CompareExcl(const PVOID pel1, const PVOID pel2) {

	PTSEXCLUSION ptse1 = (PTSEXCLUSION)pel1;
	PTSEXCLUSION ptse2 = (PTSEXCLUSION)pel2;

	if (ptse1->dwFlags & TSEXCLF_BYPROCESS && !(ptse2->dwFlags & TSEXCLF_BYPROCESS))
		return(-1);
	if (ptse2->dwFlags & TSEXCLF_BYPROCESS && !(ptse1->dwFlags & TSEXCLF_BYPROCESS))
		return(1);
	return(lstrcmpi(ptse1->szExeName, ptse2->szExeName));
}


DWORD IsWindowExcluded(HWND hwnd, PCWSTR pszExeName) {

	DWORD dwRet = 0;

	WCHAR szClassName[MAX_CLASSNAME] = L"",
		szCaption[MAX_CAPTION] = L"";
	BOOL fGetText = FALSE, fClassName = FALSE;

	for (int i = 0; i < g_nExcl; i++) {
		if (!(g_pExcl[i].dwFlags & TSEXCLF_BYPROCESS) || 
			(pszExeName && !lstrcmpi(g_pExcl[i].szExeName, pszExeName))) {

			if (g_pExcl[i].dwFlags & TSEXCLF_BYCLASS) {
				if (!fClassName) {
					if (!GetClassName(hwnd, szClassName, MAX_CLASSNAME))
						break;
					fClassName = TRUE;
				}
				if (lstrcmp(g_pExcl[i].szClassName, szClassName))
					continue;
				else if (!(g_pExcl[i].dwFlags & TSEXCLF_BYCAPTION)) {
					dwRet = (g_pExcl[i].dwFlags & TSEXCLF_PREVIEW) 
						? TSEXCLF_PREVIEW : TSEXCLF_ENUM;
					break;
				}
			} else if (!(g_pExcl[i].dwFlags & TSEXCLF_BYCAPTION)) {
				dwRet = (g_pExcl[i].dwFlags & TSEXCLF_PREVIEW) 
					? TSEXCLF_PREVIEW : TSEXCLF_ENUM;
				break;
			}
			if (g_pExcl[i].dwFlags & TSEXCLF_BYCAPTION) {
				if (!fGetText) {
					InternalGetWindowText(hwnd, szCaption, MAX_CAPTION);
					fGetText = TRUE;
				}
				if (!lstrcmp(g_pExcl[i].szCaption, szCaption)) {
					dwRet = (g_pExcl[i].dwFlags & TSEXCLF_PREVIEW) 
						? TSEXCLF_PREVIEW : TSEXCLF_ENUM;
					break;
				}
			}
		}
	}
	return(dwRet);
}

//-----------------------------------------------------------

#define TS_ALLOCUNIT					32

BOOL _AddNewTaskWindow(HWND hwndOwner, HWND hwnd) {

	TASKINFO ti;
	ZeroMemory(&ti, sizeof(TASKINFO));

	GetWindowThreadProcessId(hwndOwner, &ti.dwProcessId);
	HANDLE hProcess = NULL;
	BOOL fSuccess = FALSE;

	__try {

		hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ti.dwProcessId);

		if (hProcess) {
			// this works ok, it opens every process, even 64bit
			//wsprintf(tmp, L"process %d successfully opened\r\n", ti.dwProcessId); OutputDebugString(tmp);
			DWORD dw;
			HMODULE hMod;
			if (EnumProcessModulesEx(hProcess, &hMod, sizeof(hMod), &dw, LIST_MODULES_ALL)) {
			  // EnumProcessModules does not work for 64bit processes, don't know why
		      //wsprintf(tmp, L"process %d successfully enumed modules\r\n", ti.dwProcessId); OutputDebugString(tmp);

				int len = MAX_PATH;
				ti.pszExePath = (PWSTR)HeapAlloc(g_hheapWnd, 
					HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
				int n = GetModuleFileNameEx(hProcess, hMod, ti.pszExePath, len * sizeof(WCHAR));

				while (n >= len) {
					len += MAX_PATH;
					ti.pszExePath = (PWSTR)HeapReAlloc(g_hheapWnd, 
						HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, 
						ti.pszExePath, len * sizeof(WCHAR));
					n = GetModuleFileNameEx(hProcess, hMod, ti.pszExePath, len * sizeof(WCHAR));
				}
				if (n <= 0) {
					HeapFree(g_hheapWnd, 0, ti.pszExePath);
					ti.pszExePath = NULL;
				}
				n = GetModuleBaseName(hProcess, hMod, ti.szModuleName, len * sizeof (WCHAR) ); // this is correct
				//wsprintf(tmp, L" process id: %d", ti.dwProcessId);
				//lstrcat(ti.szModuleName, tmp);
				//n = GetModuleFileNameEx(hProcess, hMod, ti.szModuleName, len * sizeof (WCHAR) ); // full path, does not work for x64 processes
				// n = GetProcessImageFileName(hProcess, ti.szModuleName, len * sizeof (WCHAR) ); // also does not work for x64 processes
                
				//DWORD dSize = MAX_PATH;
				//if (!QueryFullProcessImageName(hProcess, 1, ti.szModuleName, &dSize )) {
				//	OutputDebugString(L"function QueryFullProcessImageName has failed\r\n");
				//}
				//lstrcat(ti.szModuleName, L"*"); // debug - just to ensure I'm in right place

				// snapshot of all processes seems able to retrieve 64-bit module names from within 32 bit process

				/*
				if (lstrlen(ti.szModuleName) == 0) {
					OutputDebugString(L"yep!\r\n");
					HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
					//if (hSnapshot == HANDLE(-1)) __raise(L"ahaaha");
					PROCESSENTRY32 pe;
					pe.dwSize = sizeof(PROCESSENTRY32);
					BOOL retval = Process32First(hSnapshot, &pe);
					while (retval) {
					  DWORD pid = pe.th32ProcessID;
					  if (pid == ti.dwProcessId) {
						  OutputDebugString(pe.szExeFile);
						  //lstrcpyn(ti.szModuleName, pe.szExeFile, MAX_PATH);
					  }
					  //OutputDebugString(pe.szExeFile);
					  //OutputDebugString(L" ");
					  //wsprintf(tmp, L" process id: %d", pid);
					  //OutputDebugString(tmp);
					  //OutputDebugString(L"\r\n");
					  retval = Process32Next(hSnapshot, &pe);
					}
					CloseHandle(hSnapshot);
				}
				*/
			}
      else {
        DWORD dwLastError = GetLastError();
        wsprintf(tmp, L"Failed to call EnumProcessModulesEx for process %d, error %x\r\n", ti.dwProcessId, dwLastError); OutputDebugString(tmp);
        DWORD dSize = MAX_PATH;
		if (!QueryFullProcessImageName(hProcess, 0, ti.szModuleName, &dSize )) {
		  OutputDebugString(L"function QueryFullProcessImageName also has failed\r\n");
		}
		else { // ok, got full process image name
			WCHAR *part;
			WCHAR last_part[4000];
			part = wcstok(ti.szModuleName, L"\\");
			while (part != NULL) {
				//OutputDebugString(part);
				//OutputDebugString(L"\r\n");
				lstrcpy(last_part, part);
				part = wcstok(NULL, L"\\");
			}
			lstrcpy(ti.szModuleName, last_part);
			//OutputDebugString(last_part);
			//OutputDebugString(L"\r\n");

		}
      }
			CloseHandle(hProcess);
			hProcess = NULL;

		} else {
			ti.pszExePath = NULL;
		}

		if (ti.pszExePath) {
			ti.nExeName = lstrlen(ti.pszExePath);
			while (ti.nExeName > 0 && ti.pszExePath[ti.nExeName - 1] != L'\\')
				ti.nExeName--;
		} else {
			ti.nExeName = 0;
		}

		DWORD dwExcl = IsWindowExcluded(hwnd, ti.pszExePath 
			? (ti.pszExePath + ti.nExeName) : NULL);
		if (dwExcl == TSEXCLF_ENUM)
			__leave;
		ti.phWndPv[0] = hwnd;
		ti.dwFlags = (dwExcl == TSEXCLF_PREVIEW) ? 1 : 0;		
		ti.nWndPv = 1;

		if (hwnd != hwndOwner && IsWindowExcluded(hwndOwner, ti.pszExePath 
			? (ti.pszExePath + ti.nExeName) : NULL) == TSEXCLF_ENUM) __leave;

		ti.hwndOwner = hwndOwner;

		if (g_dwFlagsEx & TSFEX_OWNERCAPTION ||
			!InternalGetWindowText(hwnd, ti.szCaption, MAX_CAPTION)) {
				if (!InternalGetWindowText(hwndOwner, ti.szCaption, MAX_CAPTION)) {
					if (g_dwFlagsEx & TSFEX_OWNERCAPTION)
						InternalGetWindowText(hwnd, ti.szCaption, MAX_CAPTION);
				}
			}
			GetWindowIcons(hwndOwner, &ti.hIcon, &ti.hIconSm);

			if (!(g_nTs % TS_ALLOCUNIT)) {
				_ASSERT(g_hheapWnd && g_pTs);
				g_pTs = (PTASKINFO)HeapReAlloc(g_hheapWnd, HEAP_GENERATE_EXCEPTIONS, 
					g_pTs, (g_nTs + TS_ALLOCUNIT) * sizeof(TASKINFO));					
			}
			CopyMemory(&g_pTs[g_nTs], &ti, sizeof(TASKINFO));
			g_nTs++;

			fSuccess = TRUE;
	}
	__finally {
		if (hProcess)
			CloseHandle(hProcess);
		if (!fSuccess && ti.pszExePath)
			HeapFree(g_hheapWnd, 0, ti.pszExePath);
	}
	return(fSuccess);
}


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM /*lParam*/) {

	DWORD dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
	if (dwStyle & WS_VISIBLE) {

		HWND hwndOwner, hwndTmp = hwnd;
		do {
			hwndOwner = hwndTmp;
			hwndTmp = GetWindow(hwndTmp, GW_OWNER);
		} while (hwndTmp && hwndTmp != g_hwndShell); // service messages

		DWORD dwStyleEx = GetWindowLongPtr(hwndOwner, GWL_EXSTYLE),
			dwStyleEx2 = (hwnd != hwndOwner) ? GetWindowLongPtr(hwnd, GWL_EXSTYLE) : dwStyleEx;

		int nTask = 0;

		if (hwnd != hwndOwner && dwStyleEx2 & WS_EX_APPWINDOW) {
			//_RPT2(_CRT_WARN, "task %d, hwnd = %X, hwndOwner = %X\n", g_nTs, hwnd, hwndOwner);
			nTask = g_nTs;
			hwndOwner = hwnd;
		} else {
			while (nTask < g_nTs && g_pTs[nTask].hwndOwner != hwndOwner)
				nTask++;
		}

		if (nTask < g_nTs) {
			DWORD dwExcl = IsWindowExcluded(hwnd, 
				g_pTs[nTask].pszExePath ? (g_pTs[nTask].pszExePath + g_pTs[nTask].nExeName) : NULL);
			if (dwExcl != TSEXCLF_ENUM) {
				if (g_pTs[nTask].nWndPv < MAX_WNDPV) {
					g_pTs[nTask].phWndPv[g_pTs[nTask].nWndPv] = hwnd;
					if (dwExcl == TSEXCLF_PREVIEW)
						g_pTs[nTask].dwFlags |= 1 << g_pTs[nTask].nWndPv;
					g_pTs[nTask].nWndPv++;
				}

				if (g_pTs[nTask].dwFlags & TI_DELETE) {
					if (!(GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
						g_pTs[nTask].dwFlags &= ~TI_DELETE;
				}
			}
		} else {
			if (!(dwStyleEx & WS_EX_TOOLWINDOW) || dwStyleEx2 & WS_EX_APPWINDOW || 
				(!(dwStyleEx2 & WS_EX_TOOLWINDOW) && dwStyleEx2 & WS_EX_CONTROLPARENT)) {
				if (_AddNewTaskWindow(hwndOwner, hwnd)) {
					if (!(dwStyleEx & WS_EX_TOOLWINDOW) && dwStyleEx2 & WS_EX_TOOLWINDOW)
						g_pTs[g_nTs - 1].dwFlags |= TI_DELETE;
				}
			}
		}
	}
	return(TRUE);
}


int MyEnumDesktopWindows() {

	DestroyTs();

	__try {

		g_hheapWnd = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0, 0);
		g_pTs = (PTASKINFO)HeapAlloc(g_hheapWnd, 
			HEAP_GENERATE_EXCEPTIONS, 0);

		EnumDesktopWindows(NULL, EnumWindowsProc, NULL);

		volatile int i = 0;
		while (i < g_nTs) {
			if (g_pTs[i].dwFlags & TI_DELETE) {
				MoveMemory(g_pTs + i, g_pTs + i + 1, sizeof(TASKINFO) * (g_nTs - i - 1));
				g_nTs--;
			} else i++;
		}

		// add minized to tray
		if (g_dwFlags & TSF_ENUMTRAY) {
			HWND phWti[MAX_WNDTRAY];
			int cWti = (int)SendMessage(g_hwndMain, WM_GETTRAYWINDOWS, (WPARAM)phWti, MAX_WNDTRAY);
			if (cWti > 0) {
				for (int j = 0; j < cWti; j++) {
					if (_AddNewTaskWindow(phWti[j], phWti[j]))
						g_pTs[g_nTs - 1].dwFlags |= TI_TRAYWINDOW;
				}
			}
		}

		if (g_nTs > 0 && g_nTs % TS_ALLOCUNIT) {
			g_pTs = (PTASKINFO)HeapReAlloc(g_hheapWnd, HEAP_GENERATE_EXCEPTIONS, 
				g_pTs, g_nTs * sizeof(TASKINFO));
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		g_nTs = 0;
	}
	g_nSelTs = 0;

	if (g_nTs <= 0)
		DestroyTs();

	return(g_nTs);
}

//-----------------------------------------------------------

int MyDrawShadowText(HDC hdc, LPCWSTR szText, PRECT prcText, 
					 UINT uFormat, COLORREF crText, DWORD dwShadow) {

	int nRet = 0, nDeep = 0;

	if (!(dwShadow & TSFCS_NODEEP)) {
		nDeep = (dwShadow & TSFCS_DEEPMASK) >> 24;
		if (dwShadow & TSFCS_NEGATIVEDEEP) nDeep = -nDeep;
	}
	if (nDeep != 0) {
		nRet = DrawShadowText(hdc, szText, lstrlen(szText), prcText, uFormat, 
			crText, dwShadow & TSFCS_COLORMASK, nDeep, nDeep);
	} else {
		COLORREF crOld = SetTextColor(hdc, crText);
		nRet = DrawText(hdc, szText, -1, prcText, uFormat);
		SetTextColor(hdc, crOld);
	}
	return(nRet);
}

//-----------------------------------------------------------
// return TRUE if changed
BOOL UpdateWallpaper() {

	WALLPAPER wp;
	ZeroMemory(&wp, sizeof(WALLPAPER));

	BOOL fWallpaper = (BOOL)((g_dwFlagsPv & TSFPV_DESKTOP || 
		!(g_dwFlagsPv & TSFPV_OLDSTYLE)) && g_dwFlagsPv & TSFPV_WALLPAPER);

	if (fWallpaper) {

		if (!SystemParametersInfo(SPI_GETDESKWALLPAPER, SIZEOF_ARRAY(wp.szWallpaper), 
			(PVOID)wp.szWallpaper, FALSE)) wp.szWallpaper[0] = L'\0';

		HKEY hkeyDsk;
		if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hkeyDsk)) {
			WCHAR szBuff[32];
			DWORD cbData = SIZEOF_ARRAY(szBuff);
			if (!RegQueryValueEx(hkeyDsk, L"TileWallpaper", NULL, NULL, 
				(PBYTE)szBuff, &cbData) && !lstrcmp(szBuff, L"1")) wp.fTiled = TRUE;
			cbData = SIZEOF_ARRAY(szBuff);
			if (!RegQueryValueEx(hkeyDsk, L"WallpaperStyle", NULL, NULL, 
				(PBYTE)szBuff, &cbData) && !lstrcmp(szBuff, L"2")) wp.fStretched = TRUE;
			cbData = sizeof(LARGE_INTEGER);
			RegQueryValueEx(hkeyDsk, L"ConvertedWallpaper Last WriteTime", 
				NULL, NULL, (PBYTE)&wp.liWriteTime, &cbData);
			RegCloseKey(hkeyDsk);
		}
	}

	if (g_dwFlagsPv & TSFPV_DESKTOP && g_dwFlagsPv & TSFPV_TASKBAR) {
		wp.rcWallpaper.left = wp.rcWallpaper.top = 0;
		wp.rcWallpaper.right = GetSystemMetrics(SM_CXSCREEN);
		wp.rcWallpaper.bottom = GetSystemMetrics(SM_CYSCREEN);
	} else
		SystemParametersInfo(SPI_GETWORKAREA, 0, &wp.rcWallpaper, FALSE);

	BOOL fUpdated = FALSE;

	if (fWallpaper && !g_hbitmapWpEx || (g_Wallpaper.fTiled != wp.fTiled || 
		g_Wallpaper.fStretched != wp.fStretched || 
		g_Wallpaper.liWriteTime.LowPart != wp.liWriteTime.LowPart || 
		g_Wallpaper.liWriteTime.HighPart != wp.liWriteTime.HighPart || 
		g_Wallpaper.rcWallpaper.left != wp.rcWallpaper.left || 
		g_Wallpaper.rcWallpaper.top != wp.rcWallpaper.top || 
		g_Wallpaper.rcWallpaper.right != wp.rcWallpaper.right || 
		g_Wallpaper.rcWallpaper.bottom != wp.rcWallpaper.bottom || 
		lstrcmp(g_Wallpaper.szWallpaper, wp.szWallpaper))) {

		CopyMemory(&g_Wallpaper, &wp, sizeof(WALLPAPER));

		g_rcPvEx.top = g_rcPv.top + YMARGIN_PREVIEW;
		g_rcPvEx.left = g_rcPv.left + XMARGIN_PREVIEW;
		g_rcPvEx.right = g_rcPv.right - XMARGIN_PREVIEW;
		if (!(g_dwFlagsPv & TSFPV_DESKTOP) && (g_dwFlagsPv & TSFPV_OLDSTYLE)) {
			g_rcPvEx.bottom = g_rcPv.bottom - YMARGIN_PREVIEW;
		} else {
			g_rcPvEx.bottom = /*g_rcPvEx.top +*/ ((g_rcPvEx.right - g_rcPvEx.left) * \
				(g_Wallpaper.rcWallpaper.bottom - g_Wallpaper.rcWallpaper.top)) / \
				(g_Wallpaper.rcWallpaper.right - g_Wallpaper.rcWallpaper.left);
			if (g_rcPvEx.top + g_rcPvEx.bottom > g_rcPv.bottom - YMARGIN_PREVIEW) {
				g_rcPvEx.bottom = g_rcPv.bottom - YMARGIN_PREVIEW;
				int nPvWidth = ((g_rcPvEx.bottom - g_rcPvEx.top) * \
					(g_Wallpaper.rcWallpaper.right - g_Wallpaper.rcWallpaper.left)) / \
					(g_Wallpaper.rcWallpaper.bottom - g_Wallpaper.rcWallpaper.top);
				g_rcPvEx.left = (g_rcPv.right + g_rcPv.left - nPvWidth) / 2;
				g_rcPvEx.right = g_rcPvEx.left + nPvWidth;
			} else {
				if (g_dwFlagsPv & TSFPV_VCENTER) 
					g_rcPvEx.top = (g_rcPv.top + g_rcPv.bottom - g_rcPvEx.bottom) / 2;
				g_rcPvEx.bottom += g_rcPvEx.top;
			}
		}

		if (g_hbitmapWpEx) {
			DeleteObject(g_hbitmapWpEx);
			g_hbitmapWpEx = NULL;
		}
		if (fWallpaper) {

			RECT rcWpEx;
			rcWpEx.left = rcWpEx.top = 0;
			rcWpEx.right = g_rcPvEx.right - g_rcPvEx.left;
			rcWpEx.bottom = g_rcPvEx.bottom - g_rcPvEx.top;

			HDC hdcMem = NULL;
			HBITMAP hbitmapOld = NULL;
			BOOL fSuccess = FALSE;

			__try {
				HDC hdcScreen = GetDC(NULL);
				hdcMem = CreateCompatibleDC(hdcScreen);
				g_hbitmapWpEx = CreateCompatibleBitmap(hdcScreen, rcWpEx.right, rcWpEx.bottom);
				ReleaseDC(NULL, hdcScreen);
				if (!g_hbitmapWpEx) __leave;

				hbitmapOld = (HBITMAP)SelectObject(hdcMem, g_hbitmapWpEx);
				if (!MyPaintWallpaper(hdcMem, &rcWpEx, &g_Wallpaper))
					__leave;

				fSuccess = TRUE;
			}
			__finally {
				if (hbitmapOld)
					SelectObject(hdcMem, hbitmapOld);
				if (hdcMem)
					DeleteDC(hdcMem);
			}
			if (!fSuccess && g_hbitmapWpEx) {
				DeleteObject(g_hbitmapWpEx);
				g_hbitmapWpEx = NULL;
			}
		}
		fUpdated = TRUE;
	}
	return(fUpdated);
}

//-----------------------------------------------------------

BOOL InitTsBackground(HWND hwndTs) {

	DestroyTsBackground();

	UpdateWallpaper();

	if (!(g_dwFlagsPv & TSFPV_DESKTOP)) {
		g_rcScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
		g_rcScreen.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
		g_rcScreen.right = g_rcScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		g_rcScreen.bottom = g_rcScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	} else
		g_rcScreen = g_Wallpaper.rcWallpaper;

	XFORM xForm;
	RECT rc1, rc2;
	BOOL fSuccess = FALSE;
	DWORD dwStyleXP = LOBYTE(LOWORD(g_dwFlagsEx)),
		dwStyleCl = HIBYTE(LOWORD(g_dwFlagsEx));

	if (!g_htheme && !(g_dwFlagsEx & TSFEX_NOVISUALSTYLES)) {
		g_htheme = OpenThemeData(hwndTs, (dwStyleXP == TSEX_SMWINDOWMODE) 
			? L"Window" : ((dwStyleXP == TSEX_FLATXPMODE) ? L"ExplorerBar" 
			: ((dwStyleXP == TSEX_SOFTXPMODE) ? L"Tab" 
			: ((dwStyleXP == TSEX_TASKBARMODE) ? L"TaskBar" : L"StartPanel"))));
	}

	if (g_htheme) { // Windows XP style

		HRGN hrgn = NULL, hrgnTmp = NULL;
		
		__try {

			if (dwStyleXP == TSEX_POWERTOYMODE) {

				rc1.left = rc1.top = 0;
				rc1.right = g_rcTs.right;
				rc1.bottom = g_rcTs.bottom / 2 + 1;

				__asm {
					fild int ptr g_rcTs.bottom
					fstp xForm.eDy

					fld1
					fstp xForm.eM11

					fldz
					fst xForm.eDx
					fst xForm.eM12
					fst xForm.eM21

					fsub xForm.eM11
					fstp xForm.eM22
				}

				if (GetThemeBackgroundRegion(g_htheme, NULL, 
					SPP_USERPANE, 0, &rc1, &hrgn) != S_OK) __leave;

				DWORD cbData = GetRegionData(hrgn, 0, NULL);
				if (!cbData) __leave;
				LPRGNDATA pRgnData = (LPRGNDATA)HeapAlloc(GetProcessHeap(), 0, cbData);
				if (!pRgnData) __leave;
				GetRegionData(hrgn, cbData, pRgnData);			
				hrgnTmp = ExtCreateRegion(&xForm, cbData, pRgnData);
				HeapFree(GetProcessHeap(), 0, pRgnData);
				if (hrgnTmp)
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);

			} else if (dwStyleXP == TSEX_SMWINDOWMODE) {

				if (GetThemeBackgroundRegion(g_htheme, NULL, 
					WP_SMALLCAPTION, CS_ACTIVE, &g_rcTs, &hrgn) != S_OK) __leave;

			} else if (dwStyleXP == TSEX_FLATXPMODE) {
				
				if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
					if (GetThemeBackgroundRegion(g_htheme, NULL, 
						EBP_HEADERBACKGROUND, 0, &g_rcCap, &hrgn) != S_OK) __leave;
				}
				
				if (GetThemeBackgroundRegion(g_htheme, NULL, 
					EBP_NORMALGROUPBACKGROUND, 0, &g_rcList, &hrgnTmp) != S_OK) __leave;
				if (hrgn) {
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
					DeleteObject(hrgnTmp);
				} else hrgn = hrgnTmp;
				hrgnTmp = NULL;

				if (GetThemeBackgroundRegion(g_htheme, NULL, 
					EBP_SPECIALGROUPBACKGROUND, 0, &g_rcPv, &hrgnTmp) != S_OK) __leave;
				CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);

				if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
					DeleteObject(hrgnTmp);
					hrgnTmp = NULL;

					if (GetThemeBackgroundRegion(g_htheme, NULL, 
						EBP_SPECIALGROUPHEAD, 0, &g_rcPane, &hrgnTmp) != S_OK) __leave;
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
				}
			} else if (dwStyleXP == TSEX_SOFTXPMODE) {

				if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
					if (GetThemeBackgroundRegion(g_htheme, NULL, 
						TABP_TABITEM, TIS_HOT, &g_rcCap, &hrgn) != S_OK) __leave;
				}
				if (GetThemeBackgroundRegion(g_htheme, NULL, 
					TABP_PANE, 0, &g_rcList, &hrgnTmp) != S_OK) __leave;
				if (hrgn) {
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
					DeleteObject(hrgnTmp);
				} else hrgn = hrgnTmp;
				hrgnTmp = NULL;

				if (GetThemeBackgroundRegion(g_htheme, NULL, 
					TABP_PANE, 0, &g_rcPv, &hrgnTmp) != S_OK) __leave;
				CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
				
				if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
					DeleteObject(hrgnTmp);
					hrgnTmp = NULL;

					if (GetThemeBackgroundRegion(g_htheme, NULL,  
						TABP_TABITEM, TIS_NORMAL, &g_rcPane, &hrgnTmp) != S_OK) __leave;
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
				}
			} else if (dwStyleXP == TSEX_TASKBARMODE) {

				if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
					if (GetThemeBackgroundRegion(g_htheme, NULL,  
						TBP_BACKGROUNDBOTTOM, 0, &g_rcCap, &hrgn) != S_OK) __leave;
				}
				if (GetThemeBackgroundRegion(g_htheme, NULL,  
					TBP_BACKGROUNDLEFT, 0, &g_rcList, &hrgnTmp) != S_OK) __leave;
				if (hrgn) {
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
					DeleteObject(hrgnTmp);
				} else hrgn = hrgnTmp;
				hrgnTmp = NULL;

				if (GetThemeBackgroundRegion(g_htheme, NULL,  
					TBP_BACKGROUNDRIGHT, 0, &g_rcPv, &hrgnTmp) != S_OK) __leave;
				CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
				
				if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
					DeleteObject(hrgnTmp);
					hrgnTmp = NULL;

					if (GetThemeBackgroundRegion(g_htheme, NULL,  
						TBP_BACKGROUNDTOP, 0, &g_rcPane, &hrgnTmp) != S_OK) __leave;
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
				}
			} else { //if (dwStyleXP == TSEX_DEFAULTMODE) {

				if (g_dwFlagsEx & TSFEX_LEFTRIGHT) {
					rc1.left = g_rcList.left;
					rc1.right = g_rcList.right;
				} else {
					rc1.left = g_rcPv.left;
					rc1.right = g_rcPv.right;		
				}
				rc2.left = rc1.left;
				rc2.right = rc1.right;
				rc1.top = g_rcPv.top;
				rc2.top = rc1.bottom = g_rcPv.bottom - 20;
				rc2.bottom = g_rcPv.bottom;

				if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
					if (GetThemeBackgroundRegion(g_htheme, NULL, 
						SPP_USERPANE, 0, &g_rcCap, &hrgn) != S_OK) __leave;
					if (!hrgn)
						hrgn = CreateRectRgnIndirect(&g_rcCap);
				}

				if (GetThemeBackgroundRegion(g_htheme, NULL, SPP_PLACESLIST, 0, 
					(g_dwFlagsEx & TSFEX_LEFTRIGHT) ? &g_rcPv : &g_rcList, &hrgnTmp) != S_OK) __leave;
				if (!hrgnTmp)
					hrgnTmp = CreateRectRgnIndirect((g_dwFlagsEx & TSFEX_LEFTRIGHT) ? &g_rcPv : &g_rcList);
				if (hrgn) {
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
					DeleteObject(hrgnTmp);
				} else hrgn = hrgnTmp;
				hrgnTmp = NULL;

				if (GetThemeBackgroundRegion(g_htheme, NULL, 
					SPP_PROGLIST, 0, &rc1, &hrgnTmp) != S_OK) __leave;
				if (!hrgnTmp)
					hrgnTmp = CreateRectRgnIndirect(&rc1);
				CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
				DeleteObject(hrgnTmp);
				hrgnTmp = NULL;

				if (GetThemeBackgroundRegion(g_htheme, NULL, 
					SPP_MOREPROGRAMS, 0, &rc2, &hrgnTmp) != S_OK) __leave;
				if (!hrgnTmp)
					hrgnTmp = CreateRectRgnIndirect(&rc2);
				CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);

				if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
					DeleteObject(hrgnTmp);
					hrgnTmp = NULL;

					if (GetThemeBackgroundRegion(g_htheme, NULL, 
						SPP_LOGOFF, 0, &g_rcPane, &hrgnTmp) != S_OK) __leave;
					if (!hrgnTmp)
						hrgnTmp = CreateRectRgnIndirect(&g_rcPane);
					CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR);
				}				
			}

			SetWindowRgn(hwndTs, hrgn, FALSE);

			fSuccess = TRUE;
		}
		__finally {
			if (hrgnTmp)
				DeleteObject(hrgnTmp);
			if (hrgn)
				DeleteObject(hrgn);
		}
        if (!fSuccess)
			return(fSuccess);
	} else { // classic style
		//if (dwStyleCl == TSEX_DEFAULTSTYLE)
		SetWindowRgn(hwndTs, NULL, FALSE);		
	}

	fSuccess = FALSE;

	HDC hdcMem = NULL, hdcMem2 = NULL, hdcScreen = NULL;
	HBITMAP hbitmapOld = NULL, hbitmapMem2 = NULL;

	__try {

		hdcScreen = GetDC(NULL);
		hdcMem = CreateCompatibleDC(hdcScreen);
		g_hbitmapTs = CreateCompatibleBitmap(hdcScreen, 
			g_rcTs.right - g_rcTs.left, g_rcTs.bottom - g_rcTs.top);
		if (!g_hbitmapTs)
			__leave;
		hbitmapOld = (HBITMAP)SelectObject(hdcMem, g_hbitmapTs);

		if (g_htheme) { // Windows XP style

			if (dwStyleXP == TSEX_POWERTOYMODE) {

				if (DrawThemeBackground(g_htheme, hdcMem, 
					SPP_USERPANE, 0, &rc1, NULL) != S_OK) __leave;

				hdcMem2 = CreateCompatibleDC(hdcScreen);
				hbitmapMem2 = CreateCompatibleBitmap(hdcScreen, rc1.right, rc1.bottom);
				DeleteObject(SelectObject(hdcMem2, hbitmapMem2));

				if (DrawThemeBackground(g_htheme, hdcMem2, 
					SPP_USERPANE, 0, &rc1, NULL) != S_OK) __leave;

				__asm {
					fild int ptr rc1.bottom
					fstp xForm.eDy
				}

				SetGraphicsMode(hdcMem2, GM_ADVANCED);
				SetMapMode(hdcMem2, MM_ANISOTROPIC);
				SetWorldTransform(hdcMem2, &xForm);

				BitBlt(hdcMem, 0, g_rcTs.bottom / 2, rc1.right, rc1.bottom, 
					hdcMem2, 0, 0, SRCCOPY);

				DeleteObject(hbitmapMem2);
				DeleteDC(hdcMem2);
				hdcMem2 = NULL;

			} else if (dwStyleXP == TSEX_SMWINDOWMODE) {

				if (DrawThemeBackground(g_htheme, hdcMem, 
					WP_SMALLCAPTION, CS_ACTIVE, &g_rcTs, NULL) != S_OK) __leave;

			} else if (dwStyleXP == TSEX_FLATXPMODE) {

				if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
					if (DrawThemeBackground(g_htheme, hdcMem, 
						EBP_HEADERBACKGROUND, 0, &g_rcCap, NULL) != S_OK) __leave;
				}
				if (DrawThemeBackground(g_htheme, hdcMem, 
					EBP_NORMALGROUPBACKGROUND, 0, &g_rcList, NULL) != S_OK) __leave;
				if (DrawThemeBackground(g_htheme, hdcMem, 
					EBP_SPECIALGROUPBACKGROUND, 0, &g_rcPv, NULL) != S_OK) __leave;
				if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
					if (DrawThemeBackground(g_htheme, hdcMem, 
						EBP_SPECIALGROUPHEAD, 0, &g_rcPane, NULL) != S_OK) __leave;
				}
				HBRUSH hbrFrame = CreateSolidBrush(g_crSel);
				FrameRect(hdcMem, &g_rcTs, hbrFrame);
				DeleteObject(hbrFrame);

			} else if (dwStyleXP == TSEX_SOFTXPMODE) {

				if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
					if (DrawThemeBackground(g_htheme, hdcMem, 
						TABP_TABITEM, TIS_HOT, &g_rcCap, NULL) != S_OK) __leave;
				}
				if (DrawThemeBackground(g_htheme, hdcMem, 
					TABP_PANE, 0, &g_rcList, NULL) != S_OK) __leave;
				if (DrawThemeBackground(g_htheme, hdcMem, 
					TABP_PANE, 0, &g_rcPv, NULL) != S_OK) __leave;
				
				if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
					if (DrawThemeBackground(g_htheme, hdcMem, 
						TABP_TABITEM, TIS_NORMAL, &g_rcPane, NULL) != S_OK) __leave;
				}

			} else if (dwStyleXP == TSEX_TASKBARMODE) {

				if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
					if (DrawThemeBackground(g_htheme, hdcMem, 
						TBP_BACKGROUNDBOTTOM, 0, &g_rcCap, NULL) != S_OK) __leave;
				}
				if (DrawThemeBackground(g_htheme, hdcMem, 
					TBP_BACKGROUNDLEFT, 0, &g_rcList, NULL) != S_OK) __leave;
				if (DrawThemeBackground(g_htheme, hdcMem, 
					TBP_BACKGROUNDRIGHT, 0, &g_rcPv, NULL) != S_OK) __leave;
				
				if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
					if (DrawThemeBackground(g_htheme, hdcMem, 
						TBP_BACKGROUNDTOP, 0, &g_rcPane, NULL) != S_OK) __leave;
				}
			} else { //if (dwStyleXP == TSEX_DEFAULTMODE)

				if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
					if (DrawThemeBackground(g_htheme, hdcMem, 
						SPP_USERPANE, 0, &g_rcCap, NULL) != S_OK) __leave;
				}

				if (DrawThemeBackground(g_htheme, hdcMem, SPP_PLACESLIST, 0, 
					(g_dwFlagsEx & TSFEX_LEFTRIGHT) ? &g_rcPv : &g_rcList, NULL) != S_OK) __leave;

				if (DrawThemeBackground(g_htheme, hdcMem, 
					SPP_PROGLIST, 0, &rc1, NULL) != S_OK) __leave;
				if (DrawThemeBackground(g_htheme, hdcMem, 
					SPP_MOREPROGRAMS, 0, &rc2, NULL) != S_OK) __leave;

				if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
					if (DrawThemeBackground(g_htheme, hdcMem, 
						SPP_LOGOFF, 0, &g_rcPane, NULL) != S_OK) __leave;
				}
			}
		} else { // classic style

			FillRect(hdcMem, &g_rcTs, GetSysColorBrush(COLOR_3DFACE));

			if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {
				TRIVERTEX pVertex[2];
				GRADIENT_RECT gRect;			
				pVertex[0].x = 0;
				pVertex[0].y = 0;
				COLORREF cr = GetSysColor(COLOR_ACTIVECAPTION);
				pVertex[0].Red = (COLOR16)((cr & 0xff) << 8);
				pVertex[0].Green = (COLOR16)(cr & 0xff00);
				pVertex[0].Blue = (COLOR16)((cr & 0xff0000) >> 8);
				pVertex[0].Alpha = 0x0000;

				pVertex[1].x = g_rcCap.right;
				pVertex[1].y = g_rcCap.bottom; 
				cr = GetSysColor(COLOR_GRADIENTACTIVECAPTION);
				pVertex[1].Red = (COLOR16)((cr & 0xff) << 8);
				pVertex[1].Green = (COLOR16)(cr & 0xff00);
				pVertex[1].Blue = (COLOR16)((cr & 0xff0000) >> 8);
				pVertex[1].Alpha = 0x0000;

				gRect.UpperLeft  = 0;
				gRect.LowerRight = 1;
				GradientFill(hdcMem, pVertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
			}

			DWORD dwStyle = (g_dwFlagsEx & TSFEX_LEFTRIGHT) ? BF_RIGHT : BF_LEFT;
			if (dwStyleCl == TSEX_FLATMODE)
				dwStyle |= BF_FLAT;
			DrawEdge(hdcMem, &g_rcList, EDGE_ETCHED, dwStyle);
			if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {
				DrawEdge(hdcMem, &g_rcPane, EDGE_ETCHED, 
					(dwStyleCl == TSEX_FLATMODE) ? (BF_FLAT | BF_TOP) : BF_TOP);
			}
			DrawEdge(hdcMem, &g_rcTs, EDGE_RAISED, 
				(dwStyleCl == TSEX_FLATMODE) ? (BF_FLAT | BF_RECT) : BF_RECT);
		}

		hdcMem2 = CreateCompatibleDC(hdcScreen);
		g_hbitmapList = CreateCompatibleBitmap(hdcScreen, 
			g_rcList.right - g_rcList.left, g_rcList.bottom - g_rcList.top);
		hbitmapMem2 = (HBITMAP)SelectObject(hdcMem2, g_hbitmapList);
		BitBlt(hdcMem2, 0, 0, g_rcList.right - g_rcList.left, 
			g_rcList.bottom - g_rcList.top, hdcMem, g_rcList.left, g_rcList.top, SRCCOPY);
		SelectObject(hdcMem2, hbitmapMem2);
		DeleteDC(hdcMem2);
		hdcMem2 = NULL;

		if (g_dwFlagsPv & TSFPV_DESKTOP || !(g_dwFlagsPv & TSFPV_OLDSTYLE)) {

			if (g_hbitmapWpEx) {
				hdcMem2 = CreateCompatibleDC(hdcScreen);
				hbitmapMem2 = (HBITMAP)SelectObject(hdcMem2, g_hbitmapWpEx);
				BitBlt(hdcMem, g_rcPvEx.left, g_rcPvEx.top, g_rcPvEx.right - g_rcPvEx.left, 
					g_rcPvEx.bottom - g_rcPvEx.top, hdcMem2, 0, 0, SRCCOPY);
				SelectObject(hdcMem2, hbitmapMem2);
				DeleteDC(hdcMem2);
				hdcMem2 = NULL;
			} else {
				FillRect(hdcMem, &g_rcPvEx, GetSysColorBrush(COLOR_DESKTOP));
			}

			rc1.left = g_rcPvEx.left - 1;
			rc1.top = g_rcPvEx.top - 1;
			rc1.right = g_rcPvEx.right + 1;
			rc1.bottom = g_rcPvEx.bottom + 1;

			FrameRect(hdcMem, &rc1, GetSysColorBrush(COLOR_DESKTOP));

			if (g_dwFlagsPv & TSFPV_DRAWBORDER) {
				InflateRect(&rc1, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
				if (g_htheme) {
					DrawThemeEdge(g_htheme, hdcMem, SPP_USERPANE, 0,
						&rc1, EDGE_SUNKEN, BF_RECT | BF_FLAT, NULL);
				} else {
					DrawEdge(hdcMem, &rc1, EDGE_SUNKEN, BF_RECT | BF_FLAT);
				}
			}
			if (g_dwFlagsPv & TSFPV_DESKTOP && g_dwFlagsPv & TSFPV_TASKBAR) {
				if (!(g_dwFlagsPv & TSFPV_EXCLUDELAYERED) || 
					!(GetWindowLongPtr(g_hwndTaskbar, GWL_EXSTYLE) & WS_EX_LAYERED))
					MyPrintWindow(g_hwndTaskbar, hdcMem, &g_rcPvEx, &g_rcScreen, MPW_DESKTOP);

				// add Google Sidebar to the preview
				HWND hwndGD = FindWindow(L"_GD_Sidebar", L"");
				if (IsWindow(hwndGD) && (!(g_dwFlagsPv & TSFPV_EXCLUDELAYERED) || 
					!(GetWindowLongPtr(hwndGD, GWL_EXSTYLE) & WS_EX_LAYERED)))
					MyPrintWindow(hwndGD, hdcMem, &g_rcPvEx, &g_rcScreen, MPW_DESKTOP);
				// hdcMem coordinates changed!!!
			}
		}

		fSuccess = TRUE;
	}
	__finally {
		if (hdcScreen)
			ReleaseDC(NULL, hdcScreen);
		if (hbitmapOld)
			SelectObject(hdcMem, hbitmapOld);
		DeleteDC(hdcMem);
	}
	return(fSuccess);
}

//-----------------------------------------------------------

void DestroyTsBackground() {
	if (g_htheme) {
		CloseThemeData(g_htheme);
		g_htheme = NULL;
	}
	if (g_hbitmapTs) {
		DeleteObject(g_hbitmapTs);
		g_hbitmapTs = NULL;
	}
	if (g_hbitmapList) {
		DeleteObject(g_hbitmapList);
		g_hbitmapList = NULL;
	}
}

//-----------------------------------------------------------

BOOL DrawListBackground(HDC hdc) {

	_ASSERT(g_hbitmapList);
	if (!g_hbitmapList)
		return(FALSE);

	BOOL fSuccess = FALSE;

	HDC hdcMem = CreateCompatibleDC(NULL);
	if (hdcMem) {
		HBITMAP hbitmapOld = (HBITMAP)SelectObject(hdcMem, g_hbitmapList);
		if (hbitmapOld) {
			fSuccess = BitBlt(hdc, g_rcList.left, g_rcList.top, g_rcList.right - g_rcList.left, 
				g_rcList.bottom - g_rcList.top, hdcMem, 0, 0, SRCCOPY);
			SelectObject(hdcMem, hbitmapOld);			
		}
		DeleteDC(hdcMem);
	}
	return(fSuccess);
}

//-----------------------------------------------------------

HBITMAP DrawTaskPreview(const PVDATA* pvData) {

	BOOL fSuccess = FALSE;
	HDC hdcMem = NULL;
	HBITMAP hbitmapPv = NULL, hbitmapOld = NULL;

	RECT rcPv;
	rcPv.left = rcPv.top = 0;
	rcPv.right = g_rcPvEx.right - g_rcPvEx.left;
	rcPv.bottom = g_rcPvEx.bottom - g_rcPvEx.top;

	if (!g_hbitmapTs)
		return(NULL);

	__try {
        
		HDC hdcScreen = GetWindowDC(pvData->hwndOwner);
		hdcMem = CreateCompatibleDC(hdcScreen);
		hbitmapPv = CreateCompatibleBitmap(hdcScreen, rcPv.right, rcPv.bottom);
		ReleaseDC(pvData->hwndOwner, hdcScreen);

		hbitmapOld = (HBITMAP)SelectObject(hdcMem, hbitmapPv);
		if (!hbitmapOld)
			__leave;

		HDC hdcMem2 = CreateCompatibleDC(NULL);
		if (!hdcMem2) __leave;
		HBITMAP hbitmapOld2 = (HBITMAP)SelectObject(hdcMem2, g_hbitmapTs);
		if (hbitmapOld2) {
			fSuccess = BitBlt(hdcMem, 0, 0, rcPv.right, rcPv.bottom,
				hdcMem2, g_rcPvEx.left, g_rcPvEx.top, SRCCOPY);
			SelectObject(hdcMem2, hbitmapOld2);
		}
		DeleteDC(hdcMem2);

		if (!fSuccess)
			__leave;

		fSuccess = FALSE;

		int nWnd;
		HWND hwndPv = NULL;
		RECT rcWnd, rcWork = g_rcScreen;
		BOOL fPrintWindow = FALSE, fIsHung = FALSE;

		if (!(g_dwFlagsPv & TSFPV_DESKTOP) && !(g_dwFlagsPv & TSFPV_OLDSTYLE)) {
			rcPv.left += 5;
			rcPv.top += 5;
			rcPv.right -= 5;
			rcPv.bottom -= 5;
		}

        BOOL fIsIconic = IsIconic(pvData->hwndOwner);
		if (fIsIconic || (!(g_dwFlagsPv & TSFPV_DESKTOP) && g_dwFlagsPv & TSFPV_POPUPONLY)) {

			if (fIsIconic) {
				if (g_dwFlagsPv & TSFPV_SHOWMINIMIZED && 
					IsWindowVisible(pvData->hwndOwner))
					hwndPv = pvData->hwndOwner;
			} else {
				nWnd = 0;
				while (nWnd < pvData->nWndPv) {
					if (!(pvData->dwFlags & (1 << nWnd)) && 
						IsWindow(pvData->phWndPv[nWnd]) && 
						IsWindowVisible(pvData->phWndPv[nWnd])) {
						hwndPv = pvData->phWndPv[nWnd];
						break;
					}
					nWnd++;
				}
			}
			if (hwndPv) {
				GetWindowRect(hwndPv, &rcWork);
				if ((rcWork.right - rcWork.left < rcPv.right - rcPv.left) &&
					(rcWork.bottom - rcWork.top < rcPv.bottom - rcPv.top)) {

						rcPv.left = (rcPv.right + rcPv.left - (rcWork.right - rcWork.left)) / 2;
						rcPv.right = rcPv.left + (rcWork.right - rcWork.left);
						if (g_dwFlagsPv & TSFPV_VCENTER || (fIsIconic && g_dwFlagsPv & TSFPV_DESKTOP))
							rcPv.top = (rcPv.bottom + rcPv.top - (rcWork.bottom - rcWork.top)) / 2;
						rcPv.bottom = rcPv.top + (rcWork.bottom - rcWork.top);
					}
					fIsHung = MyIsHungAppWindow(hwndPv);
					if (!fIsHung) {
						if (!(g_dwFlagsPv & TSFPV_EXCLUDELAYERED) || 
							!(GetWindowLongPtr(hwndPv, GWL_EXSTYLE) & WS_EX_LAYERED)) {
							fPrintWindow = MyPrintWindow(hwndPv, hdcMem, &rcPv, 
								&rcWork, (g_dwFlagsPv & TSFPV_OLDSTYLE && !(g_dwFlagsPv & TSFPV_VCENTER)) 
								? MPW_NOCHECKPOS | MPW_HCENTER : MPW_NOCHECKPOS | MPW_HCENTER | MPW_VCENTER);
							if (fPrintWindow)
								fIsIconic = FALSE;
						}
					}
			}
		} else {

			if (!(g_dwFlagsPv & TSFPV_DESKTOP)) {
				nWnd = 0;
				while (nWnd < pvData->nWndPv) {
					hwndPv = pvData->phWndPv[nWnd];
					if (!(pvData->dwFlags & (1 << nWnd)) && 
						IsWindow(hwndPv) && IsWindowVisible(hwndPv)) {
						GetWindowRect(hwndPv, &rcWork);
						if (g_rcScreen.left <= rcWork.right && g_rcScreen.top <= rcWork.bottom &&
							g_rcScreen.bottom >= rcWork.top && g_rcScreen.right >= rcWork.left) break;
					}
					nWnd++;
				}
				//if (nWnd >= pvData->nWndPv)
				//	__leave;
				nWnd++;
				while (nWnd < pvData->nWndPv) {
					hwndPv = pvData->phWndPv[nWnd];
					if (!(pvData->dwFlags & (1 << nWnd)) && 
						IsWindow(hwndPv) && IsWindowVisible(hwndPv)) {
						GetWindowRect(hwndPv, &rcWnd);
						if ((g_rcScreen.left <= rcWnd.right && g_rcScreen.top <= rcWnd.bottom) && 
							(g_rcScreen.bottom >= rcWnd.top && g_rcScreen.right >= rcWnd.left)) {

							if (rcWork.left > rcWnd.left)
								rcWork.left = rcWnd.left;
							if (rcWork.top > rcWnd.top)
								rcWork.top = rcWnd.top;
							if (rcWork.right < rcWnd.right)
								rcWork.right = rcWnd.right;
							if (rcWork.bottom < rcWnd.bottom)
								rcWork.bottom = rcWnd.bottom;
						}
					}
					nWnd++;
				}
				if ((rcWork.right - rcWork.left < rcPv.right - rcPv.left) &&
					(rcWork.bottom - rcWork.top < rcPv.bottom - rcPv.top)) {

					rcPv.left = (rcPv.right + rcPv.left - (rcWork.right - rcWork.left)) / 2;
					rcPv.right = rcPv.left + (rcWork.right - rcWork.left);
					rcPv.top = (rcPv.bottom + rcPv.top - (rcWork.bottom - rcWork.top)) / 2;
					rcPv.bottom = rcPv.top + (rcWork.bottom - rcWork.top);
				}
			}

			DWORD dwFlagsPv = (g_dwFlagsPv & TSFPV_DESKTOP) ? MPW_DESKTOP 
				: ((g_dwFlagsPv & TSFPV_OLDSTYLE && !(g_dwFlagsPv & TSFPV_VCENTER)) 
				? MPW_HCENTER : (MPW_HCENTER | MPW_VCENTER));

			nWnd = pvData->nWndPv - 1;
			while (nWnd >= 0) {
				hwndPv = pvData->phWndPv[nWnd];
				if (!(pvData->dwFlags & (1 << nWnd)) && 
					IsWindow(hwndPv) && IsWindowVisible(hwndPv)) {
					fIsHung = MyIsHungAppWindow(hwndPv);
					if (fIsHung) break;
					if (!(g_dwFlagsPv & TSFPV_EXCLUDELAYERED) || 
						!(GetWindowLongPtr(hwndPv, GWL_EXSTYLE) & WS_EX_LAYERED)) {
						fPrintWindow += MyPrintWindow(hwndPv, hdcMem, &rcPv, &rcWork, dwFlagsPv);
					}
				}
				nWnd--;
			}
		}

		if (fIsIconic || !fPrintWindow) {

			rcPv.left = (g_rcPvEx.right - g_rcPvEx.left - CX_ICON) / 2;
			rcPv.top = (g_rcPvEx.bottom - g_rcPvEx.top - CY_ICON) / 2;
			DrawIconEx(hdcMem, rcPv.left, rcPv.top, 
				pvData->hIcon, CX_ICON, CY_ICON, 0, NULL, DI_NORMAL);

			rcPv.left += CX_ICON - CX_SMICON / 2;
			rcPv.top += CY_ICON - CX_SMICON / 2;

			if (fIsIconic) {
				if (pvData->dwFlags & TI_TRAYWINDOW)
					rcPv.top -= CY_ICON;

				rcPv.right = rcPv.left + CX_SMICON;
				rcPv.bottom = rcPv.top + CX_SMICON;

				HTHEME hthemeWnd = !(g_dwFlagsEx & TSFEX_NOVISUALSTYLES) 
					? OpenThemeData(g_hwndTs, L"Window") : NULL;
				if (hthemeWnd) {
					DrawThemeBackground(hthemeWnd, hdcMem, WP_MINBUTTON, MINBS_NORMAL, &rcPv, NULL);
					CloseThemeData(hthemeWnd);
				} else { // draw classic min button
					DrawFrameControl(hdcMem, &rcPv, DFC_CAPTION, DFCS_CAPTIONMIN);
				}
			} else if (fIsHung) {
				DrawIconEx(hdcMem, rcPv.left, rcPv.top, LoadIcon(NULL, IDI_EXCLAMATION), 
					CX_SMICON, CY_SMICON, 0, NULL, DI_NORMAL);
			} else if (!IsWindow(pvData->hwndOwner)) {
				DrawIconEx(hdcMem, rcPv.left, rcPv.top, LoadIcon(NULL, IDI_ERROR), 
					CX_SMICON, CY_SMICON, 0, NULL, DI_NORMAL);
			}
		}

		fSuccess = TRUE;
	}
	__finally {
		if (hbitmapOld)
			SelectObject(hdcMem, hbitmapOld);
		//if (hdcMem)
		DeleteDC(hdcMem);
	}	

	return(hbitmapPv);
}

BOOL StartDrawTaskPreview(int nTask) {

	_ASSERT(g_pTs && nTask >= 0 && nTask < g_nTs);

	if (WaitForSingleObject(g_hmtxPv, 0) != WAIT_OBJECT_0)
		return(FALSE);

	if (g_hbitmapPv) {
		ReleaseMutex(g_hmtxPv);
		return(FALSE);
	}

	g_pvData.hwndOwner = g_pTs[nTask].hwndOwner;
	g_pvData.nWndPv = g_pTs[nTask].nWndPv;
	for (int i = 0; i < g_pTs[nTask].nWndPv; i++)
		g_pvData.phWndPv[i] = g_pTs[nTask].phWndPv[i];
	g_pvData.dwFlags = g_pTs[nTask].dwFlags;
	g_pvData.hIcon = g_pTs[nTask].hIcon;

	ReleaseMutex(g_hmtxPv);
	SetEvent(g_hevtPv);

	return(TRUE);
}


BOOL DrawCachedPreview(HDC hdc, HBITMAP hbitmapPv) {

	//_ASSERT(hbitmapPv);
    if (!hbitmapPv)
		return(FALSE);

	BOOL fSuccess = FALSE;

	HDC hdcMem = CreateCompatibleDC(NULL);
	if (hdcMem) {
		HBITMAP hbitmapOld = (HBITMAP)SelectObject(hdcMem, hbitmapPv);
		if (hbitmapOld) {
			fSuccess = BitBlt(hdc, g_rcPvEx.left, g_rcPvEx.top, g_rcPvEx.right - g_rcPvEx.left,
				g_rcPvEx.bottom - g_rcPvEx.top, hdcMem, 0, 0, SRCCOPY);
			SelectObject(hdcMem, hbitmapOld);
		}
		DeleteDC(hdcMem);
	}
	return(fSuccess);
}

//-----------------------------------------------------------

void OnPaint(HDC hdc) {

	if (g_nCurTs < 0 || g_nCurTs >= g_nTs)
		return;

	SetBkMode(hdc, TRANSPARENT);

	RECT rc;
	int n = g_nCurTs - g_nFirstTs;
	if (n >= 0 && n < g_nIconsX * g_nIconsY) {

		if (g_dwFlagsList & TSFL_VERTICALLIST) {
			rc.left = g_rcList.left + XMARGIN_LIST + 1;
			rc.right = g_rcList.right - XMARGIN_LIST - 1;
		} else {
			if (g_dwFlagsList & TSFL_SMALLICONS) {
				rc.left = g_rcList.left + XMARGIN_LIST + 1 + \
					(CX_SMICON + 2 * (XMARGIN_SMSEL + 1)) * (n % g_nIconsX);
				rc.right = rc.left + CX_SMICON + 2 * XMARGIN_SMSEL;
			} else {
				rc.left = g_rcList.left + XMARGIN_LIST + 1 + \
					(CX_ICON + 2 * (XMARGIN_SEL + 1)) * (n % g_nIconsX);
				rc.right = rc.left + CX_ICON + 2 * XMARGIN_SEL;
			}
		}
		if (g_dwFlagsList & TSFL_SMALLICONS) {
			rc.top = g_rcList.top + YMARGIN_LIST + 1 + \
				(CY_SMICON + 2 * (YMARGIN_SMSEL + 1)) * (n / g_nIconsX);
			rc.bottom = rc.top + CY_SMICON + 2 * YMARGIN_SMSEL;
		} else {
			rc.top = g_rcList.top + YMARGIN_LIST + 1 + (CY_ICON + \
				2 * (YMARGIN_SEL + 1)) * (n / g_nIconsX);
			rc.bottom = rc.top + CY_ICON + 2 * YMARGIN_SEL;
		}

		DrawItemSelection(hdc, &rc, 
			(g_pTs[g_nCurTs].dwFlags & TI_SELECTED) ? g_crSelMark : g_crSel);

		if (g_dwFlagsList & TSFL_DRAWFOCUSRECT) {
			SetTextColor(hdc, g_crText);
			InflateRect(&rc, 1, 1);
			DrawFocusRect(hdc, &rc);
			InflateRect(&rc, -1, -1);
		}

		if (g_dwFlagsList & TSFL_SMALLICONS) {
			InflateRect(&rc, -XMARGIN_SMSEL, -YMARGIN_SMSEL);
		} else {
			InflateRect(&rc, -XMARGIN_SEL, -YMARGIN_SEL);
		}
		if (g_dwFlagsList & TSFL_VERTICALLIST) {
			HFONT hfontOld = (HFONT)SelectObject(hdc, 
				g_hfontList ? g_hfontList : GetStockObject(DEFAULT_GUI_FONT));
			SetTextColor(hdc, (g_pTs[g_nCurTs].dwFlags & TI_SELECTED) ? g_crSelMarkText : g_crSelText);
			DrawTaskListItem(hdc, g_nCurTs, &rc, (BOOL)(g_dwFlagsList & TSFL_SMALLICONS));
			SelectObject(hdc, hfontOld);
		} else {
			DrawIconEx(hdc, rc.left, rc.top, (g_dwFlagsList & TSFL_SMALLICONS) 
				? g_pTs[g_nCurTs].hIconSm : g_pTs[g_nCurTs].hIcon, 
				rc.bottom - rc.top, rc.bottom - rc.top, 0, NULL, DI_NORMAL);
		}		
	}

	if (!(g_dwFlagsEx & TSFEX_REMOVECAPTION)) {

		if (!(g_dwFlagsEx & TSFEX_CAPNOICON)) {
			if (g_dwFlagsEx & TSFEX_CAPSMALLICON) {
				DrawIconEx(hdc, g_rcCap.left + XMARGIN_SMICON, 
					g_rcCap.top + YMARGIN_SMICON, g_pTs[g_nCurTs].hIconSm, 
					CX_SMICON, CY_SMICON, 0, NULL, DI_NORMAL);
			} else {
				DrawIconEx(hdc, g_rcCap.left + XMARGIN_ICON, 
					g_rcCap.top + YMARGIN_ICON, g_pTs[g_nCurTs].hIcon, 
					CX_ICON, CY_ICON, 0, NULL, DI_NORMAL);
			}
		}

		if (g_pTs[g_nCurTs].szCaption[0] != L'\0') {

			if (g_dwFlagsEx & TSFEX_CAPSMALLICON) {
				rc.left = g_rcCap.left + XMARGIN_SMICON;
				if (!(g_dwFlagsEx & TSFEX_CAPNOICON))
					rc.left += CX_SMICON + XMARGIN_SMICON;
				rc.right = g_rcCap.right - XMARGIN_SMICON;
				rc.top = g_rcCap.top + YMARGIN_SMICON;
				rc.bottom = g_rcCap.bottom - YMARGIN_SMICON;
			} else {
				rc.left = g_rcCap.left + XMARGIN_ICON;
				if (!(g_dwFlagsEx & TSFEX_CAPNOICON))
					rc.left += CX_ICON + XMARGIN_ICON;
				rc.right = g_rcCap.right - XMARGIN_ICON;
				rc.top = g_rcCap.top + YMARGIN_ICON;
				rc.bottom = g_rcCap.bottom - YMARGIN_ICON;
			}
			HFONT hfontOld = (HFONT)SelectObject(hdc, g_hfontCap);
			MyDrawShadowText(hdc, g_pTs[g_nCurTs].szCaption, &rc,
				DT_VCENTER | DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS, 
				g_crCapText, g_dwCapShadow);
			SelectObject(hdc, hfontOld);
		}
	}

	// draw cached preview
	//if (g_pTs[g_nCurTs].hbitmapPv)
		DrawCachedPreview(hdc, g_pTs[g_nCurTs].hbitmapPv);

	if (!(g_dwFlagsEx & TSFEX_REMOVEPANE)) {

		HFONT hfontOld = (HFONT)SelectObject(hdc, g_hfontPane 
			? g_hfontPane : GetStockObject(DEFAULT_GUI_FONT));

		rc.top = g_rcPane.top + YMARGIN_PANE;
		rc.bottom = g_rcPane.bottom;

		SetTextColor(hdc, g_crPaneText);
		WCHAR szInfo[64];

		rc.left = g_rcPv.left + XMARGIN_PANE;
		rc.right = g_rcPv.right - XMARGIN_PANE;		

		if (g_pTs[g_nCurTs].szTaskInfo[0] == L'\0' && g_pTs[g_nCurTs].pszExePath) {
			DWORD dwFormat = g_dwFlagsPane & TSP_FORMATMASK;
			switch (dwFormat) {
				case TSP_FORMATPID:
					wsprintf(g_pTs[g_nCurTs].szTaskInfo, 
						L"PID:  %u", g_pTs[g_nCurTs].dwProcessId);
					g_pTs[g_nCurTs].szTaskInfo[MAX_TASKINFO - 1] = L'\0';
					break;
				case TSP_FORMATNAME:
					lstrcpyn(g_pTs[g_nCurTs].szTaskInfo, 
						g_pTs[g_nCurTs].pszExePath + g_pTs[g_nCurTs].nExeName, MAX_TASKINFO);
					g_pTs[g_nCurTs].szTaskInfo[MAX_TASKINFO - 1] = L'\0';
					break;
				case TSP_FORMATNAMEPID: {
					lstrcpyn(g_pTs[g_nCurTs].szTaskInfo, 
						g_pTs[g_nCurTs].pszExePath + g_pTs[g_nCurTs].nExeName, MAX_TASKINFO);
					g_pTs[g_nCurTs].szTaskInfo[MAX_TASKINFO - 1] = L'\0';
					int len = lstrlen(g_pTs[g_nCurTs].szTaskInfo);
					wsprintf(szInfo, L"  [%u]", g_pTs[g_nCurTs].dwProcessId);
					lstrcpyn(g_pTs[g_nCurTs].szTaskInfo + len, szInfo, MAX_TASKINFO - len);
					g_pTs[g_nCurTs].szTaskInfo[MAX_TASKINFO - 1] = L'\0';
					break;
										}
				case TSP_FORMATDESCR: {
					DWORD dw = GetFileVersionInfoSize(g_pTs[g_nCurTs].pszExePath, &dw);
					if (dw) {
						PVOID pBlock = HeapAlloc(GetProcessHeap(), 0, dw);
						if (GetFileVersionInfo(g_pTs[g_nCurTs].pszExePath, 0, dw, pBlock)) {
							struct LANGANDCODEPAGE {
								WORD wLanguage;
								WORD wCodePage;
							} *pTranslate;
							if (VerQueryValue(pBlock, L"\\VarFileInfo\\Translation", (PVOID*)&pTranslate, 
								(PUINT)&dw) && dw >= sizeof(LANGANDCODEPAGE)) {
								wsprintf(szInfo, L"\\StringFileInfo\\%04x%04x\\FileDescription", 
									pTranslate[0].wLanguage, pTranslate[0].wCodePage);							
								PWSTR pDescription = NULL;
								if (VerQueryValue(pBlock, szInfo, (PVOID*)&pDescription, (PUINT)&dw)) {								
									lstrcpyn(g_pTs[g_nCurTs].szTaskInfo, pDescription, MAX_TASKINFO);
									g_pTs[g_nCurTs].szTaskInfo[MAX_TASKINFO - 1] = L'\0';
								}
							}
						}
						HeapFree(GetProcessHeap(), 0, pBlock);
					}
					if (g_pTs[g_nCurTs].szTaskInfo[0] == L'\0') {
						lstrcpyn(g_pTs[g_nCurTs].szTaskInfo, 
							g_pTs[g_nCurTs].pszExePath + g_pTs[g_nCurTs].nExeName, MAX_TASKINFO);
					}
					break;
									  }
			}
		}

		if (g_pTs[g_nCurTs].szTaskInfo[0] != L'\0') {
			MyDrawShadowText(hdc, g_pTs[g_nCurTs].szTaskInfo, &rc,(g_dwFlagsEx & TSFEX_LEFTRIGHT) 
				? (DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS) 
				: (DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS),
				g_crPaneText, g_dwPaneShadow);
		}

		if (g_dwFlagsPane & TSFP_TASKNUMBER) {
			rc.left = g_rcList.left + XMARGIN_PANE;
			rc.right = g_rcList.right - XMARGIN_PANE;
			if (g_nSelTs > 0)
				wsprintf(szInfo, L"%d/%d(%d)", g_nCurTs + 1, g_nTs, g_nSelTs);
			else wsprintf(szInfo, L"%d/%d", g_nCurTs + 1, g_nTs);
			MyDrawShadowText(hdc, szInfo, &rc,(g_dwFlagsEx & TSFEX_LEFTRIGHT) 
				? (DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS) 
				: (DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS),
				g_crPaneText, g_dwPaneShadow);
		}
		//klvov: draw content of g_szTypeBuffer
		if (g_dwFlagsList & TSFL_ALTMODE)
		{
			rc.left = g_rcList.left;
			rc.right = g_rcList.right - 100 ; // ? may be this should be done in the other way?
			MyDrawShadowText( hdc, g_szTypeBuffer, &rc,(g_dwFlagsEx & TSFEX_LEFTRIGHT) 
				? (DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS) 
				: (DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS),
				g_crPaneText, g_dwPaneShadow);
		}
		SelectObject(hdc, hfontOld);
	}
}

//-----------------------------------------------------------

void DrawTaskListItem(HDC hdc, int nTask, const RECT* prcItem, BOOL fSmall) {

	HICON hIcon;
	int nWidth, nHeight;
	if (fSmall) {
		hIcon = g_pTs[nTask].hIconSm;
		nWidth = CX_SMICON;
		nHeight = CY_SMICON;
	} else {
		hIcon = g_pTs[nTask].hIcon;
		nWidth = CX_ICON;
		nHeight = CY_ICON;
	}
	DrawIconEx(hdc, prcItem->left, (prcItem->top + prcItem->bottom - nHeight) / 2,
		hIcon, nWidth, nHeight, 0, NULL, DI_NORMAL);

	//WCHAR szText[MAX_CAPTION] = L"";
	//GetTaskText(nTask, szText, SIZEOF_ARRAY(szText));

	int len = lstrlen(g_pTs[nTask].szCaption);
	if (len <= 0) return;

	SIZE size;
	RECT rcText;
	rcText.left = (fSmall) ? (prcItem->left + CX_SMICON + XMARGIN_SMSEL + 1) 
		: (prcItem->left + CX_ICON + XMARGIN_SEL + 1);
	rcText.right = prcItem->right;
	rcText.top = prcItem->top;
	rcText.bottom = prcItem->bottom;

	nWidth = rcText.right - rcText.left;

	int *pFit = (int*)HeapAlloc(GetProcessHeap(), 0, len * sizeof(int));
	if (pFit) {

		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);

		LPWSTR p = g_pTs[nTask].szCaption;
		int nLines = 0;
		nHeight = rcText.bottom - rcText.top;
		while (nHeight >= tm.tmHeight && len > 0 && 
			GetTextExtentExPoint(hdc, p, len, nWidth, &pFit[nLines], NULL, &size)) {

			int nFit = pFit[nLines];
			if (p[nFit] != L'\0' && p[nFit] != L' ') {
				while (nFit >= 0 && p[nFit] != L' ')
					nFit--;
				if (nFit > 0)
					pFit[nLines] = nFit + 1;
			}
			nHeight -= tm.tmHeight + tm.tmExternalLeading;
			len -= pFit[nLines];
			p += pFit[nLines];
			nLines++;
		}

		//klvov:  choosing basing on task switching mode
		if (g_dwFlagsList & TSFL_ALTMODE) p = g_pTs[nTask].szModuleName;
		else p = g_pTs[nTask].szCaption;
		if (nLines > 0) {
			nHeight = nLines * (tm.tmHeight + tm.tmExternalLeading) - tm.tmExternalLeading;
			rcText.top = (rcText.bottom + rcText.top - nHeight) / 2;
			rcText.bottom = rcText.top + nHeight;

			int nLine = 0;
			while (nLine < nLines - 1) {
				DrawText(hdc, p, pFit[nLine], &rcText,
					DT_LEFT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
				p += pFit[nLine];
				nLine++;
				rcText.top += tm.tmHeight + tm.tmExternalLeading;
			}			
		}
		DrawText(hdc, p, -1, &rcText, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS);
	}
	HeapFree(GetProcessHeap(), 0, (PVOID)pFit);
}


void DrawItemSelection(HDC hdc, const RECT* prcItem, COLORREF crSel) {

	HBRUSH hbr = CreateSolidBrush(crSel);

	switch (g_dwFlagsList & TSL_SELSTYLEMASK) {

		case TSL_SELROUNDSTYLE: {
			HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hbr);
			int n = (g_dwFlagsList & TSFL_SMALLICONS) ? (CY_SMICON / 2) : (CY_ICON / 2);
			HPEN hpen = CreatePen(PS_SOLID, 0, crSel);
			HPEN hpenOld = (HPEN)SelectObject(hdc, hpen);
			RoundRect(hdc, prcItem->left, prcItem->top, 
				prcItem->right, prcItem->bottom, n, n);
			SelectObject(hdc, hpenOld);
			DeleteObject(hpen);
			SelectObject(hdc, hbrOld);
			break;
								}

		case TSL_SELFRAMESTYLE:
			FrameRect(hdc, prcItem, hbr);
			if (!(g_dwFlagsList & TSFL_SMALLICONS)) {
				RECT rc;
				rc.left = prcItem->left + 1;
				rc.top = prcItem->top + 1;
				rc.right = prcItem->right - 1;
				rc.bottom = prcItem->bottom - 1;
				FrameRect(hdc, &rc, hbr);
			}
			break;
		default: //case TSL_SELSOLIDSTYLE:
			FillRect(hdc, prcItem, hbr);
			break;
	}
	DeleteObject(hbr);
}


BOOL DrawTaskList() {

	_ASSERT(g_hbitmapTs);

	BOOL fSuccess = FALSE;
	HDC hdcMem = NULL;
	HBITMAP hbitmapOld = NULL;

	__try {

		hdcMem = CreateCompatibleDC(NULL);
		if (!hdcMem)
			__leave;
		hbitmapOld = (HBITMAP)SelectObject(hdcMem, g_hbitmapTs);

		if (!DrawListBackground(hdcMem))
			__leave;

		RECT rcItem, rc;
		int nTask, cIcon, dySel, dxSel;

		if (g_dwFlagsList & TSFL_SMALLICONS) {
			dxSel = XMARGIN_SMSEL + 1;
			dySel = YMARGIN_SMSEL + 1;
			cIcon = CY_SMICON;
		} else {			
			dxSel = XMARGIN_SEL + 1;
			dySel = YMARGIN_SEL + 1;
			cIcon = CY_ICON;
		}

		if (g_dwFlagsList & TSFL_VERTICALLIST) {

			rcItem.left = g_rcList.left + XMARGIN_LIST + dxSel;
			rcItem.right = g_rcList.right - XMARGIN_LIST - dxSel;
			rcItem.top = g_rcList.top + YMARGIN_LIST + dySel;
			rcItem.bottom = rcItem.top + cIcon;			

			POINT pPoints[3];
			HBRUSH hbr = NULL, hbrOld = NULL;
			HPEN hpen = NULL, hpenOld = NULL;

			nTask = g_nFirstTs;
			if (g_nFirstTs != 0) {
				/*hIcon = (HICON)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_VMORETOP), 
					IMAGE_ICON, nHeight, nHeight, LR_DEFAULTCOLOR);
				DrawIconEx(hdcMem, (rcItem.left + rcItem.right - nHeight) / 2, rcItem.top,
					hIcon, nHeight, nHeight, 0, NULL, DI_NORMAL);
				DestroyIcon(hIcon);*/

				pPoints[0].y = pPoints[2].y = rcItem.bottom - cIcon / 3;
				pPoints[1].y = pPoints[0].y - cIcon / 4;
				pPoints[1].x = (rcItem.left + rcItem.right) / 2;
				pPoints[0].x = pPoints[1].x - cIcon / 4;
				pPoints[2].x = pPoints[1].x + cIcon / 4;

				hpen = CreatePen(PS_SOLID, 1, g_crText);
				hpenOld = (HPEN)SelectObject(hdcMem, hpen);
				hbr = CreateSolidBrush(g_crText);
				hbrOld = (HBRUSH)SelectObject(hdcMem, hbr);

				Polygon(hdcMem, pPoints, SIZEOF_ARRAY(pPoints));

				rcItem.top = rcItem.bottom + 2 * dySel;
				rcItem.bottom = rcItem.top + cIcon;
				nTask++;

			}

			SetBkMode(hdcMem, TRANSPARENT);
			HFONT hfontOld = (HFONT)SelectObject(hdcMem, g_hfontList 
				? g_hfontList : GetStockObject(DEFAULT_GUI_FONT));

			int n = g_nFirstTs + g_nIconsY;
			if (n < g_nTs) n--;
			for (; nTask < MIN(n, g_nTs); nTask++) {
				if (g_pTs[nTask].dwFlags & TI_SELECTED) {
					rc.left = rcItem.left - (dxSel - 1);
					rc.top = rcItem.top - (dySel - 1);
					rc.right = rcItem.right + (dxSel - 1);
					rc.bottom = rcItem.bottom + (dySel - 1);
					DrawItemSelection(hdcMem, &rc, g_crMark);
					SetTextColor(hdcMem, g_crMarkText);
				} else 
					SetTextColor(hdcMem, g_crText);
				DrawTaskListItem(hdcMem, nTask, &rcItem, g_dwFlagsList & TSFL_SMALLICONS);
				rcItem.top = rcItem.bottom + 2 * dySel;
				rcItem.bottom = rcItem.top + cIcon;
			}

			if (nTask < g_nTs - 1){
				
				pPoints[0].y = pPoints[2].y = rcItem.top + cIcon / 3;
				pPoints[1].y = pPoints[0].y + cIcon / 4;
				pPoints[1].x = (rcItem.left + rcItem.right) / 2;
				pPoints[0].x = pPoints[1].x - cIcon / 4;
				pPoints[2].x = pPoints[1].x + cIcon / 4;

				if (!hpen) {
					hpen = CreatePen(PS_SOLID, 1, g_crText);
					hpenOld = (HPEN)SelectObject(hdcMem, hpen);
				}
				if (!hbr) {
					hbr = CreateSolidBrush(g_crText);
					hbrOld = (HBRUSH)SelectObject(hdcMem, hbr);
				}
				Polygon(hdcMem, pPoints, SIZEOF_ARRAY(pPoints));
			}
			if (hpen) {
				SelectObject(hdcMem, hpenOld);
				DeleteObject(hpen);
			}
			if (hbr) {
				SelectObject(hdcMem, hbrOld);
				DeleteObject(hbr);
			}
			SelectObject(hdcMem, hfontOld);
		} else {

			HICON hIconMore = NULL;

			nTask = 0;
			if (g_nFirstTs != 0) {
				hIconMore = (HICON)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_MORE), 
					IMAGE_ICON, cIcon, cIcon, LR_DEFAULTCOLOR);
				DrawIconEx(hdcMem, g_rcList.left + XMARGIN_LIST + dxSel,
					g_rcList.top + YMARGIN_LIST + dySel, 
					hIconMore, cIcon, cIcon, 0, NULL, DI_NORMAL);
				nTask++;
			}
			int n = g_nIconsX * g_nIconsY;
			if (g_nFirstTs + n < g_nTs)
				n--;
			for (; nTask < MIN(n, g_nTs - g_nFirstTs); nTask++) {
				rcItem.left = g_rcList.left + XMARGIN_LIST + 1 + (cIcon + 2 * dxSel) * (nTask % g_nIconsX);
				rcItem.top = g_rcList.top + YMARGIN_LIST + 1 + (cIcon + 2 * dySel) * (nTask / g_nIconsX);
				if (g_pTs[g_nFirstTs + nTask].dwFlags & TI_SELECTED) {
					rcItem.right = rcItem.left + cIcon + 2 * dxSel - 2;
					rcItem.bottom = rcItem.top + cIcon + 2 * dySel - 2;
					DrawItemSelection(hdcMem, &rcItem, g_crMark);
				}
				DrawIconEx(hdcMem, rcItem.left + dxSel - 1, rcItem.top + dySel - 1, 
					(g_dwFlagsList & TSFL_SMALLICONS) ? g_pTs[g_nFirstTs + nTask].hIconSm 
					: g_pTs[g_nFirstTs + nTask].hIcon, cIcon, cIcon, 0, NULL, DI_NORMAL);
			}
			if (g_nFirstTs + nTask < g_nTs - 1){
				if (!hIconMore) {
					hIconMore = (HICON)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_MORE), 
						IMAGE_ICON, cIcon, cIcon, LR_DEFAULTCOLOR);
				}
				DrawIconEx(hdcMem, g_rcList.left + XMARGIN_LIST + (cIcon + 2 * dxSel) * (g_nIconsX - 1), 
					g_rcList.top + YMARGIN_LIST + (cIcon + 2 * dySel) * (g_nIconsY - 1), 
					hIconMore, cIcon, cIcon, 0, NULL, DI_NORMAL);
			}
			if (hIconMore)
				DestroyIcon(hIconMore);
		}

		fSuccess = TRUE;
	}
	__finally {
		if (hdcMem) {
			SelectObject(hdcMem, hbitmapOld);
			DeleteDC(hdcMem);
		}
	}
	return(fSuccess);
}

//-----------------------------------------------------------
//-----------------------------------------------------------

int _NextTask(int nCurTs, int nNext, PBOOL pfRedraw) {

	_ASSERT(nNext == 1 || nNext == -1);

	if (pfRedraw)
		*pfRedraw = FALSE;
	if (!g_nTs)
		return(0);

	int nTask = nCurTs;
	HWND hwndPv = NULL;
	do { // find window
		nTask = (nTask + nNext) % g_nTs;
		if (nTask < 0)
			nTask += g_nTs;
		if (CheckTask(nTask, &hwndPv)) {
			if (pfRedraw)
				*pfRedraw = TRUE;
			if (!hwndPv) {
				DeleteTsItem(nTask);
				nTask -= nNext;
			}
		}
	} while (!hwndPv && g_nTs);

	return(nTask);
}

int _InstNextTask(int nCurTs, int nNext, PBOOL pfRedraw) {

	_ASSERT(nNext == 1 || nNext == -1);

	if (nCurTs < 0 || nCurTs >= g_nTs || !g_pTs[nCurTs].pszExePath)
		return(-1);

	if (pfRedraw)
		*pfRedraw = FALSE;

	BOOL fRedraw;
	int nTask = nCurTs;
	PWSTR pszInstance = g_pTs[nCurTs].pszExePath;
#pragma FIX_LATER(deleted item?...)
	do {
		nTask = _NextTask(nTask, nNext, &fRedraw);
		if (fRedraw && pfRedraw)
			*pfRedraw = TRUE;
	} while (g_nTs && nTask != g_nCurTs && lstrcmpi(pszInstance, g_pTs[nTask].pszExePath));

	return(nTask);
}


BOOL NextTask(HWND hwndTs, int nNext =1) {
	BOOL fRedraw;
	int nTask = _NextTask(g_nCurTs, nNext, &fRedraw);
	if (!g_nTs)
		return(FALSE);
	if (nTask == g_nCurTs && !fRedraw)
		return(TRUE);
	return(SelectTask(hwndTs, nTask, fRedraw));
}

BOOL InstNextTask(HWND hwndTs, int nNext =1) {

	BOOL fRedraw;
	int nTask = _InstNextTask(g_nCurTs, nNext, &fRedraw);
	if (!g_nTs)
		return(FALSE);
	if (nTask == g_nCurTs && !fRedraw)
		return(TRUE);
	return(SelectTask(hwndTs, nTask, fRedraw));
}

//-----------------------------------------------------------

int ListItemFromPoint(POINT pt) {

	if (!PtInRect(&g_rcList, pt))
		return(-1);

	int nx = (pt.x - g_rcList.left - XMARGIN_LIST),
		ny = (pt.y - g_rcList.top - YMARGIN_LIST);
	if (g_dwFlagsList & TSFL_SMALLICONS) {
		nx /= CX_SMICON + 2 * (XMARGIN_SMSEL + 1);
		ny /= CY_SMICON + 2 * (YMARGIN_SMSEL + 1);
	} else {
		nx /= CX_ICON + 2 * (XMARGIN_SEL + 1);
		ny /= CY_ICON + 2 * (YMARGIN_SEL + 1);
	}

	if (ny >= g_nIconsY)
		return(-2);

	RECT rcItem;

	if (g_dwFlagsList & TSFL_VERTICALLIST) {
		rcItem.left = g_rcList.left + XMARGIN_LIST + 1;
		rcItem.right = g_rcList.right - XMARGIN_LIST - 1;
		nx = 0;
	} else {
		if (nx >= g_nIconsX)
			return(-2);

		if (g_dwFlagsList & TSFL_SMALLICONS) {
			rcItem.left = g_rcList.left + XMARGIN_LIST + \
				(CX_SMICON + 2 * (XMARGIN_SMSEL + 1)) * nx;
			rcItem.right = rcItem.left + CX_SMICON + 2 * (XMARGIN_SMSEL + 1);
		} else {
			rcItem.left = g_rcList.left + XMARGIN_LIST + \
				(CX_ICON + 2 * (XMARGIN_SEL + 1)) * nx;
			rcItem.right = rcItem.left + CX_ICON + 2 * (XMARGIN_SEL + 1);
		}
	}

	if (g_dwFlagsList & TSFL_SMALLICONS) {
		rcItem.top = g_rcList.top + YMARGIN_LIST + (CY_SMICON + 2 * (YMARGIN_SMSEL + 1)) * ny;
		rcItem.bottom = rcItem.top + CY_SMICON + 2 * (YMARGIN_SMSEL + 1);
	} else {
		rcItem.top = g_rcList.top + YMARGIN_LIST + (CY_ICON + 2 * (YMARGIN_SEL + 1)) * ny;
		rcItem.bottom = rcItem.top + CY_ICON + 2 * (YMARGIN_SEL + 1);
	}
	
	return(PtInRect(&rcItem, pt) ? (nx + ny * g_nIconsX) : -2);
}

//-----------------------------------------------------------

HWND CreateInfoTip(HWND hwndTs) {

	HWND hwndTip = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TRANSPARENT, 
		TOOLTIPS_CLASS, L"", (!(g_dwFlagsPv & TSFPV_DESKTOP) && g_dwFlagsPv & TSFPV_OLDSTYLE) 
		? (WS_POPUP | TTS_NOPREFIX) : (WS_POPUP | TTS_NOPREFIX | TTS_BALLOON), 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwndTs, NULL, g_hinstExe, NULL);
	if (!hwndTip)
		return(NULL);
    //SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.hinst = NULL;
	ti.hwnd = hwndTs;
	ti.uFlags = TTF_TRACK | TTF_ABSOLUTE | TTF_TRANSPARENT;
	ti.lpszText = L"";
	ti.lParam = NULL;
	ti.lpReserved = 0;
	ti.uId = (UINT)-1;
	ti.rect = g_rcPvEx;

	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
	SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, g_rcTs.right - g_rcTs.left);
	SendMessage(hwndTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);

	POINT pt;	
	pt.x = g_rcPvEx.left;
	pt.y = g_rcPvEx.top;
	if (!(g_dwFlagsPv & TSFPV_DESKTOP)) {
		pt.x -= 3;
		pt.y -= 3;
	}
	ClientToScreen(hwndTs, &pt);
	SendMessage(hwndTip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));

	return(hwndTip);
}


BOOL UpdateInfoTip(int nTask) {
	
	_ASSERT(nTask >= 0 && nTask < g_nTs);

	if (!g_hwndInfo)
		return(FALSE);

	//if (!g_pTs[nTask].hwndOwner)
	//	return(FALSE);

	WCHAR szTip[1024] = L"N/A";

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, g_pTs[nTask].dwProcessId);

	if (hProcess) {

		FILETIME ftCreationTime, ftExitTime, ftKernelTime, ftUserTime;
		GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime);

		PROCESS_MEMORY_COUNTERS pmc;
		pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
		GetProcessMemoryInfo(hProcess, &pmc, sizeof(PROCESS_MEMORY_COUNTERS));

		CloseHandle(hProcess);

		LARGE_INTEGER li1, li2;
		li2.LowPart = ftCreationTime.dwLowDateTime;
		li2.HighPart = ftCreationTime.dwHighDateTime;

		FILETIME ftCurrent;
		GetSystemTimeAsFileTime(&ftCurrent);
		li1.LowPart = ftCurrent.dwLowDateTime;
		li1.HighPart = ftCurrent.dwHighDateTime;

		li1.QuadPart -= li2.QuadPart;

		const ULONGLONG ullPerSec = 10000000,
			ullPerMin = ullPerSec * 60, ullPerHour = ullPerMin * 60;

		UINT uSecDuration, uMinDuration, uHourDuration;
		uHourDuration = (UINT)(li1.QuadPart / ullPerHour);
		li1.QuadPart -= (ULONGLONG)uHourDuration * ullPerHour;
		uMinDuration = (UINT)(li1.QuadPart / ullPerMin);
		uSecDuration = (UINT)((li1.QuadPart - (ULONGLONG)uMinDuration * ullPerMin) / ullPerSec);

		li1.LowPart = ftKernelTime.dwLowDateTime;
		li1.HighPart = ftKernelTime.dwHighDateTime;
		li2.LowPart = ftUserTime.dwLowDateTime;
		li2.HighPart = ftUserTime.dwHighDateTime;

		li1.QuadPart += li2.QuadPart;

		UINT uSecCPU, uMinCPU, uHourCPU;
		uHourCPU = (UINT)(li1.QuadPart / ullPerHour);
		li1.QuadPart -= (ULONGLONG)uHourCPU * ullPerHour;
		uMinCPU = (UINT)(li1.QuadPart / ullPerMin);
		uSecCPU = (UINT)((li1.QuadPart - (ULONGLONG)uMinCPU * ullPerMin) / ullPerSec);

		WCHAR szMask[MAX_LANGLEN] = L"";
		LangLoadString(IDS_INFOTIPMASK, szMask, MAX_LANGLEN);

		wsprintf(szTip, szMask, g_pTs[nTask].pszExePath 
			? (g_pTs[nTask].pszExePath + g_pTs[nTask].nExeName) : L"N/A", g_pTs[nTask].dwProcessId, 
			uHourDuration, uMinDuration, uSecDuration, uHourCPU, uMinCPU, uSecCPU, 
			pmc.WorkingSetSize / 1024, pmc.PagefileUsage / 1024);
	} else {
		LangLoadString(IDS_NOTAVAILABLE, szTip, MAX_LANGLEN);
	}

	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = g_hwndTs;
	ti.uId = (UINT)-1;
	ti.hinst = NULL;
	ti.lpszText = szTip;

	SendMessage(g_hwndInfo, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
	//SendMessage(g_hwndInfo, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

    return(TRUE);
}


void ShowHideInfoTip(HWND hwndTs) {

	if (g_hwndInfo) {
		KillTimer(hwndTs, TIMER_INFOTIP);
		DestroyWindow(g_hwndInfo);
		g_hwndInfo = NULL;
	} else {
		g_hwndInfo = CreateInfoTip(hwndTs);
		if (g_hwndInfo) {
			UpdateInfoTip(g_nCurTs);

			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO);
			ti.hwnd = hwndTs;
			ti.uId = (UINT)-1;
			SendMessage(g_hwndInfo, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

			SetTimer(hwndTs, TIMER_INFOTIP, 1000, NULL);
		}
	}
}

//-----------------------------------------------------------

HWND CreateToolTip(HWND hwndTs) {

	HWND hwndTip = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TRANSPARENT, 
		TOOLTIPS_CLASS, L"", WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwndTs, NULL, g_hinstExe, NULL);
	if (!hwndTip)
		return(NULL);
    //SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.hinst = NULL;
	ti.hwnd = hwndTs;
	ti.uFlags = TTF_SUBCLASS;
	ti.lpszText = LPSTR_TEXTCALLBACK;
	ti.lParam = NULL;
	ti.lpReserved = 0;

	int nWidth, nHeight;
	if (g_dwFlagsList & TSFL_SMALLICONS) {
		nHeight = CY_SMICON + 2 * YMARGIN_SMSEL;
	} else nHeight = CY_ICON + 2 * YMARGIN_SEL;

	if (g_dwFlagsList & TSFL_VERTICALLIST) {
		_ASSERT(g_nIconsX == 1);
		nWidth = g_rcList.right - g_rcList.left - 2 * XMARGIN_LIST - 2;
	} else {
		if (g_dwFlagsList & TSFL_SMALLICONS) {
			nWidth = CX_SMICON + 2 * XMARGIN_SMSEL;
		} else nWidth = CX_ICON + 2 * XMARGIN_SEL;
	}
	ti.rect.left = g_rcList.left + XMARGIN_LIST + 1;
	ti.rect.right = ti.rect.left + nWidth;
	ti.rect.top = g_rcList.top + YMARGIN_LIST + 1;
	ti.rect.bottom = ti.rect.top + nHeight;
	ti.uId = 0;

	for (int y = 0; y < g_nIconsY; y++) {
		for (int x = 0; x < g_nIconsX; x++) {
			SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
			ti.uId++;
			ti.rect.left = ti.rect.right + 2;
			ti.rect.right = ti.rect.left + nWidth;
		}
		ti.rect.left = g_rcList.left + XMARGIN_LIST + 1;
		ti.rect.right = ti.rect.left + nWidth;
		ti.rect.top = ti.rect.bottom + 2;
		ti.rect.bottom = ti.rect.top + nHeight;
	}

	SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, g_rcTs.right - g_rcTs.left);

	return(hwndTip);
}

//-----------------------------------------------------------

void _StartFadeOut(HWND hwndTs) {

	KillTimer(hwndTs, TIMER_PREVIEW);
	KillTimer(hwndTs, TIMER_FADEIN);
	DestroyAltHook();

	if (g_hwndInfo) {
		DestroyWindow(g_hwndInfo);
		g_hwndInfo = NULL;
	}

	g_dwFadeOut &= ~TSFBL_STARTED;
	if (g_dwFadeOut & TSFBL_ENABLED) {
		DWORD dwAlpha = (g_dwFadeIn & TSFBL_ENABLED) ? HIBYTE(HIWORD(g_dwFadeIn)) : 255, 
			dwStep = (250 * FADE_DELAY) / LOWORD(g_dwFadeOut);
		g_dwFadeOut &= ~TSBL_LAYEREDMASK;
		if (dwAlpha > dwStep) {
			g_dwFadeOut |= TSFBL_STARTED;
			dwAlpha -= dwStep;
			g_dwFadeOut |= dwAlpha << 24;

			DWORD dwStyleEx = GetWindowLongPtr(hwndTs, GWL_EXSTYLE);
			if (!(dwStyleEx & WS_EX_LAYERED))
                SetWindowLongPtr(hwndTs, GWL_EXSTYLE, dwStyleEx | WS_EX_LAYERED);
			SetLayeredWindowAttributes(hwndTs, 0, (BYTE)dwAlpha, LWA_ALPHA);
			SetTimer(hwndTs, TIMER_FADEOUT, FADE_DELAY, NULL);			
		}
	}
}

void _SwitchToCurTask() {
	if (g_nCurTs >= 0 && g_nCurTs < g_nTs) {
		g_hwndFrgnd = NULL;		
		HWND hwndPv = NULL;
		CheckTask(g_nCurTs, &hwndPv);
		if (hwndPv) {
			if (g_pTs[g_nCurTs].dwFlags & TI_TRAYWINDOW) {
				SendMessage(g_hwndMain, WM_UNFLIPFROMTRAY, 
					(WPARAM)g_pTs[g_nCurTs].hwndOwner, (LPARAM)SC_RESTORE);
			} else {
				if (WaitForSingleObject(g_hmtxPv, INFINITE) == WAIT_OBJECT_0) {
					if (g_hbitmapPv) {
						DeleteObject(g_hbitmapPv);
						g_hbitmapPv = NULL;
					}
					ReleaseMutex(g_hmtxPv);
				}
				MySwitchToThisWindow(hwndPv);
			}
		}
	}
}

void SwitchToCurTask(HWND hwndTs) {

	_StartFadeOut(hwndTs);
	_SwitchToCurTask();
	if (!(g_dwFadeOut & TSFBL_STARTED))
		DestroyWindow(hwndTs);
}

void SwitchToCurTask2(HWND hwndTs) {

	_StartFadeOut(hwndTs);

	if (g_nCurTs >= 0 && g_nCurTs < g_nTs) {
		g_hwndFrgnd = NULL;
		HWND hwndPv = NULL;
		CheckTask(g_nCurTs, &hwndPv);
		if (hwndPv) {
			if (g_pTs[g_nCurTs].dwFlags & TI_TRAYWINDOW) {
				SendMessage(g_hwndMain, WM_UNFLIPFROMTRAY, 
					(WPARAM)g_pTs[g_nCurTs].hwndOwner, (LPARAM)SC_RESTORE);
			}
			SetForegroundWindow(hwndPv);
		}
	}
	if (!(g_dwFadeOut & TSFBL_STARTED))
		DestroyWindow(hwndTs);
}


void StartFadeOut(HWND hwndTs) {
	_StartFadeOut(hwndTs);
	if (!(g_dwFadeOut & TSFBL_STARTED))
		DestroyWindow(hwndTs);
}


//-----------------------------------------------------------

LRESULT CALLBACK TaskSwitchWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	static HWND s_hwndTip = NULL;
	static LPARAM s_lParamMove = (LPARAM)-1;

	static int s_nStickyCnt = 0;

	static BOOL s_fHand = FALSE;
	static HCURSOR s_hcurArrow = LoadCursor(NULL, IDC_ARROW);
	static HCURSOR s_hcurHand = LoadCursor(NULL, IDC_HAND);

	switch (uMsg) {

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			if (g_hbitmapTs) {
				HDC hdcMem = CreateCompatibleDC(NULL);
				if (hdcMem) {
					HBITMAP hbitmapOld = (HBITMAP)SelectObject(hdcMem, g_hbitmapTs);
					BitBlt(hdc, 0, 0, g_rcTs.right - g_rcTs.left, 
						g_rcTs.bottom - g_rcTs.top, hdcMem, 0, 0, SRCCOPY);
					SelectObject(hdcMem, hbitmapOld);
					DeleteDC(hdcMem);

					OnPaint(hdc);
				}
			}
			EndPaint(hwnd, &ps);
			break;
					   }

		case WM_ERASEBKGND:
			return(TRUE);
			//break;

		case WM_MOUSEMOVE: 
			if (s_lParamMove != lParam) {
				if (g_dwFlagsList & TSFL_MOUSEOVER || g_dwFlagsList & TSFL_HOTTRACK) {

					s_lParamMove = lParam;

					POINT pt;
					pt.x = LOWORD(lParam);
					pt.y = HIWORD(lParam);
					int nSelTs = ListItemFromPoint(pt);
					const int nTmpTs = g_nFirstTs + nSelTs;

					if (g_dwFlagsList & TSFL_HOTTRACK) {
						s_fHand = (BOOL)((nSelTs >= 0 && nTmpTs < g_nTs) || 
							((g_dwFlagsPv & TSFPV_DESKTOP || !(g_dwFlagsPv & TSFPV_OLDSTYLE)) && 
							PtInRect(&g_rcPvEx, pt)));
					}

					if (g_dwFlagsList & TSFL_MOUSEOVER && !(g_dwFadeOut & TSFBL_STARTED)) {
						if (nSelTs >= 0 && nTmpTs < g_nTs && nTmpTs != g_nCurTs)
							if (!SelectTask(hwnd, nTmpTs))
								DestroyWindow(hwnd);
					}
				}
			}
			break;

		case WM_SETCURSOR:
			SetCursor(s_fHand ? s_hcurHand : s_hcurArrow);
			return(TRUE);			

		case WM_NOTIFY: {
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			if (s_hwndTip && pnmhdr->hwndFrom == s_hwndTip) {
				switch (pnmhdr->code) {

					case TTN_GETDISPINFO: {
						LPNMTTDISPINFO pnmttdi = (LPNMTTDISPINFO)pnmhdr;
						pnmttdi->szText[0] = L'\0';
						int nCurTs = g_nFirstTs + (int)pnmhdr->idFrom;
						if (nCurTs >= 0 && nCurTs < g_nTs)
							pnmttdi->lpszText = g_pTs[nCurTs].szCaption;
						break;
										  }
				}
			}
			break;
						}

		case WM_TASKSWITCH:
			if (g_dwFadeOut & TSFBL_STARTED)
				break;
			if (g_dwFlagsList & TSFL_MENUACTIVE && wParam != IDH_SWITCH)
				break;
			SetForegroundWindow(hwnd);
			switch (wParam) {
				case IDH_NEXTTASK:
				case IDH_PREVTASK:
				case IDH_WINNEXTTASK:
				case IDH_WINPREVTASK:
					s_nStickyCnt++;
					if (!NextTask(hwnd, (wParam == IDH_NEXTTASK || wParam == IDH_WINNEXTTASK) ? 1 : -1))
						DestroyWindow(hwnd);
					break;
				case IDH_INSTNEXT:
				case IDH_INSTPREV:
					s_nStickyCnt++;
					if (!InstNextTask(hwnd, (wParam == IDH_INSTNEXT) ? 1 : -1))
						DestroyWindow(hwnd);
					break;
				case IDH_STNEXTTASK:
				case IDH_STINEXTTASK:
				case IDH_SWITCH:
					if (!(g_dwFlagsList & TSFL_TEMPSTICKY))
						SwitchToCurTask(hwnd);
					break;
				case IDH_ESCAPE:
					DestroyWindow(hwnd);
					break;
			}
			break;

		case WM_PVREADY:
			if (WaitForSingleObject(g_hmtxPv, 0) == WAIT_OBJECT_0) {
				if (g_pTs) {
					int nTask = 0;
					while (nTask < g_nTs && g_pTs[nTask].hwndOwner != g_pvData.hwndOwner)
						nTask++;
					if (nTask < g_nTs) {
						if (g_pTs[nTask].hbitmapPv)
							DeleteObject(g_pTs[nTask].hbitmapPv);						
						if (nTask == g_nCurTs) {
							HDC hdc = GetDC(hwnd);
							if (hdc) {
								DrawCachedPreview(hdc, g_hbitmapPv);
								ReleaseDC(hwnd, hdc);
							}
						}
						if (g_dwFlagsPv & TSFPV_NOCACHE) {
							DeleteObject(g_hbitmapPv);
							g_pTs[nTask].hbitmapPv = NULL;
							if (nTask == g_nCurTs && g_dwFlagsPv & TSFPV_LIVEUPDATE)
								SetTimer(hwnd, TIMER_PREVIEW, LOWORD(g_dwFlagsPv), NULL);
						} else {
							g_pTs[nTask].hbitmapPv = g_hbitmapPv;
						}
					} else
						DeleteObject(g_hbitmapPv);					
				} else
					DeleteObject(g_hbitmapPv);
				g_hbitmapPv = NULL;
				ReleaseMutex(g_hmtxPv);
			}
			break;

		case WM_TIMER:
			if (!g_pTs) break;
			if (wParam == TIMER_PREVIEW) {
				if (g_dwFadeOut & TSFBL_STARTED) {
					KillTimer(hwnd, TIMER_PREVIEW);
					break;
				}
				if (!g_pTs[g_nCurTs].hbitmapPv) {
					if (StartDrawTaskPreview(g_nCurTs))
						KillTimer(hwnd, TIMER_PREVIEW);
				}
			} else if (wParam == TIMER_FADEIN) {
				_ASSERT(g_dwFadeIn & TSFBL_ENABLED && LOWORD(g_dwFadeIn));
				DWORD dwAlpha = HIBYTE(HIWORD(g_dwFadeIn));
				dwAlpha = dwAlpha + (200 * FADE_DELAY) / LOWORD(g_dwFadeIn);
				if ((!(g_dwFlags & TSF_LAYERED) && dwAlpha >= 255) || 
					((g_dwFlags & TSF_LAYERED) && dwAlpha >= (g_dwFlags & TSF_LAYEREDMASK))) {
					dwAlpha = (g_dwFlags & TSF_LAYERED) ? (g_dwFlags & TSF_LAYEREDMASK) : 255;
					KillTimer(hwnd, TIMER_FADEIN);
				}
				g_dwFadeIn &= ~TSBL_LAYEREDMASK;
				g_dwFadeIn |= dwAlpha << 24;
				if (dwAlpha == 255) {
					SetWindowLongPtr(hwnd, GWL_EXSTYLE, 
						GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
				} else {
					SetLayeredWindowAttributes(hwnd, 0, (BYTE)dwAlpha, LWA_ALPHA);
				}
			} else if (wParam == TIMER_FADEOUT) {
				DWORD dwAlpha = HIBYTE(HIWORD(g_dwFadeOut)), 
					dwStep = (250 * FADE_DELAY) / LOWORD(g_dwFadeOut);
				g_dwFadeOut &= ~TSBL_LAYEREDMASK;
				if (dwAlpha > dwStep) {
					dwAlpha -= dwStep;
					g_dwFadeOut |= dwAlpha << 24;
					SetLayeredWindowAttributes(hwnd, 0, (BYTE)dwAlpha, LWA_ALPHA);
				} else {					
					KillTimer(hwnd, TIMER_FADEOUT);
					ShowWindow(hwnd, SW_HIDE);
					DestroyWindow(hwnd);
				}
			} else if (wParam == TIMER_TSXPSHOW) {
				KillTimer(hwnd, TIMER_TSXPSHOW);
				ShowWindow(hwnd, SW_SHOW);
			} else if (wParam == TIMER_INFOTIP) {
				UpdateInfoTip(g_nCurTs);
			}
			break;

		case WM_MOUSEWHEEL: {
			if (g_dwFadeOut & TSFBL_STARTED)
				break;
			int nNext = ((int)wParam < 0) ? 1 : -1;
			if (g_dwFlagsList & TSFL_INVERSEWHEEL)
				nNext = -nNext;
			if (!NextTask(hwnd, nNext))
				DestroyWindow(hwnd);
			break;
							}

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN: {
			if (g_dwFadeOut & TSFBL_STARTED)
				break;
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			int nSelTs = ListItemFromPoint(pt);

			if (nSelTs >= 0 && g_nFirstTs + nSelTs < g_nTs) {
				if (wParam & MK_CONTROL) {
					if (!MultiSelectTask(hwnd, g_nFirstTs + nSelTs, g_nFirstTs + nSelTs,
						!(g_pTs[g_nFirstTs + nSelTs].dwFlags & TI_SELECTED)))
						DestroyWindow(hwnd);
				} else if (wParam & MK_SHIFT) {
					if (!MultiSelectTasks(hwnd, g_nCurTs, g_nFirstTs + nSelTs, 
						!(g_pTs[g_nFirstTs + nSelTs].dwFlags & TI_SELECTED)))
						DestroyWindow(hwnd);
				} else if (g_nCurTs != g_nFirstTs + nSelTs) {
					if (!SelectTask(hwnd, g_nFirstTs + nSelTs))
						DestroyWindow(hwnd);
				}

			}/* else if (uMsg == WM_RBUTTONDOWN && PtInRect(&g_rcPvEx, pt)) {
				ShowHideInfoTip(hwnd);
			}*/
			break;
							 }
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN: {
			if (g_dwFadeOut & TSFBL_STARTED || g_dwFlagsList & TSFL_MENUACTIVE)
				break;
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			int nSelTs = ListItemFromPoint(pt);

			if (nSelTs >= 0 && g_nFirstTs + nSelTs < g_nTs) {
				if (!MultiSelectTask(hwnd, g_nFirstTs + nSelTs, g_nFirstTs + nSelTs, 
					!(g_pTs[g_nFirstTs + nSelTs].dwFlags & TI_SELECTED)))
					DestroyWindow(hwnd);
			} else if (PtInRect(&g_rcPvEx, pt)) {
				ShowHideInfoTip(hwnd);
			} else {
				if (!MultiSelectTask(hwnd, g_nCurTs, g_nCurTs, 
					!(g_pTs[g_nCurTs].dwFlags & TI_SELECTED)))
					DestroyWindow(hwnd);
			}
			break;
							 }

		case WM_LBUTTONUP: {
			if (g_dwFadeOut & TSFBL_STARTED || wParam & MK_CONTROL || 
				wParam & MK_SHIFT || !(g_dwFlagsList & TSFL_MOUSEOVER)) break;
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			int nSelTs = ListItemFromPoint(pt);

			if ((nSelTs >= 0 && (nSelTs + g_nFirstTs) == g_nCurTs) || PtInRect(&g_rcPvEx, pt))
				SwitchToCurTask(hwnd);
			break;
						   }

		case WM_RBUTTONUP: {
			if (g_dwFadeOut & TSFBL_STARTED)
				break;
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			s_fHand = FALSE;
			if (!(wParam & MK_CONTROL || wParam & MK_SHIFT)) {
				int nSelTs = ListItemFromPoint(pt);
				if ((nSelTs >= 0 && g_nCurTs == nSelTs + g_nFirstTs) || PtInRect(&g_rcPvEx, pt)) {
					ClientToScreen(hwnd, &pt);
					g_dwFlagsList |= TSFL_MENUACTIVE;
					ShowTaskMenu(hwnd, pt);
					g_dwFlagsList &= ~TSFL_MENUACTIVE;
					break;
				}
			}
			ClientToScreen(hwnd, &pt);
			g_dwFlagsList |= TSFL_MENUACTIVE;
			ShowCommonMenu(hwnd, pt);
			g_dwFlagsList &= ~TSFL_MENUACTIVE;
			break;
						   }

		case WM_LBUTTONDBLCLK:
			if (g_dwFadeOut & TSFBL_STARTED)
				break;			
			if (!(g_dwFlagsList & TSFL_MOUSEOVER)) {
				POINT pt;
				pt.x = LOWORD(lParam);
				pt.y = HIWORD(lParam);
				int nSelTs = ListItemFromPoint(pt);
				if ((nSelTs >= 0 && (nSelTs + g_nFirstTs) == g_nCurTs) || PtInRect(&g_rcPvEx, pt)) {
					SwitchToCurTask(hwnd);
					break;
				}
			}
			if (!(wParam & MK_CONTROL) && !(wParam & MK_SHIFT)) {
				MultiSelectAll(hwnd, g_nSelTs != g_nTs);
			}
			break;

		case WM_CHAR: // tag kl3
			if (g_dwFlagsList & TSFL_ALTMODE && wParam != VK_BACK) // klvov: process WM_CHAR messages only in ALT mode
			{
				if (GetAsyncKeyState(VK_CONTROL) & 0x8000) break;
				WCHAR wc[2]; wc[0] = (WCHAR) wParam; wc[1] = 0;
				if (lstrlen (g_szTypeBuffer) < (MAX_PATH - 10) ) {
					lstrcat(g_szTypeBuffer, (LPCWSTR) &wc);
				}
				ProcessTypeBuffer(hwnd);
			}
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (g_dwFadeOut & TSFBL_STARTED)
				break;
			//klvov: alternative keyboard handling proc
				if (g_dwFlagsList & TSFL_ALTMODE) // tag kl1
				{
					int lenb = lstrlen (g_szTypeBuffer);
					WCHAR szTemp[MAX_PATH] = L"";
					switch (wParam) {
						case VK_F1: //klvov: testing if this works
							//SortTaskListByModuleName(hwnd);
							lstrcpy(g_szTypeBuffer,L"");
							ProcessTypeBuffer(hwnd);
							break;
						case VK_BACK: //delete 1 character from g_szTypeBuffer
							if ( lenb > 0){ //klvov: there is something weird with lstrcpyn functions, can't understand how they work
								g_szTypeBuffer[lenb-1] = 0;
								//ZeroMemory(szTemp, sizeof (szTemp) );
								//lstrcpyn(szTemp,g_szTypeBuffer, lenb);
								//ZeroMemory(g_szTypeBuffer, sizeof (g_szTypeBuffer) );
								//lstrcpyn(g_szTypeBuffer,szTemp, lenb);
								//OutputDebugString(L"szTemp:"); OutputDebugString(szTemp); OutputDebugString(L"\n");
								//g_szTypeBuffer[lenb-1] = 0;
								ProcessTypeBuffer(hwnd);
							}
							break;
						case VK_RETURN:
							SwitchToCurTask(hwnd);
							break;
						//TODO: maybe, remove scrolling? 
						case VK_UP:
							if (g_nTs > g_nIconsX) {
								int nSelTs;
								if (g_nCurTs < g_nIconsX) {
									nSelTs = (g_nTs / g_nIconsX) * g_nIconsX + (g_nCurTs % g_nIconsX);
									if (nSelTs >= g_nTs) {
										nSelTs -= g_nIconsX;
										if (g_nTs - nSelTs <= g_nIconsX && 
											g_nTs - g_nFirstTs > g_nIconsX * g_nIconsY)
											g_nFirstTs = g_nTs;
									}
								} else
									nSelTs = g_nCurTs - g_nIconsX;
									if (nSelTs != g_nCurTs)
										if (!SelectTask(hwnd, nSelTs))
											DestroyWindow(hwnd);
							}
							lstrcpy(g_szTypeBuffer, L"");
							break;
						case VK_DOWN:
							if (g_nTs > g_nIconsX) {
								int nSelTs;
								if (g_nTs - g_nCurTs <= g_nIconsX)
									nSelTs = g_nCurTs % g_nIconsX;
								else {
									nSelTs = g_nCurTs + g_nIconsX;
									if (g_nTs - nSelTs <= g_nIconsX && 
										g_nTs - g_nFirstTs > g_nIconsX * g_nIconsY)
										g_nFirstTs = g_nTs;
								}
									if (nSelTs != g_nCurTs)
										if (!SelectTask(hwnd, nSelTs))
											DestroyWindow(hwnd);
								
							}
							lstrcpy(g_szTypeBuffer, L"");
							break;
						case VK_END:
							if (g_nCurTs != g_nTs - 1)
								if (!SelectTask(hwnd, g_nTs - 1))
									DestroyWindow(hwnd);
							lstrcpy(g_szTypeBuffer, L"");
							break;
						case VK_HOME:
							if (g_nCurTs != 0)
								if (!SelectTask(hwnd, 0))
									DestroyWindow(hwnd);
							lstrcpy(g_szTypeBuffer, L"");
							break;
						case VK_INSERT:
							/*
							int nCurTs = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? 
								_InstNextTask(g_nCurTs, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1, NULL) 
								: _NextTask(g_nCurTs, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1, NULL);;
							*/
							//first param - position after select
							//second - which task to select
							if (!MultiSelectTask(hwnd, g_nCurTs+1, g_nCurTs,  
								(g_pTs[g_nCurTs].dwFlags & TI_SELECTED)?FALSE:TRUE   ) )
								DestroyWindow(hwnd);
							break;
						case VK_DELETE: // обработчик по Win+K
							RemoveSelected(hwnd);
							break;
						case VK_F5:
							UpdateTask(hwnd, g_nCurTs);
							break;
						case VK_F2:
							MinimizeSelected(hwnd);
							break;
						case VK_F6:
							MaxRestoreSelected(hwnd, SC_RESTORE);
							break;
						case VK_F7:
							MaxRestoreSelected(hwnd, SC_MAXIMIZE);
							break;
						case VK_F4:
							CloseSelected(hwnd);
							break;
						case 0x41: // Ctrl-A - select all, Ctrl-Shift-A - select instances
							if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
								if (GetAsyncKeyState(VK_SHIFT) & 0x8000) 
									MultiSelectInstances(hwnd, TRUE);
								else MultiSelectAll(hwnd, TRUE);
							} 
							break;
						case 0x44: // Ctrl-D - deselect all, Ctrl-Shift-D - deselect instances
							if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
								if (GetAsyncKeyState(VK_SHIFT) & 0x8000) 
									MultiSelectInstances(hwnd, FALSE);
								else MultiSelectAll(hwnd, FALSE);
							} 
							break;
						case 0x43: // C - cascade
							if (GetAsyncKeyState(VK_CONTROL) & 0x8000) ReorderSelected(hwnd, RO_CASCADE);
							break;

						case 0x48: // H - tile horizontally
							if (GetAsyncKeyState(VK_CONTROL) & 0x8000) ReorderSelected(hwnd, RO_TILEHORIZONTALLY);
							break;

						case 0x56: // V - tile vertically
					if (GetAsyncKeyState(VK_CONTROL) & 0x8000) ReorderSelected(hwnd, RO_TILEVERTICALLY);
					break;
						default:
							break;
					}
				} // end of alternative keyboard handling procedure
				else // original keyboard handling proc, flag  TSFL_ALTMODE not set
				{
				switch (wParam){ 
				case VK_UP:
					if (g_nTs > g_nIconsX) {
						int nSelTs;
						if (g_nCurTs < g_nIconsX) {
							nSelTs = (g_nTs / g_nIconsX) * g_nIconsX + (g_nCurTs % g_nIconsX);
							if (nSelTs >= g_nTs) {
								nSelTs -= g_nIconsX;
								if (g_nTs - nSelTs <= g_nIconsX && 
									g_nTs - g_nFirstTs > g_nIconsX * g_nIconsY)
									g_nFirstTs = g_nTs;
							}
						} else
							nSelTs = g_nCurTs - g_nIconsX;
						if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
							if (!MultiSelectTask(hwnd, nSelTs, g_nCurTs,
								!(g_pTs[g_nCurTs].dwFlags & TI_SELECTED)))
								DestroyWindow(hwnd);
						} else {
							if (nSelTs != g_nCurTs)
								if (!SelectTask(hwnd, nSelTs))
									DestroyWindow(hwnd);
						}
					}
					break;
				case VK_DOWN:
					if (g_nTs > g_nIconsX) {
						int nSelTs;
						if (g_nTs - g_nCurTs <= g_nIconsX)
							nSelTs = g_nCurTs % g_nIconsX;
						else {
							nSelTs = g_nCurTs + g_nIconsX;
							if (g_nTs - nSelTs <= g_nIconsX && 
								g_nTs - g_nFirstTs > g_nIconsX * g_nIconsY)
								g_nFirstTs = g_nTs;
						}
						if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
							if (!MultiSelectTask(hwnd, nSelTs, g_nCurTs,
								!(g_pTs[g_nCurTs].dwFlags & TI_SELECTED)))
								DestroyWindow(hwnd);
						} else {
							if (nSelTs != g_nCurTs)
								if (!SelectTask(hwnd, nSelTs))
									DestroyWindow(hwnd);
						}
					}
					break;
				case VK_LEFT: {
					int nSelTs = (g_nCurTs == 0) ? g_nTs - 1 : (g_nCurTs - 1);
					if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
						if (!MultiSelectTask(hwnd, nSelTs, g_nCurTs,
							!(g_pTs[g_nCurTs].dwFlags & TI_SELECTED)))
							DestroyWindow(hwnd);
					} else {
						if (nSelTs != g_nCurTs)
							if (!SelectTask(hwnd, nSelTs))
								DestroyWindow(hwnd);
					}
					break;
							  }
				case VK_RIGHT: {
					int nSelTs = (g_nCurTs == g_nTs - 1) ? 0 : (g_nCurTs + 1);
					if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
						if (!MultiSelectTask(hwnd, nSelTs, g_nCurTs,
							!(g_pTs[g_nCurTs].dwFlags & TI_SELECTED)))
							DestroyWindow(hwnd);
					} else {
						if (nSelTs != g_nCurTs)
							if (!SelectTask(hwnd, nSelTs))
								DestroyWindow(hwnd);
					}
					break;
							   }

				case VK_END:
					if (g_nCurTs != g_nTs - 1)
						if (!SelectTask(hwnd, g_nTs - 1))
							DestroyWindow(hwnd);
					break;
				case VK_HOME:
					if (g_nCurTs != 0)
						if (!SelectTask(hwnd, 0))
							DestroyWindow(hwnd);
					break;

				case VK_NEXT: { // Page Down
					int nSelTs = (g_nCurTs + g_nIconsX * g_nIconsY < g_nTs - 1) 
						? (g_nCurTs + g_nIconsX * g_nIconsY) : (g_nTs - 1);
					if (nSelTs != g_nCurTs)
						if (!SelectTask(hwnd, nSelTs))
							DestroyWindow(hwnd);
					break;
							  }
				case VK_PRIOR: { // Page Up
					int nSelTs = (g_nCurTs > g_nIconsX * g_nIconsY) 
						? (g_nCurTs - g_nIconsX * g_nIconsY) : 0;
					if (nSelTs != g_nCurTs)
						if (!SelectTask(hwnd, nSelTs))
							DestroyWindow(hwnd);
					break;
							   }

				case VK_SPACE:
				case VK_RETURN:
					SwitchToCurTask(hwnd);
					break;

				case 0xC0: // ` - previous task
					if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
						if (!InstNextTask(hwnd, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 1 : -1))
							DestroyWindow(hwnd);
					} else {
						if (!NextTask(hwnd, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 1 : -1))
							DestroyWindow(hwnd);
					}
					break;

				case VK_TAB:
					if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
						if (!InstNextTask(hwnd, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1))
							DestroyWindow(hwnd);
					} else {
						if (!NextTask(hwnd, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1))
							DestroyWindow(hwnd);
					}
					break;

				case 0x53: // S - select
				case VK_INSERT: {				
					int nCurTs = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? 
						_InstNextTask(g_nCurTs, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1, NULL) 
						: _NextTask(g_nCurTs, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1, NULL);;
					if (!MultiSelectTask(hwnd, nCurTs, g_nCurTs, TRUE))
						DestroyWindow(hwnd);
					break;
								}
				case 0x44: // D - deselect
				case VK_BACK: {
					int nCurTs = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? 
						_InstNextTask(g_nCurTs, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 1 : -1, NULL) 
						: _NextTask(g_nCurTs, (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 1 : -1, NULL);;
					if (!MultiSelectTask(hwnd, nCurTs, g_nCurTs, FALSE))
						DestroyWindow(hwnd);
					break;
							  }

				case 0x41: // A - select all
					if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
						MultiSelectInstances(hwnd, (BOOL)(!(GetAsyncKeyState(VK_SHIFT) & 0x8000)));
					} else {
						MultiSelectAll(hwnd, (BOOL)(!(GetAsyncKeyState(VK_SHIFT) & 0x8000)));
					}
					break;

				case VK_DELETE: // обработчик по Win+F12
					RemoveSelected(hwnd);
					break;

				case 0x49: // I
				case VK_F1:
					ShowHideInfoTip(hwnd);
					break;

				case 0x52: // R
				case VK_F5:
					UpdateTask(hwnd, g_nCurTs);
					break;

				case 0x4D: // M - minimize
				case VK_F2:
					MinimizeSelected(hwnd);
					break;

				case 0x4E: // N - minimize to tray
				case VK_F3:
					FlipToTraySelected(hwnd);
					break;

				case VK_OEM_COMMA: // < - restore
				case VK_F6:
					MaxRestoreSelected(hwnd, SC_RESTORE);
					break;

				case VK_OEM_PERIOD: // > - maximize
				case VK_F7:
					MaxRestoreSelected(hwnd, SC_MAXIMIZE);
					break;

				case 0x51: // Q - MacOS
					if (!(g_dwFlagsList & TSFL_ENABLEQ))
						break;
				case 0x58: // X - close
				case VK_F4:
					CloseSelected(hwnd);
					break;

				case 0x54: // T
				case VK_F8:
					TerminateSelected(hwnd);
					break;

				case 0x45: // E - open exe path
					ExploreExePath(g_pTs[g_nCurTs].pszExePath);
					break;

				case 0x43: // C - cascade
					ReorderSelected(hwnd, RO_CASCADE);
					break;

				case 0x48: // H - tile horizontally
					ReorderSelected(hwnd, RO_TILEHORIZONTALLY);
					break;

				case 0x56: // V - tile vertically
					ReorderSelected(hwnd, RO_TILEVERTICALLY);
					break;

				case VK_OEM_PLUS:
				case VK_ADD: // + - sort by application
				case VK_F9:
					SortTaskList(hwnd, TRUE);
					break;

				case VK_OEM_MINUS:
				case VK_SUBTRACT: // - - sort by title
				case VK_F10:
					SortTaskList(hwnd, FALSE);
					break;

				case 0x50: // P - Preferences
					ConfigTaskSwitchXP();
					break;

				case 0x5A: // Z - enable sticky mode
					g_dwFlagsList |= TSFL_TEMPSTICKY;
					break;

				case VK_APPS: {
					POINT pt;
					if (g_dwFlagsList & TSFL_SMALLICONS) {
						pt.x = g_rcList.left + XMARGIN_LIST + 1 + XMARGIN_SMSEL + CX_SMICON / 2 + \
							(CX_SMICON + 2 * XMARGIN_SMSEL) * ((g_nCurTs - g_nFirstTs) % g_nIconsX);
						pt.y = g_rcList.top + YMARGIN_LIST + 1 + YMARGIN_SMSEL + CY_SMICON / 2 + \
							(CY_SMICON + 2 * YMARGIN_SMSEL) * ((g_nCurTs - g_nFirstTs) / g_nIconsX);
					} else {
						pt.x = g_rcList.left + XMARGIN_LIST + 1 + XMARGIN_SEL + CX_ICON / 2 + \
							(CX_ICON + 2 * XMARGIN_SEL) * ((g_nCurTs - g_nFirstTs) % g_nIconsX);
						pt.y = g_rcList.top + YMARGIN_LIST + 1 + YMARGIN_SEL + CY_ICON / 2 + \
							(CY_ICON + 2 * YMARGIN_SEL) * ((g_nCurTs - g_nFirstTs) / g_nIconsX);
					}
					ClientToScreen(hwnd, &pt);
					g_dwFlagsList |= TSFL_MENUACTIVE;
					ShowTaskMenu(hwnd, pt);
					g_dwFlagsList &= ~TSFL_MENUACTIVE;
					break;
							  }

				case VK_ESCAPE:
					DestroyWindow(hwnd);
					break;

				default:
					if (wParam >= 0x30 && wParam <= 0x39) {
						int nSel = (int)wParam - 0x30 - 1;
						if (nSel < 0) nSel = 9;
						if (nSel >= g_nTs) nSel = g_nTs - 1;
						if (nSel != g_nCurTs) {
							if (!SelectTask(hwnd, nSel, FALSE))
								DestroyWindow(hwnd);
						}
					} else if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9) {
						int nSel = (int)wParam - VK_NUMPAD0 - 1;
						if (nSel < 0) nSel = 9;
						if (nSel >= g_nTs) nSel = g_nTs - 1;
						if (nSel != g_nCurTs) {
							if (!SelectTask(hwnd, nSel, FALSE))
								DestroyWindow(hwnd);
						}
					}
					break;
				}
				}// end of original keyboard handling proc
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (wParam == VK_MENU) {
				if ((!(g_dwFlags & TSF_STICKYALTTAB) || s_nStickyCnt > 0) && 
					!(g_dwFlagsList & TSFL_TEMPSTICKY)) SwitchToCurTask(hwnd);
			}
			break;

		case WM_KILLFOCUS:
#pragma FIX_LATER(minimize all)
			g_hwndFrgnd = NULL;
			if (!(g_dwFadeOut & TSFBL_STARTED))
				DestroyWindow(hwnd);
			break;

		case WM_THEMECHANGED:
			DestroyWindow(hwnd);
			break;

		case WM_CREATE: {

			g_hwndTs = hwnd;
			g_dwFadeOut &= ~TSFBL_STARTED;
			g_dwFlagsList &= ~(
				TSFL_SORTEDTITLES 
				| TSFL_SORTEDEXEPATHS 
				| TSFL_MENUACTIVE 
				| TSFL_TEMPSTICKY
				);
			
			s_nStickyCnt = 0;

			UINT uParam = (UINT)(UINT_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams;

			if ((uParam == IDH_NEXTTASK || uParam == IDH_PREVTASK || 
				uParam == IDH_INSTNEXT || uParam == IDH_INSTPREV) && 
				!(GetAsyncKeyState(VK_MENU) & 0x8000)) {
				SetForegroundWindow(hwnd);
				_SwitchToCurTask();
                return(-1);
			}

			// calc position
			POINT pt, ptCur;
			GetCursorPos(&ptCur);

			if (g_dwFlags & TSF_ACTIVEMONITOR && GetSystemMetrics(SM_CMONITORS) > 1 && g_hwndFrgnd) {

				RECT rcFrgnd;
				HMONITOR hMonitor;

				GetWindowRect(g_hwndFrgnd, &rcFrgnd);

				if (PtInRect(&rcFrgnd, ptCur)) {
                    hMonitor = MonitorFromPoint(ptCur, MONITOR_DEFAULTTONEAREST);
				} else hMonitor = MonitorFromWindow(g_hwndFrgnd, MONITOR_DEFAULTTONEAREST);

				MONITORINFO mi;
				mi.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(hMonitor, &mi);

				if (g_dwFlags & TSF_POSLEFT)
					pt.x = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left) / 12;
				else if (g_dwFlags & TSF_POSRIGHT) {
					pt.x = mi.rcMonitor.left + ((mi.rcMonitor.right - \
						mi.rcMonitor.left) * 11) / 12 - g_rcTs.right;
				} else pt.x = (mi.rcMonitor.right + mi.rcMonitor.left - g_rcTs.right) / 2;

				if (g_dwFlags & TSF_POSTOP)
					pt.y = mi.rcMonitor.top + (mi.rcMonitor.bottom - mi.rcMonitor.top) / 8;
				else if (g_dwFlags & TSF_POSBOTTOM)
					pt.y = mi.rcMonitor.top + ((mi.rcMonitor.bottom - mi.rcMonitor.top) * 7) / 8 - g_rcTs.bottom;
				else pt.y = (mi.rcMonitor.bottom + mi.rcMonitor.top - g_rcTs.bottom) / 2;

			} else {

				int cScreen = GetSystemMetrics(SM_CXSCREEN);
				if (g_dwFlags & TSF_POSLEFT)
					pt.x = cScreen / 12;
				else if (g_dwFlags & TSF_POSRIGHT)
					pt.x = (cScreen * 11) / 12 - g_rcTs.right;
				else pt.x = (cScreen - g_rcTs.right) / 2;

				cScreen = GetSystemMetrics(SM_CYSCREEN);
				if (g_dwFlags & TSF_POSTOP)
					pt.y = cScreen / 8;
				else if (g_dwFlags & TSF_POSBOTTOM)
					pt.y = (cScreen * 7) / 8 - g_rcTs.bottom;
				else pt.y = (cScreen - g_rcTs.bottom) / 2;

				/*pt.x = ptCur.x - g_rcTs.right / 2;
				cScreen = GetSystemMetrics(SM_XVIRTUALSCREEN);
				if (pt.x < cScreen)
					pt.x = cScreen;
				else {
					cScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN) - g_rcTs.right;
					if (pt.x > cScreen)
						pt.x = cScreen;
				}
				pt.y = ptCur.y - g_rcTs.bottom / 2;
				cScreen = GetSystemMetrics(SM_YVIRTUALSCREEN);
				if (pt.y < cScreen)
					pt.y = cScreen;
				else {
					cScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN) - g_rcTs.bottom;
					if (pt.y > cScreen)
						pt.y = cScreen;
				}*/
			}

			SetForegroundWindow(hwnd);
			SetWindowPos(hwnd, HWND_TOPMOST, pt.x, pt.y, g_rcTs.right, g_rcTs.bottom, 0);

			if (!SelectTask(hwnd, g_nCurTs, TRUE))
				return(-1);

			if (g_dwFlagsList & TSFL_SHOWTOOLTIPS)
				s_hwndTip = CreateToolTip(hwnd);

			s_fHand = FALSE;

			if (MapWindowPoints(NULL, hwnd, &ptCur, 1)) {
				s_lParamMove = MAKELPARAM(ptCur.x, ptCur.y);
				if (g_dwFlagsList & TSFL_HOTTRACK) {
					int nSelTs = ListItemFromPoint(ptCur);
					s_fHand = (BOOL)((nSelTs >= 0 && (g_nFirstTs + nSelTs) < g_nTs) || 
						((g_dwFlagsPv & TSFPV_DESKTOP || !(g_dwFlagsPv & TSFPV_OLDSTYLE)) &&
						PtInRect(&g_rcPvEx, ptCur)));
				} else {
					s_fHand = FALSE;
				}
			}

			if (g_dwFlags & TSF_LAYERED || g_dwFadeIn & TSFBL_ENABLED) {
				SetWindowLongPtr(hwnd, GWL_EXSTYLE, 
					GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
				DWORD dwAlpha;
				if (g_dwFadeIn & TSFBL_ENABLED) {
                    dwAlpha = 55;
					g_dwFadeIn &= ~TSBL_LAYEREDMASK;
					g_dwFadeIn |= dwAlpha << 24;
				} else if (g_dwFlags & TSF_LAYERED) {
					dwAlpha = (g_dwFlags & TSF_LAYEREDMASK);
				} else dwAlpha = 255;
				SetLayeredWindowAttributes(hwnd, 0, (BYTE)dwAlpha, LWA_ALPHA);
				if (g_dwFadeIn & TSFBL_ENABLED)
					SetTimer(hwnd, TIMER_FADEIN, FADE_DELAY, NULL);
			}
			break;
						}

		case WM_DESTROY: {

			KillTimer(hwnd, TIMER_PREVIEW);
			KillTimer(hwnd, TIMER_INFOTIP);
			KillTimer(hwnd, TIMER_FADEIN);

			if (s_hwndTip) {
				DestroyWindow(s_hwndTip);
				s_hwndTip = NULL;
			}
			if (g_hwndInfo) {
				DestroyWindow(g_hwndInfo);
				g_hwndInfo = NULL;
			}
            DestroyTsBackground();
			if (g_hwndFrgnd) {
				MySwitchToThisWindow(g_hwndFrgnd);
				g_hwndFrgnd = NULL;
			}
			g_nCurTs = -1;
			g_hwndTs = NULL;
			g_dwFadeOut &= ~TSFBL_STARTED;

			PostQuitMessage(0);
			break;
						 }

		default:
			return(DefWindowProc(hwnd, uMsg, wParam, lParam));
	}
	return(0);
}

//-----------------------------------------------------------
/*
LRESULT CALLBACK TsBkWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
	switch (uMsg) {

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
			GetClientRect(hwnd, &rc);
			FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNTEXT));
			EndPaint(hwnd, &ps);
			break;
					   }

		case WM_CREATE:
			SetLayeredWindowAttributes(hwnd, 0, 40, LWA_ALPHA);
			break;
		case WM_DESTROY:
			break;

		default:
			return(DefWindowProc(hwnd, uMsg, wParam, lParam));
	}
	return(0);
}*/
//-----------------------------------------------------------
//-----------------------------------------------------------

//Function must read value from g_szTypeBuffer
//and tries to find matching entries in task list.
//If there is only one matching entry, switch to appropriate task immediately.
void ProcessTypeBuffer(HWND hwnd) //tag kl2
{
	wsprintf(tmp, L"ProcessTypeBuffer called, value: %s\n", g_szTypeBuffer);
	OutputDebugString(tmp);
	WCHAR ss[MAX_PATH];
	int ret = 0;
	int Matched[4000]; // hope nobody will run more than 4000 tasks
	int NumMatched = 0; // count matching entries
	ZeroMemory (Matched, sizeof (Matched) );
	int tblen = lstrlen(g_szTypeBuffer);
  //
  // перечитываем список окон (нужно для реализации
  // фильтрации по введенным символам)
  OutputDebugString(L"removed, about to reset task list\r\n");
  MyEnumDesktopWindows();
  SortTaskListByModuleName(hwnd);
  //
	if (tblen <= 0)	
	{
		SelectTask(hwnd, 0, FALSE); 
    return;
	}
	for (int i = 0; i < g_nTs; i++)
	{
		ret = (int) lstrcpyn(ss, g_pTs[i].szModuleName, tblen + 1 );
		if (ret != NULL)
		{
			if (lstrcmpi (ss, g_szTypeBuffer) == 0)
			{
				NumMatched ++ ;
				Matched [i] = 1;
				//now we must select first matching task
				if (NumMatched == 1) SelectTask(hwnd, i, FALSE);
			}
		}
	}
  if (NumMatched == 0) {
    return;
  }
	//if there is only one matching task, switch to it immediately
  else if (NumMatched == 1) {
    wsprintf(tmp, L"Single matching task, switching to it.");
    OutputDebugString(tmp);
    SwitchToCurTask(hwnd);
  }
	else { // do some clever things... may be later
    // удалить из списка то, что не начинается с введенных символов
    // с помощью DeleteTsItem
    wsprintf(tmp, L"Number of matching tasks: %d, about to delete non-matched\n",
        NumMatched);
    OutputDebugString(tmp);
    //wsprintf(tmp, L"g_nCurTs: %d\n", g_nCurTs);
    //OutputDebugString(tmp);
    //g_nCurTs
    for (int i = g_nTs - 1; i >= 0; i--)
    {
      if (Matched[i] != 1)
      {
        wsprintf(tmp, L"About to delete item: %d\n", i);
        OutputDebugString(tmp);
        DeleteTsItem(i);
      }
    }
    SortTaskListByModuleName(hwnd);
		SelectTask(hwnd, 0, FALSE); 
	}
}
