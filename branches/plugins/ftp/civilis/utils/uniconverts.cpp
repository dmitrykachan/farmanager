#include "stdafx.h"

#include "uniconverts.h"
#include "utf8.h"
#include "utf16.h"
#include "vector"

namespace Unicode
{
	std::string utf16ToUtf8(const wchar_t* str)
	{
		char32 wc;
		std::string outstr;
		while((wc = utf16::nextChar(str)) != 0)
		{
			utf8::pushChar(outstr, wc);
		}
		return outstr;
	}

	std::string utf16ToUtf8(const std::wstring &str)
	{
		return utf16ToUtf8(str.c_str());
	}

	std::wstring utf8ToUtf16(const std::string &str)
	{
		std::wstring res;
		res.reserve(str.size());
		std::string::const_iterator itr = str.begin();
		while(itr != str.end())
		{
			char32 wc = utf8::nextChar(itr, str.end());
			utf16::pushChar(res, wc);
		}
		return res;
	}

	std::string utf16ToAscii(const std::wstring &str, char replace)
	{
		std::string res;
		res.reserve(str.size());
		std::wstring::const_iterator itr = str.begin();
		while(itr != str.end())
		{
			char32 wc = utf16::nextChar(itr, str.end());
			wc = (wc > 127)? replace : wc;
			res.push_back(static_cast<char>(wc));
		}
		return res;
	}

	std::wstring AsciiToUtf16(const std::string &str, wchar_t replace)
	{
		std::wstring res;
		res.reserve(str.size());
		std::string::const_iterator itr = str.begin();
		while(itr != str.end())
		{
			wchar_t c = *itr;
			c = (c > 127)? replace : c;
			res.push_back(c);
			++itr;
		}
		return res;
	}

}
