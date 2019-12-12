// excldlg.cpp

#include "stdafx.h"
#include "main.h"
#include "ownerdrawXp.h"
#include "generic.h"
#include "lang.h"
#include "exclusions.h"
#include "resource.h"

#include <strsafe.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#pragma comment(lib, "version.lib")

//-----------------------------------------------------------------

#define HUNG_TIMEOUT					250


#define MAX_WNDPV						16

typedef struct WNDINFO {
	WCHAR szClassName[MAX_CLASSNAME];
	WCHAR szCaption[MAX_CAPTION];
	DWORD dwFlags;
	int idIcon;
} *PWNDINFO;


typedef struct ENUMTREEITEM {

	WCHAR szExclName[MAX_EXCLNAME];
	WCHAR szExeName[MAX_FILENAME];
	WCHAR szCaption[MAX_CAPTION];

	WNDINFO pWndPv[MAX_WNDPV];
	int nWndPv;

	HWND hwndOwner;
	DWORD dwProcessId;
	int idIcon;

	DWORD dwFlags;
} *PENUMTREEITEM;

//-----------------------------------------------------------------

extern vector<TSEXCLUSION> _vExclList;
vector<ENUMTREEITEM> _vEnumTree;

HIMAGELIST _himlTs						= NULL;
HWND _hwndProgman						= NULL;

PTSEXCLUSION _ptse						= NULL;

//-----------------------------------------------------------------

void GetWindowIcons(HWND hwnd, HICON* phIcon, HICON* phIconSm) {

	_ASSERT(phIcon);

	BOOL fIsHungApp = FALSE;

	HICON hIcon = NULL;
	if (!SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, 
		SMTO_ABORTIFHUNG, HUNG_TIMEOUT, (PDWORD_PTR)&hIcon)) {
		DWORD dwErr = GetLastError();
		if (dwErr == 0 || dwErr == 1460) {
			fIsHungApp = TRUE;
			goto _HUNG_ICON;
		}
	}
	if (!hIcon) 
		hIcon = (HICON)(UINT_PTR)GetClassLongPtr(hwnd, GCLP_HICON);

	if (!hIcon) {
_HUNG_ICON:		
		hIcon = LoadIcon(NULL, IDI_APPLICATION);
	}
	*phIcon = hIcon;

	if (phIconSm) {
		if (fIsHungApp)
			goto _HUNG_ICONSM;
		hIcon = NULL;
		if (!SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, 
			SMTO_ABORTIFHUNG, HUNG_TIMEOUT, (PDWORD_PTR)&hIcon)) {
			DWORD dwErr = GetLastError();
			if (dwErr == 0 || dwErr == 1460)
				goto _HUNG_ICONSM;
		}
		if (!hIcon)
			hIcon = (HICON)(UINT_PTR)GetClassLongPtr(hwnd, GCLP_HICONSM);
		if (hIcon) {
			*phIconSm = hIcon;
		} else {
_HUNG_ICONSM:
			*phIconSm = *phIcon;
		}
	}
}

//-----------------------------------------------------------------

BOOL IsWindowExcluded(PENUMTREEITEM peti, int nWnd) {

	_ASSERT(nWnd >= 0 && nWnd < MAX_WNDPV);

	peti->pWndPv[nWnd].idIcon = peti->idIcon;
	for (int i = 0; i < (int)_vExclList.size(); i++) {
		TSEXCLUSION& tse = _vExclList[i];
		if (!(tse.dwFlags & TSEXCLF_BYPROCESS) || 
			!lstrcmpi(tse.szExeName, peti->szExeName)) {
			if (tse.dwFlags & TSEXCLF_BYCLASS && 
				lstrcmp(tse.szClassName, peti->pWndPv[nWnd].szClassName)) {
				continue;
			} else if (!(tse.dwFlags & TSEXCLF_BYCAPTION)) {
				peti->pWndPv[nWnd].dwFlags |= (tse.dwFlags & TSEXCLF_PREVIEW) 
					? TSEXCLF_PREVIEW : TSEXCLF_ENUM;
				peti->pWndPv[nWnd].idIcon = 0;
				if (tse.dwFlags & TSEXCLF_BYPROCESS && !(tse.dwFlags & TSEXCLF_BYCLASS)) {
					peti->dwFlags |= (tse.dwFlags & TSEXCLF_PREVIEW) 
						? TSEXCLF_PREVIEW : TSEXCLF_ENUM;
					peti->idIcon = 0;
				}
				break;
			}
			if (tse.dwFlags & TSEXCLF_BYCAPTION) {
				if (!lstrcmp(tse.szCaption, peti->pWndPv[nWnd].szCaption)) {
					peti->pWndPv[nWnd].dwFlags |= (tse.dwFlags & TSEXCLF_PREVIEW) 
						? TSEXCLF_PREVIEW : TSEXCLF_ENUM;
					peti->pWndPv[nWnd].idIcon = 0;
					break;
				}
			}
		}
	}
	return(peti->pWndPv[nWnd].idIcon == 0);
}


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {

	DWORD dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
	if (dwStyle & WS_VISIBLE) {

		HWND hwndOwner, hwndTmp = hwnd;
		do {
			hwndOwner = hwndTmp;
			hwndTmp = GetWindow(hwndTmp, GW_OWNER);
		} while (hwndTmp && hwndTmp != _hwndProgman); // service messages

		/*vector<ENUMTREEITEM>::iterator it = _vEnumTree.begin(),
			itEnd = _vEnumTree.end();*/
		int i = 0, n = (int)_vEnumTree.size();
		while (i < n && _vEnumTree[i].hwndOwner != hwndOwner)
			i++;
		if (i < n) {
			ENUMTREEITEM& etiItem = _vEnumTree[i];
			if (etiItem.nWndPv < MAX_WNDPV) {
				if (!GetClassName(hwnd, etiItem.pWndPv[etiItem.nWndPv].szClassName, MAX_CLASSNAME))
					return(TRUE);
				if (!InternalGetWindowText(hwnd, etiItem.pWndPv[etiItem.nWndPv].szCaption, MAX_CAPTION))
					etiItem.pWndPv[etiItem.nWndPv].szCaption[0] = L'\0';
				if (etiItem.idIcon)
					IsWindowExcluded(&etiItem, etiItem.nWndPv);
				else {
					etiItem.pWndPv[etiItem.nWndPv].dwFlags = etiItem.dwFlags;
					etiItem.pWndPv[etiItem.nWndPv].idIcon = etiItem.idIcon;
				}
				etiItem.nWndPv++;
			}
		} else {

			DWORD dwStyleEx = GetWindowLongPtr(hwndOwner, GWL_EXSTYLE),
				dwStyleEx2 = (hwnd != hwndOwner) ? GetWindowLongPtr(hwnd, GWL_EXSTYLE) : dwStyleEx;
			if (!(dwStyleEx & WS_EX_TOOLWINDOW) || dwStyleEx2 & WS_EX_APPWINDOW || 
				(!(dwStyleEx2 & WS_EX_TOOLWINDOW) && dwStyleEx2 & WS_EX_CONTROLPARENT)) {

				ENUMTREEITEM eti;
				ZeroMemory(&eti, sizeof(ENUMTREEITEM));

				GetWindowThreadProcessId(hwndOwner, &eti.dwProcessId);
				if (eti.dwProcessId == lParam) // current process
					return(TRUE);

				HANDLE hProcess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, eti.dwProcessId);
				if (!hProcess)
					return(TRUE);

				DWORD dw;
				HMODULE hMod;
				WCHAR szBuff[MAX_FILEPATH];
				if (!EnumProcessModules(hProcess, &hMod, sizeof(hMod), &dw) ||
					!GetModuleFileNameEx(hProcess, hMod, szBuff, MAX_FILEPATH)) {
					CloseHandle(hProcess);
					return(TRUE);
				}
				CloseHandle(hProcess);

				PWSTR pFileName = szBuff + lstrlen(szBuff);
				while (*pFileName != L'\\' && pFileName > szBuff)
					pFileName--;
				if (*pFileName == L'\\')
					pFileName++;

				StringCchCopy(eti.szExeName, MAX_FILENAME, pFileName);

				eti.szExclName[0] = L'\0';

				DWORD cbVersion = GetFileVersionInfoSize(szBuff, &dw);
				if (cbVersion) {
					PVOID pBlock = HeapAlloc(GetProcessHeap(), 0, cbVersion);
					if (GetFileVersionInfo(szBuff, 0, cbVersion, pBlock)) {
						struct LANGANDCODEPAGE {
							WORD wLanguage;
							WORD wCodePage;
						} *pTranslate;
						if (VerQueryValue(pBlock, L"\\VarFileInfo\\Translation", (PVOID*)&pTranslate, 
							(PUINT)&dw) && dw >= sizeof(WORD) * 2) {

							StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), 
								L"\\StringFileInfo\\%04x%04x\\FileDescription", 
								pTranslate[0].wLanguage, pTranslate[0].wCodePage);
							PWSTR pDescription;
							if (VerQueryValue(pBlock, szBuff, (PVOID*)&pDescription, (PUINT)&dw)) {
								StringCchCopy(eti.szExclName, MAX_EXCLNAME, pDescription);
							}
						}
					}
					HeapFree(GetProcessHeap(), 0, pBlock);
				}

				if (eti.szExclName[0] == L'\0')
					StringCchCopy(eti.szExclName, MAX_EXCLNAME, eti.szExeName);

				eti.hwndOwner = hwndOwner;

				if (!GetClassName(hwnd, eti.pWndPv[0].szClassName, MAX_CLASSNAME))
					return(TRUE);

				if (!InternalGetWindowText(hwnd, eti.pWndPv[0].szCaption, MAX_CAPTION))
					eti.pWndPv[0].szCaption[0] = L'\0';
				eti.nWndPv++;

				if (!InternalGetWindowText(hwndOwner, eti.szCaption, MAX_CAPTION))
					if (!InternalGetWindowText(hwnd, eti.szCaption, MAX_CAPTION))
						eti.szCaption[0] = L'\0';

				HICON hIconSm, hIcon;
				GetWindowIcons(hwndOwner, &hIcon, &hIconSm);
				eti.idIcon = ImageList_AddIcon(_himlTs, hIconSm);

				IsWindowExcluded(&eti, 0);

				_vEnumTree.push_back(eti);
			}
		}
	}
	return(TRUE);
}


void UpdateEnumTree(HWND hwndTv) {

	SetWindowRedraw(hwndTv, FALSE);

	TreeView_DeleteAllItems(hwndTv);
	_vEnumTree.clear();

	ImageList_RemoveAll(_himlTs);
	HICON hIcon = (HICON)LoadImage(g_hinstExe, 
		MAKEINTRESOURCE(IDI_EXCLUSIONS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	ImageList_AddIcon(_himlTs, hIcon);
	DestroyIcon(hIcon);
	hIcon = (HICON)LoadImage(g_hinstExe, 
		MAKEINTRESOURCE(IDI_APPWINDOW), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	ImageList_AddIcon(_himlTs, hIcon);
	DestroyIcon(hIcon);

	EnumDesktopWindows(NULL, EnumWindowsProc, (LPARAM)GetCurrentProcessId());


	WCHAR szBuff[1024];

    TVINSERTSTRUCT tvis;
	tvis.hInsertAfter = TVI_LAST;
	tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.itemex.pszText = szBuff;
	
	vector<ENUMTREEITEM>::iterator it = _vEnumTree.begin(), 
		itEnd = _vEnumTree.end();
	int nItem = 0;
	while (it != itEnd) {

		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"\"%s\", %s [%u]", 
			it->szCaption, it->szExeName, it->dwProcessId);
		tvis.itemex.iImage = tvis.itemex.iSelectedImage = it->idIcon;
		tvis.hParent = TVI_ROOT;
		tvis.itemex.lParam = MAKELPARAM(nItem, -1);
        tvis.hParent = TreeView_InsertItem(hwndTv, &tvis);

		int nWnd = it->nWndPv - 1;
		while (nWnd >= 0) {			
			StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"\"%s\", %s", 
				it->pWndPv[nWnd].szCaption, it->pWndPv[nWnd].szClassName);
			tvis.itemex.lParam = MAKELPARAM(nItem, nWnd);
			tvis.itemex.iImage = tvis.itemex.iSelectedImage = it->pWndPv[nWnd].idIcon;
			TreeView_InsertItem(hwndTv, &tvis);
			nWnd--;
		}
		it++;
		nItem++;
	}
	SetWindowRedraw(hwndTv, TRUE);
}

//-----------------------------------------------------------------

BOOL ExclWnd_OnInitDialog(HWND hwnd, HWND, LPARAM lParam) {
    
	if (g_pfnEnableThemeDialogTexture)
		g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

	_ptse = (PTSEXCLUSION)lParam;
	_ASSERT(_ptse);

	CenterWindow(hwnd, GetParent(hwnd));

	/*RECT rc;
	GetWindowRect(GetParent(hwnd), &rc);
	SetWindowPos(hwnd, NULL, rc.left + (rc.right - rc.left) / 4, 
		rc.top + (rc.bottom - rc.top) / 4, 0, 0, SWP_NOZORDER | SWP_NOSIZE);*/

	_himlTs = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1);	

	HWND h = GetDlgItem(hwnd, IDC_ENUMTREE);
	TreeView_SetImageList(h, _himlTs, TVSIL_NORMAL);	

	SendMessage(hwnd, WM_LANGCHANGED, 0, 0);
	UpdateEnumTree(h);

	if (_ptse->dwFlags & TSEXCLF_NEW) {
		CheckDlgButton(hwnd, IDC_EWFROMPREVIEW, BST_CHECKED);
		CheckDlgButton(hwnd, IDC_EWENABLED, BST_CHECKED);
	} else {
		SetDlgItemText(hwnd, IDC_EWNAMEEDIT, _ptse->szExclName);
		SetDlgItemText(hwnd, IDC_EWBYPROCESSEDIT, _ptse->szExeName);
		SetDlgItemText(hwnd, IDC_EWBYCLASSEDIT, _ptse->szClassName);
		SetDlgItemText(hwnd, IDC_EWBYCAPTIONEDIT, _ptse->szCaption);
		CheckDlgButton(hwnd, IDC_EWBYPROCESS, 
			(_ptse->dwFlags & TSEXCLF_BYPROCESS) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_EWBYCLASS, 
			(_ptse->dwFlags & TSEXCLF_BYCLASS) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_EWBYCAPTION, 
			(_ptse->dwFlags & TSEXCLF_BYCAPTION) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_EWFROMPREVIEW, 
			(_ptse->dwFlags & TSEXCLF_PREVIEW) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_EWFROMENUM,
			(_ptse->dwFlags & TSEXCLF_PREVIEW) ? BST_UNCHECKED : BST_CHECKED);
		CheckDlgButton(hwnd, IDC_EWENABLED, 
			(_ptse->dwFlags & TSEXCLF_DISABLED) ? BST_UNCHECKED : BST_CHECKED);
	}
	return(TRUE);
}

//-----------------------------------------------------------------

void ExclWnd_OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT codeNotify) {

	if (codeNotify == BN_CLICKED) {
		switch (id) {

			case IDC_EWUPDATE:
				UpdateEnumTree(GetDlgItem(hwnd, IDC_ENUMTREE));
				SetDlgItemText(hwnd, IDC_EWEXCLUDED, L"");
				SetDlgItemText(hwnd, IDC_EWNAMEEDIT, L"");
				SetDlgItemText(hwnd, IDC_EWBYPROCESSEDIT, L"");
				SetDlgItemText(hwnd, IDC_EWBYCLASSEDIT, L"");
				SetDlgItemText(hwnd, IDC_EWBYCAPTIONEDIT, L"");
				CheckDlgButton(hwnd, IDC_EWBYPROCESS, BST_UNCHECKED);
				CheckDlgButton(hwnd, IDC_EWBYCLASS, BST_UNCHECKED);
				CheckDlgButton(hwnd, IDC_EWBYCAPTION, BST_UNCHECKED);
				break;

			case IDOK:
			case IDC_EWOK: {

				_ASSERT(_ptse);				

				WCHAR szErrTitle[MAX_LANGLEN] = L"", szErrText[MAX_LANGLEN] = L"";

				DWORD dwFlags = 0;
				if (IsDlgButtonChecked(hwnd, IDC_EWBYPROCESS) == BST_CHECKED)
					dwFlags |= TSEXCLF_BYPROCESS;
				if (IsDlgButtonChecked(hwnd, IDC_EWBYCLASS) == BST_CHECKED)
					dwFlags |= TSEXCLF_BYCLASS;
				if (IsDlgButtonChecked(hwnd, IDC_EWBYCAPTION) == BST_CHECKED)
					dwFlags |= TSEXCLF_BYCAPTION;
				if (IsDlgButtonChecked(hwnd, IDC_EWFROMPREVIEW) == BST_CHECKED)
					dwFlags |= TSEXCLF_PREVIEW;
				if (IsDlgButtonChecked(hwnd, IDC_EWENABLED) != BST_CHECKED)
					dwFlags |= TSEXCLF_DISABLED;

				EDITBALLOONTIP ebt;
				ebt.cbStruct = sizeof(EDITBALLOONTIP);
				ebt.pszTitle = szErrTitle;
				ebt.pszText = szErrText;
				ebt.ttiIcon = TTI_ERROR;
				if (GetWindowTextLength(GetDlgItem(hwnd, IDC_EWNAMEEDIT)) <= 0) {
					LangLoadString(IDS_ERR_EWNAMETITLE, szErrTitle, MAX_LANGLEN);
					LangLoadString(IDS_ERR_EWNAME, szErrText, MAX_LANGLEN);
					Edit_ShowBalloonTip(GetDlgItem(hwnd, IDC_EWNAMEEDIT), &ebt);
					break;
				}
				if (!(dwFlags & TSEXCLF_BYMASK)) {
					LangLoadString(IDS_ERR_EWBYPARAMSTITLE, szErrTitle, MAX_LANGLEN);
					LangLoadString(IDS_ERR_EWBYPARAMS, szErrText, MAX_LANGLEN);
					Edit_ShowBalloonTip(GetDlgItem(hwnd, IDC_EWBYPROCESSEDIT), &ebt);
					break;
				}
				if (dwFlags & TSEXCLF_BYPROCESS && 
					GetWindowTextLength(GetDlgItem(hwnd, IDC_EWBYPROCESSEDIT)) <= 0) {
					LangLoadString(IDS_ERR_EWBYPROCESSTITLE, szErrTitle, MAX_LANGLEN);
					LangLoadString(IDS_ERR_EWBYPROCESS, szErrText, MAX_LANGLEN);
					Edit_ShowBalloonTip(GetDlgItem(hwnd, IDC_EWBYPROCESSEDIT), &ebt);
					break;
				}
				if (dwFlags & TSEXCLF_BYCLASS && 
					GetWindowTextLength(GetDlgItem(hwnd, IDC_EWBYCLASSEDIT)) <= 0) {
					LangLoadString(IDS_ERR_EWBYCLASSTITLE, szErrTitle, MAX_LANGLEN);
					LangLoadString(IDS_ERR_EWBYCLASS, szErrText, MAX_LANGLEN);
					Edit_ShowBalloonTip(GetDlgItem(hwnd, IDC_EWBYCLASSEDIT), &ebt);
					break;
				}				

				GetDlgItemText(hwnd, IDC_EWNAMEEDIT, _ptse->szExclName, MAX_EXCLNAME);
				GetDlgItemText(hwnd, IDC_EWBYPROCESSEDIT, _ptse->szExeName, MAX_FILENAME);
				GetDlgItemText(hwnd, IDC_EWBYCLASSEDIT, _ptse->szClassName, MAX_CLASSNAME);
				GetDlgItemText(hwnd, IDC_EWBYCAPTIONEDIT, _ptse->szCaption, MAX_CAPTION);
				_ptse->dwFlags = dwFlags;
				EndDialog(hwnd, TRUE);
				break;
							  }
			case IDCANCEL:
			case IDC_EWCANCEL:
				EndDialog(hwnd, FALSE);
				break;
		}
	}
}

//-----------------------------------------------------------------

INT_PTR CALLBACK Dlg_ExcludeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_NOTIFY: {
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			if (pnmhdr->idFrom == IDC_ENUMTREE) {
				switch (pnmhdr->code) {
					case TVN_SELCHANGED: {
						LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
						if (pnmtv->itemNew.state & TVIS_SELECTED) {
							int nTask = LOWORD(pnmtv->itemNew.lParam),
								nWnd = HIWORD(pnmtv->itemNew.lParam);
							if (nTask >= 0 && nTask < (int)_vEnumTree.size()) {
								SetDlgItemText(hwnd, IDC_EWNAMEEDIT, _vEnumTree[nTask].szExclName);
								SetDlgItemText(hwnd, IDC_EWBYPROCESSEDIT, _vEnumTree[nTask].szExeName);

								BOOL fCustom = (BOOL)(nWnd >= 0 && nWnd < MAX_WNDPV);
								SetDlgItemText(hwnd, IDC_EWBYCLASSEDIT, 
									fCustom ? _vEnumTree[nTask].pWndPv[nWnd].szClassName : L"");
								SetDlgItemText(hwnd, IDC_EWBYCAPTIONEDIT, 
									fCustom ? _vEnumTree[nTask].pWndPv[nWnd].szCaption : L"");

								CheckDlgButton(hwnd, IDC_EWBYPROCESS, BST_CHECKED);
								CheckDlgButton(hwnd, IDC_EWBYCLASS, fCustom ? BST_CHECKED : BST_UNCHECKED);
								CheckDlgButton(hwnd, IDC_EWBYCAPTION, BST_UNCHECKED);

								DWORD dwFlags = fCustom 
									? _vEnumTree[nTask].pWndPv[nWnd].dwFlags : _vEnumTree[nTask].dwFlags;
								WCHAR szBuff[MAX_LANGLEN];
								LangLoadString((dwFlags & TSEXCLF_PREVIEW) ? IDS_EWEXCLUDED1 
									: ((dwFlags & TSEXCLF_ENUM) ? IDS_EWEXCLUDED2 : IDS_EWEXCLUDED0), 
									szBuff, MAX_LANGLEN);
								SetDlgItemText(hwnd, IDC_EWEXCLUDED, szBuff);
							}
						}
						break;
										 }
				}
			}
			break;
						}

        HANDLE_MSG(hwnd, WM_COMMAND, ExclWnd_OnCommand);

		case WM_LANGCHANGED: {
			WCHAR szBuff[MAX_LANGLEN];
			LangLoadString(IDS_EXCLUDEWND, szBuff, MAX_LANGLEN);
			SetWindowText(hwnd, szBuff);

			for (int id = 0; id <= (IDS_EWENABLED - IDS_EWOK); id++) {
				LangLoadString(IDS_EWOK + id, szBuff, SIZEOF_ARRAY(szBuff));
				SetDlgItemText(hwnd, IDC_EWOK + id, szBuff);
			}
			break;
							 }

		HANDLE_MSG(hwnd, WM_INITDIALOG, ExclWnd_OnInitDialog);

		case WM_DESTROY:
			_ptse = NULL;
			break;
	}
	return(0);
}

//-----------------------------------------------------------------

