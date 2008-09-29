#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "utils/uniconverts.h"

FTPCopyInfo::FTPCopyInfo()
{
     asciiMode       = false;
     showProcessList = false;
     addToQueque     = false;
     msgCode         = ocNone;
     download        = false;
     uploadLowCase   = false;
     FTPRename       = false;
}

bool WINAPI IsAbsolutePath(const std::wstring &path)
{
	return path.size() > 3 &&
         path[1] == ':' && path[2] == '\\';
}

const wchar_t quotes[] = L" \"%,;[]";

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

std::wstring Size2Str(__int64 sz)
{
	wchar_t letter = 0;
	double size = static_cast<double>(sz);

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
		return boost::lexical_cast<std::wstring>(sz);
	else
		return boost::lexical_cast<std::wstring>(size) + letter;
}

__int64 Str2Size(std::wstring str)
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

bool WINAPI FRealFile(const std::wstring &filename, FTPFileInfo& fd)
{
	HANDLE f;
	bool   rc;

	f = CreateFileW(filename.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	rc = f != INVALID_HANDLE_VALUE && GetFileType(f) == FILE_TYPE_DISK;

	if(rc)
	{
		fd.clear();
		fd.setFileName(filename);
		fd.setWindowsAttribute(GetFileAttributesW(filename.c_str()));
		DWORD highFileSize;
		DWORD lowFileSize = GetFileSize(f, &highFileSize);
		fd.setFileSize(lowFileSize | (static_cast<__int64>(highFileSize) << 32));

		FILETIME creationTime, lastAccess, lastWrite;
		GetFileTime(f, &creationTime, &lastAccess, &lastWrite);
		fd.setCreationTime(creationTime);
		fd.setLastAccessTime(lastAccess);
		fd.setLastWriteTime(lastWrite);
	}

	CloseHandle(f);

	return rc;
}

__int64 WINAPI Fsize(const std::wstring& filename)
{
	HANDLE f;

	f = CreateFileW(filename.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (!f || f == INVALID_HANDLE_VALUE)
		return 0;
	__int64 size = Fsize(f);
	CloseHandle(f);
	return size;
}

__int64 WINAPI Fsize(HANDLE File)
{
	LARGE_INTEGER size;

	if(GetFileSizeEx(File, &size))
		return 0;
	else
		return size.QuadPart;
}

bool WINAPI Fmove(HANDLE file, __int64 restart)
{
	LARGE_INTEGER point;
	point.QuadPart = restart;

	return SetFilePointerEx(file, point, NULL, FILE_BEGIN) != 0;
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

	if(rd)
		h = CreateFileW( nm, GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, attr, NULL );
	else
		h = CreateFileW( nm, GENERIC_WRITE,
		FILE_SHARE_READ, NULL, OPEN_ALWAYS, attr, NULL );

	if(h == INVALID_HANDLE_VALUE)
		return NULL;

	do{
		if ( toupper(mode[0]) == L'A' || mode[1] == L'+' )
			if ( SetFilePointer(h,0,NULL,FILE_END) == INVALID_SET_FILE_POINTER )
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
		slash = s.find(LOC_SLASH) != std::wstring::npos ? LOC_SLASH : NET_SLASH;

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

	if(!IgnoreSilent && is_flag(FP_LastOpMode, OPM_FIND))
		return false;

	if(WinAPI::checkForKeyPressed(&ESCCode, 1) == -1)
		return false;

	return !g_manager.opt.AskAbort ||
		FARWrappers::messageYesNo(getMsg(isConnection ? MTerminateConnection : MTerminateOp));
}

void WINAPI FixFTPSlash(std::wstring& s)
{
	std::replace(s.begin(), s.end(), LOC_SLASH, NET_SLASH);
}

void WINAPI FixLocalSlash(std::wstring &path)
{
	std::replace(path.begin(), path.end(), NET_SLASH, LOC_SLASH);
}

std::wstring getPathBranch(const std::wstring &s)
{
	size_t n = s.rfind(LOC_SLASH);
	if(n == std::wstring::npos || n == 0)
		return L"";
	else
		return std::wstring(s, 0, n);
}

std::wstring getNetPathBranch(const std::wstring &s)
{
	size_t n = s.rfind(NET_SLASH);
	if(n == std::wstring::npos || n == 0)
		return L"";
	else
		return std::wstring(s, 0, n);
}


std::wstring getPathLast(const std::wstring &s)
{
	size_t n = s.rfind(LOC_SLASH);
	if(n == std::wstring::npos || n == 0)
		return s;
	else
		return std::wstring(s.begin()+n+1, s.end());
}


void splitFilename(const std::wstring &path, std::wstring &branch, std::wstring &filename, bool local)
{
	size_t n = path.rfind(local? LOC_SLASH : NET_SLASH);
	if(n == std::wstring::npos)
	{
		branch = L"";
		filename = path;
	}
	else
	{
		if(n == 0)
			branch = L"";
		else
			branch.assign(path, 0, n);
		filename.assign(path.begin()+n+1, path.end());
	}
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


	std::wstring path, filename;
	splitFilename(L"", path, filename);
	BOOST_CHECK(path == L"" && filename == L"");
	splitFilename(L"\\", path, filename);
	BOOST_CHECK(path == L"" && filename == L"");
	splitFilename(L"abc", path, filename);
	BOOST_CHECK(path == L"" && filename == L"abc");
	splitFilename(L"abc\\def", path, filename);
	BOOST_CHECK(path == L"abc" && filename == L"def");
	splitFilename(L"abc\\\\def", path, filename);
	BOOST_CHECK(path == L"abc\\" && filename == L"def");
	splitFilename(L"abc\\def\\", path, filename);
	BOOST_CHECK(path == L"abc\\def" && filename == L"");
}

#endif