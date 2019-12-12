// TaskSwitchXP.h

#ifndef __TASKSWITCHXP_H__
#define __TASKSWITCHXP_H__

//-----------------------------------------------------------

#include "registry.h"

//-----------------------------------------------------------

#define XMARGIN_LIST					12
#define YMARGIN_LIST					14
#define XMARGIN_PREVIEW					24
#define YMARGIN_PREVIEW					16
#define XMARGIN_ICON					16
#define YMARGIN_ICON					16
#define XMARGIN_SMICON					10
#define YMARGIN_SMICON					8
#define XMARGIN_PANE					16
#define YMARGIN_PANE					4
#define XMARGIN_SEL						4
#define YMARGIN_SEL						4

#define XMARGIN_SMSEL					2
#define YMARGIN_SMSEL					2

#define CX_ICON							32
#define CY_ICON							32
#define CX_SMICON						16
#define CY_SMICON						16

#define TIMER_UPDATE					1
#define TIMER_PREVIEW					4
#define TIMER_INFOTIP					44
#define TIMER_FADEIN					444
#define TIMER_FADEOUT					888
#define TIMER_TSXPSHOW					4444
#define TIMER_RELOADICONS				545

#define FADE_DELAY						10
#define RELOADICONS_DELAY				10000

#define IDH_NEXTTASK					1000
#define IDH_PREVTASK					1001
#define IDH_WINNEXTTASK					1002
#define IDH_WINPREVTASK					1003
#define IDH_INSTNEXT					1004
#define IDH_INSTPREV					1005

#define IDH_STNEXTTASK					1006
#define IDH_STINEXTTASK					1007
#define IDH_STTRAYNEXT					1008
#define IDH_STITRAYNEXT					1009

#define IDH_SWITCH						1010
#define IDH_ESCAPE						1011

#define IDH_EXIT						1100
#define IDH_SHOWHIDE					1101
#define IDH_CONFIG						1102

#define IDH_RESTORETRAY					1200
#define IDH_MINIMIZETRAY				1201


#define WM_TASKSWITCH					(WM_USER + 984)
//#define WM_MYMOUSEMOVE					(WM_USER + 20)

/*#define MAX_CAPTION						128
#define MAX_CLASSNAME					128
#define MAX_FILENAME					256
#define MAX_FILEPATH					1024*/

#define MAX_WNDPV						16

//-----------------------------------------------------------

#define TI_NOPREVIEW					0x01000000
#define TI_NOCLOSE						0x02000000
#define TI_TRAYWINDOW					0x04000000
#define TI_SELECTED						0x10000000
#define TI_DELETE						0x20000000

typedef struct TASKINFO {

	PWSTR pszExePath;
	int nExeName;
	DWORD dwProcessId;

	HWND hwndOwner;
	HWND phWndPv[MAX_WNDPV];
	int nWndPv;

	HICON hIcon;
	HICON hIconSm;

	WCHAR szCaption[MAX_CAPTION];
	WCHAR szTaskInfo[MAX_TASKINFO];

	HBITMAP hbitmapPv;
	DWORD dwFlags;

} *PTASKINFO;


typedef struct TSEXCLUSION {
	WCHAR szExeName[MAX_FILENAME];
	WCHAR szClassName[MAX_CLASSNAME];
	WCHAR szCaption[MAX_CAPTION];
	DWORD dwFlags;
} *PTSEXCLUSION;

//-----------------------------------------------------------

extern DWORD g_dwShowDelay;
extern DWORD g_dwFlags;
extern DWORD g_dwFlagsPv;
extern UINT g_uTrayMenu;
extern UINT g_uTrayConfig;
extern UINT g_uTrayNext;
extern UINT g_uTrayPrev;


extern HWND g_hwndTs;
extern PTASKINFO g_pTs;
extern int g_nTs;
extern int g_nCurTs;


extern int g_nFirstTs;
extern int g_nSelTs;
extern int g_nIconsX;
extern int g_nIconsY;
extern HBITMAP g_hbitmapTs;
extern DWORD g_dwPvDelay;
extern HANDLE g_hheapWnd;
extern DWORD g_dwFlagsEx;
extern DWORD g_dwFlagsList;
extern RECT g_rcPvEx;
extern RECT g_rcList;
extern RECT g_rcPane;
extern HWND g_hwndInfo;
extern HWND g_hwndShell;
extern HDESK g_hDesk;
extern HANDLE g_hThreadTs;

//-----------------------------------------------------------

BOOL InitLanguage(BOOL fFirst =TRUE);
BOOL LoadSettings();
void DestroySettings();
BOOL ShowTaskSwitchWnd(UINT);
BOOL InitPreviewThread();
void DestroyPreviewThread();

BOOL CheckTask(int nTask, HWND* phWnd =NULL);
void ReplaceAltTab(DWORD);


BOOL InitTsBackground(HWND);
BOOL DrawTaskList();
BOOL UpdateInfoTip(int);
BOOL StartDrawTaskPreview(int);
void SwitchToCurTask(HWND);
void SwitchToCurTask2(HWND);
//void StartFadeOut(HWND);
void ShowHideInfoTip(HWND);
BOOL CheckColorTheme();
void CheckDefaultColors();

//-----------------------------------------------------------

#endif // __TASKSWITCHXP_H__
