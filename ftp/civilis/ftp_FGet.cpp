#include "stdafx.h"

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

	if ( SrcFile == INVALID_HANDLE_VALUE )
		return;

	if ( is_flag(OpMode,OPM_DESCR) &&
		(FileSize = GetFileSize(SrcFile,NULL)) != INVALID_FILE_SIZE )
	{
		boost::scoped_array<unsigned char> buf(new unsigned char[FileSize]);
		ReadFile(SrcFile, buf.get(),FileSize,&FileSize,NULL);
		BOOST_ASSERT(0 && "TODO");
		// TODO DWORD WriteSize = hConnect->toOEM( Buf,FileSize );
		DWORD WriteSize = FileSize;
		SetFilePointer(SrcFile, 0, NULL, FILE_BEGIN);
		WriteFile(SrcFile, buf.get(),WriteSize,&WriteSize,NULL );
		SetEndOfFile(SrcFile);
	}
	if(tm)
		SetFileTime(SrcFile,NULL,NULL,tm);
	CloseHandle(SrcFile);
}

int FTP::_FtpGetFile(const std::wstring& remoteFile, const std::wstring& newFile, bool reget, int asciiMode)
{
	/* Local file allways use '\' as separator.
	Regardles of remote settings.
	*/
	std::wstring NewFile = newFile;
	FixLocalSlash(NewFile);

	FtpSetRetryCount(&getConnection(), 0);

	int   rc;
	do{
		SetLastError(ERROR_SUCCESS);

		OperateHidden(NewFile, true);
		if((rc=FtpGetFile(&getConnection(), remoteFile, NewFile, reget, asciiMode)) != false)
		{
			OperateHidden(NewFile, false);
			return rc;
		}

		if(GetLastError() == ERROR_CANCELLED )
		{
			BOOST_LOG(INF, L"GetFileCancelled: op" << IS_SILENT(FP_LastOpMode));
			return IS_SILENT(FP_LastOpMode) ? (-1) : false;
		}

		int num = FtpGetRetryCount(&getConnection());
		if(g_manager.opt.RetryCount > 0 && num >= g_manager.opt.RetryCount)
			return false;

		FtpSetRetryCount(&getConnection(), num+1);

		if(!getConnection().ConnectMessageTimeout(MCannotDownload, remoteFile, MRetry))
			return false;

		reget = true;

		if(getConnection().keepAlive())
			continue;

		SaveUsedDirNFile();
		if(!Connect())
			return false;

	}while(1);
}

int FTP::GetFiles(FARWrappers::ItemList &items, int Move, std::wstring& DestPath,int OpMode)
{
	//Copy received items to internal data and use copy
	// because FAR delete it on next FCTL_GETPANELINFO
	FARWrappers::ItemList FilesList (items.items(), items.size());

	int rc = GetFilesInterface(FilesList, Move, DestPath, OpMode);

	FtpCmdBlock(&getConnection(), FALSE );
	if ( rc == FALSE || rc == -1 )
		LongBeepEnd( TRUE );

	IncludeMask = L"";
	ExcludeMask = L"";

	BOOST_LOG(INF, L"rc:" << rc);
	return rc;
}

int FTP::GetFilesInterface(FARWrappers::ItemList &items, int Move, std::wstring& DestPath,int OpMode)
{
	FTP             *other = OtherPlugin(this);
	FTPCopyInfo      ci;
	FARWrappers::ItemList  il;
	std::wstring		CurName;
	std::wstring     DestName;
	FARWrappers::Screen scr;
	size_t              i,isDestDir;
	FTPFileInfo      FindData;
	DWORD            DestAttr;
	int              mTitle;
	int              rc;

	if(items.size() == 0)
		return FALSE;

	if(!ShowHosts)
		return FALSE;

	BOOST_LOG(INF, L"Dest: " << DestPath);

	ci.DestPath        = DestPath;
	ci.asciiMode       = chost_.AsciiMode;
	ci.ShowProcessList = FALSE;
	ci.AddToQueque     = FALSE;
	ci.MsgCode         = IS_SILENT(OpMode) ? ocOverAll : OverrideMsgCode;
	ci.Download        = TRUE;
	ci.UploadLowCase   = g_manager.opt.UploadLowCase;
	ci.FTPRename       = ci.DestPath.find_first_of(L"/\\") != std::wstring::npos;

	if ( !IS_SILENT(OpMode))
	{
		if ( !CopyAskDialog(Move, true, &ci))
			return false;
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
			if ( ci.DestPath == other->panel_->getCurrentDirectory())
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
			s = getConnection().getCurrentDirectory();
			i = s.size();
			if ( i > 1 ) {
				for( i-=2; i && s[i] != '/' && s[i] != '\\'; i-- );
				s.resize( i+1 );
			}
			ci.DestPath = s;
			isDestDir = TRUE;
		}

		//Create items list
		if(ci.FTPRename)
			il.add(items.items(), items.size());
		else
			if(!ExpandList(items, &il, TRUE, NULL, NULL))
				return false;

		if ( ci.ShowProcessList && !ShowFilesList(&il) )
			return FALSE;

		if ( !ci.FTPRename && ci.AddToQueque ) {
			BOOST_LOG(INF, L"Files added to queue [" << il.size() << L']');
			ListToQueque( &il,&ci );
			return TRUE;
		}

		longBeep_.reset();

		//Calc full size
		getConnection().TrafficInfo->Init(&getConnection(), MStatusDownload, OpMode, &il );

		//Copy\Rename each item
		for ( i = 0; i < il.size(); i++ )
		{
			PluginPanelItem *CurPanelItem;

			CurPanelItem = &il[i];
			CurName      = FTP_FILENAME( CurPanelItem );
			DestAttr     = UINT_MAX;

			//Skip deselected in list
			if(CurPanelItem->Reserved[0] == UINT_MAX) // TODO use another constants
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

				BOOST_LOG(INF, L"Rename [" << CurName << L"] to [" << DestName << L"]");
				//TODO       if ( FtpFindFirstFile( hConnect, DestName, &FindData, NULL) )
				//         if ( FindData.getFileName() == CurName)
				//           DestAttr = FindData.get .dwFileAttributes;
			} else {
				//Copy to local disk
				if ( isDestDir )
					DestName = ci.DestPath + FixFileNameChars(CurName,TRUE);
				else
					DestName = FixFileNameChars(ci.DestPath);

				FixLocalSlash( DestName );

				//Create directory when copy
				if ( is_flag(CurPanelItem->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
					continue;
				//
				//TODO 		if(FRealFile(DestName.c_str(),&FindData.getFindData()))
				// 		{
				// 			DestAttr = GetFileAttributesW(DestName.c_str());
				// 		}
			}

			//Init current
			getConnection().TrafficInfo->InitFile(CurPanelItem, CurName, DestName);

			//Query ovirwrite
			switch( ci.MsgCode ) {
	  case      ocOver:
	  case      ocSkip:
	  case    ocResume: ci.MsgCode = ocNone;
		  break;
			}

			if ( DestAttr != UINT_MAX ) {
				//TODO       ci.MsgCode = AskOverwrite( ci.FTPRename ? MRenameTitle : MDownloadTitle, TRUE,
				//                                  &FindData.getFindData(), &CurPanelItem->FindData, ci.MsgCode );
				//       LastMsgCode = ci.MsgCode;

				switch( ci.MsgCode ) {
		case   ocOverAll:
		case      ocOver: break;
		case      ocSkip:
		case   ocSkipAll: getConnection().TrafficInfo->Skip();
			continue;
		case    ocResume:
		case ocResumeAll: break;

		case    ocCancel: return -1;
				}
			}

			//Reset local attrs
			if ( !ci.FTPRename && DestAttr != UINT_MAX &&
				(DestAttr & (FILE_ATTRIBUTE_READONLY|0/*FILE_ATTRIBUTE_HIDDEN*/)) != 0 )
				SetFileAttributesW( DestName.c_str(), DestAttr & ~(FILE_ATTRIBUTE_READONLY|0/*FILE_ATTRIBUTE_HIDDEN*/) );

			mTitle = MOk;

			//Do rename
			if ( ci.FTPRename ) {
				//Rename
				if ( !FtpRenameFile(&getConnection(), CurName, DestName) )
				{
					FtpConnectMessage(&getConnection(),
						MErrRename,
						(std::wstring(L"\"") + CurName + L"\" to \"" + DestName + L"\"").c_str(),
						-MOk );
					return FALSE;
				} else
				{
					selectFile_ = DestName; // TODO selFile_
					resetCache_ = true;
				}
			} else
				//Do download
				if((rc=_FtpGetFile(CurName, DestName,
					ci.MsgCode == ocResume || ci.MsgCode == ocResumeAll,
					ci.asciiMode )) == TRUE)
				{
					//Process description
					SetupFileTimeNDescription( OpMode, &getConnection(), DestName, &CurPanelItem->FindData.ftLastWriteTime );

					//Delete source after download
					if (Move)
					{
						if(FtpDeleteFile(&getConnection(), CurName)) 
						{
							if ( g_manager.opt.UpdateDescriptions && i < items.size())
								items[i].Flags |= PPIF_PROCESSDESCR;
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
				if (!getConnection().isConnected())
				{
					BackToHosts();
					Invalidate();
				}

				//Return error
				return FALSE;

		}/*EACH FILE*/

		//Remove empty directories after file deletion complete
		if ( Move && !ci.FTPRename )
			for (size_t i=il.size()-1; i >= 0; i--)
			{

				if ( CheckForEsc(FALSE) )
					return -1;

				CurName = FTP_FILENAME(&il[i]);

				if ( is_flag(il[i].FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
					if(FtpRemoveDirectory(&getConnection(), CurName) )
					{
						if ( i < items.size() ) {
							items[i].Flags &= ~PPIF_SELECTED;
							if (g_manager.opt.UpdateDescriptions)
								items[i].Flags |= PPIF_PROCESSDESCR;
						}
					}
			}
			return 1;
}
