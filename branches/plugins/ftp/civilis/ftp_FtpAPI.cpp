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

bool Connection::getBreakable()
{
	return breakable_;
}

void Connection::setBreakable(bool value)
{
	breakable_ = value;
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


std::wstring octal(int n)
{
	wchar_t buffer[16];
	_snwprintf(buffer, sizeof(buffer)/sizeof(*buffer), L"%o", n);
	return buffer;
}

__int64 FtpFileSize(Connection *Connect, const std::wstring &fnm)
{
	if(!Connect)
		return -1;

	if(!Connect->sizecmd(fnm))
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

	Connect->IOCallback = true;

	//Append
	Connect->restart_point = Position;

	BOOST_LOG(INF, (Position ? L"Try APPE" : L"Use PUT") << L"upload");
	if(Position)
		ExitCode = Connect->appended(localFile, remoteFile);
	else
		ExitCode = Connect->put(localFile, remoteFile);

	//Error APPE, try to resume using REST
	if(ExitCode != RPL_OK && Position && Connect->isResumeSupport())
{
		BOOST_LOG(INF, L"APPE fail, try REST upload");
		Connect->restart_point = Position;
		ExitCode = Connect->put(localFile, remoteFile);
	}

	Connect->IOCallback = false;

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