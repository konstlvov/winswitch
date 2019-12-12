// lang.cpp

#include "stdafx.h"
#include "generic.h"
#include "main.h"
#include "resource.h"
#include "lang.h"

//-----------------------------------------------------------------

typedef struct LANGPAIR {
	UINT uID;
	LPWSTR pszText;
} *PLANGPAIR;


PLANGPAIR g_pLp					= NULL;
UINT g_cLp						= 0;

LPWSTR _pszLangFile				= NULL;

CRITICAL_SECTION g_csLp; // there are ONLY 2 threads!!!

//-----------------------------------------------------------------

UINT _FindID(UINT uID) {

	UINT nLeft = 0, nRight = g_cLp;

	while (nLeft < nRight) {

		UINT nMid = (nLeft + nRight) / 2;

		if (g_pLp[nMid].uID == uID)
			return(nMid);

		if (g_pLp[nMid].uID < uID)
			nLeft = nMid + 1;
		else nRight = nMid;
	}
    return((UINT)-1);
}

//-----------------------------------------------------------------

int LangLoadString(UINT uID, LPWSTR lpBuffer, int nBufferMax) {

	if (g_pLp) {
		
		EnterCriticalSection(&g_csLp);

		UINT i = _FindID(uID);
		if (i != (UINT)-1) {
			lstrcpyn(lpBuffer, (LPCWSTR)g_pLp[i].pszText, nBufferMax);
			lpBuffer[nBufferMax - 1] = L'\0';
			LeaveCriticalSection(&g_csLp);
			return(lstrlen(lpBuffer));
		}
		LeaveCriticalSection(&g_csLp);
	}
	return(LoadString(g_hinstExe, uID, lpBuffer, nBufferMax)); // default language
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

BOOL _IsDelimitWCHAR(WCHAR ch) {
	return(ch == L' ' || ch == L'\n' || 
		ch == L'\r' || ch == L'\0' || ch == L'\t');
}

//-----------------------------------------------------------------

UINT _GetID(LPCWSTR pch, UINT nLength, UINT &pos) {

	UINT uID = 0;
	for (; pos < nLength; pos++) {
		
		WCHAR ch = pch[pos];
	
		if (ch == L'=' || _IsDelimitWCHAR(ch))
			return(uID);
        if (ch < L'0' || ch > L'9')
			break;

		uID = uID * 10 + (UINT)(ch - L'0');
	}
	return((UINT)-1);
}

//-----------------------------------------------------------------

BOOL _WaitNextLine(LPCWSTR pch, UINT nLength, UINT &pos) {
	for (; pos < nLength; pos++)
		if (pch[pos] == L'\n')
			return(TRUE);
	return(FALSE);
}

//-----------------------------------------------------------------

BOOL _SkipSpaces(LPCWSTR pch, UINT nLength, UINT &pos) {

	for (; pos < nLength; pos++) {
		WCHAR ch = pch[pos];
		if (!_IsDelimitWCHAR(ch)) {
			if (ch != L';')
				return(TRUE);
			if (!_WaitNextLine(pch, nLength, pos))
				return(FALSE);
		}
	}
	return(FALSE);
}

//-----------------------------------------------------------------

#define LP_ALLOCUNIT					64


BOOL _LoadLangStrings(LPCWSTR pchFile, UINT nLength) {

	BOOL fSuccess = FALSE;
	HANDLE hheap = GetProcessHeap();

	__try {

		g_cLp = 0;
		if (g_pLp)
			HeapFree(hheap, 0, g_pLp);
		g_pLp = (PLANGPAIR)HeapAlloc(hheap, 0, sizeof(LANGPAIR) * LP_ALLOCUNIT);
		if (!g_pLp)
			__leave;

		UINT pos = 0, uID, i;

		while (_SkipSpaces(pchFile, nLength, pos)) {

			uID = _GetID(pchFile, nLength, pos);

			if (uID < 31000 || uID > 0xffff) {
				if (!_WaitNextLine(pchFile, nLength, pos))
					__leave;
                continue;
			}

			if (!_SkipSpaces(pchFile, nLength, pos) && 
				pchFile[pos] != L'=')
				__leave;
			pos++;
			if (!_SkipSpaces(pchFile, nLength, pos) && 
				pchFile[pos] != L'\"')
				__leave;
			pos++;

			WCHAR szBuff[MAX_LANGLEN];
			for (i = 0; i < MAX_LANGLEN; i++) {
				if (pos >= nLength)
					__leave;
				WCHAR ch = pchFile[pos++];
				if (ch == L'\"')
					break;

				if (ch == L'\\') {

					ch = pchFile[pos++];

					switch(ch) {
						case L'r': szBuff[i] = L'\r'; break;
						case L'n': szBuff[i] = L'\n'; break;
						case L't': szBuff[i] = L'\t'; break;
						case L'\\': szBuff[i] = L'\\'; break;
						case L'\"':	szBuff[i] = L'\"'; break;
						default:
							szBuff[i++] = L'\\';
							if (i < MAX_LANGLEN)
								szBuff[i] = ch;
							break;
					}
				} else 
					szBuff[i] = ch;
			}
			if (i >= MAX_LANGLEN)
				__leave;

			szBuff[i] = L'\0';

			if (!(g_cLp % LP_ALLOCUNIT)) {
				g_pLp = (PLANGPAIR)HeapReAlloc(hheap, 0, 
					g_pLp, (g_cLp + LP_ALLOCUNIT) * sizeof(LANGPAIR));
				if (!g_pLp) __leave;
			}

			g_pLp[g_cLp].uID = uID;
            g_pLp[g_cLp].pszText = (LPWSTR)HeapAlloc(hheap, 0, 
				(lstrlen(szBuff) + 1) * sizeof(WCHAR));
			if (!g_pLp[g_cLp].pszText) __leave;

			lstrcpy(g_pLp[g_cLp].pszText, szBuff);
            g_cLp++;            
		}
		fSuccess = TRUE;
	}
	__finally {
	}
	if (!fSuccess || !g_pLp || g_cLp <= 0) {
		g_cLp = 0;
		if (g_pLp) {
			HeapFree(hheap, 0, g_pLp);
			g_pLp = NULL;
		}
	} else if (g_cLp % LP_ALLOCUNIT) {
		g_pLp = (PLANGPAIR)HeapReAlloc(GetProcessHeap(), 
			0, g_pLp, g_cLp * sizeof(LANGPAIR));
	}
	return(fSuccess);
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

int CompareLangID(const PVOID pel1, const PVOID pel2) {
	return((int)(((LANGPAIR*)pel1)->uID - ((LANGPAIR*)pel2)->uID));
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
	}

	if (!fSuccess) {
		g_cLp = 0;
		if (g_pLp) {
			HeapFree(GetProcessHeap(), 0, g_pLp);
			g_pLp = NULL;
		}
	} else if (g_pLp && g_cLp > 0)
		MyInsertionSort(g_pLp, g_cLp, sizeof(LANGPAIR), CompareLangID);

	return(fSuccess);
}

//-----------------------------------------------------------------

BOOL LoadLangFile(LPCWSTR pszLangFile) {

	EnterCriticalSection(&g_csLp);

	g_cLp = 0;
	if (g_pLp) {
		HeapFree(GetProcessHeap(), 0, g_pLp);
		g_pLp = NULL;
	}
	if (_pszLangFile) {
		HeapFree(GetProcessHeap(), 0, _pszLangFile);
		_pszLangFile = NULL;
	}

	BOOL fSuccess = TRUE;
	if (pszLangFile && pszLangFile[0] != L'\0') {

		WCHAR szBuff[MAX_PATH * 2 + 16] = L"";
		int n = GetModuleFileName(g_hinstExe, szBuff, MAX_PATH);
		if (n > 0 && n < MAX_PATH) {
			while (n >= 0 && szBuff[n] != L'\\') n--;
			lstrcpyn(szBuff + n + 1, L"lang\\", SIZEOF_ARRAY(szBuff) - n);
			n += lstrlen(L"lang\\");
			lstrcpyn(szBuff + n + 1, pszLangFile, SIZEOF_ARRAY(szBuff) - n);
			//if (lstrcmpi(szBuff + lstrlen(szBuff) - 4, L".lng"))
			n += lstrlen(pszLangFile);
			lstrcpyn(szBuff + n + 1, L".lng", SIZEOF_ARRAY(szBuff) - n);
		}

		fSuccess = _LoadLangFile(szBuff);
		if (fSuccess) {
			_pszLangFile = (LPWSTR)HeapAlloc(GetProcessHeap(), 
				0, ((lstrlen(pszLangFile) + 1) * sizeof(WCHAR)));
			if (_pszLangFile)
				lstrcpy(_pszLangFile, pszLangFile);
		}
	}
	LeaveCriticalSection(&g_csLp);
	return(fSuccess);
}

//-----------------------------------------------------------------

BOOL UpdateLangFile(LPCWSTR pszLangFile) {

	BOOL fChanged = FALSE;
	if (pszLangFile) {
		if (lstrcmpi(_pszLangFile, pszLangFile)) {
			if (!LoadLangFile(pszLangFile))
				ReportError(IDS_ERR_LANGFILE);
			fChanged = TRUE;
		}
	} else {
		if (!IsLangDefault()) {
			LoadLangFile(NULL);
			fChanged = TRUE;
		}
	}
	return(fChanged);
}

//-----------------------------------------------------------------

BOOL IsLangDefault() {

	BOOL fIsDefault;

	EnterCriticalSection(&g_csLp);
	fIsDefault = (BOOL)(!_pszLangFile && g_pLp == NULL && g_cLp == 0);
	LeaveCriticalSection(&g_csLp);

	return(fIsDefault);
}

//-----------------------------------------------------------------

BOOL InitThreadLang2() {
	return(InitializeCriticalSectionAndSpinCount(&g_csLp, 0));
}

//-----------------------------------------------------------------

void DestroyThreadLang2() {
	DeleteCriticalSection(&g_csLp);
}
