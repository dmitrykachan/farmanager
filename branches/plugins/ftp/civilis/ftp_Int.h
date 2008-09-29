#ifndef __FAR_PLUGIN_FTP
#define __FAR_PLUGIN_FTP

extern int					FP_LastOpMode;

#include "ftp_Cfg.h"       //Config constants and Opt structure
#include "ftp_JM.h"        //`JM` changes
#include "ftp_var.h"       //class cmd
#include "ftp_Connect.h"   //class Connection
#include "ftp_FtpAPI.h"    //FtpXXX API
#include "ftp_Ftp.h"       //class Ftp


//[ftp_FAR.cpp]
extern FTP     *WINAPI OtherPlugin( FTP *p );
extern size_t   WINAPI PluginPanelNumber( FTP *p );

//[ftp_Config.cpp]
extern int      WINAPI Config( void );

//[ftp_Dlg.cpp]
extern bool     WINAPI AskSaveList( SaveListInfo* sli );
extern bool     WINAPI GetLoginData(std::wstring &username, std::wstring &password, bool forceAsk);

//[ftp_Mix.cpp]
extern void		WINAPI AddEndSlash(std::wstring& s, wchar_t slash);
extern void     WINAPI DelEndSlash(std::wstring& s,char slash);
extern void     WINAPI FixFTPSlash(std::wstring& Path);
extern void     WINAPI FixLocalSlash(std::wstring &Path);
extern std::wstring WINAPI getName(const std::wstring &path);

extern bool     CheckForEsc(bool isConnection, bool IgnoreSilent = false);
extern bool		WINAPI IsAbsolutePath(const std::wstring &nm);

extern HANDLE   WINAPI Fopen(const wchar_t* nm, const wchar_t* mode /*R|W|A[+]*/, DWORD attr = FILE_ATTRIBUTE_NORMAL );
extern __int64  WINAPI Fsize(const std::wstring& filename);
extern __int64  WINAPI Fsize( HANDLE nm );
extern bool     WINAPI Fmove(HANDLE file, __int64 restart_point);
extern void     WINAPI Fclose( HANDLE file );
extern int      WINAPI Fwrite( HANDLE File,LPCVOID Buff,size_t Size );
extern int      WINAPI Fread( HANDLE File,LPVOID Buff,int Size );
extern BOOL     WINAPI Ftrunc( HANDLE h,DWORD move = FILE_CURRENT );

extern bool     WINAPI FRealFile(const std::wstring& filename, FTPFileInfo& fd);

extern void     IdleMessage(const wchar_t* str, int color, bool error = false);
extern std::wstring Size2Str(__int64 sz);
extern __int64  Str2Size(std::wstring str);
extern void     WINAPI QuoteStr(std::wstring &str);

extern void     WINAPI LogCmd(const std::wstring &src,CMDOutputDir out);
extern bool     WINAPI IsCmdLogFile( void );
extern std::wstring WINAPI GetCmdLogFile();

std::wstring	WINAPI FixFileNameChars(std::wstring s, bool slashes = false);

extern void     WINAPI OperateHidden(const std::wstring& fnm, bool set);

//[ftp_sock.cpp]
extern void     WINAPI scClose( SOCKET& sock,int how = SD_BOTH );

class FTPPluginManager
{
public:
	void		addPlugin(FTP *ftp);
	void		removePlugin(FTP *ftp);
	FTP*		getPlugin(size_t n);
	size_t		getPlugin(FTP *ftp);
	void		openRegKey(const std::wstring& key)
	{
		regkey_.open(HKEY_CURRENT_USER, key.c_str());
	}

	enum
	{
		PluginCount = 3 // three plugins can be being running in several cases
					// for example 2 panels is open and a user whatns reopen panel.
	};

	void readCfg();
	void WriteCfg();
	void InvalidateAll();

	const WinAPI::RegKey& getRegKey() const
	{
		return regkey_;
	}

	Options        opt;

private:
	WinAPI::RegKey		regkey_;

	boost::array<FTP*, PluginCount> FTPPanels_;
};


extern	FTPPluginManager g_manager;

std::wstring getPathBranch(const std::wstring &s);
std::wstring getNetPathBranch(const std::wstring &s);
std::wstring getPathLast(const std::wstring &s);
void splitFilename(const std::wstring &path, std::wstring &branch, std::wstring &filename, bool local = true);

inline bool IS_SILENT(int v)
{
	return (v & (OPM_FIND|OPM_VIEW|OPM_EDIT)) != 0;
}

const wchar_t FAR_VERT_CHAR		= L'\x2502';
const wchar_t FAR_SBMENU_CHAR	= L'\x10';
const wchar_t FAR_LEFT_CHAR		= L'\x25C4';
const wchar_t FAR_RIGHT_CHAR	= L'\x25BA';
const wchar_t FAR_SHADOW_CHAR	= L'\x2593';
const wchar_t FAR_FULL_CHAR		= L'\x2588';

const std::wstring g_MemoryFile = L"-";

namespace
{
	const wchar_t* HostRegName = L"Hosts\\";
}


class BeepOnLongOperation
{
private:
	WinAPI::Stopwatch stopwatch;

public:
	BeepOnLongOperation()
		: stopwatch(g_manager.opt.LongBeepTimeout)
	{
	}

	~BeepOnLongOperation()
	{
		if(stopwatch.isTimeout())
		{
			MessageBeep( MB_ICONASTERISK );
		}
	}

	void reset()
	{
		stopwatch.reset();
	}
};

#endif