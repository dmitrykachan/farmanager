#ifndef __FAR_PLUGIN_FTP
#define __FAR_PLUGIN_FTP

#if defined(__DEBUG__) && defined(__BCWIN32__) && !defined(__JM__)
  #define __JM__ 1
#endif

#if !defined(SD_BOTH)
  #define SD_RECEIVE      0x00
  #define SD_SEND         0x01
  #define SD_BOTH         0x02
#endif

#include "fstdlib.h"         //FAR plugin stdlib
#include "ftp_Plugin.h"  //Plugin

#include "servertype.h"


#include "ftp_Plugins.h"   //plugins
#include "ftp_Cfg.h"       //Config constants and Opt structure
#include "ftp_JM.h"        //`JM` changes
#include "ftp_var.h"       //class cmd
#include "ftp_Connect.h"   //class Connection
#include "ftp_FtpAPI.h"    //FtpXXX API
#include "ftp_Ftp.h"       //class Ftp


//[ftp_FAR.cpp]
extern FTP     *DECLSPEC OtherPlugin( FTP *p );
extern size_t   DECLSPEC PluginPanelNumber( FTP *p );
extern size_t   DECLSPEC PluginUsed();

//[ftp_Config.cpp]
extern void     DECLSPEC WriteCfg( void );
extern int      DECLSPEC Config( void );

//[ftp_Dlg.cpp]
extern BOOL     DECLSPEC AskSaveList( SaveListInfo* sli );
extern bool     DECLSPEC GetLoginData(std::wstring &username, std::wstring &password, bool forceAsk);

//[ftp_Mix.cpp]
extern void     DECLSPEC AddEndSlash( char *Path,char slash, size_t ssz );
extern void     DECLSPEC AddEndSlash( String& p, char slash );
extern void		DECLSPEC AddEndSlash(std::wstring& s, wchar_t slash);
extern void     DECLSPEC DelEndSlash( char *Path,char slash );
extern void     DECLSPEC DelEndSlash( String& Path,char slash );
extern void     DECLSPEC DelEndSlash(std::wstring& s,char slash);
extern void     DECLSPEC FixFTPSlash( char *Path);
extern void     DECLSPEC FixFTPSlash( String& Path);
extern void     DECLSPEC FixFTPSlash(std::wstring& Path);
extern void     DECLSPEC FixLocalSlash( char *Path );
extern void     DECLSPEC FixLocalSlash( String& Path );
extern void     DECLSPEC FixLocalSlash(std::wstring &Path);
extern char    *DECLSPEC TruncStr(char *Str,int MaxLength);
extern const char    *DECLSPEC PointToName(const char *Path );
extern std::wstring DECLSPEC getName(const std::wstring &path);

extern BOOL     DECLSPEC CheckForEsc( BOOL isConnection,BOOL IgnoreSilent = FALSE );
extern int      DECLSPEC CheckForKeyPressed( WORD *Codes,int NumCodes );
extern int      DECLSPEC IsCaseMixed(const char *Str);
extern void     DECLSPEC LocalLower(char *Str);
extern BOOL     DECLSPEC IsDirExist( CONSTSTR nm );
extern bool		DECLSPEC IsAbsolutePath(const std::wstring &nm);

#define TAddEndSlash( s,sl  ) AddEndSlash( s,sl,sizeof(s) )

extern HANDLE   DECLSPEC Fopen( CONSTSTR nm,CONSTSTR mode /*R|W|A[+]*/, DWORD attr = FILE_ATTRIBUTE_NORMAL );
extern __int64  DECLSPEC Fsize( CONSTSTR nm );
extern __int64  DECLSPEC Fsize( HANDLE nm );
extern BOOL     DECLSPEC Fmove( HANDLE file,__int64 restart_point );
extern void     DECLSPEC Fclose( HANDLE file );
extern int      DECLSPEC Fwrite( HANDLE File,LPCVOID Buff,size_t Size );
extern int      DECLSPEC Fread( HANDLE File,LPVOID Buff,int Size );
extern BOOL     DECLSPEC Ftrunc( HANDLE h,DWORD move = FILE_CURRENT );

extern BOOL     DECLSPEC FTestOpen( CONSTSTR nm );
extern BOOL     DECLSPEC FTestFind( CONSTSTR nm,WIN32_FIND_DATA* ufd = NULL );
extern BOOL     DECLSPEC FRealFile( CONSTSTR nm,WIN32_FIND_DATA* ufd = NULL );

extern int      DECLSPEC FMessage( unsigned int Flags,CONSTSTR HelpTopic,CONSTSTR *Items,
                                      int ItemsNumber,int ButtonsNumber );
extern int		DECLSPEC FMessage(unsigned int Flags, CONSTSTR HelpTopic, const std::vector<std::string> Items, int ButtonsNumber);
extern int      DECLSPEC FDialog( int X2,int Y2,CONSTSTR HelpTopic,struct FarDialogItem *Item,int ItemsNumber );
extern int      DECLSPEC FDialogEx( int X2,int Y2,CONSTSTR HelpTopic,struct FarDialogItem *Item,int ItemsNumber,
                                       DWORD Flags = 0, FARWINDOWPROC DlgProc = (FARWINDOWPROC)MAX_WORD,long Param = 0 );

extern void     DECLSPEC IdleMessage( CONSTSTR str,int color );
extern size_t   DECLSPEC StrSlashCount( CONSTSTR m );      //Rets number af any slash chars in string
extern void     DECLSPEC Size2Str( char *buff,DWORD size );
extern DWORD    DECLSPEC Str2Size( char *str );
extern void     DECLSPEC QuoteStr( char *str );
extern void     DECLSPEC QuoteStr( String& str );
extern void     DECLSPEC QuoteStr(std::wstring &str);

//[ftp_JM.cpp]
extern CONSTSTR DECLSPEC GetSocketErrorSTR( int err );
extern CONSTSTR DECLSPEC GetSocketErrorSTR( void );
extern char    *DECLSPEC PDigit( char *buff,__int64 val,size_t sz /*-1*/ );          // Converts digit to string.
extern char    *DECLSPEC FDigit( char *buff,__int64 Value,size_t BuffSize /*-1*/ );  // Output digit to string. Delimits thousands.

extern int      DECLSPEC AskYesNoMessage( CONSTSTR LngMsgNum );
extern BOOL     DECLSPEC AskYesNo( CONSTSTR LngMsgNum );
extern void     DECLSPEC SayMsg( CONSTSTR LngMsgNum );
extern void     DECLSPEC LogCmd( CONSTSTR src,CMDOutputDir out,DWORD Size = MAX_DWORD );
extern bool     DECLSPEC IsCmdLogFile( void );
extern std::wstring DECLSPEC GetCmdLogFile();

//extern char    *DECLSPEC FixFileNameChars( char *fnm,BOOL slashes = FALSE );
std::wstring	DECLSPEC FixFileNameChars(std::wstring s, BOOL slashes = FALSE);

extern void     DECLSPEC OperateHidden( CONSTSTR fnm, BOOL set );

//[ftp_sock.cpp]
extern void     DECLSPEC scClose( SOCKET& sock,int how = SD_BOTH );
extern BOOL     DECLSPEC scValid( SOCKET sock );
extern SOCKET   DECLSPEC scCreate( short addr_type = AF_INET );
extern SOCKET   DECLSPEC scAccept( SOCKET *peer, struct sockaddr FAR* addr, int* addrlen );

//[ftp_FAR.cpp]
FAR_EXTERN void   FAR_DECLSPEC SetStartupInfo( const PluginStartupInfo *Info);
FAR_EXTERN void   FAR_DECLSPEC GetPluginInfo( PluginInfo *Info);
FAR_EXTERN int    FAR_DECLSPEC Configure(int ItemNumber);
FAR_EXTERN HANDLE FAR_DECLSPEC OpenPlugin(int OpenFrom,INT_PTR Item);
FAR_EXTERN void   FAR_DECLSPEC ClosePlugin(HANDLE hPlugin);
FAR_EXTERN int    FAR_DECLSPEC GetFindData(HANDLE hPlugin,PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
FAR_EXTERN void   FAR_DECLSPEC FreeFindData(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber);
FAR_EXTERN void   FAR_DECLSPEC GetOpenPluginInfo(HANDLE hPlugin,OpenPluginInfo *Info);
FAR_EXTERN int    FAR_DECLSPEC SetDirectory(HANDLE hPlugin,CONSTSTR Dir,int OpMode);
FAR_EXTERN int    FAR_DECLSPEC GetFiles(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber,int Move,char *DestPath,int OpMode);
FAR_EXTERN int    FAR_DECLSPEC PutFiles(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode);
FAR_EXTERN int    FAR_DECLSPEC DeleteFiles(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
FAR_EXTERN int    FAR_DECLSPEC MakeDirectory(HANDLE hPlugin,char *Name,int OpMode);
FAR_EXTERN int    FAR_DECLSPEC ProcessKey(HANDLE hPlugin,int Key,unsigned int ControlState);
FAR_EXTERN int    FAR_DECLSPEC ProcessEvent(HANDLE hPlugin,int Event,void *Param);
FAR_EXTERN int    FAR_DECLSPEC Compare( HANDLE hPlugin,const PluginPanelItem *i,const PluginPanelItem *i1,unsigned int Mode );

#if defined( __DEBUG__ )
void ShowMemInfo( void );
void LogPanelItems( struct PluginPanelItem *PanelItem, size_t ItemsNumber );
#else
inline void ShowMemInfo( void ) {}
#define LogPanelItems( PanelItem,ItemsNumber )
#endif

//------------------------------------------------------------------------
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
		PluginCount = 2
	};

	void readCfg();
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

extern BOOL           SocketStartup;
extern int            SocketInitializeError;
extern FTP           *LastUsedPlugin;
extern char           DialogEditBuffer[];

extern	FTPPluginManager g_manager;

extern std::wstring getPathBranch(const std::wstring &s);
extern std::wstring getPathLast(const std::wstring &s);


namespace
{
	const wchar_t* HostRegName = L"Hosts\\";
}

#endif