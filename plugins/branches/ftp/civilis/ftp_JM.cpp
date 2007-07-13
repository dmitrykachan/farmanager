#include "stdafx.h"
#include <all_far.h>
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
#define NBUTTONSADDON 10


const wchar_t* ftp_sig	= L"ftp://";
const wchar_t* http_sig	= L"http://";
const wchar_t* ftp_sig_end = ftp_sig + 6;
const wchar_t* http_sig_end = http_sig + 7;


//------------------------------------------------------------------------
// DEBUG ONLY
//------------------------------------------------------------------------
#if defined(__FILELOG__)
void LogPanelItems(struct PluginPanelItem *pi, size_t cn)
{
	Log(( "Items in list %p: %d", pi, cn ));
	if(pi)
		for(size_t n = 0; n < cn; n++ )
		{
			Log(( "%2d) [%s] attr: %08X (%s)",
				n+1, Unicode::toOem(FTP_FILENAME( &pi[n] )).c_str(),
				pi[n].FindData.dwFileAttributes, IS_FLAG(pi[n].FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)?"DIR":"FILE"
				));
		}
}
#endif

//------------------------------------------------------------------------
int FARINProc::Counter = 0;

FARINProc::FARINProc( CONSTSTR nm,CONSTSTR s,... )
    : Name(nm)
  {  va_list  ap;
     char     str[500];
     DWORD    err = GetLastError();

     if ( s ) {
       va_start( ap,s );
       SNprintf( str, sizeof(str), "%*c%s(%s) {",
                 Counter*2,' ', nm, MessageV(s,ap) );
       va_end(ap);
     } else
       SNprintf( str, sizeof(str), "%*c%s() {",
                 Counter*2,' ', nm );

    LogCmd( str, ldInt );

    Counter++;
    SetLastError(err);
}

FARINProc::~FARINProc()
  {  DWORD err = GetLastError();
     char  str[500];

    Counter--;

    SNprintf( str, sizeof(str), "%*c}<%s>", Counter*2,' ',Name );
    LogCmd( str,ldInt );

    SetLastError(err);
}

void FARINProc::Say( CONSTSTR s,... )
  {  va_list ap;
     char    str[500];
     size_t     rc;
     DWORD   err = GetLastError();

    va_start( ap,s );
      rc = SNprintf( str, sizeof(str), "%*c", Counter*2,' ' );
      if ( rc < sizeof(str) )
        VSNprintf( str+rc, sizeof(str)-rc, s,ap );
    va_end(ap);

    LogCmd( str,ldInt );

    SetLastError(err);
}

std::wstring DECLSPEC FixFileNameChars(std::wstring s, BOOL slashes)
{  
	if(g_manager.opt.InvalidSymbols.empty()) 
		return s;
	std::wstring inv = g_manager.opt.InvalidSymbols;
	std::wstring::const_iterator itr = inv.begin();
	while(itr != inv.end())
	{
		size_t i = s.find_first_of(*itr);
		if(i != std::wstring::npos)
		{
			s[i] = g_manager.opt.CorrectedSymbols[itr-inv.begin()];
		}
		++itr;
	}
	if(slashes)
	{
		std::wstring::iterator i = s.begin();
		while(i != s.end())
		{
			if(*i == L'\\')
				*i = L'_';
			else
				if(*i == L':')
					*i = L'!';
			++i;
		}
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

CONSTSTR DECLSPEC GetSocketErrorSTR( void )
  {
 return GetSocketErrorSTR( WSAGetLastError() );
}

CONSTSTR DECLSPEC GetSocketErrorSTR( int err )
  {  static char estr[70];

     if ( !err )
       return FP_GetMsg(MWSAENoError);

     for ( int n = 0; stdSockErrors[n].MCode != MNone__; n++ )
       if ( stdSockErrors[n].Code == err )
         return FP_GetMsg( stdSockErrors[n].MCode );

     Sprintf( estr,"%s: %d",FP_GetMsg(MWSAEUnknown),err );
 return estr;
}
//------------------------------------------------------------------------
/*
   Procedure for convert digit to string
   Like an AtoI but can manipulate __int64 and has buffer limit

   Align digit to left, if buffer less when digit the output will be
   truncated at right.
   If size set to -1 the whole value will be output to ctring.
*/
char *DECLSPEC PDigit( char *buff,__int64 val,size_t sz /*=-1*/ )
  {  static char lbuff[100];
     char str[ 100 ];
     int pos = sizeof(str)-1;

     if ( !buff ) buff = lbuff;

     str[pos] = 0;
     if ( !val )
       str[--pos] = '0';
      else
     for ( ; pos && val; val /= 10 )
       str[--pos] = (char)('0'+val%10);

     if ( sz != -1 ) {
       if ( (sizeof(str))-1-pos > sz ) {
         str[ pos+sz-1 ] = FAR_RIGHT_CHAR;
       }
     }
     if ( pos <= 0 )
       str[0] = FAR_LEFT_CHAR;

     StrCpy( buff,str+pos,sz == -1 ? (-1) : (sz+1) );
 return buff;
}
/*
   Output digit to string
   Can delimit thousands by special character

   Fill buffer always with `sz` characters
   If digit less then `sz` in will be added by ' ' at left
   If digit more then buffer digit will be truncated at left
*/
char *DECLSPEC FDigit( char *buff,__int64 val, size_t sz )
  {  static char lbuff[100];
     char str[FAR_MAX_PATHSIZE];
     size_t  len,n,d;
     char *s;

     if ( !buff ) buff = lbuff;
     *buff = 0;

     if (!sz) return buff;

     if ( !val || !g_manager.opt.dDelimit || !g_manager.opt.dDelimiter ) 
	 {
       PDigit( buff,val,sz );
       return buff;
     }

     PDigit( str,val,sz );
     len = strlen(str);
     s   = str + len-1;
     if (sz == -1)
       sz = len + len/3 - ((len%3) == 0);
     d = sz;
     buff[d--] = 0;

     for ( n = 0; d >= 0 && n < sz && n < len; n++ ) {
       if ( n && (n%3) == 0 )
	   {
         buff[d--] = '.'; // TODO g_manager.opt.dDelimiter;
         sz--;
       }
       if (d >= 0)
         buff[d--] = *(s--);
     }

     if ( n > sz )
       buff[0] = FAR_LEFT_CHAR;

     if ( d >= 0 )
       for( ; d >= 0; d-- ) buff[d] = ' ';

 return buff;
}

//------------------------------------------------------------------------
/*
    Show `Message` with attention caption and query user to select YES or NO
    Returns nonzero if user select YES
*/
int DECLSPEC AskYesNoMessage( CONSTSTR LngMsgNum )
{  
	static CONSTSTR MsgItems[] = {
        FMSG(MAttention),
        NULL,
        FMSG(MYes), FMSG(MNo) };

     MsgItems[1] = LngMsgNum;

	return FMessage( FMSG_WARNING, NULL, MsgItems, ARRAY_SIZE(MsgItems),2 );
}

BOOL DECLSPEC AskYesNo( CONSTSTR LngMsgNum )
  {
 return AskYesNoMessage(LngMsgNum) == 0;
}
/*
    Show void `Message` with attention caption
*/
void DECLSPEC SayMsg( CONSTSTR LngMsgNum )
  {  CONSTSTR MsgItems[]={
        FMSG(MAttention),
        LngMsgNum,
        FMSG(MOk) };

    FMessage( FMSG_WARNING, NULL, MsgItems,ARRAY_SIZE(MsgItems),1 );
}

bool DECLSPEC IsCmdLogFile(void)
{
	return g_manager.opt.CmdLogFile[0] != 0;
}

std::wstring DECLSPEC GetCmdLogFile()
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
				wchar_t buffer[FAR_MAX_PATHSIZE] = {0};
				m = GetModuleHandleW( FP_GetPluginNameW() );
				buffer[GetModuleFileNameW(m, buffer, sizeof(buffer)/sizeof(*buffer))] = 0;
				str = buffer;
				size_t n = str.rfind(SLASH_CHAR);
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

void DECLSPEC LogCmd( CONSTSTR src,CMDOutputDir out,DWORD Size )
{

//File opened and fail
    if ( LogFile == INVALID_HANDLE_VALUE )
      return;

//Params
    if ( !IsCmdLogFile() || !src )
      return;

    src = FP_GetMsg( src );

    if ( out == ldRaw && (!Size || Size == MAX_DWORD) )
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
    LogFile = _wfopen(m.c_str(), !LogFile && g_manager.opt.TruncateLogFile ? L"w" : L"w+" );
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
      SNprintf( tmstr, sizeof(tmstr), "%d ", PluginUsed() );
      Fwrite( LogFile,tmstr,strLen(tmstr) );

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
    if ( !g_manager.opt._ShowPassword && StrCmp(src,"PASS ",5,TRUE) == 0 )
      src = "PASS *hidden*";

//Write multiline to log
    do{
       //Plugin
       SNprintf( tmstr, sizeof(tmstr), "%d ", PluginUsed() );
       Fwrite( LogFile,tmstr,strLen(tmstr) );

       //Time
       GetLocalTime( &st );
       SNprintf( tmstr,sizeof(tmstr),
                 "%4d.%02d.%02d %02d:%02d:%02d:%04d",
                 st.wYear, st.wMonth,  st.wDay,
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
       Fwrite( LogFile,tmstr,strlen(tmstr) );

       //Delay
       if ( !stOld.wYear )
          Sprintf( tmstr," ----" );
         else
          Sprintf( tmstr," %04d",
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

//------------------------------------------------------------------------
//  Connection
//------------------------------------------------------------------------
void Connection::InitIOBuff( void )
  {
    CloseIOBuff();
    IOBuff = (char*)_Alloc( Host.IOBuffSize+1 );
}
void Connection::CloseIOBuff( void )
  {
    _Del( IOBuff ); IOBuff = NULL;
}

/*
   Initialize CMD buffer data
*/
void Connection::InitCmdBuff()
{
	if(!cmdBuff_.empty())
		CloseCmdBuff();

	hIdle        = g_manager.opt.IdleShowPeriod ? FP_PeriodCreate(g_manager.opt.IdleShowPeriod) : NULL;
	CmdVisible   = TRUE;
	cmdLineSize  = g_manager.opt.CmdLine;
	cmdSize_		= g_manager.opt.CmdLength;
	RetryCount   = 0;
	//Command lines + status command
	cmdMsg_.reserve(cmdSize_ + NBUTTONSADDON);
	ResetCmdBuff();
}
/*
   Free initialize CMD buffer data
*/
void Connection::CloseCmdBuff( void )
{
	cmdBuff_.resize(0);
	rplBuff_.resize(0);
	cmdMsg_.resize(0);

	IOBuff  = NULL;
	FP_PeriodDestroy(hIdle);
}
//------------------------------------------------------------------------
/*
   Output message about internal plugin error
*/
void Connection::InternalError( void )
  {  CONSTSTR MsgItems[] = {
       FMSG(MFtpTitle),
       FMSG(MIntError),
       FMSG(MOk) };

     FMessage( FMSG_WARNING, NULL, MsgItems, ARRAY_SIZE(MsgItems), 1 );
}
/*
   Set CMD buffer data to zero state

   Used just after new connection established
*/
void Connection::ResetCmdBuff( void )
{
    hostname[0]	= 0;
    lastHost_	= L"";
    lastMsg_	= L"";
}

std::string Connection::SetCmdLine(const std::string &src, CMDOutputDir direction)
{
	std::string result = src;

	if(!g_manager.opt._ShowPassword && g_manager.opt.cmdPass_ == src)
		result = "PASS *hidden*";
	else
		if(static_cast<BYTE>(src[0]) == (BYTE)ffDM &&
			src.find("ABOR", 1, 4) != std::wstring::npos)
			result = "<ABORT>";

	switch(direction)
	{
	case ldInt:
		result = "<- " + result;
		break;
	case ldOut:
		result = "-> " + result;
	}

	size_t n = result.find('\r');
	if(n != std::wstring::npos)
		result.resize(n);
	n = result.find('\n');
	if(n != std::wstring::npos)
		result.resize(n);
	return result;
}

std::wstring Connection::SetCmdLine(const std::wstring &src, CMDOutputDir direction)
{
	std::wstring result;
	switch(direction)
	{
	case ldInt:
		result = L"<- " + src;
		break;
	case ldOut:
		result = L"-> " + src;
	}

	size_t n = result.find('\r');
	if(n != std::wstring::npos)
		result.resize(n);
	n = result.find('\n');
	if(n != std::wstring::npos)
		result.resize(n);
	return result;
}


void Connection::AddCmdLine(const std::wstring &str, CMDOutputDir direction)
{
	std::wstring buff;

	std::wstring::const_iterator itr = str.begin();
	while(itr != str.end() && (*itr == L'\r' || *itr == L'\n'))
		++itr;
	if(itr == str.end())
		return;

	buff = str; // TODO replyString_;

	//Remove overflow lines
	if(cmdBuff_.size() >= cmdSize_)
	{
		cmdBuff_.erase(cmdBuff_.begin(), cmdBuff_.begin() + (cmdSize_ - cmdBuff_.size() + 1));
		rplBuff_.erase(rplBuff_.begin(), rplBuff_.begin() + (cmdSize_ - cmdBuff_.size() + 1));
	}

	//Add new line
		LogCmd(Unicode::toOem(buff).c_str(), direction);

		cmdBuff_.push_back(toOEM(SetCmdLine(buff, direction)));
		rplBuff_.push_back(toOEM(SetCmdLine(buff, direction)));

		//Start
		if(startReply_.empty() && direction == ldIn)
		{
			// TODO wtf
			std::wstring::const_iterator itr = str.begin();
			while(itr != str.end() && 
				(std::isdigit(*itr, std::locale()) || *itr == ' ' || *itr == '\b' || *itr == '\t' ))
				++itr;

			startReply_ = FromOEM(std::string(itr, str.end())); // TODO check
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
void Connection::AddCmdLine( CONSTSTR str )
{
	std::wstring buff;

	if ( str ) {
		str = FP_GetMsg( str );
		//Skip IAC
		if ( ((BYTE)str[0]) == ffIAC && ((BYTE)str[1]) == ffIP )
			return;

		//Skip empty
		while( *str && strchr("\n\r",*str) != NULL ) str++;
		if ( !str[0] )
			return;
	}

	// TODO
}
//------------------------------------------------------------------------
/*
    Show CMD window
    Use `CmdBuff` string to write in window
    Returns nonzero if `btn` to specified or user select button from modal dialog

    IF `btn` != MNone__
      Show modal message with button `btn`
    IF `btn` < 0
      Show modal message with button Abs(`btn`) and error color of message window
    IF `HostName` != NULL
      Set LastHost to this value
*/
static WORD Keys[] = { VK_ESCAPE, 'C', 'R', VK_RETURN };

bool Connection::ConnectMessageTimeout(int Msg /*= MNone__*/, const std::wstring &hostname, int BtnMsg /*= MNone__*/ )
{  
	std::string		str;
	std::wstring	host;
	BOOL              rc, 
		first = TRUE;
	TIME_TYPE         b,e;
	CMP_TIME_TYPE     diff;
	int               secNum;

	if (IS_FLAG(FP_LastOpMode, OPM_FIND) ||
		!g_manager.opt.RetryTimeout ||
		(BtnMsg != -MRetry && BtnMsg != -MRestore) )
		return ConnectMessage( Msg, Unicode::toOem(hostname).c_str(), BtnMsg );

	host = lastHost_;

	secNum = 0;
	GET_TIME( b );
	do{
		switch( CheckForKeyPressed( Keys,ARRAY_SIZE(Keys) ) )
		{
		 case 1:
		 case 2: SetLastError( ERROR_CANCELLED );
			 return FALSE;
		 case 3:
		 case 4: return TRUE;
		}

		GET_TIME( e );
		diff = CMP_TIME( e,b );
		if ( first || diff > 1.0 )
		{
			first = FALSE;
			str = std::string("\"") + Unicode::toOem(hostname) + "\" " + 
				FP_GetMsg(MAutoRetryText) + " " +
				boost::lexical_cast<std::string>(g_manager.opt.RetryTimeout-secNum) +
				FP_GetMsg(MSeconds) + " " + FP_GetMsg(MRetryText);

			ConnectMessage(Msg, str.c_str());

			std::wstring wstr  = std::wstring(Unicode::fromOem(FP_GetMsg(Msg))) + L' ' +
				Unicode::fromOem(FP_GetMsg(MAutoRetryText)) + L' ' +
				boost::lexical_cast<std::wstring>(g_manager.opt.RetryTimeout-secNum) +
				Unicode::fromOem(FP_GetMsg(MSeconds));
			WinAPI::setConsoleTitle(wstr);

			secNum++;
			b = e;
		}

		if ( secNum > g_manager.opt.RetryTimeout )
		{
			rc = TRUE;
			break;
		}

		Sleep(100);
	}while( 1 );

	lastHost_ = host;

	return rc;
}

int Connection::ConnectMessageW( int Msg, const std::wstring &HostName,
							   int btn, int btn1, int btn2)
{
	return ConnectMessage(Msg, Unicode::toOem(HostName).c_str(), btn, btn1, btn2);
}

int Connection::ConnectMessage( int Msg /*= MNone__*/,CONSTSTR HostName /*= NULL*/,
                                int btn /*= MNone__*/,int btn1 /*= MNone__*/,int btn2 /*= MNone__*/ )
{
	//PROC(( "ConnectMessage", "%d,%s,%d,%d,%d", Msg, HostName, btn, btn1, btn2 ))
	size_t			n;
	BOOL			res;
	CONSTSTR		m;
	BOOL			exCmd;

	if ( btn < 0 && g_manager.opt.DoNotExpandErrors )
		exCmd = FALSE;
	else
		exCmd = Host.ExtCmdView;

	//Called for update CMD window but it disabled
	if ( Msg == MNone__ && !HostName && !exCmd )
		return TRUE;

	if ( Msg != MNone__ )          lastMsg_ = SetCmdLine(Unicode::fromOem(FP_GetMsg(Msg)), ldRaw);
	if ( HostName && HostName[0] ) lastHost_= SetCmdLine(Unicode::fromOem(HostName), ldRaw);

	if ( btn == MNone__ )
		if ( IS_FLAG(FP_LastOpMode,OPM_FIND)                       ||  //called from find
			!CmdVisible                                           ||  //Window disabled
			(IS_SILENT(FP_LastOpMode) && !g_manager.opt.ShowSilentProgress) ) { //show silent processing disabled
				if ( HostName )
				{
					SetLastError( ERROR_SUCCESS );
					IdleMessage( HostName, g_manager.opt.ProcessColor );
				}
				return FALSE;
		}

		cmdMsg_.resize(0);

		//Title
		String str;

		m = FP_GetMsg(MFtpTitle);
		if ( hostname[0] ) {
			if ( UserPassword[0] )
				str.printf( "%s \"%s:*@%s\"",m,userName_,hostname );
			else
				str.printf( "%s \"%s:%s\"",m,userName_,hostname );
		} else
			str = m;
		str.SetLength( cmdLineSize );
		cmdMsg_.push_back(str.c_str());

		//Error delimiter
		if (btn != MNone__ && btn < 0 )
			switch( GetLastError() ) {
		 case   ERROR_SUCCESS:
		 case ERROR_CANCELLED: break;
		 default: cmdMsg_.push_back("\x1");
		}

		if ( GetLastError() == ERROR_CANCELLED )
			SetLastError( ERROR_SUCCESS );

		//Commands
		if ( exCmd )
			for ( n = 0; n < cmdBuff_.size(); n++ )
				cmdMsg_.push_back(cmdBuff_[n]);

		//Message
		std::wstring msg;

		if(Msg != MOk && !lastMsg_.empty())
		{
			if (exCmd && !cmdBuff_.empty())
				cmdMsg_.push_back("\x1");

			if(exCmd)
			{
				SYSTEMTIME st;
				GetLocalTime(&st);
				// msg.printf("%02d:%02d:%02d \"%s\"", st.wHour, st.wMinute, st.wSecond, lastMsg_);
				std::wstringstream stream;
				stream <<	std::setw(2) << st.wHour << L':' << 
							std::setw(2) << st.wMinute << L':' << 
							std::setw(2) << st.wSecond << 
							L" \"" << lastMsg_  << + "\"";
				msg = stream.str();
			} else
				msg = lastMsg_;

			msg.resize(cmdLineSize);
			cmdMsg_.push_back(Unicode::toOem(msg.c_str()));
			Log(( "CMSG: %s", Unicode::toOem(lastMsg_.c_str())));
		}

		//Host
		std::wstring lh;
		if (!lastHost_.empty() && (!HostName || HostName[0]) )
		{
			lh = lastHost_;
			lh.resize(cmdLineSize);
			cmdMsg_.push_back(Unicode::toOem(lh));
		}

		//Buttons
#define ADD_BTN(v)  do{ cmdMsg_.push_back(v); btnAddon++; }while(0)

		int btnAddon = 0;

		if ( btn != MNone__ )
		{
			cmdMsg_.push_back("\x1");
			ADD_BTN( FP_GetMsg(Abs(btn)) );

			if ( btn1 != MNone__ ) ADD_BTN( FP_GetMsg(btn1) );
			if ( btn2 != MNone__ ) ADD_BTN( FP_GetMsg(btn2) );

			if ( btn < 0 && btn != -MOk )
				ADD_BTN( FP_GetMsg(MCancel) );
		}

		//Display error in title
		if(btn < 0 && btn != MNone__ && FP_Screen::isSaved())
		{
			WinAPI::setConsoleTitle(lastMsg_ + L" \"" + lastHost_ + L"\"");
		}

		//Message
		BOOL isErr = (btn != MNone__ || btn < 0) && GetLastError() != ERROR_SUCCESS,
			isWarn = btn != MNone__ && btn < 0;

		res = FMessage( (isErr ? FMSG_ERRORTYPE : 0) | (isWarn ? FMSG_WARNING : 0) | (exCmd ? FMSG_LEFTALIGN : 0),
			NULL, cmdMsg_, btnAddon );
		Log(( "CMSG: rc=%d", res ));

		//Del auto-added `cancel`
		btnAddon -= btn < 0 && btn != -MOk;

		//If has user buttons: return number of pressed button
		if ( btnAddon > 1 )
			return res;

		//If single button: return TRUE if button was selected
		return btn != MNone__ && res == 0;
}

void DECLSPEC OperateHidden( CONSTSTR fnm, BOOL set )
  {
     if ( !g_manager.opt.SetHiddenOnAbort ) return;
     Log(( "%s hidden", set ? "Set" : "Clr" ));

     DWORD dw = GetFileAttributes( fnm );
     if ( dw == MAX_DWORD ) return;

     if ( set )
       SET_FLAG( dw, FILE_ATTRIBUTE_HIDDEN );
      else
       CLR_FLAG( dw, FILE_ATTRIBUTE_HIDDEN );

     SetFileAttributes( fnm, dw );
}
