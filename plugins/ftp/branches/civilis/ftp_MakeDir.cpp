#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

extern std::string ftpQuote(const std::string& str);


bool FTP::FTPCreateDirectory(const std::wstring &dir, int OpMode)
{  
	BOOST_ASSERT(hConnect && "FtpCreateDirectory");
	if(dir.empty())
		return false;

	std::string mkdir = "mkdir ";
	std::wstring name;
	wchar_t		last = *dir.rbegin();

	do
	{
		//Try relative path
		name	= dir;
		if(!iswspace(last)) 
		{
			hConnect->CacheReset();
			if(hConnect->ProcessCommand(mkdir + ftpQuote(hConnect->toOEM(dir))))
				break;
		}

		//Try relative path with end slash
		hConnect->CacheReset();
		name	+= L'/';
		if(hConnect->ProcessCommand(mkdir + ftpQuote(hConnect->toOEM(dir))))
			break;

		//Try absolute path
		name = hConnect->curdir_ + L'/';
		if(dir[0] == '/')
			name.append(dir.begin()+1, dir.end());
		else
			name += dir;
		if(!iswspace(last)) 
		{
			if ( hConnect->ProcessCommand(mkdir + ftpQuote(hConnect->toOEM(dir))))
				break;
		}

		//Try absolute path with end slash
		name	+= L'/';
		if(hConnect->ProcessCommand(mkdir + ftpQuote(hConnect->toOEM(dir))))
			break;

		//Noone work
		return false;
	}while(0);

	if ( !IS_SILENT(OpMode) ) 
	{
		selectFile_ = name;
	}

	return true;
}

int FTP::MakeDirectory(std::wstring& Name,int OpMode)
{  
	PROC(("FTP::MakeDirectory",NULL));
	FTPHost h;

	if (!ShowHosts && !hConnect)
		return FALSE;
	h.Init();

	//Edit name
	if ( !IS_SILENT(OpMode) && !IS_FLAG(OpMode,OPM_NODIALOG) &&
		!EditDirectory(Name, ShowHosts ? h.hostDescription_ : L"", TRUE) )
		return -1;

	//Correct name
	if (!Name[0])
		return -1;

	//HOSTS
	if (ShowHosts)
	{
		if(Name == L"." ||	Name == L".." ||
			FTPHost::CheckHost(hostsPath_, Name))
		{
			SetLastError(ERROR_ALREADY_EXISTS);
			return FALSE;
		}

		h.Folder = TRUE;
		h.Host_ = h.MkINIFile(L"", L"");
		h.Write(this, hostsPath_);

		return TRUE;
	}

	//FTP
	FP_Screen   scr;

	//Create directory
	do{
		//Try to create
		if(hConnect && FTPCreateDirectory(Name, OpMode))
			return true;

		//If conection alive - report error
		if ( FtpCmdLineAlive(hConnect) ||
			IS_SILENT(OpMode) )
			return FALSE;

		//Try to reconnect
		if ( !Reread() )
			return FALSE;

		//Repeat operation
	}while( 1 );
}
