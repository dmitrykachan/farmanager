#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

struct DeleteData
{
	bool        DeleteAllFolders;
	bool        SkipAll;
	int         OpMode;
	int         ShowHosts;
	Connection *hConnect;
};

BOOL idDeleteCB(const FTP* ftp, PluginPanelItem &p,LPVOID dt )
{
	PROCP(ftp << L", " << dt << L"[" << FTP_FILENAME(&p) << L"]");
	int      rres;
	const FTPHost* h       = ftp->getHostPanel()->findhost(p.UserData);
	std::wstring		curname = FTP_FILENAME(&p);

	if ( ((DeleteData*)dt)->ShowHosts && !h )
		return TRUE;

	//Ask
	if(is_flag(p.FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ) 
	{
		if ( !((DeleteData*)dt)->DeleteAllFolders &&
			IS_SILENT( ((DeleteData*)dt)->OpMode ) )
		{
			const wchar_t* MsgItems[]=
			{
				((DeleteData*)dt)->ShowHosts ? getMsg(MDeleteHostsTitle):getMsg(MDeleteTitle),
				((DeleteData*)dt)->ShowHosts ? getMsg(MDeleteHostFolder):getMsg(MDeleteFolder),
				curname.c_str(),
				getMsg(MDeleteGroupDelete),
				getMsg(MDeleteGroupAll),
				getMsg(MDeleteGroupCancel)
			};

			rres = FARWrappers::message(MsgItems, 3, FMSG_WARNING|FMSG_DOWN);

			switch(rres) 
			{
				/*ESC*/
			case -1: return FALSE;

				/*Del*/
			case  0: break;

				/*DelAll*/
			case  1: ((DeleteData*)dt)->DeleteAllFolders = TRUE;
				break;

				/*Cancel*/
			case  2: return FALSE;

			}
		}
	}

	//Display
	WinAPI::setConsoleTitle(curname);
	FtpConnectMessage( ((DeleteData*)dt)->hConnect, MDeleteTitle, curname.c_str());

	//Hosts
	if(((DeleteData*)dt)->ShowHosts)
	{
		g_manager.getRegKey().deleteSubKey((std::wstring(HostRegName) + ftp->panel_->getCurrentDirectory() + L'\\' + h->getRegName()).c_str(), true);
		return true;
	}

	//FTP
	if ( is_flag(p.FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ) 
	{
		//FTP directory
		//Succ delete
		if(FtpRemoveDirectory(((DeleteData*)dt)->hConnect, curname)) 
		{
			if (g_manager.opt.UpdateDescriptions) p.Flags |= PPIF_PROCESSDESCR;
			return TRUE;
		}
	} else 
	{
		//FTP file
		//Succ delete
		if(FtpDeleteFile( ((DeleteData*)dt)->hConnect, curname) ) 
		{
			if (g_manager.opt.UpdateDescriptions) p.Flags |= PPIF_PROCESSDESCR;
			return TRUE;
		}
	}

	//Error
	BOOST_LOG(INF, L"Del error: " << ((DeleteData*)dt)->SkipAll ? L"SkipAll" : L"Ask");
	if ( ((DeleteData*)dt)->SkipAll == FALSE ) 
	{
		rres = FtpConnectMessage( ((DeleteData*)dt)->hConnect, MCannotDelete, curname.c_str(), -MCopySkip, MCopySkipAll );
		switch( rres ) 
		{
			/*skip*/     
		case 0: 
			BOOST_LOG(INF, L"Skip");
			return TRUE;

			/*skip all*/ 
		case 1: 
			((DeleteData*)dt)->SkipAll = TRUE;
			BOOST_LOG(INF, L"SkipAll");
			return TRUE;

		default: 
			BOOST_LOG(INF, L"Other");
			SetLastError( ERROR_CANCELLED );
			return FALSE;
		}
	} else
		return TRUE;
}

//---------------------------------------------------------------------------------
int FTP::DeleteFilesINT(FARWrappers::ItemList &panelItems, int OpMode)
{
	PROC;

	if (!ShowHosts) return FALSE;
	if (panelItems.size() == 0 ) return FALSE;

	//Ask
	if ( !IS_SILENT(OpMode) )
	{
		const wchar_t* MsgItems[]=
		{
			getMsg( ShowHosts ? MDeleteHostsTitle : MDeleteTitle ),
			getMsg( ShowHosts ? MDeleteHosts      : MDeleteFiles ),
			getMsg( MDeleteDelete ),
			getMsg( MDeleteCancel )
		};

		int rres = FARWrappers::message(MsgItems, 2);
		if ( rres != 0 ) return TRUE;

		if (panelItems.size() > 1)
		{
			wchar_t buff[128];
			_snwprintf(buff, 127, ShowHosts ? getMsg(MDeleteNumberOfHosts) : getMsg(MDeleteNumberOfFiles), panelItems.size());
			MsgItems[1] = buff;
			rres = FARWrappers::message(MsgItems, 2, FMSG_WARNING|FMSG_DOWN);
			if(rres != 0)
				return true;
		}
	}

	//LIST
	DeleteData data;

	data.DeleteAllFolders = FALSE;
	data.SkipAll          = FALSE;
	data.OpMode           = OpMode;
	data.ShowHosts        = ShowHosts;
	data.hConnect         = &getConnection();

	return ExpandList(panelItems, NULL, TRUE, idDeleteCB, &data);
}

//---------------------------------------------------------------------------------
int FTP::DeleteFiles(FARWrappers::ItemList &panelItems, int OpMode)
{
	PROC;
	FARWrappers::Screen scr;

	int res = DeleteFilesINT(panelItems, OpMode );

	if(res)
	{
		LongBeepEnd( TRUE );
		return TRUE;
	}

	res = GetLastError();
	if ( res == ERROR_CANCELLED )
		return TRUE;

	LongBeepEnd( TRUE );

	//Show FAR error
	if ( ShowHosts )
		return FALSE;

	//Show self error message
	std::wstring nm;
	if(panelItems.size() == 1 )
		nm = FTP_FILENAME(&panelItems[0]);
	else
		nm = boost::lexical_cast<std::wstring>(panelItems.size()) + L"  files/folders";

	FtpConnectMessage(&getConnection(), MCannotDelete, nm.c_str(), -MOk );

	return TRUE;
}
