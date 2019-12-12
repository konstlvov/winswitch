// main.cpp

#include "stdafx.h"
#include "generic.h"
#include "lang.h"
#include "ownerdrawXp.h"
#include "WinHotkeyCtrl.h"
#include "preferences.h"
#include "appearance.h"
#include "list.h"
#include "preview.h"
#include "info.h"
#include "hotkeys.h"
#include "exclusions.h"
#include "advanced.h"
#include "about.h"
#include "registry.h"
#include "resource.h"
#include "main.h"

#include <strsafe.h>

//-----------------------------------------------------------------

#define XMARGIN_PAGELIST			5
#define YMARGIN_PAGELIST			5

//-----------------------------------------------------------------

HINSTANCE g_hinstExe			= NULL;

HWND g_hwndConfig				= NULL;
HWND g_hwndMain					= NULL;
HWND g_phPages[C_PAGES]			= { NULL };

BOOL g_fIsWinXP					= FALSE;

const WCHAR g_szConfigWnd[]		= L"_As12__TaskSwitchXP_ConfigTsXP_MainWnd_";
const WCHAR g_szMainWnd[]		= L"_As12__TaskSwitchXP_MainWnd_";
const WCHAR g_szWindowName[]	= L"TaskSwitchXP Pro 2.0";
const WCHAR g_szRegKeyTs[]		= RS_TASKSWITCHXP_KEY;

HIMAGELIST _himlTc				= NULL;
int g_nComboItemHeight			= 0;

DWORD g_dwCmdLine				= CCLF_PAGEMASK;

//-----------------------------------------------------------------

LRESULT CALLBACK ConfigWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Dlg_MainProc(HWND, UINT, WPARAM, LPARAM);

//-----------------------------------------------------------------

BOOL CheckVersion() {
	OSVERSIONINFO vi = { sizeof(OSVERSIONINFO) };
	if (!GetVersionEx(&vi) || vi.dwPlatformId != VER_PLATFORM_WIN32_NT || 
		vi.dwMajorVersion < 5 || vi.dwMinorVersion < 1) {
			MessageBoxA(NULL, "This program requires features present in Windows XP/2003.", 
				"ConfigTsXP error", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
			return(FALSE);
	}
	return(TRUE);
}

//-----------------------------------------------------------------

void ParseCommandLine() {
	
	int nNumArgs;
	PWSTR *ppArgv = CommandLineToArgvW(GetCommandLineW(), &nNumArgs);

	g_dwCmdLine = CCLF_PAGEMASK;

	for (int i = 1; i < nNumArgs; i++) {
		if (ppArgv[i][0] == L'/' || ppArgv[i][0] == L'-') {
			if (!lstrcmpi(ppArgv[i] + 1, L"nocheckver")) {
				g_dwCmdLine |= CCLF_NOCHECKVER;
			} else if (!lstrcmpi(ppArgv[i] + 1, L"install")) {
				g_dwCmdLine |= CCLF_INSTALL;
			} else if (!lstrcmpi(ppArgv[i] + 1, L"defaults")) {
				g_dwCmdLine |= CCLF_DEFAULTS;
			} else if (!lstrcmpi(ppArgv[i] + 1, L"newexcl")) {
				g_dwCmdLine |= CCLF_NEWEXCL;
			} else if (!lstrcmpi(ppArgv[i] + 1, L"about")) {
				g_dwCmdLine = (g_dwCmdLine & ~CCLF_PAGEMASK) | PAGE_ABOUT;
			}
		}
	}
	HeapFree(GetProcessHeap(), 0, ppArgv);
}

//-----------------------------------------------------------------

BOOL StartTaskSwitchXP() {

	HWND hwndTs = FindWindowEx(NULL/*HWND_MESSAGE*/, NULL, g_szMainWnd, g_szWindowName);
	BOOL fSuccess = IsWindow(hwndTs);

	if (!fSuccess) {
		WCHAR szTaskSwitchXP[MAX_FILEPATH + 16];
		int n = GetModuleFileName(g_hinstExe, szTaskSwitchXP, MAX_FILEPATH);
		if (n > 0 && n < MAX_PATH) {
			while (n >= 0 && szTaskSwitchXP[n] != L'\\') n--;
			StringCchCopy(szTaskSwitchXP + n + 1, 
				SIZEOF_ARRAY(szTaskSwitchXP) - n, L"TaskSwitchXP.exe");

			STARTUPINFO si = { sizeof(STARTUPINFO) };
			PROCESS_INFORMATION pi;

			fSuccess = CreateProcess(NULL, szTaskSwitchXP, NULL, 
				NULL, FALSE, 0, NULL, NULL, &si, &pi);
			if (fSuccess) {
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			} else 
				ReportError(g_hwndMain, IDS_ERR_NOTASKSWITCH);
		} else			
			ReportError(g_hwndMain, IDS_ERR_NOTASKSWITCH); //ReportError(g_hwndMain, IDS_ERR_NOINSTDIR);
	}
	return(fSuccess);
}

//-----------------------------------------------------------------

void HelpTaskSwitchXP(int nPage) {
	WCHAR szTaskSwitchXP[MAX_FILEPATH + 32];
	int n = GetModuleFileName(g_hinstExe, szTaskSwitchXP, MAX_FILEPATH);
	if (n > 0 && n < MAX_PATH) {
		while (n >= 0 && szTaskSwitchXP[n] != L'\\') n--;		
		//switch (nPage) {
		//	default:
				StringCchCopy(szTaskSwitchXP + n + 1, 
					SIZEOF_ARRAY(szTaskSwitchXP) - n, L"TaskSwitchXP.chm::/config.html");
		//		break;
		//}
		HtmlHelp(GetDesktopWindow(), 
			szTaskSwitchXP, HH_DISPLAY_TOPIC, NULL);
	}
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

int APIENTRY wWinMain(HINSTANCE hinstExe, HINSTANCE, LPWSTR /*pszCmdLine*/, int /*nCmdShow*/) {
	
	g_hinstExe = hinstExe;

	ParseCommandLine();

	if (!(g_dwCmdLine & CCLF_NOCHECKVER)) {
		if (!CheckVersion())
			return(1);
	}

	HWND hwnd;
	if (g_dwCmdLine & (CCLF_INSTALL | CCLF_DEFAULTS)) {
		if (SilentInstall(g_dwCmdLine & CCLF_DEFAULTS)) {
			hwnd = FindWindowEx(NULL/*HWND_MESSAGE*/, NULL, g_szMainWnd, g_szWindowName);
			if (IsWindow(hwnd))
				SendMessage(hwnd, WM_RELOADSETTINGS, 0, 0);
			return(0);
		}
		return(1);
	}

	hwnd = FindWindowEx(HWND_MESSAGE, NULL, g_szConfigWnd, L"");
	if (IsWindow(hwnd)) {
		SetForegroundWindow(hwnd);
		SendMessage(hwnd, WM_REMOTECMD, g_dwCmdLine, 0);
		return(0);
	}

	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)ConfigWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= g_hinstExe;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= g_szConfigWnd;
	wcex.hIconSm		= NULL;
	if (!RegisterClassEx(&wcex))
		return(1);

	DialogBox(g_hinstExe, MAKEINTRESOURCE(IDD_MAIN), NULL, Dlg_MainProc);

	return(0);
}

//-----------------------------------------------------------------

LRESULT CALLBACK ConfigWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_REMOTECMD: {
			if (g_hwndMain) {
				/*BringWindowToTop(g_hwndMain);
				SetWindowPos(g_hwndMain, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				SetForegroundWindow(g_hwndMain);*/
				SwitchToThisWindow(g_hwndMain, TRUE);
				if (!(g_dwCmdLine & CCLF_MODALACTIVE)) {
					if (wParam & CCLF_NEWEXCL) {
						SelectPage(PAGE_EXCLUSIONS);
						PostMessage(g_phPages[PAGE_EXCLUSIONS], WM_COMMAND, 
							MAKEWPARAM(IDC_EXCLNEW, BN_CLICKED), 0);
					} else {
						int nPage = (int)(wParam & CCLF_PAGEMASK);
						if (nPage < C_PAGES)
							SelectPage(nPage);
					}
				}
			}
			break;
						   }

		case WM_CREATE:
			g_hwndConfig = hwnd;
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return(DefWindowProc(hwnd, uMsg, wParam, lParam));
	}
	return(0);
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

void SelectPage(int nPage) {

	if (nPage < 0 || nPage > C_PAGES)
		nPage = 0;

	HWND h = GetDlgItem(g_hwndMain, IDC_PAGES);

	TVITEMEX tvi;
	tvi.mask = TVIF_PARAM;
	tvi.hItem = TreeView_GetRoot(h);
	while (tvi.hItem) {
		TreeView_GetItem(h, &tvi);
		if (tvi.lParam == (LPARAM)nPage) {
			TreeView_SelectItem(h, tvi.hItem);
			break;
		}
		tvi.hItem = TreeView_GetNextSibling(h, tvi.hItem);
	}
}

//-----------------------------------------------------------------

struct FACENAME {
	WCHAR szFaceName[LF_FACESIZE];
};
typedef vector<FACENAME> VECTOR_FONTS;


int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *pelfe, NEWTEXTMETRICEX * /*pntme*/, 
							   DWORD dwFontType, LPARAM lParam) {

	if (dwFontType == TRUETYPE_FONTTYPE || dwFontType == RASTER_FONTTYPE) {
		VECTOR_FONTS *pvFonts = (VECTOR_FONTS*)lParam;
		_ASSERT(pvFonts);
		int i;
		for (i = (int)pvFonts->size() - 1; i >= 0; i--)
			if (!lstrcmpi(pvFonts->at(i).szFaceName, pelfe->elfLogFont.lfFaceName))
				break;
		if (i == -1) // :)
			pvFonts->push_back((FACENAME)(*(FACENAME*)&(pelfe->elfLogFont.lfFaceName)));
	}
	return(TRUE);
}


void InitFontControls() {
		
	LOGFONT lf;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfFaceName[0] = L'\0';
	lf.lfPitchAndFamily = 0;

	vector<FACENAME> vFonts;

	HDC hdcScreen = GetDC(NULL);
	EnumFontFamiliesEx(hdcScreen, &lf, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)&vFonts, 0);
	ReleaseDC(NULL, hdcScreen);

	HWND h1 = GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPFONTNAMECOMBO),
		h2 = GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFONTNAMECOMBO),
		h3 = GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTFONTNAMECOMBO);

	for (int i = 0; i < (int)vFonts.size(); i++) {
		ComboBox_AddString(h1, vFonts[i].szFaceName);
		ComboBox_AddString(h2, vFonts[i].szFaceName);
		ComboBox_AddString(h3, vFonts[i].szFaceName);
	}
	vFonts.clear();

	WCHAR szBuff[32];

	h1 = GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPFONTSIZECOMBO);
	h2 = GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFONTSIZECOMBO);
	h3 = GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTFONTSIZECOMBO);
	for (int i = 6; i <= 24; i++) {
		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"%d", i);
		ComboBox_AddString(h1, szBuff);
		ComboBox_AddString(h2, szBuff);
		ComboBox_AddString(h3, szBuff);
	}

	if (GetObject(GetWindowFont(g_hwndMain), sizeof(LOGFONT), (LPVOID)&lf)) {
		lf.lfWeight = FW_BOLD;
		HFONT hfont = CreateFontIndirect(&lf);
		SetWindowFont(GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPFONTBOLD), hfont, FALSE);
		SetWindowFont(GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFONTBOLD), hfont, FALSE);
		SetWindowFont(GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTFONTBOLD), hfont, FALSE);
		lf.lfWeight = FW_NORMAL;
		lf.lfItalic = TRUE;
		hfont = CreateFontIndirect(&lf);
		SetWindowFont(GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPFONTITALIC), hfont, FALSE);
		SetWindowFont(GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFONTITALIC), hfont, FALSE);
		SetWindowFont(GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTFONTITALIC), hfont, FALSE);
	}
}

//-----------------------------------------------------------------

BOOL Main_OnInitDialog(HWND hwnd, HWND /*hwndFocus*/, LPARAM /*lParam*/) {

	g_hwndMain = hwnd;

	HWND hwndConfig = CreateWindowEx(0, g_szConfigWnd, L"", 
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_DISABLED, 
		0, 0, 0, 0, HWND_MESSAGE, NULL, g_hinstExe, NULL);

	INITCOMMONCONTROLSEX iccex = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_UPDOWN_CLASS | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES
	};
	InitCommonControlsEx(&iccex);

	InitOwnerDrawCtrls(hwnd);
	InitWinHotkeyCtrls();

	HDC hdcScreen = GetWindowDC(NULL);
	g_nComboItemHeight = (10 * GetDeviceCaps(hdcScreen, LOGPIXELSY)) / 72 + 2;
	ReleaseDC(NULL, hdcScreen);

	SendMessage(hwnd, WM_SETICON, ICON_BIG, 
		(LONG)(LONG_PTR)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 
		GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, 
		(LONG)(LONG_PTR)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	if (!InitLanguage()) // load language file
		ReportError(g_hwndMain, IDS_ERR_LANGFILE);

	static DLGPROC pfnPageProcs[C_PAGES] = {
		Dlg_PreferencesProc,
		Dlg_AppearanceProc,
		Dlg_ListProc,
		Dlg_PreviewProc,
		Dlg_InfoProc,
		Dlg_HotkeysProc,
		Dlg_ExclusionsProc,
		Dlg_AdvancedProc,
		Dlg_AboutProc
	};

	for (int i = 0; i < C_PAGES; i++) {
		CreateDialog(g_hinstExe, 
			MAKEINTRESOURCE(IDD_PREFERENCESPAGE + i), hwnd, pfnPageProcs[i]);
	}

	_ASSERT(g_phPages[PAGE_PREFERENCES]);

	RECT rc, rcTab;
	GetClientRect(g_phPages[PAGE_PREFERENCES], &rc);
	
	HWND h = GetDlgItem(hwnd, IDC_PAGETAB);
	GetClientRect(h, &rcTab);
	TabCtrl_AdjustRect(h, FALSE, &rcTab);
	MapWindowRect(h, hwnd, &rcTab);

	int dx = (rc.right - rc.left) - (rcTab.right - rcTab.left),
		dy = (rc.bottom - rc.top) - (rcTab.bottom - rcTab.top);

	GetWindowRect(hwnd, &rc);
	SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left + dx, 
		rc.bottom - rc.top + dy, SWP_NOMOVE | SWP_NOZORDER);

	h = GetDlgItem(hwnd, IDC_PAGES);
	GetWindowRect(h, &rc);
	SetWindowPos(h, NULL, 0, 0, rc.right - rc.left, 
		rc.bottom - rc.top + dy, SWP_NOMOVE | SWP_NOZORDER);

	h = GetDlgItem(hwnd, IDC_PAGETITLE);
	GetWindowRect(h, &rc);
	SetWindowPos(h, NULL, 0, 0, rc.right - rc.left + dx, 
		rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

	h = GetDlgItem(hwnd, IDC_PAGETAB);
	GetWindowRect(h, &rc);
	SetWindowPos(h, NULL, 0, 0, rc.right - rc.left + dx, 
		rc.bottom - rc.top + dy, SWP_NOMOVE | SWP_NOZORDER);

	h = GetDlgItem(hwnd, IDC_OK);
	GetWindowRect(h, &rc);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
	SetWindowPos(h, NULL, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	h = GetDlgItem(hwnd, IDC_CANCEL);
	GetWindowRect(h, &rc);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
	SetWindowPos(h, NULL, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	h = GetDlgItem(hwnd, IDC_APPLY);
	GetWindowRect(h, &rc);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
	SetWindowPos(h, NULL, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	h = GetDlgItem(hwnd, IDC_TRANSLATORLINK);
	GetWindowRect(h, &rc);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
	SetWindowPos(h, NULL, rc.left, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	h = GetDlgItem(hwnd, IDC_PAGES);
	_himlTc = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 1);
	for (int i = 0; i < C_PAGES; i++) {
		HICON hIcon = (HICON)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_PREFERENCES + i), 
			IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), 
			GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		ImageList_AddIcon(_himlTc, hIcon);
		DestroyIcon(hIcon);
	}
	TreeView_SetImageList(h, _himlTc, TVSIL_NORMAL);
	TreeView_SetItemHeight(h, 26);


	TVINSERTSTRUCT tvis;
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.itemex.pszText = L"";

	for (int i = 0; i < C_PAGES; i++) {
		SetWindowPos(g_phPages[i], NULL, rcTab.left, rcTab.top, 
			0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_HIDEWINDOW);
		tvis.itemex.iImage = tvis.itemex.iSelectedImage = i;
		tvis.itemex.lParam = (LPARAM)i;
		TreeView_InsertItem(h, &tvis);
	}

	LOGFONT lf;
	if (GetObject(GetWindowFont(hwnd), sizeof(LOGFONT), (LPVOID)&lf)) {
		lf.lfWeight = FW_BOLD;
		HFONT hfont = CreateFontIndirect(&lf);
		SetWindowFont(GetDlgItem(hwnd, IDC_PAGETITLE), hfont, FALSE);		
	}

	InitFontControls();

	//SubClassUxIconButton(GetDlgItem(hwnd, IDC_PAGEHELP), IDI_HELP, 0);

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);
	LoadSettings();

	return(TRUE);
}

//-----------------------------------------------------------------

void Main_OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {

		switch (id) {

			case IDC_APPLY: {
				if (!SaveSettings(TRUE))
					ReportError(hwnd, IDS_ERR_SAVESETTINGS);
				EnableWindow(GetDlgItem(hwnd, IDC_APPLY), FALSE);
				//LoadSettings();				
				HWND hwndTs = FindWindowEx(NULL/*HWND_MESSAGE*/, NULL, g_szMainWnd, g_szWindowName);
				if (IsWindow(hwndTs))
					SendMessage(hwndTs, WM_RELOADSETTINGS, 0, 0);
				break;
							}
			case IDC_OK: {
				if (!SaveSettings())
					ReportError(hwnd, IDS_ERR_SAVESETTINGS);
				HWND hwndTs = FindWindowEx(NULL/*HWND_MESSAGE*/, NULL, g_szMainWnd, g_szWindowName);
				if (IsWindow(hwndTs))
					SendMessage(hwndTs, WM_RELOADSETTINGS, 0, 0);
				EndDialog(hwnd, TRUE);
				break;
						 }
			case IDC_CANCEL:
				EndDialog(hwnd, FALSE);
				break;

			case IDOK:
			case IDCANCEL:
				break;
		}
	}
}
//-----------------------------------------------------------------
//-----------------------------------------------------------------

void Main_OnDrawItem(const LPDRAWITEMSTRUCT pdi) {

	if (pdi->CtlID == IDC_PAGETITLE) {

		DrawMyPageTitleBk(pdi);

		RECT rc;
		rc.left = pdi->rcItem.left + 10;
		rc.top = pdi->rcItem.top;
		rc.right = pdi->rcItem.right - 10;
		rc.bottom = pdi->rcItem.bottom;

		HWND hwndTV = GetDlgItem(g_hwndMain, IDC_PAGES);
		TVITEMEX tvi;
		tvi.hItem = TreeView_GetSelection(hwndTV);
		tvi.mask = TVIF_PARAM;
		if (TreeView_GetItem(hwndTV, &tvi) && tvi.lParam < C_PAGES) {

			HICON hIcon = (HICON)LoadImage(g_hinstExe, 
				MAKEINTRESOURCE(IDI_PREFERENCES + tvi.lParam), 
				IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			DrawIconEx(pdi->hDC, rc.left, (rc.top + rc.bottom - 16) / 2, 
				hIcon, 16, 16, 0, NULL, DI_NORMAL);
			DestroyIcon(hIcon);

			rc.left += 16 + 10;

			WCHAR szText[MAX_LANGLEN] = L"";
			LangLoadString(IDS_PREFERENCESEXPAGE + (UINT)tvi.lParam, szText, MAX_LANGLEN);

			SetBkMode(pdi->hDC, TRANSPARENT);
			SetTextColor(pdi->hDC, GetSysColor(COLOR_BTNTEXT));
			DrawText(pdi->hDC, szText, -1, &rc, 
				DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
	}// else if (pdi->CtlID == IDC_PAGEHELP) {DrawUxButton(pdi);}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_DRAWITEM:
			Main_OnDrawItem((LPDRAWITEMSTRUCT)lParam);
			break;

		case WM_NOTIFY: {
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			if (pnmhdr->idFrom == IDC_PAGES) {
				switch (pnmhdr->code) {
					case TVN_SELCHANGED: {
						LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
						if (pnmtv->itemNew.state & TVIS_SELECTED) {
							int nPage = (int)pnmtv->itemNew.lParam;
							if (nPage >= 0 && nPage < C_PAGES) {
								for (int i = 0; i < C_PAGES; i++) {
									if (i != nPage) {
										ShowWindow(g_phPages[i], SW_HIDE);										
									} else {
										SetWindowPos(g_phPages[i], HWND_TOP,
											0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
									}
								}
								/*HWND hwndTV = pnmtv->hdr.hwndFrom;
								HTREEITEM hti = TreeView_GetRoot(hwndTV);
								while (hti) {
									TreeView_SetItemState(hwndTV, hti, 
										(hti == pnmtv->itemNew.hItem) ? TVIS_BOLD : 0, TVIS_BOLD);
									hti = TreeView_GetNextSibling(hwndTV, hti);
								}*/
							}
							InvalidateRect(GetDlgItem(hwnd, IDC_PAGETITLE), NULL, TRUE);
						}
						break;
										 }
				}
			} else if (pnmhdr->idFrom == IDC_TRANSLATORLINK) {
				switch (pnmhdr->code) {
					case NM_CLICK:
					case NM_RETURN: {
						PNMLINK pnmlink = reinterpret_cast<PNMLINK>(lParam);
						if (!lstrcmp(pnmlink->item.szID, L"idMyMail") ||
							!lstrcmp(pnmlink->item.szID, L"idMyHome")) {
							ShellExecute(NULL, L"open", pnmlink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
						}
						break;
									}
				}
			}
			break;
						}

		HANDLE_MSG(hwnd, WM_COMMAND, Main_OnCommand);

		case WM_HELP: {
			HWND hwndTV = GetDlgItem(g_hwndMain, IDC_PAGES);
			TVITEMEX tvi;
			tvi.hItem = TreeView_GetSelection(hwndTV);
			tvi.mask = TVIF_PARAM;
			if (TreeView_GetItem(hwndTV, &tvi)) {
				HelpTaskSwitchXP((int)tvi.lParam);
			}
			break;
					  }

		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];

			LangLoadString(IDS_CAPTION, szBuff, MAX_LANGLEN);
			SetWindowText(hwnd, szBuff);

			HWND h = GetDlgItem(hwnd, IDC_PAGES);

			TVITEMEX tvi;
			for (tvi.hItem = TreeView_GetRoot(h); tvi.hItem != NULL; 
				tvi.hItem = TreeView_GetNextSibling(h, tvi.hItem)) {
				tvi.mask = TVIF_PARAM;
				if (TreeView_GetItem(h, &tvi) && (UINT)tvi.lParam < C_PAGES) {					
					LangLoadString(IDS_PREFERENCESPAGE + (UINT)tvi.lParam, szBuff, MAX_LANGLEN);
					tvi.mask = TVIF_TEXT;
					tvi.pszText = szBuff;
					TreeView_SetItem(h, &tvi);
				}
			}

			LangLoadString(IDS_OK, szBuff, MAX_LANGLEN);
			SetDlgItemText(hwnd, IDC_OK, szBuff);
			LangLoadString(IDS_CANCEL, szBuff, MAX_LANGLEN);
			SetDlgItemText(hwnd, IDC_CANCEL, szBuff);
			LangLoadString(IDS_APPLY, szBuff, MAX_LANGLEN);
			SetDlgItemText(hwnd, IDC_APPLY, szBuff);

			InvalidateRect(GetDlgItem(hwnd, IDC_PAGETITLE), NULL, FALSE);
			break;
							 }

		case WM_TIMER:
			if (wParam == TIMER_CHECKCOLORS) {
				KillTimer(hwnd, TIMER_CHECKCOLORS);
				if (CheckColorTheme())
					CheckDefaultColors();
			}
			break;

		case WM_THEMECHANGED:
			UxThemeChanged(hwnd);
			if (CheckColorTheme())
				CheckDefaultColors();
			SetTimer(hwnd, TIMER_CHECKCOLORS, 1000, NULL);
			break;

		HANDLE_MSG(hwnd, WM_INITDIALOG, Main_OnInitDialog);

		case WM_CLOSE:
			EndDialog(hwnd, FALSE);
			break;

		case WM_DESTROY:
			DestroyWindow(g_hwndConfig);
			break;
	}
	return(0);
}

//-----------------------------------------------------------------
