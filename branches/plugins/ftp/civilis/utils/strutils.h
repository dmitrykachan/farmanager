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

}

using Utils::last;
using Utils::first;

#endif