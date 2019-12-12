// info.cpp

#include "stdafx.h"
#include "main.h"
#include "ownerdrawXp.h"
#include "lang.h"
#include "registry.h"
#include "info.h"
#include "resource.h"

//-----------------------------------------------------------------

BOOL Info_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_INFO] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	HWND h = GetDlgItem(hwnd, IDC_CAPSHADOWDEEPSPIN);
	SendMessage(h, UDM_SETRANGE32, (WPARAM)-16, 16);
	Edit_LimitText(GetDlgItem(hwnd, IDC_CAPSHADOWDEEPEDIT), 3);

	h = GetDlgItem(hwnd, IDC_PANESHADOWDEEPSPIN);
	SendMessage(h, UDM_SETRANGE32, (WPARAM)-16, 16);
	Edit_LimitText(GetDlgItem(hwnd, IDC_PANESHADOWDEEPEDIT), 3);

	h = GetDlgItem(hwnd, IDC_PANEHEIGHTSPIN);
	SendMessage(h, UDM_SETRANGE32, MIN_PANEHEIGHT, MAX_PANEHEIGHT);
	Edit_LimitText(GetDlgItem(hwnd, IDC_PANEHEIGHTEDIT), 3);

	SubClassUxColorButton(GetDlgItem(hwnd, IDC_CAPTEXTCOLOR), RGB(255,255,255));
	SubClassUxColorButton(GetDlgItem(hwnd, IDC_CAPSHADOWCOLOR), RGB(0,0,0));
	SubClassUxColorButton(GetDlgItem(hwnd, IDC_PANETEXTCOLOR), RGB(255,255,255));
	SubClassUxColorButton(GetDlgItem(hwnd, IDC_PANESHADOWCOLOR), RGB(0,0,0));

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	return(FALSE);
}

//-----------------------------------------------------------------

void Info_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		switch (id) {

			/*case IDC_SHOWCAPTION: {
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_SHOWCAPTION) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPFONTNAMETEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPFONTNAMECOMBO), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPFONTSIZETEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPFONTSIZECOMBO), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPFONTBOLD), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPFONTITALIC), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPTEXTCOLORTEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPTEXTCOLOR), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPSHOWICON), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPSMALLICON), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPDRAWSHADOW), fEnable);
				Info_OnCommand(hwnd, IDC_CAPDRAWSHADOW, NULL, BN_CLICKED);
				break;
							   }
			case IDC_SHOWINFOPANE: {
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_SHOWINFOPANE) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEHEIGHTEDIT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEHEIGHTSPIN), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEHEIGHTTEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEFONTNAMETEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEFONTNAMECOMBO), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEFONTSIZETEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEFONTSIZECOMBO), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEFONTBOLD), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANEFONTITALIC), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANETEXTCOLOR), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANETEXTCOLORTEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PROCESSINFOCOMBO), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_SHOWTASKNUMBER), fEnable);
				break;
								 }*/
			case IDC_CAPDRAWSHADOW: {
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_CAPDRAWSHADOW) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPSHADOWDEEPTEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPSHADOWDEEPEDIT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPSHADOWDEEPSPIN), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPSHADOWCOLORTEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_CAPSHADOWCOLOR), fEnable);
				break;
									}
			case IDC_PANEDRAWSHADOW: {
				BOOL fEnable = (BOOL)(IsDlgButtonChecked(hwnd, IDC_PANEDRAWSHADOW) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDC_PANESHADOWDEEPTEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANESHADOWDEEPEDIT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANESHADOWDEEPSPIN), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANESHADOWCOLORTEXT), fEnable);
				EnableWindow(GetDlgItem(hwnd, IDC_PANESHADOWCOLOR), fEnable);
				break;
									}

			case IDC_CAPTEXTCOLOR:
			case IDC_CAPSHADOWCOLOR:
			case IDC_PANETEXTCOLOR:
			case IDC_PANESHADOWCOLOR: {
				COLORREF crOld = GetUxButtonColor(hwndCtl);
				g_dwCmdLine |= CCLF_MODALACTIVE;
				UxButtonChooseColor(hwnd, hwndCtl);
				g_dwCmdLine &= ~CCLF_MODALACTIVE;
				if (crOld != GetUxButtonColor(hwndCtl)) {
					if (id == IDC_CAPTEXTCOLOR || id == IDC_CAPSHADOWCOLOR)
						g_dwFlagsEx &= ~TSFEX_DEFCAPCOLORS;
					else g_dwFlagsEx &= ~TSFEX_DEFPANECOLORS;
				}
				break;
									 }
		}
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	} else if (codeNotify == CBN_SELCHANGE || 
		codeNotify == CBN_EDITCHANGE || codeNotify == EN_CHANGE) {
		EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
	}
}

//-----------------------------------------------------------------

void Info_OnDrawItem(const LPDRAWITEMSTRUCT pdi) {

	if (pdi->CtlID == IDC_CAPFONTNAMECOMBO || pdi->CtlID == IDC_PANEFONTNAMECOMBO) {
		DrawFontCombo(pdi);
	} else if (pdi->CtlID == IDC_CAPTEXTCOLOR || pdi->CtlID == IDC_CAPSHADOWCOLOR || 
		pdi->CtlID == IDC_PANETEXTCOLOR || pdi->CtlID == IDC_PANESHADOWCOLOR) {
		DrawUxButton(pdi);
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_InfoProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_DRAWITEM: {
			Info_OnDrawItem((LPDRAWITEMSTRUCT)lParam);
			break;
						  }
		case WM_MEASUREITEM: {
			LPMEASUREITEMSTRUCT pmi = (LPMEASUREITEMSTRUCT)lParam;
			if (pmi->CtlID == IDC_CAPFONTNAMECOMBO || pmi->CtlID == IDC_PANEFONTNAMECOMBO)
				pmi->itemHeight = g_nComboItemHeight;
			break;
								}
		HANDLE_MSG(hwnd, WM_COMMAND, Info_OnCommand);
		
		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];

			for (int id = 0; id <= (IDS_PANEFORMATTEXT - IDS_CAPPANETEXT); id++) {
				LangLoadString(IDS_CAPPANETEXT + id, szBuff, SIZEOF_ARRAY(szBuff));
				SetDlgItemText(hwnd, IDC_CAPPANETEXT + id, szBuff);
			}

			HWND h = GetDlgItem(hwnd, IDC_PANEFORMATCOMBO);
			int sel = ComboBox_GetCurSel(h);
			ComboBox_ResetContent(h);
			for (UINT id = IDS_PANEFORMAT0; id <= IDS_PANEFORMAT5; id++) {
				LangLoadString(id, szBuff, SIZEOF_ARRAY(szBuff));
				ComboBox_AddString(h, szBuff);
			}
			ComboBox_SetCurSel(h, sel);
			break;
							 }

		HANDLE_MSG(hwnd, WM_INITDIALOG, Info_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
