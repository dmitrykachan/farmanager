#ifndef STRING_UTILS_HEADER
#define STRING_UTILS_HEADER

namespace Utils
{

template<size_t size>
inline void safe_strcpy(char (&dest)[size], const char* src)
{
	strncpy(dest, src, size-2);
	dest[size-1] = 0;
}

template<size_t size>
inline void safe_wcscpy(wchar_t (&dest)[size], const wchar_t* src)
{
	wcsncpy(dest, src, size-2);
	dest[size-1] = 0;
}

template<size_t size>
inline void safe_strcpy(char (&dest)[size], const std::string &src)
{
	safe_strcpy(dest, src.c_str());
}

template<size_t size>
inline void safe_wcscpy(wchar_t (&dest)[size], const std::wstring& src)
{
	safe_wcscpy(dest, src.c_str());
}

template<typename T>
T last(std::basic_string<T> con)
{
	return *(con.end()-1);
}

template<typename T>
T first(std::basic_string<T> con)
{
	return *con.begin();
}

inline void removeDuplicatedCharacter(std::wstring& s, wchar_t wc)
{
	std::wstring::iterator it = find(s.begin(), s.end(), wc);
	while(it != s.end())
	{
		++it;
		if(it != s.end() && *it == wc)
			it = s.erase(it);
		it = find(it, s.end(), wc);
	}
}

static size_t DigitCount(__int64 val)
{
	if(val == 0)
		return 1;
	else
	{
		size_t  n = 0;
		while(val)
		{
			val /= 10;
			n++;
		}
		return n;
	}
}

static std::wstring FDigit(__int64 val, wchar_t delimiter = 0)
{
	BOOST_ASSERT(val >= 0);

	if(val == 0)
		return L"0";

	size_t n = DigitCount(val);

	if(delimiter)
		n += (n-1)/3;
	std::wstring str(n, delimiter);
	str.resize(n, delimiter);
	size_t i = 0;
	while(val)
	{
		--n;
		if(delimiter && i != 0 && i % 3 == 0)
			--n;
		str[n] = static_cast<wchar_t>(val % 10) + L'0';
		val /= 10;

		++i;
	}
	return str;
}

}

using Utils::last;
using Utils::first;

#endif