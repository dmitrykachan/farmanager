#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

void SetupFileTimeNDescription( int OpMode,Connection *hConnect, std::wstring nm)
{
	if ( !is_flag(OpMode,OPM_DESCR) ) return;

	FILE *SrcFile=_wfopen(nm.c_str(), L"r+b");
	if ( !SrcFile ) return;

	int   FileSize = (int)Fsize(SrcFile);
	boost::scoped_array<BYTE> Buf(new BYTE[FileSize*3+1]);
	size_t   ReadSize = fread(Buf.get(), 1,FileSize,SrcFile);
	size_t WriteSize = ReadSize; // TODO hConnect->FromOEM( Buf,ReadSize,sizeof(BYTE)*FileSize*3+1 );
	fflush(SrcFile);
	fseek(SrcFile,0,SEEK_SET);
	fflush(SrcFile);
	fwrite(Buf.get(),1,WriteSize,SrcFile);
	fflush(SrcFile);
	fclose(SrcFile);
}

/****************************************
   PROCEDURES
     FTP::GetFiles
 ****************************************/
int FTP::_FtpPutFile(const std::wstring &localFile, const std::wstring &remoteFile, bool Reput, int AsciiMode)
{
	int rc;

	FtpSetRetryCount(&getConnection(), 0 );
	do
	{
		SetLastError( ERROR_SUCCESS );

		if ( (rc=FtpPutFile(&getConnection(), localFile, remoteFile, Reput, AsciiMode )) != FALSE )
			return rc;

		if ( GetLastError() == ERROR_CANCELLED ) {
			BOOST_LOG(INF, L"GetFileCancelled: op:" << IS_SILENT(FP_LastOpMode));
			return IS_SILENT(FP_LastOpMode) ? (-1) : FALSE;
		}

		int num = FtpGetRetryCount(&getConnection());
		if ( g_manager.opt.RetryCount > 0 && num >= g_manager.opt.RetryCount )
			return FALSE;

		FtpSetRetryCount(&getConnection(), num+1 );

		if (!getConnection().ConnectMessageTimeout(MCannotUpload, remoteFile, MRetry))
			return FALSE;

		Reput = TRUE;

		if(getConnection().keepAlive())
			continue;

		SaveUsedDirNFile();
		if ( !Connect() )
			return FALSE;

	}while( 1 );
}

int FTP::PutFiles( struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode )
{
	PROC;
	int rc;

	rc = PutFilesINT(PanelItem,ItemsNumber,Move,OpMode);
	FtpCmdBlock(&getConnection(), FALSE );

	if ( rc == FALSE || rc == -1 )
		LongBeepEnd( TRUE );

	IncludeMask = L"";
	ExcludeMask = L"";

	BOOST_LOG(INF, L"rc" << rc);
	return rc;
}

int FTP::PutFilesINT( struct PluginPanelItem *PanelItem, size_t ItemsNumber, int Move,int OpMode)
{  
	FARWrappers::Screen scr;
	FTPCopyInfo       ci;
	FARWrappers::ItemList   il;
	std::wstring		DestName;
	std::wstring		curName;
	DWORD             DestAttr;
	FTPFileInfo       FindData;
	int               mTitle;
	DWORD             SrcAttr;
	int               rc;

	if (!ItemsNumber) return 0;

	ci.DestPath  = getConnection().getCurrentDirectory();

	ci.asciiMode       = chost_.AsciiMode;
	ci.AddToQueque     = FALSE;
	ci.MsgCode         = is_flag(OpMode,OPM_FIND) ? ocOverAll : OverrideMsgCode;
	ci.ShowProcessList = FALSE;
	ci.Download        = FALSE;
	ci.UploadLowCase   = g_manager.opt.UploadLowCase;

	if ( g_manager.opt.ShowUploadDialog &&
		!IS_SILENT(OpMode))
	{

			if ( !CopyAskDialog(Move, false,&ci) )
				return FALSE;

			LastMsgCode = ci.MsgCode;
	}

	AddEndSlash( ci.DestPath,'/' );

	BOOST_ASSERT(0 && "TODO");
//	if ( !ExpandList(PanelItem,ItemsNumber,&il,FALSE) )
//		return FALSE;

	if ( ci.ShowProcessList && !ShowFilesList(&il) )
		return FALSE;

	if ( ci.AddToQueque ) {
		BOOST_LOG(INF, L"Files added to queue " << il.size());
		ListToQueque( &il,&ci );
		return TRUE;
	}

	longBeep_.reset();

	getConnection().TrafficInfo->Init(&getConnection(), MStatusUpload, OpMode, &il );

	for(size_t I = 0; I < il.size(); I++ )
	{
		std::wstring tmp;

		SrcAttr = il[I].FindData.dwFileAttributes;
		curName = tmp = FTP_FILENAME(&il[I]);

		/* File name may contain relative paths.
		Local files use '\' as dir separator, so convert
		it to '/' for remote side, BUT only in case relative paths.
		*/
		if(IsAbsolutePath(curName))
			curName = getName(curName);
		FixFTPSlash(tmp);

		//Skip deselected files
		if(il[I].Reserved[0] == UINT_MAX)
		{
			continue;
		}

		if ( ci.UploadLowCase && !is_flag(SrcAttr,FILE_ATTRIBUTE_DIRECTORY) )
		{
			std::wstring name = getName(curName);
//			if ( !IsCaseMixed(Name))
//	TODO!!!			LocalLower(Name);
		}

		DestName = ci.DestPath.c_str() + curName;

		if ( is_flag(SrcAttr,FILE_ATTRIBUTE_DIRECTORY) )
		{

//TODO			if ( FTPCreateDirectory(curName, OpMode))
				if ( I < ItemsNumber && g_manager.opt.UpdateDescriptions )
					PanelItem[I].Flags|=PPIF_PROCESSDESCR;
			continue;
		}

		DestAttr = UINT_MAX;
// TODO		MemSet( &FindData.FindData, 0, sizeof(FindData.FindData) );

		__int64 sz;

		//Check file exist
		FtpSetRetryCount(&getConnection(), 0);

		do{
			SetLastError( ERROR_SUCCESS );

			if ( FtpFindFirstFile(&getConnection(), DestName, &FindData, NULL ) )
			{
				if (boost::iequals(
						getName(FindData.getFileName()), 
						getName(DestName)
						)
					)
				DestAttr = FindData.getWindowsFileAttribute();
				break;
			} else
				if(getConnection().cmdLineAlive())
				{
					if ( (sz=FtpFileSize(&getConnection(),DestName.c_str())) != -1 ) {
// TODO						FindData.FindData = il.List[I].FindData;
						FindData.setFileSize(sz);
						DestAttr = 0;
						break;
					} else
						if ( !getConnection().cmdLineAlive() )
						{
							;
						} else
							break;

						BOOST_LOG(INF, L"Conn lost!");

						if (!getConnection().isConnected())
						{
							BackToHosts();
							Invalidate();
							return FALSE;
						}

						if ( GetLastError() == ERROR_CANCELLED )
							return IS_SILENT(FP_LastOpMode) ? (-1) : FALSE;

						int num = FtpGetRetryCount(&getConnection());
						if ( g_manager.opt.RetryCount > 0 && num >= g_manager.opt.RetryCount )
							return FALSE;

						FtpSetRetryCount(&getConnection(), num+1);

						if (!getConnection().ConnectMessageTimeout( MCannotUpload,DestName.c_str(), MRetry ) )
							return FALSE;

						SaveUsedDirNFile();
						if ( !Connect() )
							return FALSE;
				}

		}while(1);

		//Init transfer
		getConnection().TrafficInfo->InitFile(&il[I], FTP_FILENAME(&il[I]), DestName);

		//Ask over
		switch( ci.MsgCode ) {
	  case      ocOver:
	  case      ocSkip:
	  case    ocResume: ci.MsgCode = ocNone;
		  break;
		}
		if ( DestAttr != UINT_MAX ) {
// TODO			ci.MsgCode  = AskOverwrite( MUploadTitle, FALSE, &FindData.getFindData(), &il[I].FindData, ci.MsgCode );
// 			LastMsgCode = ci.MsgCode;
			switch( ci.MsgCode ) {
		case   ocOverAll:
		case      ocOver: break;

		case      ocSkip:
		case   ocSkipAll: getConnection().TrafficInfo->Skip();
			continue;
		case    ocResume:
		case ocResumeAll:
			break;

		case    ocCancel: return -1;
			}
		}

		//Upload
		if ( (rc=_FtpPutFile(FTP_FILENAME(&il[I]),
			DestName.c_str(),
			DestAttr == UINT_MAX ? FALSE : (ci.MsgCode == ocResume || ci.MsgCode == ocResumeAll),
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
				SetupFileTimeNDescription(OpMode, &getConnection(), curName);
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
			if ( !getConnection().isConnected()) {
				BackToHosts();
				Invalidate();
			}
			//Return error
			return FALSE;
	}

	if (Move)
		for (size_t I=il.size()-1;I>=0;I--)
		{
			if (CheckForEsc(FALSE))
				return -1;

			if ( is_flag(il[I].FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
				if ( RemoveDirectoryW( FTP_FILENAME(&il[I]).c_str() ) )
					if (I < ItemsNumber )
					{
						PanelItem[I].Flags&=~PPIF_SELECTED;
						if (g_manager.opt.UpdateDescriptions)
							PanelItem[I].Flags|=PPIF_PROCESSDESCR;
					}
		}

		return TRUE;
}
