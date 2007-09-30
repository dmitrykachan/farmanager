#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "utils/uniconverts.h"

FTPCopyInfo::FTPCopyInfo( void )
  {
     asciiMode       = FALSE;
     ShowProcessList = FALSE;
     AddToQueque     = FALSE;
     MsgCode         = ocNone;
     Download        = FALSE;
     UploadLowCase   = FALSE;
     FTPRename       = FALSE;
}

bool WINAPI IsAbsolutePath(const std::wstring &path)
{
	return path.size() > 3 &&
         path[1] == ':' && path[2] == '\\';
}

const wchar_t quotes[] = L" \"%,;[]";

// TODO remove
void WINAPI QuoteStr(std::wstring& str)
{  
	if(str.find_first_of(quotes) == std::wstring::npos)
		return;

	str.insert(str.begin(), L'\"');
	std::wstring::iterator itr = str.begin()+1;
	while((itr = std::find(itr, str.end(), L'\"')) != str.end())
	{
		++itr;
		itr = str.insert(itr, L'\"');
	}
}

enum
{
	SIZE_M = 1024*1024,
	SIZE_K = 1024
};

std::wstring WINAPI Size2Str(__int64 sz)
{
	wchar_t letter = 0;
	double size = (double)sz;

	if(size >= SIZE_M)
	{
		size /= SIZE_M;
		letter = L'M';
	} else
		if(size >= SIZE_K)
		{
			size /= SIZE_K;
			letter = L'K';
		}

	if(!letter )
	{
		return boost::lexical_cast<std::wstring>(sz);
	}

	std::wstring s = boost::lexical_cast<std::wstring>(size);
	if(s.find(L'.') == std::wstring::npos)
		return s;
	return s + letter;
}

__int64 WINAPI Str2Size(std::wstring str)
{  
	wchar_t  letter;
	int		mult = 1;

	if (str.empty())
		return 0;

	letter = *str.rbegin();
	if(letter == L'k' || letter == L'K')
		mult = SIZE_K;
	else
		if(letter == L'm' || letter == L'M')
			mult = SIZE_M;
	if(mult != 1)
	{
		str.resize(str.size()-1);
	}

	return static_cast<__int64>(boost::lexical_cast<double>(str)*mult);
}

bool WINAPI FRealFile(const wchar_t* nm, FAR_FIND_DATA* fd)
{
	HANDLE f;
	bool   rc;

	f = CreateFileW(nm, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	rc = f &&
		f != INVALID_HANDLE_VALUE &&
		GetFileType(f) == FILE_TYPE_DISK;

	if ( rc && fd )
	{
		fd->lpwszFileName		= const_cast<wchar_t*>(nm);
		fd->dwFileAttributes	= GetFileAttributesW(nm);
		DWORD highFileSize;
		fd->nFileSize			= GetFileSize(f, &highFileSize);
		fd->nFileSize			|= static_cast<__int64>(highFileSize) << 32;
		GetFileTime(f, &fd->ftCreationTime, &fd->ftLastAccessTime, &fd->ftLastWriteTime );
	}

	CloseHandle(f);

	return rc;
}

__int64 WINAPI Fsize(const std::wstring& filename)
{
	HANDLE f;
	DWORD lo,hi;

	f = CreateFileW(filename.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if ( !f || f == INVALID_HANDLE_VALUE )
		return 0;
	lo = GetFileSize(f, &hi);
	CloseHandle(f);

	if ( lo == UINT_MAX )
		return 0;
	else
		return ((__int64)hi) * ((__int64)UINT_MAX) + ((__int64)lo);
}

__int64 WINAPI Fsize( HANDLE File )
  {  DWORD low,hi;

     low = GetFileSize( File,&hi );
     if ( low == UINT_MAX )
       return 0;
      else
       return ((__int64)hi) * ((__int64)UINT_MAX) + ((__int64)low);
}

BOOL WINAPI Fmove( HANDLE file,__int64 restart )
  {  LONG lo = (LONG)( restart % ((__int64)UINT_MAX) ),
          hi = (LONG)( restart / ((__int64)UINT_MAX) );

    if ( SetFilePointer( file,lo,&hi,FILE_BEGIN ) == 0xFFFFFFFF &&
         GetLastError() != NO_ERROR )
      return FALSE;

 return TRUE;
}

void WINAPI Fclose( HANDLE file )
  {
    if ( file ) {
      SetEndOfFile( file );
      CloseHandle( file );
    }
}

BOOL WINAPI Ftrunc( HANDLE h,DWORD move )
  {
     if ( move != FILE_CURRENT )
       if ( SetFilePointer(h,0,NULL,move) == 0xFFFFFFFF )
         return FALSE;

 return SetEndOfFile(h);
}

HANDLE WINAPI Fopen(const wchar_t* nm, const wchar_t* mode /*R|W|A[+]*/, DWORD attr )
{
	  BOOL   rd  = toupper(mode[0]) == L'R';
     HANDLE h;

     if ( rd )
       h = CreateFileW( nm, GENERIC_READ,
                       FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, attr, NULL );
      else
       h = CreateFileW( nm, GENERIC_WRITE,
                       FILE_SHARE_READ, NULL, OPEN_ALWAYS, attr, NULL );

     if ( !h ||
          h == INVALID_HANDLE_VALUE )
       return NULL;

     do{
       if ( toupper(mode[0]) == L'A' || mode[1] == L'+' )
         if ( SetFilePointer(h,0,NULL,FILE_END) == 0xFFFFFFFF )
           break;

       if ( !rd )
         SetEndOfFile(h);  //Ignore SetEndOfFile result in case of use with CON, NUL and others

       return h;
     }while(0);

     CloseHandle(h);
 return NULL;
}

int WINAPI Fwrite( HANDLE File,LPCVOID Buff, size_t Size)
{  
	DWORD res;
	return WriteFile(File,Buff,(DWORD)Size,&res,NULL) ? ((int)res) : (-1);
}

int WINAPI Fread( HANDLE File,LPVOID Buff,int Size )
  {  DWORD res;

 return ReadFile(File,Buff,(DWORD)Size,&res,NULL) ? ((int)res) : (-1);
}

void DMessage(const std::wstring &str, bool full, int color, int y)
{
	std::wstring err;

	if(full)
	{
		err = str;

		err.resize(WinAPI::getConsoleWidth()-4, L' ');
		FARWrappers::getInfo().Text(2, y, color, err.c_str());
	} else
		FARWrappers::getInfo().Text(2, y, color, str.c_str());
}

void IdleMessage(const wchar_t* str, int color, bool error)
{
	static HANDLE hScreen;

	//Clear
 	if(!str)
	{
		if(hScreen)
		{
			FARWrappers::getInfo().RestoreScreen(hScreen);
			hScreen = NULL;
		}
		return;
	}

	//Draw
	std::wstring msg = str;
	if(is_flag(g_manager.opt.IdleMode, IDLE_CAPTION))
		WinAPI::setConsoleTitle(msg);

	if ( is_flag(g_manager.opt.IdleMode,IDLE_CONSOLE) )
	{
		if ( !hScreen )
			hScreen = FARWrappers::getInfo().SaveScreen(0,0, WinAPI::getConsoleWidth(), 2);

		DMessage(msg, error, color, 0);
		if(error)
			DMessage(WinAPI::getStringError(), error, color, 1);

		FARWrappers::getInfo().Text(0, 0, 0, NULL);
	}
}


void WINAPI AddEndSlash(std::wstring& s, wchar_t slash)
{
	if(s.size() == 0)
		return;

	if(slash == 0)
		slash = s.find(L'\\') != std::wstring::npos ? '\\' : '/';

	if(*s.rbegin() != slash)
		s.push_back(slash);
}


void WINAPI DelEndSlash(std::wstring& s, char slash)
{  
	if(!s.empty() && *(s.end()-1) == slash)
		s.resize(s.size()-1);
}


std::wstring WINAPI getName(const std::wstring &path)
{  
	std::wstring::size_type n = path.find_last_of(L"\\/:");
	if(n == std::wstring::npos)
		return path;
	else
		return std::wstring(path.begin()+n+1, path.end());
}

bool CheckForEsc(bool isConnection, bool IgnoreSilent )
{  
	const WORD  ESCCode = VK_ESCAPE;
	BOOL  rc;

	if(!IgnoreSilent && is_flag(FP_LastOpMode,OPM_FIND))
		return false;

	rc = WinAPI::checkForKeyPressed(&ESCCode, 1);
	if(rc == -1)
		return false;

	rc = !g_manager.opt.AskAbort ||
		FARWrappers::messageYesNo(getMsg( isConnection ? MTerminateConnection : MTerminateOp ));

	if(rc)
		BOOST_LOG(INF, L"ESC: cancel detected");

	return rc;
}

int WINAPI IsCaseMixed(const char *Str)
{
	char AnsiStr[1024];
	OemToChar(Str,AnsiStr);
	while (*Str && !IsCharAlpha(*Str))
		Str++;
	int Case=IsCharLower( *Str );
	while (*(Str++))
		if (IsCharAlpha(*Str) && IsCharLower(*Str) != Case)
			return(TRUE);
	return(FALSE);
}

void WINAPI LocalLower(char *Str)
{
  OemToChar(Str,Str);
  CharLower(Str);
  CharToOem(Str,Str);
}

void WINAPI FixFTPSlash(std::wstring& s )
{
	std::wstring::iterator i = s.begin();

	while((i = std::find(i, s.end(), L'\\')) != s.end())
		*i = L'/';
}

void WINAPI FixLocalSlash(std::wstring &path)
{
	std::replace(path.begin(), path.end(), L'/', L'\\');
}


std::wstring getPathBranch(const std::wstring &s)
{
	size_t n = s.rfind('\\');
	if(n == std::wstring::npos || n == 0)
		return L"";
	else
		return std::wstring(s, 0, n);
}


std::wstring getPathLast(const std::wstring &s)
{
	size_t n = s.rfind('\\');
	if(n == std::wstring::npos || n == 0)
		return s;
	else
		return std::wstring(s.begin()+n+1, s.end());
}

#ifdef CONFIG_TEST
BOOST_AUTO_TEST_CASE(testPathProcedures)
{
	BOOST_CHECK(getPathBranch(L"")		== L"");
	BOOST_CHECK(getPathBranch(L"\\")	== L"");
	BOOST_CHECK(getPathBranch(L"abc")	== L"");
	BOOST_CHECK(getPathBranch(L"abc\\def")== L"abc");
	BOOST_CHECK(getPathBranch(L"abc\\\\def")== L"abc\\");
	BOOST_CHECK(getPathBranch(L"abc\\def\\")== L"abc\\def");


	BOOST_CHECK(getPathLast(L"")		== L"");
	BOOST_CHECK(getPathLast(L"\\")		== L"\\");
	BOOST_CHECK(getPathLast(L"abc")		== L"abc");
	BOOST_CHECK(getPathLast(L"abc\\def")== L"def");
	BOOST_CHECK(getPathLast(L"abc\\\\def")== L"def");
	BOOST_CHECK(getPathLast(L"abc\\def\\")== L"");
}

#endif