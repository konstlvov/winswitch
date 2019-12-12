// exclusions.cpp

#include "stdafx.h"
#include "main.h"
#include "ownerdrawXp.h"
#include "lang.h"
#include "excldlg.h"
#include "exclusions.h"
#include "resource.h"

#include <strsafe.h>

//-----------------------------------------------------------------

vector<TSEXCLUSION> _vExclList;

//-----------------------------------------------------------------

BOOL Excl_OnInitDialog(HWND hwnd, HWND, LPARAM) {

	g_phPages[PAGE_EXCLUSIONS] = hwnd;

	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	HWND h = GetDlgItem(hwnd, IDC_EXCLLIST);
	ListView_SetExtendedListViewStyle(h, 
		LVS_EX_CHECKBOXES | LVS_EX_ONECLICKACTIVATE | LVS_EX_UNDERLINEHOT | \
		LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	RECT rc;
	int nScroll = GetSystemMetrics(SM_CXHSCROLL);
	GetClientRect(h, &rc);

	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvc.pszText = L"";
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 160;
	lvc.iSubItem = 0;
	ListView_InsertColumn(h, 0, &lvc);

	lvc.cx = rc.right - rc.left - 160 - nScroll - 2;
	lvc.iSubItem = 1;
	ListView_InsertColumn(h, 1, &lvc);

	_vExclList.clear();

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);

	return(FALSE);
}

//-----------------------------------------------------------------

BOOL UpdateExclItem(HWND hwndLV, int nItem) {

	_ASSERT(nItem >= 0 && nItem < ListView_GetItemCount(hwndLV));
	_ASSERT(ListView_GetItemCount(hwndLV) == (int)_vExclList.size());

	TSEXCLUSION& tseItem = _vExclList[nItem];

	ListView_SetCheckState(hwndLV, nItem, !(tseItem.dwFlags & TSEXCLF_DISABLED));

	LVITEM lvi;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.mask = LVIF_TEXT | LVIF_GROUPID;
	lvi.pszText = (LPWSTR)tseItem.szExclName;
	lvi.iGroupId = (tseItem.dwFlags & TSEXCLF_PREVIEW) ? 0 : 1;
	ListView_SetItem(hwndLV, &lvi);

	lvi.mask = LVIF_TEXT;

	WCHAR szBuff[MAX_FILENAME + MAX_CLASSNAME + MAX_CAPTION + 20] = L"";
	if (tseItem.dwFlags & TSEXCLF_BYPROCESS)
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), tseItem.szExeName);
	if (tseItem.dwFlags & TSEXCLF_BYCLASS) {
		if (szBuff[0] != L'\0')
			StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), L", ");
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), tseItem.szClassName);
	}
	if (tseItem.dwFlags & TSEXCLF_BYCAPTION) {
		if (szBuff[0] != L'\0')
			StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), L", ");
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), L"\"");
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), tseItem.szCaption);
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), L"\"");
	}
	lvi.pszText = szBuff;
	lvi.iSubItem = 1;
	ListView_SetItem(hwndLV, &lvi);

	//ListView_MoveItemToGroup(hwndLV, nItem, 
	//	(tseItem.dwFlags & TSEXCLF_PREVIEW) ? 0 : 1);

	return(TRUE);
}


int AddExclItem(HWND hwndLV, const PTSEXCLUSION ptse) {

	_ASSERT(hwndLV && ptse);

	int nItem = (int)_vExclList.size();
	_ASSERT(nItem == ListView_GetItemCount(hwndLV));

	for (vector<TSEXCLUSION>::iterator it = _vExclList.begin(); it != _vExclList.end(); it++) {
		if ((it->dwFlags & TSEXCLF_MASK) == (ptse->dwFlags & TSEXCLF_MASK)) {
			if (!(ptse->dwFlags & TSEXCLF_BYPROCESS) || 
				!lstrcmp(it->szExeName, ptse->szExeName)) {
				if (!(ptse->dwFlags & TSEXCLF_BYCLASS) || 
					!lstrcmp(it->szClassName, ptse->szClassName)) {
					if (!(ptse->dwFlags & TSEXCLF_BYCAPTION) || 
						!lstrcmp(it->szCaption, ptse->szCaption)) {
						return(-1);
					}
				}
			}
		}
	}

	_vExclList.push_back(*ptse);

	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.pszText = L"";
	ListView_InsertItem(hwndLV, &lvi);

	ListView_SetCheckState(hwndLV, nItem, !(ptse->dwFlags & TSEXCLF_DISABLED));
	_vExclList[nItem].dwFlags = ptse->dwFlags;

	UpdateExclItem(hwndLV, nItem);

	return(nItem);
}


void EmptyExclList(HWND hwndLV) {
	_vExclList.clear();
	ListView_DeleteAllItems(hwndLV);
}

//-----------------------------------------------------------------

void EditExclItem(HWND hwndLV, int sel) {

	if (sel >= 0 && sel < (int)_vExclList.size()) {

		TSEXCLUSION tse;
		CopyMemory(&tse, (PVOID)&_vExclList[sel], sizeof(TSEXCLUSION));
		tse.dwFlags &= ~TSEXCLF_NEW;

		/*if (ListView_GetCheckState(hwndLV, sel))
		tse.dwFlags &= ~TSEXCLF_DISABLED;
		else tse.dwFlags |= TSEXCLF_DISABLED;*/

		g_dwCmdLine |= CCLF_MODALACTIVE;
		if (DialogBoxParam(g_hinstExe, MAKEINTRESOURCE(IDD_EXCLUDEWND), 
			g_hwndMain, Dlg_ExcludeWindowProc, (LPARAM)&tse) == IDOK) {

			CopyMemory((PVOID)&_vExclList[sel], &tse, sizeof(TSEXCLUSION));
			UpdateExclItem(hwndLV, sel);
			ListView_SetItemState(hwndLV, sel, LVIS_SELECTED, LVIS_SELECTED);
			ListView_EnsureVisible(hwndLV, sel, FALSE);
			EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
		}
		g_dwCmdLine &= ~CCLF_MODALACTIVE;
	}
}

//-----------------------------------------------------------------

void Excl_OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		switch (id) {

			case IDC_EXCLNEW: {
				TSEXCLUSION tse;
				ZeroMemory(&tse, sizeof(TSEXCLUSION));
				tse.dwFlags = TSEXCLF_NEW;
				g_dwCmdLine |= CCLF_MODALACTIVE;
				if (DialogBoxParam(g_hinstExe, MAKEINTRESOURCE(IDD_EXCLUDEWND), 
					g_hwndMain, Dlg_ExcludeWindowProc, (LPARAM)&tse) == IDOK) {
					HWND hwndLV = GetDlgItem(hwnd, IDC_EXCLLIST);
					int nItem = AddExclItem(hwndLV, &tse);
					ListView_SetItemState(hwndLV, nItem, LVIS_SELECTED, LVIS_SELECTED);
					ListView_EnsureVisible(hwndLV, nItem, FALSE);
					EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
				}
				g_dwCmdLine &= ~CCLF_MODALACTIVE;
				break;
							  }

			case IDC_EXCLEDIT: {

				HWND hwndLV = GetDlgItem(hwnd, IDC_EXCLLIST);
				int sel = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
				EditExclItem(hwndLV, sel);
				break;
							   }

			case IDC_EXCLREMOVE: {
				HWND hwndLV = GetDlgItem(hwnd, IDC_EXCLLIST);
				int sel = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
				if (sel >= 0 && sel < (int)_vExclList.size()) {
					_vExclList.erase(_vExclList.begin() + sel);
					ListView_DeleteItem(hwndLV, sel);
					sel = MIN(ListView_GetItemCount(hwndLV) - 1, sel);
					ListView_SetItemState(hwndLV, sel, LVIS_SELECTED, LVIS_SELECTED);
					ListView_EnsureVisible(hwndLV, sel, FALSE);
					EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
				}
				break;
								 }
		}		
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_ExclusionsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		HANDLE_MSG(hwnd, WM_COMMAND, Excl_OnCommand);

		case WM_NOTIFY: {
			NMHDR* pnmhdr = (NMHDR*)lParam;
			if (pnmhdr->idFrom == IDC_EXCLLIST) {				
				switch (pnmhdr->code) {

					case LVN_ITEMCHANGED: {
						LPNMLISTVIEW pnmlv = reinterpret_cast<LPNMLISTVIEW>(pnmhdr);
						if (pnmlv->uNewState & LVIS_SELECTED) {
							EnableWindow(GetDlgItem(hwnd, IDC_EXCLEDIT), TRUE);
							EnableWindow(GetDlgItem(hwnd, IDC_EXCLREMOVE), TRUE);
						} else if (pnmlv->uOldState & LVIS_SELECTED) {
							EnableWindow(GetDlgItem(hwnd, IDC_EXCLEDIT), FALSE);
							EnableWindow(GetDlgItem(hwnd, IDC_EXCLREMOVE), FALSE);
						} else if (pnmlv->iItem >= 0 && pnmlv->iItem < (int)_vExclList.size()) {
							BOOL fEnabled = ListView_GetCheckState(pnmhdr->hwndFrom, pnmlv->iItem);
							TSEXCLUSION& tse = _vExclList[pnmlv->iItem];
							if ((fEnabled && tse.dwFlags & TSEXCLF_DISABLED) ||
								(!fEnabled && !(tse.dwFlags & TSEXCLF_DISABLED))) {
								if (fEnabled) tse.dwFlags &= ~TSEXCLF_DISABLED;
								else tse.dwFlags |= TSEXCLF_DISABLED;
								EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), TRUE);
							}
						}
						break;
										  }

					case LVN_KEYDOWN: {
						LPNMLVKEYDOWN pnmkd = reinterpret_cast<LPNMLVKEYDOWN>(pnmhdr);
						if (pnmkd->wVKey == VK_DELETE) {
							Excl_OnCommand(hwnd, IDC_EXCLREMOVE, NULL, BN_CLICKED);
						}
						break;
									  }

					case NM_RETURN: {
						Excl_OnCommand(hwnd, IDC_EXCLEDIT, NULL, BN_CLICKED);
						break;
									}

					case NM_DBLCLK: {
						LPNMITEMACTIVATE pnmia = reinterpret_cast<LPNMITEMACTIVATE>(pnmhdr);
						EditExclItem(pnmhdr->hwndFrom, pnmia->iItem);
						break;
									}
				}
			}
			break;
						}

		case WM_LANGCHANGED: {

			WCHAR szBuff[MAX_LANGLEN];
            for (int id = 0; id <= (IDS_EXCLREMOVE - IDS_EXCLNEW); id++) {
				LangLoadString(IDS_EXCLNEW + id, szBuff, MAX_LANGLEN);
				SetDlgItemText(hwnd, IDC_EXCLNEW + id, szBuff);
			}

			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT;
			lvc.pszText = szBuff;

			HWND h = GetDlgItem(hwnd, IDC_EXCLLIST);
			LangLoadString(IDS_EXCLNAME, szBuff, MAX_LANGLEN);
			ListView_SetColumn(h, 0, &lvc);
			LangLoadString(IDS_EXCLPARAMS, szBuff, MAX_LANGLEN);
			ListView_SetColumn(h, 1, &lvc);

			ListView_RemoveAllGroups(h);
			ListView_EnableGroupView(h, TRUE);

			LVGROUP lvg;
			lvg.cbSize = sizeof(LVGROUP);
			lvg.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
			lvg.uAlign = LVGA_HEADER_LEFT;
			lvg.iGroupId = 0;
			lvg.pszHeader = szBuff;
			lvg.cchHeader = 0;
			LangLoadString(IDS_EXCLPREVIEWGROUP, szBuff, MAX_LANGLEN);
			ListView_InsertGroup(h, 0, &lvg);

			lvg.iGroupId = 1;
			LangLoadString(IDS_EXCLENUMGROUP, szBuff, MAX_LANGLEN);
			ListView_InsertGroup(h, 1, &lvg);

			LVITEM lvi;
			lvi.iSubItem = 0;
			lvi.mask = LVIF_GROUPID;
			for (lvi.iItem = 0; lvi.iItem < (int)_vExclList.size(); lvi.iItem++) {
				lvi.iGroupId = (_vExclList[lvi.iItem].dwFlags & TSEXCLF_PREVIEW) ? 0 : 1;
				ListView_SetItem(h, &lvi);
			}
			break;
							 }

		HANDLE_MSG(hwnd, WM_INITDIALOG, Excl_OnInitDialog);
	}
	return(0);
}

//-----------------------------------------------------------------
