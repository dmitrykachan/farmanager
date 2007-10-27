#ifndef PANEL_VIEW_HEADER
#define PANEL_VIEW_HEADER

class FTP;

#include "ftp_JM.h"
#include "ftp_connect.h"

#include "utils/cache.h"

class PanelView: boost::noncopyable
{
public:
	virtual int GetFiles(
		FARWrappers::ItemList &panelItems, int Move, 
		const std::wstring &destPath, int OpMode) = 0;
	
	virtual int PutFiles(
		PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode) = 0;

	virtual int Compare(
		const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode) = 0;

	virtual bool DeleteFiles(
		const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode) = 0;

	virtual int MakeDirectory(std::wstring &name, int OpMode) = 0;

	virtual int ProcessEvent(int Event, void *Param) = 0;
	
	virtual int ProcessKey(int Key, unsigned int ControlState) = 0;

	virtual int SetDirectory(const std::wstring &Dir, int OpMode) = 0;

	virtual int GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode) = 0;

	virtual void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber) = 0;

	virtual const std::wstring getCurrentDirectory() const = 0;
	virtual void setCurrentDirectory(const std::wstring& dir) = 0;
};

class HostView: public PanelView
{
public:
	HostView()
		: plugin_(0) {}

	void setPlugin(FTP* plugin)
	{
		plugin_ = plugin;
	}

	virtual int GetFiles(FARWrappers::ItemList &panelItems, int Move, 
							const std::wstring &destPath, int OpMode);
	virtual int PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode);
	virtual int Compare(const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode);
	virtual bool DeleteFiles(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	virtual int MakeDirectory(std::wstring &name, int OpMode);
	virtual int ProcessEvent(int Event, void *Param);
	virtual int ProcessKey(int Key, unsigned int ControlState);
	virtual int SetDirectory(const std::wstring &Dir, int OpMode);
	virtual int GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber, int OpMode);
	virtual void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
	virtual const std::wstring getCurrentDirectory() const;
	virtual void setCurrentDirectory(const std::wstring& dir);

	const FTPHost* findhost(size_t id) const
	{
		if(id > 0 && id <= hosts_.size())
			return hosts_[id-1].get();
		else
			return 0;
	}

	FTPHost* findhost(size_t id)
	{
		if(id > 0 && id <= hosts_.size())
			return hosts_[id-1].get();
		else
			return 0;
	}

private:
	typedef std::vector<boost::shared_ptr<FTPHost> > HostList;
	enum CallbackStatus
	{
		OK, Skip, Cancel
	};
	static const int FTP_HOST_COL_COUNT = 3;
	typedef CallbackStatus (*WalkListCallBack)(bool enter, FTPHost &host, const std::wstring& path, void* param);

	void	readFolder(const std::wstring& dir, HostList &hostList);
	bool	walk(const PluginPanelItem* panelItems, int itemsNumber, const std::wstring &path, WalkListCallBack callback, void* param);
	bool	walk(FTPHost& host, const std::wstring &path, WalkListCallBack callback, void* param);
	static CallbackStatus deleteCallback(bool enter, FTPHost &host, const std::wstring& path, void* param);

	FTP*			plugin_;
	std::wstring	hostsPath_;

	std::vector<boost::shared_ptr<FTPHost> >	hosts_;
	FARWrappers::ItemList						itemList_;
	boost::scoped_array<wchar_t*>				columndata_;

};


class FTPFileView: public PanelView
{
public:
	FTPFileView()
		: plugin_(0) {}

	void setPlugin(FTP* plugin)
	{
		plugin_ = plugin;
	}

	virtual int		GetFiles(FARWrappers::ItemList &panelItems, int Move, 
		const std::wstring &destPath, int OpMode);
	virtual int		PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode);
	virtual int		Compare(const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode);
	virtual bool	DeleteFiles(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	virtual int		MakeDirectory(std::wstring &name, int OpMode);
	virtual int		ProcessEvent(int Event, void *Param);
	virtual int		ProcessKey(int Key, unsigned int ControlState);
	virtual int		SetDirectory(const std::wstring &dir, int OpMode);
	virtual int		GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber, int OpMode);
	virtual void	FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
	virtual const std::wstring getCurrentDirectory() const;
	virtual void	setCurrentDirectory(const std::wstring& dir);

	Connection& getConnection()
	{
		return connection_;
	}

	const Connection& getConnection() const
	{
		return connection_;
	}

private:
	typedef std::vector<boost::shared_ptr<FTPFileInfo> > FileList;
	typedef boost::shared_ptr<FileList> FileListPtr;
	typedef std::pair<FileListPtr, std::wstring> FileListCacheItem;
	typedef boost::array<FileListCacheItem, 16> FileListCacheArray;

	static const size_t				permissionError = UINT_MAX;
	enum
	{
		FTP_COL_MODE,
		FTP_COL_LINK,
		FTP_COL_MAX
	};

	FTP*							plugin_;
	boost::shared_ptr<FileList>		filelist_;
	Connection						connection_;
	FARWrappers::ItemList			itemList_;
	boost::scoped_array<wchar_t*>	columndata_;
	std::wstring					selFile_;

	Utils::CacheFixedArray<std::wstring, FileListPtr, 16> filelistCache_;

	bool			parseFtpDirOutput(FileListPtr &filelist);
	size_t			parseFtpDirOutputInt(ServerTypePtr server, FileListPtr &filelist);
	void			copyNamesToClipboard();
	void			setAttributes();
	void			saveURL();
	const FTPFileInfo& getCurrentFile() const;
	const FTPFileInfo& findfile(size_t id) const;
	FTPFileInfo&	findfile(size_t id);


	const FTPHost&	getHost() const;
	FTPHost&		getHost();
	void			selectPanelItem(const std::wstring &item) const;
	bool			reread();
	bool			createDirectory(const std::wstring &dir, int OpMode);
};


#endif