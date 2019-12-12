// list.h

#define XMARGIN_SEL						4
#define YMARGIN_SEL						4

#define XMARGIN_SMSEL					2
#define YMARGIN_SMSEL					2

#define CX_ICON							32
#define CY_ICON							32
#define CX_SMICON						16
#define CY_SMICON						16


INT_PTR CALLBACK Dlg_ListProc(HWND, UINT, WPARAM, LPARAM);
void List_OnCommand(HWND, int, HWND, UINT);


#define LCOLOR_TEXT						0
#define LCOLOR_SELTEXT					1
#define LCOLOR_SEL						2
#define LCOLOR_MARKTEXT					3
#define LCOLOR_MARK						4
#define LCOLOR_SELMARKTEXT				5
#define LCOLOR_SELMARK					6

extern COLORREF g_pcrList[7];