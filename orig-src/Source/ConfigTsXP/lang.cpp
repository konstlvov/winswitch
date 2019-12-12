// lang.cpp

#include "stdafx.h"
#include "main.h"
#include "resource.h"
#include "generic.h"
#include "cfgfile.h"
#include "lang.h"

#include <strsafe.h>

//-----------------------------------------------------------------

struct LANGPAIR {
	UINT uID;
	stringW sText;
};

UINT _FindID(UINT);
BOOL _LoadLangFile(LPCWSTR);
void _UpdateLanguage();

vector<LANGPAIR> _langPairs;
stringW _sLangFile;

//-----------------------------------------------------------------
//-----------------------------------------------------------------

UINT _FindID(UINT uID) {

	UINT nLeft = 0, nRight = (UINT)_langPairs.size();
	
	while (nLeft < nRight) {

		UINT nMid = (nLeft + nRight) / 2;

		if (_langPairs[nMid].uID == uID)
			return(nMid);

		if (_langPairs[nMid].uID < uID)
			nLeft = nMid + 1;
		else nRight = nMid;
	}
    return((UINT)-1);
}

//-----------------------------------------------------------------

int LangLoadString(UINT uID, LPWSTR lpBuffer, int nBufferMax) {

    if (!_langPairs.empty()) {
		UINT i = _FindID(uID);
		if (i != (UINT)-1) {
			StringCchCopy(lpBuffer, nBufferMax, _langPairs[i].sText.c_str());
			return(lstrlen(lpBuffer));
		}
	} 
	return(LoadString(g_hinstExe, uID, lpBuffer, nBufferMax)); // default language
}

int LangLoadString(UINT uID, stringW& s) {

	int nLength = 0;
	if (!_langPairs.empty()) {
		UINT i = _FindID(uID);
		if (i != (UINT)-1) {
			nLength = (int)_langPairs[i].sText.length();
			if (nLength > 0) {
				s = _langPairs[i].sText;
				return(nLength);
			}
		}
	}

	// default language
	WCHAR szBuff[MAX_LANGLEN];
	nLength = LoadString(g_hinstExe, uID, szBuff, MAX_LANGLEN);
	s = szBuff;
	return(nLength);
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

BOOL _LoadLangStrings(LPCWSTR pchFile, UINT nLength) {

	_langPairs.clear();
	LANGPAIR lp;
	UINT pos = 0;
    
	while (_SkipSpaces(pchFile, nLength, pos)) {

		lp.uID = _GetID(pchFile, nLength, pos);
		if (lp.uID >= 31000) {
			if (!_WaitNextLine(pchFile, nLength, pos))
				return(TRUE);
			continue;
		}

		if (!_SkipSpaces(pchFile, nLength, pos) && 
			pchFile[pos] != L'=')
			return(FALSE);
		pos++;
		if (!_SkipSpaces(pchFile, nLength, pos) && 
			pchFile[pos] != L'\"')
			return(FALSE);
		pos++;

		lp.sText.clear();
		for (int i = 0; i < MAX_LANGLEN; i++) {
			if (pos >= nLength)
				return(FALSE);
			WCHAR ch = pchFile[pos++];
			if (ch == L'\"')
				break;

			if (ch == L'\\') {

				ch = pchFile[pos++];

				switch(ch) {
					case L'r': lp.sText += L'\r'; break;
					case L'n': lp.sText += L'\n'; break;
					case L't': lp.sText += L'\t'; break;
					case L'\\': lp.sText += L'\\'; break;
					case L'\"':	lp.sText += L'\"'; break;
					default:
						lp.sText += L'\\';
						if (i < MAX_LANGLEN)
							lp.sText += ch;
						break;
				}
			} else 
				lp.sText += ch;
		}
		if (i >= MAX_LANGLEN)
			return(FALSE);
		_langPairs.push_back(lp);
	}
	return(TRUE);
}

//-----------------------------------------------------------------

bool _CompareLangID(LANGPAIR lp1, LANGPAIR lp2) {
	return(lp2.uID > lp1.uID);
}

//-----------------------------------------------------------------

BOOL _LoadLangFile(LPCWSTR pszLangFile) {

	BOOL fSuccess = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE,
		hFileMap = NULL;
	LPWSTR pchFile = NULL;
	
	__try {
		hFile = CreateFile(pszLangFile, GENERIC_READ, 
			FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			__leave;

		DWORD dwFileSize = GetFileSize(hFile, NULL);

		hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 
			0, dwFileSize, NULL);
		if (!hFileMap)
			__leave;

		pchFile = (LPWSTR)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
		if (!pchFile)
			__leave;
        
		if (*pchFile == 0xFEFF) {
			fSuccess = _LoadLangStrings(pchFile + 1, 
				dwFileSize / sizeof(WCHAR));
		}
	}
	__finally {
		if (pchFile)
			UnmapViewOfFile((LPCVOID)pchFile);
		if (hFileMap)
			CloseHandle(hFileMap);
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);

		if (!fSuccess)
			_langPairs.clear();
	}
	sort(_langPairs.begin(), _langPairs.end(), _CompareLangID);
	return(fSuccess);
}

//-----------------------------------------------------------------

BOOL LoadLangFile(LPCWSTR pszLangFile) {

	_langPairs.clear();
	_sLangFile.clear();

	if (!pszLangFile || pszLangFile[0] == L'\0') // default language
		return(TRUE);

	WCHAR szBuff[MAX_FILEPATH + 16] = L"";
	int n = GetModuleFileName(g_hinstExe, szBuff, MAX_FILEPATH);
	if (n > 0 && n < MAX_FILEPATH) {
		while (n >= 0 && szBuff[n] != L'\\') n--;
		StringCchCopy(szBuff + n + 1, 
			SIZEOF_ARRAY(szBuff) - n, L"lang\\");
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), pszLangFile);
		//if (lstrcmpi(szBuff + lstrlen(szBuff) - 4, L".lng"))
		StringCchCat(szBuff, SIZEOF_ARRAY(szBuff), L".lng");
	}

	BOOL fSuccess = _LoadLangFile(szBuff);
	if (fSuccess)
		_sLangFile = pszLangFile;
	return(fSuccess);
}

//-----------------------------------------------------------------

BOOL UpdateLangFile(LPCWSTR pszLangFile) {

	BOOL fChanged = FALSE;
	if (pszLangFile) {
		if (lstrcmpi(_sLangFile.c_str(), pszLangFile)) {
			if (!LoadLangFile(pszLangFile))
				ReportError(g_hwndMain, IDS_ERR_LANGFILE);
			fChanged = TRUE;
		}
	} else {
		if (_sLangFile.length() > 0 || _langPairs.size() > 0) {
			LoadLangFile(NULL);
			fChanged = TRUE;
		}
	}
	if (fChanged) {
		SendMessage(g_hwndMain, WM_LANGCHANGED, 0, 0);
		for (int i = 0; i < C_PAGES; i++) {
			SendMessage(g_phPages[i], WM_LANGCHANGED, 0, 0);
		}
	}
	return(fChanged);
}

//-----------------------------------------------------------------

BOOL IsLangDefault() {
	return((BOOL)(_langPairs.size() == 0));
}