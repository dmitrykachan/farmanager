#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

void SetupFileTimeNDescription( int OpMode,Connection *hConnect, std::wstring nm)
{
	if ( !IS_FLAG(OpMode,OPM_DESCR) ) return;

	FILE *SrcFile=_wfopen(nm.c_str(), L"r+b");
	if ( !SrcFile ) return;

	int   FileSize = (int)Fsize(SrcFile);
	BYTE *Buf      = (BYTE*)_Alloc( sizeof(BYTE)*FileSize*3+1 );
	size_t   ReadSize = fread(Buf,1,FileSize,SrcFile);
	size_t WriteSize = ReadSize; // TODO hConnect->FromOEM( Buf,ReadSize,sizeof(BYTE)*FileSize*3+1 );
	fflush(SrcFile);
	fseek(SrcFile,0,SEEK_SET);
	fflush(SrcFile);
	fwrite(Buf,1,WriteSize,SrcFile);
	fflush(SrcFile);
	fclose(SrcFile);
	_Del( Buf );
}

/****************************************
   PROCEDURES
     FTP::GetFiles
 ****************************************/
int FTP::_FtpPutFile(const std::wstring &localFile, const std::wstring &remoteFile, bool Reput, int AsciiMode)
{
	int rc;

	FtpSetRetryCount( hConnect,0 );
	do
	{
		SetLastError( ERROR_SUCCESS );

		if ( !hConnect )
			return FALSE;

		if ( (rc=FtpPutFile( hConnect, localFile, remoteFile, Reput, AsciiMode )) != FALSE )
			return rc;

		if ( GetLastError() == ERROR_CANCELLED ) {
			Log(( "GetFileCancelled: op:%d",IS_SILENT(FP_LastOpMode) ));
			return IS_SILENT(FP_LastOpMode) ? (-1) : FALSE;
		}

		int num = FtpGetRetryCount(hConnect);
		if ( g_manager.opt.RetryCount > 0 && num >= g_manager.opt.RetryCount )
			return FALSE;

		FtpSetRetryCount( hConnect,num+1 );

		if ( !hConnect->ConnectMessageTimeout(MCannotUpload, remoteFile, -MRetry))
			return FALSE;

		Reput = TRUE;

		if ( FtpCmdLineAlive(hConnect) &&
			FtpKeepAlive(hConnect) )
			continue;

		SaveUsedDirNFile();
		if ( !Connect() )
			return FALSE;

	}while( 1 );
}

int FTP::PutFiles( struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode )
  {  PROC(("PutFiles",NULL))
     int rc;

     LogPanelItems( PanelItem,ItemsNumber );

     rc = PutFilesINT(PanelItem,ItemsNumber,Move,OpMode);
     FtpCmdBlock( hConnect, FALSE );

     if ( rc == FALSE || rc == -1 )
       LongBeepEnd( TRUE );

     IncludeMask[0] = 0;
     ExcludeMask[0] = 0;

     Log(( "rc:%d", rc ));
 return rc;
}

int FTP::PutFilesINT( struct PluginPanelItem *PanelItem, size_t ItemsNumber, int Move,int OpMode)
{  
	FP_Screen         _scr;
	FTPCopyInfo       ci;
	FP_SizeItemList   il;
	std::wstring		DestName;
	std::wstring		curName;
	DWORD             DestAttr;
	FTPFileInfo       FindData;
	int               mTitle;
	DWORD             SrcAttr;
	int               rc;

	if (!ItemsNumber) return 0;
	if (ShowHosts)    return PutHostsFiles(PanelItem,ItemsNumber,Move,OpMode);
	if (!hConnect)    return 0;

	ci.DestPath  = FtpGetCurrentDirectory(hConnect);

	ci.asciiMode       = Host.AsciiMode;
	ci.AddToQueque     = FALSE;
	ci.MsgCode         = IS_FLAG(OpMode,OPM_FIND) ? ocOverAll : OverrideMsgCode;
	ci.ShowProcessList = FALSE;
	ci.Download        = FALSE;
	ci.UploadLowCase   = g_manager.opt.UploadLowCase;

	if ( g_manager.opt.ShowUploadDialog &&
		!IS_SILENT(OpMode) && !IS_FLAG(OpMode,OPM_NODIALOG) ) {

			if ( !CopyAskDialog(Move,FALSE,&ci) )
				return FALSE;

			LastMsgCode = ci.MsgCode;
	}

	AddEndSlash( ci.DestPath,'/' );

	if ( !ExpandList(PanelItem,ItemsNumber,&il,FALSE) )
		return FALSE;

	if ( ci.ShowProcessList && !ShowFilesList(&il) )
		return FALSE;

	if ( ci.AddToQueque ) {
		Log(( "Files added to queue [%d]",il.Count() ));
		ListToQueque( &il,&ci );
		return TRUE;
	}

	if ( LongBeep )
		FP_PeriodReset(LongBeep);

	hConnect->TrafficInfo->Init( hConnect, MStatusUpload, OpMode, &il );

	for(size_t I = 0; I < il.Count(); I++ )
	{
		std::wstring tmp;

		SrcAttr = il.List[I].FindData.dwFileAttributes;
		curName = tmp = FTP_FILENAME(&il.List[I]);

		/* File name may contain relative paths.
		Local files use '\' as dir separator, so convert
		it to '/' for remote side, BUT only in case relative paths.
		*/
		if(IsAbsolutePath(curName))
			curName = getName(curName);
		FixFTPSlash(tmp);

		//Skip deselected files
		if(il.List[I].FindData.dwReserved1 == MAX_DWORD)
		{
			continue;
		}

		if ( ci.UploadLowCase && !IS_FLAG(SrcAttr,FILE_ATTRIBUTE_DIRECTORY) )
		{
			std::wstring name = getName(curName);
//			if ( !IsCaseMixed(Name))
//	TODO!!!			LocalLower(Name);
		}

		DestName = ci.DestPath.c_str() + curName;

		if ( IS_FLAG(SrcAttr,FILE_ATTRIBUTE_DIRECTORY) )
		{

			if ( FTPCreateDirectory(curName, OpMode))
				if ( I < ItemsNumber && g_manager.opt.UpdateDescriptions )
					PanelItem[I].Flags|=PPIF_PROCESSDESCR;
			continue;
		}

		DestAttr = MAX_DWORD;
// TODO		MemSet( &FindData.FindData, 0, sizeof(FindData.FindData) );

		__int64 sz;

		//Check file exist
		FtpSetRetryCount( hConnect,0 );

		do{
			SetLastError( ERROR_SUCCESS );

			if ( FtpFindFirstFile( hConnect, Unicode::toOem(DestName).c_str(), &FindData, NULL ) ) {
				if (boost::iequals(
						getName(FindData.getFileName()), 
						getName(DestName)
						)
					)
					DestAttr = FindData.getFindData().dwFileAttributes;
				break;
			} else
				if ( !FtpCmdLineAlive(hConnect) ) {
					;
				} else
					if ( (sz=FtpFileSize(hConnect,DestName.c_str())) != -1 ) {
// TODO						FindData.FindData = il.List[I].FindData;
						FindData.setFileSize(sz);
						DestAttr = 0;
						break;
					} else
						if ( !FtpCmdLineAlive(hConnect) ) {
							;
						} else
							break;

						Log(( "Conn lost!" ));

						if ( !hConnect ) {
							BackToHosts();
							Invalidate();
							return FALSE;
						}

						if ( GetLastError() == ERROR_CANCELLED )
							return IS_SILENT(FP_LastOpMode) ? (-1) : FALSE;

						int num = FtpGetRetryCount(hConnect);
						if ( g_manager.opt.RetryCount > 0 && num >= g_manager.opt.RetryCount )
							return FALSE;

						FtpSetRetryCount( hConnect,num+1 );

						if ( !hConnect->ConnectMessageTimeout( MCannotUpload,DestName.c_str(),-MRetry ) )
							return FALSE;

						SaveUsedDirNFile();
						if ( !Connect() )
							return FALSE;

		}while(1);

		//Init transfer
		hConnect->TrafficInfo->InitFile(&il.List[I], Unicode::utf16ToUtf8(FTP_FILENAME(&il.List[I])).c_str(), Unicode::toOem(DestName).c_str());

		//Ask over
		switch( ci.MsgCode ) {
	  case      ocOver:
	  case      ocSkip:
	  case    ocResume: ci.MsgCode = ocNone;
		  break;
		}
		if ( DestAttr != MAX_DWORD ) {
			ci.MsgCode  = AskOverwrite( MUploadTitle, FALSE, &FindData.getFindData(), &il.List[I].FindData, ci.MsgCode );
			LastMsgCode = ci.MsgCode;
			switch( ci.MsgCode ) {
		case   ocOverAll:
		case      ocOver: break;

		case      ocSkip:
		case   ocSkipAll: hConnect->TrafficInfo->Skip();
			continue;
		case    ocResume:
		case ocResumeAll:
			break;

		case    ocCancel: return -1;
			}
		}

		//Upload
		if ( (rc=_FtpPutFile(FTP_FILENAME(&il.List[I]),
			DestName.c_str(),
			DestAttr == MAX_DWORD ? FALSE : (ci.MsgCode == ocResume || ci.MsgCode == ocResumeAll),
			ci.asciiMode)) == TRUE )
		{
				/*! FAR has a bug, so PanelItem stored in internal structure.
				Because of this flags PPIF_SELECTED and PPIF_PROCESSDESCR has no effect at all.

				if ( I < ItemsNumber ) {
				CLR_FLAG( PanelItem[I].Flags,PPIF_SELECTED );
				if (g_manager.opt.UpdateDescriptions) SET_FLAG( PanelItem[I].Flags,PPIF_PROCESSDESCR );
				}
				*/
				//Process description
				SetupFileTimeNDescription(OpMode, hConnect, curName);
				//Move source
				if (Move)
				{
					SetFileAttributesW(curName.c_str(), 0);
					::_wremove(curName.c_str());
				}

				mTitle = MOk;
		} else
			//Cancelled
			if ( rc == -1 || GetLastError() == ERROR_CANCELLED )
				return rc;
			else {
				//Error uploading
				mTitle = MCannotUpload;
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
	}

	if (Move)
		for (size_t I=il.Count()-1;I>=0;I--)
		{
			if (CheckForEsc(FALSE))
				return -1;

			if ( IS_FLAG(il.List[I].FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
				if ( RemoveDirectoryW( FTP_FILENAME(&il.List[I]).c_str() ) )
					if (I < ItemsNumber )
					{
						PanelItem[I].Flags&=~PPIF_SELECTED;
						if (g_manager.opt.UpdateDescriptions)
							PanelItem[I].Flags|=PPIF_PROCESSDESCR;
					}
		}

		return TRUE;
}
