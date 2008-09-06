#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "farwrapper/dialog.h"
#include "farwrapper/menu.h"

int AskDeleteQueue( void )
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
	FTPUrl* p, *p1;

	for(p = UrlsList; p; p = p1)
	{
       p1 = p->Next;
       delete p;
     }
     UrlsList = UrlsTail = NULL;
     QuequeSize = 0;
}

bool FTP::WarnExecuteQueue( QueueExecOptions* op )
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

void FTP::SetupQOpt( QueueExecOptions* op )
{
	op->RestoreState    = g_manager.opt.RestoreState;
	op->RemoveCompleted = g_manager.opt.RemoveCompleted;
}

const wchar_t* FTP::InsertCurrentToQueue()
{
	PanelInfo   pi, api;
	FARWrappers::ItemList backup,il;
	FTPCopyInfo ci;

	if(!getPanelInfo(pi) || !FARWrappers::getShortPanelInfo(api, false))
		return getMsg(MErrGetPanelInfo);

	if( pi.SelectedItemsNumber <= 0 ||
		pi.SelectedItemsNumber == 1 && !is_flag(pi.SelectedItems[0]->Flags,PPIF_SELECTED) )
		return getMsg(MErrNoSelection);

	backup.add(pi.SelectedItems, pi.SelectedItemsNumber);

	BOOL rc = ExpandList(backup, &il, TRUE );
	FARWrappers::Screen::fullRestore();
	if ( !rc )
		return GetLastError() == ERROR_CANCELLED ? NULL : getMsg(MErrExpandList);

	ci.download = true;
	if ( api.PanelType != PTYPE_FILEPANEL || api.Plugin )
		ci.destPath = L"";
	else
		ci.destPath = api.lpwszCurDir;

//TODO	ListToQueque( &il, &ci );

	return NULL;
}

const wchar_t* FTP::InsertAnotherToQueue()
{
	FARWrappers::ItemList backup,il;
	PanelInfo       pi;
	FTPCopyInfo     ci;

	if (ShowHosts )
		return getMsg(MQErrUploadHosts);

	if (!FARWrappers::getPanelInfo(pi, false))
		return getMsg(MErrGetPanelInfo);

	if ( pi.SelectedItemsNumber <= 0 ||
		pi.SelectedItemsNumber == 1 && !is_flag(pi.SelectedItems[0]->Flags,PPIF_SELECTED) )
		return getMsg(MErrNoSelection);

	if ( pi.PanelType != PTYPE_FILEPANEL || pi.Plugin )
		return getMsg(MErrNotFiles);

	backup.add(pi.SelectedItems, pi.SelectedItemsNumber);

	BOOL rc = ExpandList( backup, &il, FALSE );
	FARWrappers::Screen::fullRestore();
	if ( !rc )
		return GetLastError() == ERROR_CANCELLED ? NULL : getMsg(MErrExpandList);

	ci.download = FALSE;
	ci.destPath = panel_->getCurrentDirectory();

//TODO	ListToQueque( &il, &ci );

	return NULL;
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
	const wchar_t* err;
	FTPUrl   tmp;

	do
	{
		sel = menu.show();

		if(sel == -1)
			return;

		err = NULL;
		switch( sel )
		{
		case 0: 
			UrlInit( &tmp );
			if ( EditUrlItem( &tmp ) )
			{
				AddToQueque( &tmp );
				return;
			}
			break;
		case 1: 
			err = InsertCurrentToQueue();
			if ( !err && GetLastError() != ERROR_CANCELLED ) return;
			break;
		case 2: err = InsertAnotherToQueue();
			if ( !err && GetLastError() != ERROR_CANCELLED) return;
			break;
		}
		if ( err )
		{
			const wchar_t* itms[] = { getMsg(MQErrAdding), err, getMsg(MOk) };
			FARWrappers::message(itms, 1, FMSG_WARNING);
		}
	}while(1);
}

void FTP::QuequeMenu( void )
{
	BOOST_ASSERT(0 && "TODO: Not implemented");
/*
	int              n,
		num;
	int              Breaks[] = { VK_DELETE, VK_INSERT, VK_F4, VK_RETURN, 0 },
		BNumber;
	FarMenuItem     *mi = NULL;
	FTPUrl*          p,*p1;
	char             str1[ FAR_MAX_PATHSIZE ],
		str2[ FAR_MAX_PATHSIZE ],
		str3[ FAR_MAX_PATHSIZE ];
	QueueExecOptions exOp;

	SetupQOpt( &exOp );
	num = -1;
	do{
		mi = (FarMenuItem *)_Realloc( mi, (QuequeSize+1)*sizeof(FarMenuItem) );
		MemSet( mi, 0, QuequeSize*sizeof(FarMenuItem) );

		for( p = UrlsList,n = 0; p; p = p->Next, n++ ) 
		{
			StrCpy( str1, p->SrcPath.c_str(),    20 );
			StrCpy( str2, p->DestPath.c_str(),   20 );
			StrCpy( str3, p->FileName.cFileName, 20 );
			SNprintf( mi[n].Text, sizeof(mi[n].Text),
				"%c%c %-20s%c%-20s%c%-20s",
				p->Download ? '-' : '<', p->Download ? '>' : '-',
				str1, FAR_VERT_CHAR,
				str2, FAR_VERT_CHAR,
				str3 );
			if ( p->Error[0] )
				mi[n].Checked = TRUE;
		}

		//Title
		char title[ FAR_MAX_PATHSIZE ];
		SNprintf( title, sizeof(title), "%s: %d %s", getMsg(MQMenuTitle), n, getMsg(MQMenuItems) );

		//Menu
		if ( num != -1 && num < QuequeSize ) mi[num].Selected = TRUE;

		n = FARWrappers::getInfo().Menu( FP_Info->ModuleNumber,-1,-1,0,FMENU_SHOWAMPERSAND,
			title,
			getMsg(MQMenuFooter),
			"FTPQueue", Breaks, &BNumber, mi, QuequeSize );
		//key ESC
		if ( BNumber == -1 &&
			n == -1 )
			goto Done;

		//key Enter
		if ( BNumber == -1 ) {
			//??
			goto Done;
		}

		//Set selected
		if ( num != -1 ) mi[num].Selected = FALSE;
		num = n;

		//Process keys
		switch( BNumber ) {
			/ *DEL* /
		 case 0: if ( QuequeSize )
					 switch( AskDeleteQueue() ) {
		 case -1:
		 case  2: break;
		 case  0: p = UrlItem( n, &p1 );
			 DeleteUrlItem( p, p1 );
			 break;
		 case  1: ClearQueue();
			 break;
				 }
				 break;
				 / *Ins* /
		 case 1: InsertToQueue();
			 break;
			 / *F4* /
		 case 2: p = UrlItem( n, NULL );
			 if (p)
				 EditUrlItem( p );
			 break;
			 / *Return* /
		 case 3: if ( QuequeSize &&
					 WarnExecuteQueue(&exOp) ) {
						 ExecuteQueue(&exOp);
						 if ( !QuequeSize )
							 goto Done;
				 }
				 break;
		}

	}while(1);
Done:
	_Del( mi );
*/
}

void FTP::AddToQueque(FTPUrl* item, int pos)
{
	FTPUrl* p, *p1,*newi;

     newi = new FTPUrl;
//  TODO HOST  ASSIGN
//     *newi = *item;

     if ( pos == -1 ) pos = QuequeSize;
     p = UrlItem( pos, &p1 );
     if (p1) p1->Next = newi;
     newi->Next = p;
     if (p == UrlsList) UrlsList = newi;

     QuequeSize++;
}

void FTP::AddToQueque(FTPFileInfo& fileName, const std::wstring& path, bool Download)
{
	FTPUrl* p = new FTPUrl;

//TODO HOST ASSIGN	p->Host	= chost_;
	p->Download = Download;



	BOOST_ASSERT(0 && "Not implemented");
/*
	String  str;
	char   *m;
	int     num;
	FTPUrl* p = new FTPUrl;

	p->Next     = NULL;
	p->FileName = *FileName;
	p->Error.Null();
	p->DestPath = Path;

	if ( Download )
		m = strrchr( FileName->cFileName, '/' );
	else
		m = strrchr( FileName->cFileName, '\\' );
	if ( m ) {
		*m = 0;
		p->DestPath.Add( m );
		MemMove( FileName->cFileName, m+1, m-FileName->cFileName );
	}

	if ( Download ) {
		GetCurPath( p->SrcPath );
		AddEndSlash( p->SrcPath, '/' );
		str.printf( "%s%s", p->SrcPath.c_str(), FileName->cFileName );
		FixLocalSlash( p->DestPath );
		AddEndSlash( p->DestPath, '\\' );

		num = str.Chr( '/' );
	} else {
		PanelInfo pi;
		FarWrapper::getPanelInfo(pi, false);
		p->SrcPath = pi.CurDir;

		AddEndSlash( p->SrcPath, '\\' );
		str.printf( "%s%s", p->SrcPath.c_str(), FileName->cFileName );
		FixLocalSlash(str);
		AddEndSlash( p->DestPath, '/' );

		num = str.Chr( '\\' );
	}

	if ( num != -1 ) {
		TStrCpy( p->FileName.cFileName, str.c_str()+num+1 );
		str.SetLength( num );
		p->SrcPath = str;
	} else {
		TStrCpy( p->FileName.cFileName, str.c_str() );
		p->SrcPath.Null();
	}

	if (!UrlsList) UrlsList = p;
	if (UrlsTail)  UrlsTail->Next = p;
	UrlsTail = p;
	QuequeSize++;
*/
}

void FTP::ListToQueque(const FileList& filelist, const FTPCopyInfo& ci)
{
	for(size_t n = 0; n < filelist.size(); n++ )
	{
		//Skip dirs
		if(filelist[n]->getType() == FTPFileInfo::directory)
			continue;

		//Skip deselected in list
		// TODO if (il->at(n).Reserved[0] == UINT_MAX )
		//	continue;

		AddToQueque(*filelist[n], ci.destPath, ci.download);
	}
}

void FTP::ExecuteQueue(QueueExecOptions* op)
{
	if(!QuequeSize)
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
			AddEndSlash( ci.DestPath, '/' );
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
