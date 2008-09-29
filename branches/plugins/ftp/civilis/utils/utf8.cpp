#include "stdafx.h"
#include "unicode.h"

#include "utf8.h"
#include "utf16.h"

namespace Unicode
{

namespace utf8
{
	size_t charLength(char c)
	{
		static const unsigned char charSizeTable[256] = 
			{
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			1,  1,  1,  1,  1,  1,  1,  1, 
			0,  0,  0,  0,  0,  0,  0,  0, 
			0,  0,  0,  0,  0,  0,  0,  0, 
			0,  0,  0,  0,  0,  0,  0,  0, 
			0,  0,  0,  0,  0,  0,  0,  0, 
			0,  0,  0,  0,  0,  0,  0,  0, 
			0,  0,  0,  0,  0,  0,  0,  0, 
			0,  0,  0,  0,  0,  0,  0,  0, 
			0,  0,  0,  0,  0,  0,  0,  0, 
			2,  2,  2,  2,  2,  2,  2,  2, 
			2,  2,  2,  2,  2,  2,  2,  2, 
			2,  2,  2,  2,  2,  2,  2,  2, 
			2,  2,  2,  2,  2,  2,  2,  2, 
			3,  3,  3,  3,  3,  3,  3,  3, 
			3,  3,  3,  3,  3,  3,  3,  3, 
			4,  4,  4,  4,  4,  4,  4,  4, 
			5,  5,  5,  5,  6,  6,  0,  0
		};
		return charSizeTable[static_cast<unsigned char>(c)];
	}

	size_t charLengthSafe(char c)
	{
		size_t n = charLength(c);
		if(n < 1 || n > 4)
		{
			throw exception("incorrect lead byte");
		}
		return n;
	}

	size_t charSize(char32 c)
	{
		if(c <= 0x7f)
			return 1;
		if(c <= 0x7ff)
			return 2;
		if(c <= 0xd7ff)
			return 3;
		if(c <= 0xdfff || c > 0x10ffff)
			return 0;
		if(c <= 0xffff)
			return 3;
		return 4;
	}

	size_t width(const std::string &str)
	{
		size_t n = 0;
		std::string::const_iterator itr = str.begin();
		while(itr != str.end())
		{
			itr += charLength(*itr);
			++n;
		}
		return n;
	}

	static bool isTrail(char c)
	{
		return ((c)&0xc0)==0x80;
	}

	size_t width_safe(const std::string &str)
	{
		size_t n = 0;
		std::string::const_iterator itr = str.begin();
		while(itr != str.end())
		{
			size_t k = charLength(*itr++);
			while(k)
			{
				if(!isTrail(*itr))
				{
					throw exception(std::string("Trail byte is expected: ") + boost::lexical_cast<std::string>(*itr) );
				}
				--k;
				++itr;
			}
			n++;
		}
		return n;
	}

	void pushChar(std::string &str, char32 c)
	{
		switch(charSize(c))
		{
		case 1:
			str.push_back(static_cast<unsigned char>(c));
			break;
		case 2:
			str.push_back((c >> 6)   | 0xc0);
			str.push_back((c & 0x3F) | 0x80);
			break;
		case 3:
			str.push_back( (c >> 12)         | 0xe0);
			str.push_back(((c >>  6) & 0x3F) | 0x80);
			str.push_back( (c & 0x3F)        | 0x80);
			break;
		case 4:
			str.push_back( (c >> 18)         | 0xf0);
			str.push_back(((c >> 12) & 0x3F) | 0x80);
			str.push_back(((c >>  6) & 0x3F) | 0x80);
			str.push_back( (c & 0x3F)        | 0x80);
			break;
		}
	}


	template<typename T1, typename T2>
	char32 next_char(T1 &itr, const T2 &itr_end)
	{
		if(itr == itr_end)
			return 0;
		char32 c = (unsigned char)*itr++;
		if(c-0xc0 < 0x35)
		{
			if(itr == itr_end)
				return 0;
			size_t count = charLength(c);

			c &= (1 << (7-count))-1; // mask the lead byte

			switch(count)
			{
			case 4:
				c = (c << 6) | ((*itr++) & 0x3f);
				if(itr == itr_end)
					return 0;
			case 3:
				c = (c << 6) | ((*itr++) & 0x3f);
				if(itr == itr_end)
					return 0;
			case 2:
				c = (c << 6) | ((*itr++) & 0x3f);
				if(itr == itr_end)
					return 0;
				break;
			default:
				BOOST_ASSERT(0 && "undefined state");
			}
		}
		return c;
	}

	char32 nextChar(std::string::const_iterator &itr, const std::string::const_iterator &itr_end)
	{
		return next_char(itr, itr_end);
	}

} // namespace utf8
} // namespace Unicode