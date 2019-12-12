// exclusions.h

#include "excldlg.h"

INT_PTR CALLBACK Dlg_ExclusionsProc(HWND, UINT, WPARAM, LPARAM);
int AddExclItem(HWND, const PTSEXCLUSION);
void EmptyExclList(HWND);
