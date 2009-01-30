#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "farwrapper/dialog.h"
#include "farwrapper/menu.h"

static int AskDeleteQueue()
{
	const wchar_t* MsgItems[] =
	{
		getMsg(MAttention),
		getMsg(MQDeleteItem),
		getMsg(MQSingleItem), getMsg(MQEntireList), getMsg(MCancel)
	};

	return FARWrappers::message(MsgItems, 3, FMSG_WARNING);
}

void FTP::ClearQueue()
{
	urlsQueue_.clear();
}

bool FTP::WarnExecuteQueue(QueueExecOptions* op)
{
	FARWrappers::Dialog dlg(L"FTPProcessQueue");

	int saveAsDefault = 0;
	dlg.addDoublebox( 3, 1, 72, 9,					getMsg(MQueueParam))->
		addCheckbox	( 5, 3,		&op->RestoreState,	getMsg(MQRestore))->
		addCheckbox	( 5, 4,		&op->RemoveCompleted,getMsg(MQRemove))->
		addCheckbox	( 5, 6,		&saveAsDefault,		getMsg(MQSave))->
		addHLine	( 3, 7)->
		addDefaultButton(0,8,	1,					getMsg(MOk), DIF_CENTERGROUP)->
		addDefaultButton(0,8,	-1,					getMsg(MCancel), DIF_CENTERGROUP);
	if(dlg.show(76,11) == -1)
		return false;

	//Save to default
	if(saveAsDefault)
	{
		g_manager.opt.RestoreState    = op->RestoreState;
		g_manager.opt.RemoveCompleted = op->RemoveCompleted;
		g_manager.getRegKey().set(L"QueueRestoreState", g_manager.opt.RestoreState);
		g_manager.getRegKey().set(L"QueueRemoveCompleted", g_manager.opt.RemoveCompleted);
	}

	return true;
}

void FTP::SetupQOpt(QueueExecOptions* op)
{
	op->RestoreState    = g_manager.opt.RestoreState;
	op->RemoveCompleted = g_manager.opt.RemoveCompleted;
}

const wchar_t* FTP::InsertSelectedToQueue(bool download)
{
	return FtpFilePanel_.InsertSelectedToQueue(download);
}


void FTP::InsertToQueue( void )
{
	static const wchar_t* strings[] = 
	{
		getMsg(MQISingle),
		getMsg(MQIFTP),
		getMsg(MQIAnother),
	};
	FARWrappers::Menu menu(strings, false, FMENU_WRAPMODE);
	menu.setTitle(getMsg(MQAddTitle));
	menu.setHelpTopic(L"FTPQueueAddItem");

	int      sel;
	FTPUrlPtr tmp = FTPUrlPtr(new FTPUrl);

	do
	{
		sel = menu.show();

		if(sel == -1)
			return;

		switch( sel )
		{
		case 0: 
			UrlInit(tmp);
			if(EditUrlItem(tmp))
			{
				AddToQueque(tmp);
				return;
			}
			break;
		case 1:
		case 2:
			{
				const wchar_t* err = InsertSelectedToQueue(sel == 1);
				if(err == NULL)
					return;
				const wchar_t* itms[] = { getMsg(MQErrAdding), err, getMsg(MOk) };
				FARWrappers::message(itms, 1, FMSG_WARNING);
				break;
			}
		}
	} while(1);
}

void FTP::QuequeMenu()
{
	QueueExecOptions exOp;
	SetupQOpt(&exOp);
	int selected = -1;
	do 
	{
		FARWrappers::Menu menu(FMENU_SHOWAMPERSAND);
		menu.reserve(urlsQueue_.size());
		std::wstring line;
		for(std::vector<FTPUrlPtr>::const_iterator itr = urlsQueue_.begin(); itr != urlsQueue_.end(); itr++)
		{
			line = (*itr)->Download? L"-> " : L"<- ";
			line.append((*itr)->SrcPath, 0, 20);
			line.resize(3+20, L' ');
			line += FAR_VERT_CHAR;

			line.append((*itr)->DestPath, 0, 20);
			line.resize(3+41, L' ');
			line += FAR_VERT_CHAR;

			line.append((*itr)->fileName_, 0, 20);
			line.resize(3+62, L' ');

			/*
			if ( p->Error[0] )
			mi[n].Checked = TRUE;
			*/
			menu.addItem(line);
		}
		line = getMsg(MQMenuTitle);
		line += L": ";
		line += boost::lexical_cast<std::wstring>(urlsQueue_.size());
		line += L' ';
		line += getMsg(MQMenuItems);

		menu.setTitle(line);
		menu.setBottom(getMsg(MQMenuFooter));
		menu.setHelpTopic(L"FTPQueue");

		const static int breaks[] = {VK_DELETE, VK_INSERT, VK_F4, VK_RETURN, 0};
		menu.setBreakKeys(breaks);

		if(selected != -1 && selected < static_cast<int>(menu.size()))
			menu.select(selected);

		selected = menu.show();
		switch(menu.getBreakIndex())
		{
		case -1:
			return;
		case 0:
			if(!urlsQueue_.empty())
			{
				
				switch(AskDeleteQueue())
				{
				case -1:
				case  2:
					break;
				case 0:
					if(selected != -1)
						DeleteUrlItem(selected);
					break;
				case 1:
					urlsQueue_.clear();
				    break;
				default:
					BOOST_ASSERT("incorrect case");
				}
			}
			break;
		case 1:
			InsertToQueue();
			break;
		case 2:
		{
			if(selected != -1)
			{
				FTPUrlPtr p = UrlItem(selected);
				EditUrlItem(p);
			}
			break;
		}
		case 3:
			if(!urlsQueue_.empty() && WarnExecuteQueue(&exOp))
			{
				ExecuteQueue(&exOp);
				if(urlsQueue_.empty())
					return;
			}
			break;
		}
	} while(true);
}

void FTP::AddToQueque(FTPUrlPtr &pUrl)
{
	urlsQueue_.push_back(pUrl);
}

void FTP::AddToQueque(FTPFileInfo& file, const std::wstring& path, bool Download)
{
	FTPUrlPtr p = FTPUrlPtr(new FTPUrl);

	p->pHost_    = getConnection().getHost();
	p->Download	 = Download;
	p->DestPath  = path;

	std::wstring branch;
	splitFilename(file.getFileName(), branch, p->fileName_, !Download);
	if(!branch.empty())
	{
		AddEndSlash(p->DestPath, Download? LOC_SLASH : NET_SLASH);
		p->DestPath += branch;
		AddEndSlash(p->DestPath, Download? LOC_SLASH : NET_SLASH);
	}
	std::wstring srcpath;
	if(Download)
	{
		srcpath = getConnection().getCurrentDirectory();
		AddEndSlash(srcpath, NET_SLASH);
		srcpath += file.getFileName();
	} else
	{
		srcpath = FARWrappers::getCurrentDirectory(false);

		AddEndSlash(srcpath, LOC_SLASH);
		srcpath += file.getFileName();
	}
	splitFilename(srcpath, p->SrcPath, p->fileName_, !Download);

	urlsQueue_.push_back(p);
}

void FTP::ListToQueque(const FileList& filelist, const FTPCopyInfo& ci)
{
	for(size_t n = 0; n < filelist.size(); n++)
	{
		if(filelist[n])
		{
			//Skip dirs
			if(filelist[n]->getType() == FTPFileInfo::directory)
				continue;

			AddToQueque(*filelist[n], ci.destPath, ci.download);
		}
	}
}

void FTP::ExecuteQueue(QueueExecOptions* op)
{
	if(urlsQueue_.empty())
		return;

	FTPUrl_  url = getConnection().getHost()->url_;
	std::wstring oDir;

	oDir = panel_->getCurrentDirectory();

	ExecuteQueueINT(op);

	//Restore plugin state
	if(op->RestoreState)
	{
		if(ShowHosts)
		{
			BackToHosts();
		} else
			if(!getConnection().getHost()->url_.compare(url))
			{
				FtpHostPtr host = FtpHostPtr(new FTPHost);
				host->url_ = url;
				FullConnect(host);

				FtpFilePanel_.resetFileCache();
			}
			FtpFilePanel_.SetDirectory(oDir, 0);
			Invalidate();
	}
}

void FTP::ExecuteQueueINT( QueueExecOptions* op )
{
	BOOST_ASSERT(0 && "TODO Not implemented");
/*
	PROCP(op->RestoreState << L", " << op->RemoveCompleted)
		FARWrappers::Screen scr;
	String          DefPath, LastPath, LastName;
	BOOL            rc;
	BOOL            needUpdate = FALSE;
	FTPUrl*         prev,*p,*tmp;
	FTPCopyInfo     ci;
	WIN32_FIND_DATA fd, ffd;

	//Copy info
	ci.asciiMode       = Host.AsciiMode;
	ci.ShowProcessList = FALSE;
	ci.AddToQueque     = FALSE;
	ci.MsgCode         = ocNone;
	ci.UploadLowCase   = g_manager.opt.UploadLowCase;

	//Check othe panel info
	PanelInfo pi;
	FARWrappers::getInfo().Control( INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELINFO, &pi );
	if ( pi.PanelType != PTYPE_FILEPANEL ||
		pi.Plugin )
		DefPath.Null();
	else
		DefPath = pi.CurDir;

	//DO full list
	prev        = NULL;
	p           = UrlsList;
	LastPath.Null();
	LastName.Null();

	while( p ) {

		//Check current host the same
		BOOST_LOG(INF, L"Queue: Check current host the same" ));
		if ( !hConnect ||
			!Host.CmpConnected( &p->Host ) ) {
				Host = p->Host;
				if ( !FullConnect() ) {
					if ( GetLastError() == ERROR_CANCELLED ) break;
					p->Error.printf( "%s: %s", getMsg(MQCanNotConnect), FIO_ERROR );
					goto Skip;
				}
				ResetCache=TRUE;
		}

		//Apply other parameters
		BOOST_LOG(INF, L"Queue: Apply other parameters" ));
		Host = p->Host;
		hConnect->InitData( &Host,-1 );
		hConnect->InitIOBuff();

		//Change local dir
		BOOST_LOG(INF, L"Queue: Change local dir" ));
		do{
			char *m = p->Download ? p->DestPath.c_str() : p->SrcPath.c_str();
			if ( !m[0] ) m = DefPath.c_str();
			if ( !m[0] ) {
				p->Error = getMsg(MQNotLocal);
				goto Skip;
			}

			if ( SetCurrentDirectory(m) ) break;
			if ( DoCreateDirectory(m) )
				if ( SetCurrentDirectory(m) ) break;
			p->Error.printf( getMsg(MQCanNotChangeLocal), m, FIO_ERROR );
			goto Skip;
		}while(0);

		//Check local file
		BOOST_LOG(INF, L"Queue: Check local file" ));
		if ( !p->Download ) {
			if ( !FRealFile( p->FileName.cFileName, &fd ) ) {
				p->Error.printf( getMsg(MQNotFoundSource), p->FileName.cFileName, FIO_ERROR );
				goto Skip;
			}
		}

		//IO file
		BOOST_LOG(INF, L"Queue: IO file" ));
		//Last used FTP path and name
		LastPath = p->Download ? p->SrcPath : p->DestPath;
		LastName = PointToName(p->FileName.cFileName);

		//DOWNLOAD ------------------------------------------------
		if ( p->Download ) {
			ci.Download  = TRUE;

			ci.SrcPath = p->SrcPath;
			AddEndSlash( ci.SrcPath, '/' );
			ci.SrcPath.cat( p->FileName.cFileName );

			if ( p->DestPath.Length() ) {
				FixFileNameChars(p->DestPath);
				ci.DestPath = Unicode::utf8ToUtf16(p->DestPath.c_str());
			} else
				ci.DestPath = Unicode::utf8ToUtf16(DefPath.c_str());
			AddEndSlash( ci.DestPath, '\\' );
			ci.DestPath += Unicode::utf8ToUtf16(FixFileNameChars(p->FileName.cFileName,TRUE));

			__int64 fsz = FtpFileSize( hConnect, ci.SrcPath.c_str() );
			hConnect->trafficInfo_.Init( hConnect, MStatusDownload, 0, NULL );
			hConnect->trafficInfo_.InitFile( fsz, ci.SrcPath.c_str(), Unicode::utf16ToUtf8(ci.DestPath).c_str() );

			if ( FRealFile(ci.DestPath.c_str(),&fd) ) {
				if ( fsz != -1 ) {
					ffd = fd;
					ffd.nFileSizeHigh = (DWORD)( fsz / ((__int64)UINT_MAX) );
					ffd.nFileSizeLow  = (DWORD)( fsz % ((__int64)UINT_MAX) );
					ci.MsgCode  = AskOverwrite( MDownloadTitle, TRUE, &fd, &ffd, ci.MsgCode );
				} else
					ci.MsgCode  = AskOverwrite( MDownloadTitle, TRUE, &fd, NULL, ci.MsgCode );

				switch( ci.MsgCode ) {
			 case   ocOverAll:
			 case      ocOver: break;
			 case      ocSkip:
			 case   ocSkipAll: goto Skip;
			 case    ocResume:
			 case ocResumeAll: break;
				}
				if ( ci.MsgCode == ocCancel ) {
					SetLastError( ERROR_CANCELLED );
					break;
				}
			}

			rc = _FtpGetFile( ci.SrcPath.c_str(),
				Unicode::utf16ToUtf8(ci.DestPath).c_str(),
				ci.MsgCode == ocResume || ci.MsgCode == ocResumeAll,
				ci.asciiMode );
		} else {
			//UPLOAD -------------------------------------------------
			ci.Download  = FALSE;

			ci.SrcPath = p->SrcPath;
			AddEndSlash( ci.SrcPath, '\\' );
			ci.SrcPath.cat( PointToName(p->FileName.cFileName) );

			if ( p->DestPath[0] )
				ci.DestPath = Unicode::utf8ToUtf16(p->DestPath.c_str());
			else
				GetCurPath( ci.DestPath );
			AddEndSlash( ci.DestPath, NET_SLASH);
			ci.DestPath += Unicode::utf8ToUtf16(PointToName(p->FileName.cFileName));

			__int64 fsz = FtpFileSize( hConnect, Unicode::utf16ToUtf8(ci.DestPath).c_str());

			hConnect->trafficInfo_.Init( hConnect, MStatusUpload, 0, NULL );
			hConnect->trafficInfo_.InitFile( &fd, ci.SrcPath.c_str(), Unicode::utf16ToUtf8(ci.DestPath).c_str() );

			if ( fsz != -1 ) {
				ffd = fd;
				ffd.nFileSizeHigh = (DWORD)(fsz / ((__int64)UINT_MAX));
				ffd.nFileSizeLow  = (DWORD)(fsz % ((__int64)UINT_MAX));
				ci.MsgCode  = AskOverwrite( MUploadTitle, FALSE, &ffd, &fd, ci.MsgCode );

				switch( ci.MsgCode ) {
			 case   ocOverAll:
			 case      ocOver: break;
			 case      ocSkip:
			 case   ocSkipAll: goto Skip;
			 case    ocResume:
			 case ocResumeAll: break;
				}
				if ( ci.MsgCode == ocCancel ) {
					SetLastError( ERROR_CANCELLED );
					break;
				}
				needUpdate = TRUE;
			}

			rc = putFile( ci.SrcPath.c_str(),
				Unicode::utf16ToUtf8(ci.DestPath).c_str(),
				ci.MsgCode == ocResume || ci.MsgCode == ocResumeAll,
				ci.asciiMode );

			needUpdate = needUpdate || rc == TRUE;
		}

		//IO completed
		if ( rc == -1 ||
			!rc && GetLastError() == ERROR_CANCELLED ) {
				SetLastError( ERROR_CANCELLED );
				break;
		}
		if ( !rc ) {
			if ( p->Download )
				p->Error.printf( getMsg(MQErrDowload), FIO_ERROR );
			else
				p->Error.printf( getMsg(MQErrUpload), FIO_ERROR );
			goto Skip;
		}

		//Done
		BOOST_LOG(INF, L"Queue: Done" ));
		tmp = p->Next;
		if ( op->RemoveCompleted )
			DeleteUrlItem( p, prev );
		p = tmp;
		continue;

		//Error
Skip:
		BOOST_LOG(INF, L"Queue: Error" ));
		prev = p;
		p    = p->Next;
	}

	//Reread files on FTP in case files are uploaded
	if ( !ShowHosts &&
		hConnect &&
		getConnection().keepAlive() ) {

			if ( !op->RestoreState ) {
				if ( LastPath.Length() ) SetDirectoryStepped( LastPath.c_str(), TRUE );
				if ( LastName.Length() ) SelectFile = LastName;
			}

			FP_Screen::FullRestore();

			if ( needUpdate )
				Reread();
			else
				Invalidate();
	}
*/
}
