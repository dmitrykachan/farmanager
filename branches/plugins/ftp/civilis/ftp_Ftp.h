#ifndef __FAR_PLUGIN_FTP_FTP
#define __FAR_PLUGIN_FTP_FTP

#include "panelview.h"
#include "ftpfileview.h"

struct FTPUrl
{
	FTPHost       Host;
	std::wstring	SrcPath;
	std::wstring	DestPath;
	std::wstring	Error;
	std::wstring  fileName_;
	BOOL          Download;
	FTPUrl*       Next;
};

struct FTPCopyInfo
{
	bool	asciiMode;
	bool	showProcessList;
	bool	addToQueque;
	overCode msgCode;
	std::wstring    destPath;
	std::wstring    srcPath;   //Used only on queue processing
	bool	download;
	bool	uploadLowCase;
	bool	FTPRename;

	FTPCopyInfo();
};

struct QueueExecOptions
{
  BOOL      RestoreState;
  BOOL      RemoveCompleted;
};

class FTP;

class Backup
{
public:
	typedef std::vector<FTP*>::iterator iterator;
	typedef std::vector<FTP*>::const_iterator const_iterator;

	void			add(FTP* ftp);
	bool			find(FTP* ftp);
	void			remove(FTP* ftp);
	void			eraseAll();
	iterator		begin()			{return array_.begin();}
	const_iterator	begin() const	{return array_.begin();}
	iterator		end()			{return array_.end();}
	const_iterator	end() const		{return array_.end();}
	size_t			size() const	{return array_.size();}
	FTP*			get(size_t n) const {return array_[n];}

private:
	std::vector<FTP*> array_;
};


struct TotalFiles
{
	unsigned __int64 size_;
	unsigned __int64 files_;
};

class FTP
{
    friend class FTPCmdBlock;
private:
    bool        resetCache_;
    int         ShowHosts;
    int         SwitchingToFTP;
    int				startViewMode_;
    int         RereadRequired;
    int         ActiveColumnMode;
    BOOL        NeedToSetActiveMode;
    FTPUrl*     UrlsList, *UrlsTail;
    int         QuequeSize;

	HostView	hostPanel_;
	FTPFileView	FtpFilePanel_;

public:
	bool				PluginColumnModeSet; // TODO
	FTPCurrentStates	CurrentState;
	PanelView*			panel_;
	std::wstring		selectFile_;

    FTPHost     chost_;
	std::wstring panelTitle_;
	WinAPI::Stopwatch	longBeep_;
    int         CallLevel;

	bool getPanelInfo(PanelInfo &pi) const
	{
		return FARWrappers::getInfo().Control(const_cast<FTP*>(this), FCTL_GETPANELINFO, &pi) != 0;
	}

	const HostView* getHostPanel() const
	{
		return &hostPanel_;
	}
	
	bool getCurrentPanelItem(PluginPanelItem &item) const
	{
		PanelInfo pi;
		getPanelInfo(pi);
		if(pi.ItemsNumber > 0 && pi.CurrentItem < pi.ItemsNumber)
		{
			item = pi.PanelItems[pi.CurrentItem];
			return true;
		} else
			return false;
	}
	std::wstring getCurrentFile() const
	{
		PanelInfo pi;
		getPanelInfo(pi);

		if(pi.ItemsNumber > 0 && pi.CurrentItem < pi.ItemsNumber)
		{
			return pi.PanelItems[pi.CurrentItem].FindData.lpwszFileName;
		}
		return L"";
	}

	bool		GetHost(int title,FTPHost* p, bool ToDescription);
	bool		EditDirectory(std::wstring& Name, std::wstring &Desc, bool newDir, bool hostMode);
	bool		ExecCmdLine(const std::wstring& str, bool wasPrefix);
	UINT		SelectTable(UINT codePage);

	Connection& getConnection()
	{
		return FtpFilePanel_.getConnection();
	}
	int       Connect();

private:
    int       GetFreeKeyNumber();
    void      GetFullFileName(char *FullName,char *Name);
	int       GetHostFiles(struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move, std::wstring& DestPath,int OpMode);
    void      GetNewKeyName(char *FullKeyName);
    int       HexToNum(int Hex);
    void      HexToPassword(char *HexStr,char *Password);
    void      MakeKeyName(char *FullKeyName,int Number);
    void      PasswordToHex(char *Password,char *HexStr);
	boost::shared_ptr<ServerType> SelectServerType(boost::shared_ptr<ServerType> defType);
    int       TableNameToValue(char *TableName);

	void      FTP_FixPaths(const std::wstring &base, FARWrappers::ItemList &items, BOOL FromPlugin);
    bool      FTP_GetFindData(FARWrappers::ItemList &items, bool FromPlugin);
	bool		AddWrapper(FARWrappers::ItemList& il, PluginPanelItem &p, std::wstring& description, 
							std::wstring& host, std::wstring& directory, std::wstring& username);

	bool      FTP_SetDirectory(const std::wstring &dir, bool FromPlugin);
    int       PutFilesINT(struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move,int OpMode);
    bool      ExecCmdLineFTP(const std::wstring& str, bool Prefix);
	bool      ExecCmdLineANY(const std::wstring& str, bool Prefix);
    bool      ExecCmdLineHOST(const std::wstring& str, bool Prefix);
	bool		DoCommand(const std::wstring& str, int type, DWORD flags);
    BOOL      DoFtpConnect( int blocked );

	int       ExpandListINT(FARWrappers::ItemList &panelItems, FARWrappers::ItemList* il, TotalFiles* totalFiles, bool FromPlugin, LPVOID Param = NULL);
    int       ExpandList(FARWrappers::ItemList &panelItem, FARWrappers::ItemList* il, bool FromPlugin, LPVOID Param = NULL );
    BOOL      ShowFilesList(FARWrappers::ItemList* il );
    void      SaveList(FARWrappers::ItemList* il );
    void      InsertToQueue( void );
    const wchar_t*  InsertCurrentToQueue( void );
    const wchar_t*  InsertAnotherToQueue( void );

	int       _FtpPutFile(const std::wstring &localFile, const std::wstring &remoteFile, bool Reput, int AsciiMode);
  public:
    FTP();
    ~FTP();

	BOOL      FullConnect();

    void      GetOpenPluginInfo(struct OpenPluginInfo *Info);
    int       ProcessCommandLine(wchar_t *CommandLine);
    int       ProcessKey(int Key,unsigned int ControlState);
    int       ProcessShortcutLine(char *Line);
    int       PutFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode);
	bool      SetDirectoryStepped(const std::wstring &Dir, bool update);
	void      SaveUsedDirNFile();

    void      Invalidate(bool clearSelection = false);
	void      BackToHosts();

    void      LongBeepEnd(bool DoNotBeep = false);
    void      LongBeepCreate( void );
    BOOL      HostsMode( void )         { return ShowHosts && !SwitchingToFTP; }
    BOOL      FTPMode( void )           { return !HostsMode() && getConnection().isConnected(); }

	static	Backup	backups_;

	void      SetBackupMode( void );
    void      SetActiveMode( void );

    const wchar_t*  CloseQuery();

    FTPUrl*   UrlItem( int num, FTPUrl* *prev );
    void      UrlInit( FTPUrl* p );
    void      DeleteUrlItem( FTPUrl* p, FTPUrl* prev );
    bool      EditUrlItem( FTPUrl* p );

	void      AddToQueque(FTPFileInfo& fileName, const std::wstring& path, bool Download);
    void      AddToQueque(FTPUrl* p,int pos = -1);
    void      ListToQueque(const FileList& il, const FTPCopyInfo& ci);
    void      ClearQueue( void );

    void      SetupQOpt( QueueExecOptions* op );
    bool      WarnExecuteQueue( QueueExecOptions* op );
    void      QuequeMenu( void );
    void      ExecuteQueue( QueueExecOptions* op );
    void      ExecuteQueueINT( QueueExecOptions* op );
	bool      CopyAskDialog(bool Move, bool Download, FTPCopyInfo* ci);
	overCode  AskOverwrite(int title, BOOL Download, FTPFileInfo* dest, const FTPFileInfo* src,overCode last);


    void Call( void );
    void End( int rc = -156 );

	const FTPHost* getCurrentHost() const
	{
		PluginPanelItem item;
		if(getCurrentPanelItem(item))
			return hostPanel_.findhost(item.UserData);
		else
			return 0;
	}

	FTPHost* getCurrentHost()
	{
		PluginPanelItem item;
		if(getCurrentPanelItem(item))
			return hostPanel_.findhost(item.UserData);
		else
			return 0;
	}

	bool processArrowKeys(int key);
	int DisplayUtilsMenu();
};



extern std::vector<unsigned char> MakeCryptPassword(const std::wstring &src);
extern std::wstring DecryptPassword(const std::vector<unsigned char> &crypt);


bool SayNotReadedTerminates(const std::wstring& fnm, bool& SkipAll);


#endif