// lang.h

#define MAX_LANGLEN				256


BOOL LoadLangFile(LPCWSTR);
BOOL UpdateLangFile(LPCWSTR);
int LangLoadString(UINT, LPWSTR, int);
BOOL IsLangDefault();

BOOL InitThreadLang2();
void DestroyThreadLang2();
