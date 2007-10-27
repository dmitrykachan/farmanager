#include "stdafx.h"
#include "unicode.h"
namespace Unicode
{
namespace utf16
{
	enum
	{
		SurrogateOffset = ((0xd800<<10UL)+0xdc00-0x10000)
	};

	bool isSingle(char32 wc)
	{
		return (wc & 0xfffff800) == 0xd800;
	}

	void pushChar(std::wstring &wstr, wchar_t wc)
	{
		if(wc < 0xFFFF)
		{
			wstr.push_back(wc);
		} 
		else
		{
			wstr.push_back((wc >> 10) + 0xd7c0);
			wstr.push_back((wc & 0x3ff) | 0xdc00);
		}
	}

	static bool isLead(char32 wc)
	{
		return (wc & 0xfffffc00) == 0xd800;
	}

	static bool isTrail(char32 wc)
	{
		return (wc & 0xfffffc00) == 0xdc00;
	}

	char32 nextChar(std::wstring::const_iterator& itr, const std::wstring::const_iterator itr_end)
	{
		if(itr == itr_end)
			return  '\0';
		char32 wc = *itr++;
		if(isLead(wc)) 
		{
			char32 c2;
			if(itr != itr_end && isTrail(c2 = *itr))
			{
				++itr;
				wc = (wc << 10UL) + c2 - SurrogateOffset;
			}
		}
		return wc;
	}

	char32 nextChar(const wchar_t* (&str))
	{
		char32 wc = *str++;
		if(isLead(wc)) 
		{
			char32 c2;
			if(isTrail(c2 = *str))
			{
				++str;
				wc = (wc << 10UL) + c2 - SurrogateOffset;
			}
		}
		return wc;
	}

}

}