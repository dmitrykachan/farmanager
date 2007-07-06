#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

FTP *FTP::Backups[ FTP_MAXBACKUPS ] = { 0 };
int  FTP::BackupCount = 0;

FTP::FTP()
	: PluginColumnModeSet(false)
{
	Host.Init();

	hConnect			= NULL;
	ResetCache			= TRUE;
	ShowHosts			= TRUE;
	SwitchingToFTP		= FALSE;
	OverrideMsgCode		= ocNone;
	LastMsgCode			= ocNone;

	UrlsList			= NULL;
	QuequeSize			= 0;

	CallLevel			= 0;

	hostsPath_			= g_manager.getRegKey().get(L"LastHostsPath", L"");

	RereadRequired		= FALSE;
	CurrentState		= fcsNormal;

	KeepAlivePeriod		= g_manager.opt.KeepAlive ? FP_PeriodCreate(g_manager.opt.KeepAlive*1000) : NULL;
	LongBeep			= NULL;

	PanelInfo  pi;
	FP_Info->Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&pi);
	StartViewMode = pi.ViewMode;
	IncludeMask[0] = 0;
	ExcludeMask[0] = 0;
}

FTP::~FTP()
{
  if (hConnect) {
    delete hConnect;
    hConnect = NULL;
  }

  FP_PeriodDestroy(KeepAlivePeriod);
  LongBeepEnd( TRUE );

  FP_Info->Control( this,FCTL_SETVIEWMODE,&StartViewMode );
  DeleteFromBackup();

  ClearQueue();
}

void FTP::Call( void )
  {
    LastUsedPlugin = this;
    if ( CallLevel == 0 )
      LongBeepCreate();

    CallLevel++;
}
void FTP::End( int rc )
  {
    if ( rc != -156 ) {
      Log(( "rc=%d",rc ));
    }
    ShowMemInfo();

    if (!CallLevel) return;

    CallLevel--;

    if ( !CallLevel ) {
      LongBeepEnd();
      if (KeepAlivePeriod)
        FP_PeriodReset(KeepAlivePeriod);
    }
}

CONSTSTR FTP::CloseQuery( void )
{
	if ( UrlsList != NULL )
		return FMSG("Process queque is not empty");

	return NULL;
}

bool FTP::AddWrapper(FP_SizeItemList& il, PluginPanelItem &p,
					 std::string& description, std::string& host, 
					 std::string& directory, std::string& username)
{
	char*	Data[3];
	p.Description               = const_cast<char*>(description.c_str());
	p.CustomColumnNumber        = 3;
	p.CustomColumnData          = Data;
	p.CustomColumnData[0]       = const_cast<char*>(host.c_str());  //C0
	p.CustomColumnData[1]       = const_cast<char*>(directory.c_str());  //C1
	p.CustomColumnData[2]       = const_cast<char*>(username.c_str());  //C2

	if(!il.Add(&p))
		return false;
	return true;
}

int FTP::GetFindData( PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode )
{  
	PROC(("FTP::GetFindData",NULL))
	TIME_TYPE        b,e;
	char            *Data[3];
	FTPFileInfo FileInfo;

	*pPanelItem   = NULL;
	*pItemsNumber = 0;

	//Hosts
	if(ShowHosts) 
	{
		WinAPI::RegKey regkey(g_manager.getRegKey(), (HostRegName + hostsPath_).c_str());
		std::vector<std::wstring> hosts = regkey.getKeys();

		FP_SizeItemList il( FALSE );
		PluginPanelItem tmp;

		if(!IS_SILENT(OpMode)) 
		{
			MemSet(&tmp, 0, sizeof(tmp));
			StrCpy(tmp.FindData.cFileName,"..");
			tmp.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			if ( !IS_SILENT(OpMode) ) 
			{
				tmp.Description               = "..";
				tmp.CustomColumnNumber        = 3;
				tmp.CustomColumnData          = Data;
				tmp.CustomColumnData[0]       = "..";
				tmp.CustomColumnData[1]       = "..";
				tmp.CustomColumnData[2]       = "..";
			}
			if ( !il.Add(&tmp) )
				return FALSE;
		}
	
		hosts_.clear();

		size_t index = 0;
		std::vector<std::wstring>::const_iterator itr = hosts.begin();
		while(itr != hosts.end())
		{
			FTPHost h;
			h.Init();
			h.regKey_.open(regkey, itr->c_str());

			h.LastWrite.dwHighDateTime = 0; // TODO
			h.LastWrite.dwLowDateTime = 0;

			if(!h.Read(*itr))
				continue;
			++index;
			hosts_.insert(std::make_pair(index, h));

			MemSet( &tmp, 0, sizeof(tmp) );
			/* Panel item MUST have name the save as file saved to disk
			in case you want to copy between panels work.
			*/
			Utils::safe_strcpy(tmp.FindData.cFileName, 
				Unicode::toOem(h.MkINIFile(L"", L"")));
			tmp.FindData.ftLastWriteTime  = h.LastWrite;
			tmp.FindData.dwFileAttributes = h.Folder ? FILE_ATTRIBUTE_DIRECTORY : 0;
			tmp.Flags                     = 0; //PPIF_USERDATA;
			tmp.PackSizeHigh              = FTP_HOSTID;
			tmp.UserData                  = index;

			if(!IS_SILENT(OpMode))
			{
				if(!AddWrapper(il, tmp, Unicode::toOem(hosts_[index].hostDescription_),
						Unicode::toOem(h.Host_),
						Unicode::toOem(h.directory_),
						Unicode::toOem(h.username_)))
					return false;
			} else
				if (!il.Add(&tmp))
					return false;
			++itr;
		}
		*pPanelItem   = il.Items();
		*pItemsNumber = static_cast<int>(il.Count());
		return TRUE;
	}

	//FTP
	FP_Screen _scr;

	if ( !hConnect ) {
		goto AskConnect;
	}

Restart:


	if ( !FtpFindFirstFile( hConnect, "*", &FileInfo, &ResetCache ) ) {
		if ( GetLastError() == ERROR_NO_MORE_FILES ) {
			*pItemsNumber = 0;
			return TRUE;
		}

		if ( SwitchingToFTP && GetLastError() == ERROR_CANCELLED ) {
			;
		} else {
			if ( CurrentState == fcsExpandList ) {
				FreeFindData(*pPanelItem,*pItemsNumber);
				*pPanelItem   = NULL;
				*pItemsNumber = 0;
				return FALSE;
			}

			//Query reconnect
			do{
				if ( !hConnect )
					break;

				if ( GetLastError() == ERROR_CANCELLED )
					break;

				if ( !hConnect->ConnectMessageTimeout(MConnectionLost,Host.hostname_,-MRestore ) ) {
					Log(( "WaitMessage cancelled" ));
					break;
				}

				if ( FtpCmdLineAlive(hConnect) &&
					FtpKeepAlive(hConnect) )
					goto Restart;

				if(!selectFile_.empty() && CurrentState != fcsExpandList)
					SaveUsedDirNFile();

AskConnect:
				if ( Connect() )
					goto Restart;
				else
					break;

			}while(1);
		}

		if ( !ShowHosts )
			BackToHosts();

		FreeFindData( *pPanelItem, *pItemsNumber );

		return GetFindData(pPanelItem,pItemsNumber,OpMode);
	}

	GET_TIME(b);
	do{
		if ( g_manager.opt.ShowIdle ) {
			char str[ 200 ];
			GET_TIME( e );
			if ( CMP_TIME(e,b) > 0.5 ) {
				SNprintf( str,sizeof(str),"%s%d", FP_GetMsg(MReaded), *pItemsNumber );
				SetLastError( ERROR_SUCCESS );
				IdleMessage( str,g_manager.opt.ProcessColor );
				b = e;
				if ( CheckForEsc(FALSE) ) {
					SetLastError( ERROR_CANCELLED );
					return FALSE;
				}
			}
		}

		PluginPanelItem *NewPanelItem=*pPanelItem;
		if ( (*pItemsNumber % 1024) == 0 ) {
			if ( !NewPanelItem )
				NewPanelItem = (PluginPanelItem *)_Alloc( (1024+1)*sizeof(PluginPanelItem) );
			else
				NewPanelItem = (PluginPanelItem *)_Realloc( NewPanelItem,(*pItemsNumber+1024+1)*sizeof(PluginPanelItem) );

			if ( NewPanelItem == NULL ) {
				/*-*/Log(("GetFindData(file)::!reallocate plugin panels items %d -> %d",*pItemsNumber,*pItemsNumber+1024+1));
				return FALSE;
			}
			*pPanelItem=NewPanelItem;
		}

		PluginPanelItem *CurItem = &NewPanelItem[*pItemsNumber];
		MemSet( CurItem, 0, sizeof(PluginPanelItem) );

		CurItem->FindData = FileInfo.getFindData();
		if ( !IS_SILENT(OpMode) ) 
		{
			CurItem->CustomColumnNumber             = FTP_COL_MAX;
			CurItem->Owner                          = NULL; // FileInfo.getOwner().c_str(); // TODO can be here a bug?
			CurItem->CustomColumnData               = new char*[FTP_COL_MAX ];
			CurItem->CustomColumnData[FTP_COL_MODE] = NULL; // StrDup(FileInfo.UnixMode);
			CurItem->CustomColumnData[FTP_COL_LINK] = NULL; // StrDup(FileInfo.Link);
//			hConnect->ToOEM(CurItem->CustomColumnData[FTP_COL_LINK]);
		}

		(*pItemsNumber)++;
	}while( FtpFindNextFile(hConnect,&FileInfo) );

	return TRUE;
}

void FTP::FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber)
{
   FP_SizeItemList::Free(PanelItem,ItemsNumber);
}

void FTP::SetBackupMode( void )
  {
    PanelInfo  pi;
    FP_Info->Control( INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi );
    ActiveColumnMode = pi.ViewMode;
}

void FTP::SetActiveMode( void )
  {
     NeedToSetActiveMode = TRUE;
     CurrentState        = fcsNormal;
}

BOOL FTP::isBackup( void )
  {
    for( int n = 0; n < FTP::BackupCount; n++ )
      if ( FTP::Backups[n] == this )
        return TRUE;
 return FALSE;
}

void FTP::DeleteFromBackup( void )
  {
    for( int n = 0; n < FTP::BackupCount; n++ )
      if ( FTP::Backups[n] == this ) {
        memmove( &FTP::Backups[n], &FTP::Backups[n+1], (FTP::BackupCount-n)*sizeof(FTP*) );
        FTP::BackupCount--;
      }
}

void FTP::AddToBackup( void )
  {
     if ( !g_manager.opt.UseBackups )
       return;

     if ( !isBackup() )
       FTP::Backups[ FTP::BackupCount++ ] = this;
}
