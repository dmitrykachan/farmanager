#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

#if defined(__DEBUG__)
void ProcError( int v = 0 )
  {  int sel;

    //Assert( 0 );
    //ProcError();

    void (*p)(int) = 0;

    p( v );

    sel = 0;
    sel /= sel;
}
#endif

bool InsertHostsCmd( HANDLE h,BOOL full )
{
/*
	PanelInfo pi;
	FTPHost*  p;
	CONSTSTR  m;

	if (!FP_Info->Control(h,FCTL_GETPANELINFO,&pi ) ||
		!pi.ItemsNumber ||
		pi.CurrentItem < 0 || pi.CurrentItem >= pi.ItemsNumber ||
		(p=FTPHost::Convert(&pi.PanelItems[pi.CurrentItem])) == NULL )
		return FALSE;

	if ( full )
		m = p->HostName;
	else
		m = p->Host;

	if ( StrCmp(m,"ftp://",6,FALSE) != 0 &&
		StrCmp(m,"http://",7,FALSE) != 0 )
		FP_Info->Control( h,FCTL_INSERTCMDLINE, (void*)(p->Folder ? "" : "ftp://") );

	return FP_Info->Control( h,FCTL_INSERTCMDLINE,(HANDLE)m );
	*/
		BOOST_ASSERT(false && "TODO");
	return true;
}

int FTP::processArrowKeys(int key, int &res)
{
	PanelRedrawInfo	ri;
	PanelInfo	pi, otherPi;
	
	res = false;

	switch(key)
	{
		case  VK_END:
		case  VK_HOME:
		case  VK_UP:
		case  VK_DOWN:
			break;
		default:
			return false;
	}
			
	//Get info
	if (!FP_Info->Control( this,FCTL_GETPANELSHORTINFO,&pi) &&
		!FP_Info->Control( this,FCTL_GETPANELINFO,&pi))
		return true;

	//Skip self processing for QView work correctly
	if (!FP_Info->Control(this, FCTL_GETANOTHERPANELSHORTINFO, &otherPi) ||
		(otherPi.PanelType == PTYPE_QVIEWPANEL || otherPi.PanelType == PTYPE_TREEPANEL) )
		return true;

	//Get current
	ri.TopPanelItem = pi.TopPanelItem;
	switch(key)
	{
		case   VK_END: ri.CurrentItem = pi.ItemsNumber-1;   break;
		case  VK_HOME: ri.CurrentItem = 0;                  break;
		case    VK_UP: ri.CurrentItem = pi.CurrentItem - 1; break;
		case  VK_DOWN: ri.CurrentItem = pi.CurrentItem + 1; break;
	}

	//Move cursor
	if ( ri.CurrentItem == pi.CurrentItem || ri.CurrentItem < 0 || ri.CurrentItem >= pi.ItemsNumber )
		res = true;
	else
		res = FP_Info->Control(this,FCTL_REDRAWPANEL,&ri);
	return true;
}

int FTP::DisplayUtilsMenu()
{
	static CONSTSTR strings[] = 
	{
		/*00*/ NULL,
		/*01*/ "",
		/*02*/ FMSG(MHostParams),
		/*03*/ FMSG(MCloseConnection),
		/*04*/ FMSG(MUtilsDir),
		/*05*/ FMSG(MUtilsCmd),
		/*06*/ "",
		/*07*/ FMSG(MUtilsLog),
		/*08*/ NULL, //Sites list
		/*09*/ FMSG(MShowQueue),
	#if defined(__DEBUG__)
		/*10*/ "",
		/*11*/ FMSG("Generate DIVIDE BY ZERO &Bug (Plugin traps!)"),
	#endif
		NULL
	};

#if defined(__DEBUG__)
	strings[ 0] = Message( "%s \"" __DATE__ "\" " __TIME__ " (DEBUG) ", FP_GetMsg(MVersionTitle) );
#else
	strings[ 0] = Message( "%s \"" __DATE__ "\" " __TIME__ " ", FP_GetMsg(MVersionTitle) );
#endif
	strings[ 8] = isBackup() ? FMSG(MRemoveFromites) : FMSG(MAddToSites);

	//
	FP_MenuEx mnu(strings);
	int     prev = 0,
			sel = (ShowHosts || !hConnect) ? 7 : 2,
			file;

	mnu.Item(0)->isGrayed(true);

	do
	{
		if(ShowHosts || !hConnect)
		{
			mnu.Item(2)->isGrayed(true);
			mnu.Item(3)->isGrayed(true);
			mnu.Item(4)->isGrayed(true);
			mnu.Item(5)->isGrayed(true);
		}

		if(!g_manager.opt.UseBackups)
			mnu.Item(8)->isGrayed(true);

		mnu.Item(sel)->isSelected(true);
		sel = mnu.Execute( FMSG(MUtilsCaption),FMENU_WRAPMODE,NULL,"FTPUtils" );
		mnu.Item(prev)->isSelected(FALSE);
		prev                     = sel;

		switch(sel)
		{
			case -1: return true;

			//Version
			case  0:
				break;

			//Host parameters
			case  2:
				{
					if(ShowHosts || !hConnect) 
						break;
					FTPHost tmp = Host;
					if ( !GetHost( MEditFtpTitle, &tmp, FALSE ) ) 
						return true;

					//Reconnect
					if( !Host.CmpConnected( &tmp ) )
					{
						Host = tmp;
						if ( !FullConnect() )
						{
							BackToHosts();
							Invalidate();
						}
						return true;
					}

					Host = tmp;

					//Change connection paras
					hConnect->InitData( &Host,-1 );
					hConnect->InitIOBuff();
					Invalidate();
					return true;
				}

			//Switch to hosts
			case  3: 
				if(ShowHosts || !hConnect) 
					break;
				BackToHosts();
				Invalidate();
					return true;

			//Dir listing
			case  4: 
				if(ShowHosts || !hConnect)
					break;
				{
					char str[FAR_MAX_PATHSIZE];  //Must be static buff because of MkTemp
					FP_FSF->MkTemp(str, "FTP");
					CreateDirectory(str, NULL);
					
					TAddEndSlash( str,'\\' );
					TStrCat( str, "FTPDir.txt" );
					file = FIO_CREAT( str,0 );
					if ( file == -1 )
					{
						hConnect->ConnectMessage( MErrorTempFile,str,-MOk );
						return TRUE;
					}
					FIO_WRITE(file, Unicode::utf16ToUtf8(hConnect->output_).c_str(), static_cast<int>(hConnect->output_.size()));
					FIO_CLOSE(file);
					FP_Info->Viewer(str,Message("%s: %s {%s}",FP_GetMsg(MDirTitle),PanelTitle,str),
						0,0,-1,-1,VF_NONMODAL|VF_DELETEONCLOSE );
				}
				return true;

			//Show CMD
			case  5: 
				if(ShowHosts || !hConnect)
					break;
				file = hConnect->Host.ExtCmdView;
				hConnect->Host.ExtCmdView = TRUE;
				SetLastError(ERROR_SUCCESS);
				hConnect->ConnectMessage( MNone__,NULL,MOk );
				hConnect->Host.ExtCmdView = file;
				return true;

			// Show LOG
			case  7: 
				if(IsCmdLogFile())
					FP_Info->Viewer(Unicode::toOem(GetCmdLogFile()).c_str(), FP_GetMsg(MLogTitle),0,0,-1,-1,VF_NONMODAL|VF_ENABLE_F6 );
				return true;

			// Add\Remove sites list
			case  8: 
				if(isBackup())
					DeleteFromBackup();
				else
					AddToBackup();
				return true;

			//FTP queque
			case  9:
				QuequeMenu();
				return true;

#if defined(__DEBUG__)
			case 11: ProcError();
				break;
#endif
		}
	} while(1);
	return false;
}

int FTP::ProcessKey(int Key, unsigned int ControlState)
{  
	PROC(( "FTP::ProcessKey", "k:%08X(%c), sh:%08X",Key,Key < 127 && isprint(Key)?((char)Key):' ',ControlState ))
	PanelInfo			pi,otherPI;
	FTPHost		h;
	FTPHost		*p;
	int			res;

	//process arrows
	if(ControlState == 0 && processArrowKeys(Key, res))
		return res;

	//Check for empty command line
	if (!ShowHosts && ControlState==PKF_CONTROL && Key==VK_INSERT)
	{
		char str[1024];
		FP_Info->Control( this, FCTL_GETCMDLINE, str );
		if (!str[0])
			ControlState = PKF_CONTROL | PKF_SHIFT;
	}

	//Ctrl+BkSlash
	if (Key == VK_OEM_5 && ControlState == PKF_CONTROL)
	{
		if(ShowHosts)
			SetDirectory(L"\\", 0);
		else
			SetDirectory(L"/", 0);
		Reread();
		Invalidate();
		return true;
	}

	//Drop full url
	if(ShowHosts && ControlState == PKF_CONTROL)
	{
		if(Key == 'F')
			return InsertHostsCmd( this, true);
		else
			if(Key == VK_RETURN)
				return InsertHostsCmd(this, false);
	}

	//Utils menu
	if(ControlState == PKF_SHIFT && Key == VK_F1)
		return DisplayUtilsMenu();
	
	//Copy names
	if (!ShowHosts && ControlState==(PKF_CONTROL|PKF_SHIFT) && Key==VK_INSERT)
	{
		CopyNamesToClipboard();
		return TRUE;
	}

	//Table
	if (!ShowHosts && ControlState==PKF_SHIFT && Key==VK_F7)
	{
		hConnect->codePage_ = SelectTable(hConnect->codePage_);
		// TODO	  ("CharTable", hConnect->codePage_);
		Invalidate();
		return TRUE;
	}
	
	//Attributes
	if(hConnect && !ShowHosts && ControlState == PKF_CONTROL && Key=='A')
	{
		SetAttributes();
		Invalidate();
		return true;
	}
	
	//Save URL
	if(hConnect && !ShowHosts && ControlState==PKF_ALT && Key==VK_F6)
	{
		SaveURL();
		return true;
	}
	
	//Reread
	if (ControlState==PKF_CONTROL && Key=='R')
	{
		ResetCache = true;
		return false;
	}

	//Drop full name
	if(!ShowHosts && hConnect && ControlState==PKF_CONTROL && Key=='F')
	{
		FP_Info->Control(this,FCTL_GETPANELINFO,&pi);

		if (pi.CurrentItem >= pi.ItemsNumber)
			return false;

		std::wstring s;
		std::wstring path = FtpGetCurrentDirectory(hConnect);
		Host.MkUrl(s, path.c_str(), FTP_FILENAME(&pi.PanelItems[pi.CurrentItem]) );
		QuoteStr(s);
		FP_Info->Control( this, FCTL_INSERTCMDLINE, (void*)Unicode::toOem(s).c_str());
		return TRUE;
	}

	//Select
	if (ControlState == 0 && Key == VK_RETURN)
	{
		PluginPanelItem *cur;

		FP_Info->Control(this,FCTL_GETPANELINFO, &pi);
		if (pi.CurrentItem >= pi.ItemsNumber)
			return false;
		cur = &pi.PanelItems[pi.CurrentItem];

		//Switch to FTP
		if(ShowHosts &&	!IS_FLAG(cur->FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			p = findhost(cur->UserData);
			if (!p)
				return false;

			Host.Assign(p);
			FullConnect();

			return true;
		}

		//Change directory
		if(!ShowHosts && hConnect)
		{
			if (FTP_FILENAME(cur) != L"." &&
				FTP_FILENAME(cur) != L"..")
				if(IS_FLAG(cur->FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ||
				   IS_FLAG(cur->FindData.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT) )
				{
					if ( SetDirectoryFAR( IS_FLAG(cur->FindData.dwFileAttributes,FILE_ATTRIBUTE_REPARSE_POINT)
						? Unicode::fromOem(cur->CustomColumnData[FTP_COL_LINK])
						: FTP_FILENAME(cur),
						0 ) ) {
							FP_Info->Control( this,FCTL_UPDATEPANEL,NULL );

							struct PanelRedrawInfo RInfo;
							RInfo.CurrentItem = RInfo.TopPanelItem = 0;
							FP_Info->Control( this,FCTL_REDRAWPANEL,&RInfo );
							return TRUE;
					} else
						return TRUE;
				}
		}
	}/*ENTER*/

	//New entry
	if( ShowHosts &&
       ( (ControlState==PKF_SHIFT && Key==VK_F4) ||
         (ControlState==PKF_ALT   && Key==VK_F6) ) )
	{
		Log(( "New host" ));

		h.Init();
		if ( GetHost(MEnterFtpTitle,&h,FALSE) )
		{
			h.Write(this, hostsPath_);
			// TODO	  selectFile_ = h.regPath_;
			Invalidate();
		}
		return TRUE;
	}
	
	//Edit
	if(ShowHosts &&
       ( (ControlState==0           && Key==VK_F4) ||
         (ControlState==PKF_SHIFT   && Key==VK_F6) ||
         (ControlState==PKF_CONTROL && Key=='Z') ) )
	{
		FP_Info->Control(this,FCTL_GETPANELINFO,&pi);
		if ( pi.CurrentItem >= pi.ItemsNumber )
			return TRUE;

		p = findhost(pi.PanelItems[pi.CurrentItem].UserData);

		if ( !p ) // TODO
			return TRUE;
		if ( p->Folder )
		{
			if(!EditDirectory(p->Host_, p->hostDescription_,FALSE))
				return TRUE;
		} else {
			if ( !GetHost(MEditFtpTitle,p,Key=='Z') )
				return TRUE;
		}

		if (p->Write(this, hostsPath_))
		{
			// TODO      selectFile_ = h.regPath_;

			FP_Info->Control(this,FCTL_UPDATEPANEL,NULL);
			FP_Info->Control(this,FCTL_REDRAWPANEL,NULL);
		}

		return TRUE;
	}/*Edit*/

	//Copy/Move
	if ( !ShowHosts &&
       (ControlState == 0 || ControlState == PKF_SHIFT) &&
       Key == VK_F6 )
	{
		FTP *ftp = OtherPlugin(this);
		int  rc;

		if ( !ftp && ControlState == 0 && Key == VK_F6 )
			return FALSE;

		FP_Info->Control( this, FCTL_GETPANELINFO,        &pi );
		FP_Info->Control( this, FCTL_GETANOTHERPANELINFO, &otherPI );

		if ( pi.SelectedItemsNumber > 0 &&
			pi.CurrentItem >= 0 && pi.CurrentItem < pi.ItemsNumber ) 
		{
			do
			{
				std::wstring s;
				if ( ControlState == PKF_SHIFT ) 
				{
					s = FTP_FILENAME(&pi.PanelItems[pi.CurrentItem]);
					rc = GetFiles( &pi.PanelItems[pi.CurrentItem], 1, Key == VK_F6, s, 0 );
				} else {
					s = Unicode::fromOem(otherPI.CurDir);
					rc = GetFiles( pi.SelectedItems, pi.SelectedItemsNumber, Key == VK_F6, s, 0 );
				}

				if ( rc == TRUE || rc == -1 ) {
					FP_Screen::FullRestore();

					FP_Info->Control(this,FCTL_UPDATEPANEL,NULL);
					FP_Info->Control(this,FCTL_REDRAWPANEL,NULL);

					FP_Screen::FullRestore();

					FP_Info->Control(this,FCTL_UPDATEANOTHERPANEL,NULL);
					FP_Info->Control(this,FCTL_REDRAWANOTHERPANEL,NULL);

					FP_Screen::FullRestore();
				}
				return TRUE;
			}while(0);
		}
	}

	return false;
}
