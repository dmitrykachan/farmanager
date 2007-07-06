#ifndef UTF8_HEADER__
#define UTF8_HEADER__

#include "unicode.h"

namespace Unicode
{

namespace utf8
{
	size_t	charLength(char c);
	size_t	charSize(char32 c);
	size_t	charLengthSafe(char c);
	size_t	width(const std::string &str);
	size_t	width_safe(const std::string &str);

	void	pushChar(std::string &str, char32 c);
	char32	nextChar(std::string::const_iterator &itr, const std::string::const_iterator &itr_end);
	char32	nextChar(const char* &str);
	
	class exception: public std::exception
	{
	private:
		std::string msg_;
	public:
		exception(const char* msg)
			: msg_(msg)
		{}
		exception(const std::string &msg)
			: msg_(msg)
		{}
		virtual const char* what() const
		{
			return msg_.c_str();
		}
	};
} // namespace utf8
} // namespace Unicode
#endif
