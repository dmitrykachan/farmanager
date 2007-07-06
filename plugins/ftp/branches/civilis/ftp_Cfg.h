#ifndef __FAR_PLUGIN_CONFIGDATA
#define __FAR_PLUGIN_CONFIGDATA

//Traffic message [ftp_TraficCB.cpp]
#include "ftp_pwd.h"
#define FTR_MAXTIME           FTR_HOURSEC*99        //max calculated estimated time (sec)
#define FTR_MINBUFFSIZE       50

//Dialog edit history
#define FTP_GETHISTORY        "FTPGet"
#define FTP_PUTHISTORY        "FTPPut"
#define FTP_HOSTHISTORY       "FTPHost"
#define FTP_FOLDHISTORY       "FTPFolder"
#define FTP_USERHISTORY       "FTPUser"
#define FTP_INCHISTORY        "FTPIncludeMask"
#define FTP_EXCHISTORY        "FTPExcludeMask"

#define FTP_CMDPREFIX         L"ftp"
#define FTP_CMDPREFIX_SIZE    3

#define FTP_HOSTID            MK_ID( 'F','H','s','t' )

#define FTR_HOURSEC           3600
#define FTR_MINSEC            60

#define MAXHOSTNAMELEN        64

#define DIALOG_EDIT_SIZE      8000  //Size of max dialog edit field


class PluginPanelItemEx: public PluginPanelItem
{
public:
	PluginPanelItemEx()
	{
		std::fill_n(reinterpret_cast<int*>(this), sizeof(PluginPanelItem)/4, 0);
	}

	PluginPanelItemEx(const PluginPanelItemEx& p)
	{
		FindData		= p.FindData;
		PackSizeHigh	= p.PackSizeHigh;
		PackSize		= p.PackSize;
		Flags			= p.Flags;
		NumberOfLinks	= p.NumberOfLinks;
		CRC32			= p.CRC32;
		Reserved[0]		= p.Reserved[0];
		Reserved[1]		= p.Reserved[1];
		UserData		= p.UserData;

		Description		= strdup(p.Description);
		Owner			= strdup(p.Owner);

		if(p.CustomColumnNumber)
		{
			CustomColumnNumber = p.CustomColumnNumber;
			CustomColumnData = new char* [CustomColumnNumber];
			for(int i = 0; i < CustomColumnNumber; i++)
			{
				CustomColumnData[i] = strdup(p.CustomColumnData[i]);
			}
		} else
		{
			CustomColumnData = 0;
			CustomColumnNumber = 0;
		}
	}

	~PluginPanelItemEx()
	{
		for(int i = 0; i < CustomColumnNumber; i++)
		{
			free(CustomColumnData[i]);
		}
		delete [] CustomColumnData;
		free(Owner);
		free(Description);
	}

};



inline std::wstring FTP_FILENAME(const PluginPanelItem *p)
{
	return FPIL_ADDEXIST(p) ? FPIL_ADD_STRING(p) : Unicode::utf8ToUtf16(p->FindData.cFileName);
}

inline void SET_FTP_FILENAME(PluginPanelItem *p, const std::wstring &str)
{
	const char* s = Unicode::utf16ToUtf8(str).c_str();
	strncpy(p->FindData.cFileName, s, sizeof(p->FindData.cFileName)-1);
	p->FindData.cFileName[sizeof(p->FindData.cFileName)-1] = '\0';
	if(str.size() > sizeof(p->FindData.cFileName)-1)
	{
		FPIL_ADDSET(p, str);
	}
}


//Security flags
#define SEC_PLAINPWD          0x0001        //plugin can place password in generated url in a plain text

typedef char*                 FCmdLine;
typedef const char*           FMenuLine;

//transfer types
enum ftTypes {
  TYPE_A = 'A',
  TYPE_I = 'I',
  TYPE_E = 'E',
  TYPE_L = 'L',
  TYPE_NONE = 0
};

//Socket wait state
enum {
 ws_connect,
 ws_read,
 ws_write,
 ws_accept,
 ws_error
};

//FTP overwrite mode
enum overCode {
  ocOverAll,
  ocSkip,
  ocSkipAll,
  ocOver,
  ocResume,
  ocResumeAll,
  ocCancel,
  ocNone
};

//Current FTP state
enum FTPCurrentStates {
  fcsNormal       = 0,
  fcsExpandList   = 1,
  fcsClose        = 2,
  fcsConnecting   = 3,
  fcsFTP          = 4,
  fcsOperation    = 5,
  fcsProcessFile  = 6,
  fcs_None
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
//ExpandList callback
typedef BOOL (*ExpandListCB)(const FTP* ftp, PluginPanelItem *p,LPVOID Param );

//Save list options
enum sliTypes {
  sltUrlList,
  sltTree,
  sltGroup,
  sltNone
};

struct SaveListInfo
{

  char     path[ FAR_MAX_PATHSIZE ];
  BOOL     Append;
  BOOL     AddPrefix;             //all
  BOOL     AddPasswordAndUser;
  BOOL     Quote;
  BOOL     Size;                  //tree and group
  int      RightBound;            //tree
  sliTypes ListType;
};

//Configuration options
struct Options : public OptionsPlugin
{

	std::wstring defaultPassword_;

//Configurable
	std::wstring CmdLogFile;  //Log file where FTP commands placed                           "" (none)
  int     CmdLogLimit;                   //Limit of cmd log file (*1000 bytes)                          100 (100.000 bytes)
  BOOL    CloseDots;                     //Switch on ".." to hosts                                      TRUE
  BOOL    QuoteClipboardNames;           //Quote names placed to clipboard
  BOOL    SetHiddenOnAbort;              //Set hidden attribute on uncomplete files

//Techinfos
  DWORD   PwdSecurity;                   //Set of SEC_xxx

  std::string	fmtDateFormat;           //Server date-time format                                      "%*s %04d%02d%02d%02d%02d%02d"
  BOOL			TruncateLogFile;               //Truncate log file on plugin start                            FALSE
  std::wstring	InvalidSymbols;
  std::wstring	CorrectedSymbols;

//Queque processing
  BOOL    RestoreState;
  BOOL    RemoveCompleted;

  SaveListInfo sli;

  BOOL    _ShowPassword;                 //Show paswords in any places                                  FALSE

	std::string	cmdPut;                  //"STOR" /*APPE*/
	std::string	cmdAppe;                 //"APPE"
	std::string	cmdStor;                 //"STOR"
	std::string	cmdPutUniq;              //"STOU"
	std::string	cmdPasv_;					//"PASV"
	std::string	cmdPort;                 //"PORT"
	std::string	cmdMDTM;                 //"MDTM"
	std::string	cmdRetr;                 //"RETR"
	std::string	cmdRest_;					//"REST"
	std::string	cmdAllo;                 //"ALLO"
	std::string	cmdCwd;                  //"CWD"
	std::string	cmdXCwd;                 //"XCWD"
	std::string	cmdDel;                  //"DELE"
	std::string	cmdRen;                  //"RNFR"
	std::string	cmdRenTo;                //"RNTO"
	std::string	cmdList;                 //"LIST"
	std::string	cmdNList;                //"NLIST"
	std::string	cmdUser_;					//"USER"
	std::string	cmdPass_;					//"PASS"
	std::string	cmdAcct_;					//"ACCT"
	std::string	cmdPwd;                  //"PWD"
	std::string	cmdXPwd;                 //"XPWD"
	std::string	cmdMkd;                  //"MKD"
	std::string	cmdXMkd;                 //"XMKD"
	std::string	cmdRmd;                  //"RMD"
	std::string	cmdXRmd;                 //"XRMD"
	std::string	cmdSite;                 //"SITE"
	std::string	cmdChmod;                //"CHMOD"
	std::string	cmdUmask;                //"UMASK"
	std::string	cmdIdle;                 //"IDLE"
	std::string	cmdHelp;                 //"HELP"
	std::string	cmdQuit;                 //"QUIT"
	std::string	cmdCDUp;                 //"CDUP"
	std::string	cmdXCDUp;                //"XCUP"
	std::string	cmdSyst;                 //"SYST"
	std::string	cmdSize;                 //"SISE"
	std::string	cmdStat;                 //"STAT"
};

#endif