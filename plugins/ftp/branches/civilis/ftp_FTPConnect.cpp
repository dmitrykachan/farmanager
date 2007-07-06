#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

BOOL FTP::DoFtpConnect( int blocked )
{  
	std::wstring  hst, usr, pwd, port;
	size_t inx;
	BOOL  askPwd = Host.AskLogin;

	hConnect = NULL;
	SetLastError( ERROR_SUCCESS );

	if (Host.Host_.empty())
		return FALSE;

	CurrentState = fcsConnecting;

	//Create`n`Init connection
	hConnect = new Connection;
	if (!hConnect) {
		CurrentState = fcsNormal;
		return FALSE;
	}

	hConnect->InitData( &Host,blocked );
	hConnect->InitCmdBuff();
	hConnect->InitIOBuff();

	do{
		hConnect->Host.ExtCmdView = Host.ExtCmdView;

		hst = Host.hostname_;
		usr = Host.username_;
		pwd = Host.password_;

		if ( !hConnect->LoginComplete )
			if ( !GetLoginData(usr, pwd, Host.AskLogin || askPwd) )
				break;

		//Firewall
		if (Host.UseFirewall && !g_manager.opt.firewall.empty())
		{
			usr += L"@" + hst;
			hst += g_manager.opt.firewall;
		}

		// Find port
		inx = hst.find(L':');

		if(inx != std::wstring::npos)
		{
			BOOST_ASSERT("TEST port");
			port.assign(hst.begin() + inx + 1, hst.end());
			hst.resize(inx);
		} else
			port = L"0";

		//Connect
		Log(( "Connecting" ));
		if ( hConnect->Init(Unicode::toOem(hst).c_str(), 
							Unicode::toOem(port).c_str(), 
							Unicode::toOem(usr).c_str(), 
							Unicode::toOem(pwd).c_str()) )
		{
			Log(( "Connecting succ %d",hConnect->GetResultCode() ));
			if ( hConnect->GetResultCode() > 420 ) {
				Log(( "Connect error from server [%d]",hConnect->GetResultCode() ));
				return FALSE;
			}

			FtpSystemInfo(hConnect);

			if ( !FtpGetFtpDirectory(hConnect) ) {
				Log(( "!Connect - get dir" ));
				return FALSE;
			}

			hConnect->CheckResume();

			CurrentState = fcsFTP;
			return TRUE;
		}
		Log(( "!Init" ));

		if ( !hConnect->LoginComplete &&
			g_manager.opt.AskLoginFail && hConnect->GetResultCode() == 530 ) {
				Log(( "Reask password" ));
				hConnect->Host.ExtCmdView = TRUE;
				hConnect->ConnectMessage( MNone__, Unicode::toOem(hst).c_str(), MNone__);
				askPwd = TRUE;
				continue;
		}

		//!Connected
		SetLastError( hConnect->ErrorCode );

		//!Init
		if ( hConnect->SocketError != INVALID_SOCKET )
			hConnect->ConnectMessage( MWSANotInit,GetSocketErrorSTR(hConnect->SocketError),-MOk );
		else
			//!Cancel, but error
			if ( hConnect->ErrorCode != ERROR_CANCELLED )
				hConnect->ConnectMessageW( MCannotConnectTo,Host.Host_,-MOk );

		break;
	}while(1);

	CurrentState = fcsNormal;
	delete hConnect;
	hConnect = NULL;

	return FALSE;
}

int FTP::Connect()
  {  PROC(("FTP::Connect",NULL))
     ConnectionState  cst;
     FP_Screen        _scr;

     LogCmd( "-------- CONNECTING (plugin: " __DATE__ ", " __TIME__ ") --------",ldInt );

     do{
//Close old
      if (hConnect) {
        Log(( "old connection %p",hConnect ));
        hConnect->GetState( &cst );
        Log(( "del connection %p",hConnect ));
        delete hConnect;
        hConnect = NULL;
        Log(( "del connection done" ));
      }

//Connect
      if ( g_manager.opt.ShowIdle && IS_SILENT(FP_LastOpMode) ) {
        Log(( "Say connecting" ));
        SetLastError( ERROR_SUCCESS );
        IdleMessage( FP_GetMsg(MConnecting),g_manager.opt.ProcessColor );
      }

      if ( !DoFtpConnect( cst.Inited ? cst.Blocked : (-1) ) ||
           !hConnect ) {
        Log(( "!Connected" ));
        break;
      }

      if ( cst.Inited ) {
      //Restore old connection values or set defaults
        Log(( "Restore connection datas" ));
        hConnect->SetState( &cst );
      } else {
      //Init new connection values
        Log(( "Set connection defaults" ));
        // TODO hConnect->SetTable( TableNameToValue(Host.HostTable) );
        hConnect->Host.PassiveMode = Host.PassiveMode;
      }

//Set start FTP mode
      ShowHosts			= false;
      SwitchingToFTP	= true;
      selectFile_		= L"";
      FP_Info->Control( this,FCTL_REDRAWANOTHERPANEL,NULL );

//Set base directory
      if(hConnect && !Host.directory_.empty())
	  {
		  FtpSetCurrentDirectory(hConnect, Host.directory_);
      }

      IdleMessage( NULL,0 );
      return TRUE;
    }while(0);

    if ( hConnect ) {
      hConnect->ConnectMessageW( MCannotConnectTo, Host.Host_, -MOk );
      delete hConnect;
      hConnect = NULL;
    }
    Host.Init();
    IdleMessage( NULL,0 );

 return FALSE;
}

BOOL FTP::FullConnect()
{
	if ( Connect() )
	{
		if ( !ShowHosts ) 
		{
			BOOL isSaved = FP_Screen::isSaved() != 0;

			FP_Screen::FullRestore();
			FP_Info->Control( this,FCTL_SETVIEWMODE,&StartViewMode );
			FP_Info->Control( this,FCTL_UPDATEPANEL,NULL );

			PanelRedrawInfo ri;
			ri.CurrentItem = ri.TopPanelItem = 0;
			FP_Info->Control(this,FCTL_REDRAWPANEL,&ri);
			if ( isSaved )
				FP_Screen::Save();

			SwitchingToFTP = FALSE;
			return TRUE;
		}
	}

	return FALSE;
}
