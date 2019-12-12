// cfgfile.h

#ifndef __CFGFILE_H__
#define __CFGFILE_H__

int StringToCfgString(PCWSTR, PWSTR, int);
BOOL _SkipSpaces(LPCWSTR, UINT, UINT &);
BOOL _WaitNextLine(LPCWSTR, UINT, UINT &);
BOOL _WaitNextSection(LPCWSTR, UINT, UINT &);
BOOL _IsDelimitWCHAR(WCHAR);
UINT _GetID(LPCWSTR, UINT, UINT &);
BOOL _GetDWORD(LPCWSTR, UINT, UINT &, DWORD*);
BOOL _GetString(LPCWSTR, UINT, UINT &, PWSTR, UINT);

#endif // __CFGFILE_H__