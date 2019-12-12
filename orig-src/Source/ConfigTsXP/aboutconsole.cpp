// aboutconsole.cpp

#include "stdafx.h"
#include "main.h"

//---------------------------------------------------------------

#define CX_CONSOLE_BORDER			10
#define CY_CONSOLE_BORDER			10
#define TIMER_ABOUTCONSOLE			1
#define CONSOLE_TEXT				0xCCCCCC
#define CONSOLE_HIGHLIGHT			0xFFFFFF


LRESULT CALLBACK Cls_AboutConsoleProc(HWND, UINT, WPARAM, LPARAM);

//---------------------------------------------------------------

WCHAR _szAboutConsoleClass[]	= L"_As12__AboutConsole_";

int _cxChar						= 0;
int _cyChar						= 0;

int _nCurCharX					= -1;
int _nCurLine					= 0;

int _nCharIndex					= -1;

int _cxConsole					= 0;
int _cyConsole					= 0;

COLORREF _crConsole				= CONSOLE_TEXT;

HFONT _hfontConsole				= NULL;
HBITMAP _hbmConsole				= NULL;

//-----------------------------------------------------------------

WCHAR _szAbout[] = L"\\hTaskSwitchXP Pro 2.0\\u (build 11)\\n"
				   L"The application switching revolution...\\n"
				   L"Copyright (C) 2006 by \\hAlexander Avdonin\\u\\n"
				   L"\\s----------------------------------------\\s\\n\\n"
				   L"Compiled on "
				   __WDATE__
                   L", at "
				   __WTIME__
				   L"\\n"
				   L"\\n\\n\\hCREDITS:\\u\\n\\n"
				   L"Coding, design, graphics and support:\\n"
				   L"\\t\\hAlexander Avdonin\\u\\n\\n"
				   L"TaskSwitchXP Pro 2.0 uses:\\n"
				   L"\\t\\hFOOOD's Icons\\u  <\\hhttp://www.foood.net\\u>\\n"
				   L"\\t\\hUPX\\u  <\\hhttp://upx.sourceforge.net\\u>\\n"
				   L"\\t\\hNSIS\\u  <\\hhttp://nsis.sourceforge.net\\u>\\n\\n"
				   L"Special thanks to:\\n"
				   L"\\t\\hSourceForge.net team\\u\\n"
				   L"\\t\\hIvan Pivovarov\\u\\n"
				   L"\\t\\hKonstantin Kuleshevich\\u\\n"
				   L"\\t\\hAndre Pinheiro\\u\\n"
				   L"\\t\\hJason Schmidt\\u\\n\\n"
				   L"Beta testing:\\n"
				   L"\\t\\hLuke Hamburg\\u\\n"
                   L"\\t\\hDavid C. Uhrig\\u\\n"
				   L"\\t\\hEmanuel Teixeira\\u\\n"
				   L"\\t\\hVladimir Kopjov\\u\\n"
				   L"\\t\\hEugene Bajda\\u aka \\hLocker\\u\\n"
				   L"\\t\\hAlexander Alferowich\\u\\n"
				   L"\\t\\hSebastian Schuberth\\u\\n"
				   L"\\t\\hSergey Dindikov\\u aka \\hZeroes\\u\\n"
				   L"\\t\\hH@rRy HU4nG\\u aka \\hTTrate\\u\\n"
				   L"\\t\\hJerome Glatigny\\u aka \\hDarkSage\\u\\n"
				   L"\\t\\hMichael Seibt\\u\\n\\n"
				   //L"\\t<< ...here may be \\hyour name\\u... >>\\u\\n\\n"
				   L"Translations:\\n"
				   L"\\t\\hOrkhan Guliyev\\u\\n"						// Azeri
				   L"\\t\\hAleg Azarousky\\u\\n"						// Byelorussian
				   L"\\t\\hPoorlyte\\u\\n"								// Brazilian Portuguese
				   L"\\t\\hRuben Sanabra Ramos\\u\\n"					// Catalan
				   L"\\t\\hLoBid\\u, \\hBanxu\\u, \\hkluivert\\u\\n"	// Simp. Chinese
				   L"\\t\\hH@rRy HU4nG\\u aka \\hTTrate\\u\\n"			// Trad. Chinese
				   L"\\t\\hIwo Jimy\\u\\n"								// Croatian
				   L"\\t\\hGeralt\\u & \\hMerlin\\u\\n"					// Czech
				   L"\\t\\hCarsten Winkler\\u\\n"						// Danish
				   L"\\t\\hOskari Lavinto\\u\\n"						// Finnish
				   L"\\t\\hJerome Glatigny\\u aka \\hDarkSage\\u\\n"	// French
				   L"\\t\\hWildbear\\u\\n"
				   L"\\t\\hMarkus Schuster\\u aka \\heVo\\u\\n"			// German
				   L"\\t\\hWasilis Mandratzis\\u - \\hWalz\\u\\n"		// Greek
				   L"\\t\\hNachoum Abramovitch\\u\\n"					// Hebrew
				   L"\\t\\hSergio Novella\\u\\n"						// Italian
				   L"\\t\\hDaisuke\\u\\n"								// Japanese
				   L"\\t\\hBac\\u\\n"									// Korean
				   L"\\t\\hZsolti\\u, \\hemotions\\u\\n"				// Hungarian (Magyar)
				   L"\\t\\hPintSki\\u\\n"								// Nederlands (Dutch)
				   L"\\t\\hOivind Marman\\u\\n"							// Norvegian
				   L"\\t\\hToszcze\\u\\n"								// Polish
				   L"\\t\\hAlexandru Eftimie\\u\\n"						// Roamnian
				   L"\\t\\hOzzii\\u\\n"									// Serbian
				   L"\\t\\hPeter Vavro <Gudas>\\u\\n"					// Slovak
				   L"\\t\\hJadran Rudec\\u\\n"							// Slovenian
				   L"\\t\\hAlejandro Munoz Uribe\\u\\n"					// Spanish
				   L"\\t\\hAlvaro Quezada Arce\\u\\n"					// Spanish LA
				   L"\\t\\hKenan Balamir\\u\\n"							// Turkish
				   L"\\t\\hKela\\u\\n"									// Ukrainian
				   L"\\t\\hAndriy Vankovych\\u aka \\h|W E R T|\\u\\n"	
				   L"\\n"
				   L"--\\n"
				   L"\\hTASKSWITCHXP PRO 2.0 IS SUPPLIED \"AS IS\".\\n"
				   L"THE AUTHOR ASSUMES NO LIABILITY FOR\\n"
				   L"DAMAGES, DIRECT OR CONSEQUENTIAL, WHICH\\n"
				   L"MAY RESULT FROM THE USE OF PROGRAM.\\u\\n\\n\\n"
				   L"\\t\\t  Alexander Avdonin, "
				   __WDATE__
				   L"\\n"
				   L"\\t\\t  Saint Petersburg, Russia\\d\\d\\d\\d\\d\\d\\d\\d"
				   L"\\c\\sC:\\Windows>\\s\\d\\d\\d\\d\\hformat c:\\u\\d\\d\\n\\n"
				   L"\\h\\sWARNING:\\s\\u\\s ALL DATA ON NON-REMOVABLE DISK\\s\\n"
				   L"\\sDRIVE C: WILL BE LOST!\\s\\n\\n\\d\\d"
				   L"\\sProceed with Format (Y/N)? \\s\\d\\d\\d\\d\\d\\d\\hYES\\u\\d\\d\\n\\n"
				   L"\\sFormating progress\\s\\d\\d......................\\n"
				   L"........................................\\n"
				   L"........................................\\d\\d\\d\\n\\n"
				   L"\\sFormat complete!\\s\\n\\n\\n\\d\\d\\d\\d"
				   L"\\sC:\\>\\s\\d\\d\\d\\d\\d\\d\\d";

//---------------------------------------------------------------

HWND CreateAboutConsole(HWND hwndParent, const RECT* prc, UINT uID =0) {

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)Cls_AboutConsoleProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = g_hinstExe;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _szAboutConsoleClass;
	wcex.hIconSm = NULL;

	if (!RegisterClassEx(&wcex))
		return(NULL);

	return(CreateWindowEx(WS_EX_STATICEDGE, _szAboutConsoleClass, L"", WS_CHILD | WS_VISIBLE, 
		prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, 
		hwndParent, (HMENU)(UINT_PTR)uID, g_hinstExe, NULL));
}

//---------------------------------------------------------------

void _NewLine(HDC hdcMem) {
	
	_nCurCharX = -1;
	_nCurLine++;
	
	int tmp = (_cyConsole - 2 * CY_CONSOLE_BORDER) / _cyChar - 1;
	if (_nCurLine > tmp) {

		// scroll line
		_nCurLine = tmp;

		RECT rc, rcUpdate;
		rc.left = 0;
		rc.top = CY_CONSOLE_BORDER;
		rc.right = _cxConsole;
		rc.bottom = _cyConsole - CY_CONSOLE_BORDER;
		
		ScrollDC(hdcMem, 0, -_cyChar, &rc, &rc, NULL, &rcUpdate);

		rc.top += _nCurLine * _cyChar;
		FillRect(hdcMem, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
	}
}

//---------------------------------------------------------------

void _PrintChar(HDC hdcMem, LPCWSTR szChar, int nCharX, int nLine) {
	
	HFONT hfontOld = (HFONT)SelectObject(hdcMem, _hfontConsole);
	SetBkMode(hdcMem, TRANSPARENT);

	SetTextColor(hdcMem, _crConsole);

	TextOut(hdcMem, CX_CONSOLE_BORDER + _cxChar * nCharX, 
		CY_CONSOLE_BORDER + _cyChar * nLine, szChar, 1);		

	SelectObject(hdcMem, hfontOld);	
}

void _PrintNextChar(HDC hdcMem, LPCWSTR szChar) {

	int tmp = (_cxConsole - 2 * CX_CONSOLE_BORDER) / _cxChar - 1;
	if (_nCurCharX > tmp)
		_NewLine(hdcMem);
	_nCurCharX++;
	_PrintChar(hdcMem, szChar, _nCurCharX, _nCurLine);
}

void _ClearConsole(HDC hdcMem) {
	
	RECT rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = _cxConsole;
	rc.bottom = _cyConsole;
	FillRect(hdcMem, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
	_nCurCharX = -1;
	_nCurLine = 0;
}

//---------------------------------------------------------------

void WINAPI PrintNextChar() {

	if (!_hbmConsole)
		return;

	HDC hdcMem = CreateCompatibleDC(NULL);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, _hbmConsole);

	_nCharIndex++;
	while (_szAbout[_nCharIndex] == L'\\') {
		_nCharIndex++;
		switch (_szAbout[_nCharIndex]) {
			case L'n': // new line
				_NewLine(hdcMem);
				goto PNC_EXIT;
			case L't': // tab
				for (int i = 0; i < 4; i++)
					_PrintNextChar(hdcMem, L" ");				
				goto PNC_EXIT;
			case L'd': // delay
				goto PNC_EXIT;
			case L'c': // clear
				_ClearConsole(hdcMem);
				goto PNC_EXIT;
			case L'h': // highlight
				_crConsole = CONSOLE_HIGHLIGHT;
				_nCharIndex++;
				break;
			case L'u': // unhighlight
				_crConsole = CONSOLE_TEXT;
				_nCharIndex++;
				break;
			case L's':
				while (1) {
					_nCharIndex++;
					if (_szAbout[_nCharIndex] == L'\0')
						goto PNC_EXIT;
					else if (_szAbout[_nCharIndex] == L'\\' && 
						_szAbout[_nCharIndex+1] == L's') {
							_nCharIndex++;
							goto PNC_EXIT;
					}
					_PrintNextChar(hdcMem, _szAbout + _nCharIndex);
				}
				break;
		}
	}
	if (_szAbout[_nCharIndex] != L'\0') {
		_PrintNextChar(hdcMem, _szAbout + _nCharIndex);
	} else {
		// clear
		_nCharIndex = -1;		
		_ClearConsole(hdcMem);
	}

PNC_EXIT:

	SelectObject(hdcMem, hbmOld);
	DeleteDC(hdcMem);
}

//---------------------------------------------------------------

void WINAPI DrawCaret(HDC hdc) {
	_PrintChar(hdc, L"_", _nCurCharX + 1, _nCurLine);
}

//---------------------------------------------------------------
//---------------------------------------------------------------

LRESULT CALLBACK Cls_AboutConsoleProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

		case WM_TIMER:
			if (wParam == TIMER_ABOUTCONSOLE) {
				if (IsWindowVisible(hwnd)) {
					PrintNextChar();
					InvalidateRect(hwnd, NULL, FALSE);
				}
			}			
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			
			HDC hdcMem = CreateCompatibleDC(NULL);
			HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, _hbmConsole);
			BitBlt(hdc, 0, 0, _cxConsole, _cyConsole, hdcMem, 0, 0, SRCCOPY);
			SelectObject(hdcMem, hbmOld);
			DeleteDC(hdcMem);

			DrawCaret(hdc);
			EndPaint(hwnd, &ps);
			break;
					   }

		case WM_CREATE: {

			/*_hfontConsole = CreateFont(12, 8, 0, 0, FW_NORMAL, FALSE, 
				0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				NONANTIALIASED_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Terminal");*/
			if (!_hfontConsole)
				_hfontConsole = (HFONT)GetStockObject(OEM_FIXED_FONT);

			TEXTMETRIC tm;

			RECT rc;
			GetClientRect(hwnd, &rc);
			OffsetRect(&rc, -rc.left, -rc.top);
			
			_cxConsole = rc.right;
			_cyConsole = rc.bottom;

			HDC hdc = GetDC(hwnd);
			HFONT hfontOld = (HFONT)SelectObject(hdc, _hfontConsole);
			GetTextMetrics(hdc, &tm);
			SelectObject(hdc, hfontOld);

			_hbmConsole = CreateCompatibleBitmap(hdc, 
				_cxConsole, _cyConsole);

			ReleaseDC(hwnd, hdc);

			_cxChar = tm.tmAveCharWidth;
			_cyChar = tm.tmHeight + 2;

			// clear
			HDC hdcMem = CreateCompatibleDC(NULL);
			HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, _hbmConsole);
			FillRect(hdcMem, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
			SelectObject(hdcMem, hbmOld);
			DeleteDC(hdcMem);

			SetTimer(hwnd, TIMER_ABOUTCONSOLE, 100, NULL);

			break;
						}

		case WM_DESTROY:
			/*DeleteObject(_hfontConsole);
			/_hfontConsole = NULL;*/
			KillTimer(hwnd, TIMER_ABOUTCONSOLE);
			break;

		default:
			return(DefWindowProc(hwnd, uMsg, wParam, lParam));
	}
	return(0L);
}

//---------------------------------------------------------------
//---------------------------------------------------------------
