// generic.h

#ifndef __GENERIC_H__
#define __GENERIC_H__


#define HUNG_TIMEOUT					250
#define WM_MYTRAYMSG					(WM_USER + 100)

void ReportError(LPCWSTR);
void ReportError(UINT);
BOOL ConfirmMessage(LPCWSTR, UINT uIconType =MB_ICONQUESTION);
BOOL ConfirmMessage(UINT, UINT uIconType =MB_ICONQUESTION);
void HotKeyError(LPCWSTR, UINT, UINT);
void HotKeyError(UINT, UINT, UINT);
void FatalError(UINT);
HRGN GetPreviewRegion(HWND, CONST XFORM*);
BOOL MyIsHungAppWindow(HWND);
void GetWindowIcons(HWND, HICON*, HICON*);
BOOL MySwitchToThisWindow(HWND);
BOOL MyTerminateProcess(DWORD);
HWND FindTrayToolbarWindow();
BOOL AddTrayIcon(HWND, UINT, HICON, PCWSTR);
BOOL DeleteTrayIcon(HWND, UINT);
BOOL ReloadTrayIcon(HWND, UINT);

struct WALLPAPER {
	BOOL fTiled;
	BOOL fStretched;
	LARGE_INTEGER liWriteTime;
	RECT rcWallpaper;
	WCHAR szWallpaper[MAX_PATH];
};

BOOL MyPaintWallpaper(HDC, const RECT*, const WALLPAPER*);


#define MPW_DESKTOP						0x00000001
#define MPW_NOCHECKPOS					0x00000002
#define MPW_NOREGION					0x00000004
#define MPW_HCENTER						0x00000010
#define MPW_VCENTER						0x00000020

BOOL MyPrintWindow(HWND, HDC, const RECT*, const RECT*, DWORD);

typedef int (MYCOMPAREFUNC)(const PVOID, const PVOID);
void MyInsertionSort(PVOID, int, DWORD, MYCOMPAREFUNC);

#endif // __GENERIC_H__
