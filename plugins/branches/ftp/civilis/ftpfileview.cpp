#include "stdafx.h"
#include "panelview.h"
#include "Utils/uniconverts.h"
#include "ftp_Ftp.h"
#include "ftp_Int.h"
#include "farwrapper/dialog.h"

// returns the count of error parsed entries.
size_t FTPFileView::parseFtpDirOutputInt(ServerTypePtr server, FileListPtr &filelist)
{
	std::wstring::const_iterator itr	 = connection_.output_.begin();
	std::wstring::const_iterator itr_end = connection_.output_.end();
	ServerType::entry entry;

	size_t errors = 0;
	boost::shared_ptr<FTPFileInfo> pFileinfo(new FTPFileInfo);

	while(1)
	{
		if(itr == itr_end)
			return errors;

		entry = server->getEntry(itr, itr_end);
		if(entry.first == entry.second)
			continue;

		static const wchar_t* permission = L": Permission denied";
		if(std::search(entry.first, entry.second, permission, permission + wcslen(permission)) != entry.second)
			return permissionError;

		if(!server->isValidEntry(entry.first, entry.second))
			continue;

		BOOST_LOG(DBG, L"toParse: [" << std::wstring(entry.first, entry.second) << L"]");
		pFileinfo->clear();
		try
		{
			server->parseListingEntry(entry.first, entry.second, *pFileinfo);
			// TODO updateTime(fileinfo);
		}
		catch (std::exception &e)
		{
			connection_.AddCmdLine( 
				std::wstring(L"ParserFAIL: (") + server->getName() + L") [" + 
				std::wstring(entry.first, entry.second) + L"]:" + Unicode::utf8ToUtf16(e.what()), ldOut);
			++errors;
			continue;
		}

		BOOST_ASSERT(pFileinfo->getType() != FTPFileInfo::undefined);

		const std::wstring &filename = pFileinfo->getFileName();
		if(filename.empty() || filename == L"." || filename == L"..")
			continue;

		filelist->push_back(pFileinfo);
		pFileinfo.reset(new FTPFileInfo);
	}

}

bool FTPFileView::parseFtpDirOutput(FileListPtr &filelist)
{
	PROC;

	ServerTypePtr server = getHost().serverType_;
	if(ServerList::isAutodetect(server))
	{
		server = ServerList::autodetect(connection_.getSystemInfo());
		// TODO если не определен, тады пытаемся парсить листинги.
	}

	FTPFileInfoPtr prev(new FTPFileInfo);
	prev->clear();
	prev->setFileName(L"..");
	prev->setType(FTPFileInfo::directory);

	filelist->push_back(prev);
	filelist->reserve(50);
	size_t errors = parseFtpDirOutputInt(server, filelist);
	if(errors == permissionError)
		return false;
	
	if(errors != 0)
		BOOST_LOG(ERR, errors << L"listing parsing errors.");

	filelistCache_.add(getCurrentDirectory(), filelist);


	return true;
}

int FTPFileView::GetFiles(FARWrappers::ItemList &panelItems, int Move, const std::wstring &destPath, int OpMode)
{
	PROC;
	BOOST_ASSERT(plugin_);
	return true;
}
int FTPFileView::PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode)
{
	PROC;
	BOOST_ASSERT(plugin_);
	return true;
}
int FTPFileView::Compare(const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode)
{
	PROC;
	BOOST_ASSERT(plugin_);
	return true;
}
bool FTPFileView::DeleteFiles(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	PROC;
	BOOST_ASSERT(plugin_);
	return true;
}

int FTPFileView::MakeDirectory(std::wstring &name, int OpMode)
{
	PROCP(name << L"mode: " << OpMode);
	BOOST_ASSERT(plugin_);

	if(!getConnection().isConnected())
		return false;

	//Edit name
	if(!IS_SILENT(OpMode) && !plugin_->EditDirectory(name, std::wstring(L""), true, false))
		return -1;

	//Correct name
	if(name.empty())
		return -1;

	//FTP
	FARWrappers::Screen  scr;

	//Try to create
	if(createDirectory(name, OpMode))
		return true;

	if(IS_SILENT(OpMode))
		return false;

	//If conection alive - report error
	if(getConnection().cmdLineAlive())
		getConnection().ConnectMessage(MCreateDirError, L"", true);
	else
		getConnection().ConnectMessageTimeout(MConnectionLost, L"", MRetry);

	return true;
}

int FTPFileView::ProcessEvent(int Event, void *Param)
{
	PROCP(Event);
	BOOST_ASSERT(plugin_);

	if(plugin_->CurrentState == fcsClose || plugin_->CurrentState == fcsConnecting)
		return false;

	switch(Event)
	{
	case FE_BREAK:
		break;
	case FE_CLOSE:
		plugin_->CurrentState = fcsClose;
		break;
	case FE_REDRAW:
		if(!selFile_.empty())
		{
			std::wstring s = selFile_;
			selFile_ = L"";
			selectPanelItem(s);
		}
	    break;
	case FE_IDLE:
		if(getConnection().cmdLineAlive() && getConnection().getKeepStopWatch().isTimeout())
		{
			FTPCmdBlock blocked(plugin_, true);

			if(g_manager.opt.ShowIdle)
			{
				IdleMessage(getMsg(MKeepAliveExec), g_manager.opt.ProcessColor);
			}

			getConnection().keepAlive();
		}
	    break;
	case FE_COMMAND:
		return plugin_->ExecCmdLine(reinterpret_cast<wchar_t*>(Param), false);
		break;
	default:
	    break;
	}

	return false;
}

int FTPFileView::ProcessKey(int Key, unsigned int ControlState)
{
	PROC;
	BOOST_ASSERT(plugin_);

	//Check for empty command line
	if(ControlState==PKF_CONTROL && Key==VK_INSERT)
	{
		wchar_t str[1024];
		FARWrappers::getInfo().Control(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, str);
		if(!str[0])
			ControlState = PKF_CONTROL | PKF_SHIFT;
	}

	//Ctrl+BkSlash
	if (Key == VK_BACK && ControlState == PKF_CONTROL)
	{
		SetDirectory(L"/", 0);
		reread();
		plugin_->Invalidate();
		return true;
	}

	//Copy names
	if(ControlState==(PKF_CONTROL|PKF_SHIFT) && Key==VK_INSERT)
	{
		copyNamesToClipboard();
		return true;
	}

	//Table
	if(ControlState==PKF_SHIFT && Key==VK_F7)
	{
		getConnection().getHost().codePage_ = plugin_->SelectTable(getConnection().getHost().codePage_);
		plugin_->Invalidate();
		return true;
	}

	if(ControlState == PKF_CONTROL && Key=='A')
	{
		setAttributes();
		plugin_->Invalidate();
		return true;
	}

	//Save URL
	if(ControlState==PKF_ALT && Key==VK_F6)
	{
		saveURL();
		return true;
	}

	//Reread
	if (ControlState==PKF_CONTROL && Key=='R')
	{
		filelistCache_.clear();
		return false;
	}

	//Drop full name
	if(ControlState==PKF_CONTROL && Key=='F')
	{
		PanelInfo pi;

		FARWrappers::getInfo().Control(plugin_, FCTL_GETPANELINFO, &pi);

		if (pi.CurrentItem >= pi.ItemsNumber)
			return false;

		std::wstring s;
		std::wstring path = getConnection().getCurrentDirectory();
		getHost().MkUrl(s, path.c_str(), FTP_FILENAME(&pi.PanelItems[pi.CurrentItem]) );
		QuoteStr(s);
		FARWrappers::getInfo().Control( this, FCTL_INSERTCMDLINE, (void*)Unicode::toOem(s).c_str());
		return true;
	}

	//Select
	if (ControlState == 0 && Key == VK_RETURN)
	{
		const FTPFileInfo& fileinfo = getCurrentFile();

		if(fileinfo.getFileName() != L"." && fileinfo.getFileName() != L"..")
			if(fileinfo.getType() == FTPFileInfo::directory || 
			   fileinfo.getType() == FTPFileInfo::symlink)
			{
				if(SetDirectory((fileinfo.getType() == FTPFileInfo::directory)? 
					fileinfo.getFileName() : fileinfo.getLink(),
					0))
				{
					FARWrappers::getInfo().Control(plugin_, FCTL_UPDATEPANEL,NULL );

					struct PanelRedrawInfo RInfo;
					RInfo.CurrentItem = RInfo.TopPanelItem = 0;
					FARWrappers::getInfo().Control(plugin_, FCTL_REDRAWPANEL, &RInfo);
				}
				return true;
			}
	}/*ENTER*/

	//Copy/Move
	if((ControlState == 0 || ControlState == PKF_SHIFT) && Key == VK_F6)
	{
		FTP *otherPlugin = OtherPlugin(plugin_);
		int  rc;

		if(!otherPlugin && ControlState == 0 && Key == VK_F6 )
			return false;

		PluginPanelItem item;
		if(!plugin_->getCurrentPanelItem(item))
			return false;
		FARWrappers::ItemList items;

		items.add(item);
		if(ControlState == PKF_SHIFT)
			rc = GetFiles(items, true, item.FindData.lpwszFileName, 0);
		else
			rc = GetFiles(items, true, otherPlugin->panel_->getCurrentDirectory(), 0);

		if(rc != false)
		{
			FARWrappers::Screen::fullRestore();

			FARWrappers::getInfo().Control(this,FCTL_UPDATEPANEL,NULL);
			FARWrappers::getInfo().Control(this,FCTL_REDRAWPANEL,NULL);

			FARWrappers::Screen::fullRestore();

			FARWrappers::getInfo().Control(ANOTHER_PANEL, FCTL_UPDATEPANEL,NULL);
			FARWrappers::getInfo().Control(ANOTHER_PANEL, FCTL_REDRAWPANEL,NULL);

			FARWrappers::Screen::fullRestore();
		}
		return true;
	}

	return false;
}
int FTPFileView::SetDirectory(const std::wstring &dir, int OpMode)
{
	PROC;
	BOOST_ASSERT(plugin_);


	FARWrappers::Screen scr;
	std::wstring oldDir = getConnection().getCurrentDirectory();

	BOOST_ASSERT(!oldDir.empty());
	BOOST_ASSERT(oldDir[0] == L'/');

	BOOST_LOG(INF, L"SetDirectory [" << oldDir << "] -> [" << dir << L"]");
	if(dir == L"..")
	{
		//Back from root
		if(g_manager.opt.CloseDots && oldDir.size() == 1)
		{
			if(!IS_SILENT(OpMode))
				plugin_->BackToHosts();
			return false;
		}

		size_t pos = oldDir.rfind(L'/');
		selFile_.assign(oldDir, pos+1, oldDir.size()-pos);
	}

	if(!getConnection().isConnected())
		return false;

	if(plugin_->SetDirectoryStepped(dir, false))
		return true;

	return false;
}
int FTPFileView::GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber, int OpMode)
{
	PROC;
	BOOST_ASSERT(plugin_);
	BOOST_ASSERT(itemList_.size() == 0);

	if(!filelistCache_.find(getCurrentDirectory(), filelist_))
	{
		connection_.ls(getCurrentDirectory());

		filelist_.reset(new FileList);
		if(!parseFtpDirOutput(filelist_))
			return false;
	}

	itemList_.reserve(filelist_->size());

	columndata_.reset(new wchar_t*[FTP_COL_MAX*filelist_->size()]);
	FileList::const_iterator itr = filelist_->begin();
	size_t index = 0;
	while(itr != filelist_->end())
	{
		PluginPanelItem& i = itemList_.add(PluginPanelItem());

		const FTPFileInfo &file = **itr;

		i.FindData.dwFileAttributes		= file.getWindowsFileAttribute();
		i.FindData.ftCreationTime		= file.getCreationTime();
		i.FindData.ftLastAccessTime		= file.getLastAccessTime();
		i.FindData.ftLastWriteTime		= file.getLastWriteTime();
		i.FindData.nFileSize			= file.getFileSize();
		i.FindData.lpwszFileName		= const_cast<wchar_t*>(file.getFileName().c_str());
		i.FindData.lpwszAlternateFileName = 0;
		i.Owner							= const_cast<wchar_t*>(file.getOwner().c_str());

		i.CustomColumnData				= columndata_.get() + FTP_COL_MAX*index;
		i.CustomColumnNumber			= FTP_COL_MAX;
		i.CustomColumnData[FTP_COL_MODE]= const_cast<wchar_t*>(file.getMode().c_str());
		i.CustomColumnData[FTP_COL_LINK]= const_cast<wchar_t*>(file.getLink().c_str());

		i.UserData = ++index;
		++itr;
	}
	*pPanelItem		= itemList_.items();
	*pItemsNumber	= static_cast<int>(itemList_.size());

	return true;
}
void FTPFileView::FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber)
{
	PROC;
	BOOST_ASSERT(PanelItem == itemList_.items() && ItemsNumber == itemList_.size());
	itemList_.clear();
	columndata_.reset();
}


const FTPHost& FTPFileView::getHost() const
{
	return getConnection().getHost();
}

FTPHost& FTPFileView::getHost()
{
	return getConnection().getHost();
}

void FTPFileView::copyNamesToClipboard()
{  
	std::wstring	FullName, CopyData;
	PanelInfo		pi;
	size_t			CopySize;
	int n;

	plugin_->getPanelInfo(pi);

	getHost().MkUrl(FullName, getConnection().getCurrentDirectory(), L"");

	CopySize = (FullName.size() + 1 + 2 + 2)*pi.SelectedItemsNumber; // slash +	quotes and /r/n
	for ( CopySize = n = 0; n < pi.SelectedItemsNumber; n++ )
		CopySize += FTP_FILENAME(&pi.SelectedItems[n]).size();

	CopyData.reserve(CopySize);

	for ( n = 0; n < pi.SelectedItemsNumber; n++ ) 
	{
		std::wstring s;
		s = FullName;
		AddEndSlash(s, L'/');
		s += FTP_FILENAME(&pi.SelectedItems[n]);

		if ( g_manager.opt.QuoteClipboardNames )
			QuoteStr( s );

		CopyData += s + L"\r\n";
	}

	if(!CopyData.empty())
		WinAPI::copyToClipboard(CopyData);
}


void FTPFileView::setAttributes()
{
	// TODO listring getCurrentFileAttribute
	boost::array<int, 9> flags;

	FARWrappers::Dialog dlg(L"FTPCmd");

	dlg.addDoublebox( 3, 1,35,6,				getMsg(MChmodTitle))->
		addLabel	( 6, 2,						L"R  W  X   R  W  X   R  W  X")->
		addCheckbox	( 5, 3,		&flags[0],		L"", 1)->
		addCheckbox	( 8, 3,		&flags[1],		L"", 1)->
		addCheckbox	(11, 3,		&flags[2],		L"", 1)->
		addCheckbox	(15, 3,		&flags[3],		L"", 1)->
		addCheckbox	(18, 3,		&flags[4],		L"", 1)->
		addCheckbox	(21, 3,		&flags[5],		L"", 1)->
		addCheckbox	(25, 3,		&flags[6],		L"", 1)->
		addCheckbox	(28, 3,		&flags[7],		L"", 1)->
		addCheckbox	(31, 3,		&flags[8],		L"", 1)->
		addHLine	( 3, 4)->
		addDefaultButton(0,5,	1,				getMsg(MOk))->
		addButton	(0,5,		-1,				getMsg(MCancel));

	if(dlg.show(39, 8) == -1)
		return;

	FARWrappers::Screen scr;

	/*TODO
	AskCode = TRUE;

	for ( n = 0; n < PInfo.SelectedItemsNumber; n++) 
	{
	m = FTP_FILENAME(&PInfo.SelectedItems[n]).c_str();
	if(FtpChmod(hConnect, m, Mode))
	continue;

	if ( !AskCode )
	continue;

	switch( FtpConnectMessage( hConnect,MCannotChmod,m,-MCopySkip,MCopySkipAll) ) {
	/ *skip* /     case 0: break;

	/ *skip all* / case 1: AskCode = FALSE; break;

	default: SetLastError( ERROR_CANCELLED );
	return;
	}
	}
	*/
}

void FTPFileView::saveURL()
{  
	FTPHost h = getHost();
	std::wstring  str;

	std::wstring path = getConnection().getCurrentDirectory();

	h.MkUrl(str, path, L"", true);

	if(h.SetHostName(str, L"", L"") && plugin_->GetHost(MSaveFTPTitle,&h,FALSE))
		h.Write(getCurrentDirectory());
}

const FTPFileInfo& FTPFileView::getCurrentFile() const
{
	PluginPanelItem curItem;
	bool res = plugin_->getCurrentPanelItem(curItem);
	BOOST_ASSERT(res);
	return findfile(curItem.UserData);
}

const FTPFileInfo& FTPFileView::findfile(size_t id) const
{
	BOOST_ASSERT(id >= 1 && id <= filelist_->size());
	return *(*filelist_)[id-1];
}

FTPFileInfo& FTPFileView::findfile(size_t id)
{
	BOOST_ASSERT(id >= 1 && id <= filelist_->size());
	return *(*filelist_)[id-1];
}

const std::wstring FTPFileView::getCurrentDirectory() const
{
	return getConnection().curdir_;
}

void FTPFileView::setCurrentDirectory(const std::wstring& dir)
{
	getConnection().curdir_ = dir;
}

void FTPFileView::selectPanelItem(const std::wstring &item) const
{
	PanelInfo pi;
	plugin_->getPanelInfo(pi);

	for(int n = 0; n < pi.ItemsNumber; n++)
	{
		if(item == pi.PanelItems[n].FindData.lpwszFileName)
		{
			PanelRedrawInfo pri;
			pri.CurrentItem  = n;
			pri.TopPanelItem = pi.TopPanelItem;
			FARWrappers::getInfo().Control(plugin_, FCTL_REDRAWPANEL, &pri);
			return;
		}
	}
	BOOST_ASSERT(0 && "");
}

bool FTPFileView::reread()
{
	std::wstring olddir = getCurrentDirectory();
	std::wstring curfile = getCurrentFile().getFileName();

	//Reread
	filelistCache_.clear();
	FARWrappers::getInfo().Control(plugin_, FCTL_UPDATEPANEL, (void*)1);

	bool rc = olddir == getCurrentDirectory();
	if(rc)
		selFile_ = curfile;

	FARWrappers::getInfo().Control(plugin_, FCTL_REDRAWPANEL, NULL);

	return rc;
}

bool FTPFileView::createDirectory(const std::wstring &dir, int OpMode)
{
	if(dir.empty())
		return false;

	std::wstring name;
	wchar_t		last = *dir.rbegin();

	do
	{
		// Try relative path
		name	= dir;
		if(!iswspace(last))
		{
			if(getConnection().makedir(name))
				break;
		}

		//Try relative path with end slash
		name	+= L'/';
		if(getConnection().makedir(name))
			break;

		//Try absolute path
		name = getConnection().curdir_ + L'/';
		name.append(dir.begin() + (dir[0] == L'/'), dir.end());
		if(!iswspace(last)) 
		{
			if(getConnection().makedir(name))
				break;
		}

		//Try absolute path with end slash
		name	+= L'/';
		if(getConnection().makedir(name))
			break;

		return false;
	}while(0);

	filelistCache_.clear();
	if(!IS_SILENT(OpMode)) 
		selFile_ = name;

	return true;
}
