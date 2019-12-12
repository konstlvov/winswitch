// cfgfile.cpp

#include "stdafx.h"


//-----------------------------------------------------------------

int StringToCfgString(PCWSTR szText, PWSTR szBuff, int nBuffMax) {

	int n = 0;
	for (PCWSTR p = szText; *p != L'\0' && n < nBuffMax; p++) {
		switch (*p) {
			case L'\"':
				szBuff[n++] = L'\\';
				if (n < nBuffMax)
					szBuff[n] = L'\"';
				break;
			case L'\\':
				szBuff[n++] = L'\\';
				if (n < nBuffMax)
					szBuff[n] = L'\\';
				break;
			case L'\r':
				szBuff[n++] = L'\\';
				if (n < nBuffMax)
					szBuff[n] = L'r';
				break;
			case L'\n':
				szBuff[n++] = L'\\';
				if (n < nBuffMax)
					szBuff[n] = L'n';
				break;
			case L'\t':
				szBuff[n++] = L'\\';
				if (n < nBuffMax)
					szBuff[n] = L't';
				break;
			default:
				szBuff[n] = *p;
				break;
		}
		n++;
	}
	if (n >= nBuffMax)
		n = 0;
	szBuff[n] = L'\0';

	return(n);
}

//-----------------------------------------------------------------

BOOL _IsDelimitWCHAR(WCHAR ch) {
	return (ch == L' ' || ch == L'\n' || 
		ch == L'\r' || ch == L'\0' || ch == L'\t');
}

//-----------------------------------------------------------------

UINT _GetID(LPCWSTR pch, UINT nLength, UINT &pos) {

	UINT uID = 0, n = pos;
	for (; pos < nLength; pos++) {
		WCHAR ch = pch[pos];
		if (ch == L'=' || _IsDelimitWCHAR(ch))
			return(uID);
        if (!iswdigit(ch) || pos - n > 10)
			break;
		uID = uID * 10 + (UINT)(ch - L'0');
	}
	return((UINT)-1);
}


BOOL _GetDWORD(LPCWSTR pch, UINT nLength, UINT &pos, DWORD* pdwID) {

	UINT uID = 0, n = pos;
	for (; pos < nLength; pos++) {
		WCHAR ch = pch[pos];
		if (_IsDelimitWCHAR(ch)) {
			if (pdwID) *pdwID = uID;
			return(TRUE);
		}
        if (pos - n > 8)
			break;
		UINT d;
		if (ch >= L'0' && ch <= L'9')
			d = (UINT)(ch - L'0');
		else if (ch >= L'A' && ch <= L'F')
			d = (UINT)(ch - L'A' + 10);
		else if (ch >= L'a' && ch <= L'f')
			d = (UINT)(ch - L'a' + 10);
		else break;

		uID = uID * 16 + d;
	}
	return(FALSE);
}

//-----------------------------------------------------------------

BOOL _WaitNextLine(LPCWSTR pch, UINT nLength, UINT &pos) {
	for (; pos < nLength; pos++)
		if (pch[pos] == L'\n')
			return(TRUE);
	return(FALSE);
}


BOOL _WaitNextSection(LPCWSTR pch, UINT nLength, UINT &pos) {
	while (_WaitNextLine(pch, nLength, pos)) {
		pos++;
		if (pch[pos] == L'[')
			return(TRUE);
	}
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

BOOL _GetString(LPCWSTR pch, UINT nLength, UINT &pos, PWSTR szBuff, UINT nBuffMax) {

	if (pch[pos] != L'\"' || pos >= nLength)
		return(FALSE);
	pos++;

	UINT i = 0;
	for (; i < nBuffMax && pos < nLength; i++) {

		WCHAR ch = pch[pos++];
		if (ch == L'\"')
			break;

		if (ch == L'\\') {
			if (pos >= nLength)
				break;
			ch = pch[pos++];
			switch(ch) {
				case L'n': szBuff[i] = L'\n'; break;
				case L't': szBuff[i] = L'\t'; break;
				case L'\\': szBuff[i] = L'\\'; break;
				case L'\"':	szBuff[i] = L'\"'; break;
				default:
					szBuff[i++] = L'\\';
					if (i < nBuffMax)
						szBuff[i] = ch;
					break;
			}
		} else 
			szBuff[i] = ch;
	}
	BOOL fRet = TRUE;
	if (pch[pos - 1] != L'\"') {
		fRet = FALSE;
		i = 0;
	}
	szBuff[i] = L'\0';
	return(fRet);
}

//-----------------------------------------------------------------
