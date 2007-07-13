#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"
#include "utils/strutils.h"
#include "servertype.h"

const char quote = '\x1';

std::string ftpQuote(const std::string& str)
{
	return quote + str + quote;
}

bool ParseDirLine( Connection *Connect,BOOL AllFiles,FTPFileInfo* lpFindFileData );

//--------------------------------------------------------------------------------
BOOL FtpKeepAlive(Connection *hConnect)
  {
     if ( !hConnect ) {
       SetLastError(ERROR_INTERNET_CONNECTION_ABORTED);
       return FALSE;
     }

     hConnect->ErrorCode = ERROR_SUCCESS;
     if ( !hConnect->ProcessCommand("pwd") ) {
       SetLastError( hConnect->ErrorCode );
       return FALSE;
     } else
       return TRUE;
}

BOOL FtpIsResume( Connection *hConnect )
  {
     if (!hConnect) return FALSE;
 return ((Connection *)hConnect)->ResumeSupport;
}

BOOL FtpCmdLineAlive(Connection *hConnect)
  {
 return hConnect &&
        hConnect->connected &&
        hConnect->cmd_peer != INVALID_SOCKET;
}

void FtpSetRetryCount( Connection *hConnect,int cn )
  {
Assert( hConnect && "FtpSetRetryCount" );

    hConnect->RetryCount = cn;
}

BOOL FtpSetBreakable( Connection *hConnect,int cn )
  {
    if ( !hConnect )
      return FALSE;

    BOOL rc = hConnect->Breakable;
    if ( cn != -1 ) {
      Log(( "ESC: set brk %d->%d", hConnect->Breakable, cn ));
      hConnect->Breakable = cn;
    }
 return rc;
}

int FtpGetRetryCount( Connection *hConnect )
  {  Assert( hConnect && "FtpGetRetryCount" );

 return hConnect->RetryCount;
}

int FtpConnectMessage( Connection *hConnect,int Msg,CONSTSTR HostName,int btn /*= MNone__*/,int btn1 /*= MNone__*/,int btn2 /*= MNone__*/ )
  {
 return hConnect ? hConnect->ConnectMessage(Msg,HostName,btn,btn1,btn2) : FALSE;
}

int FtpCmdBlock( Connection *hConnect,int block )
  {  int rc = -1;

     do{
       if (!hConnect)
         break;

       rc = ((Connection *)hConnect)->CmdVisible == FALSE;

       if (block != -1)
         ((Connection *)hConnect)->CmdVisible = block == FALSE;

     }while(0);

 return rc;
}

BOOL FtpFindFirstFile( Connection *hConnect, CONSTSTR lpszSearchFile,FTPFileInfo* lpFindFileData, BOOL *ResetCache )
{
	Assert( hConnect && "FtpFindFirstFile" );
	std::string Command;
	int    AllFiles = StrCmp(lpszSearchFile,"*")==0 ||
		StrCmp(lpszSearchFile,"*.*")==0;
	int    FromCache;

	if ( ResetCache && *ResetCache == TRUE ) {
		hConnect->CacheReset();
		FtpGetFtpDirectory(hConnect);
		*ResetCache = FALSE;
	}

	if (AllFiles)
		Command = "dir";
	else {
		if (*lpszSearchFile=='\\' || lpszSearchFile[0] && lpszSearchFile[1]==':')
			lpszSearchFile = PointToName((char *)lpszSearchFile);
		Command = "dir" + ftpQuote(lpszSearchFile);
	}

	SetLastError( ERROR_SUCCESS );

	if ( !AllFiles || (FromCache=hConnect->CacheGet()) == 0 ) {
		if ( AllFiles && !IS_SILENT(FP_LastOpMode) &&
			hConnect->CmdVisible &&
			hConnect->CurrentState != fcsExpandList )
			hConnect->ConnectMessage( MRequestingFolder,
			hConnect->toOEM(hConnect->curdir_).c_str() );

		if (!hConnect->ProcessCommand(Command)) {
			SetLastError( hConnect->ErrorCode );
			return NULL;
		}
	}

	if ( AllFiles && !FromCache )
		hConnect->CacheAdd();

	return ParseDirLine(hConnect,AllFiles,lpFindFileData);
}


BOOL FtpFindNextFile(Connection *hConnect,FTPFileInfo* lpFindFileData )
  {
    Assert( hConnect && "FtpFindNextFile" );

 return ParseDirLine( hConnect,TRUE,lpFindFileData );
}

std::wstring FtpGetCurrentDirectory(Connection *hConnect)
{
	if ( !hConnect )
		return L"";

	if ( !hConnect->curdir_.empty() )
		if ( !FtpGetFtpDirectory(hConnect) )
			return L"";

	return hConnect->curdir_;
}

bool FtpSetCurrentDirectory(Connection *hConnect, const std::wstring& dir)
{  
	std::string command;

	BOOST_ASSERT(hConnect && "FtpSetCurrentDirectory");

	if(dir.empty())
		return false;

	command = "cd " + ftpQuote(Unicode::toOem(dir));
	if(!hConnect->ProcessCommand(command))
	{
		FtpGetFtpDirectory(hConnect);
		return true;
	}
	return false;
}

BOOL FtpRemoveDirectory(Connection *hConnect,CONSTSTR dir)
{
	std::string command;

	Assert( hConnect && "FtpRemoveDirectory" );

	if ( StrCmp(dir,".") == 0 ||
		StrCmp(dir,"..") == 0 )
		return true;

	hConnect->CacheReset();

	//Dir
	command = "rmdir " + ftpQuote(dir);
	if ( hConnect->ProcessCommand(command))
		return true;

	//Dir+slash
	command = "rmdir " + ftpQuote(dir + '/');
	if ( hConnect->ProcessCommand(command))
		return true;

	//Full dir
	command = "rmdir " + 
		ftpQuote(hConnect->toOEM(hConnect->curdir_) + '/' + (dir + (dir[0] == '/')));
	if(hConnect->ProcessCommand(command))
		return true;

	//Full dir+slash
	// TODO QUOTE " "
	command = "rmdir " + 
		ftpQuote(hConnect->toOEM(hConnect->curdir_) + '/' + (dir + (dir[0] == '/')) + '/');
	if(hConnect->ProcessCommand(command))
		return true;

	return FALSE;
}


BOOL FtpRenameFile(Connection *Connect, const std::wstring &existing, const std::wstring &New)
{
	std::string command;

	Assert( Connect && "FtpRenameFile" );

	Connect->CacheReset();
	command = "ren " + ftpQuote(Connect->toOEM(existing)) + ' ' + ftpQuote(Connect->toOEM(New));

	return Connect->ProcessCommand(command);
}


BOOL FtpDeleteFile(Connection *hConnect,CONSTSTR lpszFileName)
{  
	Assert( hConnect && "FtpDeleteFile" );

	hConnect->CacheReset();

	return hConnect->ProcessCommand(std::string("del ") + ftpQuote(lpszFileName));
}


std::string octal(int n)
{
	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%o", n);
	return buffer;
}

BOOL FtpChmod(Connection *hConnect,CONSTSTR lpszFileName,DWORD Mode)
{  
	Assert( hConnect && "FtpChmod" );
	hConnect->CacheReset();
	return hConnect->ProcessCommand(std::string("chmod ") + 
		octal(Mode) + " " + ftpQuote(lpszFileName));
}

BOOL FtpGetFile( Connection *Connect,CONSTSTR lpszRemoteFile,CONSTSTR lpszNewFile,BOOL Reget,int AsciiMode )
{  
	PROC(( "FtpGetFile","[%s]->[%s] %s %s",lpszRemoteFile,lpszNewFile,Reget?"REGET":"NEW",AsciiMode?"ASCII":"BIN" ));
	String full_name;
	int  ExitCode;

	Assert( Connect && "FtpGetFile" );

	//mode
	if ( AsciiMode && !Connect->ProcessCommand("ascii") ) {
		Log(( "!ascii ascii:%d",AsciiMode ));
		return FALSE;
	} else
		if ( !AsciiMode && !Connect->ProcessCommand("bin") ) {
			Log(( "!bin ascii:%d",AsciiMode ));
			return FALSE;
		}

		//Create directory
		boost::filesystem::path path = lpszNewFile;
		if(boost::filesystem::create_directories(path.branch_path()))
			return false;

		//Remote file
		if ( *lpszRemoteFile != '/' ) {
			full_name = Connect->toOEM(Connect->curdir_).c_str();
			AddEndSlash( full_name, '/' );
			full_name.Add( lpszRemoteFile );
			lpszRemoteFile = full_name.c_str();
		}

		//Get file
		Connect->IOCallback = TRUE;
		if ( Reget && !Connect->ResumeSupport ) {
			Connect->AddCmdLine( FMSG(MResumeRestart) );
			Reget = FALSE;
		}
		std::string command = 
			std::string(Reget? "reget " : "get ") + ftpQuote(lpszRemoteFile) + ' ';
			ftpQuote(lpszNewFile);
		ExitCode = Connect->ProcessCommand(command);
		Connect->IOCallback = false;

		return ExitCode;
}

__int64 FtpFileSize(Connection *Connect, const std::wstring &fnm)
{  
    if ( !Connect ) return -1;

	std::string command = "size " + ftpQuote(Connect->toOEM(fnm));

    if ( !Connect->ProcessCommand(command) )
	{
      Log(( "!size" ));
      return -1;
    } else {
		std::string line = Connect->GetReply();
		return Parser::parseUnsignedNumber(line.begin()+4, line.end(), 
									std::numeric_limits<__int64>::max(), false); 
    }
}

bool FtpPutFile(Connection *Connect, const std::wstring &localFile, std::wstring remoteFile, bool Reput, int AsciiMode)
{  
	int     ExitCode;
	__int64 Position;

	BOOST_ASSERT(Connect && "FtpPutFile");
	BOOST_ASSERT(!remoteFile.empty());

	Connect->CacheReset();
	if ( AsciiMode && !Connect->ProcessCommand("ascii") ||
		!AsciiMode && !Connect->ProcessCommand("bin")) {
			Log(( "!Set mode" ));
			return FALSE;
	}

	if (AsciiMode)
		Reput = FALSE;

	//Remote file
	if( (remoteFile.size() > 0 && remoteFile[0] == '\\') ||
		(remoteFile.size() > 1 && remoteFile[1] == ':'))
		remoteFile = getName(remoteFile);

	if (remoteFile[0] != '/' )
		remoteFile = Connect->curdir_ + L'/' + remoteFile;

	if(Reput)
	{
		Position = FtpFileSize(Connect, remoteFile);
		if ( Position == -1 )
			return FALSE;
	} else
		Position = 0;

	Connect->IOCallback = TRUE;

	//Append
	Connect->restart_point = Position;

	std::string loc = Connect->toOEM(localFile);
	std::string rem = Connect->toOEM(remoteFile);

	std::string command = std::string(Position? "appe " : "put ") +
		ftpQuote(loc) + ' ' + ftpQuote(rem);

	Log(( "%s upload", Position ? "Try APPE" : "Use PUT" ));
	ExitCode = Connect->ProcessCommand(command);

	//Error APPE, try to resume using REST
	if ( !ExitCode && Position && Connect->ResumeSupport ) {
		Log(( "APPE fail, try REST upload" ));
		Connect->restart_point = Position;
		command = "put " + ftpQuote(loc) + ' ' + ftpQuote(rem);
		ExitCode = Connect->ProcessCommand(command);
	}

	Connect->IOCallback = FALSE;

	return ExitCode;
}

std::wstring FtpSystemInfo(Connection *Connect)
{  
	Assert( Connect && "FtpSystemInfo" );

	if(!Connect->SystemInfoFilled)
	{
		FP_Screen _scr;
		Connect->SystemInfoFilled = TRUE;
		if (Connect->ProcessCommand("syst"))
		{
			std::string tmp = Connect->GetReply();
			size_t n;
			n = tmp.find_first_of("\r\n");
			if(n != std::string::npos)
				tmp.resize(n);


			if(tmp.size() > 3 && isdigit(tmp[0]) && isdigit(tmp[1]) && isdigit(tmp[2]))
				Connect->systemInfo_ = Unicode::fromOem(tmp.c_str()+4);
			else
				Connect->systemInfo_ = Unicode::fromOem(tmp);
		} else
		{
			Connect->systemInfo_ = L"";
		}
	}

	return Connect->systemInfo_;
}

bool FtpGetFtpDirectory(Connection *Connect)
{  
	std::wstring	dir;

	//Exec
	{  
		FP_Screen  _scr;
		if (!Connect->ProcessCommand("pwd"))
			return false;
	}

	// TODO FF
	std::wstring s = Unicode::fromOem(Connect->GetReply());

	Connect->curdir_ = Connect->Host.serverType_->parsePWD(s.begin(), s.end());
	if(Connect->curdir_ == L"")
	{
		// TODO wtf
		std::wstring::const_iterator itr = Connect->curdir_.begin();
		Parser::skipNumber(itr, Connect->curdir_.end());
		Connect->curdir_.assign(itr, Connect->curdir_.end()); 

		// TODO
		//Set classic path
		/* Unix:
		- name enclosed with '"'
		- if '"' is in name it doubles
		*/
/*
		itr = std::find(s.begin(), s.end(), '\"');
		if(itr != s.end())
		{
			++itr;
			std::wstring s1 = L"";
			std::wstring::const_iterator e = itr;
			while(true)
			{
				e = std::find(e, s.end(), '\"');
				if(e == s.end())
				{
					s1.append(itr, e);
					break;
				} else
				{
					if((e + 1) != s.end() && *(e+1) == L'\"')
					{
						s1.append(itr, e+1);
						e += 2;
						itr = e;
					} else
					{
						s1.append(itr, e);
						break;
					}
				}
			}
			Connect->curdir_ = s1;
		} else
*/
			Connect->curdir_ = s;
	}


	//Remove NL\CR
	size_t n = Connect->curdir_.find_first_of(L"\r\n");
	if(n != std::wstring::npos)
		Connect->curdir_.resize(n);
	return true;
}

//------------------------------------------------------------------------
void BadFormat( Connection *Connect,CONSTSTR Line,BOOL inParce )
{
    Connect->AddCmdLine( Line );
    FtpConnectMessage( Connect, MNone__,
                       inParce
                         ? "Error parsing files list. Please read \"BugReport_*.txt\" and report to developer."
                         : "Can not find listing parser. Please read \"BugReport_*.txt\" and report to developer.",
                       -MOk );
}

boost::shared_ptr<ServerType> FTP::SelectServerType(boost::shared_ptr<ServerType> defType)
{  
	std::vector<FarMenuItem> MenuItems;

	ServerList &list = ServerList::instance();
	MenuItems.reserve(list.size());

	ServerList::const_iterator itr = list.begin();

	std::vector<std::string> vec;

	class MaxNameSize: public std::binary_function<ServerTypePtr, ServerTypePtr, bool>
	{
	public:
		bool operator()(const ServerTypePtr &l, ServerTypePtr &r)
		{
			return l->getName().size() < r->getName().size();
		}
	};

	size_t maxNameLengh = std::max_element(list.begin(), list.end(), MaxNameSize())
		->get()->getName().size();
	
	while(itr != list.end()) 
	{
		FarMenuItem item;
		std::string name = Unicode::utf16ToUtf8((*itr)->getName());
		size_t nspaces = maxNameLengh - name.size();
		if(nspaces != 0)
		{
			std::string spaces;
			spaces.insert(spaces.begin(), nspaces, ' ');
			name += spaces;
		}
		name += FAR_VERT_CHAR + Unicode::utf16ToUtf8((*itr)->getDescription());
		Utils::safe_strcpy(item.Text, name.c_str());
		item.Checked	= false;
		item.Selected	= ((*itr)->getName() == defType->getName())? true : false;
		item.Separator	= false;
		MenuItems.push_back(item);
		++itr;
	}

	int rc = FP_Info->Menu(FP_Info->ModuleNumber, -1, -1, 0, FMENU_AUTOHIGHLIGHT,
		FP_GetMsg(MTableTitle), NULL,NULL,NULL,NULL, &MenuItems[0], (int) MenuItems.size());

	return (rc == -1)? defType : list[rc];
}

bool ParseDirLine(Connection *Connect, BOOL AllFiles, FTPFileInfo* p)
{  
	PROC(( "ParseDirLine", "%p,%d", Connect, AllFiles ))

	std::wstring::const_iterator itr = Connect->output_.begin();
	std::wstring::const_iterator itr_end = Connect->output_.end();
	ServerType::entry entry;

	ServerTypePtr server = Connect->Host.serverType_;
	if(ServerList::isAutodetect(server))
	{
		server = ServerList::autodetect(Connect->systemInfo_);
	}

	while(1)
	{
		if(itr == itr_end)
			return true;

		entry = server->getEntry(itr, itr_end);
		if(entry.first == entry.second)
			continue;

		if(std::wstring(entry.first, entry.second).find(L": Permission denied") != std::wstring::npos)
		{
			SetLastError(ERROR_ACCESS_DENIED);
			return false;
		}

		if(!server->isValidEntry(entry.first, entry.second))
			continue;

		Log(( "toParse: [%s]", Unicode::utf16ToUtf8(std::wstring(entry.first, entry.second)).c_str()));
		FTPFileInfo fileinfo;
		try
		{
			server->parseListingEntry(entry.first, entry.second, fileinfo);
			// updateTime(fileinfo);
		}
		catch (std::exception &e)
		{
			Connect->AddCmdLine( 
				std::wstring(L"ParserFAIL: (") + server->getName() + L") [" + 
				std::wstring(entry.first, entry.second) + L"]:" + Unicode::utf8ToUtf16(e.what()), ldOut);
			continue;
		}

		BOOST_ASSERT(fileinfo.getType() != FTPFileInfo::undefined);

		std::wstring filename = fileinfo.getFileName();
		if(filename.empty() || filename == L"." || (!AllFiles && filename == L"..")) // TODO
			continue;

			//Correct attrs TODO
/*
			if ( p->FileType == NET_DIRECTORY ||
				p->FileType == NET_SYM_LINK_TO_DIR )
				SET_FLAG( p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY );

			if ( p->FileType == NET_SYM_LINK_TO_DIR ||
				p->FileType == NET_SYM_LINK )
				SET_FLAG( p->FindData.dwFileAttributes,FILE_ATTRIBUTE_REPARSE_POINT );
*/
	}

	SetLastError(ERROR_NO_MORE_FILES);
	return false;
}