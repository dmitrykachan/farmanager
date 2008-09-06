#include "stdafx.h"
#include "servertype.h"
#include "utils/uniconverts.h"
#include "utils/codecvt.h"

#ifdef CONFIG_TEST
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/included/unit_test_framework.hpp>

#include "ftp_Int.h"

extern	FTPPluginManager g_manager;

class Sturtup
{
public:
	Sturtup()
	{
		PluginStartupInfo info;
		memset(&info, 0, sizeof(info));

		info.GetMsg = getMsg;
		info.Message = message;
		FARWrappers::setStartupInfo(info);	

		g_manager.openRegKey(L"Software\\Far18\\Plugins\\FTP_debug");
	}
	
	static const wchar_t* WINAPI getMsg(INT_PTR PluginNumber, int MsgId)
	{
		static std::wstring s = L"Message: " + boost::lexical_cast<std::wstring>(MsgId);
		return s.c_str();
	}

	static int WINAPI message(INT_PTR PluginNumber, DWORD Flags, const wchar_t *HelpTopic,
		const wchar_t * const *Items, int ItemsNumber, int ButtonsNumber)
	{
		return true;
	};


} startup;

::boost::unit_test::test_suite*
init_unit_test_suite( int, char* [] )
{
	using namespace ::boost::unit_test;
	assign_op( framework::master_test_suite().p_name.value, BOOST_TEST_STRINGIZE( BOOST_TEST_MODULE ).trim( "\"" ), 0 );
	return 0;
}


std::wistream& operator >> (std::wistream& is, std::vector<FTPFileInfo> &list)
{
	FTPFileInfo fi;
	list.clear();
	while(is >> fi)
	{
		list.push_back(fi);
	}
	return is;
}

std::wostream& operator << (std::wostream& os, const std::vector<FTPFileInfo> &list)
{

	std::copy(list.begin(), list.end(), std::ostream_iterator<FTPFileInfo, wchar_t>(os, L"\n"));
	return os;
}

static bool updateTime(FTPFileInfo& fileinfo)
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

BOOST_AUTO_TEST_CASE(listing_test)
{
	std::ifstream fi("test\\ftpdir.txt", std::ifstream::binary);
	std::wofstream os("test\\ftpdir.out");

	os.rdbuf()->pubimbue(std::locale(std::locale(), new utf8_conversion()));
	BOOST_REQUIRE(fi.good());


	std::wstringstream ss;
	ss << L"[d]   drws--s---   2007-06-04 14:11:00   2007-06-04 14:11:00   2007-06-04 14:11:00         4096   [      lion]   [     users]   [   file.name]   [  ]";

	FTPFileInfo ff;
	{
		BOOST_CHECK(ss >> ff);
	}

	std::wstring indir;
	std::wstring outdir;
	std::wstring res;
	std::string line;
	std::wstring wline;
	std::wstring parser;
	std::vector<FTPFileInfo> filesinfo;
	std::vector<FTPFileInfo> filesinfo_orig;

	enum State
	{
		start, ftpdir, outputdir
	};
	State state = start;

	while(std::getline(fi, line))
	{
		if(*(line.end()-1) == '\r')
		{
			line.erase(line.end()-1, line.end());
		}
		wline = Unicode::utf8ToUtf16(line);
		if(wline.compare(0, 4, L">>>>") == 0)
		{
			if(!indir.empty())
			{
				ServerList &servers = ServerList::instance();
				ServerList::const_iterator ser = servers.find(parser);
				BOOST_CHECK_MESSAGE(ser != servers.end(), "unknown server name");

				std::wstring::iterator itr = indir.begin();
				ServerType::entry entry;

				filesinfo.clear();

				while(true)
				{
					if(itr == indir.end())
						break;
					FTPFileInfo info;
					try
					{
						entry = (*ser)->getEntry(itr, indir.end());
						if(entry.first == entry.second)
							continue;

						(*ser)->parseListingEntry(entry.first, entry.second, info);
						updateTime(info);
						filesinfo.push_back(info);
					}
					catch (std::exception &)
					{
						BOOST_ASSERT(0);
					}

				}


				std::wistringstream(outdir) >> filesinfo_orig;

				os << filesinfo;
				os << "------------------------------------------------------------------------------------\n";
				if(state == outputdir)
				{
					std::vector<FTPFileInfo>::const_iterator itr1, itr2, itr1_end, itr2_end;
					itr1 = filesinfo.begin();
					itr1_end = filesinfo.end();
					itr2 = filesinfo_orig.begin();
					itr2_end = filesinfo_orig.end();
					while(itr1 != itr1_end && itr2 != itr2_end)
					{
						std::wstring diff = getTextDiff(*itr1, *itr2);
						BOOST_CHECK_MESSAGE(diff == L"", 
							Unicode::utf16ToUtf8(std::wstring(L"\n") + diff));
						++itr1; ++itr2;
					}
					BOOST_CHECK_MESSAGE(itr1 == itr1_end && itr2 == itr2_end, "Different number of files");
				}
			}
			indir = L"";
			state = ftpdir;
			if(wline.size() > 4)
				parser.assign(wline.begin()+4, wline.end());
			else
				wline = L"";
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

BOOST_AUTO_TEST_CASE(util_string_test)
{
	std::wstring s = L"abcde\"fffff\"\"ggggg\"vvv";
	Utils::removeDuplicatedCharacter(s, '\"');
	BOOST_CHECK_MESSAGE(s == L"abcde\"fffff\"ggggg\"vvv", "removeDuplicatedCharacter error");

	s = L"\"";
	Utils::removeDuplicatedCharacter(s, '\"');
	BOOST_CHECK_MESSAGE(s == L"\"", "removeDuplicatedCharacter error");

	s = L"\"";
	Utils::removeDuplicatedCharacter(s, '\"');
	BOOST_CHECK_MESSAGE(s == L"\"", "removeDuplicatedCharacter error");

	BOOST_CHECK(Utils::FDigit(0, L',')			== L"0"); 
	BOOST_CHECK(Utils::FDigit(1, L',')			== L"1"); 
	BOOST_CHECK(Utils::FDigit(12, L',')			== L"12"); 
	BOOST_CHECK(Utils::FDigit(123, L',')		== L"123"); 
	BOOST_CHECK(Utils::FDigit(1234, L',')		== L"1,234"); 
	BOOST_CHECK(Utils::FDigit(12345, L',')		== L"12,345"); 
	BOOST_CHECK(Utils::FDigit(123456, L',')		== L"123,456"); 
	BOOST_CHECK(Utils::FDigit(1234567, L',')	== L"1,234,567"); 
	BOOST_CHECK(Utils::FDigit(12345678, L',')	== L"12,345,678"); 
	BOOST_CHECK(Utils::FDigit(123456789, L',')	== L"123,456,789"); 
	BOOST_CHECK(Utils::FDigit(1234567890, L',')	== L"1,234,567,890"); 
	BOOST_CHECK(Utils::FDigit(12345678901, L',')== L"12,345,678,901"); 
	BOOST_CHECK(Utils::FDigit(112345678901, L',')== L"112,345,678,901"); 
	BOOST_CHECK(Utils::FDigit(1112345678901, L',')== L"1,112,345,678,901"); 

	BOOST_CHECK(Utils::FDigit(12345678901)== L"12345678901");
}

#endif