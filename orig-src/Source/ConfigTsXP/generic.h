// generic.h

#ifndef __GENERIC_H__
#define __GENERIC_H__

void ReportError(HWND, LPCWSTR);
void ReportError(HWND, UINT);
BOOL ConfirmMessage(HWND, LPCWSTR, UINT uIconType =MB_ICONQUESTION);
BOOL ConfirmMessage(HWND, UINT, UINT uIconType =MB_ICONQUESTION);
BOOL CenterWindow(HWND, HWND =NULL);

#endif // __GENERIC_H__