// generic.cpp

#include "stdafx.h"
#include "main.h"
#include "lang.h"
#include "generic.h"
#include "resource.h"

//-----------------------------------------------------------------

void ReportError(HWND hwnd, LPCWSTR szError) {
	WCHAR szCaption[MAX_LANGLEN];
	LangLoadString(IDS_ERRCAPTION, szCaption, MAX_LANGLEN);
	MessageBeep(MB_ICONERROR);
	g_dwCmdLine |= CCLF_MODALACTIVE;
	MessageBox(hwnd, szError, szCaption, 
		MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND);
	g_dwCmdLine &= ~CCLF_MODALACTIVE;
}

void ReportError(HWND hwnd, UINT uID) {
	WCHAR szError[MAX_LANGLEN];
	LangLoadString(uID, szError, MAX_LANGLEN);
	ReportError(hwnd, szError);
}


//-----------------------------------------------------------

BOOL ConfirmMessage(HWND hwnd, LPCWSTR szConfirm, UINT uIconType) {
	
	WCHAR szCaption[MAX_LANGLEN];
	LangLoadString(IDS_CONFIRMCAPTION, szCaption, MAX_LANGLEN);

	UINT uType = uIconType | MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND;	
	MessageBeep(MB_ICONQUESTION);
	return(MessageBox(hwnd, szConfirm, szCaption, uType) == IDYES);
}

BOOL ConfirmMessage(HWND hwnd, UINT uConfirmID, UINT uIconType) {
	WCHAR szConfirm[MAX_LANGLEN];
	LangLoadString(uConfirmID, szConfirm, MAX_LANGLEN);
	return(ConfirmMessage(hwnd, szConfirm, uIconType));
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

BOOL CenterWindow(HWND hwnd, HWND hwndParent) {

	if (!hwndParent)
		hwndParent = GetDesktopWindow();

	RECT rc, rcParent;
	GetWindowRect(hwnd, &rc);

	GetWindowRect(hwndParent, &rcParent);
	//MapWindowPoints(hwndParent, NULL, (LPPOINT)&rcParent, 2);

	return(SetWindowPos(hwnd, NULL, 
		(rcParent.left + rcParent.right) / 2 - (rc.right - rc.left) / 2, 
		(rcParent.top + rcParent.bottom) / 2 - (rc.bottom - rc.top) / 2, 
		0, 0, SWP_NOSIZE | SWP_NOZORDER));
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------
//-----------------------------------------------------------------
