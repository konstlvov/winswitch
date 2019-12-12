// registry.cpp

#include "stdafx.h"
#include "generic.h"
#include "lang.h"
#include "ownerdrawXp.h"
#include "WinHotkeyCtrl.h"
#include "main.h"
#include "preferences.h"
#include "appearance.h"
#include "list.h"
#include "preview.h"
#include "info.h"
#include "hotkeys.h"
#include "exclusions.h"
#include "excldlg.h"
#include "advanced.h"
#include "cfgfile.h"
#include "resource.h"
#include "registry.h"

#include <strsafe.h>

//-----------------------------------------------------------------

UINT _puTrayMsg[] = {
	0,
	WM_LBUTTONUP,
	WM_LBUTTONDBLCLK,
	WM_MBUTTONUP,
	WM_MBUTTONDBLCLK,
	WM_RBUTTONUP,
	WM_RBUTTONDBLCLK
};

extern vector<TSEXCLUSION> _vExclList;
HKEY g_hkeyRoot					= HKEY_CURRENT_USER;
DWORD _dwShowDelay				= DEFAULT_SHOWDELAY;
BOOL _fUnBlockQ					= FALSE;
BOOL _fWheelTab					= FALSE;
DWORD g_dwFlagsEx				= TSEX_DEFCOLORSMASK;

//-----------------------------------------------------------------

BOOL SilentInstall(BOOL fDefaults) {

	DWORD cbData, tmp;
	HKEY hkey;

	g_hkeyRoot = HKEY_CURRENT_USER;
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, KEY_WRITE, &hkey) || 
		!RegCreateKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL)) {

		if (fDefaults) {
			tmp = (g_hkeyRoot == HKEY_CURRENT_USER) ? 0 : 1;
			RegSetValueEx(hkey, RS_ALLUSERS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
		} else {
			cbData = sizeof(DWORD);
			if (!RegQueryValueEx(hkey, RS_ALLUSERS, 0, NULL, (PBYTE)&tmp, &cbData)) {
				if (tmp == 1)
					g_hkeyRoot = HKEY_LOCAL_MACHINE;
			} else {
				tmp = (g_hkeyRoot == HKEY_CURRENT_USER) ? 0 : 1;
				RegSetValueEx(hkey, RS_ALLUSERS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
			}
		}
		RegCloseKey(hkey);
	}
	
	if (RegOpenKeyEx(g_hkeyRoot, g_szRegKeyTs, 0, KEY_READ | KEY_WRITE, &hkey)) {
		fDefaults = TRUE;
		if (RegCreateKeyEx(g_hkeyRoot, g_szRegKeyTs, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hkey, NULL))
			return(FALSE);
	}

	tmp = DEFAULT_FLAGS;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_FLAGS, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_FLAGS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_FLAGSEX;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_FLAGSEX, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_FLAGSEX, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_FADEIN;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_FADEIN, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_FADEIN, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_FADEOUT;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_FADEOUT, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_FADEOUT, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_FLAGSPV;
	if (GetSystemMetrics(SM_CMONITORS) > 1)
		tmp &= ~TSFPV_DESKTOP;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_FLAGSPV, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_FLAGSPV, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_PVDELAY;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_PVDELAY, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_PVDELAY, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_PVWIDTH;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_PVWIDTH, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_PVWIDTH, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = DEFAULT_FLAGSLIST;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_FLAGSLIST, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_FLAGSLIST, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_LISTWIDTH;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_LISTWIDTH, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_LISTWIDTH, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_LISTHEIGHT;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_LISTHEIGHT, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_LISTHEIGHT, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	WCHAR szFont[LF_FACESIZE] = DEFAULT_LISTFONTNAME;
	cbData = sizeof(szFont);
	if (fDefaults || RegQueryValueEx(hkey, RS_LISTFONTNAME, 0, NULL, (PBYTE)szFont, &cbData)) {
		RegSetValueEx(hkey, RS_LISTFONTNAME, 0, REG_SZ, 
			(PBYTE)szFont, (lstrlen(szFont) + 1) * sizeof(WCHAR));
	}
	tmp = DEFAULT_LISTFONTATTR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_LISTFONTATTR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_LISTFONTATTR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = DEFAULT_TEXTCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_TEXTCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_TEXTCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_SELTEXTCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_SELTEXTCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_SELTEXTCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_SELCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_SELCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_SELCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_MARKTEXTCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_MARKTEXTCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_MARKTEXTCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_MARKCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_MARKCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_MARKCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_SELMARKTEXTCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_SELMARKTEXTCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_SELMARKTEXTCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_SELMARKCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_SELMARKCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_SELMARKCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	
	lstrcpyn(szFont, DEFAULT_CAPFONTNAME, SIZEOF_ARRAY(szFont));
	cbData = sizeof(szFont);
	if (fDefaults || RegQueryValueEx(hkey, RS_CAPFONTNAME, 0, NULL, (PBYTE)szFont, &cbData)) {
		RegSetValueEx(hkey, RS_CAPFONTNAME, 0, REG_SZ, 
			(PBYTE)szFont, (lstrlen(szFont) + 1) * sizeof(WCHAR));
	}
	tmp = DEFAULT_CAPFONTATTR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_CAPFONTATTR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_CAPFONTATTR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_CAPTEXTCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_CAPTEXTCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_CAPTEXTCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_CAPSHADOW;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_CAPSHADOW, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_CAPSHADOW, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = DEFAULT_FLAGSPANE;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_FLAGSPANE, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_FLAGSPANE, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	
	lstrcpyn(szFont, DEFAULT_PANEFONTNAME, SIZEOF_ARRAY(szFont));
	cbData = sizeof(szFont);
	if (fDefaults || RegQueryValueEx(hkey, RS_PANEFONTNAME, 0, NULL, (PBYTE)szFont, &cbData)) {
		RegSetValueEx(hkey, RS_PANEFONTNAME, 0, REG_SZ, 
			(PBYTE)szFont, (lstrlen(szFont) + 1) * sizeof(WCHAR));
	}
	tmp = DEFAULT_PANEFONTATTR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_PANEFONTATTR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_PANEFONTATTR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_PANETEXTCOLOR;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_PANETEXTCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_PANETEXTCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_PANESHADOW;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_PANESHADOW, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_PANESHADOW, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = DEFAULT_EXITHK;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_EXITHK, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_EXITHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_SHOWHIDEHK;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_SHOWHIDEHK, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_SHOWHIDEHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_CONFIGHK;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_CONFIGHK, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_CONFIGHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_APPLISTHK;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_APPLISTHK, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_APPLISTHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_INSTLISTHK;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_INSTLISTHK, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_INSTLISTHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_MINIMIZETRAYHK;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_MINIMIZETRAYHK, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_MINIMIZETRAYHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_RESTORETRAYHK;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_RESTORETRAYHK, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_RESTORETRAYHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = DEFAULT_TRAYMENU;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_TRAYMENU, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_TRAYMENU, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_TRAYCONFIG;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_TRAYCONFIG, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_TRAYCONFIG, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_TRAYNEXT;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_TRAYAPPLIST, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_TRAYAPPLIST, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_TRAYPREV;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_TRAYINSTLIST, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_TRAYINSTLIST, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = DEFAULT_CONFIGFLAGS;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_CONFIGFLAGS, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_CONFIGFLAGS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_CONFIGPOS;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_CONFIGPOS, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_CONFIGPOS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = DEFAULT_CONFIGPAGE;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_CONFIGPAGE, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_CONFIGPAGE, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = DEFAULT_SHOWDELAY;
	cbData = sizeof(DWORD);
	if (fDefaults || RegQueryValueEx(hkey, RS_SHOWDELAY, 0, NULL, (PBYTE)&tmp, &cbData))
		RegSetValueEx(hkey, RS_SHOWDELAY, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	RegCloseKey(hkey);
	return(TRUE);
}


void _SetFlags(DWORD dwFlags) {

	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_HIGHPRIORITY, 
		(dwFlags & TSF_HIGHPRIORITY) ? BST_CHECKED : BST_UNCHECKED);	
	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_CHECKMSTS, 
		(dwFlags & TSF_CHECKMSTS) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_EXTMOUSE, 
		(dwFlags & TSF_EXTMOUSE) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_ENUMTRAY, 
		(dwFlags & TSF_ENUMTRAY) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_SHOWTRAYICON, 
		(dwFlags & TSF_SHOWTRAYICON) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_USECUSTOMICON, 
		(dwFlags & TSF_USECUSTOMICON) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_RELOADICONS, 
		(dwFlags & TSF_RELOADICONS) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_APPEARANCE], IDC_ACTIVEMONITOR, 
		(dwFlags & TSF_ACTIVEMONITOR) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_APPEARANCE], IDC_DROPSHADOW, 
		(dwFlags & TSF_DROPSHADOW) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_APPEARANCE], IDC_TRANSPARENCY, 
		(dwFlags & TSF_LAYERED) ? BST_CHECKED : BST_UNCHECKED);

	int n = 255 - (dwFlags & TSF_LAYEREDMASK) + 21;
	if (n < 0) n = 21;
	if (n > 255) n = 255;
	SendMessage(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_TRANSPARENCYSLIDER), 
		TBM_SETPOS, TRUE, (LPARAM)n);
	SendMessage(g_phPages[PAGE_APPEARANCE], WM_HSCROLL, TB_ENDTRACK, NULL);
	Appr_OnCommand(g_phPages[PAGE_APPEARANCE], IDC_TRANSPARENCY, NULL, BN_CLICKED);

	if (dwFlags & TSF_POSTOP) n = 0;
	else if (dwFlags & TSF_POSBOTTOM) n = 6;
	else n = 3;
	if (dwFlags & TSF_POSLEFT) ;
	else if (dwFlags & TSF_POSRIGHT) n += 2;
	else n += 1;
	ComboBox_SetCurSel(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_SCREENPOSCOMBO), n);

	CheckDlgButton(g_phPages[PAGE_HOTKEYS], IDC_REPLACEALTTAB, 
		(dwFlags & TSF_NOREPLACEALTTAB) ? BST_UNCHECKED : BST_CHECKED);
	CheckDlgButton(g_phPages[PAGE_HOTKEYS], IDC_INSTANCESWITCHER, 
		(dwFlags & TSF_INSTSWITCHER) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_HOTKEYS], IDC_STICKYALTTAB, 
		(dwFlags & TSF_STICKYALTTAB) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_HOTKEYS], IDC_HOOKALTTAB, 
		(dwFlags & TSF_HOOKALTTAB) ? BST_CHECKED : BST_UNCHECKED);

	_fWheelTab = (BOOL)(dwFlags & TSF_WHEELTAB);
}

inline void _SetCustomIcon(PCWSTR szCustomIcon) {
	SetCustomIcon(g_phPages[PAGE_PREFERENCES], szCustomIcon);
	Pref_OnCommand(g_phPages[PAGE_PREFERENCES], IDC_USECUSTOMICON, NULL, BN_CLICKED);
}

void _SetFlagsEx(DWORD dwFlagsEx) {

	int n = LOBYTE(LOWORD(dwFlagsEx));
	if (n < 0 || n > MAX_VISUALMODES)
		n = TSEX_DEFAULTMODE;
	ComboBox_SetCurSel(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_VISUALMODECOMBO), n);
	n = HIBYTE(LOWORD(dwFlagsEx));
	if (n < 0 || n > MAX_CLASSICMODES)
		n = TSEX_DEFAULTMODE;
	ComboBox_SetCurSel(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_CLASSICMODECOMBO), n);

	CheckDlgButton(g_phPages[PAGE_APPEARANCE], IDC_NOVISUALSTYLES, 
		(dwFlagsEx & TSFEX_NOVISUALSTYLES) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_APPEARANCE], IDC_LEFTRIGHT, 
		(dwFlagsEx & TSFEX_LEFTRIGHT) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_INFO], IDC_CAPSHOWICON, 
		(dwFlagsEx & TSFEX_CAPNOICON) ? BST_UNCHECKED : BST_CHECKED);
	CheckDlgButton(g_phPages[PAGE_INFO], IDC_CAPSMALLICON, 
		(dwFlagsEx & TSFEX_CAPSMALLICON) ? BST_CHECKED : BST_UNCHECKED);
	/*CheckDlgButton(g_phPages[PAGE_INFO], IDC_SHOWPROCESSINFO, 
		(dwFlagsEx & TSFEX_SHOWPROCESSINFO) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_INFO], IDC_SHOWTASKNUMBER, 
		(dwFlagsEx & TSFEX_SHOWTASKNUMBER) ? BST_CHECKED : BST_UNCHECKED);*/
	CheckDlgButton(g_phPages[PAGE_ADVANCED], IDC_OWNERCAPTION, 
		(dwFlagsEx & TSFEX_OWNERCAPTION) ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(g_phPages[PAGE_ADVANCED], IDC_REMOVECAPTION, 
		(dwFlagsEx & TSFEX_REMOVECAPTION) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_ADVANCED], IDC_REMOVEPANE, 
		(dwFlagsEx & TSFEX_REMOVEPANE) ? BST_CHECKED : BST_UNCHECKED);

	g_dwFlagsEx = dwFlagsEx & TSEX_DEFCOLORSMASK;
}


void _SetFlagsPv(DWORD dwFlagsPv) {

	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVDESKTOPSTYLE, 
		(dwFlagsPv & TSFPV_DESKTOP) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVWINDOWSTYLE, 
		(dwFlagsPv & TSFPV_DESKTOP) ? BST_UNCHECKED : BST_CHECKED);
    CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVWALLPAPER, 
		(dwFlagsPv & TSFPV_WALLPAPER) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVDRAWBORDER, 
		(dwFlagsPv & TSFPV_DRAWBORDER) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVTASKBAR, 
		(dwFlagsPv & TSFPV_TASKBAR) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVVCENTER, 
		(dwFlagsPv & TSFPV_VCENTER) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVPOPUPONLY, 
		(dwFlagsPv & TSFPV_POPUPONLY) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVOLDSTYLE, 
		(dwFlagsPv & TSFPV_OLDSTYLE) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVSHOWMINIMIZED, 
		(dwFlagsPv & TSFPV_SHOWMINIMIZED) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVEXCLUDELAYERED, 
		(dwFlagsPv & TSFPV_EXCLUDELAYERED) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVNOCACHE, 
		(dwFlagsPv & TSFPV_NOCACHE) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_PREVIEW], IDC_PVLIVEUPDATE, 
		(dwFlagsPv & TSFPV_LIVEUPDATE) ? BST_CHECKED : BST_UNCHECKED);

	int n = dwFlagsPv & TSPV_UPDATEMASK;
	if (n < MIN_PVUPDATE || n > MAX_PVUPDATE)
		n = DEFAULT_PVUPDATE;
	SendMessage(GetDlgItem(g_phPages[PAGE_PREVIEW], IDC_PVUPDATESPIN), UDM_SETPOS32, 0, n);

	Pv_OnCommand(g_phPages[PAGE_PREVIEW], IDC_PVDESKTOPSTYLE, NULL, BN_CLICKED);
	Pv_OnCommand(g_phPages[PAGE_PREVIEW], IDC_PVNOCACHE, NULL, BN_CLICKED);
}


void _SetFlagsList(DWORD dwFlagsList) {

	CheckDlgButton(g_phPages[PAGE_LIST], IDC_SHOWTOOLTIPS, 
		(dwFlagsList & TSFL_SHOWTOOLTIPS) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_LIST], IDC_HOTTRACK, 
		(dwFlagsList & TSFL_HOTTRACK) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_LIST], IDC_MOUSEOVER, 
		(dwFlagsList & TSFL_MOUSEOVER) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_LIST], IDC_INVERSEWHEEL, 
		(dwFlagsList & TSFL_INVERSEWHEEL) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_LIST], IDC_SMALLICONS, 
		(dwFlagsList & TSFL_SMALLICONS) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_LIST], IDC_DRAWFOCUSRECT, 
		(dwFlagsList & TSFL_DRAWFOCUSRECT) ? BST_CHECKED : BST_UNCHECKED);
	int n = dwFlagsList & TSL_SELSTYLEMASK;
	if (n < 0 || n >= MAX_SELSTYLES)
		n = DEFAULT_SELSTYLE;
	ComboBox_SetCurSel(GetDlgItem(g_phPages[PAGE_LIST], IDC_SELSTYLECOMBO), n);

	n = 0; // list style
	if (dwFlagsList & TSFL_VERTICALLIST)
		n = 1;
	ComboBox_SetCurSel(GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTSTYLECOMBO), n);
	List_OnCommand(g_phPages[PAGE_LIST], IDC_LISTSTYLECOMBO, 
		GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTSTYLECOMBO), CBN_SELCHANGE);

	_fUnBlockQ = (BOOL)(dwFlagsList & TSFL_ENABLEQ);
}

void _SetListWidth(int nListWidth, DWORD dwFlagsList) {
	if (nListWidth < MIN_LISTWIDTH || nListWidth > MAX_LISTWIDTH)
		nListWidth = DEFAULT_LISTWIDTH;
	SendMessage(GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTWIDTHSPIN), 
		UDM_SETPOS32, 0, (dwFlagsList & TSFL_VERTICALLIST) ? nListWidth : DEFAULT_LISTWIDTH);
	if (dwFlagsList & TSFL_VERTICALLIST)
        nListWidth = DEFAULT_LISTWIDTH;
	if (dwFlagsList & TSFL_SMALLICONS)
		nListWidth /= (CX_SMICON + 2 * (XMARGIN_SMSEL + 1));
	else nListWidth /= (CX_ICON + 2 * (XMARGIN_SEL + 1));
	SendMessage(GetDlgItem(g_phPages[PAGE_LIST], IDC_ICONSXSPIN), UDM_SETPOS32, 0, nListWidth);
}

void _SetListHeight(int nListHeight, DWORD dwFlagsList) {
	if (nListHeight < MIN_LISTHEIGHT || nListHeight > MAX_LISTHEIGHT)
		nListHeight = DEFAULT_LISTHEIGHT;
	if (dwFlagsList & TSFL_SMALLICONS)
		nListHeight /= (CY_SMICON + 2 * (YMARGIN_SMSEL + 1));
	else nListHeight /= (CY_ICON + 2 * (YMARGIN_SEL + 1));
	SendMessage(GetDlgItem(g_phPages[PAGE_LIST], IDC_ICONSYSPIN), UDM_SETPOS32, 0, nListHeight);
}

void _SetFadeIn(DWORD dwFadeIn) {
	int n = dwFadeIn & TSBL_SPEEDMASK;
	if (n < MIN_FADEINSPEED || n > MAX_FADEINSPEED)
		n = DEFAULT_FADEINSPEED;
	SendMessage(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_FADEINSPIN), UDM_SETPOS32, 0, n);
	CheckDlgButton(g_phPages[PAGE_APPEARANCE], IDC_FADEIN, 
		(dwFadeIn & TSFBL_ENABLED) ? BST_CHECKED : BST_UNCHECKED);
	Appr_OnCommand(g_phPages[PAGE_APPEARANCE], IDC_FADEIN, NULL, BN_CLICKED);
}

void _SetFadeOut(DWORD dwFadeOut) {
	int n = dwFadeOut & TSBL_SPEEDMASK;
	if (n < MIN_FADEOUTSPEED || n > MAX_FADEOUTSPEED)
		n = DEFAULT_FADEOUTSPEED;
	SendMessage(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_FADEOUTSPIN), UDM_SETPOS32, 0, n);
	CheckDlgButton(g_phPages[PAGE_APPEARANCE], IDC_FADEOUT, 
		(dwFadeOut & TSFBL_ENABLED) ? BST_CHECKED : BST_UNCHECKED);
	Appr_OnCommand(g_phPages[PAGE_APPEARANCE], IDC_FADEOUT, NULL, BN_CLICKED);
}

inline void _SetPvDelay(DWORD dwPvDelay) {
	if (dwPvDelay < MIN_PVDELAY || dwPvDelay > MAX_PVDELAY) {
		dwPvDelay = DEFAULT_PVDELAY;
		/*SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, (LPVOID)&dwPvDelay, 0);
		dwPvDelay = (dwPvDelay + 1) * 50;*/
	}
	SendMessage(GetDlgItem(g_phPages[PAGE_PREVIEW], IDC_PVDELAYSPIN), 
		UDM_SETPOS32, 0, dwPvDelay);
}

inline void _SetPvWidth(DWORD dwPvWidth) {
	if (dwPvWidth > MAX_PVWIDTH || dwPvWidth < MIN_PVWIDTH)
		dwPvWidth = DEFAULT_PVWIDTH;
	SendMessage(GetDlgItem(g_phPages[PAGE_PREVIEW], IDC_PVWIDTHSPIN), 
		UDM_SETPOS32, 0, dwPvWidth);
}

inline void _SetCapFontName(PCWSTR szBuff) {
	HWND h = GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPFONTNAMECOMBO);
	int n = ComboBox_FindString(h, -1, szBuff);
	if (n == CB_ERR)
		n = 0;
    ComboBox_SetCurSel(h, n);
}

void _SetCapFontAttr(DWORD dwCapFontAttr) {

	WCHAR szBuff[32];
	HWND h = GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPFONTSIZECOMBO);
	StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"%u", (dwCapFontAttr & TSFCF_SIZEMASK));
	int n = ComboBox_FindString(h, -1, szBuff);
	if (n != CB_ERR)
		ComboBox_SetCurSel(h, n);
	else SetWindowText(h, szBuff);

	CheckDlgButton(g_phPages[PAGE_INFO], IDC_CAPFONTBOLD, 
		(dwCapFontAttr & TSFCF_BOLD) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_INFO], IDC_CAPFONTITALIC, 
		(dwCapFontAttr & TSFCF_ITALIC) ? BST_CHECKED : BST_UNCHECKED);
}

inline void _SetCapShadowColors(DWORD dwCapTextColor, DWORD dwCapShadow) {
	
	SetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPTEXTCOLOR), dwCapTextColor);

	CheckDlgButton(g_phPages[PAGE_INFO], IDC_CAPDRAWSHADOW, 
		(dwCapShadow & TSFCS_NODEEP) ? BST_UNCHECKED : BST_CHECKED);
	Info_OnCommand(g_phPages[PAGE_INFO], IDC_CAPDRAWSHADOW, NULL, BN_CLICKED);
	int n = (dwCapShadow & TSFCS_DEEPMASK) >> 24;
	if (dwCapShadow & TSFCS_NEGATIVEDEEP) n = -n;
	SendMessage(GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPSHADOWDEEPSPIN), UDM_SETPOS32, 0, n);
	SetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], 
		IDC_CAPSHADOWCOLOR), dwCapShadow & TSFCS_COLORMASK);
}


inline void _SetPaneFontName(PCWSTR szBuff) {
	HWND h = GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFONTNAMECOMBO);
	int n = ComboBox_FindString(h, -1, szBuff);
	if (n == CB_ERR)
		n = 0;
    ComboBox_SetCurSel(h, n);
}

void _SetPaneFontAttr(DWORD dwPaneFontAttr) {

	WCHAR szBuff[32];
	HWND h = GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFONTSIZECOMBO);
	StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"%u", (dwPaneFontAttr & TSFCF_SIZEMASK));
	int n = ComboBox_FindString(h, -1, szBuff);
	if (n != CB_ERR)
		ComboBox_SetCurSel(h, n);
	else SetWindowText(h, szBuff);

	CheckDlgButton(g_phPages[PAGE_INFO], IDC_PANEFONTBOLD, 
		(dwPaneFontAttr & TSFCF_BOLD) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_INFO], IDC_PANEFONTITALIC, 
		(dwPaneFontAttr & TSFCF_ITALIC) ? BST_CHECKED : BST_UNCHECKED);
}

inline void _SetPaneShadowColors(DWORD dwPaneTextColor, DWORD dwPaneShadow) {
	
	SetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], IDC_PANETEXTCOLOR), dwPaneTextColor);

	CheckDlgButton(g_phPages[PAGE_INFO], IDC_PANEDRAWSHADOW, 
		(dwPaneShadow & TSFCS_NODEEP) ? BST_UNCHECKED : BST_CHECKED);
	Info_OnCommand(g_phPages[PAGE_INFO], IDC_PANEDRAWSHADOW, NULL, BN_CLICKED);
	int n = (dwPaneShadow & TSFCS_DEEPMASK) >> 24;
	if (dwPaneShadow & TSFCS_NEGATIVEDEEP) n = -n;
	SendMessage(GetDlgItem(g_phPages[PAGE_INFO], IDC_PANESHADOWDEEPSPIN), UDM_SETPOS32, 0, n);
	SetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], 
		IDC_PANESHADOWCOLOR), dwPaneShadow & TSFCS_COLORMASK);
}

inline void _SetFlagsPane(DWORD dwFlagsPane) {
	DWORD tmp = HIWORD(dwFlagsPane);
	if (tmp > MAX_PANEHEIGHT || tmp < MIN_PANEHEIGHT)
		tmp = DEFAULT_PANEHEIGHT;
	SendMessage(GetDlgItem(g_phPages[PAGE_INFO], 
		IDC_PANEHEIGHTSPIN), UDM_SETPOS32, 0, tmp);

	tmp = dwFlagsPane & TSP_FORMATMASK;
	if (tmp > TSP_FORMATDESCR)
		tmp = DEFAULT_PANEFORMAT;
    tmp += 1;
	if (!(dwFlagsPane & TSFP_TASKNUMBER) && tmp == 1)
		tmp = 0;
	ComboBox_SetCurSel(GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFORMATCOMBO), tmp);
}


inline void _SetListFontName(PCWSTR szBuff) {
	HWND h = GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTFONTNAMECOMBO);
	int n = ComboBox_FindString(h, -1, szBuff);
	if (n == CB_ERR)
		n = 0;
    ComboBox_SetCurSel(h, n);
}

void _SetListFontAttr(DWORD dwListFontAttr) {

	WCHAR szBuff[32];
	HWND h = GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTFONTSIZECOMBO);
	StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"%u", (dwListFontAttr & TSFCF_SIZEMASK));
	int n = ComboBox_FindString(h, -1, szBuff);
	if (n != CB_ERR)
		ComboBox_SetCurSel(h, n);
	else SetWindowText(h, szBuff);

	CheckDlgButton(g_phPages[PAGE_LIST], IDC_LISTFONTBOLD, 
		(dwListFontAttr & TSFCF_BOLD) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_LIST], IDC_LISTFONTITALIC, 
		(dwListFontAttr & TSFCF_ITALIC) ? BST_CHECKED : BST_UNCHECKED);
}


void _SetTrayAction(UINT idc, DWORD dwAction) {
	int n = 0;
	while (n < SIZEOF_ARRAY(_puTrayMsg) && _puTrayMsg[n] != dwAction) n++;
    if (n >= SIZEOF_ARRAY(_puTrayMsg))
		n = 0;
	ComboBox_SetCurSel(GetDlgItem(g_phPages[PAGE_ADVANCED], idc), n);
}

inline void _SetConfigFlags(DWORD dwCfgFlags) {

	CheckDlgButton(g_phPages[PAGE_ADVANCED], IDC_CFGTOPMOST, 
		(dwCfgFlags & CFGF_TOPMOST) ? BST_CHECKED : BST_UNCHECKED);
	//CheckDlgButton(g_phPages[PAGE_ADVANCED], IDC_CFGTASKBAR, 
	//	(dwCfgFlags & CFGF_TASKBAR) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_ADVANCED], IDC_CFGPOSITION, 
		(dwCfgFlags & CFGF_POSITION) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_ADVANCED], IDC_CFGLASTPAGE, 
		(dwCfgFlags & CFGF_LASTPAGE) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(g_phPages[PAGE_ADVANCED], IDC_CFGTASKSWITCHXP, 
		(dwCfgFlags & CFGF_TASKSWITCHXP) ? BST_CHECKED : BST_UNCHECKED);

	/*tmp = GetWindowLongPtr(g_hwndMain, GWL_EXSTYLE);
	SetWindowLongPtr(g_hwndMain, GWL_EXSTYLE, (dwCfgFlags & CFGF_TASKBAR) 
		? (tmp | WS_EX_APPWINDOW) : (tmp & ~WS_EX_APPWINDOW));*/
	SetWindowPos(g_hwndMain, (dwCfgFlags & CFGF_TOPMOST) 
		? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}


void CheckDefaultColors() {

	BOOL fThemeActive = IsThemeActive();
	COLORREF crCapText, crPaneText;	

	if (g_dwFlagsEx & TSFEX_DEFCAPCOLORS) {
		crCapText = GetSysColor(COLOR_CAPTIONTEXT);
		SetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], 
			IDC_CAPTEXTCOLOR), crCapText);
		SetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], 
			IDC_CAPSHADOWCOLOR), (~crCapText) & 0xffffff);
		//CheckDlgButton(g_phPages[PAGE_INFO], IDC_CAPDRAWSHADOW, 
		//	fThemeActive ? BST_CHECKED : BST_UNCHECKED);
		//Info_OnCommand(g_phPages[PAGE_INFO], IDC_CAPDRAWSHADOW, NULL, BN_CLICKED);
	}

	if (g_dwFlagsEx & TSFEX_DEFPANECOLORS) {
		crPaneText = GetSysColor(COLOR_CAPTIONTEXT);
		SetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], 
			IDC_PANETEXTCOLOR), crPaneText);
		SetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], 
			IDC_PANESHADOWCOLOR), (~crPaneText) & 0xffffff);
		//CheckDlgButton(g_phPages[PAGE_INFO], IDC_PANEDRAWSHADOW, 
		//	fThemeActive ? BST_CHECKED : BST_UNCHECKED);
		//Info_OnCommand(g_phPages[PAGE_INFO], IDC_PANEDRAWSHADOW, NULL, BN_CLICKED);
	}

	if (g_dwFlagsEx & TSFEX_DEFLISTCOLORS) {
		g_pcrList[LCOLOR_TEXT] = GetSysColor(COLOR_MENUTEXT);
		g_pcrList[LCOLOR_SELTEXT] = GetSysColor(COLOR_HIGHLIGHTTEXT);
		g_pcrList[LCOLOR_SEL] = GetSysColor(COLOR_MENUHILIGHT);
		g_pcrList[LCOLOR_MARKTEXT] = GetSysColor(COLOR_HIGHLIGHTTEXT);
		g_pcrList[LCOLOR_MARK] = GetSysColor(COLOR_HIGHLIGHT);
		g_pcrList[LCOLOR_SELMARKTEXT] = GetSysColor(COLOR_HIGHLIGHTTEXT);
		g_pcrList[LCOLOR_SELMARK] = GetSysColor(COLOR_HOTLIGHT);
		List_OnCommand(g_phPages[PAGE_LIST], IDC_LCOLORCOMBO, NULL, CBN_SELCHANGE);
	}
}

// return FALSE - if theme changed
BOOL CheckColorTheme() {

	WCHAR szCurTheme[MAX_PATH], szTheme[MAX_PATH] = L"";
	WCHAR szCurColors[32], szColors[32] = L"";
	if (GetCurrentThemeName(szCurTheme, SIZEOF_ARRAY(szCurTheme), 
		szCurColors, SIZEOF_ARRAY(szCurColors), NULL, 0) != S_OK) {
		szCurTheme[0] = L'\0';
		szCurColors[0] = L'\0';
	}

	HKEY hkey;
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
		DWORD cbData = MAX_PATH * sizeof(WCHAR);
		if (RegQueryValueEx(hkey, RS_THEMEFILE, 0, NULL, (PBYTE)szTheme, &cbData))
			szTheme[0] = L'\0';
		cbData = 32 * sizeof(WCHAR);
		if (RegQueryValueEx(hkey, RS_COLORTHEME, 0, NULL, (PBYTE)szColors, &cbData))
			szColors[0] = L'\0';
		RegCloseKey(hkey);
	}
	if (lstrcmpi(szCurTheme, szTheme) || lstrcmpi(szCurColors, szColors))
		g_dwFlagsEx |= TSEX_DEFCOLORSMASK;

	CheckDefaultColors();

	return(FALSE);
}

//-----------------------------------------------------------------

BOOL InitLanguage() {

	HKEY hkey;
	DWORD cbData, tmp;

	g_hkeyRoot = HKEY_CURRENT_USER;
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
		cbData = sizeof(DWORD);
		if (!RegQueryValueEx(hkey, RS_ALLUSERS, 0, NULL, (PBYTE)&tmp, &cbData) && tmp == 1)
			g_hkeyRoot = HKEY_LOCAL_MACHINE;
		RegCloseKey(hkey);
	}

	BOOL fSuccess = TRUE;	
	if (!RegOpenKeyEx(g_hkeyRoot, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
		WCHAR szBuff[MAX_DATALEN];
		cbData = MAX_DATALEN * sizeof(WCHAR);
		if (!RegQueryValueEx(hkey, RS_LANGFILE, 0, NULL, (PBYTE)szBuff, &cbData) && szBuff[0] != L'\0')
			fSuccess = LoadLangFile(szBuff);
		RegCloseKey(hkey);
	}
	return(fSuccess);
}

//-----------------------------------------------------------------

void LoadSettings(BOOL fDefaults) {

	HKEY hkey = NULL;
	DWORD cbData, tmp, tmp2;

	// per user settings
	if (!fDefaults) {
		g_hkeyRoot = HKEY_CURRENT_USER;
		if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, KEY_READ, &hkey)) {
			cbData = sizeof(DWORD);
			if (!RegQueryValueEx(hkey, RS_ALLUSERS, 0, NULL, (PBYTE)&tmp, &cbData) && tmp == 1)
				g_hkeyRoot = HKEY_LOCAL_MACHINE;
			RegCloseKey(hkey);
		}
		if (g_hkeyRoot == HKEY_LOCAL_MACHINE) {
			LONG lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, KEY_WRITE, &hkey);
			if (lErr == 2) {
				lErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, NULL, 
					REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
			}
			if (!lErr)
				RegCloseKey(hkey);
			EnableWindow(GetDlgItem(g_phPages[PAGE_PREFERENCES], 
				IDC_PERUSERSETTINGS), (BOOL)(lErr == 0));
		}
	} else if (IsWindowEnabled(GetDlgItem(g_phPages[PAGE_PREFERENCES], IDC_PERUSERSETTINGS))) {
		g_hkeyRoot = HKEY_CURRENT_USER;
	}
	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_PERUSERSETTINGS, 
		(BOOL)(g_hkeyRoot == HKEY_CURRENT_USER) ? BST_CHECKED : BST_UNCHECKED);

	// autostart
	if (!fDefaults) {
		tmp = BST_UNCHECKED;
		if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RS_RUN_KEY, 0, KEY_READ, &hkey)) {
			if (!RegQueryValueEx(hkey, RS_TASKSWITCHXP, 0, NULL, NULL, NULL))
				tmp = BST_CHECKED;
			RegCloseKey(hkey);
		}
		if (tmp == BST_UNCHECKED && 
			!RegOpenKeyEx(HKEY_CURRENT_USER, RS_RUN_KEY, 0, KEY_READ, &hkey)) {
			if (!RegQueryValueEx(hkey, RS_TASKSWITCHXP, 0, NULL, NULL, NULL))
				tmp = BST_CHECKED;
			RegCloseKey(hkey);
		}		
	} else
		tmp = BST_CHECKED;
	CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_AUTOSTART, tmp);

	if (fDefaults || RegOpenKeyEx(g_hkeyRoot, g_szRegKeyTs, 0, KEY_READ, &hkey))
		hkey = NULL;

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_FLAGS, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_FLAGS;
	_SetFlags(tmp);

	WCHAR szBuff[MAX_DATALEN];
	cbData = MAX_DATALEN * sizeof(WCHAR);
	if (!hkey || RegQueryValueEx(hkey, RS_CUSTOMICON, 0, NULL, (PBYTE)szBuff, &cbData))
		szBuff[0] = L'\0';
	_SetCustomIcon(szBuff);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_FLAGSEX, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_FLAGSEX;
    _SetFlagsEx(tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_FADEIN, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_FADEIN;
	_SetFadeIn(tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_FADEOUT, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_FADEOUT;
	_SetFadeOut(tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_FLAGSPV, 0, NULL, (PBYTE)&tmp, &cbData)) {
		tmp = DEFAULT_FLAGSPV;
		if (GetSystemMetrics(SM_CMONITORS) > 1)
			tmp &= ~TSFPV_DESKTOP;
	}
	_SetFlagsPv(tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_PVDELAY, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_PVDELAY;
	_SetPvDelay(tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_PVWIDTH, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_PVWIDTH;
	_SetPvWidth(tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_FLAGSLIST, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_FLAGSLIST;
	_SetFlagsList(tmp);

	int n;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_LISTWIDTH, 0, NULL, (PBYTE)&n, &cbData))
		n = DEFAULT_LISTWIDTH;
	_SetListWidth(n, tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_LISTHEIGHT, 0, NULL, (PBYTE)&n, &cbData))
		n = DEFAULT_LISTHEIGHT;
	_SetListHeight(n, tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_TEXTCOLOR, 0, NULL, (PBYTE)&g_pcrList[LCOLOR_TEXT], &cbData))
		g_pcrList[LCOLOR_TEXT] = DEFAULT_TEXTCOLOR;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_SELTEXTCOLOR, 0, NULL, (PBYTE)&g_pcrList[LCOLOR_SELTEXT], &cbData))
		g_pcrList[LCOLOR_SELTEXT] = DEFAULT_SELTEXTCOLOR;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_SELCOLOR, 0, NULL, (PBYTE)&g_pcrList[LCOLOR_SEL], &cbData))
		g_pcrList[LCOLOR_SEL] = DEFAULT_SELCOLOR;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_MARKTEXTCOLOR, 0, NULL, (PBYTE)&g_pcrList[LCOLOR_MARKTEXT], &cbData))
		g_pcrList[LCOLOR_MARKTEXT] = DEFAULT_MARKTEXTCOLOR;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_MARKCOLOR, 0, NULL, (PBYTE)&g_pcrList[LCOLOR_MARK], &cbData))
		g_pcrList[LCOLOR_MARK] = DEFAULT_MARKCOLOR;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_SELMARKTEXTCOLOR, 0, NULL, 
		(PBYTE)&g_pcrList[LCOLOR_SELMARKTEXT], &cbData))
		g_pcrList[LCOLOR_SELMARKTEXT] = DEFAULT_SELMARKTEXTCOLOR;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_SELMARKCOLOR, 0, NULL, 
		(PBYTE)&g_pcrList[LCOLOR_SELMARK], &cbData))
		g_pcrList[LCOLOR_SELMARK] = DEFAULT_SELMARKCOLOR;	
	ComboBox_SetCurSel(GetDlgItem(g_phPages[PAGE_LIST], IDC_LCOLORCOMBO), 0);
	List_OnCommand(g_phPages[PAGE_LIST], IDC_LCOLORCOMBO, NULL, CBN_SELCHANGE);

	cbData = LF_FACESIZE * sizeof(WCHAR);
	if (!hkey || RegQueryValueEx(hkey, RS_CAPFONTNAME, 0, NULL, (PBYTE)szBuff, &cbData))
		StringCchCopy(szBuff, LF_FACESIZE, DEFAULT_CAPFONTNAME);
	_SetCapFontName(szBuff);
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_CAPFONTATTR, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_CAPFONTATTR;
	_SetCapFontAttr(tmp);
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_CAPTEXTCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_CAPTEXTCOLOR;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_CAPSHADOW, 0, NULL, (PBYTE)&tmp2, &cbData))
		tmp2 = DEFAULT_CAPSHADOW;
	_SetCapShadowColors(tmp, tmp2);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_FLAGSPANE, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_FLAGSPANE;
	_SetFlagsPane(tmp);

	cbData = LF_FACESIZE * sizeof(WCHAR);
	if (!hkey || RegQueryValueEx(hkey, RS_PANEFONTNAME, 0, NULL, (PBYTE)szBuff, &cbData))
		StringCchCopy(szBuff, LF_FACESIZE, DEFAULT_PANEFONTNAME);
	_SetPaneFontName(szBuff);
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_PANEFONTATTR, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_PANEFONTATTR;
	_SetPaneFontAttr(tmp);
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_PANETEXTCOLOR, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_PANETEXTCOLOR;
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_PANESHADOW, 0, NULL, (PBYTE)&tmp2, &cbData))
		tmp2 = DEFAULT_PANESHADOW;
	_SetPaneShadowColors(tmp, tmp2);

	cbData = LF_FACESIZE * sizeof(WCHAR);
	if (!hkey || RegQueryValueEx(hkey, RS_LISTFONTNAME, 0, NULL, (PBYTE)szBuff, &cbData))
		StringCchCopy(szBuff, LF_FACESIZE, DEFAULT_LISTFONTNAME);
	_SetListFontName(szBuff);
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_LISTFONTATTR, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_LISTFONTATTR;
	_SetListFontAttr(tmp);
	cbData = sizeof(DWORD);

	// hotkeys
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_EXITHK, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_EXITHK;
	SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_EXITHK), tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_SHOWHIDEHK, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_SHOWHIDEHK;
	SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_SHOWHIDEHK), tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_CONFIGHK, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_CONFIGHK;
	SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_CONFIGHK), tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_APPLISTHK, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_APPLISTHK;
	SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_APPLISTHK), tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_INSTLISTHK, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_INSTLISTHK;
	SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_INSTLISTHK), tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_MINIMIZETRAYHK, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_MINIMIZETRAYHK;
	SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_MINIMIZETRAYHK), tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_RESTORETRAYHK, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_RESTORETRAYHK;
	SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_RESTORETRAYHK), tmp);

	// tray actions
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_TRAYMENU, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_TRAYMENU;
	_SetTrayAction(IDC_TRAYMENUCOMBO, tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_TRAYCONFIG, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_TRAYCONFIG;
	_SetTrayAction(IDC_TRAYCONFIGCOMBO, tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_TRAYAPPLIST, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_TRAYNEXT;
	_SetTrayAction(IDC_TRAYAPPCOMBO, tmp);

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_TRAYINSTLIST, 0, NULL, (PBYTE)&tmp, &cbData))
		tmp = DEFAULT_TRAYPREV;
	_SetTrayAction(IDC_TRAYINSTCOMBO, tmp);
    
	// config settings
	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_CONFIGFLAGS, 0, NULL, (PBYTE)&tmp2, &cbData))
		tmp2 = DEFAULT_CONFIGFLAGS;
	_SetConfigFlags(tmp2);

	if (!fDefaults) {
		if (tmp2 & CFGF_TASKSWITCHXP)
			StartTaskSwitchXP();

		if (tmp2 & CFGF_POSITION) {
			cbData = sizeof(DWORD);
			if (!hkey || RegQueryValueEx(hkey, RS_CONFIGPOS, 0, NULL, (PBYTE)&tmp, &cbData))
				tmp = DEFAULT_CONFIGPOS;
			if (tmp != DEFAULT_CONFIGPOS) {
				n = GetSystemMetrics(SM_XVIRTUALSCREEN);
				if (LOWORD(tmp) >= n && LOWORD(tmp) < n + GetSystemMetrics(SM_CXVIRTUALSCREEN)) {
					n = GetSystemMetrics(SM_YVIRTUALSCREEN);
					if (HIWORD(tmp) >= n && HIWORD(tmp) < n + GetSystemMetrics(SM_CYVIRTUALSCREEN)) {
						SetWindowPos(g_hwndMain, HWND_TOP, LOWORD(tmp), HIWORD(tmp), 0, 0, SWP_NOSIZE);
					}
				}
			}
		}

		if (g_dwCmdLine & CCLF_NEWEXCL) {
			n = PAGE_EXCLUSIONS;
		} else {
			n = (int)(g_dwCmdLine & CCLF_PAGEMASK);
			g_dwCmdLine |= CCLF_PAGEMASK;
		}
		if (n < 0 || n >= C_PAGES) {
			cbData = sizeof(DWORD);
			if (!(tmp2 & CFGF_LASTPAGE) || 
				RegQueryValueEx(hkey, RS_CONFIGPAGE, 0, NULL, (PBYTE)&n, &cbData) || 
				n < 0 || n >= C_PAGES) n = DEFAULT_CONFIGPAGE;
		}
		SelectPage(n);
	}

	cbData = sizeof(DWORD);
	if (!hkey || RegQueryValueEx(hkey, RS_SHOWDELAY, 0, NULL, (PBYTE)&_dwShowDelay, &cbData))
		_dwShowDelay = DEFAULT_SHOWDELAY;

	if (hkey) {

		HKEY hkeyExcl;
		if (RegOpenKeyEx(hkey, RS_EXCLUSIONS_SUBKEY, 0, KEY_READ, &hkeyExcl))
			hkeyExcl = NULL;

		HWND h = GetDlgItem(g_phPages[PAGE_EXCLUSIONS], IDC_EXCLLIST);
		EmptyExclList(h);

		if (hkeyExcl) {

			TSEXCLUSION tse;
			cbData = sizeof(DWORD);
			if (!RegQueryValueEx(hkeyExcl, RS_EXCLCOUNT, 0, NULL, (PBYTE)&tmp, &cbData)) {

				for (UINT i = 0; i < MAX(tmp, MAX_EXCLUSIONS); i++) {				
					StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"Flags%u", i);
					cbData = sizeof(DWORD);
					if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, (PBYTE)&tse.dwFlags, &cbData))
						break;
					StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"ExclName%u", i);
					cbData = MAX_EXCLNAME * sizeof(WCHAR);
					if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, (PBYTE)&tse.szExclName, &cbData))
						tse.szExclName[0] = L'\0';

					StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"ExeName%u", i);
					cbData = MAX_FILENAME * sizeof(WCHAR);
					if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, (PBYTE)&tse.szExeName, &cbData))
						tse.szExeName[0] = L'\0';
					StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"ClassName%u", i);
					cbData = MAX_CLASSNAME * sizeof(WCHAR);
					if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, (PBYTE)&tse.szClassName, &cbData))
						tse.szClassName[0] = L'\0';
					StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"Caption%u", i);
					cbData = MAX_CAPTION * sizeof(WCHAR);
					if (RegQueryValueEx(hkeyExcl, szBuff, 0, NULL, (PBYTE)&tse.szCaption, &cbData))
						tse.szCaption[0] = L'\0';
					AddExclItem(h, &tse);
				}
			}
			RegCloseKey(hkeyExcl);
		}
		RegCloseKey(hkey);
	}

	if (!fDefaults && g_dwCmdLine & CCLF_NEWEXCL) {
		g_dwCmdLine &= ~CCLF_NEWEXCL;
		PostMessage(g_phPages[PAGE_EXCLUSIONS], WM_COMMAND, 
			MAKEWPARAM(IDC_EXCLNEW, BN_CLICKED), 0);
	}

	if (!fDefaults) {
		if (!CheckPreviewMode(g_phPages[PAGE_PREVIEW])) {
			Pv_OnCommand(g_phPages[PAGE_PREVIEW], IDC_PVWINDOWSTYLE, NULL, BN_CLICKED);
		}
	}
	if (CheckColorTheme())
		CheckDefaultColors();

	EnableWindow(GetDlgItem(g_hwndMain, IDC_APPLY), FALSE);
}

//-----------------------------------------------------------------
// return 0 - success, (-1) - exclusions, (-2) - settings reloaded, (-3) - bad file
// халява - никогда так не делайте!!!
int ImportSettings(PCWSTR pszFile) {

	_ASSERT(pszFile);

	BOOL fAllUsers = FALSE;
	DWORD dwShowDelay = _dwShowDelay;
	HANDLE hFile = INVALID_HANDLE_VALUE, hFileMap = NULL;
	PWSTR pchFile = NULL;
	WCHAR szBuff[MAX_DATALEN];
	DWORD tmp, tmp2;

	static const WCHAR s_szTaskSwitchXP[] = L"[TaskSwitchXP Pro]";
	static const WCHAR s_szExclusions[] = L"[Exclusions]";
	int nRet = -3;

	__try {
		hFile = CreateFile(pszFile, GENERIC_READ, 
			FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			__leave;

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize > 1024 * 1024)
			__leave;

		hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 
			0, dwFileSize, NULL);
		if (!hFileMap) __leave;

		pchFile = (PWSTR)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
		if (!pchFile) __leave;

		UINT nLength = dwFileSize / sizeof(WCHAR), pos = 0;

		if (!_SkipSpaces(pchFile, nLength, pos))
			__leave;
		if (_tcsncmp(pchFile + pos, s_szTaskSwitchXP, SIZEOF_ARRAY(s_szTaskSwitchXP) - 1))
			__leave;
		pos += SIZEOF_ARRAY(s_szTaskSwitchXP) - 1;

		if (!_SkipSpaces(pchFile, nLength, pos) ||
			!_GetDWORD(pchFile, nLength, pos, (DWORD*)&fAllUsers)) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_AUTOSTART, 
			tmp ? BST_CHECKED : BST_UNCHECKED);
		nRet++;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetFlags(tmp);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetString(pchFile, nLength, pos, szBuff, MAX_DATALEN)) __leave;
		_SetCustomIcon(szBuff);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetFlagsEx(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetFadeIn(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetFadeOut(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetFlagsPv(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetPvDelay(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetPvWidth(tmp);

		int n;
        if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetFlagsList(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, (DWORD*)&n)) __leave;
		_SetListWidth(n, tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, (DWORD*)&n)) __leave;
		_SetListHeight(n, tmp);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetString(pchFile, nLength, pos, szBuff, LF_FACESIZE)) __leave;
		_SetListFontName(szBuff);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetListFontAttr(tmp);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &g_pcrList[LCOLOR_TEXT])) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &g_pcrList[LCOLOR_SELTEXT])) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &g_pcrList[LCOLOR_SEL])) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &g_pcrList[LCOLOR_MARKTEXT])) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &g_pcrList[LCOLOR_MARK])) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &g_pcrList[LCOLOR_SELMARKTEXT])) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &g_pcrList[LCOLOR_SELMARK])) __leave;

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetString(pchFile, nLength, pos, szBuff, LF_FACESIZE)) __leave;
		_SetCapFontName(szBuff);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetCapFontAttr(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp2)) __leave;
		_SetCapShadowColors(tmp, tmp2);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetFlagsPane(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetString(pchFile, nLength, pos, szBuff, LF_FACESIZE)) __leave;
		_SetPaneFontName(szBuff);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetPaneFontAttr(tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp2)) __leave;
		_SetPaneShadowColors(tmp, tmp2);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_EXITHK), tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_SHOWHIDEHK), tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_CONFIGHK), tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_APPLISTHK), tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_INSTLISTHK), tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_MINIMIZETRAYHK), tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		SetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], IDC_RESTORETRAYHK), tmp);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetTrayAction(IDC_TRAYMENUCOMBO, tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetTrayAction(IDC_TRAYCONFIGCOMBO, tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetTrayAction(IDC_TRAYAPPCOMBO, tmp);
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetTrayAction(IDC_TRAYINSTCOMBO, tmp);

		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &tmp)) __leave;
		_SetConfigFlags(tmp);
		
		if (!_SkipSpaces(pchFile, nLength, pos) || 
			!_GetDWORD(pchFile, nLength, pos, &dwShowDelay)) __leave;
		nRet++;
		
		if (IsWindowEnabled(GetDlgItem(g_phPages[PAGE_PREFERENCES], IDC_PERUSERSETTINGS)))
			g_hkeyRoot = fAllUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
		CheckDlgButton(g_phPages[PAGE_PREFERENCES], IDC_PERUSERSETTINGS, 
			(BOOL)(g_hkeyRoot == HKEY_CURRENT_USER) ? BST_CHECKED : BST_UNCHECKED);
		_dwShowDelay = dwShowDelay;

		// exclusions
		while (1) {
			if (!_WaitNextSection(pchFile, nLength, pos))
				__leave;
			if (!_tcsncmp(pchFile + pos, s_szExclusions, SIZEOF_ARRAY(s_szExclusions) - 1)) {
				pos += SIZEOF_ARRAY(s_szExclusions) - 1;
				break;
			}			
		}
		HWND h = GetDlgItem(g_phPages[PAGE_EXCLUSIONS], IDC_EXCLLIST);
		TSEXCLUSION tse;
		while (_SkipSpaces(pchFile, nLength, pos)) {
			if (!_GetString(pchFile, nLength, pos, tse.szExclName, MAX_EXCLNAME) ||
				!_SkipSpaces(pchFile, nLength, pos)) __leave;
			if (!_GetString(pchFile, nLength, pos, tse.szExeName, MAX_FILENAME) ||
				!_SkipSpaces(pchFile, nLength, pos)) __leave;
			if (!_GetString(pchFile, nLength, pos, tse.szClassName, MAX_CLASSNAME) ||
				!_SkipSpaces(pchFile, nLength, pos)) __leave;
			if (!_GetString(pchFile, nLength, pos, tse.szCaption, MAX_CAPTION) ||
				!_SkipSpaces(pchFile, nLength, pos)) __leave;
			if (!_GetDWORD(pchFile, nLength, pos, &tse.dwFlags))
				__leave;
			tse.dwFlags |= TSEXCLF_DISABLED;
			AddExclItem(h, &tse);
		}
		nRet++;
	}
	__finally {
		if (pchFile)
			UnmapViewOfFile(pchFile);
		if (hFileMap)
			CloseHandle(hFileMap);
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
	}
	if (nRet == -2)
		LoadSettings(TRUE);
	else if (CheckColorTheme())
		CheckDefaultColors();

	return(nRet);
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

inline DWORD _GetAllUsers() {
	return((IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], 
		IDC_PERUSERSETTINGS) == BST_CHECKED) ? 0 : 1);
}

inline DWORD _GetAutoStart() {
	return((IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], 
		IDC_AUTOSTART) == BST_CHECKED) ? 1 : 0);
}


DWORD _GetFlags() {

	DWORD dwFlags = 0;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], IDC_HIGHPRIORITY) == BST_CHECKED)
		dwFlags |= TSF_HIGHPRIORITY;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], IDC_CHECKMSTS) == BST_CHECKED)
		dwFlags |= TSF_CHECKMSTS;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], IDC_EXTMOUSE) == BST_CHECKED)
		dwFlags |= TSF_EXTMOUSE;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], IDC_ENUMTRAY) == BST_CHECKED)
		dwFlags |= TSF_ENUMTRAY;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], IDC_SHOWTRAYICON) == BST_CHECKED)
		dwFlags |= TSF_SHOWTRAYICON;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], IDC_USECUSTOMICON) == BST_CHECKED)
		dwFlags |= TSF_USECUSTOMICON;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], IDC_RELOADICONS) == BST_CHECKED)
		dwFlags |= TSF_RELOADICONS;
	if (IsDlgButtonChecked(g_phPages[PAGE_APPEARANCE], IDC_ACTIVEMONITOR) == BST_CHECKED)
		dwFlags |= TSF_ACTIVEMONITOR;
	if (IsDlgButtonChecked(g_phPages[PAGE_APPEARANCE], IDC_DROPSHADOW) == BST_CHECKED)
		dwFlags |= TSF_DROPSHADOW;
	if (IsDlgButtonChecked(g_phPages[PAGE_APPEARANCE], IDC_TRANSPARENCY) == BST_CHECKED)
		dwFlags |= TSF_LAYERED;
	if (IsDlgButtonChecked(g_phPages[PAGE_HOTKEYS], IDC_REPLACEALTTAB) != BST_CHECKED)
		dwFlags |= TSF_NOREPLACEALTTAB;
	if (IsDlgButtonChecked(g_phPages[PAGE_HOTKEYS], IDC_INSTANCESWITCHER) == BST_CHECKED)
		dwFlags |= TSF_INSTSWITCHER;
	if (IsDlgButtonChecked(g_phPages[PAGE_HOTKEYS], IDC_STICKYALTTAB) == BST_CHECKED)
		dwFlags |= TSF_STICKYALTTAB;
	if (IsDlgButtonChecked(g_phPages[PAGE_HOTKEYS], IDC_HOOKALTTAB) == BST_CHECKED)
		dwFlags |= TSF_HOOKALTTAB;

	dwFlags |= (255 - (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_APPEARANCE], 
			IDC_TRANSPARENCYSLIDER), TBM_GETPOS, 0, 0) + 21) & TSF_LAYEREDMASK;
	int n = ComboBox_GetCurSel(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_SCREENPOSCOMBO));
	if (n != CB_ERR) {
		if (n < 3)
			dwFlags |= TSF_POSTOP;
		else if (n > 5)
			dwFlags |= TSF_POSBOTTOM;
		if (!(n % 3))
			dwFlags |= TSF_POSLEFT;
		else if (!((n + 1) % 3))
			dwFlags |= TSF_POSRIGHT;
	}

	if (_fWheelTab)
		dwFlags |= TSF_WHEELTAB;

	return(dwFlags);
}

inline void _GetCustomIcon(PWSTR pBuff, UINT cchBuff) {
	UINT len = GetDlgItemText(g_phPages[PAGE_PREFERENCES], 
		IDC_CUSTOMICONEDIT, pBuff, cchBuff);
	if (len == 0 || len >= cchBuff)
        pBuff[0] = L'\0';
}

DWORD _GetFlagsEx() {

	int n = ComboBox_GetCurSel(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_VISUALMODECOMBO));
	if (n == CB_ERR)
		n = TSEX_DEFAULTMODE;
	DWORD dwFlagsEx = n;
	n = ComboBox_GetCurSel(GetDlgItem(g_phPages[PAGE_APPEARANCE], IDC_CLASSICMODECOMBO));
	if (n == CB_ERR)
		n = TSEX_DEFAULTMODE;
	dwFlagsEx |= (n << 8) & 0xff00;

	if (IsDlgButtonChecked(g_phPages[PAGE_APPEARANCE], IDC_NOVISUALSTYLES) == BST_CHECKED)
		dwFlagsEx |= TSFEX_NOVISUALSTYLES;
	if (IsDlgButtonChecked(g_phPages[PAGE_APPEARANCE], IDC_LEFTRIGHT) == BST_CHECKED)
		dwFlagsEx |= TSFEX_LEFTRIGHT;
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_CAPSHOWICON) != BST_CHECKED)
		dwFlagsEx |= TSFEX_CAPNOICON;
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_CAPSMALLICON) == BST_CHECKED)
		dwFlagsEx |= TSFEX_CAPSMALLICON;
	/*if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_SHOWPROCESSINFO) == BST_CHECKED)
		dwFlagsEx |= TSFEX_SHOWPROCESSINFO;
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_SHOWTASKNUMBER) == BST_CHECKED)
		dwFlagsEx |= TSFEX_SHOWTASKNUMBER;*/
	if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_OWNERCAPTION) == BST_CHECKED)
		dwFlagsEx |= TSFEX_OWNERCAPTION;
	if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_REMOVECAPTION) == BST_CHECKED)
		dwFlagsEx |= TSFEX_REMOVECAPTION;
	if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_REMOVEPANE) == BST_CHECKED)
		dwFlagsEx |= TSFEX_REMOVEPANE;

	dwFlagsEx |= (g_dwFlagsEx & TSEX_DEFCOLORSMASK);

	return(dwFlagsEx);
}


DWORD _GetFlagsPv() {

	BOOL fError;
	DWORD dwFlagsPv = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_PREVIEW], 
		IDC_PVUPDATESPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
	if (fError)
		dwFlagsPv = DEFAULT_PVUPDATE;

	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVDESKTOPSTYLE) == BST_CHECKED)
		dwFlagsPv |= TSFPV_DESKTOP;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVWALLPAPER) == BST_CHECKED)
		dwFlagsPv |= TSFPV_WALLPAPER;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVDRAWBORDER) == BST_CHECKED)
		dwFlagsPv |= TSFPV_DRAWBORDER;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVTASKBAR) == BST_CHECKED)
		dwFlagsPv |= TSFPV_TASKBAR;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVVCENTER) == BST_CHECKED)
		dwFlagsPv |= TSFPV_VCENTER;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVPOPUPONLY) == BST_CHECKED)
		dwFlagsPv |= TSFPV_POPUPONLY;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVOLDSTYLE) == BST_CHECKED)
		dwFlagsPv |= TSFPV_OLDSTYLE;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVSHOWMINIMIZED) == BST_CHECKED)
		dwFlagsPv |= TSFPV_SHOWMINIMIZED;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVEXCLUDELAYERED) == BST_CHECKED)
		dwFlagsPv |= TSFPV_EXCLUDELAYERED;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVNOCACHE) == BST_CHECKED)
		dwFlagsPv |= TSFPV_NOCACHE;
	if (IsDlgButtonChecked(g_phPages[PAGE_PREVIEW], IDC_PVLIVEUPDATE) == BST_CHECKED)
		dwFlagsPv |= TSFPV_LIVEUPDATE;

	return(dwFlagsPv);
}


DWORD _GetFlagsList() {

	DWORD dwFlagsList = ComboBox_GetCurSel(GetDlgItem(g_phPages[PAGE_LIST], IDC_SELSTYLECOMBO));
	dwFlagsList &= TSL_SELSTYLEMASK;
	if (IsDlgButtonChecked(g_phPages[PAGE_LIST], IDC_SHOWTOOLTIPS) == BST_CHECKED)
		dwFlagsList |= TSFL_SHOWTOOLTIPS;
	if (IsDlgButtonChecked(g_phPages[PAGE_LIST], IDC_HOTTRACK) == BST_CHECKED)
		dwFlagsList |= TSFL_HOTTRACK;
	if (IsDlgButtonChecked(g_phPages[PAGE_LIST], IDC_MOUSEOVER) == BST_CHECKED)
		dwFlagsList |= TSFL_MOUSEOVER;
	if (IsDlgButtonChecked(g_phPages[PAGE_LIST], IDC_INVERSEWHEEL) == BST_CHECKED)
		dwFlagsList |= TSFL_INVERSEWHEEL;
	if (IsDlgButtonChecked(g_phPages[PAGE_LIST], IDC_SMALLICONS) == BST_CHECKED)
		dwFlagsList |= TSFL_SMALLICONS;
	if (IsDlgButtonChecked(g_phPages[PAGE_LIST], IDC_DRAWFOCUSRECT) == BST_CHECKED)
		dwFlagsList |= TSFL_DRAWFOCUSRECT;

	if (ComboBox_GetCurSel(GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTSTYLECOMBO)) == 1)
		dwFlagsList |= TSFL_VERTICALLIST;

	if (_fUnBlockQ)
		dwFlagsList |= TSFL_ENABLEQ;

	return(dwFlagsList);
}

DWORD _GetListWidth(DWORD dwFlagsList) {

	BOOL fError;
	DWORD dwListWidth;
	if (dwFlagsList & TSFL_VERTICALLIST) {
		dwListWidth = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_LIST],
			IDC_LISTWIDTHSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
		if (fError)
			dwListWidth = DEFAULT_LISTWIDTH;
	} else {
		dwListWidth = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_LIST],
			IDC_ICONSXSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
		if (!fError) {
			if (dwFlagsList & TSFL_SMALLICONS)
				dwListWidth *= (CX_SMICON + 2 * (XMARGIN_SMSEL + 1));
			else dwListWidth *= (CX_ICON + 2 * (XMARGIN_SEL + 1));
		} else dwListWidth = DEFAULT_LISTWIDTH;
	}
	return(dwListWidth);
}

DWORD _GetListHeight(DWORD dwFlagsList) {

	BOOL fError;
	DWORD dwListHeight = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_LIST],
		IDC_ICONSYSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
	if (!fError) {
		if (dwFlagsList & TSFL_SMALLICONS)
			dwListHeight *= (CY_SMICON + 2 * (YMARGIN_SMSEL + 1));
		else dwListHeight *= (CY_ICON + 2 * (YMARGIN_SEL + 1));
	} else 
		dwListHeight = DEFAULT_LISTHEIGHT;

	return(dwListHeight);
}


DWORD _GetFadeIn() {
	BOOL fError;
	DWORD dwFadeIn = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_APPEARANCE], 
		IDC_FADEINSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
	if (fError)
		dwFadeIn = DEFAULT_FADEINSPEED;
	dwFadeIn &= TSBL_SPEEDMASK;
	if (IsDlgButtonChecked(g_phPages[PAGE_APPEARANCE], IDC_FADEIN) == BST_CHECKED)
		dwFadeIn |= TSFBL_ENABLED;
	return(dwFadeIn);
}

DWORD _GetFadeOut() {
	BOOL fError;
	DWORD dwFadeOut = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_APPEARANCE], 
		IDC_FADEOUTSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
	if (fError)
		dwFadeOut = DEFAULT_FADEOUTSPEED;
	dwFadeOut &= TSBL_SPEEDMASK;
	if (IsDlgButtonChecked(g_phPages[PAGE_APPEARANCE], IDC_FADEOUT) == BST_CHECKED)
		dwFadeOut |= TSFBL_ENABLED;
	return(dwFadeOut);
}

inline DWORD _GetPvDelay() {
	BOOL fError;
	DWORD dwPvDelay = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_PREVIEW],
		IDC_PVDELAYSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
	if (fError)
		dwPvDelay = DEFAULT_PVDELAY;
	return(dwPvDelay);
}

inline DWORD _GetPvWidth() {
	BOOL fError;
	DWORD dwPvWidth = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_PREVIEW],
		IDC_PVWIDTHSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
	if (fError)
		dwPvWidth = DEFAULT_PVWIDTH;
	return(dwPvWidth);
}

void _GetCapFontName(PWSTR pBuff, UINT cchBuff) {
	HWND h = GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPFONTNAMECOMBO);
	int len = CB_ERR, n = ComboBox_GetCurSel(h);
	if (n != CB_ERR) {
		len = ComboBox_GetLBTextLen(h, n);
		if (len != CB_ERR && len < (int)cchBuff)
			len = ComboBox_GetLBText(h, n, pBuff);
	}
	if (len == CB_ERR)
		pBuff[0] = L'\0';
}

DWORD _GetCapFontAttr() {

	WCHAR szBuff[32];
	DWORD dwCapFontAttr = 10;
	if (GetDlgItemText(g_phPages[PAGE_INFO], 
		IDC_CAPFONTSIZECOMBO, szBuff, SIZEOF_ARRAY(szBuff))) {

		dwCapFontAttr = _wtol(szBuff);
		if (dwCapFontAttr < 8) dwCapFontAttr = 8;
		else if (dwCapFontAttr > TSFCF_SIZEMASK)
			dwCapFontAttr = TSFCF_SIZEMASK;
	}
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_CAPFONTBOLD) == BST_CHECKED)
		dwCapFontAttr |= TSFCF_BOLD;
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_CAPFONTITALIC) == BST_CHECKED)
		dwCapFontAttr |= TSFCF_ITALIC;

	return(dwCapFontAttr);
}

inline DWORD _GetCapTextColor() {
	return((DWORD)GetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPTEXTCOLOR)));
}

DWORD _GetCapShadow() {
	DWORD dwCapShadow = (DWORD)GetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], 
		IDC_CAPSHADOWCOLOR)) & TSFCS_COLORMASK;
	int n = (int)SendMessage(GetDlgItem(g_phPages[PAGE_INFO], IDC_CAPSHADOWDEEPSPIN), UDM_GETPOS32, 0, 0);
	if (n < 0) {
		dwCapShadow |= TSFCS_NEGATIVEDEEP;
		n = -n;
	}
	dwCapShadow |= (n << 24) & TSFCS_DEEPMASK;
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_CAPDRAWSHADOW) == BST_UNCHECKED)
		dwCapShadow |= TSFCS_NODEEP;
    return(dwCapShadow);   
}


void _GetPaneFontName(PWSTR pBuff, UINT cchBuff) {
	HWND h = GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFONTNAMECOMBO);
	int len = CB_ERR, n = ComboBox_GetCurSel(h);
	if (n != CB_ERR) {
		len = ComboBox_GetLBTextLen(h, n);
		if (len != CB_ERR && len < (int)cchBuff)
			len = ComboBox_GetLBText(h, n, pBuff);
	}
	if (len == CB_ERR)
		pBuff[0] = L'\0';
}

DWORD _GetPaneFontAttr() {
	WCHAR szBuff[32];
	DWORD dwPaneFontAttr = 8;
	if (GetDlgItemText(g_phPages[PAGE_INFO], 
		IDC_PANEFONTSIZECOMBO, szBuff, SIZEOF_ARRAY(szBuff))) {

		dwPaneFontAttr = _wtol(szBuff);
		if (dwPaneFontAttr < 6) dwPaneFontAttr = 6;
		else if (dwPaneFontAttr > TSFCF_SIZEMASK)
			dwPaneFontAttr = TSFCF_SIZEMASK;
	}
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_PANEFONTBOLD) == BST_CHECKED)
		dwPaneFontAttr |= TSFCF_BOLD;
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_PANEFONTITALIC) == BST_CHECKED)
		dwPaneFontAttr |= TSFCF_ITALIC;
	return(dwPaneFontAttr);
}

inline DWORD _GetPaneTextColor() {
	return((DWORD)GetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], IDC_PANETEXTCOLOR)));
}

DWORD _GetPaneShadow() {
	DWORD dwPaneShadow = (DWORD)GetUxButtonColor(GetDlgItem(g_phPages[PAGE_INFO], 
		IDC_PANESHADOWCOLOR)) & TSFCS_COLORMASK;
	int n = (int)SendMessage(GetDlgItem(g_phPages[PAGE_INFO], 
		IDC_PANESHADOWDEEPSPIN), UDM_GETPOS32, 0, 0);
	if (n < 0) {
		dwPaneShadow |= TSFCS_NEGATIVEDEEP;
		n = -n;
	}
	dwPaneShadow |= (n << 24) & TSFCS_DEEPMASK;
	if (IsDlgButtonChecked(g_phPages[PAGE_INFO], IDC_PANEDRAWSHADOW) == BST_UNCHECKED)
		dwPaneShadow |= TSFCS_NODEEP;
    return(dwPaneShadow);	
}

inline DWORD _GetFlagsPane() {
	BOOL fError;
	DWORD dwPaneHeight = (DWORD)SendMessage(GetDlgItem(g_phPages[PAGE_INFO], 
		IDC_PANEHEIGHTSPIN), UDM_GETPOS32, 0, (LPARAM)&fError);
	if (fError)
		dwPaneHeight = DEFAULT_PANEHEIGHT;

	DWORD dwFlagsPane = 0;
	int n = ComboBox_GetCurSel(GetDlgItem(g_phPages[PAGE_INFO], IDC_PANEFORMATCOMBO));
	if (n > 1 && n <= 5)
		dwFlagsPane = n - 1;
	if (n != 0)
		dwFlagsPane |= TSFP_TASKNUMBER;
	return(MAKELONG(dwFlagsPane, dwPaneHeight));
}


void _GetListFontName(PWSTR pBuff, UINT cchBuff) {
	HWND h = GetDlgItem(g_phPages[PAGE_LIST], IDC_LISTFONTNAMECOMBO);
	int len = CB_ERR, n = ComboBox_GetCurSel(h);
	if (n != CB_ERR) {
		len = ComboBox_GetLBTextLen(h, n);
		if (len != CB_ERR && len < (int)cchBuff)
			len = ComboBox_GetLBText(h, n, pBuff);
	}
	if (len == CB_ERR)
		pBuff[0] = L'\0';
}

DWORD _GetListFontAttr() {
	WCHAR szBuff[32];
	DWORD dwListFontAttr = 8;
	if (GetDlgItemText(g_phPages[PAGE_LIST], 
		IDC_LISTFONTSIZECOMBO, szBuff, SIZEOF_ARRAY(szBuff))) {

		dwListFontAttr = _wtol(szBuff);
		if (dwListFontAttr < 6) dwListFontAttr = 6;
		else if (dwListFontAttr > TSFCF_SIZEMASK)
			dwListFontAttr = TSFCF_SIZEMASK;
	}
	if (IsDlgButtonChecked(g_phPages[PAGE_LIST], IDC_LISTFONTBOLD) == BST_CHECKED)
		dwListFontAttr |= TSFCF_BOLD;
	if (IsDlgButtonChecked(g_phPages[PAGE_LIST], IDC_LISTFONTITALIC) == BST_CHECKED)
		dwListFontAttr |= TSFCF_ITALIC;
	return(dwListFontAttr);
}


inline DWORD _GetHk(UINT idc) {
	DWORD tmp = GetWinHotkey(GetDlgItem(g_phPages[PAGE_HOTKEYS], idc));
	if (!(tmp & 0xff)) tmp = 0;
	return(tmp);
}

inline DWORD _GetTrayAction(UINT idc) {
	int n = ComboBox_GetCurSel(GetDlgItem(g_phPages[PAGE_ADVANCED], idc));
	if (n < 0 || n >= SIZEOF_ARRAY(_puTrayMsg)) n = 0;
	return(_puTrayMsg[n]);
}

DWORD _GetConfigFlags() {
	DWORD dwConfigFlags = 0;
	if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_CFGTOPMOST) == BST_CHECKED)
		dwConfigFlags |= CFGF_TOPMOST;
	//if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_CFGTASKBAR) == BST_CHECKED)
	//	dwConfigFlags |= CFGF_TASKBAR;
	if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_CFGPOSITION) == BST_CHECKED)
		dwConfigFlags |= CFGF_POSITION;
	if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_CFGLASTPAGE) == BST_CHECKED)
		dwConfigFlags |= CFGF_LASTPAGE;
	if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_CFGTASKSWITCHXP) == BST_CHECKED)
		dwConfigFlags |= CFGF_TASKSWITCHXP;
	return(dwConfigFlags);
}
DWORD _GetConfigPos() {
	RECT rcConfig;
    GetWindowRect(g_hwndMain, &rcConfig);
	return(MAKELONG(rcConfig.left, rcConfig.top));
}
DWORD _GetConfigPage() {
	DWORD dwPage = (DWORD)-1;
	HWND h = GetDlgItem(g_hwndMain, IDC_PAGES);
	TVITEMEX tvi;
	tvi.hItem = TreeView_GetSelection(h);
	tvi.mask = TVIF_PARAM;
	if (TreeView_GetItem(h, &tvi))
		dwPage = (DWORD)tvi.lParam;
	return(dwPage);
}

//-----------------------------------------------------------------

BOOL SaveSettings(BOOL fApply) {
	
	HKEY hkey;
	WCHAR szBuff[MAX_DATALEN];
	DWORD tmp;

	// per user settings
	g_hkeyRoot = (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], 
			IDC_PERUSERSETTINGS) == BST_CHECKED) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
		 
	LONG lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, KEY_WRITE, &hkey);
	if (!lErr || (lErr == 2 && 
		!RegCreateKeyEx(HKEY_LOCAL_MACHINE, g_szRegKeyTs, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL))) {
		tmp = (g_hkeyRoot == HKEY_CURRENT_USER) ? 0 : 1;
		RegSetValueEx(hkey, RS_ALLUSERS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
		RegCloseKey(hkey);
	}

	// autostart
	if (!RegOpenKeyEx(g_hkeyRoot, RS_RUN_KEY, 0, KEY_WRITE, &hkey)) {
		if (IsDlgButtonChecked(g_phPages[PAGE_PREFERENCES], IDC_AUTOSTART) == BST_CHECKED) {
			int n = GetModuleFileName(g_hinstExe, szBuff, MAX_DATALEN - 16);
			if (n != 0 && n < MAX_DATALEN - 16) {
				while (n >= 0 && szBuff[n] != L'\\') n--;
				StringCchCopy(szBuff + n + 1, SIZEOF_ARRAY(szBuff) - n, L"TaskSwitchXP.exe");
				RegSetValueEx(hkey, RS_TASKSWITCHXP, 0, REG_SZ, 
					(PBYTE)szBuff, (lstrlen(szBuff) + 1) * sizeof(WCHAR));
			}
		} else 
			RegDeleteValue(hkey, RS_TASKSWITCHXP);
		RegCloseKey(hkey);
	}
	if (!RegOpenKeyEx((g_hkeyRoot == HKEY_CURRENT_USER) 
		? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, RS_RUN_KEY, 0, KEY_WRITE, &hkey)) {
		RegDeleteValue(hkey, RS_TASKSWITCHXP);
		RegCloseKey(hkey);
	}

	// open key
	if (RegOpenKeyEx(g_hkeyRoot, g_szRegKeyTs, 0, KEY_WRITE, &hkey)) {
		if (RegCreateKeyEx(g_hkeyRoot, g_szRegKeyTs, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL))
			return(FALSE);
	}

	tmp = _GetFlags();
	RegSetValueEx(hkey, RS_FLAGS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	_GetCustomIcon(szBuff, SIZEOF_ARRAY(szBuff));
	RegSetValueEx(hkey, RS_CUSTOMICON, 0, REG_SZ, 
		(PBYTE)szBuff, (lstrlen(szBuff) + 1) * sizeof(WCHAR));
	tmp = _GetFlagsEx();
	RegSetValueEx(hkey, RS_FLAGSEX, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
    tmp = _GetFadeIn();
	RegSetValueEx(hkey, RS_FADEIN, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetFadeOut();
	RegSetValueEx(hkey, RS_FADEOUT, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = _GetFlagsPv();	
	RegSetValueEx(hkey, RS_FLAGSPV, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetPvDelay();
	RegSetValueEx(hkey, RS_PVDELAY, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetPvWidth();
	RegSetValueEx(hkey, RS_PVWIDTH, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	DWORD tmp2 = _GetFlagsList();
	RegSetValueEx(hkey, RS_FLAGSLIST, 0, REG_DWORD, (PBYTE)&tmp2, sizeof(DWORD));
	tmp = _GetListWidth(tmp2);
	RegSetValueEx(hkey, RS_LISTWIDTH, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetListHeight(tmp2);
	RegSetValueEx(hkey, RS_LISTHEIGHT, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	_GetListFontName(szBuff, LF_FACESIZE);
	RegSetValueEx(hkey, RS_LISTFONTNAME, 0, REG_SZ, 
		(PBYTE)szBuff, (lstrlen(szBuff) + 1) * sizeof(WCHAR));
	tmp = _GetListFontAttr();
	RegSetValueEx(hkey, RS_LISTFONTATTR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	RegSetValueEx(hkey, RS_TEXTCOLOR, 0, REG_DWORD, 
		(PBYTE)&g_pcrList[LCOLOR_TEXT], sizeof(DWORD));
	RegSetValueEx(hkey, RS_SELTEXTCOLOR, 0, REG_DWORD, 
		(PBYTE)&g_pcrList[LCOLOR_SELTEXT], sizeof(DWORD));
	RegSetValueEx(hkey, RS_SELCOLOR, 0, REG_DWORD, 
		(PBYTE)&g_pcrList[LCOLOR_SEL], sizeof(DWORD));
	RegSetValueEx(hkey, RS_MARKTEXTCOLOR, 0, REG_DWORD, 
		(PBYTE)&g_pcrList[LCOLOR_MARKTEXT], sizeof(DWORD));
	RegSetValueEx(hkey, RS_MARKCOLOR, 0, REG_DWORD, 
		(PBYTE)&g_pcrList[LCOLOR_MARK], sizeof(DWORD));
	RegSetValueEx(hkey, RS_SELMARKTEXTCOLOR, 0, REG_DWORD, 
		(PBYTE)&g_pcrList[LCOLOR_SELMARKTEXT], sizeof(DWORD));
	RegSetValueEx(hkey, RS_SELMARKCOLOR, 0, REG_DWORD, 
		(PBYTE)&g_pcrList[LCOLOR_SELMARK], sizeof(DWORD));

	_GetCapFontName(szBuff, LF_FACESIZE);
	RegSetValueEx(hkey, RS_CAPFONTNAME, 0, REG_SZ, 
		(PBYTE)szBuff, (lstrlen(szBuff) + 1) * sizeof(WCHAR));
	tmp = _GetCapFontAttr();
	RegSetValueEx(hkey, RS_CAPFONTATTR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetCapTextColor();
	RegSetValueEx(hkey, RS_CAPTEXTCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
    tmp = _GetCapShadow();
	RegSetValueEx(hkey, RS_CAPSHADOW, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = _GetFlagsPane();
	RegSetValueEx(hkey, RS_FLAGSPANE, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	_GetPaneFontName(szBuff, LF_FACESIZE);
	RegSetValueEx(hkey, RS_PANEFONTNAME, 0, REG_SZ, 
		(PBYTE)szBuff, (lstrlen(szBuff) + 1) * sizeof(WCHAR));
	tmp = _GetPaneFontAttr();
	RegSetValueEx(hkey, RS_PANEFONTATTR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetPaneTextColor();
	RegSetValueEx(hkey, RS_PANETEXTCOLOR, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
    tmp = _GetPaneShadow();
	RegSetValueEx(hkey, RS_PANESHADOW, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = _GetHk(IDC_EXITHK);
	RegSetValueEx(hkey, RS_EXITHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetHk(IDC_SHOWHIDEHK);
	RegSetValueEx(hkey, RS_SHOWHIDEHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetHk(IDC_CONFIGHK);
	RegSetValueEx(hkey, RS_CONFIGHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetHk(IDC_APPLISTHK);
	RegSetValueEx(hkey, RS_APPLISTHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetHk(IDC_INSTLISTHK);
	RegSetValueEx(hkey, RS_INSTLISTHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetHk(IDC_MINIMIZETRAYHK);
	RegSetValueEx(hkey, RS_MINIMIZETRAYHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetHk(IDC_RESTORETRAYHK);
	RegSetValueEx(hkey, RS_RESTORETRAYHK, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = _GetTrayAction(IDC_TRAYMENUCOMBO);
	RegSetValueEx(hkey, RS_TRAYMENU, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetTrayAction(IDC_TRAYCONFIGCOMBO);
	RegSetValueEx(hkey, RS_TRAYCONFIG, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetTrayAction(IDC_TRAYAPPCOMBO);
	RegSetValueEx(hkey, RS_TRAYAPPLIST, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetTrayAction(IDC_TRAYINSTCOMBO);
	RegSetValueEx(hkey, RS_TRAYINSTLIST, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	tmp = _GetConfigFlags();
	RegSetValueEx(hkey, RS_CONFIGFLAGS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetConfigPos();
	RegSetValueEx(hkey, RS_CONFIGPOS, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));
	tmp = _GetConfigPage();
	RegSetValueEx(hkey, RS_CONFIGPAGE, 0, REG_DWORD, (PBYTE)&tmp, sizeof(DWORD));

	RegSetValueEx(hkey, RS_SHOWDELAY, 0, REG_DWORD, (PBYTE)&_dwShowDelay, sizeof(DWORD));

	WCHAR szCurColors[32];
	if (GetCurrentThemeName(szBuff, MAX_PATH, 
		szCurColors, SIZEOF_ARRAY(szCurColors), NULL, 0) != S_OK) {
		szBuff[0] = L'\0';
		szCurColors[0] = L'\0';
	}
	RegSetValueEx(hkey, RS_THEMEFILE, 0, REG_SZ, 
		(PBYTE)szBuff, (lstrlen(szBuff) + 1) * sizeof(WCHAR));
	RegSetValueEx(hkey, RS_COLORTHEME, 0, REG_SZ, 
		(PBYTE)szCurColors, (lstrlen(szCurColors) + 1) * sizeof(WCHAR));

	// exclusions
	SHDeleteKey(hkey, RS_EXCLUSIONS_SUBKEY);

	HKEY hkeyExcl;
	if (RegCreateKeyEx(hkey, RS_EXCLUSIONS_SUBKEY, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyExcl, NULL)) {
		RegCloseKey(hkey);
		return(FALSE);
	}

	int n;
	HWND h = GetDlgItem(g_phPages[PAGE_EXCLUSIONS], IDC_EXCLLIST);
	for (n = 0; n < (int)_vExclList.size(); n++) {
		TSEXCLUSION& tse = _vExclList[n];
		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"ExclName%u", n);
		RegSetValueEx(hkeyExcl, szBuff, 0, REG_SZ, (PBYTE)tse.szExclName, 
			(lstrlen(tse.szExclName) + 1) * sizeof(WCHAR));
		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"ExeName%u", n);
		RegSetValueEx(hkeyExcl, szBuff, 0, REG_SZ, (PBYTE)tse.szExeName, 
			(lstrlen(tse.szExeName) + 1) * sizeof(WCHAR));
		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"ClassName%u", n);
		RegSetValueEx(hkeyExcl, szBuff, 0, REG_SZ, (PBYTE)tse.szClassName, 
			(lstrlen(tse.szClassName) + 1) * sizeof(WCHAR));
		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"Caption%u", n);
		RegSetValueEx(hkeyExcl, szBuff, 0, REG_SZ, (PBYTE)tse.szCaption, 
			(lstrlen(tse.szCaption) + 1) * sizeof(WCHAR));
		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"Flags%u", n);
		RegSetValueEx(hkeyExcl, szBuff, 0, REG_DWORD, (PBYTE)&tse.dwFlags, sizeof(DWORD));
	}
	RegSetValueEx(hkeyExcl, RS_EXCLCOUNT, 0, REG_DWORD, (PBYTE)&n, sizeof(DWORD));

	RegCloseKey(hkeyExcl);

	// save language file
	h = GetDlgItem(g_phPages[PAGE_PREFERENCES], IDC_LANGUAGECOMBO);
	n = ComboBox_GetCurSel(h);
	if (n != CB_ERR) {
		if (ComboBox_GetItemData(h, n) && ComboBox_GetLBTextLen(h, n) < MAX_PATH) {
			if (ComboBox_GetLBText(h, n, szBuff) == CB_ERR)
				szBuff[0] = L'\0';
		} else 
			szBuff[0] = L'\0';
	} else 
		szBuff[0] = L'\0';
	RegSetValueEx(hkey, RS_LANGFILE, 0, REG_SZ, 
		(PBYTE)szBuff, (lstrlen(szBuff) + 1) * sizeof(WCHAR));

	RegCloseKey(hkey);

	if (fApply) {
		SetWindowPos(g_hwndMain, 
			(IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_CFGTOPMOST) == BST_CHECKED)
			? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		UpdateLangFile(szBuff);
	}

	if (IsDlgButtonChecked(g_phPages[PAGE_ADVANCED], IDC_CFGTASKSWITCHXP) == BST_CHECKED)
		StartTaskSwitchXP();

	return(TRUE);
}

//-----------------------------------------------------------------

BOOL __WriteString(HANDLE hFile, PCWSTR szBuff) {
	DWORD dwWrite, dwWritten;
	dwWrite = lstrlen(szBuff) * sizeof(WCHAR);
	WriteFile(hFile, szBuff, dwWrite, &dwWritten, NULL);
	return((BOOL)(dwWrite == dwWritten));
}

BOOL _WriteString(HANDLE hFile, PCWSTR szText, PCWSTR pszComment =NULL) {
	
	WCHAR szBuff[2 * MAX_DATALEN + 128];
	szBuff[0] = L'\"';
	int n = StringToCfgString(szText, szBuff + 1, 2 * MAX_DATALEN);
	szBuff[n + 1] = L'\"';
	szBuff[n + 2] = L'\0';
	if (pszComment) {
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), L" ; ");
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), pszComment);
	}
	StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), L"\r\n");
	return(__WriteString(hFile, szBuff));
}

BOOL _WriteDWORD(HANDLE hFile, DWORD dw, PCWSTR pszComment =NULL) {
	WCHAR szBuff[128];
	if (pszComment) {
		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"%0 8X ; %s\r\n", dw, pszComment);
	} else {
		StringCchPrintf(szBuff, SIZEOF_ARRAY(szBuff), L"%0 8X\r\n", dw);
	}
	return(__WriteString(hFile, szBuff));
}


BOOL ExportSettings(PCWSTR pszFile) {

	_ASSERT(pszFile);

	BOOL fSuccess = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	WCHAR szBuff[MAX_DATALEN];

	static const WCHAR s_szCfgHead[] = 
		L"; TaskSwitchXP Pro 2.0 configuration file\r\n"
		L"; Copyright (c) 2004-2005 by Alexander Avdonin\r\n\r\n"
		L"; Don't edit this file manually!!!\r\n\r\n\r\n"
		L"[TaskSwitchXP Pro]\r\n\r\n";

	__try {
		hFile = CreateFile(pszFile, GENERIC_WRITE, 0, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) __leave;

		if (!__WriteString(hFile, s_szCfgHead)) __leave;
		if (!_WriteDWORD(hFile, _GetAllUsers(), RS_ALLUSERS)) __leave;
		if (!_WriteDWORD(hFile, _GetAutoStart(), L"AutoStart")) __leave;

		if (!_WriteDWORD(hFile, _GetFlags(), RS_FLAGS)) __leave;
		_GetCustomIcon(szBuff, SIZEOF_ARRAY(szBuff));
		if (!_WriteString(hFile, szBuff, RS_CUSTOMICON)) __leave;
		if (!_WriteDWORD(hFile, _GetFlagsEx(), RS_FLAGSEX)) __leave;
		if (!_WriteDWORD(hFile, _GetFadeIn(), RS_FADEIN)) __leave;
		if (!_WriteDWORD(hFile, _GetFadeOut(), RS_FADEOUT)) __leave;
		if (!_WriteDWORD(hFile, _GetFlagsPv(), RS_FLAGSPV)) __leave;
		if (!_WriteDWORD(hFile, _GetPvDelay(), RS_PVDELAY)) __leave;
		if (!_WriteDWORD(hFile, _GetPvWidth(), RS_PVWIDTH)) __leave;
		DWORD tmp = _GetFlagsList();
		if (!_WriteDWORD(hFile, tmp, RS_FLAGSLIST)) __leave;
		if (!_WriteDWORD(hFile, _GetListWidth(tmp), RS_LISTWIDTH)) __leave;
		if (!_WriteDWORD(hFile, _GetListHeight(tmp), RS_LISTHEIGHT)) __leave;

		_GetListFontName(szBuff, LF_FACESIZE);
		if (!_WriteString(hFile, szBuff, RS_LISTFONTNAME)) __leave;
		if (!_WriteDWORD(hFile, _GetListFontAttr(), RS_LISTFONTATTR)) __leave;

		if (!_WriteDWORD(hFile, g_pcrList[LCOLOR_TEXT], RS_TEXTCOLOR)) __leave;
		if (!_WriteDWORD(hFile, g_pcrList[LCOLOR_SELTEXT], RS_SELTEXTCOLOR)) __leave;
		if (!_WriteDWORD(hFile, g_pcrList[LCOLOR_SEL], RS_SELCOLOR)) __leave;
		if (!_WriteDWORD(hFile, g_pcrList[LCOLOR_MARKTEXT], RS_MARKTEXTCOLOR)) __leave;
		if (!_WriteDWORD(hFile, g_pcrList[LCOLOR_MARK], RS_MARKCOLOR)) __leave;
		if (!_WriteDWORD(hFile, g_pcrList[LCOLOR_SELMARKTEXT], RS_SELMARKTEXTCOLOR)) __leave;
		if (!_WriteDWORD(hFile, g_pcrList[LCOLOR_SELMARK], RS_SELMARKCOLOR)) __leave;

		_GetCapFontName(szBuff, LF_FACESIZE);
		if (!_WriteString(hFile, szBuff, RS_CAPFONTNAME)) __leave;
		if (!_WriteDWORD(hFile, _GetCapFontAttr(), RS_CAPFONTATTR)) __leave;
		if (!_WriteDWORD(hFile, _GetCapTextColor(), RS_CAPTEXTCOLOR)) __leave;
		if (!_WriteDWORD(hFile, _GetCapShadow(), RS_CAPSHADOW)) __leave;

		if (!_WriteDWORD(hFile, _GetFlagsPane(), RS_FLAGSPANE)) __leave;
		_GetPaneFontName(szBuff, LF_FACESIZE);
		if (!_WriteString(hFile, szBuff, RS_PANEFONTNAME)) __leave;
		if (!_WriteDWORD(hFile, _GetPaneFontAttr(), RS_PANEFONTATTR)) __leave;
		if (!_WriteDWORD(hFile, _GetPaneTextColor(), RS_PANETEXTCOLOR)) __leave;
		if (!_WriteDWORD(hFile, _GetPaneShadow(), RS_PANESHADOW)) __leave;
		
		if (!_WriteDWORD(hFile, _GetHk(IDC_EXITHK), RS_EXITHK)) __leave;
		if (!_WriteDWORD(hFile, _GetHk(IDC_SHOWHIDEHK), RS_SHOWHIDEHK)) __leave;
		if (!_WriteDWORD(hFile, _GetHk(IDC_CONFIGHK), RS_CONFIGHK)) __leave;
		if (!_WriteDWORD(hFile, _GetHk(IDC_APPLISTHK), RS_APPLISTHK)) __leave;
		if (!_WriteDWORD(hFile, _GetHk(IDC_INSTLISTHK), RS_INSTLISTHK)) __leave;
		if (!_WriteDWORD(hFile, _GetHk(IDC_MINIMIZETRAYHK), RS_MINIMIZETRAYHK)) __leave;
		if (!_WriteDWORD(hFile, _GetHk(IDC_RESTORETRAYHK), RS_RESTORETRAYHK)) __leave;
		if (!_WriteDWORD(hFile, _GetTrayAction(IDC_TRAYMENUCOMBO), RS_TRAYMENU)) __leave;
		if (!_WriteDWORD(hFile, _GetTrayAction(IDC_TRAYCONFIGCOMBO), RS_TRAYCONFIG)) __leave;
		if (!_WriteDWORD(hFile, _GetTrayAction(IDC_TRAYAPPCOMBO), RS_TRAYAPPLIST)) __leave;
		if (!_WriteDWORD(hFile, _GetTrayAction(IDC_TRAYINSTCOMBO), RS_TRAYINSTLIST)) __leave;
		if (!_WriteDWORD(hFile, _GetConfigFlags(), RS_CONFIGFLAGS)) __leave;
		if (!_WriteDWORD(hFile, _dwShowDelay, RS_SHOWDELAY)) __leave;

		if (!__WriteString(hFile, L"\r\n\r\n[Exclusions]\r\n\r\n")) __leave;
		for (int n = 0; n < (int)_vExclList.size(); n++) {
			TSEXCLUSION& tse = _vExclList[n];
			if (!_WriteString(hFile, tse.szExclName, L"ExclName")) __leave;
			if (!_WriteString(hFile, tse.szExeName, L"ExeName")) __leave;
			if (!_WriteString(hFile, tse.szClassName, L"ClassName")) __leave;
			if (!_WriteString(hFile, tse.szCaption, L"Caption")) __leave;
			if (!_WriteDWORD(hFile, tse.dwFlags, L"Flags\r\n")) __leave;			
		}
		fSuccess = TRUE;
	}
	__finally {
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
	}
	if (!fSuccess && hFile != INVALID_HANDLE_VALUE)
		DeleteFile(pszFile);

	return(fSuccess);
}

//-----------------------------------------------------------------
