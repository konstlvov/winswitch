// generic.cpp

#include "stdafx.h"
#include "main.h"
#include "generic.h"
#include "lang.h"
#include "resource.h"

//-----------------------------------------------------------

void ReportError(LPCWSTR szError) {
	
	WCHAR szCaption[MAX_LANGLEN];
	LangLoadString(IDS_ERRCAPTION, szCaption, MAX_LANGLEN);

	MessageBeep(MB_ICONERROR);
	MessageBox(g_hwndMain, szError, szCaption, 
		MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
}

void ReportError(UINT uErrorID) {
	WCHAR szError[MAX_LANGLEN];
	LangLoadString(uErrorID, szError, MAX_LANGLEN);
	ReportError(szError);
}

void FatalError(UINT uErrorID) {
	ReportError(uErrorID);
	ExitProcess(1);
}

//-----------------------------------------------------------

BOOL ConfirmMessage(LPCWSTR szConfirm, UINT uIconType) {
	
	WCHAR szCaption[MAX_LANGLEN];
	LangLoadString(IDS_CONFIRMCAPTION, szCaption, MAX_LANGLEN);

	UINT uType = uIconType | MB_YESNO | MB_TOPMOST | MB_SETFOREGROUND;	
	MessageBeep(MB_ICONQUESTION);
	return(MessageBox(g_hwndMain, szConfirm, szCaption, uType) == IDYES);
}

BOOL ConfirmMessage(UINT uConfirmID, UINT uIconType) {
	WCHAR szConfirm[MAX_LANGLEN];
	LangLoadString(uConfirmID, szConfirm, MAX_LANGLEN);
	return(ConfirmMessage(szConfirm, uIconType));
}

//-----------------------------------------------------------

void HotKeyError(LPCWSTR szHk, UINT fModifiers, UINT vkCode) {
	WCHAR szHkError[MAX_LANGLEN], szError[2 * MAX_LANGLEN + 20];
	LangLoadString(IDS_ERR_HKERROR, szHkError, MAX_LANGLEN);
	wsprintf(szError, szHkError, szHk, fModifiers, vkCode);
	ReportError(szError);
}

void HotKeyError(UINT uHkID, UINT fModifiers, UINT vkCode) {
	WCHAR szHk[MAX_LANGLEN];
	LangLoadString(uHkID, szHk, SIZEOF_ARRAY(szHk));
	HotKeyError(szHk, fModifiers, vkCode);
}

//-----------------------------------------------------------

HRGN GetPreviewRegion(HWND hwnd, CONST XFORM* pxForm) {

	HRGN hrgn = NULL, hrgnWnd = NULL;
	LPRGNDATA pRgnData = NULL;

	__try {

		hrgnWnd = CreateRectRgn(0, 0, 0, 0);
		if (GetWindowRgn(hwnd, hrgnWnd) == ERROR)
			__leave;

		DWORD cbData = GetRegionData(hrgnWnd, 0, NULL);
		if (!cbData) __leave;
		pRgnData = (LPRGNDATA)HeapAlloc(GetProcessHeap(), 0, cbData);
		GetRegionData(hrgnWnd, cbData, pRgnData);

		//if (pRgnData->rdh.iType != RDH_RECTANGLES)
		//	__leave;

		hrgn = ExtCreateRegion(pxForm, cbData, pRgnData);
	}
	__finally {
		if (hrgnWnd)
			DeleteObject(hrgnWnd);
		if (pRgnData)
			HeapFree(GetProcessHeap(), 0, pRgnData);
	}
	return(hrgn);
}

//-----------------------------------------------------------

BOOL MyIsHungAppWindow(HWND hwnd) {

    if (g_pfnIsHungAppWindow && g_pfnIsHungAppWindow(hwnd))
		return(TRUE);

	DWORD dw;
	LRESULT lResult = SendMessageTimeout(hwnd, WM_NULL, 0, 0, 
		SMTO_ABORTIFHUNG, HUNG_TIMEOUT, &dw);
	if (lResult)
		return(FALSE);

	DWORD dwErr = GetLastError();
	return(dwErr == 0 || dwErr == 1460);
}

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
		if (!hIcon) {
			if (!SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, 
				SMTO_ABORTIFHUNG, HUNG_TIMEOUT, (PDWORD_PTR)&hIcon)) {
				DWORD dwErr = GetLastError();
				if (dwErr == 0 || dwErr == 1460)
					goto _HUNG_ICONSM;
			}
		}
		if (!hIcon) {
			hIcon = (HICON)(UINT_PTR)GetClassLongPtr(hwnd, GCLP_HICONSM);
		}
		if (hIcon) {
			*phIconSm = hIcon;
		} else {
_HUNG_ICONSM:
			*phIconSm = *phIcon;
		}
	}
}

//-----------------------------------------------------------

const float g_fl2 = (float)2.;

BOOL MyPrintWindow(HWND hwndPv, HDC hdc, const RECT* prcPv, const RECT* prcWork, DWORD dwFlags) {

	HRGN hrgnPv = NULL, hrgnOld = NULL;

	RECT rcWnd;
	GetWindowRect(hwndPv, &rcWnd);
	if (!(dwFlags & MPW_NOCHECKPOS)) {
		if (rcWnd.left > prcWork->right || rcWnd.top > prcWork->bottom ||
			rcWnd.right < prcWork->left || rcWnd.bottom < prcWork->top) return(FALSE);
	}

	rcWnd.right -= rcWnd.left;
	rcWnd.bottom -= rcWnd.top;
	if (rcWnd.right == 0 || rcWnd.bottom == 0)
		return(FALSE);

	float cxWork, cyWork;
	XFORM xForm;
	int tmp;

	tmp = prcWork->right - prcWork->left;
	__asm {
		fild tmp
		fstp cxWork
	}
	tmp = prcWork->bottom - prcWork->top;
	__asm {
		fild tmp
		fstp cyWork
	}
	__asm {
		fldz
		fst xForm.eM12
		fstp xForm.eM21
	}

	tmp = prcPv->right - prcPv->left;
	__asm {
		fild tmp
		fstp xForm.eM11
	}
	tmp = prcPv->bottom - prcPv->top;
	__asm {
		fild tmp
		fstp xForm.eM22
	}

	int leftPv = prcPv->left,
		topPv = prcPv->top;

	if (dwFlags & MPW_DESKTOP) {

		__asm {
			fld xForm.eM11
			fdiv cxWork
            fst xForm.eDy
			fimul int ptr rcWnd.left

			fiadd int ptr leftPv
			fstp xForm.eDx

			fld xForm.eDy
			fimul int ptr rcWnd.top
			fiadd int ptr topPv
			fstp xForm.eDy
		
			fld xForm.eM11
			fdiv cxWork
			fst xForm.eM11
			fstp xForm.eM22
		}
	} else {

		__asm {
			fld xForm.eM11 // xForm.eM11 * cyWork > xForm.eM22 * cxWork
			fmul cyWork
			fld xForm.eM22
			fmul cxWork
			fcompp
			fnstsw ax
			test ah,5
			jz _CALC_XFORM2

			fld xForm.eM22 // xForm.eM22 /= cyWork;
			fdiv cyWork
			fst xForm.eM22
			fstp xForm.eM11 // xForm.eM11 = xForm.eM22;
			jmp _CALC_XFORM_END

_CALC_XFORM2:
			fld xForm.eM11 // xForm.eM11 /= cxWork;
			fdiv cxWork
			fst xForm.eM11
			fstp xForm.eM22 // xForm.eM22 = xForm.eM11;

_CALC_XFORM_END:
		}
		//xForm.eDx = (float)prcPv->left + (float)(rcWnd.left - prcWork->left) * xForm.eM11;
		tmp = rcWnd.left - prcWork->left;
		__asm {
			fild tmp
			fmul xForm.eM11
			fiadd leftPv
			fstp xForm.eDx
		}
		if (dwFlags & MPW_HCENTER) {
			//xForm.eDx += ((float)(prcPv->right - prcPv->left) - cxWork * xForm.eM11) / (float)2.;
			tmp = prcPv->right - prcPv->left;
			__asm {
				fld xForm.eM11
				fmul cxWork
                fisubr tmp
                fdiv g_fl2
				fadd xForm.eDx
				fstp xForm.eDx
			}
		}
		//xForm.eDy = prcPv->top + (float)(rcWnd.top - prcWork->top) * xForm.eM22;
		tmp = rcWnd.top - prcWork->top;
		__asm {
			fild tmp
			fmul xForm.eM22
			fiadd topPv
			fstp xForm.eDy
		}
		if (dwFlags & MPW_VCENTER) {
			//xForm.eDy += ((float)(prcPv->bottom - prcPv->top) - cyWork * xForm.eM22) / (float)2.;
			tmp = prcPv->bottom - prcPv->top;
			__asm {
				fld xForm.eM22
				fmul cyWork
                fisubr tmp
                fdiv g_fl2
				fadd xForm.eDy
				fstp xForm.eDy
			}
		}		
	}

	hrgnPv = CreateRectRgn(prcPv->left, prcPv->top, prcPv->right, prcPv->bottom);
	if (!(dwFlags & MPW_NOREGION)) {
		HRGN hrgnWnd = GetPreviewRegion(hwndPv, &xForm);
		if (hrgnWnd) {
			CombineRgn(hrgnPv, hrgnPv, hrgnWnd, RGN_AND);
			DeleteObject(hrgnWnd);
		}
	}
	hrgnOld = (HRGN)SelectObject(hdc, hrgnPv);

	SetGraphicsMode(hdc, GM_ADVANCED);
	SetMapMode(hdc, MM_ANISOTROPIC);
	SetWorldTransform(hdc, &xForm);
	SetStretchBltMode(hdc, HALFTONE);

	/*LARGE_INTEGER liStart, liFreq, liCur;
	QueryPerformanceFrequency(&liFreq);
	QueryPerformanceCounter(&liStart);*/

	BOOL fRet = PrintWindow(hwndPv, hdc, 0);

	/*QueryPerformanceCounter(&liCur);
	WCHAR szBuff[128];
	wsprintf(szBuff, L"%I64d", ((liCur.QuadPart - liStart.QuadPart) * 1000) / liFreq.QuadPart);
	ReportError(szBuff);*/

	if (hrgnPv) {
		SelectObject(hdc, hrgnOld);
		DeleteObject(hrgnPv);
	}
	return(fRet);
}

//-----------------------------------------------------------

BOOL MySwitchToThisWindow(HWND hwnd) {

	if (!IsWindow(hwnd) /*|| !IsWindowVisible(hwnd)*/)
		return(FALSE);
	if (MyIsHungAppWindow(hwnd))
		return(FALSE);

	BOOL fSuccess = TRUE;

	HWND hwndFrgnd = GetForegroundWindow();
	if (hwnd != hwndFrgnd) {

		BringWindowToTop(hwnd);
		SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		if (!SetForegroundWindow(hwnd)) {

			if (!hwndFrgnd)
				hwndFrgnd = FindWindow(L"Shell_TrayWnd", NULL);

			DWORD idFrgnd = GetWindowThreadProcessId(hwndFrgnd, NULL);
			DWORD idSwitch = GetWindowThreadProcessId(hwnd, NULL);

			_RPT2(_CRT_WARN, "SetForegroundWindow1: idFrgnd = %d, idSwitch = %d\n", idFrgnd, idSwitch);

			AttachThreadInput(idFrgnd, idSwitch, TRUE);

			if (!SetForegroundWindow(hwnd)) {
				INPUT inp[4];
				ZeroMemory(&inp, sizeof(inp));
				inp[0].type = inp[1].type = inp[2].type = inp[3].type = INPUT_KEYBOARD;
				inp[0].ki.wVk = inp[1].ki.wVk = inp[2].ki.wVk = inp[3].ki.wVk = VK_MENU;
				inp[0].ki.dwFlags = inp[2].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
				inp[1].ki.dwFlags = inp[3].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
				SendInput(4, inp, sizeof(INPUT));

				fSuccess = SetForegroundWindow(hwnd);
			}

			AttachThreadInput(idFrgnd, idSwitch, FALSE);
		}

		//BringWindowToTop(hwnd);
		//SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	if (IsIconic(hwnd)) {
		/*DWORD dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
		if (dwExStyle & WS_EX_LAYERED) { // see "Bugs with layered windows" in TaskSwtichXP docs
			SetWindowLongPtr(hwnd, GWL_EXSTYLE, dwExStyle & ~WS_EX_LAYERED);
			//SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
		}*/
		//ShowWindow(hwnd, SW_RESTORE);
		PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
	}/* else {
		InvalidateRect(hwnd, NULL, FALSE);
	}*/

	return(fSuccess);
}

//-----------------------------------------------------------

BOOL MyPaintWallpaper(HDC hdc, const RECT* prcPv, const WALLPAPER* pwp) {

	BOOL fBmp = FALSE;

	HBITMAP hbitmap = (HBITMAP)LoadImage(NULL, pwp->szWallpaper, 
		IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (hbitmap) {
		BITMAP bmp;
		if (GetObject(hbitmap, sizeof(BITMAP), (PVOID)&bmp)) {

			HDC hdcBmp = CreateCompatibleDC(NULL);
			HBITMAP hbitmapOld = (HBITMAP)SelectObject(hdcBmp, hbitmap);
			//_RPT1(_CRT_WARN, "hbitmpaOld = %X\r\n", hbitmapOld);
			if (hbitmapOld) {
				int xBmp, yBmp, cxBmp, cyBmp;
				if (pwp->fTiled) {

					cxBmp = (bmp.bmWidth * (prcPv->right - prcPv->left)) / \
						(pwp->rcWallpaper.right - pwp->rcWallpaper.left);
					cyBmp = (bmp.bmHeight * (prcPv->bottom - prcPv->top)) / \
						(pwp->rcWallpaper.bottom - pwp->rcWallpaper.top);

					HDC hdcPt = CreateCompatibleDC(hdcBmp);
					HBITMAP hbitmapPt = CreateCompatibleBitmap(hdcBmp, cxBmp, cyBmp);
					if (hbitmapPt) {
						HBITMAP hbitmapPtOld = (HBITMAP)SelectObject(hdcPt, hbitmapPt);
						SetStretchBltMode(hdcPt, HALFTONE);
						StretchBlt(hdcPt, 0, 0, cxBmp, cyBmp, hdcBmp, 0, 0, 
							bmp.bmWidth, bmp.bmHeight, SRCCOPY);
						SelectObject(hdcPt, hbitmapPtOld);
					}
					DeleteDC(hdcPt);

					HBRUSH hbrPattern = CreatePatternBrush(hbitmapPt);
					if (hbrPattern) {
						fBmp = FillRect(hdc, prcPv, hbrPattern);
						DeleteObject(hbrPattern);
					}
					DeleteObject(hbitmapPt);

				} else {

					int cxScreen = GetSystemMetrics(SM_CXSCREEN),
						cyScreen = GetSystemMetrics(SM_CYSCREEN);

					int xPv, yPv, cxPv, cyPv;
					if (pwp->fStretched) {
						xPv = prcPv->left;
						yPv = prcPv->top;
						cxPv = prcPv->right - prcPv->left;
						cyPv = prcPv->bottom - prcPv->top;

						xBmp = (pwp->rcWallpaper.left * bmp.bmWidth) / cxScreen;
						cxBmp = bmp.bmWidth - ((cxScreen - \
							(pwp->rcWallpaper.right - pwp->rcWallpaper.left)) * bmp.bmWidth) / cxScreen;
						yBmp = (pwp->rcWallpaper.top * bmp.bmHeight) / cyScreen;
						cyBmp = bmp.bmHeight - ((cyScreen - \
							(pwp->rcWallpaper.bottom - pwp->rcWallpaper.top)) * bmp.bmHeight) / cyScreen;
					} else {

						FillRect(hdc, prcPv, GetSysColorBrush(COLOR_DESKTOP));

						if (bmp.bmWidth < pwp->rcWallpaper.right - pwp->rcWallpaper.left) {
							xBmp = 0;
							cxBmp = bmp.bmWidth;
							cxPv = (bmp.bmWidth * (prcPv->right - prcPv->left)) / \
								(pwp->rcWallpaper.right - pwp->rcWallpaper.left);
							xPv = (prcPv->left + prcPv->right - cxPv) / 2;								
						} else {
							cxBmp = pwp->rcWallpaper.right - pwp->rcWallpaper.left;
							xBmp = (bmp.bmWidth - cxBmp) / 2;
							cxPv = prcPv->right - prcPv->left;
							xPv = prcPv->left;
						}
						if (bmp.bmHeight < pwp->rcWallpaper.bottom - pwp->rcWallpaper.top) {
							yBmp = 0;
							cyBmp = bmp.bmHeight;
							cyPv = (bmp.bmHeight * (prcPv->bottom - prcPv->top)) / \
								(pwp->rcWallpaper.bottom - pwp->rcWallpaper.top);
							yPv = (prcPv->top + prcPv->bottom - cyPv) / 2;
						} else {
							cyBmp = pwp->rcWallpaper.bottom - pwp->rcWallpaper.top;
							yBmp = (bmp.bmHeight - cyBmp) / 2;
							cyPv = prcPv->bottom - prcPv->top;
							yPv = prcPv->top;
						}
					}
					int nStretchModeOld = SetStretchBltMode(hdc, HALFTONE);
					fBmp = StretchBlt(hdc, xPv, yPv, cxPv, cyPv,
						hdcBmp, xBmp, yBmp, cxBmp, cyBmp, SRCCOPY);
					SetStretchBltMode(hdc, nStretchModeOld);
				}
				SelectObject(hdcBmp, hbitmapOld);
			}
			DeleteDC(hdcBmp);
		}
		DeleteObject(hbitmap);
	}
	return(fBmp);
}

//-----------------------------------------------------------

BOOL MyTerminateProcess(DWORD dwProcessId) {
	BOOL fSuccess = FALSE;
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
	if (hProcess) {
		fSuccess = TerminateProcess(hProcess, (UINT)-1);
		CloseHandle(hProcess);
	}
	return(fSuccess);
}

//-----------------------------------------------------------
//-----------------------------------------------------------

void MyInsertionSort(PVOID pArr, int nNumEl, DWORD dwSizeEl, MYCOMPAREFUNC pfnCompareFunc) {

	PVOID pvTmp = HeapAlloc(GetProcessHeap(), 0, dwSizeEl);
	if (!pvTmp) return;

	for (int i = 1; i < nNumEl; i++) {
		int j = i - 1;
		while (j >= 0 && 
			pfnCompareFunc((PBYTE)pArr + j * dwSizeEl, (PBYTE)pArr + i * dwSizeEl) > 0)
			j--;
		j++;
		if (j != i) {
			CopyMemory(pvTmp, (PBYTE)pArr + i * dwSizeEl, dwSizeEl);
			MoveMemory((PBYTE)pArr + dwSizeEl * (j + 1), 
				(PBYTE)pArr + j * dwSizeEl, (i - j) * dwSizeEl);
			CopyMemory((PBYTE)pArr + j * dwSizeEl, pvTmp, dwSizeEl);
		}
	}
	HeapFree(GetProcessHeap(), 0, pvTmp);
}

//-----------------------------------------------------------
//-----------------------------------------------------------

HWND FindTrayToolbarWindow() {

	HWND hwndTray = FindWindow(L"Shell_TrayWnd", NULL);
	if (hwndTray) {
		hwndTray = FindWindowEx(hwndTray, NULL, L"TrayNotifyWnd", NULL);
		if (hwndTray) {
			hwndTray = FindWindowEx(hwndTray, NULL, L"SysPager", NULL);
			if (hwndTray) {				
				hwndTray = FindWindowEx(hwndTray, NULL, L"ToolbarWindow32", NULL);
			}
		}
	}
	return(hwndTray);
}

//-----------------------------------------------------------

BOOL AddTrayIcon(HWND hwnd, UINT uID, HICON hIcon, PCWSTR szTip) {

	NOTIFYICONDATA nid; 
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.hWnd = hwnd;
	nid.uID = uID;
	nid.uCallbackMessage = WM_MYTRAYMSG;
	nid.hIcon = hIcon;
	lstrcpyn(nid.szTip, szTip, SIZEOF_ARRAY(nid.szTip));

	return(Shell_NotifyIcon(NIM_ADD, &nid));
}

//-----------------------------------------------------------------

BOOL DeleteTrayIcon(HWND hwnd, UINT uID) {
	NOTIFYICONDATA nid; 
	nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = uID;
	return(Shell_NotifyIcon(NIM_DELETE, &nid));
}

//-----------------------------------------------------------

BOOL ReloadTrayIcon(HWND hwnd, UINT uID) {
	NOTIFYICONDATA nid; 
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uFlags = NIF_STATE;
    nid.hWnd = hwnd;
    nid.uID = uID;
	nid.dwStateMask = NIS_HIDDEN;
	nid.dwState = 0;
	return(Shell_NotifyIcon(NIM_MODIFY, &nid));
}

//-----------------------------------------------------------
