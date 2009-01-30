#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

std::auto_ptr<CommandsList> Connection::commandsList_ = std::auto_ptr<CommandsList>(new CommandsList);

void Connection::lostpeer()
{
	if(isConnected())
	{
		AbortAllRequest();
	}
}

static void parseCommandLine(const std::wstring &line, std::vector<std::wstring> &v)
{
	v.clear();
	v.reserve(5);

	std::wstring::const_iterator itr;

	std::wstring token;

	itr = line.begin();
	enum
	{
		Normal, InQuote, Screen
	} state = Normal;
	while(itr != line.end())
	{
		token = L"";
		Parser::skipSpaces(itr, line.end());

		while(itr != line.end())
		{
			if(state != InQuote)
			{
				if(std::isspace(*itr, defaultLocale_))
				{
					++itr;
					break;
				} else
					switch(*itr)
				{
					case L'\\':
						if(state == Screen)
						{
							token += *itr;
							state = Normal;
						}
						else
							state = Screen;
						break;
					case quoteMark:
						state = InQuote;
						break;
					case L' ': case L'\t':
						break;
					default:
						token += *itr;
						break;
				}
			} else
			{
				if(*itr == quoteMark)
					state = Normal;
				else
					token += *itr;
			}
			++itr;
		}
		if(!token.empty())
			v.push_back(token);
	}

}

#ifdef CONFIG_TEST
BOOST_AUTO_TEST_CASE(TestParseCommandLine)
{
	std::vector<std::wstring> vec;
	parseCommandLine(L"", vec);
	BOOST_CHECK(vec.empty());

	parseCommandLine(L"  abc\t ", vec);
	BOOST_CHECK(vec.size() == 1 && vec[0] == L"abc");

	parseCommandLine(L"  abc\t def \\+-\\\\", vec);
	BOOST_CHECK(vec.size() == 3 && vec[0] == L"abc" && vec[1] == L"def" && vec[2] == L"+-\\");

	parseCommandLine(L"  abc\t def \x1\\+-\\\\\x1 ", vec);
	BOOST_CHECK(vec.size() == 3 && vec[0] == L"abc" && vec[1] == L"def" && vec[2] == L"\\+-\\\\");	
}
#endif


Connection::Result Connection::ProcessCommand(const std::wstring &line)
{  
	ResetOutput();
	std::vector<std::wstring> vec;
	parseCommandLine(line, vec);

	if(vec.empty()) 
	{
		AddCmdLine(L"Incorrect parameters", ldInt);
		return Error;
	}

	Command cmd;
	if(!commandsList_->find(vec[0], cmd))
	{
		BOOST_LOG(INF, L"!cmd");
		AddCmdLine(L"Unknown command", ldInt);
		return Error;
	}

	if(cmd.getMustConnect() && !isConnected())
	{
		BOOST_LOG(INF, L"!connected");
		AddCmdLine(L"This command is valid when the connection is active", ldInt);
		return Error;
	}

	brk_flag  = FALSE;
	
	Result res = cmd.execute(*this, vec);

	brk_flag = FALSE;

	return res;
}