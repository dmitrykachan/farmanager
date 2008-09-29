#ifndef __FAR_PLUGIN_CONFIGDATA
#define __FAR_PLUGIN_CONFIGDATA

#include "servertype.h"

const int FTR_MINBUFFSIZE = 50;

//Dialog edit history
#define FTP_GETHISTORY        L"FTPGet"
#define FTP_PUTHISTORY        L"FTPPut"
#define FTP_HOSTHISTORY       L"FTPHost"
#define FTP_FOLDHISTORY       L"FTPFolder"
#define FTP_USERHISTORY       L"FTPUser"
#define FTP_INCHISTORY        L"FTPIncludeMask"
#define FTP_EXCHISTORY        L"FTPExcludeMask"

#define FTP_CMDPREFIX         L"ftp"
#define FTP_CMDPREFIX_SIZE    3

inline std::wstring FTP_FILENAME(const PluginPanelItem *p)
{
	return p->FindData.lpwszFileName;
}


//Security flags
#define SEC_PLAINPWD          0x0001        //plugin can place password in generated url in a plain text

//transfer types
enum ftTypes {
  TYPE_A = 'A',
  TYPE_I = 'I',
  TYPE_NONE = 0
};


//FTP overwrite mode
enum overCode {
  ocOverAll = 1,
  ocSkip,
  ocSkipAll,
  ocOver,
  ocResume,
  ocResumeAll,
  ocNone,
  ocCancel = -1
};

//CMDLog output directions
enum CMDOutputDir {
  ldOut,     //plugin to server
  ldIn,      //server to plugin
  ldInt,     //internal log
  ldRaw,     //Log raw data dump (w\o header lines)
  ld_None
};

class FTP;

//Save list options
enum sliTypes
{
	sltUrlList,
	sltTree,
	sltGroup,
	sltNone
};

struct SaveListInfo
{
	std::wstring path;
	bool	Append;
	bool	AddPrefix;             //all
	bool	AddPasswordAndUser;
	bool	Quote;
	bool	Size;                  //tree and group
	int		RightBound;            //tree
	sliTypes ListType;
};

enum
{
	IDLE_CONSOLE = 1,
	IDLE_CAPTION = 2,
	IDLE_BOTH    = IDLE_CONSOLE | IDLE_CAPTION
};
//Configuration options
struct Options // : public OptionsPlugin
{
	// OptionsPlugin
	int     AddToDisksMenu;
	int     AddToPluginsMenu;
	int     DisksMenuDigit;
	int     ReadDescriptions;
	int     UpdateDescriptions;
	int     UploadLowCase;
	int     ShowUploadDialog;
	int     ResumeDefault;
	std::wstring firewall;
	int     PassiveMode;
	std::wstring	DescriptionNames;
	//Configurable
	BOOL    dDelimit;                      //Delimite digits with spec chars                              TRUE
	wchar_t dDelimiter;                    //Character delimiter                                          '.'
	BOOL    AskAbort;                      //Ask Yes/No on abort pressed                                  TRUE
	int     CmdLine;                       //Length of one line in command buffer                         70
	int     CmdCount;                     //Length of command lines in cache                             7
	int     IOBuffSize;                    //Size of IO buffer                                            10.000 (bytes)
	int     ExtCmdView;                    //Extended CMD window                                          TRUE
	int     KeepAlive;                     //Keep alive period (sec)                                      60
	int     PluginColumnMode;              //Default mode for hosts panel                                 -1 (have no preferred mode)
	BOOL    TimeoutRetry;                  //Auto retry operation if timeout error occured                TRUE
	int     RetryCount;                    //Count of auto retryes                                        0
	BOOL    LogOutput;                     //Write output data to log                                     FALSE
	int     LongBeepTimeout;               //long operation beeper
	int     WaitTimeout;                   //Maximum timeout to wait data receiving  (sec)                300
	BOOL    ShowIdle;                      //Show idle percent                                            TRUE
	int     IdleColor;                     //idle percent text color                                      FAR_COLOR(fccCYAN,fccBLUE)
	int     IdleMode;                      //Show mode of idle info (set of IDLE_xxx flags)               IDLE_CONSOLE
	int     ProcessColor;                  //color of processing string in quite mode                     FAR_COLOR(fccBLACK,fccLIGHTGRAY) | COL_DIALOGBOX
	ServerTypePtr    defaultServerType_;   //Type of server
	BOOL    FFDup;                         //Duplicate FF symbols on transfer to server
	BOOL    UndupFF;                       //Remove FF duplicate from PWD
	bool    ShowSilentProgress;            //Show normal progress on silent operations
	BOOL    ProcessCmd;                    //Default for command line processing
	BOOL    UseBackups;                    //Use FTP backups
	int     IdleShowPeriod;                //Period to refresh idle state (ms)
	int     IdleStartPeriod;               //Period before first idle message shown (ms)
	BOOL    AutoAnonymous;                 //Fill blank name with "anonimous"
	int     RetryTimeout;                  //Timeout of auto-retry (sec)
	BOOL    DoNotExpandErrors;             //Do not expand CMD window on error
	int     AskLoginFail;                  //Reask user name and password if login fail                   TRUE
	std::wstring defaultPassword_;

//Configurable
	std::wstring	CmdLogFile;  //Log file where FTP commands placed                           "" (none)
  int     CmdLogLimit;                   //Limit of cmd log file (*1000 bytes)                          100 (100.000 bytes)
	bool	CloseDots;                     //Switch on ".." to hosts                                      TRUE
  BOOL    QuoteClipboardNames;           //Quote names placed to clipboard
  BOOL    SetHiddenOnAbort;              //Set hidden attribute on uncomplete files

//Techinfos
  int		PwdSecurity;                   //Set of SEC_xxx

  std::wstring	fmtDateFormat;           //Server date-time format                                      "%*s %04d%02d%02d%02d%02d%02d"
  BOOL			TruncateLogFile;               //Truncate log file on plugin start                            FALSE
  std::wstring	InvalidSymbols;
  std::wstring	CorrectedSymbols;

//Queque processing
  BOOL    RestoreState;
  BOOL    RemoveCompleted;

  SaveListInfo sli;

	bool			showPassword_;                 //Show paswords in any places                                  FALSE

	std::wstring	cmdPut;                  //"STOR" /*APPE*/
	std::wstring	cmdAppe;                 //"APPE"
	std::wstring	cmdStor;                 //"STOR"
	std::wstring	cmdPutUniq;              //"STOU"
	std::wstring	cmdPasv_;				//"PASV"
	std::wstring	cmdPort;                 //"PORT"
	std::wstring	cmdMDTM;                 //"MDTM"
	std::wstring	cmdRetr;                 //"RETR"
	std::wstring	cmdRest_;					//"REST"
	std::wstring	cmdAllo;                 //"ALLO"
	std::wstring	cmdCwd;                  //"CWD"
	std::wstring	cmdXCwd;                 //"XCWD"
	std::wstring	cmdDel;                  //"DELE"
	std::wstring	cmdRen;                  //"RNFR"
	std::wstring	cmdRenTo;                //"RNTO"
	std::wstring	cmdList;                 //"LIST"
	std::wstring	cmdNList;                //"NLIST"
	std::wstring	cmdUser_;					//"USER"
	std::wstring	cmdPass_;					//"PASS"
	std::wstring	cmdAcct_;					//"ACCT"
	std::wstring	cmdPwd;                  //"PWD"
	std::wstring	cmdXPwd;                 //"XPWD"
	std::wstring	cmdMkd;                  //"MKD"
	std::wstring	cmdXMkd;                 //"XMKD"
	std::wstring	cmdRmd;                  //"RMD"
	std::wstring	cmdXRmd;                 //"XRMD"
	std::wstring	cmdSite;                 //"SITE"
	std::wstring	cmdChmod;                //"CHMOD"
	std::wstring	cmdUmask;                //"UMASK"
	std::wstring	cmdIdle;                 //"IDLE"
	std::wstring	cmdHelp;                 //"HELP"
	std::wstring	cmdQuit;                 //"QUIT"
	std::wstring	cmdCDUp;                 //"CDUP"
	std::wstring	cmdXCDUp;                //"XCUP"
	std::wstring	cmdSyst;                 //"SYST"
	std::wstring	cmdSize;                 //"SISE"
	std::wstring	cmdStat;                 //"STAT"
};

#endif