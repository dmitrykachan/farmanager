#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"
#include <boost/algorithm/string/trim.hpp>
#include "utils/winwrapper.h"

/* ANY state */
bool FTP::ExecCmdLineANY(const std::wstring& str, bool Prefix)
{  
	bool             iscmd = ShowHosts || Prefix;
	FTPUrl           ui;
	QueueExecOptions op;

	std::wstring::const_iterator itr = str.begin();
	Parser::skipNSpaces(itr, str.end());
	if(itr != str.end())
		++itr;
	std::wstring cmd(str.begin(), itr);
	Parser::skipSpaces(itr, str.end());

	if(boost::iequals(cmd, "help") || cmd == L"?")
	{
		FARWrappers::getInfo().ShowHelp(FARWrappers::getModuleName(), L"FTPCommandLineHelp", FHELP_SELFHELP);
		return true;
	}

	if(iscmd == false)
		return false;

	if(boost::iequals(cmd, L"qadd"))
	{
		UrlInit(&ui);
		if(itr != str.end())
		{
			ui.SrcPath.assign(itr, str.end());
			if(EditUrlItem(&ui))
				AddToQueque(&ui);
		}
		return true;
	}

	if(boost::iequals(cmd, L"xadd"))
	{
		UrlInit(&ui);
		if(itr != str.end())
		{
			ui.SrcPath.assign(itr, str.end());
			if(EditUrlItem(&ui))
				AddToQueque(&ui);
		}
		SetupQOpt(&op);
		if(QuequeSize && WarnExecuteQueue(&op))
		{
			ExecuteQueue(&op);
			if(QuequeSize)
				QuequeMenu();
		}

		return true;
	}

	if(boost::iequals(cmd, L"q"))
	{
		QuequeMenu();
		return true;
	}

	if(boost::iequals(cmd, L"qx"))
	{
		SetupQOpt( &op );
		if(QuequeSize && WarnExecuteQueue(&op))
		{
			ExecuteQueue(&op);
			if(QuequeSize)
				QuequeMenu();
		}
		return true;
	}
	return false;
}

/* HOSTS state */
bool FTP::ExecCmdLineHOST(const std::wstring &str, bool prefix)
{
	if(boost::iequals(str, L"EXIT") || boost::iequals(str, L"QUIT"))
	{
		CurrentState = fcsClose;
		FARWrappers::getInfo().Control(this, FCTL_CLOSEPLUGIN, NULL);
		return true;
	}

	//Connect to ftp
	if(boost::starts_with(str, L"//"))
	{
		chost_.Init();
		chost_.SetHostName(std::wstring(L"ftp:") + str, L"", L"");
		FullConnect();
		return true;
	}
	
	//Connect to http
	if(boost::istarts_with(str, L"HTTP://"))
	{
		chost_.Init();
		chost_.SetHostName(str, L"", L"");
		FullConnect();
		return true;
	}

	if(boost::istarts_with(str, L"CD "))
	{
		std::wstring::const_iterator itr = str.begin()+3;
		Parser::skipSpaces(itr, str.end());
		return FtpFilePanel_.SetDirectory(std::wstring(itr, str.end()), 0);
	}
	return false;
}

#define FCMD_SINGLE_COMMAND 0
#define FCMD_FULL_COMMAND   1

#define FCMD_SHOW_MSG       0x0001
#define FCMD_SHOW_EMSG      0x0002

bool FTP::DoCommand(const std::wstring& str, int type, DWORD flags)
{
	FARWrappers::Screen scr;

	bool	ext = getConnection().getHost().ExtCmdView,
			dex = g_manager.opt.DoNotExpandErrors;
	int  rc;
	std::wstring m;

	getConnection().getHost().ExtCmdView = TRUE;
	g_manager.opt.DoNotExpandErrors     = FALSE;

	m = str;

	//Process command types
	switch( type ) 
	{
	case FCMD_SINGLE_COMMAND: 
		rc = getConnection().command(m);
		if(Connection::isPrelim(rc))
		{
			do 
			{
				rc = getConnection().getreply();
			} while(Connection::isPrelim(rc));
		}
		rc = Connection::isComplete(rc);
		break;

	case   FCMD_FULL_COMMAND: //Convert string quoting to '\x1'
		{
			std::wstring::iterator itr = m.begin();
			while(itr != m.end())
			{
				if(*itr == L'\\')
				{
					if(++itr == m.end())
						break;
				}
				else
				{
					if(*itr == L'\"')
						*itr = quoteMark;
				}
				++itr;
			}
			rc = getConnection().ProcessCommand(m);
		}
		break;
	}

	if ( (rc && is_flag(flags,FCMD_SHOW_MSG)) ||
		(!rc && is_flag(flags,FCMD_SHOW_EMSG)) )
		getConnection().ConnectMessage(MOk, L"", rc != 0, MOk);

	//Special process of known commands
	if(type == FCMD_SINGLE_COMMAND)
		if(boost::istarts_with(str, L"CWD "))
			resetCache_ = true;

	getConnection().getHost().ExtCmdView = ext;
	g_manager.opt.DoNotExpandErrors     = dex;
	return rc == RPL_OK;
}

/* FTP state */
bool FTP::ExecCmdLineFTP(const std::wstring& str, bool Prefix)
{
	std::wstring s = str;
	if(Prefix && s.size()> 0 && s[0] == L'/')
	{
		// TODO possible a bug
		FTPHost tmp;
		tmp.Assign(&chost_);
		if(boost::istarts_with(s, L"FTP:") == false)
			s = L"ftp:" + s;
		if(tmp.SetHostName(s, L"", L""))
		{
			if (chost_.CmpConnected(&tmp) )
			{
				FtpFilePanel_.SetDirectory(tmp.url_.directory_, 0);
				return true;
			}
			chost_.Assign(&tmp);
			FullConnect();
			return true;
		}
	}

	std::wstring::const_iterator itr = s.begin();
	Parser::skipNSpaces(itr, s.end());
	if(itr != s.end())
		++itr;
	std::wstring cmd(itr, s.end());
	Parser::skipSpaces(itr, s.end());
	s.assign(itr, s.end());

	//Switch to hosts
	if(boost::iequals(cmd, L"TOHOSTS") == 0)
	{
		getConnection().disconnect();
		return true;
	}

	//DIRFILE
	if(boost::iequals(cmd, L"DIRFILE") == 0)
	{
		getConnection().DirFile = s;
		return true;
	}

	//CMD
	if(boost::iequals(cmd, L"CMD"))
	{
		if(!s.empty())
			DoCommand(s, FCMD_FULL_COMMAND, FCMD_SHOW_MSG|FCMD_SHOW_EMSG );
		return true;
	}

	BOOST_LOG(INF, L"ProcessCmd" << chost_.ProcessCmd);
	if (!chost_.ProcessCmd)
		return false;

	//CD
	if(boost::iequals(cmd, L"CD"))
	{
		if(!s.empty())
		{
			FtpFilePanel_.SetDirectory(s, 0);
			return true;
		} 
		else
			return false;
	}

	//Manual command line to server
	if(!Prefix)
	{
		DoCommand(s, FCMD_SINGLE_COMMAND, FCMD_SHOW_MSG|FCMD_SHOW_EMSG);
		return true;
	}

	return false;
}

bool FTP::ExecCmdLine(const std::wstring& str, bool wasPrefix)
{
	BOOL      isConn = getConnection().isConnected();
	FARWrappers::Screen scr;

	if(str.empty())
		return false;

	std::wstring::const_iterator itr = str.begin();
	bool prefix = Parser::checkString(itr, str.end(), L"FTP:", false) || wasPrefix;

	do{
		//Any state
		if(ExecCmdLineANY(std::wstring(itr, str.end()), prefix))
			break;

		//HOSTS state
		if(ShowHosts && ExecCmdLineHOST(str, prefix))
			break;

		//CONNECTED state
		if(!ShowHosts && ExecCmdLineFTP(str, prefix))
			break;

		//Unprocessed
		if(prefix)
		{
			FARWrappers::getInfo().Control(this, FCTL_SETCMDLINE, (void*)L"");
			FARWrappers::getInfo().ShowHelp(FARWrappers::getModuleName(), L"FTPCommandLineHelp", FHELP_SELFHELP);
			return true;
		} else
			return false;
	}while(0);

	//processed
	FARWrappers::getInfo().Control( this,FCTL_SETCMDLINE,(void*)"" );

	if(isConn && (!getConnection().isConnected()))
		BackToHosts();

	if(CurrentState != fcsClose)
		Invalidate();
	else
		FARWrappers::getInfo().Control(0,FCTL_REDRAWPANEL,NULL);

	return TRUE;
}

template<typename Itr, typename CH>
Itr skip(Itr i, Itr e, CH ch)
{
	while(i != e && *i == ch)
	{
		++i;
	}
	return i;
}

int FTP::ProcessCommandLine(wchar_t *CommandLine)
{
	if(!CommandLine)
		return false;

	FTPUrl_ url;
	std::wstring cmdLine = boost::trim_copy(std::wstring(CommandLine));
	if(cmdLine.empty())
		return false;

	//Hosts
	if(cmdLine[0] != L'/')
		return ExecCmdLine(cmdLine, true);

	if(url.parse(cmdLine))
		return false;

	//Connect
	FTPUrl            ui;
	QueueExecOptions  op;
	std::wstring::iterator UrlName = 
		(*cmdLine.rbegin() != '/' && cmdLine.find(L'/') != std::wstring::npos)? 
			cmdLine.begin() + cmdLine.rfind(cmdLine, L'/') : cmdLine.end();

	chost_.Init();
	UrlInit( &ui );
	SetupQOpt( &op );

	if(UrlName != cmdLine.end())
		ui.SrcPath = L"ftp://" + cmdLine;

	//Get URL
	BOOST_ASSERT(0 && "TODO");
	if(!ui.SrcPath.empty())
	{
		if (!EditUrlItem(&ui))
		{
			static const wchar_t* itms[] = 
			{ 
				getMsg(MRejectTitle),
				getMsg(MQItemCancelled),
				getMsg(MYes),
				getMsg(MNo)
			};
			if(FARWrappers::message(itms, 2, FMSG_WARNING) != 0)
				return false;
			*UrlName = 0;
			UrlName++;
		} else {
			//User confirm downloading: execute queque
			AddToQueque( &ui );

			if ( QuequeSize &&
				WarnExecuteQueue(&op) ) {
					ExecuteQueue(&op);
					return TRUE;
			}

			delete UrlsList;
			UrlsList = UrlsTail = NULL;
			QuequeSize = 0;

			return FALSE;
		}
	}

	//Connect to host
	ClearQueue();

	//Fill`n`Connect
	chost_.SetHostName(CommandLine, L"", L"");

	if(!FullConnect())
		return false;

	FARWrappers::Screen::fullRestore();

	if(UrlName != cmdLine.end() && !ShowHosts)
		selectFile_.assign(UrlName, cmdLine.end()); // TODO selFile
	return true;
}
