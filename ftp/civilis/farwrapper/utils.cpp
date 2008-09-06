#include "stdafx.h"
#include "utils.h"
#include "../utils/strutils.h"

namespace FARWrappers
{
bool patternCmp(const std::wstring &str, const std::wstring &pattern, wchar_t* delimiters, bool skipPath)
{
	if(pattern.empty())
		return false;

	if(delimiters == NULL)
	{
		return getInfo().CmpName(pattern.c_str(), str.c_str(), skipPath) != 0;
	}

	size_t off = 0;
	std::wstring s;
	do
	{
		size_t pos = str.find_first_of(delimiters, off);
		if(pos == std::wstring::npos)
			s.assign(str, off, str.size()-off);
		else
			s.assign(str, off, pos-off);
		if(s.empty())
			return false;

		if(getInfo().CmpName(pattern.c_str(), s.c_str(), skipPath))
			return true;
		++off;
	}while(true);

	return false;
}

void convertFindData(WIN32_FIND_DATAW& w, const FAR_FIND_DATA &f)
{
	w.dwFileAttributes	= f.dwFileAttributes;
	w.ftCreationTime	= f.ftCreationTime;
	w.ftLastAccessTime	= f.ftLastAccessTime;
	w.ftLastWriteTime	= f.ftLastWriteTime;
	Utils::safe_wcscpy(w.cAlternateFileName, f.lpwszAlternateFileName);
	Utils::safe_wcscpy(w.cFileName, f.lpwszFileName);
	w.nFileSizeLow		= f.nFileSize & 0xFFFFFFFF;
	w.nFileSizeHigh		= f.nFileSize >> 32;
	w.dwReserved0		= 0;
	w.dwReserved1		= 0;
}

}