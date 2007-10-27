#ifndef LISTING_HEADER
#define LISTING_HEADER

#include "fileinfo.h"


namespace Listing
{
	template <typename T1, typename T2>
	static bool skipSpaces(T1& itr, T2& itr_end)
	{
		while(itr != itr_end && std::isspace(*itr, defaultLocale_))
		{
			++itr;
		}
		return true;
	}
/*

	template<typename T1, typename T2>
	static void skipNumber(T1& itr, const T2 &itr_end, bool skipspaces = true)
	{
		T1 itr_save = itr;
		while(itr != itr_end && std::isdigit(*itr, defaultLocale_))
		{
			++itr;
		}
		bool res = itr_save != itr;
		if(skipspaces)
			skipSpaces(itr, itr_end);
		return res;
	}

*/
	template<typename T1, typename T2, typename T3>
	static bool skipString(T1& itr, const T2 &itr_end, const T3* str, bool skipspaces = true)
	{
		T1 i = itr;
		while(i != itr_end && *i == *str)
		{
			++i; ++str;
		}
		bool res = *str == 0;
		if(res)
		{
			itr = i;
			if(skipspaces)
				skipSpaces(itr, itr_end);
		}
		return res;
	}

	template<typename Itr1, typename Itr2, typename C>
	static bool checkChar(Itr1& itr, const Itr2 &itr_end, C ch)
	{
		return itr != itr_end && *itr == ch;
	}

	template<typename Itr1, typename Itr2, typename C>
	static bool checkString(Itr1& itr, const Itr2 &itr_end, const C* str)
	{
		Itr1 i = itr;
		while(i != itr_end && *i == *str)
		{
			++i; ++str;
		}
		if(*str != 0)
			return false;
		else
		{
			itr = i;
			return true;
		}
	}

	template<typename I1, typename I2, typename O>
	static bool copyWord(I1 &itr, const I2 &itr_end, O out)
	{
		while(itr != itr_end && std::isalpha(*itr, defaultLocale_))
		{
			*out++ = *itr++;
		}
		return true;
	}

	template<typename I1, typename I2, typename T>
	static T parseUnsignedNumber(I1 &itr, const I2& itr_end, T def, bool skipspaces = true)
	{
		if(itr == itr_end || !isdigit((unsigned char)*itr))
			return def;

		T result = 0;
		while(itr != itr_end && isdigit((unsigned char)*itr))
		{
			result = result*10 + *itr - '0';
			++itr;
		}
		if(skipspaces)
			skipSpaces(itr, itr_end);
		return result;
	}

	template<typename I1, typename I2, typename T>
	static T parseNumber(I1 &itr, const I2& itr_end, T def, bool skipspaces = true)
	{
		if(itr == itr_end)
			return def;

		T sign = 1;

		if(*itr == '-') 
		{
			sign = -1;
			++itr;
		} else
			if(*itr == '+')
				++itr;

		T result = parseUnsignedNumber(itr, itr_end, def, skipspaces);

		return (sign == -1)? -result : result;
	}

	template<typename I1, typename I2>
	static bool skipWord(I1 &itr, const I2 &itr_end, bool skipspaces = true)
	{
		while(itr != itr_end && !isspace((unsigned char)*itr))
		{
			++itr;
		}
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
	static bool parseMonth(I1 &itr, const I2 &itr_end, Month& month, bool skipspaces = true)
	{
		static const char months[12][4] = 
		{ 
			"jan", "feb", "mar", "apr", "may", "jun", 
			"jul", "aug", "sep", "oct", "nov", "dec"
		};
		char m[32];
		char* p = m;
		while(itr != itr_end && std::isalpha(*itr, defaultLocale_) && p < m + sizeof(m)-1)
		{
			*p++ = tolower(*itr++);
		}
		*p = '\0';
		for(int i = 0; i < sizeof(months)/sizeof(*months); i++)
		{
			if(strcmp(months[i], m) == 0)
			{
				month = static_cast<Month>(i+1);
				if(skipspaces)
					skipSpaces(itr, itr_end);
				return true;
			}
		}
		return false;
	}

	template<typename I1, typename I2, typename T>
	static bool parseNumberRange(I1 &itr, const I2 &itr_end, T& result, T r1, T r2, bool skipspaces = true)
	{
		result = parseNumber(itr, itr_end, r2+1);
		if(skipspaces)
			skipSpaces(itr, itr_end);
		if(result >= r1 && result <= r2)
			return true;
		else
			return false;
	}

	template<typename I1, typename I2, typename T>
	static bool parseUnsignedNumberRange(I1 &itr, const I2 &itr_end, T& result, T r1, T r2, bool skipspaces = true)
	{
		result = parseUnsignedNumber(itr, itr_end, r2+1, skipspaces);
		if(result >= r1 && result <= r2)
			return true;
		else
			return false;
	}


	template<typename I1, typename I2>
	static bool parseTime(I1 &itr, const I2 &itr_end, WinAPI::FileTime &time, bool skipspaces = true)
	{
		int hours, minutes, seconds = 0;
		const int invalid = -1;
		if(!parseUnsignedNumberRange(itr, itr_end, hours, 0, 23, false))
			return false;
		if(!checkChar(itr, itr_end, L':'))
			return false;
		++itr;
		if(!parseUnsignedNumberRange(itr, itr_end, minutes, 0, 59))
			return false;
		if(checkChar(itr, itr_end, L':'))
		{
			++itr;
			if(!parseUnsignedNumberRange(itr, itr_end, seconds, 0, 59))
				return false;
		}
		if(checkString(itr, itr_end, L"AM"))
		{
			if(hours == 0 || hours > 12)
				return false;
		} else
			if(checkString(itr, itr_end, L"PM"))
			{
				if(hours == 0 || hours > 12)
					return false;
				hours = (hours + 12) % 24; 
			}

		if(skipspaces)
			skipSpaces(itr, itr_end);
		time.setTime(hours, minutes, seconds);
		return true;
	}

	bool parseVxWorksListing(FTPFileInfo &fileinfo, 
		std::wstring::const_iterator itr, 
		const std::wstring::const_iterator &itr_end);
	std::wstring parseListing(const std::wstring &listing);
}


#endif