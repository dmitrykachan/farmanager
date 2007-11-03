#ifndef FILETIME_HEADER
#define FILETIME_HEADER

#include "unicode.h"

namespace WinAPI
{

class FileTime: public FILETIME
{
private:
	inline void check(bool value, const char* const what)
	{
		if(!value)
		{
			throw std::exception(what);
		}
	}

public:
	FileTime()
	{
		dwLowDateTime = 0;
		dwHighDateTime = 0;
	}

	FileTime(const FILETIME &ft)
	{
		dwLowDateTime = ft.dwLowDateTime;
		dwHighDateTime= ft.dwHighDateTime;
	}

	FileTime(int year, int month, int day, int hours, int minutes, int seconds, int milliseconds = 0)
	{
		setDate(year, month, day);
		setTime(hours, minutes, seconds, milliseconds);
	}


	bool empty() const
	{
		return dwHighDateTime == 0 && dwLowDateTime == 0;
	}

	SYSTEMTIME getSystemTime() const
	{
		SYSTEMTIME st;
		if(empty())
		{
			st = getCurrentSystemTime();
		} else
		{
			FileTimeToSystemTime(this, &st);
		}
		return st;
	}

	static SYSTEMTIME getCurrentSystemTime()
	{
		SYSTEMTIME st;
		GetSystemTime(&st);
		return st;
	}

	bool setTime(int hours, int minutes, int seconds, int milliseconds = 0)
	{
		SYSTEMTIME st = getSystemTime();
		st.wHour = hours;
		st.wMinute = minutes;
		st.wSecond = seconds;
		st.wMilliseconds = milliseconds;
		return SystemTimeToFileTime(&st, this) != 0;
	}

	void setDate(int year, int month, int day)
	{
		SYSTEMTIME st = getSystemTime();
		st.wDay = day;
		st.wMonth = month;
		st.wYear = year;
		check(SystemTimeToFileTime(&st, this) != false, "date is incorrect");
	}

	ULONGLONG get() const
	{
		return((ULONGLONG( dwHighDateTime )<<32)|dwLowDateTime);
	}

	void set(ULONGLONG time)
	{
		dwLowDateTime  = DWORD(time);
		dwHighDateTime = DWORD(time>>32);
	}

	void getTime(int &hour, int &minute, int &second, int &millisecond)
	{
		SYSTEMTIME st = getSystemTime();
		hour			= st.wHour;
		minute			= st.wMinute;
		second			= st.wSecond;
		millisecond		= st.wMilliseconds;
	}

	void getDate(int &year, int &month, int &day)
	{
		SYSTEMTIME st = getSystemTime();
		year	= st.wYear;
		month	= st.wMonth;
		day		= st.wDay;
	}

	void getDate(std::wstring &ws)
	{
		SYSTEMTIME st = getSystemTime();
		std::wstringstream os;
		os  << std::setw(2) << std::setfill(L'0') << st.wDay << L'.' 
			<< std::setw(2) << std::setfill(L'0') << st.wMonth << L'.' 
			<< std::setw(2) << std::setfill(L'0') << st.wYear;
		ws = os.str();
	}
};

inline std::wostream& operator<<(std::wostream& os, FileTime& ft)
{
	SYSTEMTIME st;
	st = ft.getSystemTime();
	os  << std::setw(4) << std::setfill(L'0') << st.wYear << L'-'
		<< std::setw(2) << std::setfill(L'0') << st.wMonth << L'-' 
		<< std::setw(2) << std::setfill(L'0') << st.wDay << L' ' 
		<< std::setw(2) << std::setfill(L'0') << st.wHour << L':' 
		<< std::setw(2) << std::setfill(L'0') << st.wMinute << L':' 
		<< std::setw(2) << std::setfill(L'0') << st.wSecond; 
	return os;
}

inline bool operator == (const FILETIME &l, const FILETIME &r)
{
	return l.dwHighDateTime == r.dwHighDateTime &&
			l.dwLowDateTime == r.dwLowDateTime;
}

inline bool operator != (const FILETIME &l, const FILETIME &r)
{
	return !(l == r);
}

} // namespace winapi

#ifdef CONFIG_TEST

namespace
{
BOOST_AUTO_TEST_CASE(test_filetime)
{
	WinAPI::FileTime ft;
	int hours, minutes, seconds, ms, day, year, month;
	BOOST_CHECK(ft.setTime(1,2,3));
	ft.getTime(hours, minutes, seconds, ms);
	BOOST_CHECK_EQUAL(hours, 1);
	BOOST_CHECK_EQUAL(minutes, 2);
	BOOST_CHECK_EQUAL(seconds, 3);
	BOOST_CHECK_EQUAL(ms, 0);

	BOOST_CHECK(ft.setTime(24,2,33) == false);
	BOOST_CHECK(ft.setTime(4,60,33) == false);
	BOOST_CHECK(ft.setTime(4,2,330) == false);
	BOOST_CHECK(ft.setTime(4,2,33,-5) == false);

	BOOST_CHECK_NO_THROW(ft.setDate(1982, 9, 21));
	ft.getDate(year, month, day);
	BOOST_CHECK_EQUAL(year, 1982);
	BOOST_CHECK_EQUAL(month, 9);
	BOOST_CHECK_EQUAL(day, 21);
}
}

#endif

#endif