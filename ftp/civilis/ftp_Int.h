#ifndef __FAR_PLUGIN_FTP
#define __FAR_PLUGIN_FTP

#if !defined(SD_BOTH)
  #define SD_RECEIVE      0x00
  #define SD_SEND         0x01
  #define SD_BOTH         0x02
#endif

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
extern int      WINAPI IsCaseMixed(const char *Str);
extern void     WINAPI LocalLower(char *Str);
extern bool		WINAPI IsAbsolutePath(const std::wstring &nm);

extern HANDLE   WINAPI Fopen(const wchar_t* nm, const wchar_t* mode /*R|W|A[+]*/, DWORD attr = FILE_ATTRIBUTE_NORMAL );
extern __int64  WINAPI Fsize(const std::wstring& filename);
extern __int64  WINAPI Fsize( HANDLE nm );
extern BOOL     WINAPI Fmove( HANDLE file,__int64 restart_point );
extern void     WINAPI Fclose( HANDLE file );
extern int      WINAPI Fwrite( HANDLE File,LPCVOID Buff,size_t Size );
extern int      WINAPI Fread( HANDLE File,LPVOID Buff,int Size );
extern BOOL     WINAPI Ftrunc( HANDLE h,DWORD move = FILE_CURRENT );

extern bool     WINAPI FRealFile(const wchar_t* nm, FAR_FIND_DATA* fd = NULL );

extern void     IdleMessage(const wchar_t* str, int color, bool error = false);
extern std::wstring WINAPI Size2Str(__int64 sz);
extern __int64  WINAPI Str2Size(std::wstring str);
extern void     WINAPI QuoteStr(std::wstring &str);

//[ftp_JM.cpp]
extern const std::wstring GetSocketErrorSTR(int err);
extern const std::wstring GetSocketErrorSTR();

extern void     WINAPI LogCmd(const wchar_t* src,CMDOutputDir out,DWORD Size = UINT_MAX );
extern bool     WINAPI IsCmdLogFile( void );
extern std::wstring WINAPI GetCmdLogFile();

std::wstring	WINAPI FixFileNameChars(std::wstring s, BOOL slashes = FALSE);

extern void     WINAPI OperateHidden(const std::wstring& fnm, bool set);

//[ftp_sock.cpp]
extern void     WINAPI scClose( SOCKET& sock,int how = SD_BOTH );

struct FHandle {
  HANDLE Handle;
 public:
  FHandle( void )     : Handle(NULL) {}
  FHandle( HANDLE h ) : Handle(h)    {}
  ~FHandle()                         { Close(); }

  void Close( void )                 { if (Handle) { Fclose(Handle); Handle = NULL; } }
};


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
	void addWait(time_t tm);


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

extern std::wstring getPathBranch(const std::wstring &s);
extern std::wstring getPathLast(const std::wstring &s);

inline bool IS_SILENT(int v)
{
	return (v & (OPM_FIND|OPM_VIEW|OPM_EDIT)) != 0;
}

#define FAR_VERT_CHAR                        L'\xB3' //³
#define FAR_SBMENU_CHAR                      '\x10' //

namespace
{
	const wchar_t* HostRegName = L"Hosts\\";
}

#endif