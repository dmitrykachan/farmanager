#include "stdafx.h"

#include "ftp_Int.h"
#include "utils/uniconverts.h"

void FTP::LongBeepEnd(bool DoNotBeep)
{
	if(!DoNotBeep && longBeep_.isTimeout())
	{
		MessageBeep( MB_ICONASTERISK );
	}
	longBeep_.setTimeout(0);
}

void FTP::LongBeepCreate( void )
{
	if(g_manager.opt.LongBeepTimeout)
	{
		longBeep_.reset();
	}
}

void FTP::SaveUsedDirNFile()
{  
	// TODO remove
}

void FTP::FTP_FixPaths(const std::wstring &base, FARWrappers::ItemList &items, BOOL FromPlugin )
{  
	std::wstring str;
	if(base.empty())
		return;

	for(size_t n = 0 ; n < items.size(); ++n) 
	{
		std::wstring CurName = FTP_FILENAME(&items[n]);

		if (CurName == L".." || CurName == L".")
			continue;

		str = base + (FromPlugin? L'/' : L'\\') + CurName;

		FARWrappers::setFileName(items[n], str);
	}
}

int FTP::ExpandList(FARWrappers::ItemList &panelItem, FARWrappers::ItemList* il, bool FromPlugin, LPVOID Param)
{
	BOOST_ASSERT(!chost_.url_.directory_.empty());
	bool pSaved  = !selectFile_.empty(); // TODO selFile_
	int  rc;

	{
		FTPConnectionBreakable _brk(&getConnection(), false);
		FTPCurrentStates olds = CurrentState;
		bool             old_ext;

		CurrentState  = fcsExpandList;
		if(getConnection().isConnected())
		{
			old_ext						= getConnection().getHost().ExtCmdView;
			getConnection().getHost().ExtCmdView	= false;
			getConnection().CurrentState		= fcsExpandList;
		}

		if(!pSaved)
			SaveUsedDirNFile();

		rc = ExpandListINT(panelItem, il, 0, FromPlugin, Param);

		if(getConnection().isConnected())
		{
			getConnection().getHost().ExtCmdView = old_ext;
			getConnection().CurrentState    = olds;
		}
		CurrentState  = olds;
	}

	BOOST_LOG(INF, L"ExpandList rc=" << rc);

	if(!pSaved)
	{
		if (!rc)
		{
			if (!chost_.url_.directory_.empty())
			{
				if(boost::algorithm::iequals(panel_->getCurrentDirectory(), chost_.url_.directory_) == false)
					FtpFilePanel_.SetDirectory(chost_.url_.directory_, FP_LastOpMode);
			}
		} else
		{
			selectFile_		= L""; // TODO selFile_
			chost_.url_.directory_	= L"";
		}
	}

	return rc;
}

bool FTP::FTP_SetDirectory(const std::wstring &dir, bool FromPlugin )
{  
	if(FromPlugin)
		return FtpFilePanel_.SetDirectory(dir,OPM_SILENT ) == TRUE;
	else
		return SetCurrentDirectoryW(dir.c_str()) != false;
}

void copy(FAR_FIND_DATA& f, const WIN32_FIND_DATAW &w)
{
	f.dwFileAttributes	= w.dwFileAttributes;
	f.ftCreationTime	= w.ftCreationTime;
	f.ftLastAccessTime	= w.ftLastAccessTime;
	f.ftLastWriteTime	= w.ftLastWriteTime;
	f.lpwszAlternateFileName = wcsdup(w.cAlternateFileName);
	f.lpwszFileName		= wcsdup(w.cFileName);
	f.nFileSize			= static_cast<__int64>(w.nFileSizeLow) + (static_cast<__int64>(w.nFileSizeHigh) << 32);
	f.nPackSize			= 0;
}

bool FTP::FTP_GetFindData(FARWrappers::ItemList &items, bool FromPlugin)
{
	items.clear();

    if(FromPlugin)
	{
		BOOST_ASSERT(0 && "TODO");
		return true;
//		return GetFindData( PanelItem,ItemsNumber,OPM_SILENT );
    }

    PROC;

    WIN32_FIND_DATAW fd;
    PluginPanelItem p;
    HANDLE          h = FindFirstFileW(L"*.*",&fd );

    if(h == INVALID_HANDLE_VALUE)
	{
      return true;
    }

    do
	{
		std::wstring CurName = fd.cFileName;
		if (CurName == L".." || CurName == L".")
			continue;

		//Reset Reserved becouse it used by plugin but may cantain trash after API call
		fd.dwReserved0 = 0;
		fd.dwReserved1 = 0;

		//Reset plugin structure
		memset( &p,0,sizeof(PluginPanelItem) );

		//Copy win32 data
		memmove( &p.FindData,&fd,sizeof(p.FindData) );

		items.add(p);
	}while( FindNextFileW(h,&fd) );

    FindClose(h);
	return true;
}

int FTP::ExpandListINT(FARWrappers::ItemList &panelItems, FARWrappers::ItemList* illlll, TotalFiles* totalFiles, bool FromPlugin, LPVOID Param)
{  
	int              res;
	FARWrappers::ItemList dirPanelItems;
	std::wstring		m;
	std::wstring		curname;
	size_t				n;
	__int64          lSz,lCn;
	std::wstring		curp;

	// TODO
	std::wstring	IncludeMask, ExcludeMask;

	if(panelItems.size() == 0)
		return true;

	if(CheckForEsc(FALSE,TRUE))
	{
		BOOST_LOG(INF, L"ESC: ExpandListINT cancelled");
		SetLastError( ERROR_CANCELLED );
		return false;
	}

	for(n = 0; n < panelItems.size(); n++)
	{
		curname = FTP_FILENAME(&panelItems[n]);

		if(curname == L".." || curname == L".")
			continue;

		//============
		//FILE
		if(!is_flag(panelItems[n].FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			m = getName(curname);
			if(!IncludeMask.empty() && !FARWrappers::patternCmp(IncludeMask.c_str(), m.c_str())) 
				continue;
			if(FARWrappers::patternCmp(ExcludeMask.c_str(), m.c_str())) 
				continue;

			if(totalFiles)
			{
				totalFiles->size_ += panelItems[n].FindData.nFileSize;
				++totalFiles->files_;
			}

			//Add
			illlll->add(panelItems[n]);
			continue;
		}

		//============
		//DIR
		//Get name
		m = getName(curname);

		//Remember current
		curp = panel_->getCurrentDirectory();

		//Set dir
		if(!FTP_SetDirectory(m, FromPlugin))
			return false;

		if(FromPlugin)
		{
			if(curp == panel_->getCurrentDirectory())
				continue;
		}

		//Get subdir list
//TODO 		if(!cb) 
// 		{
// 			std::wstring str;
// 			if(totalFiles)
// 				str = boost::lexical_cast<std::wstring>(totalFiles->size_) + L"b in " +
// 					  boost::lexical_cast<std::wstring>(totalFiles->files_) + L": " + curname;
// 			else
// 				str = curname;
// 			getConnection().ConnectMessage(MScaning, str.c_str());
// 		} else
// 			getConnection().ConnectMessage(MScaning, getName(curname).c_str());

		if(FTP_GetFindData(dirPanelItems, FromPlugin))
		{
			FTP_FixPaths(curname, dirPanelItems, FromPlugin);
			if(illlll->size() > 0 && totalFiles)
			{
				lSz = totalFiles->size_;
				lCn = totalFiles->files_;
			}
			BOOST_ASSERT(0 && "TODO");
			res = ExpandListINT(dirPanelItems, illlll, totalFiles, FromPlugin, Param);
			if(illlll->size() > 0 && res && totalFiles)
			{
				lSz = totalFiles->size_;
				lCn = totalFiles->files_;
				illlll->at(illlll->size()).FindData.nFileSize = lSz;
				illlll->at(illlll->size()).Reserved[1]  = (DWORD)lCn; // TODO FindData.dwReserved0
			}
		} else
			return false;

		if(!res) return false;

		if (!FTP_SetDirectory(L"..", FromPlugin ))
			return false;

	}

	return true;
}

void FTP::Invalidate(bool clearSelection)
{
	FARWrappers::getInfo().Control(this, FCTL_UPDATEPANEL, reinterpret_cast<void*>(clearSelection));
	FARWrappers::getInfo().Control(this, FCTL_REDRAWPANEL, NULL);
}

void FTP::BackToHosts()
{  
	int num = g_manager.getRegKey().get(L"LastHostsMode", 2);

	selectFile_		= getCurrentHost()->getIniFilename();
    SwitchingToFTP	= false;
    RereadRequired	= true;
    ShowHosts		= true;
    chost_.url_.fullhostname_	= L"";
	panel_			= &hostPanel_;

    FARWrappers::getInfo().Control( this,FCTL_SETVIEWMODE,&num );
}