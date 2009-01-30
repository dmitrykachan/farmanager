#include "stdafx.h"
#include "panelview.h"

#include "dialogs.h"
#include "farwrapper/dialog.h"

static bool checkRewrite(const std::wstring &filename, const std::wstring &title, bool &skipAll, bool &overwriteAll)
{
	if(overwriteAll)
		return true;
	DWORD attr = GetFileAttributesW(filename.c_str());

	if(attr == INVALID_FILE_ATTRIBUTES)
		return true;
	if(skipAll)
		return false;

	const std::wstring message = getMsg(
				is_flag(attr,FILE_ATTRIBUTE_READONLY) ? MAlreadyExistRO : MAlreadyExist);
	switch(Dialogs::overwrite(title, message, filename))
	{
	case Dialogs::Cancel:
	case Dialogs::Skip:
		return false;
	case Dialogs::Overwrite:
		break;
	case Dialogs::SkipAll:
		skipAll = true;
		return false;
	case Dialogs::OverwriteAll:
		overwriteAll = true;
		break;
	}
	if(!DeleteFileW(filename.c_str()))
		if(!SetFileAttributesW(filename.c_str(), FILE_ATTRIBUTE_NORMAL) && !DeleteFileW(filename.c_str()))
		{
			const wchar_t* MsgItems[] =
			{
				getMsg(MError),
				getMsg(MCannotCopyHost),
				filename.c_str(),
				getMsg(MOk)
			};
			FARWrappers::message(MsgItems,  1, FMSG_WARNING|FMSG_DOWN|FMSG_ERRORTYPE);

			return false;
		}

	return true;
}

static bool askFolder(const std::wstring& title, const std::wstring& message, std::wstring &path)
{
	FARWrappers::Dialog dlg(L"FTPCmd");

	dlg.addDoublebox( 3, 1,72, 6,	title)->
		addLabel	( 5, 2,			message)->
		addEditor	( 5, 3,70,	&path)->
		addHLine	( 3, 4)->
		addDefaultButton(0,5,	0,	getMsg(MCopy), DIF_CENTERGROUP)->
		addButton	(0,5,	-1,		getMsg(MCopy), DIF_CENTERGROUP);
	
	return (dlg.show(76, 8) == -1)? false : true;
}

PanelView::Result HostView::GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, 
			 const std::wstring &destPath, int OpMode)
{
	PROC;
	BOOST_ASSERT(plugin_);
	std::wstring path = destPath;
	if(!IS_SILENT(OpMode))
	{
		bool ask = Move? 
			askFolder(getMsg(MMoveHostTitle), getMsg(MMoveHostTo), path) :
			askFolder(getMsg(MCopyHostTitle), getMsg(MCopyHostTo), path);
		if(!ask || path.empty())
			return Canceled;
	}

	BOOST_ASSERT(!path.empty());

	bool inside = false;
	if(path == L"..")
	{
		if(getCurrentDirectory().empty())
			return Failed;
		path = getPathBranch(getCurrentDirectory());
		inside = true;
	} else
	{
		// TODO copying moving inside FTP plugins. i.e. it is necessary
		// to determinate that the path is not from file system
	}

	if(inside)
	{
		bool troubles = false;
		BOOST_ASSERT(0 && "TODO");
		for(int n = 0; n < ItemsNumber; n++)
		{
			const FtpHostPtr& p = findhost(PanelItem[n].UserData);
			if(!p)
				continue;

			//Check for folders
			if(p->Folder)
			{
				FARWrappers::message(MCanNotMoveFolder);
				troubles = true;
				continue;
			}

			BOOST_ASSERT(0 && "TODO and check");
			p->setRegPath(path);
			if(!p->Write())
			{
				troubles = true;
				continue;
			}

			if(Move && !p->Folder)
			{
				g_manager.getRegKey().deleteSubKey((std::wstring(HostRegName) + getCurrentDirectory() + L'\\' + p->getRegName()).c_str(), true);
				PanelItem[n].Flags &= ~PPIF_SELECTED;
			}
		}
		return troubles? Failed : Succeeded;
	}

	bool skipAll = false, overwriteAll = false;
	for(int n = 0; n < ItemsNumber; n++)
	{
		const FtpHostPtr& p = findhost(PanelItem[n].UserData);

		if(p->Folder)
			continue;

		std::wstring filename = path + L'\\' + p->getIniFilename();

		if(!checkRewrite(filename, 
					getMsg(Move ? MMoveHostTitle : MCopyHostTitle), 
					skipAll, overwriteAll))
			continue;

		if(!p->WriteINI(filename))
		{
			DeleteFileW(filename.c_str());
		} else
			if (Move)
			{
				BOOST_ASSERT(0 && "TODO delete key");
				//	TODO			  FP_DeleteRegKey(p->regKey_);
			}
	}

	return Succeeded;
}


void HostView::readFolder(const std::wstring& dir, HostList &hostList)
{
	try
	{
		std::wstring path = dir;
		WinAPI::RegKey regkey(g_manager.getRegKey(), (HostRegName+path).c_str());
		std::vector<std::wstring> hosts = regkey.getKeys();
		hostList.reserve(hosts.size());

		std::vector<std::wstring>::const_iterator itr = hosts.begin();
		path += L'\\';
		while(itr != hosts.end())
		{
			boost::shared_ptr<FTPHost> h(new FTPHost);
			h->Init();

			h->lastEditTime_.dwHighDateTime = 0;
			h->lastEditTime_.dwLowDateTime = 0;

			if(!h->Read(*itr++, path))
				continue;
			hostList.push_back(h);
		}
	}
	catch(WinAPI::RegKey::exception&)
	{
	}
}

bool HostView::GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber, int OpMode)
{
	PROC;
	BOOST_ASSERT(plugin_);
	BOOST_ASSERT(itemList_.size() == 0);

	hosts_.clear();
	readFolder(getCurrentDirectory(), hosts_);

	itemList_.clear();
	columndata_.reset(new wchar_t*[FTP_HOST_COL_COUNT*(hosts_.size()+1)]);

	if(!IS_SILENT(OpMode)) 
	{
		PluginPanelItem& i = itemList_.add(PluginPanelItem());

		FARWrappers::clear(i);
		i.FindData.lpwszFileName    = L"..";
		i.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		i.Description               = L"..";
		i.CustomColumnNumber        = FTP_HOST_COL_COUNT;
		i.CustomColumnData          = columndata_.get();
		i.CustomColumnData[0]       = L"..";
		i.CustomColumnData[1]       = L"..";
		i.CustomColumnData[2]       = L"..";
		i.UserData                  = ParentDirHostID;
	}

	itemList_.reserve(itemList_.size()+hosts_.size());
	std::vector<boost::shared_ptr<FTPHost> >::const_iterator itr = hosts_.begin();
	size_t index = ParentDirHostID+1;
	while(itr != hosts_.end())
	{
		PluginPanelItem& i = itemList_.add(PluginPanelItem());
		FARWrappers::clear(i);

		i.FindData.lpwszFileName	= const_cast<wchar_t*>((*itr)->getIniFilename().c_str());
		i.FindData.ftLastWriteTime  = (*itr)->lastEditTime_;
		i.FindData.dwFileAttributes = (*itr)->Folder ? FILE_ATTRIBUTE_DIRECTORY : 0;
		i.Flags                     = 0;
		i.UserData                  = index;

		if(!IS_SILENT(OpMode))
		{
			i.Description               = const_cast<wchar_t*>((*itr)->hostDescription_.c_str());
			i.CustomColumnNumber        = FTP_HOST_COL_COUNT;
			i.CustomColumnData          = columndata_.get()+index*FTP_HOST_COL_COUNT;
			i.CustomColumnData[0]       = const_cast<wchar_t*>((*itr)->url_.Host_.c_str());  //C0
			i.CustomColumnData[1]       = const_cast<wchar_t*>((*itr)->url_.directory_.c_str());  //C1
			i.CustomColumnData[2]       = const_cast<wchar_t*>((*itr)->url_.username_.c_str());  //C2
		}

		++itr;
		++index;
	}
	*pPanelItem		= itemList_.items();
	*pItemsNumber	= static_cast<int>(itemList_.size());

	return true;
}


bool HostView::SetDirectory(const std::wstring &Dir,int /*OpMode*/)
{
	PROC;
	BOOST_ASSERT(plugin_);

	FARWrappers::Screen scr;
	std::wstring oldDir = getCurrentDirectory();

	if(Dir == L"..")
	{
		if(oldDir.empty() || oldDir == L"\\")
		{
			BOOST_LOG(INF, L"Close plugin");
			FARWrappers::getInfo().Control(plugin_, FCTL_CLOSEPLUGIN, 0, 0);
			return true;
		}

		setCurrentDirectory(getPathBranch(getCurrentDirectory()));
		BOOST_LOG(INF, L"DirBack");
		return true;
	}
	
	if(Dir.empty() || (Dir == L"\\"))
	{
		setCurrentDirectory(L"");
		BOOST_LOG(INF, L"Set to root");
		return true;
	}

	if(Dir[0] == L'\\')
		setCurrentDirectory(Dir);
	else
		setCurrentDirectory(getCurrentDirectory() + L'\\' + Dir);

	return true;
}


bool InsertHostsCmd(FTP* ftp, bool full)
{
	const FtpHostPtr&  p = ftp->getSelectedHost();

	std::wstring m = full? p->url_.toString() : p->url_.Host_;
	return FARWrappers::getInfo().Control(ftp, FCTL_INSERTCMDLINE, 0, reinterpret_cast<LONG_PTR>(m.c_str()));
}

bool HostView::ProcessKey(int Key, unsigned int ControlState)
{
	PROC;
	BOOST_ASSERT(plugin_);

	//Ctrl+BkSlash
	if(Key == VK_BACK && ControlState == PKF_CONTROL)
	{
		SetDirectory(L"\\", 0);
		plugin_->Invalidate(true);
		return true;
	}

	if(ControlState == PKF_CONTROL)
	{
		if(Key == 'F')
			return InsertHostsCmd(plugin_, true);
		else
			if(Key == VK_RETURN)
				return InsertHostsCmd(plugin_, false);
	}

	if(ControlState == 0 && Key == VK_RETURN)
	{
		FtpHostPtr& p = plugin_->getSelectedHost();

		//Switch to FTP
		if(p && !p->Folder)
		{
			plugin_->FullConnect(p);

			return true;
		}
	}

	//New entry
	if((ControlState==PKF_SHIFT && Key==VK_F4) ||
	   (ControlState==PKF_ALT   && Key==VK_F6))
	{
		FtpHostPtr h = FtpHostPtr(new FTPHost);

		h->Init();
		if(plugin_->EditHostDlg(MEnterFtpTitle, h, false))
		{
			h->setRegPath(getCurrentDirectory());
			h->Write();
			h->MkINIFile();

			plugin_->Invalidate();
			selectPanelItem(h->getIniFilename());
		}
		return true;
	}

	//Edit
	if( (ControlState==0           && Key==VK_F4) ||
		(ControlState==PKF_SHIFT   && Key==VK_F6) ||
		(ControlState==PKF_CONTROL && Key=='Z') )
	{
		FtpHostPtr& p = plugin_->getSelectedHost();
		if(p->Folder)
		{
			if(!plugin_->EditDirectory(p->url_.Host_, p->hostDescription_, false, true))
				return true;
		} else 
		{
			if(!plugin_->EditHostDlg(MEditFtpTitle,p,Key=='Z'))
				return true;
		}

		p->MkINIFile();
		if (p->Write())
		{
			FARWrappers::getInfo().Control(plugin_, FCTL_UPDATEPANEL, 0, 0);
			FARWrappers::getInfo().Control(plugin_, FCTL_REDRAWPANEL, 0, 0);
		}

		return true;
	}

	return false;
}

class is_icompare
{
public:
	is_icompare( const std::locale& Loc=std::locale() ) 
		: loc_(Loc) {}

	template< typename T1, typename T2 >
	int operator()(const T1& Arg1, const T2& Arg2) const
	{
		T1 c1 = std::toupper<T1>(Arg1,loc_);
		T2 c2 = std::toupper<T2>(Arg2,loc_);
		if (c1 < c2)
			return -1;
		else
			if (c1 == c2)
				return 0;
		else
			return 1;
	}

private:
	std::locale loc_;
};

static int icompare(const std::wstring& str1, const std::wstring& str2)
{
	is_icompare comp;
	std::wstring::const_iterator itr1 = str1.begin();
	std::wstring::const_iterator itr2 = str2.begin();
	while(itr1 != str1.end() && itr2 != str2.end())
	{
		int res = comp(*itr1, *itr2);
		if(res != 0)
			return res;
		++itr1; ++itr2;
	}
	return (itr1 == str1.end())? 1 : -1;
}

PanelView::CompareResult HostView::Compare(const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode)
{
	PROC;
	BOOST_ASSERT(plugin_);
	if(Mode == SM_UNSORTED)
		return UseInternal;

	if(!i1 || !i2)
		return UseInternal;

	const FtpHostPtr& p1 = findhost(i1->UserData);
	const FtpHostPtr& p2 = findhost(i2->UserData);
	int      n;

	if(!p1 || !p2)
		return UseInternal;

	switch(Mode)
	{
	case   SM_EXT: n = icompare(p1->url_.directory_,  p2->url_.directory_);	break;
	case SM_DESCR: n = icompare(p1->hostDescription_, p2->hostDescription_);	break;
	case SM_OWNER: n = icompare(p1->url_.username_,   p2->url_.username_);		break;

	case SM_MTIME:
	case SM_CTIME:
	case SM_ATIME: n = (int)CompareFileTime(&p2->lastEditTime_, &p1->lastEditTime_);
		break;
 
	default: n = icompare(p1->url_.Host_, p2->url_.Host_);
		break;
	}

	do
	{
		if(n) break;

		n = icompare(p1->url_.Host_,		p2->url_.Host_);
		if(n) break;

		n = icompare(p1->url_.username_,	p2->url_.username_);
		if(n) break;

		n = icompare(p1->hostDescription_,	p2->hostDescription_);
	}while(0);

	return static_cast<CompareResult>(n);
}

void HostView::FreeFindData(PluginPanelItem *PanelItem, int ItemsNumber)
{
	PROC;
	BOOST_ASSERT(PanelItem == itemList_.items() && static_cast<size_t>(ItemsNumber) == itemList_.size());
	itemList_.clear();
	columndata_.reset();
}


struct DeleteData: boost::noncopyable
{
	FTP*	plugin;
	bool	deleteAllFolders;
	bool	skipAll;
	int		opMode;
	bool	result;
};

HostView::CallbackStatus 
HostView::deleteCallback(bool enter, FTPHost &host, const std::wstring& path, void* param)
{
	BOOST_ASSERT(enter == true);

	DeleteData* data = reinterpret_cast<DeleteData*>(param);

	if(host.Folder && !data->deleteAllFolders && !IS_SILENT(data->opMode))
	{
		const wchar_t* MsgItems[]=
		{
			getMsg(MDeleteHostsTitle),
			getMsg(MDeleteHostFolder),
			host.url_.toString().c_str(),
			getMsg(MDeleteGroupDelete),
			getMsg(MDeleteGroupAll),
			getMsg(MSkip),
			getMsg(MDeleteGroupCancel)
		};

		int res = FARWrappers::message(MsgItems, 4, FMSG_WARNING|FMSG_DOWN);

		switch(res) 
		{
			/*ESC*/
			case -1: 
			/*Cancel*/
			case  3:
				return Cancel;
			/*Del*/
			case  0:
				break;
			/*DelAll*/
			case  1: 
				data->deleteAllFolders = true;
				break;
			case 2:
				return Skip;
		}
	}
	BOOST_LOG(ERR, "deleted: " << path + L'\\' + host.getRegName());
	g_manager.getRegKey().deleteSubKey(
		(std::wstring(HostRegName) + path + L'\\' + host.getRegName()).c_str(), true);
	return host.Folder? Skip : OK;
}

bool HostView::DeleteFiles(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	PROC;
	if(ItemsNumber == 0)
		return false;

	//Ask
	if (!IS_SILENT(OpMode))
	{
		const wchar_t* MsgItems[]=
		{
			getMsg(MDeleteHostsTitle),
			getMsg(MDeleteHosts),
			getMsg(MDeleteDelete),
			getMsg(MDeleteCancel)
		};

		if(FARWrappers::message(MsgItems, 2) != 0)
			return true;

		if(ItemsNumber > 1)
		{
			wchar_t buff[128];
			_snwprintf(buff, sizeof(buff)/sizeof(*buff), getMsg(MDeleteNumberOfHosts), ItemsNumber);
			MsgItems[1] = buff;
			if(FARWrappers::message(MsgItems, 2, FMSG_WARNING|FMSG_DOWN) != 0)
				return true;
		}
	}

	DeleteData data;
	data.plugin				= plugin_;
	data.deleteAllFolders	= false;
	data.opMode				= OpMode;
	data.skipAll			= false;

	return walk(PanelItem, ItemsNumber, getCurrentDirectory(), deleteCallback, &data);
}


bool HostView::walk(const PluginPanelItem* panelItems, int itemsNumber, const std::wstring &path, WalkListCallBack callback, void* param)
{
	bool res = true;
	for(int i = 0; res && i < itemsNumber; ++i)
	{
		res = walk(*findhost(panelItems[i].UserData), path, callback, param);
	}
	return res;
}

bool HostView::walk(FTPHost& host, const std::wstring &path, WalkListCallBack callback, void* param)
{
	bool res = true;
	if(host.Folder)
	{
		CallbackStatus cbstatus = callback(true, host, path, param);
		if(cbstatus == Cancel)
			return false;
		if(cbstatus == OK)
		{
			HostList list;
			std::wstring subpath = path + host.getIniFilename();
			readFolder(subpath, list);
			HostList::iterator itr = list.begin();
			while(res && itr != list.end())
			{
				res = walk(**itr, subpath, callback, param);
				++itr;
			}
			cbstatus = callback(false, host, path, param);
			if(cbstatus == Cancel)
				return false;
		}
	} else
	{
		if(callback(true, host, path, param) == Cancel)
			return false;
	}
	return true;
}


PanelView::Result HostView::MakeDirectory(std::wstring &name, int OpMode)
{
	PROCP(name << OpMode);
	FTPHost h;

	h.Init();

	if(!IS_SILENT(OpMode) && !plugin_->EditDirectory(name, h.hostDescription_, true, true))
		return Canceled;

	if(name.empty())
		return Canceled;

	if(name == L"." ||	name == L"..")
	{
		SetLastError(ERROR_ALREADY_EXISTS);
		return Failed;
	}

	h.Folder = true;
	h.url_.Host_ = name;
	h.setRegPath(getCurrentDirectory());
	h.Write();

	return Succeeded;
}

PanelView::Result HostView::PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode)
{
	bool            SkipAll = false;
	std::wstring	destPath, srcFile, destName, curName;
	DWORD           srcAttr;
	int 			n;
	FTPHost         h;
	FARWrappers::ItemList il;

	destPath = getCurrentDirectory() + L'\\';

	BOOST_ASSERT(0 && "TODO");
	// 	if (!ExpandList(PanelItem,ItemsNumber,&il,FALSE))
	// 		return 0;

	for(n = 0; n < itemsNumber; n++ )
	{
		if (CheckForEsc(false))
			return Canceled;

		h.Init();
		curName = panelItems[n].FindData.lpwszFileName;

		srcAttr = GetFileAttributesW(curName.c_str());
		if(srcAttr == INVALID_FILE_ATTRIBUTES)
			continue;

		if(is_flag(srcAttr,FILE_ATTRIBUTE_DIRECTORY))
		{
			h.Folder = true;
			h.url_.Host_ = curName.c_str();
			h.setRegPath(getCurrentDirectory());
			h.Write();
			continue;
		}

		if(curName.find(L'\\') == std::wstring::npos)
		{
			srcFile		= L".\\" + curName;
			destName	= destPath;
		} else
		{
			srcFile		= curName;
			destName	= destPath + curName;
			size_t n	= destName.find('\\');
			BOOST_ASSERT(n != std::wstring::npos);
			destName.resize(n);
		}

		if(!h.ReadINI(srcFile))
		{
			if(!IS_SILENT(OpMode))
				if(SayNotReadedTerminates(srcFile, SkipAll))
					return Canceled;
			continue;
		}

		BOOST_LOG(INF, L"Write new host [" << srcFile << L"] -> [" << destName << L"]");
		BOOST_ASSERT(0 && "TODO and check (write)");
		h.setRegPath(destName);
		h.Write();

		if (Move)
		{
			SetFileAttributesW(curName.c_str(),0);
			DeleteFileW(curName.c_str());
		}

		if (n < itemsNumber)
			panelItems[n].Flags &= ~PPIF_SELECTED;
	}

	if (Move)
	{
		for(n = itemsNumber-1; n >= 0; n-- )
		{
			if(CheckForEsc(FALSE))
				return Canceled;

			if(is_flag(il[n].FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY))
				if(RemoveDirectoryW( FTP_FILENAME(&il[n]).c_str()))
					if ( n < itemsNumber )
						panelItems[n].Flags &= ~PPIF_SELECTED;
		}
	}

	return Succeeded;
}


bool HostView::ProcessEvent(int Event, void *Param)
{
	PROCP(Event);
	BOOST_LOG(DBG, L"Event: " << Event);

//TODO	if(plugin_->CurrentState == fcsClose || plugin_->CurrentState == fcsConnecting)
//		return false;

	switch(Event)
	{
	case FE_CHANGEVIEWMODE:
		{
			int viewMode = FARWrappers::getPanelInfo().ViewMode;
			BOOST_LOG(INF, L"New ColumnMode " << viewMode);
			g_manager.getRegKey().set(L"LastHostsMode", viewMode);
		}
		break;
	case FE_REDRAW:
		if(!pluginColumnModeSet_)
		{
			if(g_manager.opt.PluginColumnMode != -1)
				FARWrappers::getInfo().Control(plugin_, FCTL_SETVIEWMODE, g_manager.opt.PluginColumnMode, 0);
			else
			{
				int num = g_manager.getRegKey().get(L"LastHostsMode", 2);
				FARWrappers::getInfo().Control(plugin_, FCTL_SETVIEWMODE, num, 0);
			}
			pluginColumnModeSet_ = true;
		}
		if(!selectedHost_.empty())
		{
			const std::wstring s = selectedHost_;
			selectedHost_.clear(); // prevent the recursion update
			selectPanelItem(s);
		}

		break;

	case FE_CLOSE:
		{
			BOOST_LOG(INF, L"Close notify");
			g_manager.getRegKey().set(L"LastHostsMode", FARWrappers::getPanelInfo().ViewMode);
		}
		break;

	case FE_IDLE:
	case FE_BREAK:
		break;
	case FE_COMMAND:
		return plugin_->ExecCmdLine(reinterpret_cast<wchar_t*>(Param), false);
		break;
	}

	return false;
}

const std::wstring& HostView::getCurrentDirectory() const
{
	return hostsPath_;
}

void HostView::setCurrentDirectory(const std::wstring& dir)
{
	hostsPath_ = dir;
	g_manager.getRegKey().set(L"LastHostsPath", dir);
}

void HostView::selectPanelItem(const std::wstring &item) const
{
	PanelInfo info = FARWrappers::getPanelInfo();
	FARWrappers::PanelItem it(true);
	for(int n = 0; n < info.ItemsNumber; n++)
	{
		it.getPanelItem(n);
		if(item == it.get().FindData.lpwszFileName)
		{
			PanelRedrawInfo pri;
			pri.CurrentItem  = n;
			pri.TopPanelItem = info.TopPanelItem;
			FARWrappers::getInfo().Control(plugin_, FCTL_REDRAWPANEL, 0, reinterpret_cast<LONG_PTR>(&pri));
			return;
		}
	}
	BOOST_ASSERT(0 && "host not found");
	return;
}


void HostView::setSelectedHost(const std::wstring& hostname)
{
	selectedHost_ = hostname;
}