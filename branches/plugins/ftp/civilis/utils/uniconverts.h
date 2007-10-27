#ifndef UNICODE_CONVERT_HEADER
#define UNICODE_CONVERT_HEADER

#include "unicode.h"
#include "winwrapper.h"

namespace Unicode
{
	std::string		utf16ToUtf8(const std::wstring &str);
	std::wstring	utf8ToUtf16(const std::string &str);
	std::string		utf16ToAscii(const std::wstring &str, char replace = '_');
	std::wstring	AsciiToUtf16(const std::string &str, wchar_t replace = L'_');

	inline std::string		toOem(const std::wstring& str)
	{
		return WinAPI::toOEM(str);
	}

	inline std::wstring	fromOem(const std::string& str)
	{
		return WinAPI::fromOEM(str);
	}

	template <typename T>
	std::vector<T> strToVec(const std::basic_string<T> &str)
	{
		return std::vector<T>(str.begin(), str.end());
	}
}

#endif