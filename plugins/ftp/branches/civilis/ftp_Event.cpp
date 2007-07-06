#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

int FTP::ProcessEvent(int Event,void *Param)
{
#if defined(__FILELOG__)
	static CONSTSTR states[] = { "fcsNormal", "fcsExpandList", "fcsClose", "fcsConnecting","fcsFTP" };

	PROC(( "ProcessEvent","%d (%s),%08X",Event,states[CurrentState],Param ))
#endif
		PanelInfo pi;
	int       n;
	const FTPHost*  p;
	pi.PanelItems = NULL;

	if ( Event == FE_BREAK ||
		CurrentState == fcsClose ||
		CurrentState == fcsConnecting )
	{
			Log(( "Skip event" ));
			return FALSE;
	}

	//Close notify
	if ( Event == FE_CLOSE )
	{
		Log(( "Close notify" ));
		CurrentState = fcsClose;

		if(ShowHosts)
		{
			if(!pi.PanelItems)
				FP_Info->Control( this, FCTL_GETPANELINFO, &pi );
			g_manager.getRegKey().set(L"LastHostsMode", pi.ViewMode);
			Log(( "Write HostsMode: %d",pi.ViewMode ));
		}

		return FALSE;
	}

	//Position cursor to added|corrected item on panel
	if ( Event == FE_REDRAW )
	{
		if (!selectFile_.empty())
		{
			Log(( "PositionItem: [%s]", Unicode::toOem(selectFile_).c_str()));

			if ( !pi.PanelItems)
				FP_Info->Control( this, FCTL_GETPANELINFO, &pi );

			for ( n = 0; n < pi.ItemsNumber; n++ ) {
				if ( ShowHosts )
				{
					BOOST_ASSERT(0 && "TODO WTF");
					if ((p=findhost(pi.PanelItems[n].UserData)) == 0)// ||
						//p->regPath_ != selectFile_) TODO
						continue;
				} else
				{
					if(selectFile_ == FTP_FILENAME(&pi.PanelItems[n]))
						continue;
				}

				Log(( "PosItem[%d] [%s]", n, FTP_FILENAME(&pi.PanelItems[n]) ));
				PanelRedrawInfo pri;
				pri.CurrentItem  = n;
				pri.TopPanelItem = pi.TopPanelItem;
				selectFile_		= L"";
				FP_Info->Control(this,FCTL_REDRAWPANEL,&pri);
				break;
			}
			selectFile_ = L"";
		}
	}/*REDRAW*/


	//Correct saved hosts mode on change
	if ( ShowHosts && Event == FE_CHANGEVIEWMODE ) {
		if ( !pi.PanelItems )
			FP_Info->Control( this, FCTL_GETPANELINFO, &pi );

		Log(( "New ColumnMode %d",pi.ViewMode ));
		g_manager.getRegKey().set(L"LastHostsMode", pi.ViewMode);
	}

	//Set start hosts panel mode
	if ( Event == FE_REDRAW ) {
		if ( ShowHosts && !PluginColumnModeSet ) {
			if ( g_manager.opt.PluginColumnMode != -1 )
				FP_Info->Control( this,FCTL_SETVIEWMODE,&g_manager.opt.PluginColumnMode );
			else
			{
				int num = g_manager.getRegKey().get(L"LastHostsMode", 2);
				FP_Info->Control( this,FCTL_SETVIEWMODE,&num );
			}
			PluginColumnModeSet = true;
		} else
			if ( isBackup() && NeedToSetActiveMode ) {
				FP_Info->Control( this,FCTL_SETVIEWMODE,&ActiveColumnMode );
				NeedToSetActiveMode = FALSE;
			}
	}

	//Keep alive
	if ( Event == FE_IDLE          &&
		!ShowHosts                &&
		FtpCmdLineAlive(hConnect) &&
		KeepAlivePeriod           &&
		FP_PeriodEnd(KeepAlivePeriod) ) {
			Log(( "Keep alive" ));
			FTPCmdBlock blocked(this,TRUE);

			if ( g_manager.opt.ShowIdle ) {
				SetLastError( ERROR_SUCCESS );
				IdleMessage( FP_GetMsg(MKeepAliveExec),g_manager.opt.ProcessColor );
			}

			FtpKeepAlive(hConnect);
			Log(( "end Keep alive" ));
	}
	//CMD line
	if ( Event == FE_COMMAND ) {
		Log(( "CMD line" ));
		return ExecCmdLine( (CONSTSTR)Param,FALSE );
	}

	return FALSE;
}
