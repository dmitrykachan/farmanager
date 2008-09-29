#ifndef PARSER_PRIMITIVE_HEADER
#define PARSER_PRIMITIVE_HEADER

namespace Parser
{
	class exception: public std::exception
	{
	public:
		exception(const char * const& str)
			: std::exception(str)
		{};
	};

	inline void check(bool value, const char* const what)
	{
		if(!value)
			throw exception(what);
	}

	template <typename T1, typename T2>
	static bool skipSpaces(T1& itr, const T2& itr_end)
	{
		while(itr != itr_end && std::isspace(*itr, defaultLocale_))
			++itr;
		return true;
	}

	template <typename T1, typename T2>
	static bool skipNSpaces(T1& itr, const T2& itr_end)
	{
		while(itr != itr_end && !std::isspace(*itr, defaultLocale_))
			++itr;
		return true;
	}

	template<typename T1, typename T2>
	static void skipNumber(T1& itr, const T2 &itr_end, bool skipspaces = true)
	{
		check(itr != itr_end && std::isdigit(*itr, defaultLocale_), "the number is expected");
		while(itr != itr_end && std::isdigit(*itr, defaultLocale_))
			++itr;
		if(skipspaces)
			skipSpaces(itr, itr_end);
	}

	template<typename T1, typename T2, typename T3>
	static void skipString(T1& itr, const T2 &itr_end, const T3* str, bool skipspaces = true)
	{
		T1 i = itr;
		while(i != itr_end && *i == *str)
		{
			++i; ++str;
		}
		check(*str == 0, "there are not the string");
		itr = i;
		if(skipspaces)
			skipSpaces(itr, itr_end);
	}

	template<typename Itr1, typename Itr2, typename C>
	static bool checkChar(Itr1& itr, const Itr2 &itr_end, C ch)
	{
		return itr != itr_end && *itr == ch;
	}

	template<typename Itr1, typename Itr2, typename C>
	static bool checkString(Itr1& itr, const Itr2 &itr_end, const C* str, bool matchCase = true)
	{
		Itr1 i = itr;
		if(matchCase)
		{
			while(i != itr_end && *i == *str)
			{
				++i; ++str;
			}
		} else
		{
			while(i != itr_end && std::toupper(*i, defaultLocale_) == std::toupper(*str, defaultLocale_))
			{
				++i; ++str;
			}
		}

		if(*str != 0)
			return false;
		else
		{
			itr = i;
			return true;
		}
	}

	template<typename I1, typename I2, typename T>
	static T parseUnsignedNumber(I1 &itr, const I2& itr_end, T def, bool skipspaces = true)
	{
		bool isdigit = (itr != itr_end && std::isdigit(*itr, defaultLocale_));
		if(!isdigit)
			return def;

		T result = 0;
		while(isdigit)
		{
			int dig = *itr - '0';
			if(result > (std::numeric_limits<T>::max() - dig)/10)
				return def;
			result = result*10 + dig;
			++itr;
			isdigit = (itr != itr_end && std::isdigit(*itr, defaultLocale_));
		}
		if(skipspaces)
			skipSpaces(itr, itr_end);
		return result;
	}

	template<typename T, typename I1, typename I2>
	static T parseUnsignedNumber(I1 &itr, const I2& itr_end, bool skipspaces = true)
	{
		bool isdigit = (itr != itr_end && std::isdigit(*itr, defaultLocale_));
		check(isdigit, "a number is not valid");

		T result = 0;
		while(isdigit)
		{
			int dig = *itr - '0';
			check(result <= (std::numeric_limits<T>::max() - dig)/10, "a number is not valid");
			result = result*10 + dig;
			++itr;
			isdigit = (itr != itr_end && std::isdigit(*itr, defaultLocale_));
		}
		if(skipspaces)
			skipSpaces(itr, itr_end);
		return result;
	}

	template<typename I1, typename I2>
	static bool skipWord(I1 &itr, const I2 &itr_end, bool skipspaces = true)
	{
		while(itr != itr_end && !isspace((unsigned char)*itr))
			++itr;
		if(skipspaces)
			skipSpaces(itr, itr_end);
		return true;
	}

	enum Month
	{
		January = 1, February, March, April, May, June, July, August, September,
		October, November, December
	};

	template<typename I1, typename I2>
	static Month parseMonth(I1 &itr, const I2 &itr_end, bool skipspaces = true)
	{
		static const wchar_t months[12][4] = 
		{ 
			L"jan", L"feb", L"mar", L"apr", L"may", L"jun", 
			L"jul", L"aug", L"sep", L"oct", L"nov", L"dec"
		};
		wchar_t m[32];
		wchar_t* p = m;
		while(itr != itr_end && std::isalpha(*itr, defaultLocale_) && p < m + sizeof(m)-1)
		{
			*p++ = std::tolower(*itr++, defaultLocale_);
		}
		*p = '\0';
		for(int i = 0; i < sizeof(months)/sizeof(*months); i++)
		{
			if(wcscmp(months[i], m) == 0)
			{
				if(skipspaces)
					skipSpaces(itr, itr_end);
				return static_cast<Month>(i+1);
			}
		}
		throw exception("unknown month");
	}

	template<typename I1, typename I2, typename T>
	static T parseUnsignedNumberRange(I1 &itr, const I2 &itr_end, T r1, T r2, bool skipspaces = true)
	{
		T result = parseUnsignedNumber(itr, itr_end, r2+1, skipspaces);
		check(result >= r1 && result <= r2, "number is out of range");
		return result;
	}

	template<typename I1, typename I2, typename T>
	static bool checkUnsignedNumber(I1 &itr, const I2 &itr_end, T& result, bool skipspaces = true)
	{
		result = parseUnsignedNumber(itr, itr_end, -1, skipspaces);
		return result != -1;
	}

	template<typename I1, typename I2, typename T>
	static bool checkUnsignedNumberRange(I1 &itr, const I2 &itr_end, T& result, T r1, T r2, bool skipspaces = true)
	{
		result = parseUnsignedNumber(itr, itr_end, r2+1, skipspaces);
		return (result >= r1 && result <= r2);
	}

	template<typename I1, typename I2>
	static void parseTime(I1 &itr, const I2 &itr_end, WinAPI::FileTime &time, bool skipspaces = true)
	{
		int hours, minutes, seconds = 0;
		hours = parseUnsignedNumberRange(itr, itr_end, 0, 23, false);
		skipString(itr, itr_end, L":");
		minutes = parseUnsignedNumberRange(itr, itr_end, 0, 59);
		if(checkChar(itr, itr_end, L':'))
		{
			++itr;
			seconds = parseUnsignedNumberRange(itr, itr_end, 0, 59);
		}
		if(checkString(itr, itr_end, L"AM"))
		{
			check(hours > 0 && hours <= 12, "hours have incorrect range");
		} else
			if(checkString(itr, itr_end, L"PM"))
			{
				check(hours > 0 && hours <= 12, "hours have incorrect range");
				hours = (hours + 12) % 24; 
			}

		if(skipspaces)
			skipSpaces(itr, itr_end);
		time.setTime(hours, minutes, seconds);
	}

	/*
	2005-06-20
	*/
	static void parseFullDate(std::wstring::const_iterator &itr, 
		const std::wstring::const_iterator &itr_end, WinAPI::FileTime &ft)
	{
		using namespace Parser;
		int year, day, month;
		year = parseUnsignedNumberRange(itr, itr_end, 1600, 2100);
		skipString(itr, itr_end, L"-");
		month = parseUnsignedNumberRange(itr, itr_end, 1, 12);
		skipString(itr, itr_end, L"-");
		day = parseUnsignedNumberRange(itr, itr_end, 1, 31);
		ft.setDate(year, month, day);
	}

	static void parseUnixDateTime(std::wstring::const_iterator &itr, 
		const std::wstring::const_iterator &itr_end, WinAPI::FileTime &ft)
	{
		using namespace Parser;
		bool notParseTime = false;
		if(std::isdigit(*itr, defaultLocale_))
		{
			parseFullDate(itr, itr_end, ft);
		} else
		{
			Month mon = parseMonth(itr, itr_end);

			int day = parseUnsignedNumberRange(itr, itr_end, 1, 31);
			std::wstring::const_iterator itr_save = itr;
			int year;
			if(!checkUnsignedNumberRange(itr, itr_end, year, 1600, 2100))
			{
				year = WinAPI::FileTime::getCurrentSystemTime().wYear;
				itr = itr_save;
			}
			else
				notParseTime = true;
			ft.setDate(year, mon, day);
		}

		if(notParseTime)
			ft.setTime(0, 0, 0);
		else
			parseTime(itr, itr_end, ft);
	}

	// 04-26-01  05:28PM
	static void parseWindowsDate(std::wstring::const_iterator &itr, 
		const std::wstring::const_iterator &itr_end, WinAPI::FileTime &ft, bool skipspaces = true)
	{
		using namespace Parser;
		
		int month = parseUnsignedNumberRange(itr, itr_end, 1, 12);
		skipString(itr, itr_end, L"-");
		int day = parseUnsignedNumberRange(itr, itr_end, 1, 31);
		skipString(itr, itr_end, L"-");
		int year = parseUnsignedNumberRange(itr, itr_end, 0, 199) + 1900;
		if(year <= 1950)
			year += 100;
		ft.setDate(year, month, day);
		if(skipspaces)
			skipSpaces(itr, itr_end);
	}

	static void parseOs2Attribute(std::wstring::const_iterator &itr, 
		const std::wstring::const_iterator &itr_end, int& attribute, bool skipspaces = true)
	{
		attribute = 0;
		bool finish = false;
		while(itr != itr_end && !finish)
		{
			switch(std::tolower(*itr, defaultLocale_))
			{
			case L'a': attribute |= FILE_ATTRIBUTE_ARCHIVE; break;
			case L'h': attribute |= FILE_ATTRIBUTE_HIDDEN; break;
			case L'r': attribute |= FILE_ATTRIBUTE_READONLY; break;
			case L's': attribute |= FILE_ATTRIBUTE_SYSTEM; break;
			default:
				finish = true;
				continue;
			}
			++itr;
		};
		if(skipspaces)
			skipSpaces(itr, itr_end);
	}

	template<typename T>
	class compareIgnoreCase: std::binary_function<T, T, bool>
	{
	public:
		compareIgnoreCase() {};
		bool operator()(T ch1, T ch2)
		{
			return	std::tolower(ch1, defaultLocale_) == 
				std::tolower(ch2, defaultLocale_);
		}
	};

	template<typename InItr1, typename InItr2>
	bool equalIgnoreCase(InItr1 first, InItr1 last, InItr2 first2)
	{
		return std::equal(first, last, first2, compareIgnoreCase<std::iterator_traits<InItr1>::value_type>());
	}

} // namaspace Parser


#endif