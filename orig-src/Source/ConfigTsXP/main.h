// main.h

#ifndef __MAIN_H__
#define __MAIN_H__

#include "..\\TaskSwitchXP\\registry.h"

typedef struct TSEXCLUSION {
	WCHAR szExclName[MAX_EXCLNAME];
	WCHAR szExeName[MAX_FILENAME];
	WCHAR szClassName[MAX_CLASSNAME];
	WCHAR szCaption[MAX_CAPTION];
	DWORD dwFlags;
} *PTSEXCLUSION;


#define C_PAGES						9

#define PAGE_PREFERENCES			0
#define PAGE_APPEARANCE				1
#define PAGE_LIST					2
#define PAGE_PREVIEW				3
#define PAGE_INFO					4
#define PAGE_HOTKEYS				5
#define PAGE_EXCLUSIONS				6
#define PAGE_ADVANCED				7
#define PAGE_ABOUT					8


#define TIMER_CHECKCOLORS				99

extern HINSTANCE g_hinstExe;
extern HWND g_hwndMain;
extern HWND g_phPages[C_PAGES];
extern const WCHAR g_szMainWnd[];
extern const WCHAR g_szRegKeyTs[];

extern int g_nComboItemHeight;
extern DWORD g_dwCmdLine;


void Main_OnCommand(HWND, int, HWND, UINT);
BOOL StartTaskSwitchXP();
void SelectPage(int nPage);

#endif // __MAIN_H__