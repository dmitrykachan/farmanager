#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"
#include "../utils/uniconverts.h"

std::map<unsigned int, std::wstring> g_stringmap;

//---------------------------------------------------------------------------------
void FTP::LongBeepEnd( BOOL DoNotBeep /*= FALSE*/ )
  {
     if ( LongBeep ) {
       if ( FP_PeriodEnd(LongBeep) && !DoNotBeep ) {
         MessageBeep( MB_ICONASTERISK );
       }
       FP_PeriodDestroy( LongBeep );
       LongBeep = NULL;
     }
}
void FTP::LongBeepCreate( void )
  {
     if ( g_manager.opt.LongBeepTimeout ) {
       LongBeepEnd( FALSE );
       LongBeep = FP_PeriodCreate( g_manager.opt.LongBeepTimeout*1000 );
     }
}
//---------------------------------------------------------------------------------
void FTP::SaveUsedDirNFile( void )
{  
	PanelInfo pi;

	//Save current file to restore
	if ( !ShowHosts && hConnect ) 
	{
		Host.directory_ = FtpGetCurrentDirectory(hConnect);
	}

	//Save current file to restore
	if ( FP_Info->Control(this,FCTL_GETPANELINFO,&pi) )
	{
		if ( pi.ItemsNumber > 0 && pi.CurrentItem < pi.ItemsNumber )
		{
			selectFile_ = FTP_FILENAME( &pi.PanelItems[pi.CurrentItem]);
		}
	}
}

std::wstring FTP::GetCurPath()
{
	if(ShowHosts)
		return hostsPath_;

	return FtpGetCurrentDirectory(hConnect);
}

void FTP::FTP_FixPaths( CONSTSTR base, PluginPanelItem *p, int cn, BOOL FromPlugin )
{  
	std::wstring str;
	if(!base || !base[0])
		return;

	for( ; cn--; p++ ) 
	{
		std::wstring CurName = FTP_FILENAME( p );

		if (CurName == L".." || CurName == L"." == 0)
			continue;

		str = Unicode::utf8ToUtf16(base) + (FromPlugin? L'/' : L'\\') + CurName;

		SET_FTP_FILENAME(p, str);
	}
}

int FTP::ExpandList( PluginPanelItem *pi, size_t icn,FP_SizeItemList* il,BOOL FromPlugin, ExpandListCB cb,LPVOID Param )
{  
	PROC(("ExpandList","cn:%d, ilcn:%d/%d, %s, cb:%08X",icn,il ? il->Count() : 0,il ? il->MaxCount : 0,FromPlugin?"PLUGIN":"LOCAL",cb ))

		BOOL             pSaved  = !Host.directory_.empty() && !selectFile_.empty();
	BOOL             old_ext = hConnect ? hConnect->Host.ExtCmdView : FALSE;
	FTPCurrentStates olds = CurrentState;
	int              rc;

	{  FTPConnectionBreakable _brk( hConnect,FALSE );
	CurrentState  = fcsExpandList;
	if ( hConnect ) {
		hConnect->Host.ExtCmdView = FALSE;
		hConnect->CurrentState    = fcsExpandList;
	}

	if ( !pSaved )
		SaveUsedDirNFile();

	rc = ExpandListINT(pi, icn,il,FromPlugin,cb,Param );

	if ( hConnect ) {
		hConnect->Host.ExtCmdView = old_ext;
		hConnect->CurrentState    = olds;
	}
	CurrentState  = olds;
	}

	Log(( "ExpandList rc=%d",rc ));

#if defined(__FILELOG__)
	if ( rc ) {
		Log(( "Expand succ ends containing:" ));
		if ( il )
			LogPanelItems( il->Items(), il->Count() );
		else
			Log(( "Files list does not contains files" ));
	}
#endif

	if (!pSaved)
	{
		if (!rc)
		{
			SaveLastError _err;
			if (!Host.directory_.empty())
			{
				std::wstring path = GetCurPath();
				if(boost::algorithm::iequals(path, Host.directory_) == false)
					SetDirectory(Host.directory_, FP_LastOpMode);
			}
		} else
		{
			selectFile_		= L"";
			Host.directory_	= L"";
		}
	}

	return rc;
}

//---------------------------------------------------------------------------------
bool FTP::FTP_SetDirectory(const std::wstring &dir, bool FromPlugin )
{  
	if(FromPlugin)
		return SetDirectory(dir,OPM_SILENT ) == TRUE;
	else
		return SetCurrentDirectoryW(dir.c_str());
}

BOOL FTP::FTP_GetFindData( PluginPanelItem **PanelItem,int *ItemsNumber,BOOL FromPlugin )
  {
    if ( FromPlugin ) {
      Log(( "PLUGIN GetFindData" ));
      return GetFindData( PanelItem,ItemsNumber,OPM_SILENT );
    }

    PROC(( "LOCAL GetFindData", NULL ))

    FP_SizeItemList il( FALSE );
    WIN32_FIND_DATA fd;
    PluginPanelItem p;
    HANDLE          h = FindFirstFile( "*.*",&fd );

    if ( h == INVALID_HANDLE_VALUE ) {
#if defined(__FILELOG__)
      char path[ FAR_MAX_PATHSIZE ];
      path[GetCurrentDirectory( sizeof(path),path )] = 0;
      Log(( "Files in [%s] not found: %s", path, FIO_ERROR ));
#endif
      *PanelItem   = NULL;
      *ItemsNumber = 0;
      return TRUE;
    } else {
#if defined(__FILELOG__)
      char path[ FAR_MAX_PATHSIZE ];
      path[GetCurrentDirectory( sizeof(path),path )] = 0;
      Log(( "Files in [%s] are found", path ));
#endif
    }

    do{
      char *CurName = fd.cFileName;
      if ( StrCmp( CurName,"..") == 0 || StrCmp(CurName,".") == 0 ) continue;

      Log(( "Found: [%s]%d", fd.cFileName, fd.dwFileAttributes ));

      //Reset Reserved becouse it used by plugin but may cantain trash after API call
      fd.dwReserved0 = 0;
      fd.dwReserved1 = 0;

      //Reset plugin structure
      MemSet( &p,0,sizeof(PluginPanelItem) );

      //Copy win32 data
      MemMove( &p.FindData,&fd,sizeof(p.FindData) );

      if ( !il.Add(&p,1) )
        return FALSE;
    }while( FindNextFile(h,&fd) );

    FindClose( h );
    *PanelItem   = il.Items();
    *ItemsNumber = static_cast<int>(il.Count());
 return TRUE;
}

void FTP::FTP_FreeFindData( PluginPanelItem *PanelItem,int ItemsNumber,BOOL FromPlugin )
  {
    if ( PanelItem && ItemsNumber ) {
      if ( FromPlugin )
        FreeFindData( PanelItem,ItemsNumber );
       else
        _Del( PanelItem );
    }
}

//---------------------------------------------------------------------------------
int FTP::ExpandListINT( PluginPanelItem *pi, size_t icn,FP_SizeItemList* il,BOOL FromPlugin,ExpandListCB cb,LPVOID Param )
{  
	PluginPanelItem *DirPanelItem;
     int              DirItemsNumber,res;
	 std::wstring		m;
	 std::wstring		curname;
     size_t				n,num;
     __int64          lSz,lCn;
	 std::wstring		curp;

    if ( !icn )
      return TRUE;

    if ( CheckForEsc(FALSE,TRUE) ) {
      Log(( "ESC: ExpandListINT cancelled" ));
      SetLastError( ERROR_CANCELLED );
      return FALSE;
    }

    for( n = 0; n < icn; n++ ) {
		curname = FTP_FILENAME(&pi[n]);

      if (curname == L".." || curname == L".")
        continue;

//============
//FILE
	  if ( !IS_FLAG(pi[n].FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
	  {
		  m = getName(curname);
		  if ( IncludeMask[0] && !FP_InPattern(IncludeMask, Unicode::utf16ToUtf8(m).c_str()) ) 
		  {
			  continue;
		  }
		  if ( ExcludeMask[0] && FP_InPattern(ExcludeMask,Unicode::utf16ToUtf8(m).c_str()) ) 
		  {
			  continue;
		  }

		  if (cb && !cb(this, &pi[n],Param) )
			  return FALSE;

		  if ( il ) {
			  il->TotalFullSize += ((__int64)pi[n].FindData.nFileSizeHigh) * ((__int64)MAX_DWORD) +
				  ((__int64)pi[n].FindData.nFileSizeLow);
			  il->TotalFiles++;

			  //Add
			  PluginPanelItem *tmp = il->Add( &pi[n] );

			  //Reset spesial plugin fields
			  tmp->FindData.dwReserved0 = 0;
			  tmp->FindData.dwReserved1 = 0;
		  }

		  continue;
	  }

//============
//DIR
	//Get name
	m = getName(curname);

	//Remember current
	curp = GetCurPath();

    //Set dir
	if(!FTP_SetDirectory(m, FromPlugin))
        return FALSE;

      if(FromPlugin)
	  {
			if(curp == GetCurPath())
				continue;
      }

	  //Get subdir list
	  if (hConnect && !cb) 
	  {
		  String str;
		  char   digit[ 50 ];
		  if ( il )
			  str.printf( "%sb in %u: %s",
					FDigit(digit,il->TotalFullSize,-1),
					(int)il->TotalFiles,
					Unicode::utf16ToUtf8(curname).c_str());
		  else
			  str = Unicode::utf16ToUtf8(curname).c_str();

		  FtpConnectMessage( hConnect, MScaning, str.c_str() );
	  } else
		  if (hConnect)
			  FtpConnectMessage( hConnect, MScaning, Unicode::utf16ToUtf8(getName(curname)).c_str());

      if ( il ) {
        num = il->Count();
        il->Add( &pi[n] );
      } else
        num = -1;

      if ( FTP_GetFindData( &DirPanelItem,&DirItemsNumber,FromPlugin ) ) {
		  FTP_FixPaths(Unicode::utf16ToUtf8(curname).c_str(), DirPanelItem, DirItemsNumber, FromPlugin );
          if ( num != -1 ) {
            lSz = il->TotalFullSize;
            lCn = il->TotalFiles;
          }
          res = ExpandListINT( DirPanelItem,DirItemsNumber,il,FromPlugin,cb,Param );
          if ( num != -1 && res ) {
            lSz = il->TotalFullSize - lSz;
            lCn = il->TotalFiles    - lCn;
            il->Item(num)->FindData.nFileSizeHigh = (DWORD)(lSz / ((__int64)MAX_DWORD));
            il->Item(num)->FindData.nFileSizeLow  = (DWORD)(lSz % ((__int64)MAX_DWORD));
            il->Item(num)->FindData.dwReserved0   = (DWORD)lCn;
          }
        FTP_FreeFindData( DirPanelItem,DirItemsNumber,FromPlugin );
      } else
        return FALSE;

      if ( !res ) return FALSE;

      if (!FTP_SetDirectory(L"..", FromPlugin ))
        return FALSE;

      if (cb && !cb(this, &pi[n],Param) )
        return FALSE;
    }

  return TRUE;
}
//---------------------------------------------------------------------------------
void FTP::Invalidate( void )
  {
    FP_Info->Control(this,FCTL_UPDATEPANEL,NULL);
    FP_Info->Control(this,FCTL_REDRAWPANEL,NULL);
}

//---------------------------------------------------------------------------------
BOOL FTP::Reread( void )
{
	PanelInfo pi;
	std::wstring oldp, newp, cur;

	oldp = GetCurPath();

	//Save current file to restore
	FP_Info->Control( this, FCTL_GETPANELINFO, &pi );
	if(pi.ItemsNumber > 0 && pi.CurrentItem < pi.ItemsNumber)
		cur = FTP_FILENAME(&pi.PanelItems[pi.CurrentItem]);

	//Reread
	if ( !ShowHosts )
		ResetCache = TRUE;

	FP_Info->Control( this, FCTL_UPDATEPANEL, (void*)1 );

	//Redraw
	newp = GetCurPath();
	int rc = oldp == newp;

	if(rc)
	{
		selectFile_ = cur;
	}

	FP_Info->Control( this, FCTL_REDRAWPANEL, NULL );

	return rc;
}

//---------------------------------------------------------------------------------
void FTP::CopyNamesToClipboard( void )
{  
	std::wstring     FullName, CopyData;
	PanelInfo  pi;
	size_t        CopySize;
	int n;

	FP_Info->Control( this, FCTL_GETPANELINFO, &pi );

	std::wstring s = FtpGetCurrentDirectory(hConnect);

	Host.MkUrl(FullName, s, L"");

	CopySize = (FullName.size() + 1 + 2 + 2)*pi.SelectedItemsNumber; // slash +	quotes and /r/n
	for ( CopySize = n = 0; n < pi.SelectedItemsNumber; n++ )
		CopySize += FTP_FILENAME(&pi.SelectedItems[n]).size();

	CopyData.reserve(CopySize);

	for ( n = 0; n < pi.SelectedItemsNumber; n++ ) 
	{
		std::wstring s;
		s = FullName;
		AddEndSlash(s, L'/');
		s += FTP_FILENAME(&pi.SelectedItems[n]);

		if ( g_manager.opt.QuoteClipboardNames )
			QuoteStr( s );

		CopyData += s + L"\r\n";
	}

	if(!CopyData.empty())
		WinAPI::copyToClipboard(CopyData);
}

//---------------------------------------------------------------------------------
void FTP::BackToHosts( void )
{  
	int num = g_manager.getRegKey().get(L"LastHostsMode", 2);

// TODO	selectFile_       = Host.regKey_;
    SwitchingToFTP   = FALSE;
    RereadRequired   = TRUE;
    ShowHosts        = TRUE;
    Host.hostname_	= L"";

    delete hConnect;
    hConnect = NULL;

    FP_Info->Control( this,FCTL_SETVIEWMODE,&num );
}
