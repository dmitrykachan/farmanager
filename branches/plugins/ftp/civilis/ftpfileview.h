#pragma once

#include "panelview.h"

struct FTPCopyInfo;

class FTPFileView: public PanelView
{
public:
	enum WalkerResult
	{
		OK, Skip, Cancel
	};
	typedef boost::shared_ptr<FileList> FileListPtr;

	FTPFileView()
		: plugin_(0) {}

	void setPlugin(FTP* plugin)
	{
		plugin_ = plugin;
	}

	virtual Result	GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, 
		const std::wstring &destPath, int OpMode);
	virtual Result	PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode);
	virtual CompareResult Compare(const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode);
	virtual bool	DeleteFiles(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	virtual Result	MakeDirectory(std::wstring &name, int OpMode);
	virtual bool	ProcessEvent(int Event, void *Param);
	virtual bool	ProcessKey(int Key, unsigned int ControlState);
	virtual bool	SetDirectory(const std::wstring &dir, int OpMode);
	virtual bool	GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber, int OpMode);
	virtual void	FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
	virtual const std::wstring getCurrentDirectory() const;
	virtual void	setCurrentDirectory(const std::wstring& dir);

	bool			ftpGetCurrentDirectory();
	bool			ftpSetCurrentDirectory(const std::wstring& dir);

	Result			GetFiles(FARWrappers::ItemList &list, int Move, const std::wstring &destPath, int OpMode);
	Result			initFtpCopyInfo(const std::wstring &destPath, int opMode, bool isMove, FTPCopyInfo* info, bool& isDestDir);
	const FTPFileInfoPtr getFile(size_t id) const;
	FTPFileInfoPtr	getFile(size_t id);
	bool			readFolder(const std::wstring &path, FileListPtr& filelist, bool useCache = true);
	void			resetFileCache();
	void			setSelectedFile(const std::wstring& filename);

	Connection& getConnection()
	{
		return connection_;
	}

	const Connection& getConnection() const
	{
		return connection_;
	}
	const wchar_t* FTPFileView::InsertSelectedToQueue(bool download);

private:
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
	FileListPtr						filelist_;
	Connection						connection_;
	FARWrappers::ItemList			itemList_;
	boost::scoped_array<wchar_t*>	columndata_;
	std::wstring					selFile_;

	Utils::CacheFixedArray<std::wstring, FileListPtr, 16> filelistCache_;

	bool			parseFtpDirOutput(const std::wstring& path, FileListPtr &filelist);
	size_t			parseFtpDirOutputInt(ServerTypePtr server, FileListPtr &filelist);
	void			copyNamesToClipboard();
	void			setAttributes();
	void			saveURL();
	const FTPFileInfoPtr getCurrentFile() const;
	FTP*			getFTP() const;

	const FtpHostPtr& getHost() const;
	FtpHostPtr&		getHost();
	void			selectPanelItem(const std::wstring &item) const;
	bool			reread();
	bool			createDirectory(const std::wstring &dir, int OpMode);

	Result			deleteFilesInt(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	bool			removeDirectory(const std::wstring &dir);
	bool			deleteFile(const std::wstring& filename);
	bool			renameFile(const std::wstring &existing, const std::wstring &New);

	bool			findFile(const std::wstring& filename, FTPFileInfoPtr &fileinfo, bool useCache = true);
	Result			getFilesInt(FARWrappers::ItemList &items, int Move, const std::wstring& destPath, int OpMode);
	Result			downloadFile(const std::wstring& remoteFile, const std::wstring& newFile, bool reget, bool asciiMode, FTPProgress& trafficInfo);
	Result 			downloadFileInt(std::wstring remoteFile, const std::wstring& newFile, bool reget, bool asciiMode, FTPProgress& trafficInfo);
	Result          putFilesInt(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode);

	void			saveUsedDirNFile();
	void			setDescriptionFlag(PluginPanelItem* items, int number);
	bool			SetDirectoryStepped(const std::wstring &Dir, bool update);
	bool			showFilesList(FileList& filelist);
	void			saveList(FileList& filelist);
	Result			putFile(const std::wstring &localFile, const std::wstring &remoteFile, bool Reput, bool AsciiMode, FTPProgress& trafficInfo);
};
