// tscontrol.cpp

#include "stdafx.h"
#include "generic.h"
#include "main.h"
#include "lang.h"
#include "TaskSwitchXP.h"
#include "tscontrol.h"
#include "resource.h"

//-----------------------------------------------------------

BOOL _SelectTsItem(HWND hwndTs, int nSelTs, BOOL fRedraw) {

	if (!g_pTs || g_nTs <= 0)
		return(FALSE);

	if (nSelTs < 0)
		nSelTs = 0;
	else if (nSelTs >= g_nTs)
		nSelTs = g_nTs - 1;

	HWND hwndPv = NULL;
	while (g_nTs > 0 && CheckTask(nSelTs, &hwndPv) && !hwndPv) {
        DeleteTsItem(nSelTs);
		if (nSelTs >= g_nTs)
			nSelTs = g_nTs - 1;
		fRedraw = TRUE;
	}
	if (g_nTs <= 0)
		return(FALSE);

	g_nCurTs = nSelTs;

	int n = g_nFirstTs + g_nIconsX * g_nIconsY;
	if (fRedraw || !g_hbitmapTs || (g_nFirstTs && g_nCurTs == g_nFirstTs) || 
		g_nCurTs < g_nFirstTs || (g_nTs > n && g_nCurTs >= n - 1)) {

		if (!g_hbitmapTs) {
			if (!InitTsBackground(hwndTs))
				return(FALSE);
		}

		if (g_nCurTs > g_nFirstTs) {
			if (g_nCurTs >= n || (g_nTs > n && g_nCurTs == n - 1)) {
				g_nFirstTs = (g_nCurTs - g_nCurTs % g_nIconsX) - g_nIconsX * (g_nIconsY - 1);
				n = g_nFirstTs + g_nIconsX * g_nIconsY;
				if (g_nTs > n && g_nCurTs >= n - 1)
					g_nFirstTs += g_nIconsX;
			}
		} else {
			g_nFirstTs = (g_nCurTs - g_nCurTs % g_nIconsX);
			if (g_nFirstTs && g_nFirstTs == g_nCurTs)
				g_nFirstTs -= g_nIconsX;
			n = ((g_nTs + g_nIconsX - 1) / g_nIconsX);
			if (n - g_nFirstTs / g_nIconsX < g_nIconsY) {
				g_nFirstTs = (g_nTs - g_nTs % g_nIconsX) - g_nIconsX * g_nIconsY;
				if (g_nTs % g_nIconsX)
					g_nFirstTs += g_nIconsX;
			}
		}
		if (g_nFirstTs < 0)
			g_nFirstTs = 0;

        if (!DrawTaskList())
			return(FALSE);
	}
	return(TRUE);
}


BOOL SelectTask(HWND hwndTs, int nSelTs, BOOL fRedraw) {

	_ASSERT(g_pTs && g_nTs > 0);

	if (!_SelectTsItem(hwndTs, nSelTs, fRedraw))
		return(FALSE);
	InvalidateRect(hwndTs, NULL, FALSE);
	if (!g_pTs[g_nCurTs].hbitmapPv)
		SetTimer(hwndTs, TIMER_PREVIEW, g_dwPvDelay, NULL);
	UpdateInfoTip(g_nCurTs);
	return(TRUE);
}

//-----------------------------------------------------------

BOOL MultiSelectTask(HWND hwndTs, int nCurTs, int nSelTs, BOOL fSelect) {

	_ASSERT(g_pTs && g_nTs > 0);

	BOOL fRedraw = FALSE;
	if (nSelTs >= 0 && nSelTs < g_nTs) {
		if (fSelect) {
			if (!(g_pTs[nSelTs].dwFlags & TI_SELECTED)) {
				g_pTs[nSelTs].dwFlags |= TI_SELECTED;
				g_nSelTs++;
				fRedraw = TRUE;
			}
		} else {
			if (g_pTs[nSelTs].dwFlags & TI_SELECTED) {
				g_pTs[nSelTs].dwFlags &= ~TI_SELECTED;
				g_nSelTs--;
				fRedraw = TRUE;
			}
		}
	}
	BOOL fSelNew = (BOOL)(g_nCurTs != nCurTs);
	if (fSelNew || fRedraw) {
		if (!_SelectTsItem(hwndTs, nCurTs, fRedraw))
			return(FALSE);
	}

	InvalidateRect(hwndTs, fSelNew ? NULL : &g_rcList, FALSE);
	if (!fSelNew)
		InvalidateRect(hwndTs, &g_rcPane, FALSE);
	if (!g_pTs[g_nCurTs].hbitmapPv)
		SetTimer(hwndTs, TIMER_PREVIEW, g_dwPvDelay, NULL);
	UpdateInfoTip(g_nCurTs);
	return(TRUE);
}

//-----------------------------------------------------------

BOOL _MultiSelectTasks(int nFirst, int nLast, BOOL fSelect) {

	_ASSERT(g_pTs && g_nTs > 0);

	if (nFirst < 0)
		nFirst = 0;
	if (nLast >= g_nTs || nLast < 0)
		nLast = g_nTs - 1;

	BOOL nSelTsOld = g_nSelTs;
	if (fSelect) {
		for (int i = nFirst; i <= nLast; i++) {
			if (!(g_pTs[i].dwFlags & TI_SELECTED)) {
				g_pTs[i].dwFlags |= TI_SELECTED;
				g_nSelTs++;
			}
		}
	} else {
		for (int i = nFirst; i <= nLast; i++) {
			if (g_pTs[i].dwFlags & TI_SELECTED) {
				g_pTs[i].dwFlags &= ~TI_SELECTED;
				g_nSelTs--;
			}
		}
	}
	return(nSelTsOld != g_nSelTs);
}

BOOL MultiSelectTasks(HWND hwndTs, int nFirst, int nLast, BOOL fSelect) {

	_ASSERT(g_pTs && g_nTs > 0);

	int nCurTs = nLast;
	if (nFirst > nLast) {
		nLast = nFirst;
		nFirst = nCurTs;
	}
	BOOL fSelNew = _MultiSelectTasks(nFirst, nLast, fSelect);
	return(SelectTask(hwndTs, nCurTs, fSelNew));
}


BOOL MultiSelectInstances(HWND hwndTs, BOOL fSelect) {

	_ASSERT(g_pTs && g_nTs > 0);

	BOOL nSelTsOld = g_nSelTs;
	for (int i = 0; i < g_nTs; i++) {
		if (i == g_nCurTs || 
			!lstrcmpi(g_pTs[g_nCurTs].pszExePath, g_pTs[i].pszExePath)) {
				if (fSelect) {
					if (!(g_pTs[i].dwFlags & TI_SELECTED)) {
						g_pTs[i].dwFlags |= TI_SELECTED;
						g_nSelTs++;
					}
				} else {
					if (g_pTs[i].dwFlags & TI_SELECTED) {
						g_pTs[i].dwFlags &= ~TI_SELECTED;
						g_nSelTs--;
					}					
				}
			}
	}
	if (nSelTsOld != g_nSelTs) {
		if (!_SelectTsItem(hwndTs, g_nCurTs, TRUE))
			return(FALSE);
		InvalidateRect(hwndTs, &g_rcList, FALSE);
		InvalidateRect(hwndTs, &g_rcPane, FALSE);
	}
	return(TRUE);
}

BOOL MultiSelectAll(HWND hwndTs, BOOL fSelect) {
	BOOL fSelNew = _MultiSelectTasks(0, g_nTs - 1, fSelect);
	if (fSelNew) {
		if (!_SelectTsItem(hwndTs, g_nCurTs, TRUE))
			return(FALSE);
		InvalidateRect(hwndTs, &g_rcList, FALSE);
		InvalidateRect(hwndTs, &g_rcPane, FALSE);
	}
	return(TRUE);
}

//-----------------------------------------------------------

void DeleteTsItem(int nTask) {
	
	_ASSERT(g_pTs && nTask >= 0 && nTask < g_nTs && g_nTs > 0);

	if (g_pTs[nTask].dwFlags & TI_SELECTED)
		g_nSelTs--;

	if (g_pTs[nTask].pszExePath)
		HeapFree(g_hheapWnd, 0, g_pTs[nTask].pszExePath);

	if (g_pTs[nTask].hbitmapPv)
		DeleteObject(g_pTs[nTask].hbitmapPv);

	//if (nTask < g_nTs - 1)
    MoveMemory(g_pTs + nTask, g_pTs + nTask + 1, sizeof(TASKINFO) * (g_nTs - nTask - 1));
	g_nTs--;
	HeapReAlloc(g_hheapWnd, 0, g_pTs, g_nTs * sizeof(TASKINFO));
}

//-----------------------------------------------------------
// return TRUE if icon/caption changed
BOOL CheckTask(int nTask, HWND* phWnd) {

	_ASSERT(g_pTs && nTask >= 0 && nTask < g_nTs);

	if (!IsWindow(g_pTs[nTask].hwndOwner)) {
		g_pTs[nTask].hwndOwner = NULL;
		g_pTs[nTask].nWndPv = 0;
		g_pTs[nTask].szCaption[0] = L'\0';
		if (g_pTs[nTask].hbitmapPv) {
			DeleteObject(g_pTs[nTask].hbitmapPv);
			g_pTs[nTask].hbitmapPv = NULL;
		}		
		if (phWnd)
			*phWnd = NULL;
		return(TRUE);
	}

	BOOL fRet = FALSE;

	HWND hwnd = NULL;
	int nWnd = 0;
	while (nWnd < g_pTs[nTask].nWndPv) {
		if (IsWindow(g_pTs[nTask].phWndPv[nWnd])) {
			if (g_pTs[nTask].dwFlags & TI_TRAYWINDOW || 
				IsWindowVisible(g_pTs[nTask].phWndPv[nWnd])) {
				hwnd = g_pTs[nTask].phWndPv[nWnd];
				break;
			}
		}
		if (g_pTs[nTask].phWndPv[nWnd]) {
			g_pTs[nTask].phWndPv[nWnd] = NULL;
			fRet = TRUE;
		}
		nWnd++;
	}
	if (fRet && !(g_dwFlagsEx & TSFEX_OWNERCAPTION)) {
		if (!InternalGetWindowText(hwnd, g_pTs[nTask].szCaption, MAX_CAPTION))
			if (!InternalGetWindowText(g_pTs[nTask].hwndOwner, g_pTs[nTask].szCaption, MAX_CAPTION))
				g_pTs[nTask].szCaption[0] = L'\0';
	}

	if (phWnd) {
		HWND hwndPopup = GetLastActivePopup(g_pTs[nTask].hwndOwner);
#pragma FIX_LATER(2.07-2.08 - Borland environments)
		*phWnd = IsWindowVisible(hwndPopup) ? hwndPopup : hwnd;
	}

	if (!hwnd) {
		g_pTs[nTask].hwndOwner = NULL;
		g_pTs[nTask].nWndPv = 0;
		g_pTs[nTask].szCaption[0] = L'\0';
		if (g_pTs[nTask].hbitmapPv) {
			DeleteObject(g_pTs[nTask].hbitmapPv);
			g_pTs[nTask].hbitmapPv = NULL;
		}
		return(TRUE);
	}
	return(FALSE);
}

//-----------------------------------------------------------
// TRUE - need to redraw
BOOL InvalidateTsItem(int nTask, HWND *phWnd =NULL) {

	_ASSERT(g_pTs && nTask >= 0 && nTask < g_nTs);

	HWND hwndPv;
	BOOL fRedraw = CheckTask(nTask, &hwndPv);
	if (phWnd)
		*phWnd = hwndPv;

	if (hwndPv) {
		WCHAR szCaption[MAX_CAPTION] = L"";
		if (g_dwFlagsEx & TSFEX_OWNERCAPTION ||
			!InternalGetWindowText(hwndPv, szCaption, MAX_CAPTION)) {
			if (!InternalGetWindowText(g_pTs[nTask].hwndOwner, szCaption, MAX_CAPTION)) {
				if (g_dwFlagsEx & TSFEX_OWNERCAPTION)
					InternalGetWindowText(hwndPv, szCaption, MAX_CAPTION);
			}
			g_pTs[nTask].szCaption[MAX_CAPTION - 1] = L'\0';
		}
		fRedraw = lstrcmp(szCaption, g_pTs[nTask].szCaption);
		if (fRedraw)
			lstrcpyn(g_pTs[nTask].szCaption, szCaption, MAX_CAPTION);
	} else {
		fRedraw = TRUE;
	}
	return(fRedraw);
}


void UpdateTask(HWND hwndTs, int nTask) {

	_ASSERT(g_pTs && nTask >= 0 && nTask < g_nTs);

	BOOL fRedraw = InvalidateTsItem(nTask);
	if (fRedraw) {
		if (!_SelectTsItem(hwndTs, nTask, TRUE)) {
			DestroyWindow(hwndTs);
			return;
		}
	}
	StartDrawTaskPreview(g_nCurTs);
	InvalidateRect(hwndTs, fRedraw ? NULL : &g_rcPvEx, FALSE);
}

//-----------------------------------------------------------

void UpdateTaskList() {

	HWND hwndPv;
	volatile int nCurTs = 0;
	while (nCurTs < g_nTs) {
		if (g_pTs[nCurTs].dwFlags & TI_TRAYWINDOW && 
			!(g_dwFlags & TSF_ENUMTRAY)) {
			DeleteTsItem(nCurTs);
		} else {
			InvalidateTsItem(nCurTs, &hwndPv);
			if (!hwndPv) {
				DeleteTsItem(nCurTs);
			} else {			
				if (g_pTs[nCurTs].hbitmapPv) {
					DeleteObject(g_pTs[nCurTs].hbitmapPv);
					g_pTs[nCurTs].hbitmapPv = NULL;
				}			
				nCurTs++;			
			}
		}
	}
}

//-----------------------------------------------------------

void RemoveSelected(HWND hwndTs) {

	DeleteTsItem(g_nCurTs);
	
	volatile int nCurTs = 0;
	while (nCurTs < g_nTs) {
		if (g_pTs[nCurTs].dwFlags & TI_SELECTED) {
			DeleteTsItem(nCurTs);
		} else nCurTs++;		
	}
	if (g_nTs <= 0 || !SelectTask(hwndTs, g_nCurTs, TRUE))
		DestroyWindow(hwndTs);
}

//-----------------------------------------------------------

void MinimizeSelected(HWND hwndTs) {

	BOOL fRedraw = FALSE;
	DWORD dw;

	for (int i = 0; i < g_nTs; i++) {
		if (g_pTs[i].dwFlags & TI_SELECTED || i == g_nCurTs) {
			if (g_pTs[i].dwFlags & TI_TRAYWINDOW) {
				SendMessage(g_hwndMain, WM_UNFLIPFROMTRAY, 
					(WPARAM)g_pTs[i].hwndOwner, SC_MINIMIZE);
				g_pTs[i].dwFlags &= ~TI_TRAYWINDOW;
				fRedraw = TRUE;
			} else {
				DWORD dwStyle = GetWindowLongPtr(g_pTs[i].hwndOwner, GWL_STYLE);
				if (dwStyle & WS_MINIMIZEBOX && !(dwStyle & WS_MINIMIZE)) {
					SendMessageTimeout(g_pTs[i].hwndOwner, WM_SYSCOMMAND, 
						SC_MINIMIZE, 0, SMTO_ABORTIFHUNG, 500, &dw);
					fRedraw = TRUE;
				}
			}
		}
	}
	if (fRedraw) {
		UpdateTaskList();
		if (!SelectTask(hwndTs, g_nCurTs, TRUE)) {
			DestroyWindow(hwndTs);
			return;
		}
	}
}

//-----------------------------------------------------------

void FlipToTraySelected(HWND hwndTs) {

	BOOL fRedraw = FALSE;
	for (int i = 0; i < g_nTs; i++) {
		if (g_pTs[i].dwFlags & TI_SELECTED || i == g_nCurTs) {
			if (SendMessage(g_hwndMain, WM_FLIPTOTRAY, (WPARAM)g_pTs[i].hwndOwner, 0)) {
				g_pTs[i].dwFlags |= TI_TRAYWINDOW;
				if (g_pTs[i].hbitmapPv) {
					DeleteObject(g_pTs[i].hbitmapPv);
					g_pTs[i].hbitmapPv = NULL;
				}
				fRedraw = TRUE;
			}
		}
	}
	if (fRedraw) {
		UpdateTaskList();
		if (!SelectTask(hwndTs, g_nCurTs, TRUE)) {
			DestroyWindow(hwndTs);
			return;
		}
	}
}

//-----------------------------------------------------------

void MaxRestoreSelected(HWND hwndTs, WPARAM wParam) {

	_ASSERT(wParam == SC_MAXIMIZE || wParam == SC_RESTORE);

	HWND *phSC = (HWND*)HeapAlloc(GetProcessHeap(), 0, g_nTs * sizeof(HWND));
	if (!phSC) return;
	HWND hwndPv;
	int nWnd = 0;
	for (int i = 0; i < g_nTs; i++) {
		if (g_pTs[i].dwFlags & TI_SELECTED || i == g_nCurTs) {
			if (g_pTs[i].dwFlags & TI_TRAYWINDOW) {
				SendMessage(g_hwndMain, WM_UNFLIPFROMTRAY, 
					(WPARAM)g_pTs[i].hwndOwner, SC_MINIMIZE);
				g_pTs[i].dwFlags &= ~TI_TRAYWINDOW;				
			}
			CheckTask(i, &hwndPv);
			if (hwndPv) {
				if (i == g_nCurTs && IsIconic(hwndPv))
					hwndPv = NULL;
				phSC[nWnd++] = hwndPv;
			}
		}
	}
	if (!nWnd) {
		HeapFree(GetProcessHeap(), 0, phSC);
		UpdateTaskList();
        return;
	}

	if (nWnd > 1) {
		ANIMATIONINFO ai;
		ai.cbSize = sizeof(ANIMATIONINFO);
		ai.iMinAnimate = FALSE;
		SystemParametersInfo(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &ai, FALSE);

		if (ai.iMinAnimate) {
			ai.iMinAnimate = 0;
			SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &ai, FALSE);
			SetTimer(g_hwndMain, TIMER_SETANIMATION, 500, NULL);
		}
	}

	SwitchToCurTask(hwndTs);
	for (int i = 0; i < nWnd; i++) {
		DWORD dwStyle = GetWindowLongPtr(phSC[i], GWL_STYLE);
		if (!(dwStyle & WS_MAXIMIZEBOX) || (wParam == SC_MAXIMIZE && dwStyle & WS_MAXIMIZE) ||
			(wParam == SC_RESTORE && !(dwStyle & WS_MAXIMIZE) && !IsIconic(phSC[i])))
			continue;
		PostMessage(phSC[i], WM_SYSCOMMAND, wParam, 0);
	}
	HeapFree(GetProcessHeap(), 0, phSC);
}

//-----------------------------------------------------------

void CloseSelected(HWND hwndTs) {

	HWND *phSC = (HWND*)HeapAlloc(GetProcessHeap(), 0, g_nTs * sizeof(HWND));
	if (!phSC) return;
	int nWnd = 0;
	for (int i = 0; i < g_nTs; i++) {
		if (g_pTs[i].dwFlags & TI_SELECTED || i == g_nCurTs) {
			if (g_pTs[i].dwFlags & TI_TRAYWINDOW) {
				SendMessage(g_hwndMain, WM_UNFLIPFROMTRAY, 
					(WPARAM)g_pTs[i].hwndOwner, SC_MINIMIZE);
				g_pTs[i].dwFlags &= ~TI_TRAYWINDOW;				
			}
			if (IsWindow(g_pTs[i].hwndOwner)) {
				phSC[nWnd++] = g_pTs[i].hwndOwner;
			}
		}
	}
	if (!nWnd) {
		HeapFree(GetProcessHeap(), 0, phSC);
		UpdateTaskList();
        return;
	}
	SwitchToCurTask2(hwndTs);
	for (int i = 0; i < nWnd; i++)
		PostMessage(phSC[i], WM_SYSCOMMAND, SC_CLOSE, 0);
	HeapFree(GetProcessHeap(), 0, phSC);
}

//-----------------------------------------------------------

void TerminateSelected(HWND hwndTs) {

    DWORD dwShellId = 0;
	GetWindowThreadProcessId(g_hwndShell, &dwShellId);

	DWORD *pPIDs = (DWORD*)HeapAlloc(GetProcessHeap(), 0, g_nTs * sizeof(DWORD));
	if (!pPIDs) return;
	HWND *phShell = (HWND*)HeapAlloc(GetProcessHeap(), 0, g_nTs * sizeof(HWND));
	if (!phShell) return;

	int nShell = 0, nPIDs = 0;
	for (int i = 0; i < g_nTs; i++) {
		if (g_pTs[i].dwFlags & TI_SELECTED || i == g_nCurTs) {
			if (g_pTs[i].dwFlags & TI_TRAYWINDOW) {
				SendMessage(g_hwndMain, WM_UNFLIPFROMTRAY, 
					(WPARAM)g_pTs[i].hwndOwner, SC_MINIMIZE);
				g_pTs[i].dwFlags &= ~TI_TRAYWINDOW;
			}
			DWORD dwProcessId = g_pTs[i].dwProcessId;
			if (dwProcessId != dwShellId) {
				int n = 0;
				while (n < nPIDs && pPIDs[n] != dwProcessId) n++;
				if (n >= nPIDs) {
					if (MyTerminateProcess(dwProcessId))
						pPIDs[nPIDs++] = dwProcessId;
				}
			} else {
				PostMessage(g_pTs[i].hwndOwner, WM_SYSCOMMAND, SC_CLOSE, 0);
				phShell[nShell++] = g_pTs[i].hwndOwner;
			}
		}
	}

	volatile int nCurTs = 0;
	while (nCurTs < g_nTs) {
		int n = 0;
		if (g_pTs[nCurTs].dwProcessId != dwShellId) {
			for (; n < nPIDs; n++) {
				if (pPIDs[n] == g_pTs[nCurTs].dwProcessId)
					break;
			}
			if (n < nPIDs)
				DeleteTsItem(nCurTs);
			else nCurTs++;
		} else {
			for (; n < nShell; n++) {
				if (phShell[n] == g_pTs[nCurTs].hwndOwner)
					break;
			}
			if (n < nShell)
				DeleteTsItem(nCurTs);
			else nCurTs++;
		}
	}

    HeapFree(GetProcessHeap(), 0, pPIDs);
	HeapFree(GetProcessHeap(), 0, phShell);

	if (nShell || nPIDs) {
		UpdateTaskList();
		if (!SelectTask(hwndTs, g_nCurTs, TRUE)) {
			DestroyWindow(hwndTs);
			return;
		}
	}
}

//-----------------------------------------------------------

void ExploreExePath(PCWSTR pszExePath) {
	if (!pszExePath)
		return;
	int len = lstrlen(pszExePath);
	PWSTR pszCmdLine = (PWSTR)HeapAlloc(GetProcessHeap(), 0, (len + 100) * sizeof(WCHAR));
	if (!pszCmdLine)
		return;
	wsprintf(pszCmdLine, L"/e,/select, \"%s\"", pszExePath);
	ShellExecute(NULL, L"open", L"explorer", pszCmdLine, NULL, SW_SHOWNORMAL);
	HeapFree(GetProcessHeap(), 0, pszCmdLine);
}

//-----------------------------------------------------------

void ReorderSelected(HWND hwndTs, UINT uOrder) {

	DWORD dw;
	HWND hwndPv;
	HWND *phSel = (HWND*)HeapAlloc(GetProcessHeap(), 0, g_nTs * sizeof(HWND));
	if (!phSel) return;
	int nWnd = 0;
	for (int i = 0; i < g_nTs; i++) {
		if (g_pTs[i].dwFlags & TI_SELECTED || i == g_nCurTs) {
			CheckTask(i, &hwndPv);
			if (hwndPv) {
				phSel[nWnd++] = hwndPv;
				if (IsIconic(g_pTs[i].hwndOwner)) {
					SendMessageTimeout(g_pTs[i].hwndOwner, WM_SYSCOMMAND, 
						SC_RESTORE, 0, SMTO_ABORTIFHUNG, 500, &dw);
				}				
			}			
		}
	}
	if (!nWnd) {
		HeapFree(GetProcessHeap(), 0, phSel);
		UpdateTaskList();
        return;
	}

	if (uOrder == RO_TILEHORIZONTALLY || uOrder == RO_TILEVERTICALLY) {
		TileWindows(NULL, (uOrder == RO_TILEHORIZONTALLY) 
			? MDITILE_HORIZONTAL : MDITILE_VERTICAL, NULL, nWnd, phSel);
	} else {
		CascadeWindows(NULL, MDITILE_SKIPDISABLED, NULL, nWnd, phSel);
	}

	SwitchToCurTask(hwndTs);

	HeapFree(GetProcessHeap(), 0, phSel);
}

//-----------------------------------------------------------
//-----------------------------------------------------------

int CompareTitles(const PVOID pel1, const PVOID pel2) {
	int nRet = lstrcmp(((PTASKINFO)pel1)->szCaption, ((PTASKINFO)pel2)->szCaption);
	if (!nRet)
		nRet = lstrcmpi(((PTASKINFO)pel1)->pszExePath, ((PTASKINFO)pel2)->pszExePath);
	if (g_dwFlagsList & TSFL_SORTEDTITLES)
		nRet = -nRet;
	return(nRet);
}

int CompareExePaths(const PVOID pel1, const PVOID pel2) {
	int nRet = lstrcmpi(((PTASKINFO)pel1)->pszExePath, ((PTASKINFO)pel2)->pszExePath);
	if (!nRet)
		nRet = lstrcmp(((PTASKINFO)pel1)->szCaption, ((PTASKINFO)pel2)->szCaption);
	if (g_dwFlagsList & TSFL_SORTEDEXEPATHS)
		nRet = -nRet;
	return(nRet);
}

int CompareModuleNames(const PVOID pel1, const PVOID pel2) {
	int nRet = lstrcmpi(((PTASKINFO)pel1)->szModuleName, ((PTASKINFO)pel2)->szModuleName);
	//if (!nRet)
	//	nRet = lstrcmp(((PTASKINFO)pel1)->szCaption, ((PTASKINFO)pel2)->szCaption);
	//if (g_dwFlagsList & TSFL_SORTEDEXEPATHS)
	//	nRet = -nRet;
	return(nRet);
}

//-----------------------------------------------------------

//TODO: this function probably must be removed. It's better to rewrite
//SortTaskList function replacing it's second parameter with enum value
//controlling sorting mode.
void SortTaskListByModuleName(HWND hwndTs)
{
	PWSTR pCur = NULL;
	if (g_nCurTs >= 0 && g_nCurTs < g_nTs)
		pCur = g_pTs[g_nCurTs].szModuleName;

	MyInsertionSort(g_pTs, g_nTs, sizeof(TASKINFO), CompareModuleNames );
	//if (fExePaths) g_dwFlagsList ^= TSFL_SORTEDEXEPATHS;
	//else g_dwFlagsList ^= TSFL_SORTEDTITLES;
	if (pCur) {
		for (g_nCurTs = 0; g_nCurTs < g_nTs; g_nCurTs++)
			if (g_pTs[g_nCurTs].szModuleName == pCur)
				break;
	} else g_nCurTs = 0;

	if (!SelectTask(hwndTs, g_nCurTs, TRUE)) {
		DestroyWindow(hwndTs);
		return;
	}
}
void SortTaskList(HWND hwndTs, BOOL fExePaths) {

	PWSTR pCur = NULL;
	if (g_nCurTs >= 0 && g_nCurTs < g_nTs)
		pCur = g_pTs[g_nCurTs].pszExePath;

	MyInsertionSort(g_pTs, g_nTs, sizeof(TASKINFO), 
		fExePaths ? CompareExePaths : CompareTitles);
	if (fExePaths)
		g_dwFlagsList ^= TSFL_SORTEDEXEPATHS;
	else g_dwFlagsList ^= TSFL_SORTEDTITLES;

	if (pCur) {
		for (g_nCurTs = 0; g_nCurTs < g_nTs; g_nCurTs++)
			if (g_pTs[g_nCurTs].pszExePath == pCur)
				break;
	} else g_nCurTs = 0;

	if (!SelectTask(hwndTs, g_nCurTs, TRUE)) {
		DestroyWindow(hwndTs);
		return;
	}
}

//-----------------------------------------------------------
//-----------------------------------------------------------

void ShowTaskMenu(HWND hwndTs, POINT pt) {

	WCHAR szBuff[MAX_LANGLEN];
	HMENU hmenu = CreatePopupMenu();
	for (int i = 0; i <= (IDM_TERMINATE - IDM_SWITCH); i++) {
		LangLoadString(IDS_SWITCH + i, szBuff, SIZEOF_ARRAY(szBuff));
		AppendMenu(hmenu, MF_STRING, IDM_SWITCH + i, szBuff);
	}
	InsertMenu(hmenu, IDM_MINIMIZE, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	InsertMenu(hmenu, IDM_RESTORE, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	InsertMenu(hmenu, IDM_CASCADE, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	//InsertMenu(hmenu, IDM_CLOSE, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	InsertMenu(hmenu, IDM_TERMINATE, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	SetMenuDefaultItem(hmenu, IDM_SWITCH, MF_BYCOMMAND);

	if (g_nSelTs <= 0 || (g_nSelTs == 1 && g_pTs[g_nCurTs].dwFlags & TI_SELECTED)) {
		EnableMenuItem(hmenu, IDM_CASCADE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hmenu, IDM_TILEHORIZONTALLY, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hmenu, IDM_TILEVERTICALLY, MF_BYCOMMAND | MF_GRAYED);
	}

	SetForegroundWindow(hwndTs);
	UINT uMenuID = TrackPopupMenu(hmenu, 
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
		pt.x, pt.y, 0, hwndTs, NULL);
	PostMessage(hwndTs, WM_NULL, 0, 0);

	DestroyMenu(hmenu);

	switch (uMenuID) {
		case IDM_SWITCH:
			SwitchToCurTask(hwndTs);
			break;
		case IDM_MINIMIZE:
			MinimizeSelected(hwndTs);
			break;
		case IDM_FLIPTOTRAY:
			FlipToTraySelected(hwndTs);
			break;
		case IDM_MAXIMIZE:
			MaxRestoreSelected(hwndTs, SC_MAXIMIZE);
			break;
		case IDM_RESTORE:
			MaxRestoreSelected(hwndTs, SC_RESTORE);
			break;
		case IDM_CLOSE:
			CloseSelected(hwndTs);
			break;
		case IDM_CASCADE:
			ReorderSelected(hwndTs, RO_CASCADE);
			break;
		case IDM_TILEVERTICALLY:
			ReorderSelected(hwndTs, RO_TILEVERTICALLY);
			break;
		case IDM_TILEHORIZONTALLY:
			ReorderSelected(hwndTs, RO_TILEHORIZONTALLY);
			break;
		case IDM_TERMINATE:
			TerminateSelected(hwndTs);
			break;		
	}
}

//-----------------------------------------------------------

void ShowCommonMenu(HWND hwndTs, POINT pt) {

	WCHAR szBuff[MAX_LANGLEN];
	HMENU hmenu = CreatePopupMenu();
	for (int i = 0; i <= (IDM_SORTBYEXEPATH - IDM_SHOWINFO); i++) {
		LangLoadString(IDS_SHOWINFO + i, szBuff, SIZEOF_ARRAY(szBuff));
		AppendMenu(hmenu, MF_STRING, IDM_SHOWINFO + i, szBuff);
	}
	InsertMenu(hmenu, IDM_SELECTALL, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	InsertMenu(hmenu, IDM_REMOVESEL, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	InsertMenu(hmenu, IDM_SORTBYTITLE, MF_SEPARATOR | MF_BYCOMMAND, 0, NULL);
	CheckMenuItem(hmenu, IDM_SHOWINFO, g_hwndInfo 
		? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));
	SetMenuDefaultItem(hmenu, (g_nSelTs == g_nTs) 
		? IDM_SELECTNONE : IDM_SELECTALL, MF_BYCOMMAND);

	SetForegroundWindow(hwndTs);
	UINT uMenuID = TrackPopupMenu(hmenu, 
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
		pt.x, pt.y, 0, hwndTs, NULL);
	PostMessage(hwndTs, WM_NULL, 0, 0);

	DestroyMenu(hmenu);

	switch (uMenuID) {
		case IDM_SHOWINFO:
			ShowHideInfoTip(hwndTs);
			break;
		case IDM_SELECTALL:
			MultiSelectAll(hwndTs, TRUE);
			break;
		case IDM_SELECTNONE:
			MultiSelectAll(hwndTs, FALSE);
			break;
//		case IDM_INVERTSEL:
//			break;
		case IDM_REMOVESEL:
			RemoveSelected(hwndTs);
			break;
		case IDM_SORTBYTITLE:
			SortTaskList(hwndTs, FALSE);
			break;
		case IDM_SORTBYEXEPATH:
			SortTaskList(hwndTs, TRUE);
			break;
	}
}

//-----------------------------------------------------------
