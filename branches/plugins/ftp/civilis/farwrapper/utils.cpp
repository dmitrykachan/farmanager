#include "stdafx.h"
#include "utils.h"

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

}