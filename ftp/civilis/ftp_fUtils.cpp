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
	FARWrappers::getInfo().Control(this, FCTL_UPDATEPANEL, clearSelection, 0);
	FARWrappers::getInfo().Control(this, FCTL_REDRAWPANEL, 0, 0);
}

void FTP::BackToHosts()
{  
	int num = g_manager.getRegKey().get(L"LastHostsMode", 2);

    ShowHosts		= true;
	panel_			= &hostPanel_;

	hostPanel_.setSelectedHost(getConnection().getHost()->getIniFilename());

    FARWrappers::getInfo().Control(this, FCTL_SETVIEWMODE, num, 0);
}
