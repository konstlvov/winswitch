// WinHotkeyCtrl.cpp

#include "stdafx.h"
#include "vkCodes.h"
#include "WinHotkeyCtrl.h"

//-----------------------------------------------------------------

//namespace NWinHotkeyCtrl {

//-----------------------------------------------------------------

#define WM_KEY						(WM_USER + 444)

#define MAKEHKCDATA(vkCode, fModSet, fModRel, fIsPressed) \
	((DWORD)(((BYTE)((DWORD_PTR)(vkCode) & 0xff)) | \
	(((DWORD)((BYTE)((DWORD_PTR)(fModSet) & 0xff))) << 8) | \
	(((DWORD)((BYTE)((DWORD_PTR)(fModRel) & 0xff))) << 16) | \
	(((DWORD)((BYTE)((DWORD_PTR)(fIsPressed) & 0xff))) << 24)))

/*****************************************************************/
/*                                                               */
/*  DWORD dwHkcData = GetWindowLongPtr(hwndHkc, GWLP_USERDATA);  */
/*                                                               */
/*  LOBYTE(LOWORD(dwHkcData)) - key virtual code                 */
/*  HIBYTE(LOWORD(dwHkcData)) - modifiers set                    */
/*  LOBYTE(HIWORD(dwHkcData)) - modifiers released               */
/*  HIBYTE(HIWORD(dwHkcData)) - key is pressed(?)                */
/*                                                               */
/*****************************************************************/


LRESULT CALLBACK _WinHotkeyCtrlProc(HWND, UINT, WPARAM, LPARAM);
void _SetHkcText(HWND, DWORD);

//-----------------------------------------------------------------

WNDPROC _wpEditProc					= NULL;
HHOOK _hhookKb						= NULL;
HWND _hwndHkc						= NULL;

//-----------------------------------------------------------------

BOOL InitWinHotkeyCtrls() {
	
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	if (!GetClassInfoEx(GetModuleHandle(NULL), _T("Edit"), &wcex))
		return(FALSE);
	_wpEditProc = wcex.lpfnWndProc;

	return(TRUE);
}

//-----------------------------------------------------------------

BOOL SubClassWinHotkeyCtrl(HWND hwndHkc) {

	_ASSERT(hwndHkc);

	//_ASSERT(_wpEditProc);
	if (!_wpEditProc)
		if (!InitWinHotkeyCtrls())
			return(FALSE);
	
	SetWindowLongPtr(hwndHkc, GWLP_WNDPROC, (LONG)(LONG_PTR)(WNDPROC)_WinHotkeyCtrlProc);
	SetWindowLongPtr(hwndHkc, GWLP_USERDATA, (LONG)MAKEHKCDATA(0,0,0,0));
	_SetHkcText(hwndHkc, 0);

	return(TRUE);
}

//-----------------------------------------------------------------

LRESULT CALLBACK _LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT lRet = 0;
	if (nCode == HC_ACTION) {

		BOOL fAltDown = (BOOL)(GetAsyncKeyState(VK_MENU) & 0x8000),
			fCtrlDown = (BOOL)(GetAsyncKeyState(VK_CONTROL) & 0x8000),
			fShiftDown = (BOOL)(GetAsyncKeyState(VK_SHIFT) & 0x8000),
			fWinDown = (BOOL)(GetAsyncKeyState(VK_LWIN) & 0x8000 || GetAsyncKeyState(VK_RWIN) & 0x8000);

		UINT vkCode = ((PKBDLLHOOKSTRUCT)lParam)->vkCode;

		if ((vkCode == VK_DELETE && fCtrlDown && fAltDown && !fShiftDown) || 
			(vkCode == VK_TAB && fAltDown && !fCtrlDown && !fWinDown)) {
			PostMessage(_hwndHkc, WM_KEY, 0, 0);
		} else {
			PostMessage(_hwndHkc, WM_KEY, vkCode, (wParam & 1));
			if (!(wParam & 1) && vkCode != VK_LWIN && vkCode != VK_RWIN &&
				vkCode != VK_CONTROL && vkCode != VK_LCONTROL && vkCode != VK_RCONTROL && 
				vkCode != VK_SHIFT && vkCode != VK_LSHIFT && vkCode != VK_RSHIFT && 
				vkCode != VK_MENU && vkCode != VK_LMENU && vkCode != VK_RMENU) lRet = 1;
		}
	}
	return(lRet ? lRet : CallNextHookEx(_hhookKb, nCode, wParam, lParam));	
}

//-----------------------------------------------------------------

BOOL _UninstallKbHook() {
	BOOL fOk = FALSE;
	if (_hhookKb) {
		fOk = UnhookWindowsHookEx(_hhookKb);
		_hhookKb = NULL;
	}
	_hwndHkc = NULL;
	return(fOk);
}

//-----------------------------------------------------------------

BOOL _InstallKbHook(HWND hwndHkc) {

	if (_hhookKb)
		_UninstallKbHook();

	_hwndHkc = hwndHkc;

	_hhookKb = SetWindowsHookEx(WH_KEYBOARD_LL, 
		(HOOKPROC)_LowLevelKeyboardProc, GetModuleHandle(NULL), NULL);

	return(_hhookKb != NULL);
}

//-----------------------------------------------------------------

void _SetHkcText(HWND hwndHkc, DWORD dwHk) {

	TCHAR pszText[64];
	if (dwHk && HotkeyToString(dwHk, pszText, sizeof(pszText) / sizeof(pszText[0])))
		SetWindowText(hwndHkc, pszText);
	else SetWindowText(hwndHkc, _T("None"));
	SendMessage(hwndHkc, EM_SETSEL, 0x8fffffff, 0x8fffffff);
}

//-----------------------------------------------------------------

DWORD GetWinHotkey(HWND hwndHkc) {
	_ASSERT(hwndHkc);
	return(LOWORD(GetWindowLongPtr(hwndHkc, GWLP_USERDATA)));
}

//-----------------------------------------------------------------

void SetWinHotkey(HWND hwndHkc, DWORD dwHk) {

	UINT vkCode = LOBYTE(LOWORD(dwHk));
	UINT fModifiers = !vkCode ? 0 : HIBYTE(LOWORD(dwHk));
    DWORD dwHkcData = MAKEHKCDATA(vkCode, fModifiers, fModifiers, FALSE);
	SetWindowLongPtr(hwndHkc, GWLP_USERDATA, dwHkcData);
	_SetHkcText(hwndHkc, dwHkcData);
}

//-----------------------------------------------------------------

LRESULT CALLBACK _WinHotkeyCtrlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_KEY: {

			DWORD dwHkcData = (DWORD)GetWindowLongPtr(hwnd, GWLP_USERDATA);

			DWORD vkCode = LOBYTE(LOWORD(dwHkcData));
			DWORD fModSet = HIBYTE(LOWORD(dwHkcData));
			DWORD fModRel = LOBYTE(HIWORD(dwHkcData));
			BOOL fIsPressed = HIBYTE(HIWORD(dwHkcData));

			DWORD fMod = 0;
			BOOL fRedraw = TRUE;

			if (wParam == 0) { //clear
				fModSet = fModRel = 0;
				vkCode = 0;
			} else {

			switch (wParam) {
				case VK_LWIN:
				case VK_RWIN: fMod = MOD_WIN; break;
				case VK_CONTROL:
				case VK_LCONTROL:
				case VK_RCONTROL: fMod = MOD_CONTROL; break;
				case VK_MENU:
				case VK_LMENU: 
				case VK_RMENU: fMod = MOD_ALT; break;
				case VK_SHIFT:
				case VK_LSHIFT:
				case VK_RSHIFT: fMod = MOD_SHIFT; break;				
			}

			if (fMod) { // modifier
				if (!lParam) { // press
					if(!fIsPressed && vkCode) {
						fModSet = fModRel = 0;
						vkCode = 0;
					} 
					fModRel &= ~fMod;
				} else if (fModSet & fMod) // release
					fModRel |= fMod;

				if (fIsPressed || !vkCode) {
					if (!lParam) { // press
						if (!(fModSet & fMod)) { // new modifier
							fModSet |= fMod;
						} else
							fRedraw = FALSE;
					} else fModSet &= ~fMod;
				}
			} else { // another key

				if (wParam == VK_DELETE && fModSet == (MOD_CONTROL | MOD_ALT)) {
					fModSet = fModRel = 0; // skip "Ctrl + Alt + Del"
					vkCode = 0;
					fIsPressed = FALSE;
				} else if (wParam == vkCode && lParam) {
					fIsPressed = FALSE;
					fRedraw = FALSE;
				} else {
					if (!fIsPressed && !lParam) { // pressed a another key
						if (fModRel & fModSet) {
							fModSet = fModRel = 0;
						}
						vkCode = (DWORD)wParam;
						fIsPressed = TRUE;
					}
				}
			}
			}
			dwHkcData = MAKEHKCDATA(vkCode, fModSet, fModRel, fIsPressed);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)dwHkcData);

			if (fRedraw)
				_SetHkcText(hwnd, dwHkcData);

			return(0);
					 }

		case WM_SETFOCUS:
			_InstallKbHook(hwnd);
            break;

		case WM_KILLFOCUS:
			_UninstallKbHook();
            break;
            
		case WM_CONTEXTMENU: {

			UINT id;
			HMENU hmenu, hmenu2;
			hmenu = CreatePopupMenu();

			hmenu2 = CreatePopupMenu();
			for (id = VK_BROWSER_BACK; id <= VK_LAUNCH_APP2; id++)
				AppendMenu(hmenu2, MF_STRING, id, GetKeyName(id));
			AppendMenu(hmenu, MF_STRING | MF_POPUP, (UINT_PTR)hmenu2, _T("Multimedia"));

			hmenu2 = CreatePopupMenu();
			AppendMenu(hmenu2, MF_STRING, VK_RETURN, GetKeyName(VK_RETURN));
			AppendMenu(hmenu2, MF_STRING, VK_ESCAPE, GetKeyName(VK_ESCAPE));
			AppendMenu(hmenu2, MF_STRING, VK_TAB, GetKeyName(VK_TAB));
			AppendMenu(hmenu2, MF_STRING, VK_CAPITAL, GetKeyName(VK_CAPITAL));
			AppendMenu(hmenu2, MF_STRING, VK_BACK, GetKeyName(VK_BACK));
			AppendMenu(hmenu2, MF_STRING, VK_INSERT, GetKeyName(VK_INSERT));
			AppendMenu(hmenu2, MF_STRING, VK_DELETE, GetKeyName(VK_DELETE));
			for (id = VK_SPACE; id <= VK_DOWN; id++)
				AppendMenu(hmenu2, MF_STRING, id, GetKeyName(id));
			AppendMenu(hmenu, MF_STRING | MF_POPUP, (UINT_PTR)hmenu2, _T("Standard"));

			hmenu2 = CreatePopupMenu();
			for (id = VK_F1; id <= VK_F24; id++)
				AppendMenu(hmenu2, MF_STRING, id, GetKeyName(id));
			AppendMenu(hmenu, MF_STRING | MF_POPUP, (UINT_PTR)hmenu2, _T("Functionality"));


			DWORD dwHkcData = (DWORD)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			DWORD vkCode = LOBYTE(LOWORD(dwHkcData));
			DWORD fModSet = HIBYTE(LOWORD(dwHkcData));
			DWORD fModRel = LOBYTE(HIWORD(dwHkcData));
			BOOL fIsPressed = HIBYTE(HIWORD(dwHkcData));

			AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hmenu, (fModSet & MOD_WIN) ? 
				(MF_STRING | MF_CHECKED) : MF_STRING, VK_LWIN, _T("Win-key"));
			AppendMenu(hmenu, (fModSet & MOD_CONTROL) ? 
                (MF_STRING | MF_CHECKED) : MF_STRING, VK_CONTROL, _T("Control-key"));
			AppendMenu(hmenu, (fModSet & MOD_SHIFT) ? 
				(MF_STRING | MF_CHECKED) : MF_STRING, VK_SHIFT, _T("Shift-key"));
			AppendMenu(hmenu, (fModSet & MOD_ALT) ? 
				(MF_STRING | MF_CHECKED) : MF_STRING, VK_MENU, _T("Alt-key"));
			AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hmenu, (vkCode == 0) ? 
				(MF_STRING | MF_CHECKED) : MF_STRING, 0x1, _T("None"));
			
			UINT uMenuID = TrackPopupMenu(hmenu, 
				TPM_RIGHTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
				LOWORD(lParam), HIWORD(lParam), 0, hwnd, NULL);

			if (uMenuID && uMenuID < 256) {
				switch (uMenuID) {
					case VK_LWIN:
						if (vkCode) {
							fModSet ^= MOD_WIN;
							fModRel |= fModSet & MOD_WIN;
						}
						break;
					case VK_CONTROL:
						if (vkCode) {
							fModSet ^= MOD_CONTROL;
							fModRel |= fModSet & MOD_CONTROL;
						}
						break;
					case VK_SHIFT:
						if (vkCode) {
							fModSet ^= MOD_SHIFT;
							fModRel |= fModSet & MOD_SHIFT;
						}
						break;
					case VK_MENU:
						if (vkCode) {
							fModSet ^= MOD_ALT;
							fModRel |= fModSet & MOD_ALT;
						}
						break;

					case 0x1:
						vkCode = 0;
						fModSet = fModRel = 0;
						fIsPressed = FALSE;
						break;

					default:
						vkCode = uMenuID;
						fIsPressed = FALSE;
						break;
				}
				dwHkcData = MAKEHKCDATA(vkCode, fModSet, fModRel, fIsPressed);
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)dwHkcData);
				_SetHkcText(hwnd, dwHkcData);
				SetFocus(hwnd);
			}
			DestroyMenu(hmenu);			
			return(0);
							 }

		case WM_LBUTTONDBLCLK:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)0);
			_SetHkcText(hwnd, 0);
			break;

		case WM_CHAR:
			break;

		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
			return(0);

		case WM_DESTROY:
			if (hwnd == _hwndHkc)
				_UninstallKbHook();
			break;
	}
	return(CallWindowProc(_wpEditProc, hwnd, uMsg, wParam, lParam));
}

//-----------------------------------------------------------------

//} // namespace NWinHotkeyCtrl

//-----------------------------------------------------------------
