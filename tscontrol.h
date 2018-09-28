// tscontrol.h

#ifndef __TSCONTROL_H__
#define __TSCONTROL_H__

BOOL SelectTask(HWND, int, BOOL fRedraw =FALSE);
BOOL MultiSelectTask(HWND, int, int, BOOL);
BOOL MultiSelectTasks(HWND, int, int, BOOL);
BOOL MultiSelectInstances(HWND, BOOL);
BOOL MultiSelectAll(HWND, BOOL);

BOOL CheckTask(int, HWND*);
void UpdateTask(HWND, int);
void ShowTaskMenu(HWND, POINT);
void ShowCommonMenu(HWND, POINT);
void DeleteTsItem(int);

void RemoveSelected(HWND);
void MinimizeSelected(HWND);
void FlipToTraySelected(HWND);
void MaxRestoreSelected(HWND, WPARAM);
void CloseSelected(HWND);
void TerminateSelected(HWND);

void ExploreExePath(PCWSTR);


#define RO_CASCADE				0
#define RO_TILEVERTICALLY		1
#define RO_TILEHORIZONTALLY		2

void ReorderSelected(HWND, UINT);
void SortTaskList(HWND, BOOL);
void SortTaskListByModuleName(HWND);

#endif // __TSCONTROL_H__
