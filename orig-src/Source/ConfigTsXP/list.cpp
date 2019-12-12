// list.cpp

#include "stdafx.h"
#include "main.h"
#include "ownerdrawXp.h"
#include "lang.h"
#include "registry.h"
#include "list.h"
#include "resource.h"

//-----------------------------------------------------------------

COLORREF g_pcrList[7] = { 0, 0, 0, 0, 0, 0, 0 };

//-----------------------------------------------------------------

BOOL List_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_LIST] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	SendMessage(GetDlgItem(hwnd, IDC_ICONSXSPIN), 
		UDM_SETRANGE32, (WPARAM)MIN_ICONSX, MAX_ICONSX);
	Edit_LimitText(GetDlgItem(hwnd, IDC_ICONSXEDIT), 5);

	UDACCEL pua[] = { { 0, 10 }, { 1, 50 } };
	HWND h = GetDlgItem(hwnd, IDC_LISTWIDTHSPIN);
	SendMessage(h, UDM_SETRANGE32, (WPARAM)MIN_LISTWIDTH, MAX_LISTWIDTH);
	SendMessage(h, UDM_SETACCEL, SIZEOF_ARRAY(pua), (LPARAM)pua);
	Edit_LimitText(GetDlgItem(hwnd, IDC_LISTWIDTHEDIT), 5);

	SendMessage(GetDlgItem(hwnd, IDC_ICONSYSPIN), 
		UDM_SETRANGE32, (WPARAM)MIN_ICONSY, MAX_ICONSY);
	Edit_LimitText(GetDlgItem(hwnd, IDC_ICONSYEDIT), 5);

	SubClassUxColorButton(GetDlgItem(hwnd, IDC_LCOLOR), 0);

	return(FALSE);
}

//-----------------------------------------------------------------

void List_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		switch (id) {

			case IDC_SMALLICONS: {
				BOOL fError, fSmall = (BOOL)(IsDlgButtonChecked(hwnd, IDC_SMALLICONS) == BST_CHECKED);
				//BOOL fVertical = (BOOL)(ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LISTSTYLECOMBO)) == 1);
                
				int n = (int)SendMessage(GetDlgItem(hwnd, IDC_ICONSXSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
				if (fError) n = DEFAULT_ICONSX;
                if (fSmall) n *= 2;
				else if (!fError)
					n = n / 2 + n % 2;
				SendMessage(GetDlgItem(hwnd, IDC_ICONSXSPIN), UDM_SETPOS32, 0, n);

				n = (int)SendMessage(GetDlgItem(hwnd, IDC_ICONSYSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
				if (fError) n = DEFAULT_ICONSY;
                if (fSmall) n *= 2;
				else if (!fError)
					n = n / 2 + n % 2;
				SendMessage(GetDlgItem(hwnd, IDC_ICONSYSPIN), UDM_SETPOS32, 0, n);
				break;
								 }
			case IDC_LCOLOR:
				if (UxButtonChooseColor(hwnd, hwndCtl)) {
					int sel = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LCOLORCOMBO));
					if (sel >= 0 && sel < SIZEOF_ARRAY(g_pcrList)) {
						COLORREF cr = GetUxButtonColor(hwndCtl);
						if (cr != g_pcrList[sel]) {
							g_dwFlagsEx &= ~TSFEX_DEFLISTCOLORS;
							g_pcrList[sel] = cr;
						}
					}
				}
				break;
		}
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	} else if (codeNotify == CBN_SELCHANGE) {
		switch (id) {
			case IDC_LISTSTYLECOMBO: {
				int sel = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LISTSTYLECOMBO));
				int nCmdShow = (sel == 0) ? SW_SHOW : SW_HIDE;
				ShowWindow(GetDlgItem(hwnd, IDC_ICONSXTEXT), nCmdShow);
				ShowWindow(GetDlgItem(hwnd, IDC_ICONSXEDIT), nCmdShow);
				ShowWindow(GetDlgItem(hwnd, IDC_ICONSXSPIN), nCmdShow);
				nCmdShow = (sel == 1) ? SW_SHOW : SW_HIDE;
				ShowWindow(GetDlgItem(hwnd, IDC_LISTWIDTHTEXT), nCmdShow);
				ShowWindow(GetDlgItem(hwnd, IDC_LISTWIDTHEDIT), nCmdShow);
				ShowWindow(GetDlgItem(hwnd, IDC_LISTWIDTHSPIN), nCmdShow);
				break;
									 }
			case IDC_LCOLORCOMBO: {
				int sel = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LCOLORCOMBO));
				if (sel >= 0 && sel < SIZEOF_ARRAY(g_pcrList)) {
					g_dwCmdLine |= CCLF_MODALACTIVE;
					SetUxButtonColor(GetDlgItem(hwnd, IDC_LCOLOR), g_pcrList[sel]);
					g_dwCmdLine &= ~CCLF_MODALACTIVE;
				}
				break;
								  }
		}
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	} else if (codeNotify == EN_CHANGE) {
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	}
}

//-----------------------------------------------------------------

void List_OnDrawItem(const LPDRAWITEMSTRUCT pdi) {

	if (pdi->CtlID == IDC_LISTFONTNAMECOMBO) {
		DrawFontCombo(pdi);
	} else if (pdi->CtlID == IDC_LCOLOR) {
		DrawUxButton(pdi);
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_ListProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_DRAWITEM:
			List_OnDrawItem((LPDRAWITEMSTRUCT)lParam);
			break;

		case WM_MEASUREITEM: {
			LPMEASUREITEMSTRUCT pmi = (LPMEASUREITEMSTRUCT)lParam;
			if (pmi->CtlID == IDC_LISTFONTNAMECOMBO)
				pmi->itemHeight = g_nComboItemHeight;
			break;
								}

		HANDLE_MSG(hwnd, WM_COMMAND, List_OnCommand);

		
		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];

			for (UINT id = 0; id <= (IDS_LCOLORTEXT - IDS_LISTSTYLETEXT); id++) {
				LangLoadString(IDS_LISTSTYLETEXT + id, szBuff, SIZEOF_ARRAY(szBuff));
				SetDlgItemText(hwnd, IDC_LISTSTYLETEXT + id, szBuff);
			}

			HWND h = GetDlgItem(hwnd, IDC_LISTSTYLECOMBO);
			int sel = ComboBox_GetCurSel(h);
			ComboBox_ResetContent(h);
			for (UINT id = IDS_LISTSTYLE0; id <= IDS_LISTSTYLE1; id++) {
				LangLoadString(id, szBuff, SIZEOF_ARRAY(szBuff));
				ComboBox_AddString(h, szBuff);
			}
			ComboBox_SetCurSel(h, sel);

			h = GetDlgItem(hwnd, IDC_SELSTYLECOMBO);
			sel = ComboBox_GetCurSel(h);
			ComboBox_ResetContent(h);
			for (UINT id = IDS_SELSTYLE0; id <= IDS_SELSTYLE2; id++) {
				LangLoadString(id, szBuff, SIZEOF_ARRAY(szBuff));
				ComboBox_AddString(h, szBuff);
			}
			ComboBox_SetCurSel(h, sel);

			h = GetDlgItem(hwnd, IDC_LCOLORCOMBO);
			sel = ComboBox_GetCurSel(h);
			ComboBox_ResetContent(h);
			for (UINT id = IDS_LCOLOR0; id <= IDS_LCOLOR6; id++) {
				LangLoadString(id, szBuff, SIZEOF_ARRAY(szBuff));
				ComboBox_AddString(h, szBuff);
			}
			ComboBox_SetCurSel(h, sel);
			break;
							 }

		HANDLE_MSG(hwnd, WM_INITDIALOG, List_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
