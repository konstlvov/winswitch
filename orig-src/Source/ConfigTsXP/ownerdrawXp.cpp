// ownerdrawXp.cpp

#include "stdafx.h"
#include "main.h"
#include "generic.h"
#include "ownerdrawXp.h"
#include "resource.h"

//-----------------------------------------------------------------

LRESULT CALLBACK _UxButtonProc(HWND, UINT, WPARAM, LPARAM);
BOOL LoadUxThemeDll();
void FreeUxThemeDll();


typedef HTHEME (WINAPI *OPENTHEMEDATA)(HWND, LPCWSTR);
typedef HRESULT (WINAPI *CLOSETHEMEDATA)(HTHEME);
typedef BOOL (WINAPI *ISTHEMEBACKGROUNDPARTIALLYTRANSPARENT)(HTHEME, int, int);
typedef HRESULT (WINAPI *DRAWTHEMEPARENTBACKGROUND)(HWND, HDC, RECT*);
typedef HRESULT (WINAPI *DRAWTHEMEBACKGROUND)(HTHEME, HDC, int, int, const RECT*, const RECT*);
typedef HRESULT (WINAPI *GETTHEMEBACKGROUNDCONTENTRECT)(HTHEME, HDC, int, int, const RECT*, RECT*);

OPENTHEMEDATA _pfnOpenThemeData													= NULL;
CLOSETHEMEDATA _pfnCloseThemeData												= NULL;
ISTHEMEBACKGROUNDPARTIALLYTRANSPARENT _pfnIsThemeBackgroundPartiallyTransparent	= NULL;
DRAWTHEMEPARENTBACKGROUND _pfnDrawThemeParentBackground							= NULL;
DRAWTHEMEBACKGROUND _pfnDrawThemeBackground										= NULL;
GETTHEMEBACKGROUNDCONTENTRECT _pfnGetThemeBackgroundContentRect					= NULL;
ENABLETHEMEDIALOGTEXTURE g_pfnEnableThemeDialogTexture							= NULL;

//-----------------------------------------------------------------

HINSTANCE _hinstUxThemeDll		= NULL;
HTHEME _hthemeBtn				= NULL;
HTHEME _hthemeTab				= NULL;
WNDPROC _wpBtnProc				= NULL;

//-----------------------------------------------------------------

BOOL LoadUxThemeDll(HWND hwnd) {

	_ASSERT(_hinstUxThemeDll == NULL);

	_hinstUxThemeDll = LoadLibrary(L"UxTheme.dll");

	if (_hinstUxThemeDll) {

		_pfnOpenThemeData = (OPENTHEMEDATA) \
			GetProcAddress(_hinstUxThemeDll, "OpenThemeData");
		if (_pfnOpenThemeData) {
			_hthemeBtn = (_pfnOpenThemeData)(hwnd, L"Button");
			_hthemeTab = (_pfnOpenThemeData)(hwnd, L"Tab");

			_pfnCloseThemeData = (CLOSETHEMEDATA) \
				GetProcAddress(_hinstUxThemeDll, "CloseThemeData");
			_pfnIsThemeBackgroundPartiallyTransparent = (ISTHEMEBACKGROUNDPARTIALLYTRANSPARENT) \
				GetProcAddress(_hinstUxThemeDll, "IsThemeBackgroundPartiallyTransparent");
			_pfnDrawThemeParentBackground = (DRAWTHEMEPARENTBACKGROUND) \
				GetProcAddress(_hinstUxThemeDll, "DrawThemeParentBackground");
			_pfnDrawThemeBackground = (DRAWTHEMEBACKGROUND) \
				GetProcAddress(_hinstUxThemeDll, "DrawThemeBackground");
			_pfnGetThemeBackgroundContentRect = (GETTHEMEBACKGROUNDCONTENTRECT) \
				GetProcAddress(_hinstUxThemeDll, "GetThemeBackgroundContentRect");
			g_pfnEnableThemeDialogTexture = (ENABLETHEMEDIALOGTEXTURE) \
				GetProcAddress(_hinstUxThemeDll, "EnableThemeDialogTexture");
		}

		if (!_pfnOpenThemeData || !_pfnCloseThemeData ||  
			!_pfnIsThemeBackgroundPartiallyTransparent ||
			!_pfnDrawThemeParentBackground || !_pfnDrawThemeBackground ||
			!_pfnGetThemeBackgroundContentRect || 
			!g_pfnEnableThemeDialogTexture || !_hthemeBtn) {
				FreeUxThemeDll();
			}
	}
	return(_hinstUxThemeDll != NULL);
}

//-----------------------------------------------------------------

void FreeUxThemeDll() {

	if (_hinstUxThemeDll) {
		if (_hthemeBtn) {
			(_pfnCloseThemeData)(_hthemeBtn);
			_hthemeBtn = NULL;
		}
		if (_hthemeTab) {
			(_pfnCloseThemeData)(_hthemeTab);
			_hthemeTab = NULL;
		}
		FreeLibrary(_hinstUxThemeDll);
		_hinstUxThemeDll = NULL;
	}
}

//-----------------------------------------------------------------

void InitOwnerDrawCtrls(HWND hwnd) {

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	GetClassInfoEx(g_hinstExe, WC_BUTTON, &wcex);
	_wpBtnProc = wcex.lpfnWndProc;
	
	LoadUxThemeDll(hwnd);
}

//-----------------------------------------------------------------

void UxThemeChanged(HWND hwnd) {

	if (_hthemeBtn) {
		(_pfnCloseThemeData)(_hthemeBtn);
		_hthemeBtn = NULL;
	}
	if (_hthemeTab) {
		(_pfnCloseThemeData)(_hthemeTab);
		_hthemeTab = NULL;
	}

	if (!_hinstUxThemeDll)
		LoadUxThemeDll(hwnd);
	else {
		if (_hinstUxThemeDll) {
			_hthemeBtn = (_pfnOpenThemeData)(hwnd, L"Button");
			_hthemeTab = (_pfnOpenThemeData)(hwnd, L"Tab");
		}
	}
}

//-----------------------------------------------------------------

void DrawMyPageTitleBk(const DRAWITEMSTRUCT *pdi) {

	RECT itemRect = pdi->rcItem;

	if (_hthemeTab) { // XP style

		_ASSERT(_pfnIsThemeBackgroundPartiallyTransparent &&
			_pfnDrawThemeParentBackground && _pfnDrawThemeBackground);

		if ((_pfnIsThemeBackgroundPartiallyTransparent)(_hthemeTab, TABP_PANE, 0))
			(_pfnDrawThemeParentBackground)(pdi->hwndItem, pdi->hDC, &itemRect);
		(_pfnDrawThemeBackground)(_hthemeTab, pdi->hDC, TABP_PANE, 0, &itemRect, NULL);

	} else {

		DrawFrameControl(pdi->hDC, &itemRect, DFC_BUTTON, DFCS_BUTTONPUSH);
	}
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

typedef struct UXBUTTON_DATA {
	DWORD dwType;
	LPARAM lParam1;
	LPARAM lParam2;
	BOOL fMouseOverBtn;
} *PUXBUTTON_DATA;

#define UXBTN_ICON16				1
#define UXBTN_COLOR					2

//-----------------------------------------------------------------

void SubClassUxIconButton(HWND hwndBtn, UINT idIcon, UINT idIconDisabled) {

	SetWindowLong(hwndBtn, GWL_STYLE, 
		GetWindowLong(hwndBtn, GWL_STYLE) | BS_OWNERDRAW);

	PUXBUTTON_DATA puxbd = new UXBUTTON_DATA;

	puxbd->dwType = UXBTN_ICON16;
	puxbd->fMouseOverBtn = FALSE;
	if (idIcon) {
		puxbd->lParam1 = (LPARAM)LoadIcon(g_hinstExe, MAKEINTRESOURCE(idIcon));
	} else puxbd->lParam1 = NULL;
	
	if (idIconDisabled) {
		puxbd->lParam2 = (LPARAM)LoadIcon(g_hinstExe, MAKEINTRESOURCE(idIconDisabled));
	} else puxbd->lParam2 = NULL;

	SetWindowLongPtr(hwndBtn, GWLP_USERDATA, (LONG)(LONG_PTR)puxbd);
	SetWindowLongPtr(hwndBtn, GWLP_WNDPROC, (LONG)(LONG_PTR)(WNDPROC)_UxButtonProc);
}

void SubClassUxColorButton(HWND hwndBtn, COLORREF cr) {

	SetWindowLong(hwndBtn, GWL_STYLE, 
		GetWindowLong(hwndBtn, GWL_STYLE) | BS_OWNERDRAW);

	PUXBUTTON_DATA puxbd = new UXBUTTON_DATA;

	puxbd->dwType = UXBTN_COLOR;
	puxbd->fMouseOverBtn = FALSE;
	puxbd->lParam1 = (LPARAM)cr;
	puxbd->lParam2 = NULL;
	
	SetWindowLongPtr(hwndBtn, GWLP_USERDATA, (LONG)(LONG_PTR)puxbd);
	SetWindowLongPtr(hwndBtn, GWLP_WNDPROC, (LONG)(LONG_PTR)(WNDPROC)_UxButtonProc);
}

//-----------------------------------------------------------------

void DrawUxButton(const DRAWITEMSTRUCT *pdi) {

	PUXBUTTON_DATA puxbd = (PUXBUTTON_DATA)(LONG_PTR)\
		GetWindowLongPtr(pdi->hwndItem, GWLP_USERDATA);
	_ASSERT(puxbd);

	int iStateId = PBS_NORMAL;
	RECT itemRect = pdi->rcItem;
	SetBkMode(pdi->hDC, TRANSPARENT);

	if (_hthemeBtn) { // XP style

		_ASSERT(_pfnIsThemeBackgroundPartiallyTransparent &&
			_pfnDrawThemeParentBackground && _pfnDrawThemeBackground);

		if (pdi->itemState & ODS_DISABLED)
			iStateId = PBS_DISABLED;
		else if (pdi->itemState & ODS_SELECTED)
			iStateId = PBS_PRESSED;
		else if (puxbd->fMouseOverBtn)
			iStateId = PBS_HOT;
		else if (pdi->itemState & ODS_FOCUS)
			iStateId = PBS_DEFAULTED;

		if ((_pfnIsThemeBackgroundPartiallyTransparent)(_hthemeBtn, BP_PUSHBUTTON, iStateId))
			(_pfnDrawThemeParentBackground)(pdi->hwndItem, pdi->hDC, &itemRect);
		(_pfnDrawThemeBackground)(_hthemeBtn, pdi->hDC, BP_PUSHBUTTON, iStateId, &itemRect, NULL);

	} else {

		if (pdi->itemState & ODS_FOCUS) {
			HBRUSH hbr = CreateSolidBrush(RGB(0,0,0));
			FrameRect(pdi->hDC, &itemRect, hbr);
			InflateRect(&itemRect, -1, -1);
			DeleteObject(hbr);
		}

		FillRect(pdi->hDC, &itemRect, GetSysColorBrush(COLOR_BTNFACE));
		
		if (pdi->itemState & ODS_SELECTED) { // draw pressed button
			HBRUSH hbrBtnShadow = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
			FrameRect(pdi->hDC, &itemRect, hbrBtnShadow);
			DeleteObject(hbrBtnShadow);
		} else { // draw non pressed button
			DrawFrameControl(pdi->hDC, &itemRect, DFC_BUTTON, 
				(puxbd->fMouseOverBtn ? DFCS_BUTTONPUSH | DFCS_HOT : DFCS_BUTTONPUSH));
			/* | ((pdi->itemState & ODS_SELECTED) ? DFCS_PUSHED : 0)*/
		}
	}

	switch (puxbd->dwType) {
	
		case UXBTN_ICON16: {

			const int cxDlgUnit = LOWORD(GetDialogBaseUnits()),
				cxSmIcon = 16,
				cySmIcon = 16;
			int x, y;
			int xIcon = -1;

			WCHAR szText[1024];
			if (GetWindowText(pdi->hwndItem, szText, SIZEOF_ARRAY(szText))) {
				SIZE sizeText;
				int n = 0;
				x = pdi->rcItem.left + cxSmIcon + cxDlgUnit + 6;
				if (pdi->rcItem.right - x - 6 > 0) {
					if (GetTextExtentExPoint(pdi->hDC, szText, lstrlen(szText), 
						pdi->rcItem.right - x - 6, &n, NULL, &sizeText)) {
						if (n > 0 && szText[n] != L'\0') {
							szText[n--] = L'\0';
							for (int i = 0; i < 3; i++) {
								if (n < 0) break;
								szText[n--] = L'.';
							}
						}
						sizeText.cx = min(sizeText.cx, pdi->rcItem.right - x - 6);
						x = x + (pdi->rcItem.right - x - 6 - sizeText.cx) / 2;
						xIcon = x - cxDlgUnit - cxSmIcon;
						y = pdi->rcItem.top + (pdi->rcItem.bottom - pdi->rcItem.top - sizeText.cy) / 2;
						if (pdi->itemState & ODS_SELECTED && !_hthemeBtn) {
							x++;
							y++;
						}
						DrawState(pdi->hDC, NULL, NULL, (LPARAM)szText, 0, 
							x, y, sizeText.cx, sizeText.cy, (pdi->itemState & ODS_DISABLED) 
							? DST_PREFIXTEXT | DSS_DISABLED : DST_PREFIXTEXT | DSS_NORMAL);
					}
				}
			}
			HICON hIcon = (pdi->itemState & ODS_DISABLED) 
				? (HICON)puxbd->lParam2 : (HICON)puxbd->lParam1;
			if (hIcon) {
				if (xIcon == -1) {
					xIcon = pdi->rcItem.left + (pdi->rcItem.right - pdi->rcItem.left - cxSmIcon) / 2;
				}
				y = pdi->rcItem.top + (pdi->rcItem.bottom - pdi->rcItem.top - cySmIcon) / 2;

				if (pdi->itemState & ODS_SELECTED && !_hthemeBtn) {
					xIcon++;
					y++;
				}
				DrawIconEx(pdi->hDC, xIcon, y, 
					hIcon, cxSmIcon, cySmIcon, 0, NULL, DI_NORMAL);
			}
			break;
						   }

		case UXBTN_COLOR: {

			if (!(pdi->itemState & ODS_DISABLED)) {
				RECT rc = pdi->rcItem;
				int dx = 2, dy = 2;
				if (_hthemeBtn) { // XP style
					_ASSERT(_pfnGetThemeBackgroundContentRect);
					_pfnGetThemeBackgroundContentRect(_hthemeBtn, pdi->hDC, BP_PUSHBUTTON, iStateId, &itemRect, &rc);
				} else {
					dx += GetSystemMetrics(SM_CXEDGE);
					dy += GetSystemMetrics(SM_CYEDGE);
				}
				InflateRect(&rc, -dx, -dy);

				if (pdi->itemState & ODS_SELECTED && !_hthemeBtn)
					OffsetRect(&rc, 1, 1);

				HBRUSH hbr = CreateSolidBrush((COLORREF)puxbd->lParam1);
				FillRect(pdi->hDC, &rc, hbr);
				DeleteObject(hbr);

				FrameRect(pdi->hDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
			}
			break;
						  }
		default:
			_ASSERT(FALSE);
	}

	if (pdi->itemState & ODS_FOCUS && !(pdi->itemState & ODS_NOFOCUSRECT)) {
		InflateRect(&itemRect, -3, -3);
		DrawFocusRect(pdi->hDC, &itemRect);
	}
}

//-----------------------------------------------------------------

LRESULT CALLBACK _UxButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_MOUSEMOVE: {
			PUXBUTTON_DATA puxbd = (PUXBUTTON_DATA)(LONG_PTR)\
				GetWindowLongPtr(hwnd, GWLP_USERDATA);
			_ASSERT(puxbd);

			if (!puxbd->fMouseOverBtn) {
				TRACKMOUSEEVENT tme;
				tme.cbSize      = sizeof(tme);
				tme.dwFlags     = TME_LEAVE;
				tme.hwndTrack   = hwnd;
				tme.dwHoverTime = 0;

				if (TrackMouseEvent(&tme)) {
					puxbd->fMouseOverBtn = TRUE;
					InvalidateRect(hwnd, NULL, TRUE);
				}
			}
			break;
						   }

		case WM_MOUSELEAVE: {
			PUXBUTTON_DATA puxbd = (PUXBUTTON_DATA)(LONG_PTR)\
				GetWindowLongPtr(hwnd, GWLP_USERDATA);
			_ASSERT(puxbd);
			puxbd->fMouseOverBtn = FALSE;
			InvalidateRect(hwnd, NULL, TRUE);
			break;
							}

		/*case WM_SETCURSOR: {
			HCURSOR hCursor = LoadCursor(NULL, IDC_HAND);
			if (!hCursor)
				hCursor = LoadCursor(NULL, IDC_ARROW);
			SetCursor(hCursor);
			return(TRUE);
			//break;
						   }*/
		
		case WM_LBUTTONDBLCLK:
			PostMessage(hwnd, WM_LBUTTONDOWN, wParam, lParam);
			return(0);

		case WM_DESTROY: {
            PUXBUTTON_DATA puxbd = (PUXBUTTON_DATA)(LONG_PTR)\
				GetWindowLongPtr(hwnd, GWLP_USERDATA);
			_ASSERT(puxbd);
			delete puxbd;
			break;
						 }
	}
	return(CallWindowProc(_wpBtnProc, hwnd, uMsg, wParam, lParam));
}

//-----------------------------------------------------------------

void SetUxButtonColor(HWND hwndBtn, COLORREF cr) {
	PUXBUTTON_DATA puxbd = (PUXBUTTON_DATA)(LONG_PTR)\
		GetWindowLongPtr(hwndBtn, GWLP_USERDATA);
	_ASSERT(puxbd);
	_ASSERT(puxbd->dwType == UXBTN_COLOR);
	puxbd->lParam1 = (LPARAM)cr;
	InvalidateRect(hwndBtn, NULL, TRUE);
}

COLORREF GetUxButtonColor(HWND hwndBtn) {
	PUXBUTTON_DATA puxbd = (PUXBUTTON_DATA)(LONG_PTR)\
		GetWindowLongPtr(hwndBtn, GWLP_USERDATA);
	_ASSERT(puxbd);
	_ASSERT(puxbd->dwType == UXBTN_COLOR);
	return(!puxbd ? 0 : (COLORREF)puxbd->lParam1);
}

//-----------------------------------------------------------------

UINT_PTR CALLBACK UxButtonCCHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
		case WM_INITDIALOG:
			//if (g_pfnEnableThemeDialogTexture)
			//	g_pfnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);
			CenterWindow(hwnd, GetParent(hwnd));
			break;
	}
	return(0);
}

BOOL UxButtonChooseColor(HWND hwndOwner, HWND hwndBtn) {

	static COLORREF s_pcrCustColors[16] = { 
		0xffffff, 0xffffff, 0xffffff, 0xffffff, 
		0xffffff, 0xffffff, 0xffffff, 0xffffff, 
		0xffffff, 0xffffff, 0xffffff, 0xffffff, 
		0xffffff, 0xffffff, 0xffffff, 0xffffff
	};

	CHOOSECOLOR cc = { 0 };				
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hwndOwner;
	cc.lpCustColors = s_pcrCustColors;
	cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ENABLEHOOK;
	cc.lpfnHook = UxButtonCCHookProc;

	cc.rgbResult = GetUxButtonColor(hwndBtn);

	BOOL fRet = ChooseColor(&cc);
	if (fRet)
		SetUxButtonColor(hwndBtn, cc.rgbResult);

	return(fRet);
}

/*
BOOL UxButtonShowMenu(HWND hwndOwner, HWND hwndBtn) {
		
	HMENU hmenu = CreatePopupMenu();
	AppendMenu(hmenu, MF_STRING, IDM_UXBTNDEFAULT, L"System color");
	AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hmenu, MF_STRING, IDM_UXBTNCUSTOM, L"Custom...");

	RECT rc;
	GetWindowRect(hwndBtn, &rc);

	SetForegroundWindow(hwndOwner);
	UINT uMenuID = TrackPopupMenu(hmenu, 
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
		rc.left, rc.bottom, 0, hwndOwner, NULL);
	PostMessage(hwndOwner, WM_NULL, 0, 0);

	DestroyMenu(hmenu);

	BOOL fRet = FALSE;

	switch (uMenuID) {
		case IDM_UXBTNDEFAULT:
			break;
		case IDM_UXBTNCUSTOM:
			fRet = UxButtonChooseColor(hwndOwner, hwndBtn);
			break;
	}
	return(fRet);
}
*/
//-----------------------------------------------------------------

BOOL DrawFontCombo(const LPDRAWITEMSTRUCT pdi) {

	BOOL fSuccess = FALSE;
    
	WCHAR szFontName[LF_FACESIZE];
	int len = ComboBox_GetLBTextLen(pdi->hwndItem, pdi->itemID);
	if (len > 0 && len < LF_FACESIZE) {
		if (ComboBox_GetLBText(pdi->hwndItem, pdi->itemID, szFontName) != CB_ERR) {

			HFONT hfont = CreateFont(-MulDiv(10, GetDeviceCaps(pdi->hDC, LOGPIXELSY), 72),
				0, 0, 0, FW_NORMAL, FALSE, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
				CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, szFontName);

			COLORREF crBkOld = SetBkColor(pdi->hDC, GetSysColor(
				(pdi->itemState & ODS_DISABLED) ? COLOR_3DFACE 
				: (pdi->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW));
			COLORREF crTextOld = SetTextColor(pdi->hDC, GetSysColor(
				(pdi->itemState & ODS_DISABLED) ? COLOR_GRAYTEXT 
				: ((pdi->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT)));

			HFONT hfontOld = (HFONT)SelectObject(pdi->hDC, 
				hfont ? hfont : GetStockObject(DEFAULT_GUI_FONT));

			TEXTMETRIC tm;
			GetTextMetrics(pdi->hDC, &tm);

			int x = LOWORD(GetDialogBaseUnits()) / 2,
				y = (pdi->rcItem.bottom + pdi->rcItem.top - tm.tmHeight) / 2;

			fSuccess = ExtTextOut(pdi->hDC, x, y, ETO_CLIPPED | ETO_OPAQUE, &pdi->rcItem, 
				szFontName, lstrlen(szFontName), NULL);

			if (pdi->itemState & ODS_SELECTED)
				DrawFocusRect(pdi->hDC, &pdi->rcItem);

			SelectObject(pdi->hDC, hfontOld);
			if (hfont)
				DeleteObject(hfont);
			SetBkColor(pdi->hDC, crBkOld);
			SetTextColor(pdi->hDC, crTextOld);
		}
	}
	return(fSuccess);
}