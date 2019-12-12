// preview.cpp

#include "stdafx.h"
#include "main.h"
#include "generic.h"
#include "ownerdrawXp.h"
#include "lang.h"
#include "preview.h"
#include "resource.h"

//-----------------------------------------------------------------

BOOL Pv_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_PREVIEW] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	UDACCEL pua[] = { { 0, 10 }, { 1, 50 } };

	HWND h = GetDlgItem(hwnd, IDC_PVDELAYSPIN);
	SendMessage(h, UDM_SETRANGE32, (WPARAM)MIN_PVDELAY, MAX_PVDELAY);
	SendMessage(h, UDM_SETACCEL, SIZEOF_ARRAY(pua), (LPARAM)pua);
	Edit_LimitText(GetDlgItem(hwnd, IDC_PVDELAYEDIT), 4);

	h = GetDlgItem(hwnd, IDC_PVWIDTHSPIN);
	SendMessage(h, UDM_SETRANGE32, MIN_PVWIDTH, MAX_PVWIDTH);
	SendMessage(h, UDM_SETACCEL, SIZEOF_ARRAY(pua), (LPARAM)pua);
	Edit_LimitText(GetDlgItem(hwnd, IDC_PVWIDTHEDIT), 4);

	h = GetDlgItem(hwnd, IDC_PVUPDATESPIN);
	SendMessage(h, UDM_SETRANGE32, MIN_PVUPDATE, MAX_PVUPDATE);
	SendMessage(h, UDM_SETACCEL, SIZEOF_ARRAY(pua), (LPARAM)pua);
	Edit_LimitText(GetDlgItem(hwnd, IDC_PVUPDATEEDIT), 5);

	return(FALSE);
}

//-----------------------------------------------------------------

BOOL CheckPreviewMode(HWND hwnd) {
	if (GetSystemMetrics(SM_CMONITORS) > 1 && 
		IsDlgButtonChecked(hwnd, IDC_PVDESKTOPSTYLE) == BST_CHECKED) {
		if (ConfirmMessage(hwnd, IDS_MULTIMONITOR, MB_ICONWARNING)) {
			CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVDESKTOPSTYLE, BST_UNCHECKED);
			CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVWINDOWSTYLE, BST_CHECKED);
			//EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
			return(FALSE);
		}
	}
	return(TRUE);
}

//-----------------------------------------------------------------

void Pv_OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		switch (id) {
			case IDC_PVDESKTOPSTYLE:
			case IDC_PVWINDOWSTYLE: {				
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_PVDESKTOPSTYLE) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_PVTASKBAR), fEnable);
				fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_PVWINDOWSTYLE) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_PVPOPUPONLY), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PVOLDSTYLE), fEnable);				
				break;
									 }

			case IDC_PVNOCACHE: {
				BOOL fEnable = IsDlgButtonChecked(hwnd, IDC_PVNOCACHE);
				EnableWindow(GetDlgItem(hwnd, IDC_PVLIVEUPDATE), fEnable);
				if (fEnable)
					fEnable = IsDlgButtonChecked(hwnd, IDC_PVLIVEUPDATE);
				EnableWindow(GetDlgItem(hwnd, IDC_PVUPDATEEDIT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PVUPDATESPIN), fEnable);
				break;
								}

			case IDC_PVLIVEUPDATE: {
				BOOL fEnable = IsDlgButtonChecked(hwnd, IDC_PVLIVEUPDATE);
				EnableWindow(GetDlgItem(hwnd, IDC_PVUPDATEEDIT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PVUPDATESPIN), fEnable);
				break;
								   }
		}
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	} else if (codeNotify == EN_CHANGE) {
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_PreviewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
		HANDLE_MSG(hwnd, WM_COMMAND, Pv_OnCommand);

		case WM_LANGCHANGED:
			WCHAR szBuff[MAX_LANGLEN];
			for (int id = 0; id <= (IDS_PVLIVEUPDATE - IDS_PVSTYLETEXT); id++) {
				LangLoadString(IDS_PVSTYLETEXT + id, szBuff, MAX_LANGLEN);
				SetDlgItemText(hwnd, IDC_PVSTYLETEXT + id, szBuff);
			}
			break;

		HANDLE_MSG(hwnd, WM_INITDIALOG, Pv_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
