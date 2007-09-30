#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

Backup FTP::backups_;

FTP::FTP()
	: PluginColumnModeSet(false)
{
	hostPanel_.setPlugin(this);
	FtpFilePanel_.setPlugin(this);

	chost_.Init();

	resetCache_			= true;
	ShowHosts			= TRUE;
	panel_				= &hostPanel_;
	SwitchingToFTP		= FALSE;
	OverrideMsgCode		= ocNone;
	LastMsgCode			= ocNone;

	UrlsList			= NULL;
	QuequeSize			= 0;

	CallLevel			= 0;

	hostPanel_.setCurrentDirectory(g_manager.getRegKey().get(L"LastHostsPath", L""));

	RereadRequired		= FALSE;
	CurrentState		= fcsNormal;

	PanelInfo  pi;
	FARWrappers::getShortPanelInfo(pi);
	startViewMode_		= pi.ViewMode;
	IncludeMask = L"";
	ExcludeMask = L"";
}

FTP::~FTP()
{
	LongBeepEnd(true);

	FARWrappers::getInfo().Control(this, FCTL_SETVIEWMODE, &startViewMode_);
	FTP::backups_.remove(this);

	ClearQueue();
}

void FTP::Call( void )
{
	if(CallLevel == 0)
		LongBeepCreate();

	CallLevel++;
}

void FTP::End( int rc )
{
	BOOST_LOG(DBG, L"::End rc=" << rc);

	if (!CallLevel) return;

	CallLevel--;

	if(!CallLevel)
	{
		LongBeepEnd();
	}
}

const wchar_t* FTP::CloseQuery()
{
	if ( UrlsList != NULL )
		return L"Process queque is not empty";
	return NULL;
}

bool FTP::AddWrapper(FARWrappers::ItemList& il, PluginPanelItem &p,
					 std::wstring& description, std::wstring& host, 
					 std::wstring& directory, std::wstring& username)
{
	return true;
}

int FTP::GetFindData( PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode )
{  
	PROC;
	FTPFileInfo FileInfo;

	*pPanelItem   = NULL;
	*pItemsNumber = 0;


	//FTP
	FARWrappers::Screen scr;

	if(!getConnection().isConnected() && !Connect())
		return false;

Restart:

	if(!FtpFindFirstFile(&getConnection(), L"*", &FileInfo, &resetCache_))
	{
		if ( GetLastError() == ERROR_NO_MORE_FILES )
		{
			*pItemsNumber = 0;
			return TRUE;
		}

		if (!SwitchingToFTP || GetLastError() != ERROR_CANCELLED && CurrentState == fcsExpandList)
		{
			// TODO FreeFindData(*pPanelItem,*pItemsNumber);
			*pPanelItem   = NULL;
			*pItemsNumber = 0;
			return FALSE;
		}

			//Query reconnect
		do
		{
			if ( GetLastError() == ERROR_CANCELLED )
				break;

			if(!getConnection().ConnectMessageTimeout(MConnectionLost, chost_.url_.fullhostname_, MRestore ))
			{
				BOOST_LOG(INF, L"WaitMessage canceled");
				break;
			}

			if(getConnection().keepAlive())
				goto Restart;

			if(!selectFile_.empty() && CurrentState != fcsExpandList) // TODO selFile
				SaveUsedDirNFile();

			if ( Connect() )
				goto Restart;
			else
				break;

		}while(1);

		if ( !ShowHosts )
			BackToHosts();

		// TODO FreeFindData( *pPanelItem, *pItemsNumber );

		return GetFindData(pPanelItem,pItemsNumber,OpMode);
	}

	return true;
}

void FTP::SetBackupMode( void )
{
    PanelInfo  pi;
    FARWrappers::getInfo().Control( INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi );
    ActiveColumnMode = pi.ViewMode;
}

void FTP::SetActiveMode( void )
{
     NeedToSetActiveMode = TRUE;
     CurrentState        = fcsNormal;
}

void Backup::add(FTP* ftp)
{
	if(!g_manager.opt.UseBackups)
		return;

	if(!find(ftp))
		array_.push_back(ftp);
}

bool Backup::find(FTP* ftp)
{
	return std::find(array_.begin(), array_.end(), ftp) != array_.end();
}

void Backup::remove(FTP* ftp)
{
	std::vector<FTP*>::iterator new_end = std::remove(array_.begin(), array_.end(), ftp);
	array_.erase(new_end, array_.end());
}

void Backup::eraseAll()
{
	std::vector<FTP*>::iterator itr = array_.begin();
	while(itr != array_.end())
	{
		delete *itr;
		++itr;
	}
}
