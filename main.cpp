// main.cpp

#include "stdafx.h"
#include "generic.h"
#include "TaskSwitchXP.h"
#include "registry.h"
#include "lang.h"
#include "main.h"
#include "resource.h"


//-----------------------------------------------------------

const WCHAR g_szRegKeyTs[]				= RS_TASKSWITCHXP_KEY;

const WCHAR g_szTaskSwitch[]			= L"_As12__TaskSwitchXP_";
const WCHAR g_szMainWnd[]				= L"_As12__TaskSwitchXP_MainWnd_";
const WCHAR g_szTaskSwitchWnd[]			= L"_As12__TaskSwitchXP_TaskSwitchWnd_";
//const WCHAR g_szTsBkClass[]				= L"_As12__TaskSwitchXP_BackgroundWnd_";
const WCHAR g_szWindowName[]			= L"TaskSwitchXP Pro 2.0";

HINSTANCE g_hinstExe					= NULL;
HWND g_hwndMain							= NULL;
HICON g_hIconTray						= NULL;
ISHUNGAPPWINDOW g_pfnIsHungAppWindow	= NULL;
//ENDTASK g_pfnEndTask					= NULL;
DWORD g_dwCmdLine						= 0;

#define IDC_FIRSTTRAYICON				1000

HHOOK g_hhookMouse						= NULL;
WNDTRAYINFO g_pWti[MAX_WNDTRAY]		    = { { 0, 0 } };
UINT g_cWti								= 0;


POINT _ptRClick = { 0, 0 };
DWORD _dwRData = 0;
DWORD _dwRExtraInfo = 0;

//-----------------------------------------------------------

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
void _UnflipFromTray(UINT, UINT uCmd =SC_RESTORE);
void ShowFlippedSystemMenu(UINT);
BOOL FlipToTray(HWND);
void UnflipFromTrayID(UINT, UINT uCmd =SC_RESTORE);
void UnflipFromTray(HWND, UINT uCmd =SC_RESTORE);

//-----------------------------------------------------------

BOOL CheckVersion() {
	OSVERSIONINFO vi = { sizeof(OSVERSIONINFO) };
	if (GetVersionEx(&vi)) {
		if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT && 
			vi.dwMajorVersion >= 5 && vi.dwMinorVersion >= 1) 
			return(TRUE);
	}
	return(FALSE);
}

//-----------------------------------------------------------

void ParseCommandLine() {
	
	int nNumArgs;
	PWSTR *ppArgv = CommandLineToArgvW(GetCommandLineW(), &nNumArgs);

	g_dwCmdLine = 0;

	for (int i = 1; i < nNumArgs; i++) {
		if (ppArgv[i][0] == L'/' || ppArgv[i][0] == L'-') {
			if (!lstrcmpi(ppArgv[i] + 1, L"nocheckver")) {
				g_dwCmdLine |= CCLF_NOCHECKVER;
			} else if (!lstrcmpi(ppArgv[i] + 1, L"list")) {
				g_dwCmdLine |= CCLF_STNEXTTASK;
			} else if (!lstrcmpi(ppArgv[i] + 1, L"inst-list")) {
				g_dwCmdLine |= CCLF_STINEXTTASK;
			} else if (!lstrcmpi(ppArgv[i] + 1, L"escape")) {
				g_dwCmdLine |= CCLF_ESCAPE;
			}
		}
	}
	HeapFree(GetProcessHeap(), 0, ppArgv);
}

//-----------------------------------------------------------

#ifdef _DEBUG
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
#else // _DEBUG
void APIENTRY _myWinMain() {
#endif // _DEBUG

    g_hinstExe = GetModuleHandle(NULL);

	int nExitCode = 1;
	//HANDLE hmtx = NULL;
    
	__try {

		ParseCommandLine();

		if (!CheckVersion() && !(g_dwCmdLine & CCLF_NOCHECKVER)) {
			MessageBoxA(NULL, "This program requires features present in Windows XP/2003.", 
				"Error", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
			__leave;
		}

		/*hmtx = CreateMutex(NULL, FALSE, g_szTaskSwitch);
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			ReportError(IDS_ERR_EXISTS);
			__leave;
		}*/

		InitThreadLang2();
		InitLanguage();

		HWND hwndTs = FindWindowEx(NULL/*HWND_MESSAGE*/, NULL, g_szMainWnd, g_szWindowName);
		if (IsWindow(hwndTs)) {
			if (g_dwCmdLine != 0) {
				SetForegroundWindow(hwndTs);
				PostMessage(hwndTs, WM_REMOTECMD, g_dwCmdLine, 0);
			} else {
				if (ConfirmMessage(IDS_CONFIRM_RUNNING, MB_ICONEXCLAMATION | MB_DEFBUTTON2))
					PostMessage(hwndTs, WM_DESTROY, 0, 0);
				//PostMessage(hwndTs, WM_RELOADSETTINGS, 0, 0);
			}
			__leave;
		}

		HINSTANCE hinstUser32 = LoadLibrary(L"user32.dll");
		g_pfnIsHungAppWindow = (ISHUNGAPPWINDOW)GetProcAddress(hinstUser32, "IsHungAppWindow");
		//g_pfnEndTask = (ENDTASK)GetProcAddress(hinstUser32, "EndTask");

		WNDCLASSEX wcex;
		wcex.cbSize			= sizeof(WNDCLASSEX);
		wcex.style			= 0;
		wcex.lpfnWndProc	= (WNDPROC)MainWndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= g_hinstExe;
		wcex.hIcon			= NULL;
		wcex.hCursor		= NULL;
		wcex.hbrBackground	= NULL;
		wcex.lpszMenuName	= NULL;
		wcex.lpszClassName	= g_szMainWnd;
		wcex.hIconSm		= NULL;
		if (!RegisterClassEx(&wcex))
			__leave;

		if (!CreateWindowEx(WS_EX_TOOLWINDOW, g_szMainWnd, g_szWindowName, 
			0, 0, 0, 0, 0, NULL/*HWND_MESSAGE*/, NULL, g_hinstExe, NULL)) __leave;

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		nExitCode = (int)msg.wParam;
	}
	__finally {
		DestroyThreadLang2();
		//if (hmtx) CloseHandle(hmtx);		
	}
#ifdef _DEBUG
	return(nExitCode);
#else // _DEBUG
	ExitProcess((UINT)nExitCode);
#endif
}

//-----------------------------------------------------------
//-----------------------------------------------------------

BOOL _GetIconPathIndex(LPWSTR szIconFile, int* pnIconIndex) {

	_ASSERT(szIconFile);

	int nIconIndex = 0;
	int n = lstrlen(szIconFile) - 1, t = 1;
	while (n >= 0 && szIconFile[n] >= L'0' && szIconFile[n] <= L'9' && t < 100000) {
		nIconIndex += (szIconFile[n] - L'0') * t;
		t *= 10;
		n--;
	}

	if (szIconFile[n] == L',') {
		szIconFile[n] = L'\0';
		if (pnIconIndex)
			*pnIconIndex = nIconIndex;
		return(TRUE);
	}
	return(FALSE);
}


BOOL ShowTrayIcon(BOOL fShow) {

	BOOL fRet = DeleteTrayIcon(g_hwndMain, IDI_TASKSWITCHXP);
	g_dwFlags &= ~TSF_SHOWTRAYICON;

	if (g_hIconTray) {
		DestroyIcon(g_hIconTray);
		g_hIconTray = NULL;
	}

	if (fShow) {

		HICON hIconSm = NULL;
		if (g_dwFlags & TSF_USECUSTOMICON) {
			HKEY hkey = NULL;
			if (!RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
				WCHAR szBuff[MAX_DATALEN] = L"";
				DWORD cbData = MAX_DATALEN * sizeof(WCHAR);
				if (!RegQueryValueEx(hkey, RS_CUSTOMICON, 0, NULL, (PBYTE)szBuff, &cbData)) {
					int nIconIndex;
					if (_GetIconPathIndex(szBuff, &nIconIndex)) {
						if (!ExtractIconEx(szBuff, nIconIndex, NULL, &hIconSm, 1))
							hIconSm = NULL;
					}
				}
				RegCloseKey(hkey);
			}
		}

		if (!hIconSm) {
			g_hIconTray = (HICON)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_TASKSWITCHXP), 
				IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		} else {
			g_hIconTray = CopyIcon(hIconSm);
			DestroyIcon(hIconSm);
		}

		if (!g_hIconTray)
			g_hIconTray = LoadIcon(NULL, IDI_WARNING);

		fRet = AddTrayIcon(g_hwndMain, IDI_TASKSWITCHXP, g_hIconTray, g_szWindowName);
		g_dwFlags |= TSF_SHOWTRAYICON;
	}
	return(fRet);
}

//-----------------------------------------------------------

BOOL HelpTaskSwitchXP() {

	BOOL fSuccess = FALSE;

	static const WCHAR s_szHelp[] = L"TaskSwitchXP.chm";
	WCHAR szBuff[MAX_DATALEN] = L"";
	const int nMaxLen = MAX_DATALEN - SIZEOF_ARRAY(s_szHelp);

	int n = GetModuleFileName(g_hinstExe, szBuff, nMaxLen);
	if (n != 0 && n < nMaxLen) {
		while (n >= 0 && szBuff[n] != L'\\') n--;
		lstrcpyn(szBuff + n + 1, s_szHelp, SIZEOF_ARRAY(szBuff) - n);
		fSuccess = (BOOL)((INT_PTR)ShellExecute(NULL, L"open", szBuff, NULL, NULL, SW_SHOWNORMAL) > 32);
	}
	return(fSuccess);
}


BOOL ConfigTaskSwitchXP(LPCWSTR szParams) {

	_ASSERT(lstrlen(szParams) <= 32);

	BOOL fSuccess = FALSE;

	static const WCHAR s_szConfig[] = L"ConfigTsXP.exe";
	WCHAR szBuff[MAX_DATALEN + 32] = L"";
	const int nMaxLen = MAX_DATALEN - SIZEOF_ARRAY(s_szConfig);

	int n = GetModuleFileName(g_hinstExe, szBuff, nMaxLen);
	if (n != 0 && n < nMaxLen) {

		while (n >= 0 && szBuff[n] != L'\\') n--;
		lstrcpyn(szBuff + n + 1, s_szConfig, SIZEOF_ARRAY(szBuff) - n);
		if (szParams) {
			lstrcat(szBuff, L" ");
			lstrcat(szBuff, szParams);
		}

		STARTUPINFO si = { sizeof(STARTUPINFO) };
		PROCESS_INFORMATION pi;

		fSuccess = CreateProcess(NULL, szBuff, NULL, 
			NULL, FALSE, 0, NULL, NULL, &si, &pi);
		if (fSuccess) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		} else 
			ReportError(IDS_ERR_CONFIGTSXP);
	} else 
		ReportError(IDS_ERR_CONFIGTSXP); //ReportError(IDS_ERR_NOINSTDIR);

	return(fSuccess);
}

//-----------------------------------------------------------

void ShowTrayMenu() {

	POINT pt;
	GetCursorPos(&pt);

	WCHAR szBuff[MAX_LANGLEN];

	HMENU hmenu = CreatePopupMenu();
	for (int i = 0; i <= (IDM_EXIT - IDM_STTRAYNEXT); i++) {
		LangLoadString(IDS_STTRAYNEXT + i, szBuff, SIZEOF_ARRAY(szBuff));
		AppendMenu(hmenu, MF_STRING, IDM_STTRAYNEXT + i, szBuff);
	}
	InsertMenu(hmenu, IDM_CONFIG, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	InsertMenu(hmenu, IDM_EXIT, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);

	HMENU hmenuPopup = CreatePopupMenu();
	for (int i = 0; i <= (IDM_ABOUT - IDM_DOCUMENTATION); i++) {
		LangLoadString(IDS_DOCUMENTATION + i, szBuff, SIZEOF_ARRAY(szBuff));
		AppendMenu(hmenuPopup, MF_STRING, IDM_DOCUMENTATION + i, szBuff);
	}
	LangLoadString(IDS_HELP, szBuff, SIZEOF_ARRAY(szBuff));
	InsertMenu(hmenu, IDM_CONFIG, MF_STRING | MF_BYCOMMAND | MF_POPUP, 
		(UINT_PTR)hmenuPopup, szBuff);
	InsertMenu(hmenu, 4, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);

	hmenuPopup = CreatePopupMenu();
	for (int i = 0; i <= (IDM_NEWEXCLUSION - IDM_HIDE); i++) {
		LangLoadString(IDS_HIDE + i, szBuff, SIZEOF_ARRAY(szBuff));
		AppendMenu(hmenuPopup, MF_STRING, IDM_HIDE + i, szBuff);
	}
	InsertMenu(hmenuPopup, IDM_NEWEXCLUSION, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	InsertMenu(hmenuPopup, IDM_REPLACEALTTAB, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	CheckMenuItem(hmenuPopup, IDM_REPLACEALTTAB, !(g_dwFlags & TSF_NOREPLACEALTTAB) 
		? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));
	CheckMenuItem(hmenuPopup, IDM_INSTSWITCHER, (g_dwFlags & TSF_INSTSWITCHER) 
		? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));
	CheckMenuItem(hmenuPopup, IDM_STICKYALTTAB, (g_dwFlags & TSF_STICKYALTTAB) 
		? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));
	CheckMenuItem(hmenuPopup, IDM_HOOKALTTAB, (g_dwFlags & TSF_HOOKALTTAB) 
		? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));
//	CheckMenuItem(hmenuPopup, IDM_EXTMOUSE, (g_dwFlags & TSF_EXTMOUSE) 
//		? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));
	LangLoadString(IDS_QUICKCONFIG, szBuff, SIZEOF_ARRAY(szBuff));
	InsertMenu(hmenu, IDM_CONFIG, MF_STRING | MF_BYCOMMAND | MF_POPUP, 
		(UINT_PTR)hmenuPopup, szBuff);

	SetForegroundWindow(g_hwndMain);
	UINT uMenuID = (UINT)TrackPopupMenu(hmenu, 
		TPM_RIGHTBUTTON | TPM_RIGHTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, 
		pt.x, pt.y, 0, g_hwndMain, NULL);
	PostMessage(g_hwndMain, WM_NULL, 0, 0);
	DestroyMenu(hmenu);

	switch (uMenuID) {
		case IDM_STTRAYNEXT:
			ShowTaskSwitchWnd(IDH_STTRAYNEXT);
			break;
		case IDM_STITRAYNEXT:
			ShowTaskSwitchWnd(IDH_STITRAYNEXT);
			break;
		case IDM_CONFIG:
			ConfigTaskSwitchXP();
			break;
		case IDM_HIDE:
			ShowTrayIcon(FALSE);
			break;
		case IDM_REPLACEALTTAB:
			ReplaceAltTab((g_dwFlags & TSF_NOREPLACEALTTAB) 
				? (g_dwFlags & ~TSF_NOREPLACEALTTAB) : (g_dwFlags | TSF_NOREPLACEALTTAB));
			break;
		case IDM_INSTSWITCHER:
			ReplaceAltTab((g_dwFlags & TSF_INSTSWITCHER) 
				? (g_dwFlags & ~TSF_INSTSWITCHER) : (g_dwFlags | TSF_INSTSWITCHER));
			break;
		case IDM_STICKYALTTAB:
			g_dwFlags ^= TSF_STICKYALTTAB;
			break;
		case IDM_HOOKALTTAB:
			ReplaceAltTab((g_dwFlags & TSF_HOOKALTTAB) 
				? (g_dwFlags & ~TSF_HOOKALTTAB) : (g_dwFlags | TSF_HOOKALTTAB));
			break;
//		case IDM_EXTMOUSE:
//			EnableExtMouse(!(g_dwFlags & TSF_EXTMOUSE), g_dwFlags & TSF_WHEELTAB);
//			break;
		case IDM_NEWEXCLUSION:
			ConfigTaskSwitchXP(L"/newexcl");
			break;
		case IDM_DOCUMENTATION:
			HelpTaskSwitchXP();
			break;
		case IDM_HOMEPAGE:
			ShellExecute(NULL, L"open", L"http://www.ntwind.com/taskswitchxp/", NULL, NULL, SW_SHOWNORMAL);
			break;
		case IDM_ABOUT:
			ConfigTaskSwitchXP(L"/about");
			break;
		case IDM_EXIT:
			DestroyWindow(g_hwndMain);
			break;
	}
}

//-----------------------------------------------------------------

void ReloadTsTrayIcons() {
	if (g_dwFlags & TSF_SHOWTRAYICON) {
		if (!ReloadTrayIcon(g_hwndMain, IDI_TASKSWITCHXP))
			ShowTrayIcon(TRUE);
	}
	for (int i = (int)g_cWti - 1; i >= 0; i--) {
		if (!ReloadTrayIcon(g_hwndMain, g_pWti[i].uID)) {
			if (IsWindow(g_pWti[i].hwnd)) {
				WCHAR szCaption[MAX_CAPTION];
				InternalGetWindowText(g_pWti[i].hwnd, szCaption, MAX_CAPTION);
				AddTrayIcon(g_hwndMain, g_pWti[i].uID, g_pWti[i].hIconSm, szCaption);
			}
		}
	}
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	static BOOL s_fConfirmActive = FALSE;
	static UINT s_uTaskbarCreated = 0;

	switch (uMsg) {

		case WM_HOTKEY: {

			switch (wParam) {

				case IDH_NEXTTASK:
				case IDH_PREVTASK:
				case IDH_WINNEXTTASK:
				case IDH_WINPREVTASK:
				case IDH_INSTNEXT:
				case IDH_INSTPREV:
				case IDH_STNEXTTASK:
				case IDH_STINEXTTASK:
					ShowTaskSwitchWnd((UINT)wParam);
					break;

				case IDH_MINIMIZETRAY:
					FlipToTray(GetForegroundWindow());
					break;

				case IDH_RESTORETRAY:
					if (g_cWti > 0)
						_UnflipFromTray(g_cWti - 1);
					break;

				case IDH_SHOWHIDE:
					ShowTrayIcon(!(g_dwFlags & TSF_SHOWTRAYICON));
					break;

				case IDH_CONFIG:
					ConfigTaskSwitchXP();
					break;
				case IDH_ALTAPPLIST:
					//MessageBox (0, L"This must display alternative task switching window\r\n"
					//	L"(or the same window but in alternative mode)", L"klvov says:", MB_OK);
					//ShowAltTSWindow();
					//OutputDebugString(L"IDH_ALTAPPLIST message received\n");
					ShowTaskSwitchWnd((UINT)wParam);
					break;

				case IDH_EXIT:
					if (!(g_dwFlags & TSF_NOCONFIRMEXIT)) {
						if (s_fConfirmActive) { // activate confirm dialog
#pragma FIX_LATER(multiple exit confirmations)
							break;
							/*HWND hwndMsg = FindWindowEx(NULL, NULL, L"#32770", L"TaskSwitchXP confirmation");
							if (hwndMsg) {
								SetForegroundWindow(hwndMsg);
								break;
							}*/
						}

						s_fConfirmActive = TRUE;
						if (ConfirmMessage(IDS_CONFIRM_EXIT))
							DestroyWindow(hwnd);
						s_fConfirmActive = FALSE;
					} else 
						DestroyWindow(hwnd);
					break;
			}
			break;
							}

		case WM_TIMER:
			if (wParam == TIMER_RIGHTUP) {
				KillTimer(hwnd, TIMER_RIGHTUP);
				mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP, 
					_ptRClick.x, _ptRClick.y, _dwRData, _dwRExtraInfo);
			} else if (wParam == TIMER_SETANIMATION) {
				KillTimer(hwnd, TIMER_SETANIMATION);
				ANIMATIONINFO ai;
				ai.cbSize = sizeof(ANIMATIONINFO);
				ai.iMinAnimate = TRUE;
				SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &ai, FALSE);
			} else if (wParam == TIMER_CHECKTRAYWND) {
				for (UINT i = 0; i < g_cWti; i++) {
					if (!IsWindow(g_pWti[i].hwnd) || IsWindowVisible(g_pWti[i].hwnd))
						_UnflipFromTray(i, SC_MINIMIZE);
				}
			} else if (wParam == TIMER_RELOADICONS) {
				if (!(g_dwFlags & TSF_RELOADICONS))
					KillTimer(hwnd, TIMER_RELOADICONS);
				ReloadTsTrayIcons();
			} else if (wParam == TIMER_CHECKCOLORS) {
				KillTimer(hwnd, TIMER_CHECKCOLORS);
				if (CheckColorTheme())
					CheckDefaultColors();
			} else if (wParam == TIMER_CLOSEDESK) {
				if (WaitForSingleObject(g_hThreadTs, 0) == WAIT_OBJECT_0) {
					KillTimer(hwnd, TIMER_CLOSEDESK);
					//_RPT0(_CRT_WARN, "close desk\n");
					if (g_hThreadTs) {
						CloseHandle(g_hThreadTs);
						g_hThreadTs = NULL;
					}
					if (g_hDesk) {
						CloseDesktop(g_hDesk);
						g_hDesk = NULL;
					}
				}
			}
			break;

		case WM_REMOTECMD:
			SwitchToThisWindow(hwnd, TRUE);
			if (wParam & CCLF_ESCAPE) {
				if (g_hwndTs)
					PostMessage(g_hwndTs, WM_TASKSWITCH, IDH_ESCAPE, 0);
			} else if (wParam & CCLF_STINEXTTASK) {
				ShowTaskSwitchWnd(IDH_STINEXTTASK);
			} else if (wParam & CCLF_STNEXTTASK) {
				ShowTaskSwitchWnd(IDH_STNEXTTASK);
			}
			break;

		case WM_MYTRAYMSG:
			if (wParam == IDI_TASKSWITCHXP) {
				if ((UINT)lParam == g_uTrayMenu) {
					ShowTrayMenu();
				} else if ((UINT)lParam == g_uTrayConfig) {
					ConfigTaskSwitchXP();
				} else if ((UINT)lParam == g_uTrayNext) {
					ShowTaskSwitchWnd(IDH_STTRAYNEXT);
				} else if ((UINT)lParam == g_uTrayPrev) {
					ShowTaskSwitchWnd(IDH_STITRAYNEXT);
				}
			} else {
				if (lParam == WM_LBUTTONUP) {
                    UnflipFromTrayID((UINT)wParam);
				} else if (lParam == WM_RBUTTONUP) {
					ShowFlippedSystemMenu((UINT)wParam);
				} else if (lParam == WM_MBUTTONUP)
					UnflipFromTrayID((UINT)wParam, 0);
			}
			break;

		case WM_EXTMOUSE:
			if (wParam == HTMINBUTTON) {
				FlipToTray((HWND)lParam);
			} else if (wParam == HTMAXBUTTON) {
				HWND h = (HWND)lParam;
				DWORD dwExStyle = GetWindowLongPtr(h, GWL_EXSTYLE);
				SetWindowPos(h, ((dwExStyle & WS_EX_TOPMOST) 
					? HWND_NOTOPMOST : HWND_TOPMOST), 0, 0, 0, 0, 
					SWP_NOSIZE | SWP_NOMOVE);
			}
			break;

		case WM_FLIPTOTRAY:
			return(FlipToTray((HWND)wParam));
			//break;

		case WM_UNFLIPFROMTRAY:
			UnflipFromTray((HWND)wParam, (UINT)lParam);
			break;

		case WM_GETTRAYWINDOWS:
			if (wParam) {
				HWND *phWnd = (HWND*)wParam;
				int cWti = 0;
				while (cWti < MIN((int)lParam, (int)g_cWti)) {
					phWnd[cWti] = g_pWti[cWti].hwnd;
					cWti++;
				}
				return(cWti);
			} else 
				return(g_cWti);
			break;

		case WM_RELOADSETTINGS:
			if (!LoadSettings()) {
				ReportError(IDS_ERR_LOADSETTINGS);
				DestroyWindow(hwnd);
			}
			break;

		case WM_THEMECHANGED:
			if (CheckColorTheme())
				CheckDefaultColors();
			SetTimer(hwnd, TIMER_CHECKCOLORS, 1000, NULL);
			break;

		case WM_CREATE: {

			s_uTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");
			g_hwndMain = hwnd;

			SendMessage(hwnd, WM_SETICON, ICON_BIG, 
				(LONG)(LONG_PTR)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_TASKSWITCHXP), IMAGE_ICON, 
				GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, 
				(LONG)(LONG_PTR)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_TASKSWITCHXP), IMAGE_ICON, 
				GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));

			if (!LoadSettings()) {
				ReportError(IDS_ERR_LOADSETTINGS);
				return(-1);
			}
			if (!InitPreviewThread())
				return(-1);

			// Weird huh? I cannot understand this...
			if (g_dwShowDelay && g_dwFlagsPv & TSFPV_DESKTOP && g_dwFlagsPv & TSFPV_TASKBAR) {

				RECT rcPvEx;
				rcPvEx.left = rcPvEx.top = 0;
				rcPvEx.right = 102;
				rcPvEx.bottom = 76;
				HDC hdcScreen = GetDC(NULL);
				HDC hdcMem = CreateCompatibleDC(hdcScreen);
				HBITMAP hbitmapMem = CreateCompatibleBitmap(hdcScreen, 
					rcPvEx.right - rcPvEx.left, rcPvEx.bottom - rcPvEx.top);
				ReleaseDC(NULL, hdcScreen);

				if (hbitmapMem) {
					HBITMAP hbitmapOld = (HBITMAP)SelectObject(hdcMem, hbitmapMem);

					SetForegroundWindow(g_hwndMain);
					MyPrintWindow(FindWindow(L"Shell_TrayWnd", L""), 
						hdcMem, &rcPvEx, &rcPvEx, MPW_NOCHECKPOS);

					SelectObject(hdcMem, hbitmapOld);
					DeleteObject(hbitmapMem);
				}
				DeleteDC(hdcMem);
			}
			SetTimer(hwnd, TIMER_CHECKTRAYWND, 1000, NULL);

			if (g_dwCmdLine & CCLF_STINEXTTASK) {
				ShowTaskSwitchWnd(IDH_STINEXTTASK);
			} else if (g_dwCmdLine & CCLF_STNEXTTASK) {
				ShowTaskSwitchWnd(IDH_STNEXTTASK);
			}
			break;
						}

		case WM_CLOSE:
			break;

		case WM_DESTROY:
			KillTimer(hwnd, TIMER_CHECKTRAYWND);
			DestroySettings();
			DestroyPreviewThread();
			for (int i = (int)g_cWti - 1; i >= 0; i--)
				_UnflipFromTray((UINT)i, 0);
			PostQuitMessage(0);
			break;

		default:
			if (uMsg == s_uTaskbarCreated) {
				ReloadTsTrayIcons();
			} else 
				return(DefWindowProc(hwnd, uMsg, wParam, lParam));
			break;
	}
	return(0);
}

//-----------------------------------------------------------
//-----------------------------------------------------------

BOOL FlipToTray(HWND hwnd) {

	if (g_cWti >= MAX_WNDTRAY)
		return(FALSE);
	if (!IsWindow(hwnd))
		return(FALSE);

	HWND hwndShell = GetShellWindow(), hwndTmp, hwndOwner;
	hwndTmp = hwndOwner = hwnd;
	do {
		hwndOwner = hwndTmp;
		hwndTmp = GetWindow(hwndTmp, GW_OWNER);
	} while (hwndTmp && hwndTmp != hwndShell);

	DWORD dw = GetWindowLongPtr(hwndOwner, GWL_STYLE);
	if (!(dw & WS_MINIMIZEBOX) || dw & WS_CHILD)
		return(FALSE);

	//SetForegroundWindow(g_hwndMain);
	//MySwitchToThisWindow(GetShellWindow());

	for (UINT i = 0; i < g_cWti; i++) {
		if (g_pWti[i].hwnd == hwndOwner) {
			if (IsWindowVisible(hwndOwner))
				_UnflipFromTray(i, 0);
			else return(FALSE);
		}
	}

	UINT uID = IDC_FIRSTTRAYICON;
	if (g_cWti > 0) {
		while (uID < IDC_FIRSTTRAYICON + MAX_WNDTRAY) {
			BOOL fIs1 = TRUE;
			for (UINT i = 0; i < g_cWti; i++) {
				if (g_pWti[i].uID == uID)
					fIs1 = FALSE;
			}
			if (fIs1) break;
			uID++;
		}
	}

	HICON hIcon, hIconSm;
	GetWindowIcons(hwndOwner, &hIcon, &hIconSm);

	WCHAR szCaption[MAX_CAPTION];
	InternalGetWindowText(hwndOwner, szCaption, MAX_CAPTION);
	if (szCaption[0] == L'\0' && hwnd != hwndOwner) {
		InternalGetWindowText(hwnd, szCaption, MAX_CAPTION);
	}

	if (MyIsHungAppWindow(hwndOwner))
		return(FALSE);

	RECT rcTray = { 0 };

	g_pWti[g_cWti].fAnimate = FALSE;

	if (!(dw & WS_MINIMIZE)) {

		GetWindowRect(hwnd, &g_pWti[g_cWti].rcWnd);
	
		ANIMATIONINFO ai;
		ai.cbSize = sizeof(ANIMATIONINFO);
		ai.iMinAnimate = FALSE;
		SystemParametersInfo(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &ai, FALSE);

		if (ai.iMinAnimate) {
			HWND hwndTray = FindTrayToolbarWindow();
			if (hwndTray) {
				GetWindowRect(hwndTray, &rcTray);
				rcTray.bottom = rcTray.top;
				rcTray.right = rcTray.left;

				KillTimer(g_hwndMain, TIMER_SETANIMATION);
				g_pWti[g_cWti].fAnimate = TRUE;			
				ai.iMinAnimate = 0;
				SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &ai, FALSE);
				SetTimer(g_hwndMain, TIMER_SETANIMATION, 500, NULL);

				DrawAnimatedRects(g_hwndMain, IDANI_CAPTION, 
					&g_pWti[g_cWti].rcWnd, &rcTray);
			}
		}

		//ShowWindow(hwndOwner, SW_MINIMIZE);
		if (!SendMessageTimeout(hwndOwner, WM_SYSCOMMAND, SC_MINIMIZE, 0, 
			SMTO_ABORTIFHUNG, 500, &dw)) return(FALSE);
	}

	//ShowWindow(hwndOwner, SW_HIDE);
	SetWindowPos(hwndOwner, NULL, 0, 0, 0, 0, 
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);

	//if (g_pWti[g_cWti].fAnimate) {
	//}

	g_pWti[g_cWti].hwnd = hwndOwner;
	g_pWti[g_cWti].uID = uID;
	g_pWti[g_cWti].hIconSm = hIconSm;
	g_cWti++;
	AddTrayIcon(g_hwndMain, uID, hIconSm, szCaption);
	return(TRUE);
}

//-----------------------------------------------------------------

void _UnflipFromTray(UINT i, UINT uCmd) {

	if (MyIsHungAppWindow(g_pWti[i].hwnd))
		return;

	DeleteTrayIcon(g_hwndMain, g_pWti[i].uID);
	if (IsWindow(g_pWti[i].hwnd)) {

		RECT rcTray = { 0 };

		if (g_pWti[i].fAnimate && uCmd == SC_RESTORE) {

			ANIMATIONINFO ai;
			ai.cbSize = sizeof(ANIMATIONINFO);
			ai.iMinAnimate = FALSE;
			SystemParametersInfo(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &ai, FALSE);

			if (ai.iMinAnimate) {
				HWND hwndTray = FindTrayToolbarWindow();
				if (hwndTray) {
					GetWindowRect(hwndTray, &rcTray);
					rcTray.bottom = rcTray.top;
					rcTray.right = rcTray.left;
#pragma FIX_LATER(tray animation <-> taskbar pos)

					KillTimer(g_hwndMain, TIMER_SETANIMATION);
					ai.iMinAnimate = 0;
					SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &ai, FALSE);
					SetTimer(g_hwndMain, TIMER_SETANIMATION, 500, NULL);

					DrawAnimatedRects(g_hwndMain, IDANI_CAPTION, &rcTray, &g_pWti[i].rcWnd);
				}
			}
		}

		//ShowWindow(g_pWti[i].hwnd, SW_SHOW);
		SetWindowPos(g_pWti[i].hwnd, NULL, 0, 0, 0, 0, 
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
		if (uCmd == SC_RESTORE || uCmd == SC_CLOSE) {
			//ShowWindow(g_pWti[i].hwnd, SW_RESTORE);
			MySwitchToThisWindow(g_pWti[i].hwnd);
			if (uCmd == SC_CLOSE)
				PostMessage(g_pWti[i].hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
		}
	}
	if (i - 1 != g_cWti) {
		MoveMemory(g_pWti + i,  g_pWti + i + 1, 
			(g_cWti - i - 1) * sizeof(WNDTRAYINFO));
	}
	g_pWti[--g_cWti].hwnd = NULL;
}


void UnflipFromTrayID(UINT uID, UINT uCmd) {  // icon identifier

	UINT i;
	for (i = 0; i < g_cWti; i++)
		if (g_pWti[i].uID == uID) break;
	if (i < g_cWti)
		_UnflipFromTray(i, uCmd);
}

void UnflipFromTray(HWND hwnd, UINT uCmd) {

	if (!hwnd)
		return;

	UINT i;
	for (i = 0; i < g_cWti; i++)
		if (g_pWti[i].hwnd == hwnd) break;
	if (i < g_cWti)
		_UnflipFromTray(i, uCmd);
}

//-----------------------------------------------------------------

void ShowFlippedSystemMenu(UINT uID) {

	UINT nWti;
	for (nWti = 0; nWti < g_cWti; nWti++)
		if (g_pWti[nWti].uID == uID) break;
	if (nWti >= g_cWti)
		return;

	if (IsWindow(g_pWti[nWti].hwnd)) {

		POINT pt;
		GetCursorPos(&pt);

		WCHAR szBuff[MAX_LANGLEN];

		HMENU hmenu = CreatePopupMenu();
		for (int i = 0; i <= (IDM_EMCLOSE - IDM_EMRESTOREALL); i++) {
			LangLoadString(IDS_EMRESTOREALL + i, szBuff, SIZEOF_ARRAY(szBuff));
			AppendMenu(hmenu, MF_STRING, IDM_EMRESTOREALL + i, szBuff);
		}
		WCHAR szCaption[MAX_LANGLEN + 32] = L"";
		LangLoadString(IDS_EMCAPTION, szBuff, SIZEOF_ARRAY(szBuff));
        wsprintf(szCaption, szBuff, g_cWti);
		szCaption[MAX_LANGLEN + 31] = L'\0';
		InsertMenu(hmenu, IDM_EMRESTOREALL, MF_STRING | MF_BYCOMMAND, IDM_EMCAPTION, szCaption);
		SetMenuDefaultItem(hmenu, IDM_EMCAPTION, MF_BYCOMMAND);
		
		EnableMenuItem(hmenu, IDM_EMCAPTION, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		InsertMenu(hmenu, IDM_EMRESTOREALL, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
		InsertMenu(hmenu, IDM_EMRESTORE, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
		SetMenuDefaultItem(hmenu, IDM_EMRESTORE, MF_BYCOMMAND);

		SetForegroundWindow(g_hwndMain);
		UINT uMenuID = TrackPopupMenu(hmenu, 
			TPM_RIGHTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
			pt.x, pt.y, 0, g_hwndMain, NULL);
		PostMessage(g_hwndMain, WM_NULL, 0, 0);

		DestroyMenu(hmenu);

		if (uMenuID >= IDM_EMRESTORE && uMenuID <= IDM_EMCLOSE) {
			_UnflipFromTray(nWti, (uMenuID == IDM_EMRESTORE) ? SC_RESTORE 
				: ((uMenuID == IDM_EMCLOSE) ? SC_CLOSE : 0));
		} else if (uMenuID >= IDM_EMRESTOREALL && uMenuID <= IDM_EMCLOSEALL) {
			UINT uCmd = (uMenuID == IDM_EMRESTOREALL) ? SC_RESTORE 
				: ((uMenuID == IDM_EMCLOSEALL) ? SC_CLOSE : 0);
            for (int i = (int)g_cWti - 1; i >= 0; i--)
				_UnflipFromTray((UINT)i, uCmd);
		}
	} else _UnflipFromTray(nWti);
}

//-----------------------------------------------------------

LRESULT CALLBACK ExtMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {

	static DWORD s_dwAction = 0;

	static int s_nRBtnBlock = 0;
	static POINT s_ptRClick = { 0, 0 };
	static DWORD s_dwRData = 0;
	static DWORD s_dwRExtraInfo = 0;

    if (nCode == HC_ACTION) {
		PMSLLHOOKSTRUCT pmh = (PMSLLHOOKSTRUCT)lParam;
        switch (wParam) {

			case WM_MOUSEWHEEL:

				if (s_nRBtnBlock) {

					s_nRBtnBlock = 2;

					BOOL fUp = ((g_dwFlagsList & TSFL_INVERSEWHEEL) 
						? ((int)pmh->mouseData < 0) : ((int)pmh->mouseData >= 0));

					if (!(GetAsyncKeyState(VK_MENU) & 0x8000))
						keybd_event(VK_LMENU, 0, 0, 0);

					if (fUp)
						keybd_event(VK_LSHIFT, 0, 0, 0);
					keybd_event(VK_TAB, 0, 0, 0);
					keybd_event(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
					if (fUp)
						keybd_event(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);

					return(1);
				}
				break;

			case WM_RBUTTONDOWN: {

				s_ptRClick.x = pmh->pt.x;
				s_ptRClick.y = pmh->pt.y;
				s_dwRData = pmh->mouseData;
				s_dwRExtraInfo = (DWORD)pmh->dwExtraInfo;

				if (s_nRBtnBlock == 0) {
					if (g_dwFlags & TSF_EXTMOUSE) {
						HWND hwnd = WindowFromPoint(pmh->pt);
						DWORD dw;
						if (SendMessageTimeout(hwnd, WM_NCHITTEST, 0, 
							MAKELPARAM((WORD)pmh->pt.x, (WORD)pmh->pt.y), 
							SMTO_ABORTIFHUNG, HUNG_TIMEOUT, &dw)) {

							if (dw == HTMINBUTTON || dw == HTMAXBUTTON) {
								s_dwAction = dw;
								return(1);
							} else {
								s_dwAction = 0;
							}
						}
					}
					if (g_dwFlags & TSF_WHEELTAB) {
						s_nRBtnBlock = 1;
						return(1);
					}
				} else if (s_nRBtnBlock == 1) {
					s_nRBtnBlock = 0;
				} else if (s_nRBtnBlock == 2) {
					s_nRBtnBlock = 0;
					keybd_event(VK_LMENU, 0, KEYEVENTF_KEYUP, 0);
				}
				break;
								 }

			case WM_RBUTTONUP: {
				if (s_nRBtnBlock == 0) {
					if (g_dwFlags & TSF_EXTMOUSE && (s_dwAction == HTMINBUTTON || s_dwAction == HTMAXBUTTON)) {
						HWND hwnd = WindowFromPoint(pmh->pt);
						DWORD dw;
						if (SendMessageTimeout(hwnd, WM_NCHITTEST, 0, 
							MAKELPARAM((WORD)pmh->pt.x, (WORD)pmh->pt.y), 
							SMTO_ABORTIFHUNG, HUNG_TIMEOUT, &dw)) {

							if (dw == HTMINBUTTON || dw == HTMAXBUTTON) {
								if (!(GetAsyncKeyState(VK_MENU) & 0x8000))
									keybd_event(VK_LMENU, 0, KEYEVENTF_KEYUP, 0);
								PostMessage(g_hwndMain, WM_EXTMOUSE, (WPARAM)dw, (LPARAM)hwnd);
								return(1);
							}
						}
					}
				} else if (s_nRBtnBlock == 1) {

					mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN, 
						s_ptRClick.x, s_ptRClick.y, s_dwRData, s_dwRExtraInfo);

					_ptRClick.x = pmh->pt.x;
					_ptRClick.y = pmh->pt.y;
					_dwRData = pmh->mouseData;
					_dwRExtraInfo = (DWORD)pmh->dwExtraInfo;
					SetTimer(g_hwndMain, TIMER_RIGHTUP, 50, NULL);

					return(1);
				} else if (s_nRBtnBlock == 2) {
					s_nRBtnBlock = 0;
					keybd_event(VK_LMENU, 0, KEYEVENTF_KEYUP, 0);
					return(1);
				}
				break;
							   }

			default:
				if ((wParam != WM_MOUSEMOVE && s_nRBtnBlock) || s_nRBtnBlock == 1) {
					mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN, 
						s_ptRClick.x, s_ptRClick.y, s_dwRData, s_dwRExtraInfo);
				}
				break;
		}
    }
    return(CallNextHookEx(g_hhookMouse, nCode, wParam, lParam));
}

//-----------------------------------------------------------------

BOOL EnableExtMouse(BOOL fEnableMin, BOOL fEnableWheel) {

	BOOL fSuccess = TRUE;

	if (fEnableMin)
		g_dwFlags |= TSF_EXTMOUSE;
	else g_dwFlags &= ~TSF_EXTMOUSE;

	if (fEnableWheel)
		g_dwFlags |= TSF_WHEELTAB;
	else g_dwFlags &= ~TSF_WHEELTAB;

	if (fEnableMin || fEnableWheel) {		
		if (g_hhookMouse)
			UnhookWindowsHookEx(g_hhookMouse);
		g_hhookMouse = SetWindowsHookEx(WH_MOUSE_LL, 
			(HOOKPROC)ExtMouseProc, g_hinstExe, NULL);
		if (g_hhookMouse)			
			fSuccess = TRUE;
	} else if (g_hhookMouse) {
		fSuccess = UnhookWindowsHookEx(g_hhookMouse);
		g_hhookMouse = NULL;
	}
	if (!g_hhookMouse)
		g_dwFlags &= ~(TSF_EXTMOUSE | TSF_WHEELTAB);

	return(fSuccess);
}

//-----------------------------------------------------------------
