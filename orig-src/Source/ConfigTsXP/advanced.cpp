// advanced.cpp

#include "stdafx.h"
#include "main.h"
#include "lang.h"
#include "ownerdrawXp.h"
#include "WinHotkeyCtrl.h"
#include "advanced.h"
#include "resource.h"

//-----------------------------------------------------------------

BOOL Adv_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_ADVANCED] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	return(FALSE);
}

//-----------------------------------------------------------------

void Adv_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	} else if (codeNotify == CBN_SELCHANGE) {
		switch (id) {
			case IDC_TRAYMENUCOMBO:
			case IDC_TRAYCONFIGCOMBO:
			case IDC_TRAYAPPCOMBO:
			case IDC_TRAYINSTCOMBO: {
				int sel = ComboBox_GetCurSel(GetDlgItem(hwnd, id));
				if (sel != 0 && sel != CB_ERR) {
					for (int idc = IDC_TRAYINSTCOMBO; idc <= IDC_TRAYMENUCOMBO; idc++) {
						if (idc != id) {
							int selIdc = ComboBox_GetCurSel(GetDlgItem(hwnd, idc));
							if (selIdc != 0 && selIdc != CB_ERR && (sel - 1) / 2 == (selIdc - 1) / 2)
								ComboBox_SetCurSel(GetDlgItem(hwnd, idc), 0);
						}
					}
				}
				break;
									}
		}
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_AdvancedProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
		HANDLE_MSG(hwnd, WM_COMMAND, Adv_OnCommand);

		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];

			for (int id = 0; id <= (IDS_CFGTASKSWITCHXP - IDS_CAPTIONSTEXT); id++) {
				LangLoadString(IDS_CAPTIONSTEXT + id, szBuff, SIZEOF_ARRAY(szBuff));
				SetDlgItemText(hwnd, IDC_CAPTIONSTEXT + id, szBuff);
			}

			HWND h1 = GetDlgItem(hwnd, IDC_TRAYMENUCOMBO);
			int sel1 = ComboBox_GetCurSel(h1);
			ComboBox_ResetContent(h1);
			HWND h2 = GetDlgItem(hwnd, IDC_TRAYCONFIGCOMBO);
			int sel2 = ComboBox_GetCurSel(h2);
			ComboBox_ResetContent(h2);
			HWND h3 = GetDlgItem(hwnd, IDC_TRAYAPPCOMBO);
			int sel3 = ComboBox_GetCurSel(h3);
			ComboBox_ResetContent(h3);
			HWND h4 = GetDlgItem(hwnd, IDC_TRAYINSTCOMBO);			
			int sel4 = ComboBox_GetCurSel(h4);
			ComboBox_ResetContent(h4);
			
			for (UINT id = IDS_TRAYACTION0; id <= IDS_TRAYACTION6; id++) {
				LangLoadString(id, szBuff, SIZEOF_ARRAY(szBuff));
				ComboBox_AddString(h1, szBuff);
				ComboBox_AddString(h2, szBuff);
				ComboBox_AddString(h3, szBuff);
				ComboBox_AddString(h4, szBuff);
			}
			ComboBox_SetCurSel(h1, sel1);
			ComboBox_SetCurSel(h2, sel2);
			ComboBox_SetCurSel(h3, sel3);
			ComboBox_SetCurSel(h4, sel4);
			break;
							 }

		HANDLE_MSG(hwnd, WM_INITDIALOG, Adv_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
