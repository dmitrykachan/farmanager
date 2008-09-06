#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

Backup FTP::backups_;

FTP::FTP()
{
	hostPanel_.setPlugin(this);
	FtpFilePanel_.setPlugin(this);

	ShowHosts			= TRUE;
	panel_				= &hostPanel_;

	UrlsList			= NULL;
	QuequeSize			= 0;

	CallLevel			= 0;

	hostPanel_.setCurrentDirectory(g_manager.getRegKey().get(L"LastHostsPath", L""));

	PanelInfo  pi;
	FARWrappers::getShortPanelInfo(pi);
	startViewMode_		= pi.ViewMode;
}

FTP::~FTP()
{
	LongBeepEnd(true);

	FARWrappers::getInfo().Control(this, FCTL_SETVIEWMODE, &startViewMode_);
	FTP::backups_.remove(this);

	ClearQueue();
}

void FTP::Call( void )
{
	if(CallLevel == 0)
		LongBeepCreate();

	CallLevel++;
}

void FTP::End( int rc )
{
	BOOST_LOG(DBG, L"::End rc=" << rc);

	if (!CallLevel) return;

	CallLevel--;

	if(!CallLevel)
	{
		LongBeepEnd();
	}
}

const wchar_t* FTP::CloseQuery()
{
	if ( UrlsList != NULL )
		return L"Process queque is not empty";
	return NULL;
}

void FTP::SetBackupMode( void )
{
    PanelInfo  pi;
    FARWrappers::getInfo().Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi );
    ActiveColumnMode = pi.ViewMode;
}

void FTP::SetActiveMode( void )
{
     NeedToSetActiveMode = TRUE;
}

void Backup::add(FTP* ftp)
{
	if(!g_manager.opt.UseBackups)
		return;

	if(!find(ftp))
		array_.push_back(ftp);
}

bool Backup::find(FTP* ftp)
{
	return std::find(array_.begin(), array_.end(), ftp) != array_.end();
}

void Backup::remove(FTP* ftp)
{
	std::vector<FTP*>::iterator new_end = std::remove(array_.begin(), array_.end(), ftp);
	array_.erase(new_end, array_.end());
}

void Backup::eraseAll()
{
	std::vector<FTP*>::iterator itr = array_.begin();
	while(itr != array_.end())
	{
		delete *itr;
		++itr;
	}
}
