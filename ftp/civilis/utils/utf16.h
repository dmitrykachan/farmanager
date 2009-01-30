#ifndef UTF16_HEADER__
#define UTF16_HEADER__

#include "unicode.h"

namespace Unicode
{
	namespace utf16
	{
		char32 nextChar(std::wstring::const_iterator& itr, const std::wstring::const_iterator itr_end);
		char32 nextChar(const wchar_t* (&str));
		void pushChar(std::wstring &wstr, char32 wc);
	}
}

#endif