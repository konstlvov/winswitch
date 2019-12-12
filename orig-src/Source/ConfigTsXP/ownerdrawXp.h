// ownerdrawXp.h

#ifndef __OWNERDRAWXP_H__
#define __OWNERDRAWXP_H__

void InitOwnerDrawCtrls(HWND);
void UxThemeChanged(HWND);

void DrawMyPageTitleBk(const DRAWITEMSTRUCT *);

void SubClassUxIconButton(HWND, UINT, UINT);
void SubClassUxColorButton(HWND, COLORREF);
void DrawUxButton(const DRAWITEMSTRUCT *);
void SetUxButtonColor(HWND, COLORREF);
COLORREF GetUxButtonColor(HWND);
BOOL UxButtonChooseColor(HWND, HWND);
BOOL UxButtonShowMenu(HWND, HWND);

typedef HRESULT (WINAPI *ENABLETHEMEDIALOGTEXTURE)(HWND, DWORD);
extern ENABLETHEMEDIALOGTEXTURE g_pfnEnableThemeDialogTexture;

BOOL DrawFontCombo(const LPDRAWITEMSTRUCT);

#endif // __OWNERDRAWXP_H__