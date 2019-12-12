// main.h

extern const WCHAR g_szRegKeyTs[];
extern const WCHAR g_szTaskSwitch[];
extern const WCHAR g_szMainWnd[];
extern const WCHAR g_szTaskSwitchWnd[];
//extern const WCHAR g_szTsBkClass[];
extern const WCHAR g_szWindowName[];
extern HINSTANCE g_hinstExe;
extern HWND g_hwndMain;

typedef BOOL (WINAPI *ISHUNGAPPWINDOW)(HWND);
extern ISHUNGAPPWINDOW g_pfnIsHungAppWindow;

//typedef BOOL (WINAPI *ENDTASK)(HWND, BOOL, BOOL);
//extern ENDTASK g_pfnEndTask;

#define WM_EXTMOUSE						(WM_USER + 42)
#define WM_FLIPTOTRAY					(WM_USER + 43)
#define WM_UNFLIPFROMTRAY				(WM_USER + 44)
#define WM_GETTRAYWINDOWS				(WM_USER + 45)

#define MAX_WNDTRAY						50

typedef struct WNDTRAYINFO {
    HWND hwnd;
    UINT uID;
	HICON hIconSm;
	RECT rcWnd;
	BOOL fAnimate;
} *PWNDTRAYINFO;


BOOL ShowTrayIcon(BOOL);
BOOL EnableExtMouse(BOOL, BOOL);
BOOL ConfigTaskSwitchXP(LPCWSTR szParams =NULL);

#define TIMER_SETANIMATION				32
#define TIMER_RIGHTUP					88
#define TIMER_CHECKCOLORS				99
#define TIMER_CHECKTRAYWND				100
#define TIMER_CLOSEDESK					110