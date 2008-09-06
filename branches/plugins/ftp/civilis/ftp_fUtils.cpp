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

// TODO remove
int FTP::ExpandList(FARWrappers::ItemList &panelItem, FARWrappers::ItemList* il, bool FromPlugin, LPVOID Param)
{
	return 0;
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


void FTP::Invalidate(bool clearSelection)
{
	FARWrappers::getInfo().Control(this, FCTL_UPDATEPANEL, reinterpret_cast<void*>(clearSelection));
	FARWrappers::getInfo().Control(this, FCTL_REDRAWPANEL, NULL);
}

void FTP::BackToHosts()
{  
	int num = g_manager.getRegKey().get(L"LastHostsMode", 2);

    ShowHosts		= true;
	panel_			= &hostPanel_;

    FARWrappers::getInfo().Control(this, FCTL_SETVIEWMODE, &num);
}
