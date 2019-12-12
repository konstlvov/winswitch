// about.cpp

#include "stdafx.h"
#include "main.h"
#include "lang.h"
#include "ownerdrawXp.h"
#include "aboutconsole.h"
#include "resource.h"

//-----------------------------------------------------------------

BOOL About_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_ABOUT] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	RECT rc;
	HWND h = GetDlgItem(hwnd, IDC_ABOUTCONSOLE);
	GetWindowRect(h, &rc);
	DestroyWindow(h);
	MapWindowPoints(GetDesktopWindow(), hwnd, (LPPOINT)&rc, 2);
	CreateAboutConsole(hwnd, &rc, IDC_ABOUTCONSOLE);

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	return(FALSE);
}

//-----------------------------------------------------------------
/*
void About_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
}
*/
//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_AboutProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_NOTIFY: {
			LPNMHDR pnmhdr = reinterpret_cast<LPNMHDR>(lParam);
			if (pnmhdr->idFrom == IDC_HOMELINK) {
				switch (pnmhdr->code) {
					case NM_CLICK:
					case NM_RETURN: {
						PNMLINK pnmlink = reinterpret_cast<PNMLINK>(lParam);
						if (!lstrcmp(pnmlink->item.szID, L"idHome")) {
							ShellExecute(NULL, L"open", pnmlink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
						}
						break;
									}
				}
			} else if (pnmhdr->idFrom == IDC_TRANSLATORINFO) {
				switch (pnmhdr->code) {
					case NM_CLICK:
					case NM_RETURN: {
						PNMLINK pnmlink = reinterpret_cast<PNMLINK>(lParam);
						if (!lstrcmp(pnmlink->item.szID, L"idTranslatorMail") || !lstrcmp(pnmlink->item.szID, L"idTranslatorHome")) {
							ShellExecute(NULL, L"open", pnmlink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
						}
						break;
									}
				}
			}
			break;
						}

		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];			
			LangLoadString(IDS_TRANSLATORINFO, szBuff, MAX_LANGLEN);
			SetDlgItemText(hwnd, IDC_TRANSLATORINFO, szBuff);
			break;
							 }
		HANDLE_MSG(hwnd, WM_INITDIALOG, About_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
