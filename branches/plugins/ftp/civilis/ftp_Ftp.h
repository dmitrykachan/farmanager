#ifndef __FAR_PLUGIN_FTP_FTP
#define __FAR_PLUGIN_FTP_FTP

#include "panelview.h"
#include "ftpfileview.h"

struct FTPUrl
{
	FtpHostPtr		pHost_;
	std::wstring	SrcPath;
	std::wstring	DestPath;
	std::wstring	Error;
	std::wstring  fileName_;
	bool			Download;
};

typedef boost::shared_ptr<FTPUrl> FTPUrlPtr;

struct FTPCopyInfo: boost::noncopyable
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

struct QueueExecOptions: boost::noncopyable
{
	bool RestoreState;
	bool RemoveCompleted;
};

class FTP;

class Backup: boost::noncopyable
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


struct TotalFiles: boost::noncopyable
{
	unsigned __int64 size_;
	unsigned __int64 files_;
};

class FTP: boost::noncopyable
{
    friend class FTPCmdBlock;
private:
    bool         ShowHosts;
    int			startViewMode_;
    int         ActiveColumnMode;
    BOOL        NeedToSetActiveMode;
	std::vector<FTPUrlPtr> urlsQueue_;

	HostView	hostPanel_;
	FTPFileView	FtpFilePanel_;

public:
	PanelView*			panel_;

	std::wstring panelTitle_;
	WinAPI::Stopwatch	longBeep_;
    int         CallLevel;

	const HostView* getHostPanel() const
	{
		return &hostPanel_;
	}
	
	bool getCurrentPanelItem(PluginPanelItem &item) const
	{
		FARWrappers::PanelInfoAuto  pi(this, false);

		if(pi.ItemsNumber > 0 && pi.CurrentItem < pi.ItemsNumber)
		{
			item = pi.PanelItems[pi.CurrentItem];
			return true;
		} else
			return false;
	}
	std::wstring getCurrentFile() const
	{
		FARWrappers::PanelInfoAuto  pi(this, false);
		if(pi.ItemsNumber > 0 && pi.CurrentItem < pi.ItemsNumber)
		{
			return pi.PanelItems[pi.CurrentItem].FindData.lpwszFileName;
		}
		return L"";
	}

	bool		EditHostDlg(int title, FtpHostPtr& p, bool ToDescription);
	bool		EditDirectory(std::wstring& Name, std::wstring &Desc, bool newDir, bool hostMode);
	bool		ExecCmdLine(const std::wstring& str, bool wasPrefix);
	static UINT	SelectTable(UINT codePage);
	static boost::shared_ptr<ServerType> SelectServerType(boost::shared_ptr<ServerType> defType);

	Connection& getConnection()
	{
		return FtpFilePanel_.getConnection();
	}
	const Connection& getConnection() const
	{
		return FtpFilePanel_.getConnection();
	}
	int       Connect(FtpHostPtr& host);

private:
    bool      ExecCmdLineFTP(const std::wstring& str, bool Prefix);
	bool      ExecCmdLineANY(const std::wstring& str, bool Prefix);
    bool      ExecCmdLineHOST(const std::wstring& str, bool Prefix);
	bool		DoCommand(const std::wstring& str, int type, DWORD flags);
    bool		DoFtpConnect(FtpHostPtr& host);

    void      InsertToQueue( void );
    const wchar_t*  InsertSelectedToQueue(bool activePanel);

  public:
    FTP();
    ~FTP();

	BOOL      FullConnect(FtpHostPtr& host);

    void      GetOpenPluginInfo(struct OpenPluginInfo *Info);
    int       ProcessCommandLine(wchar_t *CommandLine);
    int       ProcessKey(int Key,unsigned int ControlState);
    int       ProcessShortcutLine(char *Line);

    void      Invalidate(bool clearSelection = false);
	void      BackToHosts();

    void      LongBeepEnd(bool DoNotBeep = false);
    void      LongBeepCreate( void );
    bool      FTPMode() const
	{
		return !ShowHosts && getConnection().isConnected();
	}

	static	Backup	backups_;

	void      SetBackupMode( void );
    void      SetActiveMode( void );

    const wchar_t*  CloseQuery();

    FTPUrlPtr UrlItem(size_t num);
    void      UrlInit(FTPUrlPtr &p);
    void      DeleteUrlItem(size_t index);
    bool      EditUrlItem(FTPUrlPtr& p);

	void      AddToQueque(FTPFileInfo& file, const std::wstring& path, bool Download);
    void      AddToQueque(FTPUrlPtr &pUrl);
    void      ListToQueque(const FileList& il, const FTPCopyInfo& ci);
    void      ClearQueue();

    void      SetupQOpt( QueueExecOptions* op );
    bool      WarnExecuteQueue( QueueExecOptions* op );
    void      QuequeMenu( void );
    void      ExecuteQueue( QueueExecOptions* op );
    void      ExecuteQueueINT( QueueExecOptions* op );
	bool      CopyAskDialog(bool Move, bool Download, FTPCopyInfo* ci);
	overCode  AskOverwrite(int title, BOOL Download, const FTPFileInfo& dest, const FTPFileInfo& src,overCode last);


    void Call( void );
    void End( int rc = -156 );

	const FtpHostPtr& getSelectedHost() const
	{
		PluginPanelItem item;
		if(getCurrentPanelItem(item) && item.UserData != HostView::ParentDirHostID)
			return hostPanel_.findhost(item.UserData);
		else
			return emptyHost_;
	}

	FtpHostPtr& getSelectedHost()
	{
		PluginPanelItem item;
		if(getCurrentPanelItem(item) && item.UserData != HostView::ParentDirHostID)
			return hostPanel_.findhost(item.UserData);
		else
			return emptyHost_;
	}

	bool processArrowKeys(int key);
	int DisplayUtilsMenu();

private:
	FtpHostPtr	emptyHost_;
};



extern std::vector<unsigned char> MakeCryptPassword(const std::wstring &src);
extern std::wstring DecryptPassword(const std::vector<unsigned char> &crypt);


bool SayNotReadedTerminates(const std::wstring& fnm, bool& SkipAll);


#endif