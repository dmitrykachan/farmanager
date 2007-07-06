#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

/****************************************
   PROCEDURES
     FTP::GetFiles
 ****************************************/
void SetupFileTimeNDescription(int OpMode,Connection *hConnect, const std::wstring &nm,FILETIME *tm)
{  
	HANDLE SrcFile = CreateFileW(nm.c_str(), GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL );
	DWORD  FileSize;
	BYTE  *Buf;

	if ( SrcFile == INVALID_HANDLE_VALUE )
		return;

	if ( IS_FLAG(OpMode,OPM_DESCR) &&
		(FileSize=GetFileSize(SrcFile,NULL)) != 0xFFFFFFFFUL ) {
			Buf      = (BYTE*)_Alloc( sizeof(BYTE)*FileSize );
			ReadFile(SrcFile,Buf,FileSize,&FileSize,NULL);
			// TODO      DWORD WriteSize = hConnect->toOEM( Buf,FileSize );
			DWORD WriteSize = FileSize;
			SetFilePointer( SrcFile,0,NULL,FILE_BEGIN );
			WriteFile( SrcFile,Buf,WriteSize,&WriteSize,NULL );
			SetEndOfFile( SrcFile );
			_Del( Buf );
	}
	if ( tm )
		SetFileTime(SrcFile,NULL,NULL,tm);
	CloseHandle(SrcFile);
}

int FTP::_FtpGetFile( CONSTSTR lpszRemoteFile,CONSTSTR lpszNewFile,BOOL Reget,int AsciiMode )
  {
    /* Local file allways use '\' as separator.
       Regardles of remote settings.
    */
    FixLocalSlash( (char*)lpszNewFile ); //Hack it to (char*).

    FtpSetRetryCount( hConnect,0 );

    int   rc;
    do{
      SetLastError( ERROR_SUCCESS );

      if ( !hConnect )
        return FALSE;

      OperateHidden( lpszNewFile, TRUE );
      if ( (rc=FtpGetFile( hConnect,lpszRemoteFile,lpszNewFile,Reget,AsciiMode )) != FALSE ) {
        OperateHidden( lpszNewFile, FALSE );
        return rc;
      }

      if ( GetLastError() == ERROR_CANCELLED ) {
        Log(( "GetFileCancelled: op:%d",IS_SILENT(FP_LastOpMode) ));
        return IS_SILENT(FP_LastOpMode) ? (-1) : FALSE;
      }

      int num = FtpGetRetryCount(hConnect);
      if ( g_manager.opt.RetryCount > 0 && num >= g_manager.opt.RetryCount )
        return FALSE;

      FtpSetRetryCount( hConnect,num+1 );

	  if(!hConnect->ConnectMessageTimeout(MCannotDownload, Unicode::fromOem(lpszRemoteFile), -MRetry)) // TODO
        return FALSE;

      Reget = TRUE;

      if ( FtpCmdLineAlive(hConnect) &&
           FtpKeepAlive(hConnect) )
        continue;

      SaveUsedDirNFile();
      if ( !Connect() )
        return FALSE;

    }while( 1 );
}

int FTP::GetFiles( struct PluginPanelItem *PanelItem,int ItemsNumber,int Move, std::wstring& DestPath,int OpMode )
  {  PROC(("FTP::GetFiles","cn:%d, %s [%s] op:%08X",ItemsNumber, Move ? "MOVE" : "COPY", DestPath, OpMode ))

     LogPanelItems( PanelItem,ItemsNumber );

     //Copy received items to internal data and use copy
     // because FAR delete it on next FCTL_GETPANELINFO
     FP_SizeItemList FilesList;

     FilesList.Add( PanelItem,ItemsNumber );

     int rc = GetFilesInterface( FilesList.Items(), FilesList.Count(),
                                 Move, DestPath, OpMode);

     FtpCmdBlock( hConnect, FALSE );
     if ( rc == FALSE || rc == -1 )
       LongBeepEnd( TRUE );

     IncludeMask[0] = 0;
     ExcludeMask[0] = 0;

     Log(( "rc:%d", rc ));
 return rc;
}

int FTP::GetFilesInterface( struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move, std::wstring& DestPath,int OpMode)
{
	FTP             *other = OtherPlugin(this);
     FTPCopyInfo      ci;
     FP_SizeItemList  il;
	 std::wstring		CurName;
	 std::wstring     DestName;
     FP_Screen        _scr;
     size_t              i,isDestDir;
     FTPFileInfo      FindData;
     DWORD            DestAttr;
     int              mTitle;
     int              rc;

  if ( !ItemsNumber )
    return FALSE;

  //Hosts
  if ( ShowHosts )
    return GetHostFiles(PanelItem,ItemsNumber,Move,DestPath,OpMode);

  if ( !ShowHosts && !hConnect )
    return FALSE;

  Log(( "Dest: %s", DestPath.c_str() ));

  ci.DestPath        = DestPath;
  ci.asciiMode       = Host.AsciiMode;
  ci.ShowProcessList = FALSE;
  ci.AddToQueque     = FALSE;
  ci.MsgCode         = IS_SILENT(OpMode) ? ocOverAll : OverrideMsgCode;
  ci.Download        = TRUE;
  ci.UploadLowCase   = g_manager.opt.UploadLowCase;
  ci.FTPRename       = ci.DestPath.find_first_of(L"/\\") != std::wstring::npos;

  if ( !IS_SILENT(OpMode) && !IS_FLAG(OpMode,OPM_NODIALOG) ) {
    if ( !CopyAskDialog(Move,TRUE,&ci) )
      return FALSE;
    LastMsgCode = ci.MsgCode;
  }

  i = ci.DestPath.size()-1;

 //Rename only if move
  if ( !Move && ci.FTPRename )
    ci.FTPRename = FALSE;

 //Unquote
  if ( ci.DestPath[0] == '\"' && ci.DestPath[i] == '\"' ) {
    i -= 2;
	ci.DestPath.erase(i+1, 1 );
    ci.DestPath.erase(0, 1 );
  }

 //Remove prefix
  if ( ci.DestPath == L"ftp:") {
    i -= 4;
    ci.DestPath.erase(0, 4);
  }

  do {
   //Copy - allways on local disk
    if ( !Move ) {
      isDestDir = TRUE;
      AddEndSlash( ci.DestPath,'\\' );
      break;
    }

   //Rename of server
    if ( ci.FTPRename ) {
      isDestDir = ci.DestPath[i] == '/';
      break;
    }

   //Move to the other panel
	if ( Move && other )
	{
		if ( ci.DestPath == other->GetCurPath())
		{
			isDestDir    = TRUE;
			ci.FTPRename = TRUE;
			break;
		}
	}

   //Single name
	if (ci.DestPath.find_first_of(L"/\\") == std::wstring::npos) 
	{
      isDestDir    = FALSE;
      ci.FTPRename = TRUE;
      break;
    }

   //Path on server
	if ( ci.DestPath.find_first_of(L'\\') != std::wstring::npos && ci.DestPath[i] == L'/' ) {
      isDestDir    = TRUE;
      ci.FTPRename = TRUE;
      break;
    }

   //Path on local disk
    isDestDir = TRUE;
    AddEndSlash( ci.DestPath,'\\' );
  }while(0);

 //Check for move to parent folder
  if ( ci.FTPRename )
    if ( ci.DestPath == L".." ) 
	{
		std::wstring s;
		s = FtpGetCurrentDirectory(hConnect);
      i = s.size();
      if ( i > 1 ) {
        for( i-=2; i && s[i] != '/' && s[i] != '\\'; i-- );
        s.resize( i+1 );
      }
      ci.DestPath = s;
      isDestDir = TRUE;
    }

//Create items list
  if (ci.FTPRename)
    il.Add( PanelItem, ItemsNumber );
   else
  if ( !ExpandList(PanelItem,ItemsNumber,&il,TRUE,NULL,NULL) )
    return FALSE;

  if ( ci.ShowProcessList && !ShowFilesList(&il) )
    return FALSE;

  if ( !ci.FTPRename && ci.AddToQueque ) {
    Log(( "Files added to queue [%d]",il.Count() ));
    ListToQueque( &il,&ci );
    return TRUE;
  }

  if ( LongBeep )
    FP_PeriodReset( LongBeep );

//Calc full size
  hConnect->TrafficInfo->Init( hConnect, MStatusDownload, OpMode, &il );

//Copy\Rename each item
  for ( i = 0; i < il.Count(); i++ ) {
    PluginPanelItem *CurPanelItem;

    CurPanelItem = &il.List[i];
    CurName      = FTP_FILENAME( CurPanelItem );
    DestAttr     = MAX_DWORD;

    //Skip deselected in list
    if ( CurPanelItem->FindData.dwReserved1 == MAX_DWORD )
      continue;

    //Rename on ftp
    if ( ci.FTPRename ) 
	{
      if ( isDestDir ) 
	  {
		DestName = ci.DestPath;
        AddEndSlash( DestName, '/' );
        DestName += CurName;
      } else
        DestName = ci.DestPath;

      Log(( "Rename [%s] to [%s]",CurName,DestName.c_str() ));
      if ( FtpFindFirstFile( hConnect, Unicode::utf16ToUtf8(DestName).c_str(), &FindData, NULL) )
        if ( FindData.getFileName() == CurName)
          DestAttr = FindData.getFindData().dwFileAttributes;
    } else {
    //Copy to local disk
      if ( isDestDir )
        DestName = ci.DestPath + FixFileNameChars(CurName,TRUE);
       else
        DestName = FixFileNameChars(ci.DestPath);

      FixLocalSlash( DestName );

    //Create directory when copy
      if ( IS_FLAG(CurPanelItem->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
        continue;
    //
	  if ( FRealFile(Unicode::utf16ToUtf8(DestName).c_str(),&FindData.getFindData()) ) {
        Log(( "Real file: [%s]",DestName.c_str() ));
        DestAttr = GetFileAttributesW(DestName.c_str());
      }
    }

    //Init current
	hConnect->TrafficInfo->InitFile( CurPanelItem, Unicode::utf16ToUtf8(CurName).c_str(), Unicode::utf16ToUtf8(DestName).c_str() );

    //Query ovirwrite
    switch( ci.MsgCode ) {
      case      ocOver:
      case      ocSkip:
      case    ocResume: ci.MsgCode = ocNone;
                     break;
    }

    if ( DestAttr != MAX_DWORD ) {
      ci.MsgCode = AskOverwrite( ci.FTPRename ? MRenameTitle : MDownloadTitle, TRUE,
                                 &FindData.getFindData(), &CurPanelItem->FindData, ci.MsgCode );
      LastMsgCode = ci.MsgCode;

      switch( ci.MsgCode ) {
        case   ocOverAll:
        case      ocOver: break;
        case      ocSkip:
        case   ocSkipAll: hConnect->TrafficInfo->Skip();
                          continue;
        case    ocResume:
        case ocResumeAll: break;

        case    ocCancel: return -1;
      }
    }

   //Reset local attrs
   if ( !ci.FTPRename && DestAttr != MAX_DWORD &&
        (DestAttr & (FILE_ATTRIBUTE_READONLY|0/*FILE_ATTRIBUTE_HIDDEN*/)) != 0 )
     SetFileAttributesW( DestName.c_str(), DestAttr & ~(FILE_ATTRIBUTE_READONLY|0/*FILE_ATTRIBUTE_HIDDEN*/) );

   mTitle = MOk;

//Do rename
    if ( ci.FTPRename ) {
      //Rename
      if ( !FtpRenameFile(hConnect, CurName, DestName) )
	  {
        FtpConnectMessage( hConnect,
                           MErrRename,
                           Message("\"%s\" to \"%s\"",CurName,DestName.c_str()),
                           -MOk );
        return FALSE;
      } else {
        selectFile_ = DestName;
        ResetCache = TRUE;
      }
    } else
//Do download
    if ( (rc=_FtpGetFile( Unicode::utf16ToUtf8(CurName).c_str(), Unicode::utf16ToUtf8(DestName).c_str(),
                          ci.MsgCode == ocResume || ci.MsgCode == ocResumeAll,
                          ci.asciiMode )) == TRUE ) {
/*! FAR has a bug, so PanelItem stored in internal structure.
    Because of this flags PPIF_SELECTED and PPIF_PROCESSDESCR has no effect at all.

      if ( i < ItemsNumber ) {
        CLR_FLAG( PanelItem[i].Flags,PPIF_SELECTED );
        if (g_manager.opt.UpdateDescriptions)
          SET_FLAG( PanelItem[i].Flags,PPIF_PROCESSDESCR );
      }
*/
    //Process description
      SetupFileTimeNDescription( OpMode,hConnect, DestName, &CurPanelItem->FindData.ftLastWriteTime );

    //Delete source after download
      if (Move) {
        if ( FtpDeleteFile(hConnect,Unicode::utf16ToUtf8(CurName).c_str()) ) 
		{
          if ( g_manager.opt.UpdateDescriptions && i < ItemsNumber )
            PanelItem[i].Flags |= PPIF_PROCESSDESCR;
        } else
          mTitle = MCannotDelete;
      }
    } else {
//Error downloading
      //Cancelled
      if ( rc == -1 || GetLastError() == ERROR_CANCELLED )
        return rc;
       else
      //Other error
        mTitle = MCannotDownload;
    }

//Process current file finished
   //All OK
    if ( mTitle == MOk || mTitle == MNone__ )
      continue;

    //Connection lost
    if ( !hConnect ) {
      BackToHosts();
      Invalidate();
    }

    //Return error
    return FALSE;

  }/*EACH FILE*/

//Remove empty directories after file deletion complete
  if ( Move && !ci.FTPRename )
    for (size_t i=il.Count()-1;i>=0;i--) {

      if ( CheckForEsc(FALSE) )
        return -1;

      CurName = FTP_FILENAME( &il.List[i] );

      if ( IS_FLAG(il.List[i].FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
        if ( FtpRemoveDirectory(hConnect,Unicode::utf16ToUtf8(CurName).c_str()) ) {
          if ( i < ItemsNumber ) {
            PanelItem[i].Flags &= ~PPIF_SELECTED;
            if (g_manager.opt.UpdateDescriptions)
              PanelItem[i].Flags |= PPIF_PROCESSDESCR;
          }
        }
    }
  return 1;
}
