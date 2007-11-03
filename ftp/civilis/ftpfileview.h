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


	FTPFileView()
		: plugin_(0) {}

	void setPlugin(FTP* plugin)
	{
		plugin_ = plugin;
	}

	virtual Result		GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, 
		const std::wstring &destPath, int OpMode);
	virtual PutResult	PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode);
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

	Connection& getConnection()
	{
		return connection_;
	}

	const Connection& getConnection() const
	{
		return connection_;
	}

private:
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
	FileListPtr						filelist_;
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
	const FTPFileInfoPtr getCurrentFile() const;
	const FTPFileInfoPtr findfile(size_t id) const;
	FTPFileInfoPtr	findfile(size_t id);
	FTP*			getFTP() const;

	const FTPHost&	getHost() const;
	FTPHost&		getHost();
	void			selectPanelItem(const std::wstring &item) const;
	bool			reread();
	bool			createDirectory(const std::wstring &dir, int OpMode);

	Result			deleteFilesInt(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	bool			readFolder(const std::wstring &path, FileListPtr& filelist);
	bool			removeDirectory(const std::wstring &dir);
	bool			deleteFile(const std::wstring& filename);
	bool			renameFile(const std::wstring &existing, const std::wstring &New);

	bool			findFile(const std::wstring& filename, FTPFileInfoPtr &fileinfo);
	Result			getFilesInt(FARWrappers::ItemList &items, int Move, const std::wstring& destPath, int OpMode);
	Result			downloadFile(const std::wstring& remoteFile, const std::wstring& newFile, bool reget, bool asciiMode);
	bool			downloadFileInt(std::wstring remoteFile, const std::wstring& newFile, bool reget, bool asciiMode);

	void			saveUsedDirNFile();
	__int64			fileSize(const std::wstring &filename);
	void			setDescriptionFlag(PluginPanelItem* items, int number);


	class MakeRecursiveList
	{
	private:
		FileList	*list_;
	public:
		MakeRecursiveList(FileList* list)
			: list_(list)
		{
			BOOST_ASSERT(list);
		}
		WalkerResult operator()(bool enter, FTPFileInfoPtr info, const std::wstring& path)
		{
			if(enter)
			{
				FTPFileInfoPtr p(new FTPFileInfo(*info));
				if(!path.empty())
					p->setFileName(path + NET_SLASH + p->getFileName());
				list_->push_back(p);
			}
			return OK;
		}
	};

	class MakeList
	{
	private:
		FileList	*list_;
	public:
		MakeList(FileList* list)
			: list_(list)
		{
			BOOST_ASSERT(list);
		}
		WalkerResult operator()(bool enter, FTPFileInfoPtr info, const std::wstring& path)
		{
			if(enter && info->getType() == FTPFileInfo::directory)
			{
				FTPFileInfoPtr p(new FTPFileInfo(*info));
				if(!path.empty())
					p->setFileName(path + NET_SLASH + p->getFileName());
				list_->push_back(p);
				return Skip;
			}
			return OK;
		}
	};

	template<typename Pred>
	Result walk(const PluginPanelItem* panelItems, int itemsNumber, const std::wstring &path, Pred pred)
	{
		Result res = Succeeded;
		for(int i = 0; res && i < itemsNumber; ++i)
		{
			res = walk(findfile(panelItems[i].UserData), path, pred);
		}
		return res;
	}

	template<typename Pred>
	Result walk(FTPFileInfoPtr &file, const std::wstring &path, Pred pred)
	{
		bool res = true;
		if(file->getType() == FTPFileInfo::directory)
		{
			WalkerResult cbstatus = pred(true, file, path);
			if(cbstatus == Cancel)
				return Canceled;
			if(cbstatus == OK)
			{
				FileListPtr list(new FileList);
				std::wstring subpath = path;
				if(!subpath.empty())
					subpath += NET_SLASH;
				subpath += file->getFileName();
				if(readFolder(subpath, list))
				{
					FileList::iterator itr = list->begin();
					while(res && itr != list->end())
					{
						if((*itr)->getFileName() != L"..")
							res = walk(*itr, subpath, pred);
						++itr;
					}
					cbstatus = pred(false, file, path);
					if(cbstatus == Cancel)
						return Canceled;
				}
			}
		} else
		{
			if(pred(true, file, path) == Cancel)
				return Canceled;
		}
		return Succeeded;
	}



};
