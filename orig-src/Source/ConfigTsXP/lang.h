// lang.h

#ifndef __LANG_H__
#define __LANG_H__

#define WM_LANGCHANGED			(WM_USER + 100)
#define MAX_LANGLEN				1024

BOOL LoadLangFile(LPCWSTR);
BOOL UpdateLangFile(LPCWSTR);
int LangLoadString(UINT, LPWSTR, int);
int LangLoadString(UINT, stringW&);
INT_PTR CALLBACK Dlg_ChangeLanguageProc(HWND, UINT, WPARAM, LPARAM);
BOOL IsLangDefault();

#endif // __LANG_H__