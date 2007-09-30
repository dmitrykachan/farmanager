#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "utils/strutils.h"
#include "utils/uniconverts.h"
#include "servertype.h"

#include "farwrapper/menu.h"

const char quote = '\x1';

std::string ftpQuote(const std::string& str)
{
	return quote + str + quote;
}

std::string ftpQuote(const std::wstring& str)
{
	return quote + Unicode::toOem(str) + quote;
}

bool FtpIsResume(Connection *hConnect)
{
	BOOST_ASSERT(hConnect);
	return hConnect->resumeSupport_;
}

void FtpSetRetryCount( Connection *hConnect,int cn )
{
	BOOST_ASSERT(hConnect && "FtpSetRetryCount");

    hConnect->RetryCount = cn;
}

bool Connection::getBreakable()
{
	return breakable_;
}

void Connection::setBreakable(bool value)
{
	breakable_ = value;
}

int FtpGetRetryCount( Connection *hConnect )
{
	BOOST_ASSERT(hConnect && "FtpGetRetryCount");

	return hConnect->RetryCount;
}

int FtpConnectMessage( Connection *hConnect,int Msg, const wchar_t* HostName,int btn /*= MNone__*/,int btn1 /*= MNone__*/,int btn2 /*= MNone__*/ )
{
	return hConnect ? hConnect->ConnectMessage(Msg,HostName,btn,btn1,btn2) : FALSE;
}

int FtpCmdBlock(Connection *hConnect, int block)
{
	int rc = -1;

	if (!hConnect)
		return -1;

	rc = hConnect->CmdVisible == FALSE;

	if (block != -1)
		hConnect->CmdVisible = block == FALSE;
	return rc;
}

bool FtpFindFirstFile(Connection *hConnect, const std::wstring &searchFile, FTPFileInfo* lpFindFileData, bool *ResetCache)
{
	BOOST_ASSERT(hConnect && "FtpFindFirstFile");
	int    AllFiles = (searchFile == L"*" || searchFile == L"*.*");

	if(!AllFiles)
	{
		std::wstring dir = getName(searchFile);
		if (AllFiles && !IS_SILENT(FP_LastOpMode) &&
			hConnect->CmdVisible &&
			hConnect->CurrentState != fcsExpandList )
			hConnect->ConnectMessage( MRequestingFolder, hConnect->getCurrentDirectory());

		if(!hConnect->ls(dir))
		{
			SetLastError(hConnect->ErrorCode);
			return NULL;
		}
	}
	return 0;
}


bool FtpSetCurrentDirectory(Connection *hConnect, const std::wstring& dir)
{  
	BOOST_ASSERT(hConnect && "FtpSetCurrentDirectory");

	if(dir.empty())
		return false;

	if(hConnect->cd(dir))
	{
		FtpGetFtpDirectory(hConnect);
		return true;
	}
	return false;
}

bool FtpRemoveDirectory(Connection *hConnect, const std::wstring &dir)
{
	BOOST_ASSERT( hConnect && "FtpRemoveDirectory" );

	if(dir == L"." || dir == L".." || dir.empty())
		return true;

	// TODO hConnect->CacheReset();

	//Dir
	if(hConnect->removedir(dir) != RPL_OK)
		return true;

	//Dir+slash
	if(hConnect->removedir(dir + L'/') != RPL_OK)
		return true;

	//Full dir
	std::wstring fulldir = hConnect->getCurrentDirectory() + L'/';
	fulldir.append(dir, dir[0] == L'/', dir.size());
	if(hConnect->removedir(fulldir) != RPL_OK)
		return true;

	//Full dir+slash
	fulldir += L'/';
	if(hConnect->removedir(fulldir) != RPL_OK)
		return true;

	return false;
}


BOOL FtpRenameFile(Connection *Connect, const std::wstring &existing, const std::wstring &New)
{
	BOOST_ASSERT( Connect && "FtpRenameFile" );

	// TODO Connect->CacheReset();
	return Connect->renamefile(existing, New) == RPL_OK;
}


bool FtpDeleteFile(Connection *hConnect, const std::wstring& filename)
{  
	BOOST_ASSERT( hConnect && "FtpDeleteFile" );

	// TODO hConnect->CacheReset();

	return hConnect->deleteFile(filename) != 0;
}


std::wstring octal(int n)
{
	wchar_t buffer[16];
	_snwprintf(buffer, sizeof(buffer)/sizeof(*buffer), L"%o", n);
	return buffer;
}

bool FtpChmod(Connection *hConnect, const std::wstring &filename, DWORD Mode)
{  
	BOOST_ASSERT( hConnect && "FtpChmod" );
	// TODO hConnect->CacheReset();
	return hConnect->chmod(filename, octal(Mode)) == RPL_OK;
}

bool FtpGetFile(Connection *Connect, const std::wstring& RemoteFile, const std::wstring& newFile, bool reget, int asciiMode)
{
	PROCP(L"[" << RemoteFile << L"]->[" << newFile << L"] " << (reget? L"REGET" : L"NEW") << (asciiMode? L"ASCII" : L"BIN"));
	int  ExitCode;
	std::wstring	remoteFile = RemoteFile;

	BOOST_ASSERT( Connect && "FtpGetFile" );

	//mode
	if(asciiMode && Connect->setascii() != RPL_OK)
	{
		BOOST_LOG(INF, L"!ascii ascii:" << asciiMode);
		return false;
	} else
		if(!asciiMode && Connect->setbinary()!= RPL_OK)
		{
			BOOST_LOG(INF, L"!bin ascii:" << asciiMode);
			return false;
		}

	//Create directory
	boost::filesystem::wpath path = newFile;
	if(boost::filesystem::create_directories(path.branch_path()))
		return false;

	//Remote file
	if(!remoteFile.empty() && remoteFile[0] != L'/')
	{
		std::wstring full_name;
		full_name = Connect->getCurrentDirectory();
		AddEndSlash(full_name, '/');
		full_name += remoteFile;
		remoteFile = full_name + remoteFile;
	}

	//Get file
	Connect->IOCallback = true;
	if (reget && !Connect->resumeSupport_)
	{
		Connect->AddCmdLine(getMsg(MResumeRestart), ldRaw);
		reget = false;
	}
	if(reget)
		ExitCode = Connect->reget(remoteFile, newFile);
	else
		ExitCode = Connect->get(remoteFile, newFile);
	Connect->IOCallback = false;

	return ExitCode != 0;
}

__int64 FtpFileSize(Connection *Connect, const std::wstring &fnm)
{
	if(!Connect)
		return -1;

	if(!Connect->sizecmd(fnm) != RPL_OK)
	{
		BOOST_LOG(INF, L"!size");
		return -1;
	} else
	{
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

	// TOOD Connect->CacheReset();
	if ( AsciiMode && Connect->setascii() != RPL_OK ||
		!AsciiMode && Connect->setbinary() != RPL_OK)
	{
			BOOST_LOG(INF, L"!Set mode");
			return FALSE;
	}

	if (AsciiMode)
		Reput = FALSE;

	//Remote file
	if( (remoteFile.size() > 0 && remoteFile[0] == '\\') ||
		(remoteFile.size() > 1 && remoteFile[1] == ':'))
		remoteFile = getName(remoteFile);

	if (remoteFile[0] != '/' )
		remoteFile = Connect->getCurrentDirectory() + L'/' + remoteFile;

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

	BOOST_LOG(INF, (Position ? L"Try APPE" : L"Use PUT") << L"upload");
	if(Position)
		ExitCode = Connect->appended(localFile, remoteFile);
	else
		ExitCode = Connect->put(localFile, remoteFile);

	//Error APPE, try to resume using REST
	if(ExitCode != RPL_OK && Position && Connect->resumeSupport_)
	{
		BOOST_LOG(INF, L"APPE fail, try REST upload");
		Connect->restart_point = Position;
		ExitCode = Connect->put(localFile, remoteFile);
	}

	Connect->IOCallback = FALSE;

	return ExitCode != 0;
}

void FtpSystemInfo(Connection *Connect)
{  
	BOOST_ASSERT( Connect && "FtpSystemInfo" );

	if(Connect->getSystemInfo().empty())
	{
		FARWrappers::Screen scr;
		if(Connect->syst())
		{
			std::string tmp = Connect->GetReply();
			size_t n;
			n = tmp.find_first_of("\r\n");
			if(n != std::string::npos)
				tmp.resize(n);


			if(tmp.size() > 3 && isdigit(tmp[0]) && isdigit(tmp[1]) && isdigit(tmp[2]))
				Connect->setSystemInfo(Connect->FromOEM(std::string(tmp.begin()+4, tmp.end())));
			else
				Connect->setSystemInfo(Connect->FromOEM(tmp));
		}
	}
}

static std::wstring unDupFF(const Connection *connect, std::string str)
{
	size_t n = str.find("\xFF\xFF");
	while(n != std::string::npos)
	{
		str.replace(n, 2, 1, '\xFF');
		n = str.find("\xFF\xFF", n);
	}
	return connect->FromOEM(str);
}

bool FtpGetFtpDirectory(Connection *Connect)
{  
	std::wstring	dir;

	FARWrappers::Screen scr;
	if(!Connect->pwd())
		return false;

	std::wstring s = unDupFF(Connect, Connect->GetReply());
	std::wstring::const_iterator itr = s.begin();
	Parser::skipNumber(itr, s.end());

	Connect->curdir_ = Connect->getHost().serverType_->parsePWD(itr, s.end());
	return !Connect->curdir_.empty();
}

boost::shared_ptr<ServerType> FTP::SelectServerType(boost::shared_ptr<ServerType> defType)
{
	FARWrappers::Menu menu(FMENU_AUTOHIGHLIGHT);

	ServerList &list = ServerList::instance();
	menu.reserve(list.size());
	menu.setTitle(getMsg(MTableTitle));

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
		std::wstring name = (*itr)->getName();
		size_t nspaces = maxNameLengh - name.size();
		name += std::wstring(nspaces, L' ');
		name += FAR_VERT_CHAR + (*itr)->getDescription();
		menu.addItem(name);
		++itr;
	}
	BOOST_ASSERT(list.find(defType->getName()) != list.end());
	menu.select(list.find(defType->getName()) - list.begin());
	int rc = menu.show();

	return (rc == -1)? defType : list[rc];
}


const std::wstring& Connection::getCurrentDirectory() const
{
	BOOST_ASSERT(!curdir_.empty());
	return curdir_;
}

bool Connection::keepAlive()
{
	if(cmdLineAlive())
	{
		pwd();
		return true;
	}
	return false;
}