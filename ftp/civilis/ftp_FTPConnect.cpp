#include "stdafx.h"
#include "utils/uniconverts.h"

#pragma hdrstop

#include "ftp_Int.h"

bool FTP::DoFtpConnect(FtpHostPtr& host)
{
	SetLastError( ERROR_SUCCESS );

	if (host->url_.Host_.empty())
		return false;

	getConnection().InitData(host);
	getConnection().InitCmdBuff();

	BOOST_LOG(INF, L"Connecting");

	Connection::Result result = getConnection().Init(this);
	if(result == Connection::Done)
	{
		getConnection().getSystemInfo();

		if(!FtpFilePanel_.ftpGetCurrentDirectory())
		{
			BOOST_LOG(INF, L"!Connect - get dir");
			return FALSE;
		}

		getConnection().CheckResume();

		return true;
	}

	//Connect
	BOOST_LOG(INF, L"!Init");
	if(result != Connection::Cancel)
		getConnection().ConnectMessage(MCannotConnect, getConnection().getHost()->url_.Host_, true, MOk );

	return false;
}

#define TOUNICODE_(x) L##x
#define TOUNICODE(x) TOUNICODE_(x)

int FTP::Connect(FtpHostPtr& host)
{
	PROC;
	FARWrappers::Screen scr;

	LogCmd(this, L"-------- CONNECTING (plugin: " TOUNICODE(__DATE__) L", " TOUNICODE(__TIME__) L") --------", ldInt);

	getConnection().disconnect();

	//Connect
	if(g_manager.opt.ShowIdle && IS_SILENT(FP_LastOpMode))
	{
		BOOST_LOG(INF, L"Say connecting");
		SetLastError( ERROR_SUCCESS );
		IdleMessage( getMsg(MConnecting),g_manager.opt.ProcessColor );
	}

	try
	{
		if(DoFtpConnect(host))
		{
			//Set start FTP mode
			ShowHosts			= false;
			panel_				= &FtpFilePanel_;
			FARWrappers::getInfo().Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);

			//Set base directory
			if(!getConnection().getHost()->url_.directory_.empty())
			{
				FtpFilePanel_.ftpSetCurrentDirectory(getConnection().getHost()->url_.directory_);
			}

			IdleMessage( NULL,0 );
		} else
			return false;
	}
	catch (std::exception &exc)
	{
		std::string s = exc.what();
		getConnection().AddCmdLine(Unicode::AsciiToUtf16(s), ldInt);
		getConnection().ConnectMessage(MCannotConnect, getConnection().getHost()->url_.Host_, true, MOk);
		IdleMessage(NULL, 0);
		BackToHosts();
		return false;
	}
	return true;
}

bool FTP::FullConnect(FtpHostPtr& host)
{
	if(Connect(host))
	{
		if(!ShowHosts)
		{
			bool isSaved = FARWrappers::Screen::isSaved();

			FARWrappers::Screen::fullRestore();
			FARWrappers::getInfo().Control(this, FCTL_SETVIEWMODE, startViewMode_, 0);
			FARWrappers::getInfo().Control(this, FCTL_UPDATEPANEL, 0, 0);

			PanelRedrawInfo ri;
			ri.CurrentItem = ri.TopPanelItem = 0;
			FARWrappers::getInfo().Control(this,FCTL_REDRAWPANEL, 0, reinterpret_cast<LONG_PTR>(&ri));
			if(isSaved)
				FARWrappers::Screen::save();

			return true;
		}
	}

	return false;
}
