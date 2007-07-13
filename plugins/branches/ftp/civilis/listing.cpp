#include "stdafx.h"
#include "listing.h"
#include "fileinfo.h"
#include "utils/uniconverts.h"

std::locale defaultLocale_;

namespace Listing
{
	enum ParsingMode
	{
		unix, vxworks, dos, automatic 
	};

	typedef std::wstring::const_iterator const_iterator;
	typedef std::wstring::iterator iterator;

	bool parseVxWorksListing(FTPFileInfo &fileinfo, const_iterator itr, const const_iterator &itr_end);
	bool parseUnixListing(FTPFileInfo &fileinfo, const_iterator itr, const const_iterator &itr_end);
	bool parseDosListing(FTPFileInfo &fileinfo, const_iterator itr, const const_iterator &itr_end);


	class Parser
	{
	public:
		virtual bool operator()(FTPFileInfo &fi, const_iterator& itr, const const_iterator& itr_end) const = 0;
	};

	class ParseVxWorks: public Parser
	{
		virtual bool operator()(FTPFileInfo &fi, const_iterator& itr, const const_iterator& itr_end) const
		{
			return parseVxWorksListing(fi, itr, itr_end);
		}

	};

	class ParseUnix: public Parser
	{
		virtual bool operator()(FTPFileInfo &fi, const_iterator& itr, const const_iterator& itr_end) const
		{
			return parseUnixListing(fi, itr, itr_end);
		}
	};

	class ParseDos: public Parser
	{
		virtual bool operator()(FTPFileInfo &fi, const_iterator& itr, const const_iterator& itr_end) const
		{
			return parseDosListing(fi, itr, itr_end);
		}
	};

	ParseUnix parseUnix;
	ParseVxWorks parseVxWorks;
	ParseDos parseDos;

	struct ParserTable
	{
		Parser		*parser_;
		ParsingMode mode_;
	};

	boost::array<ParserTable, 3> parserTable;
	
	void initParserTable()
	{
		parserTable[0].parser_ = &parseVxWorks;
		parserTable[0].mode_ = vxworks;
		parserTable[1].parser_ = &parseUnix;
		parserTable[1].mode_ = unix;
		parserTable[2].parser_ = &parseDos;
		parserTable[2].mode_ = dos;
	}

	/*
	drwxrwxrwx   1  pdt2        4096 TUE FEB 13 18:21:02 2007 load
	drwxrwxrwx   1  pdt2        4096 MON FEB 05 18:30:36 2007 web
	-rwxrwxrwx   1  pdt2       25554 MON FEB 05 18:29:50 2007 disk.sys
	-rwxrwxrwx   1  pdt2      898140 TUE FEB 13 18:20:56 2007 mainos.sym
	*/
	bool parseVxWorksListing(FTPFileInfo &fileinfo, 
		const_iterator itr, const const_iterator &itr_end)
	{
		std::wstring::const_iterator itr2;

		skipSpaces(itr, itr_end);
		fileinfo.setUnixMode(itr, itr_end);
		skipSpaces(itr, itr_end);
		skipNumber(itr, itr_end);

		itr2 = itr;
		skipWord(itr, itr_end, false);
		fileinfo.setOwner(itr2, itr);
		skipSpaces(itr, itr_end);

		if(fileinfo.setFileSize(
			parseUnsignedNumber(itr, itr_end, FTPFileInfo::incorrectSize)) == false)
			return false;
		skipWord(itr, itr_end);
		Month mon;
		if(parseMonth(itr, itr_end, mon) == false)
			return false;
		int day;
		if(!parseUnsignedNumberRange(itr, itr_end, day, 1, 31))
			return false;

		WinAPI::FileTime time;
		if(!parseTime(itr, itr_end, time))
			return false;

		int year;
		if(!parseUnsignedNumberRange(itr, itr_end, year, 1900, 2100))
			return false;
		if(!time.setDate(year, mon, day))
			return false;
		fileinfo.setLastWriteTime(time);

		fileinfo.setFileName(itr, itr_end, false);
		return true;
	}

/*
	2005-06-20
*/
	bool parseFullDate(std::wstring::const_iterator &itr, 
		const std::wstring::const_iterator &itr_end, WinAPI::FileTime &ft)
	{
		int year, day, month;
		if(!parseUnsignedNumberRange(itr, itr_end, year, 1900, 2100))
			return false;
		if(*itr != L'-')
			return false;
		++itr;
		if(!parseUnsignedNumberRange(itr, itr_end, month, 1, 12))
			return false;
		if(*itr != L'-')
			return false;
		++itr;
		if(!parseUnsignedNumberRange(itr, itr_end, day, 1, 31))
			return false;

		return ft.setDate(year, month, day);
	}

	bool parseDate(std::wstring::const_iterator &itr, 
		const std::wstring::const_iterator &itr_end, WinAPI::FileTime &ft)
	{
		int year, day, month;

		if(!parseUnsignedNumberRange(itr, itr_end, month, 1, 12))
			return false;
		if(*itr != L'-')
			return false;
		++itr;

		if(!parseUnsignedNumberRange(itr, itr_end, day, 1, 31))
			return false;
		if(*itr != L'-')
			return false;
		++itr;

		if(!parseUnsignedNumberRange(itr, itr_end, year, 0, 99))
			return false;
		year += (year < 50)? 2000 : 1900 ;

		return ft.setDate(year, month, day);
	}

/*
	Nov  8 23:23
	Nov 15 2006
	2005-06-20 14:22
*/
	bool parseUnixDateTime(std::wstring::const_iterator &itr, 
		const std::wstring::const_iterator &itr_end, WinAPI::FileTime &ft)
	{
		bool notParseTime = false;
		if(std::isdigit(*itr, defaultLocale_))
		{
			if(!parseFullDate(itr, itr_end, ft))
				return false;
		} else
		{
			Month mon;
			if(parseMonth(itr, itr_end, mon) == false)
				return false;

			int day;
			if(!parseUnsignedNumberRange(itr, itr_end, day, 1, 31))
				return false;

			int year;
			std::wstring::const_iterator itr_save = itr;
			if(!parseUnsignedNumberRange(itr, itr_end, year, 1900, 2100))
			{
				year = WinAPI::FileTime::getCurrentSystemTime().wYear;
				itr = itr_save;
			}
			else
				notParseTime = true;
			ft.setDate(year, mon, day);
		}

		if(notParseTime)
			return ft.setTime(0, 0, 0);
		else
			return parseTime(itr, itr_end, ft);
	}

	/*
	* ls -l listing:
	*
	* drwxr-xr-x    2 montulli eng          512 Nov  8 23:23 CVS
	* -rw-r--r--    1 montulli eng         2244 Nov  8 23:23 Imakefile
	* -rw-r--r--    1 montulli eng        14615 Nov  9 17:03 Makefile
	*
	* ls -s listing:
	*
	* 69792668804186112 drwxrw-rw- 1 root  root    0 Nov 15 14:01 .
	* 69792668804186112 drwxrw-rw- 1 root  root    0 Nov 15 14:01 ..
	* 69792668804186113 dr-x-w---- 1 root  root  512 Dec  1 02:16 Archives
	*
	* Full dates:
	*
	* -rw-r--r--  1 panfilov users 139264 2005-06-20 14:22 AddTransToStacks.doc
	* -rw-------  1 panfilov users    535 2005-07-08 19:21 .bash_history
	* -rw-r--r--  1 panfilov users    703 2004-10-14 14:14 .bash_profile
	* -rw-r--r--  1 panfilov users   1290 2004-10-14 14:14 .bashrc
	*/
	bool parseUnixListing(FTPFileInfo &fileinfo, 
		const_iterator itr, const const_iterator &itr_end)
	{
		std::wstring::const_iterator itr2;

		skipSpaces(itr, itr_end);
		if(std::isdigit(*itr, defaultLocale_))
			skipNumber(itr, itr_end);

		fileinfo.setUnixMode(itr, itr_end);
		skipString(itr, itr_end, "+", false);
		skipSpaces(itr, itr_end);
		skipNumber(itr, itr_end);

		itr2 = itr;
		skipWord(itr, itr_end, false);
		fileinfo.setOwner(itr2, itr);
		skipSpaces(itr, itr_end);

		// skip @
		skipString(itr, itr_end, L"@ ");
		
		itr2 = itr;
		skipWord(itr, itr_end, false);
		fileinfo.setGroup(itr2, itr);
		skipSpaces(itr, itr_end);

		// skip @
		skipString(itr, itr_end, L"@ ");

		if(fileinfo.getType() == FTPFileInfo::specialdevice)
		{
			// Special devices do not have size. But they have
			// device number fields. Skip it.
			skipNumber(itr, itr_end);
			skipString(itr, itr_end, ",");
			skipNumber(itr, itr_end);

		} else
		{
			if(fileinfo.setFileSize(
				parseUnsignedNumber(itr, itr_end, FTPFileInfo::incorrectSize)) == false)
				return false;
		}

		WinAPI::FileTime ft;
		if(!parseUnixDateTime(itr, itr_end, ft))
			return false;

		fileinfo.setLastWriteTime(ft);
		fileinfo.setFileName(itr, itr_end, fileinfo.getType() == FTPFileInfo::symlink);
		return true;
	}


/*
06-29-95  03:05PM       <DIR>          muntemp
05-02-95  10:03AM               961590 naxp11e.zip
*/

	bool parseDosListing(FTPFileInfo &fileinfo, 
		const_iterator itr, const const_iterator &itr_end)
	{
		skipSpaces(itr, itr_end);

		WinAPI::FileTime ft;
		if(!parseDate(itr, itr_end, ft))
			false;
		
		if(!parseTime(itr, itr_end, ft))
			false;

		fileinfo.setLastWriteTime(ft);
		
		if(skipString(itr, itr_end, L"<DIR>"))
		{
			fileinfo.setType(FTPFileInfo::directory);
			fileinfo.setFileSize(0);
		}
		else
		{
			fileinfo.setType(FTPFileInfo::file);
			if(fileinfo.setFileSize(
				parseUnsignedNumber(itr, itr_end, FTPFileInfo::incorrectSize)) == false)
				return false;
		}

		fileinfo.setFileName(itr, itr_end, fileinfo.getType() == FTPFileInfo::symlink);

		return true;
	}

	bool updateTime(FTPFileInfo& fileinfo)
	{
		WinAPI::FileTime ft = fileinfo.getLastWriteTime();
		if(ft.empty())
			return false;
		if(fileinfo.getCreationTime().empty())
			fileinfo.setCreationTime(ft);
		if(fileinfo.getLastAccessTime().empty())
			fileinfo.setLastAccessTime(ft);
		return true;
	}

	const Parser* findParser(ParsingMode mode)
	{
		boost::array<ParserTable, 2>::const_iterator itr;
		itr = std::find_if(parserTable.begin(), parserTable.end(), boost::bind(&ParserTable::mode_, _1) == mode);
		if(itr != parserTable.end())
			return itr->parser_;
		else
			return 0;
	}

	std::wstring parseListing(const std::wstring &listing, ParsingMode mode)
	{
		std::wstring::const_iterator itr = listing.begin();
		std::wstring::const_iterator itr_end;

		std::wostringstream stream;

		do
		{
			FTPFileInfo fileinfo;
			itr_end = std::find(itr, listing.end(), '\n');
			if(itr == itr_end)
				break;
			const Parser* parser = findParser(mode);
			BOOST_ASSERT(parser);

			bool res = (*parser)(fileinfo, itr, itr_end);
			if(res)
				res = updateTime(fileinfo);
//			BOOST_CHECK(res);
			if(res)
				BOOST_CHECK(stream << fileinfo);
			else
				BOOST_CHECK(stream << "FAILED!!\n");

			if(itr_end == listing.end())
				break;
			itr = itr_end+1;
		} while(true);
		return stream.str();
	}
}


#ifdef CONFIG_TEST
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/included/unit_test_framework.hpp>


static Listing::ParsingMode chartomode(char c)
{
	switch(c)
	{
	case 'a': return Listing::automatic;
	case 'u': return Listing::unix;
	case 'v': return Listing::vxworks;
	case 'd': return Listing::dos;
	default:
		return Listing::automatic;
	}
}


boost::unit_test::test_suite*
init_unit_test_suite( int argc, char* argv[] ) {
	boost::unit_test::auto_unit_test_suite_t* master_test_suite = boost::unit_test::auto_unit_test_suite();
	
	boost::unit_test::const_string new_name = boost::unit_test::const_string( BOOST_AUTO_TEST_MAIN );

	if( !new_name.is_empty() )
		boost::unit_test::assign_op( master_test_suite->p_name.value, new_name, 0 );

	master_test_suite->argc = argc;
	master_test_suite->argv = argv;

	return master_test_suite;
}



BOOST_AUTO_TEST_CASE(listing_test)
{
	std::ifstream fi("test\\ftpdir.txt", std::ifstream::binary);
	std::ofstream os("test\\ftpdir.out");
	BOOST_REQUIRE(fi.good());

	std::wstring indir;
	std::wstring outdir;
	std::wstring res;
	std::string line;
	char mode = -1;

	enum State
	{
		start, ftpdir, outputdir
	};
	State state = start;

	Listing::initParserTable();

	while(getline(fi, line))
	{
		if(*(line.end()-1) == '\r')
		{
			line.erase(line.end()-1, line.end());
		}
		if(line.compare(0, 4, ">>>>") == 0)
		{
			if(!indir.empty())
			{
				res = Listing::parseListing(indir, chartomode(mode));
				os << Unicode::utf16ToUtf8(res) << "---------------\n";
				if(state == outputdir)
				{
					std::wstring::const_iterator i1,i2,i_end, o1,o2,o_end;
					i1		= res.begin();
					i_end	= res.end();
					o1		= outdir.begin();
					o_end	= outdir.end();
					do 
					{
						if(i1 == i_end || o1 == o_end)
						{
							if((i1 == i_end) ^ (o1 == o_end))
								BOOST_CHECK(false);
							break;
						}
						i2 = std::find(i1, i_end, L'\n');
						o2 = std::find(o1, o_end, L'\n');
						BOOST_CHECK_MESSAGE(std::wstring(i1, i2) == std::wstring(o1, o2), 
							Unicode::utf16ToUtf8(std::wstring(i1, i2) + L"!=" + std::wstring(o1, o2)));
						i1 = i2;
						if(i1 != i_end)
							++i1;
						o1 = o2;
						if(o1 != o_end)
							++o1;

					} while(true);
				}
			}
			indir = L"";
			state = ftpdir;
			if(line.size() > 4)
				mode = line[4];
			else
				mode = -1;
			continue;
		}
		if(line == "<<<<")
		{
			outdir = L"";
			state = outputdir;
			continue;
		}
		switch(state)
		{
			case ftpdir:
				indir += Unicode::utf8ToUtf16(line) + L'\n';
				break;
			case outputdir:
				outdir += Unicode::utf8ToUtf16(line) + L'\n';
				break;
		}
	}
}


static void checkParsingFullDate(const std::wstring &str, const std::wstring &res)
{
	WinAPI::FileTime ft;
	std::wstring outstr;
	BOOST_CHECK_MESSAGE(Listing::parseFullDate(str.begin(), str.end(), ft), 
		std::string("Parse error: ") + Unicode::utf16ToUtf8(str));
	ft.getDate(outstr);
	BOOST_CHECK_MESSAGE(outstr == res, Unicode::utf16ToUtf8(outstr + L" instead " + res));
}

static void checkParsingDateTime(const std::wstring &str, const std::wstring &res)
{
	WinAPI::FileTime ft;
	std::wstring outstr;
	BOOST_CHECK_MESSAGE(Listing::parseUnixDateTime(str.begin(), str.end(), ft), 
		std::string("Parse error: ") + Unicode::utf16ToUtf8(str));
	ft.getDate(outstr);
	BOOST_CHECK_MESSAGE(outstr == res, Unicode::utf16ToUtf8(outstr + L" instead " + res));
}


BOOST_AUTO_TEST_CASE(parsing_date_time)
{
	std::wstring str;

	checkParsingFullDate(L"2005-12-13", L"13.12.2005");
	checkParsingFullDate(L"2005-2-13",  L"13.02.2005");
	checkParsingFullDate(L"2005-2-1",   L"01.02.2005");

	std::wstring currentYear = boost::lexical_cast<std::wstring>(WinAPI::FileTime::getCurrentSystemTime().wYear);

	checkParsingDateTime(L"2005-06-20 14:22",   L"20.06.2005");
	checkParsingDateTime(L"Nov 8 23:23",   L"08.11." + currentYear);
	checkParsingDateTime(L"Nov 15 14:01",  L"15.11." + currentYear);
	checkParsingDateTime(L"Mar 28  2000",  L"28.03.2000");
}
#endif