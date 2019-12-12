// hotkeys.cpp

#include "stdafx.h"
#include "main.h"
#include "ownerdrawXp.h"
#include "WinHotkeyCtrl.h"
#include "lang.h"
#include "hotkeys.h"
#include "resource.h"

//-----------------------------------------------------------------

BOOL Hk_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_HOTKEYS] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	SubClassWinHotkeyCtrl(GetDlgItem(hwnd, IDC_APPLISTHK));
	SubClassWinHotkeyCtrl(GetDlgItem(hwnd, IDC_INSTLISTHK));
	SubClassWinHotkeyCtrl(GetDlgItem(hwnd, IDC_EXITHK));
	SubClassWinHotkeyCtrl(GetDlgItem(hwnd, IDC_SHOWHIDEHK));
	SubClassWinHotkeyCtrl(GetDlgItem(hwnd, IDC_CONFIGHK));

	SubClassWinHotkeyCtrl(GetDlgItem(hwnd, IDC_MINIMIZETRAYHK));
	SubClassWinHotkeyCtrl(GetDlgItem(hwnd, IDC_RESTORETRAYHK));
	
	return(FALSE);
}

//-----------------------------------------------------------------

void Hk_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	} else if (codeNotify == EN_CHANGE) {
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_HotkeysProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
		HANDLE_MSG(hwnd, WM_COMMAND, Hk_OnCommand);

		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];
			for (int id = 0; id <= (IDS_RESTORETRAYHKTEXT - IDS_REPLACEALTTAB); id++) {
				LangLoadString(IDS_REPLACEALTTAB + id, szBuff, SIZEOF_ARRAY(szBuff));
				SetDlgItemText(hwnd, IDC_REPLACEALTTAB + id, szBuff);
			}
			break;
							 }

		 HANDLE_MSG(hwnd, WM_INITDIALOG, Hk_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
