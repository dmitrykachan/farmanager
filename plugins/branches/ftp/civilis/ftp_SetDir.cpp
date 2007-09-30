#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

bool FTP::SetDirectoryStepped(const std::wstring &Dir, bool update)
{
	//Empty dir
	if (Dir.empty())
		return true;

	//Try direct change
	if(FtpSetCurrentDirectory(&getConnection(), Dir))
	{
		if (update) Invalidate();
		return true;
	}

	//dir name contains only one name
	if(Dir.find(L'/') == std::wstring::npos)
	{
		//Cancel
		return FALSE;
	}

	//Cancel changes in automatic mode
	if ( IS_SILENT(FP_LastOpMode) )
		return FALSE;

	//Ask user
	{  static const wchar_t* MsgItems[] = {
		getMsg(MAttention),
		getMsg(MAskDir1),
		NULL,
		getMsg(MAskDir3),
		getMsg(MYes), getMsg(MNo) };

		MsgItems[2] = Dir.c_str();
		if (FARWrappers::message(MsgItems, 2, FMSG_WARNING|FMSG_LEFTALIGN) != 0 )
		{
			SetLastError( ERROR_CANCELLED );
			return FALSE;
		}
	}

	//Try change to directory one-by-one
	std::wstring str;
	std::wstring::const_iterator itr = Dir.begin();
	std::wstring::const_iterator m; 

	do
	{
		if(*itr == L'/')
		{
			++itr;
			if(itr == Dir.end()) 
				break;
			str = L"/";
		} else
		{
			m = std::find(itr, Dir.end(), L'/');
			if(m != Dir.end())
			{
				str.assign(Dir.begin(), m);
				itr = m + 1;
			} else
				str = Dir;
		}

		BOOST_LOG(INF, L"Dir: [" << str << L"]");
		if ( !FtpSetCurrentDirectory(&getConnection(),str) )
			return FALSE;

		if (update) Invalidate();
	} while(m != Dir.end());

	return true;
}
