// preferences.cpp

#include "stdafx.h"
#include "lang.h"
#include "main.h"
#include "ownerdrawXp.h"
#include "registry.h"
#include "generic.h"
#include "preferences.h"
#include "resource.h"

#include <strsafe.h>


extern stringW _sLangFile;

//-----------------------------------------------------------------

BOOL Pref_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_PREFERENCES] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	WCHAR szBuff[MAX(MAX_FILEPATH + 16, MAX_LANGLEN)];
	HWND h = GetDlgItem(hwnd, IDC_LANGUAGECOMBO);    

	// find .\lang\*.lng
	int n = GetModuleFileName(g_hinstExe, szBuff, MAX_FILEPATH);
	if (n > 0 && n < MAX_FILEPATH) {

		while (n >= 0 && szBuff[n] != L'\\') n--;
		StringCchCopy(szBuff + n + 1, 
			SIZEOF_ARRAY(szBuff) - n, L"lang\\*.lng");

		ComboBox_ResetContent(h);

		WIN32_FIND_DATA wfd;
		HANDLE hFindFile = FindFirstFile(szBuff, &wfd);
		if (hFindFile != INVALID_HANDLE_VALUE) {
			do {
				wfd.cFileName[lstrlen(wfd.cFileName) - 4] = L'\0';
				n = ComboBox_AddString(h, wfd.cFileName);
				ComboBox_SetItemData(h, n, TRUE);
			} while (FindNextFile(hFindFile, &wfd));
			FindClose(hFindFile);
		}
	}

	LoadString(g_hinstExe, IDS_LANGNAME, szBuff, MAX_LANGLEN);
	n = ComboBox_AddString(h, szBuff);
	ComboBox_SetItemData(h, n, FALSE);

	if (!IsLangDefault()) {
		int find = ComboBox_FindStringExact(h, -1, _sLangFile.c_str());
		if (find != CB_ERR)
			n = find;
	}
	ComboBox_SetCurSel(h, n);

	return(FALSE);
}

//-----------------------------------------------------------------

BOOL _SetCustomIcon(HWND hwndPref, LPCWSTR szFileName, int nIconIndex) {

	WCHAR szBuff[MAX_DATALEN];
	ExpandEnvironmentStrings(szFileName, szBuff, SIZEOF_ARRAY(szBuff));

	HICON hIconSm = NULL;
	if (!ExtractIconEx(szBuff, nIconIndex, NULL, &hIconSm, 1) || !hIconSm) {
        hIconSm = (HICON)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_TASKSWITCHXP16), IMAGE_ICON, 
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	}

	hIconSm = Static_SetIcon(GetDlgItem(hwndPref, IDC_CUSTOMICON), hIconSm);
	if (hIconSm)
		DestroyIcon(hIconSm);

	WCHAR szIconIndex[32];
	StringCchPrintf(szIconIndex, SIZEOF_ARRAY(szIconIndex), L",%d", nIconIndex);
	StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), szIconIndex);
	SetDlgItemText(hwndPref, IDC_CUSTOMICONEDIT, szBuff);

	return(TRUE);
}

//-----------------------------------------------------------------

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

//-----------------------------------------------------------------

BOOL SetCustomIcon(HWND hwndPref, LPCWSTR szIconFile) {

	_ASSERT(szIconFile);

	int nIconIndex;
	WCHAR szBuff[MAX_DATALEN];
	StringCchCopy(szBuff, SIZEOF_ARRAY(szBuff), szIconFile);
	if (!_GetIconPathIndex(szBuff, &nIconIndex)) {
		HICON hIconSm = (HICON)LoadImage(g_hinstExe, MAKEINTRESOURCE(IDI_TASKSWITCHXP16), IMAGE_ICON, 
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		hIconSm = Static_SetIcon(GetDlgItem(hwndPref, IDC_CUSTOMICON), hIconSm);
		if (hIconSm)
			DestroyIcon(hIconSm);
		SetDlgItemText(hwndPref, IDC_CUSTOMICONEDIT, L"");
		return(FALSE);
	}
	return(_SetCustomIcon(hwndPref, szBuff, nIconIndex));
}

//-----------------------------------------------------------------

void Pref_OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		switch (id) {

			case IDC_USECUSTOMICON: {
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_USECUSTOMICON) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_CUSTOMICONEDIT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CUSTOMICONBROWSE), fEnable);
				//EnableWindow(GetDlgItem(hwnd, IDC_CUSTOMICON), fEnable);
				ShowWindow(GetDlgItem(hwnd, IDC_CUSTOMICON), fEnable ? SW_SHOW : SW_HIDE);
				EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
				break;
									}

			case IDC_CUSTOMICONBROWSE: {
				WCHAR szIconFile[MAX_DATALEN];
				GetDlgItemText(hwnd, IDC_CUSTOMICONEDIT, szIconFile, SIZEOF_ARRAY(szIconFile));
				int nIconIndex = 0;
				if (!_GetIconPathIndex(szIconFile, &nIconIndex))
					szIconFile[0] = L'\0';
				if (PickIconDlg(hwnd, szIconFile, SIZEOF_ARRAY(szIconFile), &nIconIndex)) {
					_SetCustomIcon(hwnd, szIconFile, nIconIndex);
					EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
				}
				break;
									   }

			case IDC_EXPORTSETTINGS:
			case IDC_IMPORTSETTINGS: {

				OPENFILENAME ofn = { sizeof(OPENFILENAME) };

				ofn.hwndOwner = hwnd;
				ofn.lpstrInitialDir = NULL;
				
				WCHAR szFilter[MAX_LANGLEN+32];
				LangLoadString(IDS_CFGFILTER, szFilter, MAX_LANGLEN);
				int len = lstrlen(szFilter);
				szFilter[len + 1] = L'*'; szFilter[len + 2] = L'.'; szFilter[len + 3] = L'c';
				szFilter[len + 4] = L'f'; szFilter[len + 5] = L'g'; 
				szFilter[len + 6] = L'\0'; szFilter[len + 7] = L'\0';
				ofn.lpstrFilter = szFilter;
				ofn.nFilterIndex = 1;

				WCHAR szBuff[MAX_FILEPATH+4] = L"TaskSwitchXP.cfg";
				ofn.lpstrFile = szBuff;
				ofn.nMaxFile = MAX_FILEPATH;

				g_dwCmdLine |= CCLF_MODALACTIVE;
				if (id == IDC_IMPORTSETTINGS) {
					ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
					ofn.lpstrFile[0] = L'\0';
					if (GetOpenFileName(&ofn)) {
						int nRet = ImportSettings(szBuff);
						if (nRet == -2)
							ReportError(g_hwndMain, IDS_ERR_DEFIMPORT);
						else if (nRet == -3)
							ReportError(g_hwndMain, IDS_ERR_IMPORT);
						EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
					}
				} else {
					ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
					if (GetSaveFileName(&ofn)) {						
						int len = lstrlen(szBuff);
						if (len <= 4 || lstrcmpi(szBuff + len - 4, L".cfg"))
							StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), L".cfg");
						if (!ExportSettings(szBuff))
							ReportError(g_hwndMain, IDS_ERR_EXPORT);
						//if (GetCurrentDirectory(MAX_DATALEN, szBuff))
						//	_sSettingsDir = szBuff;
					}
				}
				g_dwCmdLine &= ~CCLF_MODALACTIVE;
				break;
									 }

			case IDC_RESTOREDEFAULTS:
				LoadSettings(TRUE);
				EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
				break;

			default:
				EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
				break;
		}
	} else if (codeNotify == CBN_SELCHANGE) {
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_PreferencesProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		HANDLE_MSG(hwnd, WM_COMMAND, Pref_OnCommand);

		case WM_NOTIFY: {
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			if (pnmhdr->idFrom == IDC_LANGLINK) {
				switch (pnmhdr->code) {
					case NM_CLICK:
					case NM_RETURN: {
						PNMLINK pnmlink = reinterpret_cast<PNMLINK>(lParam);
						if (!lstrcmp(pnmlink->item.szID, L"idLang")) {
							ShellExecute(NULL, L"open", 
								L"http://www.ntwind.com/forum/viewforum.php?f=3", NULL, NULL, SW_SHOWNORMAL);
						}
						break;
									}
				}
			}
			break;
						}

		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];
			for (int id = 0; id <= (IDS_ENUMTRAY - IDS_LANGUAGETEXT); id++) {
				LangLoadString(IDS_LANGUAGETEXT + id, szBuff, MAX_LANGLEN);
				SetDlgItemText(hwnd, IDC_LANGUAGETEXT + id, szBuff);
			}
			for (int id = 0; id <= (IDS_EXPORTSETTINGS - IDS_RESTOREDEFAULTS); id++) {
				LangLoadString(IDS_RESTOREDEFAULTS + id, szBuff, MAX_LANGLEN);
				SetDlgItemText(hwnd, IDC_RESTOREDEFAULTS + id, szBuff);
			}
			break;
							 }
		HANDLE_MSG(hwnd, WM_INITDIALOG, Pref_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
