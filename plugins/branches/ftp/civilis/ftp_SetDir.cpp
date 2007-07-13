#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

bool FTP::SetDirectoryStepped(const std::wstring &Dir, bool update)
{
	if ( ShowHosts || !hConnect )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	//Empty dir
	if (Dir.empty())
		return true;

	//Try direct change
	if(FtpSetCurrentDirectory(hConnect, Dir))
	{
		if (update) Invalidate();
		if ( ShowHosts || !hConnect ) {
			SetLastError( ERROR_CANCELLED );
			return false;
		}
		return true;
	}

	//dir name contains only one name
	if(Dir.find(L'/') == std::wstring::npos)
	{
		//Cancel
		return FALSE;
	}

	//Cancel changes in automatic mode
	if ( IS_SILENT(FP_LastOpMode) )
		return FALSE;

	//Ask user
	{  static CONSTSTR MsgItems[] = {
		FMSG(MAttention),
		FMSG(MAskDir1),
		NULL,
		FMSG(MAskDir3),
		FMSG(MYes), FMSG(MNo) };

		MsgItems[2] = Unicode::toOem(Dir).c_str();
		if ( FMessage( FMSG_WARNING|FMSG_LEFTALIGN, NULL, MsgItems, ARRAY_SIZE(MsgItems),2 ) != 0 )
		{
			SetLastError( ERROR_CANCELLED );
			return FALSE;
		}
	}

	//Try change to directory one-by-one
	std::wstring str;
	std::wstring::const_iterator itr = Dir.begin();
	std::wstring::const_iterator m; 

	do
	{
		if(*itr == L'/')
		{
			++itr;
			if(itr == Dir.end()) 
				break;
			str = L"/";
		} else
		{
			m = std::find(itr, Dir.end(), L'/');
			if(m != Dir.end())
			{
				str.assign(Dir.begin(), m);
				itr = m + 1;
			} else
				str = Dir;
		}

		Log(( "Dir: [%s]",str ));
		if ( !FtpSetCurrentDirectory(hConnect,str) )
			return FALSE;

		if (update) Invalidate();
		if ( ShowHosts || !hConnect ) {
			SetLastError( ERROR_CANCELLED );
			return FALSE;
		}

	} while(m != Dir.end());

	return true;
}

int FTP::SetDirectoryFAR(const std::wstring &Dir,int OpMode)
{  
	FP_Screen _scr;
	return SetDirectory(Dir, OpMode );
}

bool FTP::CheckDotsBack(const std::wstring &OldDir, const std::wstring& CmdDir)
{  
	std::wstring str;

	if(!g_manager.opt.CloseDots || CmdDir == L"..")
		return false;

	str = GetCurPath();
	DelEndSlash(str, '/');

	if(str == OldDir)
	{
		BackToHosts();
		return true;
	}

	return false;
}

int FTP::SetDirectory(const std::wstring &Dir,int OpMode )
{  
	size_t			Slash;
	std::wstring	oldDir;
	int				rc;

	oldDir = GetCurPath();

	Log(( "FTP::SetDirectory [%s] -> [%s]", Unicode::toOem(oldDir).c_str(),Dir ));

	//Hosts
	if (ShowHosts)
	{
		do{
			//Up
			if (Dir == L"..")
			{
				if (oldDir.empty() || oldDir == L"\\")
				{
					Log(( "Close plugin" ));
					CurrentState = fcsClose;
					FP_Info->Control( (HANDLE)this, FCTL_CLOSEPLUGIN, NULL );
					return true;
				}

				hostsPath_ = getPathBranch(hostsPath_);
				Log(( "DirBack" ));
				break;
			} else
				//Root
				if (Dir.empty() || (Dir == L"\\"))
				{
					hostsPath_ = L"";
					Log(( "Set to root" ));
					break;
				} else
				//Directory
				{
					hostsPath_ += L'\\' + Dir;
					Log(( "InDir" ));
					break;
				}
		}while(0);

		//Save old directory
		g_manager.getRegKey().set(L"LastHostsPath", hostsPath_);

		return TRUE;
	}

	//FTP
	/* Back to prev directory.
	Directories allways separated by '/'!
	*/
	if(Dir == L"..")
	{

		//Back from root
		if ( g_manager.opt.CloseDots &&
			oldDir[0] == L'/')
		{
				if ( !IS_SILENT(OpMode) )
					BackToHosts();
				return FALSE;
		}

		//Del last slash if any
		DelEndSlash(oldDir, '/');

		//Locate prev slash
		Slash = oldDir.rfind('/');

		//Set currently leaving directory to select in panel
		if (Slash != std::wstring::npos)
			selectFile_.assign(oldDir.begin() + Slash + 1, oldDir.end());
	}

	//Change directory
	rc = TRUE;
	do{
		if ( !hConnect )
		{
			rc = FALSE;
			break;
		}

		if(SetDirectoryStepped(Dir, false))
		{
			if ( !IS_SILENT(OpMode) )
				CheckDotsBack(oldDir, Dir);
			break;
		}

		if ( !hConnect || GetLastError() == ERROR_CANCELLED )
		{
				rc = FALSE;
				break;
		}

		if ( !hConnect->ConnectMessageTimeout( MChangeDirError,Dir,-MRetry ) ) {

			rc = FALSE;
			break;
		}

		if ( FtpCmdLineAlive(hConnect) &&
			FtpKeepAlive(hConnect) )
			continue;

		SaveUsedDirNFile();
		if ( !FullConnect() ) {
			rc = FALSE;
			break;
		}

	}while( 1 );

	return rc;
}
