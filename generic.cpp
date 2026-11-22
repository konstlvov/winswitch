// generic.cpp

#include "stdafx.h"
#include <stdarg.h>
#include "main.h"
#include "generic.h"
#include "lang.h"
#include "resource.h"
#include "TaskSwitchXP.h"

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

void ReportErrorToDebug(LPCWSTR szError) {
	OutputDebugString(szError);
}

void ReportErrorToDebug(UINT uErrorID) {
	WCHAR szError[MAX_LANGLEN];
	LangLoadString(uErrorID, szError, MAX_LANGLEN);
	ReportErrorToDebug(szError);
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
	ReportErrorToDebug(szError);
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

	DWORD_PTR dw;
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

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT			0x00000002
#endif

#ifndef DWMWA_EXTENDED_FRAME_BOUNDS
#define DWMWA_EXTENDED_FRAME_BOUNDS		9
#endif

#define PV_SCORE_GRID					32
#define PV_SCORE_LOW					64
#define PV_MAX_CAPTURE_SIZE				4096

typedef struct PV_OSVERSIONINFO {
	ULONG dwOSVersionInfoSize;
	ULONG dwMajorVersion;
	ULONG dwMinorVersion;
	ULONG dwBuildNumber;
	ULONG dwPlatformId;
	WCHAR szCSDVersion[128];
} *PPV_OSVERSIONINFO;
typedef LONG (NTAPI *PV_RTLGETVERSION)(PPV_OSVERSIONINFO);

typedef struct PVSCORE {
	int nScore;
	int cUnique;
	int cNonBlack;
} PVSCORE;

typedef struct PVFRAME {
	HBITMAP hbm;
	BYTE* pBits;
	BOOL fOk;
	DWORD dwErr;
	DWORD dwTimeMs;
	DWORD dwFlags;
	int cxSrc;
	int cySrc;
	BOOL fFullContent;
	PVSCORE score;
} PVFRAME;

static BOOL PvIsWin81OrGreater() {

	static int s_fWin81 = -1;
	if (s_fWin81 >= 0)
		return((BOOL)s_fWin81);

	BOOL fWin81 = FALSE;
	// RtlGetVersion is not affected by the absence of a supportedOS manifest entry,
	// unlike GetVersionEx / VerifyVersionInfo (they would report 6.2 on Win10/11).
	HMODULE hmodNtdll = GetModuleHandle(L"ntdll.dll");
	if (hmodNtdll) {
		PV_RTLGETVERSION pfnRtlGetVersion = 
			(PV_RTLGETVERSION)GetProcAddress(hmodNtdll, "RtlGetVersion");
		if (pfnRtlGetVersion) {
			PV_OSVERSIONINFO vi = { sizeof(PV_OSVERSIONINFO) };
			if (pfnRtlGetVersion(&vi) >= 0)
				fWin81 = (vi.dwMajorVersion > 6 || 
					(vi.dwMajorVersion == 6 && vi.dwMinorVersion >= 3));
		}
	}
	s_fWin81 = fWin81;
	return(fWin81);
}

static BOOL PvGetWindowBounds(HWND hwndPv, RECT* prc, HRESULT* phrDwm) {

	HRESULT hrDwm = E_FAIL;
	RECT rcDwm = { 0 };
	HMODULE hmodDwm = LoadLibrary(L"dwmapi.dll");
	if (hmodDwm) {
		typedef HRESULT (WINAPI *PV_DWMGETWINDOWATTRIBUTE)(HWND, DWORD, PVOID, DWORD);
		PV_DWMGETWINDOWATTRIBUTE pfnDwm = (PV_DWMGETWINDOWATTRIBUTE)
			GetProcAddress(hmodDwm, "DwmGetWindowAttribute");
		if (pfnDwm)
			hrDwm = pfnDwm(hwndPv, DWMWA_EXTENDED_FRAME_BOUNDS, &rcDwm, sizeof(rcDwm));
		FreeLibrary(hmodDwm);
	}
	if (phrDwm)
		*phrDwm = hrDwm;

	if (SUCCEEDED(hrDwm) && rcDwm.right > rcDwm.left && rcDwm.bottom > rcDwm.top) {
		*prc = rcDwm;
		return(TRUE);
	}
	if (!GetWindowRect(hwndPv, prc))
		return(FALSE);
	return(prc->right > prc->left && prc->bottom > prc->top);
}

static void PvDebugLog(LPCWSTR szFmt, ...) {

	if (g_dwPvCaptureMode != PVCAP_DIAGNOSTIC)
		return;

	WCHAR szBuf[1024];
	va_list args;
	va_start(args, szFmt);
	_vsnwprintf_s(szBuf, SIZEOF_ARRAY(szBuf), _TRUNCATE, szFmt, args);
	va_end(args);
	OutputDebugString(szBuf);
}

static void PvLogWindowInfo(HWND hwndPv) {

	DWORD pid = 0;
	GetWindowThreadProcessId(hwndPv, &pid);
	WCHAR szProc[MAX_PATH] = L"?";
	HANDLE hproc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hproc) {
		GetModuleBaseName(hproc, NULL, szProc, SIZEOF_ARRAY(szProc));
		CloseHandle(hproc);
	}

	RECT rcWnd = { 0 }, rcNative = { 0 };
	HRESULT hrDwm = E_FAIL;
	GetWindowRect(hwndPv, &rcWnd);
	BOOL fDwmBounds = PvGetWindowBounds(hwndPv, &rcNative, &hrDwm);
	LPCWSTR pszBounds = fDwmBounds ? L"dwm" : L"wnd";

	PvDebugLog(L"PvCapture: hwnd=%p pid=%u proc=%s iconic=%d visible=%d "
		L"rect=(%d,%d)-(%d,%d) native=(%d,%d)-(%d,%d) nativeSize=%dx%d bounds=%s dwmhr=0x%08lX\n",
		(void*)hwndPv, pid, szProc, IsIconic(hwndPv), IsWindowVisible(hwndPv),
		rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom,
		rcNative.left, rcNative.top, rcNative.right, rcNative.bottom,
		rcNative.right - rcNative.left, rcNative.bottom - rcNative.top,
		pszBounds, (ULONG)hrDwm);
}

static PVSCORE PvScoreFrame(const BYTE* pBits, int cx, int cy) {

	PVSCORE score = { 0, 0, 0 };
	BYTE seen[4096] = { 0 }; // 32768 bits for 5-bit-per-channel RGB

	int sx = MIN(PV_SCORE_GRID, cx), sy = MIN(PV_SCORE_GRID, cy);
	for (int iy = 0; iy < sy; iy++) {
		int y = (sy == 1) ? (cy / 2) : (iy * (cy - 1) / (sy - 1));
		const BYTE* pRow = pBits + (size_t)y * cx * 4;
		for (int ix = 0; ix < sx; ix++) {
			int x = (sx == 1) ? (cx / 2) : (ix * (cx - 1) / (sx - 1));
			const BYTE* p = pRow + x * 4;
			int r = p[2], g = p[1], b = p[0];
			int idx = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
			if (!(seen[idx >> 3] & (1 << (idx & 7)))) {
				seen[idx >> 3] |= (BYTE)(1 << (idx & 7));
				score.cUnique++;
			}
			if (r + g + b > 24)
				score.cNonBlack++;
		}
	}
	score.nScore = score.cUnique * 16 + score.cNonBlack * 2;
	return(score);
}

static BOOL PvSetTempDcMode(HDC hdcTemp, BOOL fFullContent, const XFORM* pxForm) {

	XFORM xfId = { 1, 0, 0, 1, 0, 0 };

	if (fFullContent) {
		// PW_RENDERFULLCONTENT ignores the GDI world transform, so leave GM_ADVANCED
		// cleanly and give it a plain device-pixel DC.
		if (GetGraphicsMode(hdcTemp) == GM_ADVANCED &&
			!SetWorldTransform(hdcTemp, &xfId))
			return(FALSE);
		if (!SetGraphicsMode(hdcTemp, GM_COMPATIBLE))
			return(FALSE);
		if (!SetMapMode(hdcTemp, MM_TEXT))
			return(FALSE);
		SetWindowOrgEx(hdcTemp, 0, 0, NULL);
		SetViewportOrgEx(hdcTemp, 0, 0, NULL);
	} else {
		// Legacy PrintWindow honours xfTemp, which maps the window into the preview.
		if (!SetGraphicsMode(hdcTemp, GM_ADVANCED))
			return(FALSE);
		if (!SetMapMode(hdcTemp, MM_ANISOTROPIC))
			return(FALSE);
		SetWindowExtEx(hdcTemp, 1, 1, NULL);
		SetViewportExtEx(hdcTemp, 1, 1, NULL);
		SetWindowOrgEx(hdcTemp, 0, 0, NULL);
		SetViewportOrgEx(hdcTemp, 0, 0, NULL);
		if (!SetWorldTransform(hdcTemp, pxForm))
			return(FALSE);
	}
	SetStretchBltMode(hdcTemp, HALFTONE);
	return(TRUE);
}

static BOOL PvResetDcForBlit(HDC hdc, DWORD* pdwErr) {

	XFORM xfId = { 1, 0, 0, 1, 0, 0 };
	if (pdwErr)
		*pdwErr = 0;

	if (GetGraphicsMode(hdc) == GM_ADVANCED && !SetWorldTransform(hdc, &xfId)) {
		if (pdwErr)
			*pdwErr = GetLastError();
		return(FALSE);
	}
	if (!SetGraphicsMode(hdc, GM_COMPATIBLE)) {
		if (pdwErr)
			*pdwErr = GetLastError();
		return(FALSE);
	}
	if (!SetMapMode(hdc, MM_TEXT)) {
		if (pdwErr)
			*pdwErr = GetLastError();
		return(FALSE);
	}
	SetWindowOrgEx(hdc, 0, 0, NULL);
	SetViewportOrgEx(hdc, 0, 0, NULL);
	return(TRUE);
}

static BOOL PvPrintWindowCascade(HWND hwndPv, HDC hdc, const RECT* prcPv, const XFORM* pxForm) {

	// prcPv is already in device pixels of the preview bitmap (the preview DC is
	// MM_TEXT, and xForm maps window coordinates into that space).
	RECT rcDst = *prcPv;
	int cxBmp = rcDst.right - rcDst.left, cyBmp = rcDst.bottom - rcDst.top;
	if (cxBmp <= 0 || cyBmp <= 0 || 
		cxBmp > PV_MAX_CAPTURE_SIZE || cyBmp > PV_MAX_CAPTURE_SIZE) {
		PvDebugLog(L"PvCapture: invalid capture size %dx%d, legacy only\n", cxBmp, cyBmp);
		return(PrintWindow(hwndPv, hdc, 0));
	}

	// Shift the same window -> preview transform so that prcPv's top-left corner
	// maps to (0,0) in the temporary bitmap.
	XFORM xfTemp = *pxForm;
	xfTemp.eDx -= (FLOAT)rcDst.left;
	xfTemp.eDy -= (FLOAT)rcDst.top;

	HDC hdcTemp = CreateCompatibleDC(hdc);
	if (!hdcTemp)
		return(PrintWindow(hwndPv, hdc, 0));

	RECT rcNative = { 0 };
	HRESULT hrDwm = E_FAIL;
	BOOL fHaveNative = PvGetWindowBounds(hwndPv, &rcNative, &hrDwm);
	int cxNative = rcNative.right - rcNative.left;
	int cyNative = rcNative.bottom - rcNative.top;

	if (g_dwPvCaptureMode == PVCAP_DIAGNOSTIC)
		PvLogWindowInfo(hwndPv);

	LARGE_INTEGER liFreq = { 0 }, liStart = { 0 }, liEnd = { 0 };
	QueryPerformanceFrequency(&liFreq);

	PVFRAME frames[3];
	ZeroMemory(frames, sizeof(frames));
	DWORD dwCandidates[3];
	int cCandidates = 0;
	if (PvIsWin81OrGreater()) {
		dwCandidates[cCandidates++] = PW_RENDERFULLCONTENT;
		if (g_dwPvCaptureMode == PVCAP_DIAGNOSTIC)
			dwCandidates[cCandidates++] = PW_RENDERFULLCONTENT | PW_CLIENTONLY;
	}
	dwCandidates[cCandidates++] = 0; // legacy candidate is always the last one

	if (!fHaveNative && cCandidates > 1) {
		PvDebugLog(L"PvCapture: native bounds unavailable hr=0x%08lX err=%u, "
			L"full-content candidates skipped\n", (ULONG)hrDwm, GetLastError());
	}

	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	for (int i = 0; i < cCandidates; i++) {
		BOOL fFullContent = (dwCandidates[i] & PW_RENDERFULLCONTENT) != 0;
		if (fFullContent && !fHaveNative) {
			PvDebugLog(L"PvCapture: candidate[%d] skipped: no native size\n", i);
			continue;
		}
		int cxSrc = fFullContent ? cxNative : cxBmp;
		int cySrc = fFullContent ? cyNative : cyBmp;

		if (!PvSetTempDcMode(hdcTemp, fFullContent, &xfTemp)) {
			PvDebugLog(L"PvCapture: candidate[%d] DC setup failed err=%u\n", i, GetLastError());
			continue;
		}

		bi.bmiHeader.biWidth = cxSrc;
		bi.bmiHeader.biHeight = cySrc;
		BYTE* pBits = NULL;
		HBITMAP hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (PVOID*)&pBits, NULL, 0);
		if (!hbm)
			continue;
		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcTemp, hbm);
		if (!hbmOld) {
			DeleteObject(hbm);
			continue;
		}
		if (pBits)
			ZeroMemory(pBits, (size_t)cxSrc * cySrc * 4);

		SetLastError(0);
		QueryPerformanceCounter(&liStart);
		BOOL fOk = PrintWindow(hwndPv, hdcTemp, dwCandidates[i]);
		DWORD dwErr = GetLastError();
		QueryPerformanceCounter(&liEnd);

		PVSCORE score = { 0, 0, 0 };
		if (fOk && pBits)
			score = PvScoreFrame(pBits, cxSrc, cySrc);

		DWORD dwMs = (liFreq.QuadPart && liEnd.QuadPart >= liStart.QuadPart)
			? (DWORD)((liEnd.QuadPart - liStart.QuadPart) * 1000 / liFreq.QuadPart) : 0;
		PVFRAME* pFrame = &frames[i];
		pFrame->hbm = hbm;
		pFrame->pBits = pBits;
		pFrame->fOk = fOk;
		pFrame->dwErr = fOk ? 0 : dwErr;
		pFrame->dwTimeMs = dwMs;
		pFrame->dwFlags = dwCandidates[i];
		pFrame->cxSrc = cxSrc;
		pFrame->cySrc = cySrc;
		pFrame->fFullContent = fFullContent;
		pFrame->score = score;

		PvDebugLog(L"PvCapture: candidate[%d] flags=0x%08X ok=%d err=%u full=%d "
			L"src=%dx%d preview=%dx%d time=%ums score=%d unique=%d nonblack=%d\n",
			i, dwCandidates[i], fOk, pFrame->dwErr, fFullContent, cxSrc, cySrc,
			cxBmp, cyBmp, dwMs, score.nScore, score.cUnique, score.cNonBlack);

		SelectObject(hdcTemp, hbmOld);
	}

	int iLegacy = cCandidates - 1;
	// Start with legacy so that an exact score tie keeps the original frame.
	int iBest = (frames[iLegacy].hbm && frames[iLegacy].fOk) ? iLegacy : -1;
	for (int i = 0; i < cCandidates; i++) {
		if (!frames[i].hbm || !frames[i].fOk)
			continue;
		if (iBest < 0 || frames[i].score.nScore > frames[iBest].score.nScore)
			iBest = i;
	}
	// Equally poor frames (a legitimately dark window, for example) go with legacy.
	if (iBest >= 0 && iBest != iLegacy && frames[iLegacy].fOk && 
		frames[iBest].score.nScore < PV_SCORE_LOW &&
		frames[iLegacy].score.nScore < PV_SCORE_LOW)
		iBest = iLegacy;

	BOOL fRet = FALSE;
	if (iBest >= 0) {
		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcTemp, frames[iBest].hbm);
		int nSave = SaveDC(hdc);
		if (!nSave) {
			PvDebugLog(L"PvCapture: SaveDC failed err=%u, retrying legacy\n", GetLastError());
		} else {
			DWORD dwErrReset = 0;
			if (!PvResetDcForBlit(hdc, &dwErrReset) ||
				!PvResetDcForBlit(hdcTemp, &dwErrReset)) {
				PvDebugLog(L"PvCapture: reset DC for blit failed err=%u, retrying legacy\n", dwErrReset);
			} else if (frames[iBest].fFullContent) {
				if (!SetStretchBltMode(hdc, HALFTONE)) {
					PvDebugLog(L"PvCapture: SetStretchBltMode failed err=%u, retrying legacy\n", GetLastError());
				} else if (StretchBlt(hdc, rcDst.left, rcDst.top, cxBmp, cyBmp,
					hdcTemp, 0, 0, frames[iBest].cxSrc, frames[iBest].cySrc, SRCCOPY)) {
					PvDebugLog(L"PvCapture: selected flags=0x%08X score=%d src=%dx%d preview=%dx%d\n",
						frames[iBest].dwFlags, frames[iBest].score.nScore,
						frames[iBest].cxSrc, frames[iBest].cySrc, cxBmp, cyBmp);
					fRet = TRUE;
				} else {
					PvDebugLog(L"PvCapture: StretchBlt failed err=%u, retrying legacy\n", GetLastError());
				}
			} else {
				if (BitBlt(hdc, rcDst.left, rcDst.top, cxBmp, cyBmp, hdcTemp, 0, 0, SRCCOPY)) {
					PvDebugLog(L"PvCapture: selected flags=0x%08X score=%d src=%dx%d preview=%dx%d\n",
						frames[iBest].dwFlags, frames[iBest].score.nScore,
						frames[iBest].cxSrc, frames[iBest].cySrc, cxBmp, cyBmp);
					fRet = TRUE;
				} else {
					PvDebugLog(L"PvCapture: BitBlt failed err=%u, retrying legacy\n", GetLastError());
				}
			}
			RestoreDC(hdc, nSave);
		}
		if (hbmOld)
			SelectObject(hdcTemp, hbmOld);
		if (!fRet) {
			fRet = PrintWindow(hwndPv, hdc, 0);
			PvDebugLog(L"PvCapture: selected flags=0x00000000 (legacy retry) ok=%d\n", fRet);
		}
	} else {
		PvDebugLog(L"PvCapture: legacy only (no successful candidate)\n");
	}

	for (int i = 0; i < cCandidates; i++) {
		if (frames[i].hbm)
			DeleteObject(frames[i].hbm);
	}
	DeleteDC(hdcTemp);
	return(fRet);
}

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

	float cxWork = (float)(prcWork->right - prcWork->left);
	float cyWork = (float)(prcWork->bottom - prcWork->top);

	XFORM xForm = { 0 };
	xForm.eM11 = (float)(prcPv->right - prcPv->left);
	xForm.eM22 = (float)(prcPv->bottom - prcPv->top);

	int leftPv = prcPv->left,
		topPv = prcPv->top;

	if (dwFlags & MPW_DESKTOP) {

		FLOAT flScale = xForm.eM11 / cxWork;
		xForm.eM11 = flScale;
		xForm.eM22 = flScale;
		xForm.eDx = flScale * (FLOAT)rcWnd.left + (FLOAT)leftPv;
		xForm.eDy = flScale * (FLOAT)rcWnd.top + (FLOAT)topPv;

	} else {

		if (xForm.eM11 * cyWork > xForm.eM22 * cxWork) {
			xForm.eM22 /= cyWork;
			xForm.eM11 = xForm.eM22;
		} else {
			xForm.eM11 /= cxWork;
			xForm.eM22 = xForm.eM11;
		}
		xForm.eDx = (FLOAT)(rcWnd.left - prcWork->left) * xForm.eM11 + (FLOAT)leftPv;
		if (dwFlags & MPW_HCENTER) {
			xForm.eDx += ((FLOAT)(prcPv->right - prcPv->left) - cxWork * xForm.eM11) / 2.f;
		}
		xForm.eDy = (FLOAT)(rcWnd.top - prcWork->top) * xForm.eM22 + (FLOAT)topPv;
		if (dwFlags & MPW_VCENTER) {
			xForm.eDy += ((FLOAT)(prcPv->bottom - prcPv->top) - cyWork * xForm.eM22) / 2.f;
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

	BOOL fRet = FALSE;
	if ((dwFlags & MPW_TASKPREVIEW) && g_dwPvCaptureMode != PVCAP_LEGACY) {
		fRet = PvPrintWindowCascade(hwndPv, hdc, prcPv, &xForm);
	} else {
		// Background/taskbar captures and legacy mode keep the original call.
		fRet = PrintWindow(hwndPv, hdc, 0);
	}

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
