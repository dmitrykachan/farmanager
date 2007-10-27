#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

BOOL FTP::DoFtpConnect( int blocked )
{  
	std::wstring  hst, usr, pwd, port;
	BOOL  askPwd = chost_.AskLogin;

	SetLastError( ERROR_SUCCESS );

	if (chost_.url_.Host_.empty())
		return FALSE;

	CurrentState = fcsConnecting;

	getConnection().InitData(&chost_, blocked);
	getConnection().InitCmdBuff();
	getConnection().InitIOBuff();
	getConnection().LoginComplete = true;

	do{
		getConnection().getHost().ExtCmdView = chost_.ExtCmdView;

		hst = chost_.url_.Host_;
		usr = chost_.url_.username_;
		pwd = chost_.url_.password_;

		if ( !getConnection().LoginComplete )
			if ( !GetLoginData(usr, pwd, chost_.AskLogin || askPwd) )
				break;

		//Firewall
		if(chost_.UseFirewall && !g_manager.opt.firewall.empty())
		{
			usr += L"@" + hst;
			hst += g_manager.opt.firewall;
		}

		//Connect
		BOOST_LOG(INF, L"Connecting");
		if(getConnection().Init(hst, chost_.url_.port_, usr, pwd, this))
		{
			FtpSystemInfo(&getConnection());

			if(!FtpGetFtpDirectory(&getConnection()))
			{
				BOOST_LOG(INF, L"!Connect - get dir");
				return FALSE;
			}

			getConnection().CheckResume();

			CurrentState = fcsFTP;
			return TRUE;
		}
		BOOST_LOG(INF, L"!Init");

		if (!getConnection().LoginComplete &&
			g_manager.opt.AskLoginFail/*TODO && hConnect->GetResultCode() == NotLoggedIn*/) {
				BOOST_LOG(INF, L"Reask password");
				getConnection().getHost().ExtCmdView = TRUE;
				getConnection().ConnectMessage( MNone__, hst, MNone__);
				askPwd = TRUE;
				continue;
		}

		//!Connected
		SetLastError(getConnection().ErrorCode);

		//!Init
//TODO		if ( hConnect->SocketError != INVALID_SOCKET )
//			hConnect->ConnectMessage(MWSANotInit, GetSocketErrorSTR(hConnect->SocketError), false, MOk);
//		else
			//!Cancel, but error
			if (getConnection().ErrorCode != ERROR_CANCELLED)
				getConnection().ConnectMessage(MCannotConnectTo, chost_.url_.Host_, true, MOk );

		break;
	}while(1);

	CurrentState = fcsNormal;
	return FALSE;
}

#define TOUNICODE_(x) L##x
#define TOUNICODE(x) TOUNICODE_(x)

int FTP::Connect()
{
	PROC;
	ConnectionState  cst;
	FARWrappers::Screen scr;

	LogCmd(L"-------- CONNECTING (plugin: " TOUNICODE(__DATE__) L", " TOUNICODE(__TIME__) L") --------",ldInt );

	//Close old
	if (getConnection().isConnected())
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
		DoFtpConnect( cst.Inited ? cst.Blocked : (-1));
		if ( cst.Inited ) {
			//Restore old connection values or set defaults
			BOOST_LOG(INF, L"Restore connection datas");
			getConnection().SetState( &cst );
		} else {
			//Init new connection values
			BOOST_LOG(INF, L"Set connection defaults");
			getConnection().getHost().PassiveMode = chost_.PassiveMode;
		}

		//Set start FTP mode
		ShowHosts			= false;
		panel_				= &FtpFilePanel_;
		SwitchingToFTP		= true;
		FARWrappers::getInfo().Control(ANOTHER_PANEL, FCTL_REDRAWPANEL, NULL);

		//Set base directory
		if(!chost_.url_.directory_.empty())
		{
			FtpSetCurrentDirectory(&getConnection(), chost_.url_.directory_);
		}

		IdleMessage( NULL,0 );
	}
	catch (ConnectionError &e)
	{
		BOOST_LOG(ERR, L"!Connected");
		getConnection().AddCmdLine(e.what(), ldRaw);
		getConnection().ConnectMessage(MCannotConnectTo, chost_.url_.Host_, true, MOk );
		chost_.Init();
		IdleMessage( NULL,0 );
		BackToHosts();
		
		return false;
	}
	return true;
}

BOOL FTP::FullConnect()
{
	if ( Connect() )
	{
		if ( !ShowHosts ) 
		{
			bool isSaved = FARWrappers::Screen::isSaved();

			FARWrappers::Screen::fullRestore();
			FARWrappers::getInfo().Control( this,FCTL_SETVIEWMODE,&startViewMode_ );
			FARWrappers::getInfo().Control( this,FCTL_UPDATEPANEL,NULL );

			PanelRedrawInfo ri;
			ri.CurrentItem = ri.TopPanelItem = 0;
			FARWrappers::getInfo().Control(this,FCTL_REDRAWPANEL,&ri);
			if(isSaved)
				FARWrappers::Screen::save();

			SwitchingToFTP = FALSE;
			return TRUE;
		}
	}

	return FALSE;
}
