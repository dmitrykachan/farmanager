#ifndef __FAR_PLUGIN_FTP_FTP
#define __FAR_PLUGIN_FTP_FTP

enum
{
	FTP_COL_MODE,
	FTP_COL_LINK,
	FTP_COL_MAX
};

#define FTP_MAXBACKUPS 10

struct FTPUrl
{
	FTPHost       Host;
	std::wstring	SrcPath;
	std::wstring	DestPath;
	String        Error;
	std::wstring  fileName_;
	bool          Download;
	FTPUrl*       Next;
};

struct FTPCopyInfo
{
  BOOL      asciiMode;
  BOOL      ShowProcessList;
  BOOL      AddToQueque;
  overCode  MsgCode;
  std::wstring    DestPath;
  String    SrcPath;   //Used only on queue processing
  BOOL      Download;
  BOOL      UploadLowCase;
  BOOL      FTPRename;

  FTPCopyInfo( void );
};

struct QueueExecOptions
{

  BOOL      RestoreState;
  BOOL      RemoveCompleted;
};

class FTP
{
    friend class FTPCmdBlock;
	std::wstring	selectFile_;
    BOOL        ResetCache;
    int         ShowHosts;
    int         SwitchingToFTP;
	std::wstring	hostsPath_;
    int         StartViewMode;
    int         RereadRequired;
    FTPCurrentStates CurrentState;
    char        IncludeMask[ FAR_MAX_PATHSIZE ];
    char        ExcludeMask[ FAR_MAX_PATHSIZE ];
    bool		PluginColumnModeSet;
    int         ActiveColumnMode;
    BOOL        NeedToSetActiveMode;
    FTPUrl*     UrlsList, *UrlsTail;
    int         QuequeSize;
    overCode    LastMsgCode,
                OverrideMsgCode;
  public:
    FTPHost     Host;
    char        PanelTitle[512];
    HANDLE      LongBeep;
    HANDLE      KeepAlivePeriod;
    Connection *hConnect;
    int         CallLevel;
  private:
	std::map<size_t, FTPHost> hosts_;


    int       Connect();
    void      CopyNamesToClipboard( void );
    int       DeleteFilesINT(PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
    int       GetFreeKeyNumber();
    void      GetFullFileName(char *FullName,char *Name);
    void      GetFullKey(char *FullKeyName,CONSTSTR Name);
	int       GetHostFiles(struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move, std::wstring& DestPath,int OpMode);
    void      GetNewKeyName(char *FullKeyName);
    int       HexToNum(int Hex);
    void      HexToPassword(char *HexStr,char *Password);
    void      MakeKeyName(char *FullKeyName,int Number);
    void      PasswordToHex(char *Password,char *HexStr);
    int       PutHostsFiles(struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move,int OpMode);
    void      SaveURL();
	boost::shared_ptr<ServerType> SelectServerType(boost::shared_ptr<ServerType> defType);
    UINT      SelectTable(UINT codePage);
    void      SetAttributes();
    int       TableNameToValue(char *TableName);
  private:
	  bool      EditDirectory(std::wstring& Name, std::wstring &Desc,BOOL newDir );
    void      FTP_FixPaths( CONSTSTR base,PluginPanelItem *p,int cn,BOOL FromPlugin );
    void      FTP_FreeFindData( PluginPanelItem *PanelItem,int ItemsNumber,BOOL FromPlugin );
    BOOL      FTP_GetFindData( PluginPanelItem **PanelItem,int *ItemsNumber,BOOL FromPlugin );
	bool		AddWrapper(FP_SizeItemList& il, PluginPanelItem &p, std::string& description, 
							std::string& host, std::string& directory, std::string& username);

	bool      FTP_SetDirectory(const std::wstring &dir, bool FromPlugin);
	int       GetFilesInterface(struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move, std::wstring& DestPath,int OpMode);
    BOOL      GetHost( int title,FTPHost* p,BOOL ToDescription );
    BOOL      Reread( void );
    int       PutFilesINT(struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move,int OpMode);
    void      SaveUsedDirNFile( void );
    BOOL      ExecCmdLine( CONSTSTR str, BOOL Prefix );
    BOOL      ExecCmdLineFTP( CONSTSTR str, BOOL Prefix );
    BOOL      ExecCmdLineANY( CONSTSTR str, BOOL Prefix );
    BOOL      ExecCmdLineHOST( CONSTSTR str, BOOL Prefix );
    BOOL      DoCommand( CONSTSTR str, int type, DWORD flags );
    BOOL      DoFtpConnect( int blocked );

	int       ExpandListINT(struct PluginPanelItem *PanelItem,size_t ItemsNumber,FP_SizeItemList* il,BOOL FromPlugin,ExpandListCB cb = NULL,LPVOID Param = NULL );
    int       ExpandList(struct PluginPanelItem *PanelItem, size_t ItemsNumber,FP_SizeItemList* il,BOOL FromPlugin,ExpandListCB cb = NULL,LPVOID Param = NULL );
    BOOL      CopyAskDialog( BOOL Move, BOOL Download,FTPCopyInfo* ci );
    BOOL      ShowFilesList( FP_SizeItemList* il );
    overCode  AskOverwrite( int title,BOOL Download,WIN32_FIND_DATA* dest,WIN32_FIND_DATA* src,overCode last );
    void      BackToHosts( void );
    BOOL      FullConnect();
    void      SaveList( FP_SizeItemList* il );
	bool      SetDirectoryStepped(const std::wstring &Dir, bool update);
    void      InsertToQueue( void );
    CONSTSTR  InsertCurrentToQueue( void );
    CONSTSTR  InsertAnotherToQueue( void );
	bool      CheckDotsBack(const std::wstring &OldDir, const std::wstring& CmdDir);
	bool      FTPCreateDirectory(const std::wstring &dir, int OpMode);

    int       _FtpGetFile( CONSTSTR lpszRemoteFile,CONSTSTR lpszNewFile,BOOL Reget,int AsciiMode );
	int       _FtpPutFile(const std::wstring &localFile, const std::wstring &remoteFile, bool Reput, int AsciiMode);
  public:
    FTP();
    ~FTP();
    int       DeleteFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
    void      FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
	int       GetFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,int Move, std::wstring& DestPath,int OpMode);
    int       GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
    void      GetOpenPluginInfo(struct OpenPluginInfo *Info);
	int       MakeDirectory(std::wstring& Name,int OpMode);
    int       ProcessCommandLine(char *CommandLine);
    int       ProcessEvent(int Event,void *Param);
    int       ProcessKey(int Key,unsigned int ControlState);
    int       ProcessShortcutLine(char *Line);
    int       PutFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode);
	int       SetDirectory(const std::wstring &Dir,int OpMode);
	int       SetDirectoryFAR(const std::wstring& Dir, int OpMode);

    void      Invalidate( void );
	std::wstring GetCurPath();

    void      LongBeepEnd( BOOL DoNotBeep = FALSE );
    void      LongBeepCreate( void );
    BOOL      HostsMode( void )         { return ShowHosts && !SwitchingToFTP; }
    BOOL      FTPMode( void )           { return !HostsMode() && hConnect; }

    static FTP *Backups[ FTP_MAXBACKUPS ];
    static int  BackupCount;

    void      SetBackupMode( void );
    void      SetActiveMode( void );
    BOOL      isBackup( void );
    void      DeleteFromBackup( void );
    void      AddToBackup( void );

    CONSTSTR  CloseQuery( void );

    FTPUrl*   UrlItem( int num, FTPUrl* *prev );
    void      UrlInit( FTPUrl* p );
    void      DeleteUrlItem( FTPUrl* p, FTPUrl* prev );
    BOOL      EditUrlItem( FTPUrl* p );

    void      AddToQueque( WIN32_FIND_DATA* FileName, CONSTSTR Path, BOOL Download );
    void      AddToQueque( FTPUrl* p,int pos = -1 );
    void      ListToQueque( FP_SizeItemList* il,FTPCopyInfo* ci );
    void      ClearQueue( void );

    void      SetupQOpt( QueueExecOptions* op );
    BOOL      WarnExecuteQueue( QueueExecOptions* op );
    void      QuequeMenu( void );
    void      ExecuteQueue( QueueExecOptions* op );
    void      ExecuteQueueINT( QueueExecOptions* op );

    void Call( void );
    void End( int rc = -156 );

	std::wstring getHostPath() const
	{
		return hostsPath_;
	}

	const FTPHost* findhost(size_t id) const
	{
		std::map<size_t, FTPHost>::const_iterator itr = hosts_.find(id);
		if(itr == hosts_.end())
			return 0;
		return &(itr->second);
	}

	
	FTPHost* findhost(size_t id)
	{
		std::map<size_t, FTPHost>::iterator itr = hosts_.find(id);
		if(itr == hosts_.end())
			return 0;
		return &itr->second;
	}
	int processArrowKeys(int key, int &res);
	int DisplayUtilsMenu();
};


extern std::vector<unsigned char> MakeCryptPassword(const std::wstring &src);
extern std::wstring DecryptPassword(const std::vector<unsigned char> &crypt);
extern std::wstring DecryptPassword_old(const std::vector<unsigned char> &crypt);


#endif