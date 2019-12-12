// appearance.cpp

#include "stdafx.h"
#include "lang.h"
#include "main.h"
#include "ownerdrawXp.h"
#include "appearance.h"
#include "resource.h"

#include <strsafe.h>

//-----------------------------------------------------------------

BOOL Appr_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_APPEARANCE] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	UDACCEL pua[] = { { 0, 10 }, { 1, 50 } };
	HWND h = GetDlgItem(hwnd, IDC_FADEINSPIN);
	SendMessage(h, UDM_SETRANGE32, MIN_FADEINSPEED, MAX_FADEINSPEED);
	SendMessage(h, UDM_SETACCEL, SIZEOF_ARRAY(pua), (LPARAM)pua);
	h = GetDlgItem(hwnd, IDC_FADEOUTSPIN);
	SendMessage(h, UDM_SETRANGE32, MIN_FADEOUTSPEED, MAX_FADEOUTSPEED);
	SendMessage(h, UDM_SETACCEL, SIZEOF_ARRAY(pua), (LPARAM)pua);

	Edit_LimitText(GetDlgItem(hwnd, IDC_PVDELAYEDIT), 5);

	h = GetDlgItem(hwnd, IDC_TRANSPARENCYSLIDER);
	SendMessage(h, TBM_SETRANGE,  TRUE, MAKELONG(21, 255));
	SendMessage(h, TBM_SETTICFREQ,  13, 0);
	SendMessage(h, TBM_SETBUDDY,  FALSE, (LPARAM)GetDlgItem(hwnd, IDC_TRANSPARENCYTEXT));

	return(FALSE);
}

//-----------------------------------------------------------------

void Appr_OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		switch (id) {
			case IDC_FADEIN: {
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_FADEIN) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_FADEINEDIT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_FADEINSPIN), fEnable);
				break;
							 }
			case IDC_FADEOUT: {
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_FADEOUT) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_FADEOUTEDIT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_FADEOUTSPIN), fEnable);
				break;
							 }
			case IDC_TRANSPARENCY: {
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_TRANSPARENCY) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_TRANSPARENCYSLIDER), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_TRANSPARENCYTEXT), fEnable);
				break;
								   }
		}
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	} else if (codeNotify == CBN_SELCHANGE || codeNotify == EN_CHANGE) {
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_AppearanceProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_HSCROLL: {
			WCHAR szBuff[32];
			int nPos = (int)SendDlgItemMessage(hwnd, IDC_TRANSPARENCYSLIDER, TBM_GETPOS, 0, 0);
			nPos = (nPos - 21) * 100 / 260;
			StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"%d%%", nPos);
			SetDlgItemText(hwnd, IDC_TRANSPARENCYTEXT, szBuff);
			EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
			break;
						 }

		HANDLE_MSG(hwnd, WM_COMMAND, Appr_OnCommand);

		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];

			for (UINT id = 0; id <= (IDS_LEFTRIGHT - IDS_SCREENPOSTEXT); id++) {
				LangLoadString(IDS_SCREENPOSTEXT + id, szBuff, SIZEOF_ARRAY(szBuff));
				SetDlgItemText(hwnd, IDC_SCREENPOSTEXT + id, szBuff);
			}

			HWND h = GetDlgItem(hwnd, IDC_SCREENPOSCOMBO);
			int sel = ComboBox_GetCurSel(h);
			ComboBox_ResetContent(h);
			for (UINT id = IDS_SCREENPOS0; id <= IDS_SCREENPOS8; id++) {
				LangLoadString(id, szBuff, SIZEOF_ARRAY(szBuff));
				ComboBox_AddString(h, szBuff);
			}
			ComboBox_SetCurSel(h, sel);

			h = GetDlgItem(hwnd, IDC_VISUALMODECOMBO);
			sel = ComboBox_GetCurSel(h);
			ComboBox_ResetContent(h);
			for (UINT id = IDS_VISUALMODE0; id <= IDS_VISUALMODE5; id++) {
				LangLoadString(id, szBuff, SIZEOF_ARRAY(szBuff));
				ComboBox_AddString(h, szBuff);
			}
			ComboBox_SetCurSel(h, sel);

			h = GetDlgItem(hwnd, IDC_CLASSICMODECOMBO);
			sel = ComboBox_GetCurSel(h);
			ComboBox_ResetContent(h);
			for (UINT id = IDS_CLASSICMODE0; id <= IDS_CLASSICMODE1; id++) {
				LangLoadString(id, szBuff, SIZEOF_ARRAY(szBuff));
				ComboBox_AddString(h, szBuff);
			}
			ComboBox_SetCurSel(h, sel);
			break;
							 }

		HANDLE_MSG(hwnd, WM_INITDIALOG, Appr_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
