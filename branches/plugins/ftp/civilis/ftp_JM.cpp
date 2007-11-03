#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

/*  1 -       title
    2 -       --- betweeen commands and status
    3 - [OPT] status
    4 - [OPT] --- between status and buttons
              commands
    5 - [OPT] --- betweeen commands and status
    6 - [OPT] button
    7 - [OPT] button1
    8 - [OPT] button2
    9 - [OPT] button Cancel
*/
const int NBUTTONSADDON = 10;

const wchar_t* horizontalLine = L"\x1";

std::wstring WINAPI FixFileNameChars(std::wstring s, bool  slashes)
{  
	if(g_manager.opt.InvalidSymbols.empty()) 
		return s;
	std::wstring::const_iterator old_itr = g_manager.opt.InvalidSymbols.begin();
	std::wstring::const_iterator new_itr = g_manager.opt.CorrectedSymbols.begin();

	while(old_itr != g_manager.opt.InvalidSymbols.end())
	{
		std::replace(s.begin(), s.end(), *old_itr, *new_itr);
		++old_itr;
	}
	if(slashes)
	{
		std::replace(s.begin(), s.end(), L'\\', L'_');
		std::replace(s.begin(), s.end(), L':', L'!');
	}
	return s;
}


//------------------------------------------------------------------------
/*
   Create Socket errr string.
   Returns static buffer

   Windows has no way to create string by socket error, so make it inside plugin
*/
static struct {
  int   Code;
  int   MCode;
} stdSockErrors[] = {
/* Windows Sockets definitions of regular Microsoft C error constants */
       { WSAEINTR,            MWSAEINTR },
       { WSAEBADF,            MWSAEBADF },
       { WSAEACCES,           MWSAEACCES },
       { WSAEFAULT,           MWSAEFAULT },
       { WSAEINVAL,           MWSAEINVAL },
       { WSAEMFILE,           MWSAEMFILE },
/* Windows Sockets definitions of regular Berkeley error constants */
       { WSAEWOULDBLOCK,      MWSAEWOULDBLOCK },
       { WSAEINPROGRESS,      MWSAEINPROGRESS },
       { WSAEALREADY,         MWSAEALREADY },
       { WSAENOTSOCK,         MWSAENOTSOCK },
       { WSAEDESTADDRREQ,     MWSAEDESTADDRREQ },
       { WSAEMSGSIZE,         MWSAEMSGSIZE },
       { WSAEPROTOTYPE,       MWSAEPROTOTYPE },
       { WSAENOPROTOOPT,      MWSAENOPROTOOPT },
       { WSAEPROTONOSUPPORT,  MWSAEPROTONOSUPPORT },
       { WSAESOCKTNOSUPPORT,  MWSAESOCKTNOSUPPORT },
       { WSAEOPNOTSUPP,       MWSAEOPNOTSUPP },
       { WSAEPFNOSUPPORT,     MWSAEPFNOSUPPORT },
       { WSAEAFNOSUPPORT,     MWSAEAFNOSUPPORT },
       { WSAEADDRINUSE,       MWSAEADDRINUSE },
       { WSAEADDRNOTAVAIL,    MWSAEADDRNOTAVAIL },
       { WSAENETDOWN,         MWSAENETDOWN },
       { WSAENETUNREACH,      MWSAENETUNREACH },
       { WSAENETRESET,        MWSAENETRESET },
       { WSAECONNABORTED,     MWSAECONNABORTED },
       { WSAECONNRESET,       MWSAECONNRESET },
       { WSAENOBUFS,          MWSAENOBUFS },
       { WSAEISCONN,          MWSAEISCONN },
       { WSAENOTCONN,         MWSAENOTCONN },
       { WSAESHUTDOWN,        MWSAESHUTDOWN },
       { WSAETOOMANYREFS,     MWSAETOOMANYREFS },
       { WSAETIMEDOUT,        MWSAETIMEDOUT },
       { WSAECONNREFUSED,     MWSAECONNREFUSED },
       { WSAELOOP,            MWSAELOOP },
       { WSAENAMETOOLONG,     MWSAENAMETOOLONG },
       { WSAEHOSTDOWN,        MWSAEHOSTDOWN },
       { WSAEHOSTUNREACH,     MWSAEHOSTUNREACH },
       { WSAENOTEMPTY,        MWSAENOTEMPTY },
       { WSAEPROCLIM,         MWSAEPROCLIM },
       { WSAEUSERS,           MWSAEUSERS },
       { WSAEDQUOT,           MWSAEDQUOT },
       { WSAESTALE,           MWSAESTALE },
       { WSAEREMOTE,          MWSAEREMOTE },
/* Extended Windows Sockets error constant definitions */
       { WSASYSNOTREADY,      MWSASYSNOTREADY },
       { WSAVERNOTSUPPORTED,  MWSAVERNOTSUPPORTED },
       { WSANOTINITIALISED,   MWSANOTINITIALISED },
       { WSAEDISCON,          MWSAEDISCON },
       { WSAHOST_NOT_FOUND,   MWSAHOST_NOT_FOUND },
       { WSATRY_AGAIN,        MWSATRY_AGAIN },
       { WSANO_RECOVERY,      MWSANO_RECOVERY },
       { WSANO_DATA,          MWSANO_DATA },
       { WSANO_ADDRESS,       MWSANO_ADDRESS },
       { 0, MNone__ }
};

const std::wstring GetSocketErrorSTR()
{
	return GetSocketErrorSTR(WSAGetLastError());
}

const std::wstring GetSocketErrorSTR(int err)
{  
	if(!err)
		return getMsg(MWSAENoError);

	for(int n = 0; stdSockErrors[n].MCode != MNone__; n++)
		if ( stdSockErrors[n].Code == err)
			return getMsg(stdSockErrors[n].MCode);

	return getMsg(MWSAEUnknown) + boost::lexical_cast<std::wstring>(err);
}

/*
    Show `Message` with attention caption and query user to select YES or NO
    Returns nonzero if user select YES
*/
bool WINAPI IsCmdLogFile(void)
{
	return !g_manager.opt.CmdLogFile.empty();
}

std::wstring WINAPI GetCmdLogFile()
{  
	static std::wstring str;
	HMODULE m;

	if(!g_manager.opt.CmdLogFile.empty())
	{
		if(g_manager.opt.CmdLogFile.size() > 1 && g_manager.opt.CmdLogFile[1] == L':')
			return g_manager.opt.CmdLogFile;
		else
			if(!str.empty())
				return str;
			else
			{
				wchar_t buffer[_MAX_PATH+1] = {0};
				m = GetModuleHandleW(L"ftp.dll");
				buffer[GetModuleFileNameW(m, buffer, sizeof(buffer)/sizeof(*buffer))] = 0;
				str = buffer;
				size_t n = str.rfind('\\');
				str.resize(n+1);
				str += g_manager.opt.CmdLogFile;
				return str;
			}
	} else
		return L"";
}
/*
   Writes one string to log file

   Start from end if file at open bigger then limit size
   `out` is a direction of string (server, plugin, internal)
*/

static HANDLE LogFile = NULL;

void WINAPI LogCmd(const wchar_t* src,CMDOutputDir out, DWORD Size)
{
//File opened and fail
    if ( LogFile == INVALID_HANDLE_VALUE )
      return;

//Params
    if ( !IsCmdLogFile() || !src )
      return;

    if ( out == ldRaw && (!Size || Size == UINT_MAX) )
      return;
     else
    if ( !src[0] )
      return;

//Open file
    //Name
	std::wstring m = GetCmdLogFile();
    if(m.empty())
      return;

    //Open
    LogFile = Fopen(m.c_str(), !LogFile && g_manager.opt.TruncateLogFile ? L"w" : L"w+" );
    if ( !LogFile ) {
      LogFile = INVALID_HANDLE_VALUE;
      return;
    }

    //Check limitations
    if ( g_manager.opt.CmdLogLimit &&
         Fsize(LogFile) >= (__int64)g_manager.opt.CmdLogLimit*1000 )
      Ftrunc(LogFile,FILE_BEGIN);

//-- USED DATA
    static SYSTEMTIME stOld = { 0 };

    SYSTEMTIME st;
    char       tmstr[ 100 ];

//-- RAW
    if ( out == ldRaw ) {
//      SNprintf( tmstr, sizeof(tmstr), "%d ", PluginUsed() );
      Fwrite( LogFile,tmstr,strlen(tmstr) );

      signed n;
      for( n = ((int)Size)-1; n > 0 && strchr( "\n\r",src[n]); n-- );
      if ( n > 0 ) {
        Fwrite( LogFile,"--- RAW ---\r\n",13 );
        Fwrite( LogFile,src,Size );
        Fwrite( LogFile,"\r\n--- RAW ---\r\n",15 );
      }
      Fclose(LogFile);
      return;
    }

//-- TEXT

//Replace PASW
    if ( !g_manager.opt._ShowPassword && wcscmp(src, L"PASS ") == 0)
		src = L"PASS *hidden*";

//Write multiline to log
    do{
       //Plugin
//       SNprintf( tmstr, sizeof(tmstr), "%d ", PluginUsed() );
       Fwrite( LogFile,tmstr,strlen(tmstr) );

       //Time
       GetLocalTime( &st );
       _snprintf( tmstr,sizeof(tmstr),
                 "%4d.%02d.%02d %02d:%02d:%02d:%04d",
                 st.wYear, st.wMonth,  st.wDay,
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
       Fwrite( LogFile,tmstr,strlen(tmstr) );

       //Delay
       if ( !stOld.wYear )
          sprintf(tmstr, " ----");
         else
          sprintf(tmstr, " %04d",
                   (st.wSecond-stOld.wSecond)*1000 + (st.wMilliseconds-stOld.wMilliseconds) );
       Fwrite( LogFile,tmstr,strlen(tmstr) );

       stOld = st;

       //Direction
       if ( out == ldInt )
         Fwrite( LogFile,"| ",2 );
        else
         Fwrite( LogFile,(out == ldOut) ? "|->" : "|<-",3 );

       //Message
       for ( ; *src && !strchr("\r\n",*src); src++ )
         Fwrite( LogFile,src,1 );
       Fwrite( LogFile,"\n",1 );

       while( *src && strchr("\r\n",*src) ) src++;
    }while( *src );

    Fclose( LogFile );
}

void Connection::InitIOBuff( void )
{
    IOBuff.reset(new char[getHost().IOBuffSize+1]);
}

/*
   Initialize CMD buffer data
*/
void Connection::InitCmdBuff()
{
	if(!cmdBuff_.empty())
		CloseCmdBuff();

	CmdVisible		= TRUE;
	retryCount_		= 0;
	//Command lines + status command
	cmdMsg_.reserve(g_manager.opt.CmdCount + NBUTTONSADDON);
}
/*
   Free initialize CMD buffer data
*/
void Connection::CloseCmdBuff( void )
{
	cmdBuff_.resize(0);
	cmdMsg_.resize(0);
}
//------------------------------------------------------------------------
/*
   Output message about internal plugin error
*/
void Connection::InternalError( void )
{
	FARWrappers::message(MFtpTitle, MIntError);
}

void Connection::pushCmdLine(const std::wstring &src, CMDOutputDir direction)
{
	std::wstring result = src;

	if(!g_manager.opt._ShowPassword && g_manager.opt.cmdPass_ == src)
		result = L"PASS *hidden*";
	else
		if(!src.empty() && src[0] == cffDM &&
			src.find(L"ABOR", 1, 4) != std::wstring::npos)
			result = L"<ABORT>";

	switch(direction)
	{
	case ldInt:
		result = L"<- " + src;
		break;
	case ldOut:
		result = L"-> " + src;
	}

	if(direction == ldRaw)
	{
		size_t n, start = 0;
		std::wstring s;
		do 
		{
 			n = result.find_first_of(L"\n\r", start);
			s = std::wstring(result, start, (n == std::wstring::npos)? n : n-start);
			size_t ofs = 0;
			do 
			{
				cmdBuff_.push_back(std::wstring(s, ofs, g_manager.opt.CmdLine-4));
				ofs += g_manager.opt.CmdLine;
			} while(ofs < s.size());
			if(n == std::wstring::npos)
				break;
			start = result.find_first_not_of(L"\n\r", n+1);
		} while(start != std::wstring::npos);
	} else
	{
		size_t n = std::min(result.find_first_of(L"\n\r"),
							static_cast<size_t>(g_manager.opt.CmdLine-4));
		n = std::min(result.size(), n);
		result.resize(n);
		cmdBuff_.push_back(result);
	}
}


void Connection::AddCmdLine(const std::wstring &str, CMDOutputDir direction)
{
	std::wstring buff;

	std::wstring::const_iterator itr = str.begin();
	if(str.find_first_not_of(L"\r\n") == std::wstring::npos)
		return;

	buff = str; // TODO replyString_;

	const size_t cmdSize = g_manager.opt.CmdCount;
	//Remove overflow lines
	if(cmdBuff_.size() >= cmdSize)
		cmdBuff_.erase(cmdBuff_.begin(), cmdBuff_.begin() + (cmdBuff_.size()-cmdSize+1));

	//Add new line
	LogCmd(buff.c_str(), direction);

	pushCmdLine(buff, direction);
	if(cmdBuff_.size() > cmdSize)
		cmdBuff_.erase(cmdBuff_.begin(), cmdBuff_.begin() + (cmdBuff_.size()-cmdSize));

	//Start
	if(startReply_.empty() && direction == ldIn)
	{
		size_t start = str.find_first_not_of(L"0123456789 -");
		if(start == std::wstring::npos)
			start = 0;
		size_t end = str.find_first_of(L"\n\r", start);
		if(end == std::wstring::npos)
			end = str.size();
		startReply_.assign(str, start, end-start);
	}

	ConnectMessage();
}

/*
   Add a string to `RPL buffer` and `CMD buffer`

   Do not accept empty strings
   Scrolls buffers up if length of buffers bigger then CMD length
   Call ConnectMessage to refresh CMD window

   IF `str` == NULL
     Place to buffer last response string
*/
void Connection::AddCmdLine(const std::string& str, CMDOutputDir direction)
{

	char ffIacIp[] = {cffIAC, cffIP, 0};
	if(boost::starts_with(str, ffIacIp))
		return;

	AddCmdLine(FromOEM(str), direction);
}


int Connection::ConnectMessage(std::wstring message, bool messageTime,
								const std::wstring &title, const std::wstring &bottom, 
								bool showcmds, bool error,
								int button1, int button2, int button3)
{
	cmdMsg_.resize(0);
	cmdMsg_.push_back(title);

	if(messageTime)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		std::wstringstream stream;
		stream << std::setfill(L'0') << std::setw(2) << st.wHour << L':' << 
			std::setw(2) << st.wMinute << L':' << std::setw(2) << st.wSecond <<
			L" \"" << message << + "\"";
		message = stream.str();
	};
	message.resize(g_manager.opt.CmdLine-4, L' ');
	cmdMsg_.push_back(message);

	if(showcmds)
	{
		cmdMsg_.push_back(horizontalLine);

		std::deque<std::wstring>::const_iterator itr = cmdBuff_.begin();
		while(itr != cmdBuff_.end())
		{
			cmdMsg_.push_back(*itr);
			++itr;
		}
		for(size_t n = cmdBuff_.size(); n < static_cast<size_t>(g_manager.opt.CmdCount); ++n)
			cmdMsg_.push_back(L"");
	}

	if(!bottom.empty())
	{
		cmdMsg_.push_back(horizontalLine);
		cmdMsg_.push_back(bottom);
	}

	int buttonCount = 0;
	if(button1 != MNone__)
	{
		cmdMsg_.push_back(horizontalLine);
		cmdMsg_.push_back(getMsg(button1));
		++buttonCount;
		if(button2 != MNone__)
		{
			cmdMsg_.push_back(getMsg(button2));
			++buttonCount;
			if(button3 != MNone__)
			{
				cmdMsg_.push_back(getMsg(button3));
				++buttonCount;
			}
		}
	}

	FARWrappers::Screen scr;
	return FARWrappers::message(cmdMsg_, buttonCount, (error? FMSG_WARNING : 0) | FMSG_LEFTALIGN);	
}

bool Connection::ConnectMessageTimeout(int Msg, const std::wstring& hostname, int BtnMsg)
{  
	bool	first = true;
	int		secNum;

	if (is_flag(FP_LastOpMode, OPM_FIND) || !g_manager.opt.RetryTimeout)
		return ConnectMessage(Msg, hostname, BtnMsg);

	secNum = 0;
	WinAPI::Stopwatch stopwatch(1000);
	do
	{
		static const WORD Keys[] = { VK_ESCAPE, 'C', 'R', VK_RETURN };

		switch(WinAPI::checkForKeyPressed(Keys))
		{
		 case 0: case 1:
			 return false;
		 case 2:
		 case 3:
			 return true;
		}

		if(first || stopwatch.isTimeout())
		{
			first = false;
			std::wstring s = std::wstring(L" ") + getMsg(MAutoRetryText) + L" " +
				boost::lexical_cast<std::wstring>(g_manager.opt.RetryTimeout-secNum) +
				getMsg(MSeconds);

			ConnectMessage(std::wstring(getMsg(Msg)) + s, false, hostname, 
						getMsg(MRetryText), true, true);

			WinAPI::setConsoleTitle(std::wstring(getMsg(Msg)) + s);

			secNum++;
			stopwatch.reset();
		}

		if(secNum > g_manager.opt.RetryTimeout)
			return false;

		Sleep(100);
	}while( 1 );

	return false;
}

int Connection::ConnectMessage(std::wstring msg, std::wstring subtitle, bool error,
							   int btn, int btn1, int btn2)
{
	PROCP(msg << L", " << subtitle << L", " << btn << L", " << btn1 << L", " << btn2); 
	int			res;
	bool		exCmd;

	if(error && g_manager.opt.DoNotExpandErrors)
		exCmd = false;
	else
		exCmd = getHost().ExtCmdView;

	//Called for update CMD window but it disabled
	if(msg.empty() && subtitle.empty() && !exCmd)
		return true;

	if(subtitle.empty())
	{
		if(!userName_.empty())
		{
			subtitle = userName_ + L':';
			if (!userPassword_.empty())
				subtitle += L"*@";
		}
		subtitle += getHost().url_.Host_;
	}

	if(btn == MNone__)
		if ( is_flag(FP_LastOpMode,OPM_FIND)                       ||  //called from find
			!CmdVisible                                           ||  //Window disabled
			(IS_SILENT(FP_LastOpMode) && !g_manager.opt.ShowSilentProgress) )
		{ //show silent processing disabled
			if(!msg.empty())
				IdleMessage(msg.c_str(), g_manager.opt.ProcessColor );
			return false;
		}

	//Title
	std::wstring title = getMsg(MFtpTitle) + std::wstring(L" \"") + subtitle + L'\"';

	if(!msg.empty())
		lastMsg_ = msg;

	//Display error in title
	if(error && FARWrappers::Screen::isSaved())
	{
		WinAPI::setConsoleTitle(lastMsg_ + L" \"" + title + L"\"");
	}

	res = ConnectMessage(lastMsg_, true, title, L"", exCmd, error, btn, btn1, btn2);

//SLEEP!!	Sleep(1000);

	//If has user buttons: return number of pressed button
	if(btn1 != MNone__)
		return res;

	//If single button: return TRUE if button was selected
	return btn != MNone__ && res == 0;
}


int Connection::ConnectMessage(int Msg, std::wstring subtitle, bool error,
							   int btn, int btn1, int btn2)
{
	return ConnectMessage(Msg == MNone__? L"" : getMsg(Msg), subtitle, error, btn, btn1, btn2);
}

void WINAPI OperateHidden(const std::wstring& fnm, bool set)
{
	if(!g_manager.opt.SetHiddenOnAbort)
		return;
	BOOST_LOG(INF, (set ? L"Set" : L"Clr") << L" hidden");

	DWORD dw = GetFileAttributesW(fnm.c_str());
	if(dw == INVALID_FILE_ATTRIBUTES)
		return;

	set_flag(dw, FILE_ATTRIBUTE_HIDDEN, set);

	SetFileAttributesW(fnm.c_str(), dw);
}


