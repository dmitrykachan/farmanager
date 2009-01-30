#ifndef PANEL_VIEW_HEADER
#define PANEL_VIEW_HEADER

class FTP;

#include "ftp_JM.h"
#include "ftp_connect.h"

#include "utils/cache.h"

class PanelView: boost::noncopyable
{
public:
	enum Result
	{
		Succeeded = 1, Failed = 0, Canceled = -1, NotMove = 2
	};

	enum CompareResult
	{
		Less = -1, Equal = 0, More = 1, UseInternal = -2
	};

	virtual Result GetFiles(
		PluginPanelItem *PanelItem, int ItemsNumber, int Move, 
		const std::wstring &destPath, int OpMode) = 0;
	
	virtual Result PutFiles(
		PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode) = 0;

	virtual CompareResult Compare(
		const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode) = 0;

	virtual bool DeleteFiles(
		const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode) = 0;

	virtual Result MakeDirectory(std::wstring &name, int OpMode) = 0;

	virtual bool ProcessEvent(int Event, void *Param) = 0;
	
	virtual bool ProcessKey(int Key, unsigned int ControlState) = 0;

	virtual bool SetDirectory(const std::wstring &Dir, int OpMode) = 0;

	virtual bool GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode) = 0;

	virtual void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber) = 0;

	virtual const std::wstring& getCurrentDirectory() const = 0;
	virtual void setCurrentDirectory(const std::wstring& dir) = 0;
};

class HostView: public PanelView
{
public:
	HostView()
		: plugin_(0), pluginColumnModeSet_(false) {}

	void setPlugin(FTP* plugin)
	{
		plugin_ = plugin;
	}

	virtual Result		GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, 
							const std::wstring &destPath, int OpMode);
	virtual Result		PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode);
	virtual CompareResult Compare(const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode);
	virtual bool		DeleteFiles(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	virtual Result		MakeDirectory(std::wstring &name, int OpMode);
	virtual bool		ProcessEvent(int Event, void *Param);
	virtual bool		ProcessKey(int Key, unsigned int ControlState);
	virtual bool		SetDirectory(const std::wstring &Dir, int OpMode);
	virtual bool		GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber, int OpMode);
	virtual void		FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
	virtual const std::wstring& getCurrentDirectory() const;
	virtual void		setCurrentDirectory(const std::wstring& dir);

	const FtpHostPtr& findhost(size_t id) const
	{
		BOOST_ASSERT(id > 0 && id <= hosts_.size());
		return hosts_[id-1];
	}

	FtpHostPtr& findhost(size_t id)
	{
		BOOST_ASSERT(id > 0 && id <= hosts_.size());
		return hosts_[id-1];
	}

	size_t    hostCount() const
	{
		return hosts_.size();
	}

	void				setSelectedHost(const std::wstring &hostname);

	static const int ParentDirHostID = 0;

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
	void	selectPanelItem(const std::wstring &item) const;
	static CallbackStatus deleteCallback(bool enter, FTPHost &host, const std::wstring& path, void* param);

	FTP*			plugin_;
	std::wstring	hostsPath_;
	std::wstring	selectedHost_;

	std::vector<boost::shared_ptr<FTPHost> >	hosts_;
	FARWrappers::ItemList						itemList_;
	boost::scoped_array<wchar_t*>				columndata_;
	bool			pluginColumnModeSet_;
};


#endif