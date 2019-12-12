// registry.h

//#ifndef __REGISTRY_H__
//#define __REGISTRY_H__

void LoadSettings(BOOL fDefaults =FALSE);
BOOL SaveSettings(BOOL fApply =FALSE);
BOOL ExportSettings(PCWSTR);
int ImportSettings(PCWSTR);
BOOL SilentInstall(BOOL fDefaults =FALSE);
BOOL InitLanguage();

BOOL CheckColorTheme();
void CheckDefaultColors();

extern HKEY g_hkeyRoot;
extern DWORD g_dwFlagsEx;

//#endif // __REGISTRY_H__