#include "stdafx.h"
#include "ftpfileview.h"
#include "Utils/uniconverts.h"
#include "ftp_Ftp.h"
#include "ftp_Int.h"
#include "farwrapper/dialog.h"
#include "progress.h"

static bool updateTime(FTPFileInfo& fileinfo)
{
	WinAPI::FileTime ft = fileinfo.getLastWriteTime();
	if(ft.empty())
		return false;
	if(fileinfo.getCreationTime().empty())
		fileinfo.setCreationTime(ft);
	if(fileinfo.getLastAccessTime().empty())
		fileinfo.setLastAccessTime(ft);
	return true;
}

// returns the count of error parsed entries.
size_t FTPFileView::parseFtpDirOutputInt(ServerTypePtr server, FileListPtr &filelist)
{
	std::wstring::const_iterator itr	 = connection_.output_.begin();
	std::wstring::const_iterator itr_end = connection_.output_.end();
	ServerType::entry entry;

	size_t errors = 0;
	boost::shared_ptr<FTPFileInfo> pFileinfo(new FTPFileInfo);

	BOOST_LOG(ERR, "Start listing parsing");

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
			updateTime(*pFileinfo);
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

PanelView::Result FTPFileView::GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const std::wstring &destPath, int OpMode)
{
	//Copy received items to internal data and use copy
	// because FAR delete it on next FCTL_GETPANELINFO
	FARWrappers::ItemList FilesList(PanelItem, ItemsNumber);

	return GetFiles(FilesList, Move, destPath, OpMode);
}

extern void SetupFileTimeNDescription(int OpMode,Connection *hConnect, const std::wstring &nm,FILETIME *tm); // TODO



PanelView::Result FTPFileView::GetFiles(FARWrappers::ItemList &list, int Move, const std::wstring &destPath, int OpMode)
{
		Result rc = getFilesInt(list, Move, destPath, OpMode);

	FtpCmdBlock(&getConnection(), false);
	if(rc == Failed || rc == Canceled)
		getFTP()->LongBeepEnd(true);

	BOOST_LOG(INF, L"rc:" << rc);
	return rc;
}

PanelView::Result FTPFileView::initFtpCopyInfo(const std::wstring &destPath, int opMode, bool isMove, FTPCopyInfo* info, bool& isDestDir)
{
	info->destPath        = destPath;
	//Unquote
	if(first(info->destPath) == L'\"' && last(info->destPath) == L'\"')
	{
		info->destPath.resize(info->destPath.size()-1);
		info->destPath.erase(0, 1);
	}

	info->asciiMode       = getHost().AsciiMode;
	info->showProcessList = false;
	info->addToQueque     = false;
	info->msgCode         = IS_SILENT(opMode) ? ocOverAll : ocNone;
	info->download        = true;
	info->uploadLowCase   = g_manager.opt.UploadLowCase;
	//Rename only if move
	info->FTPRename       = isMove? info->destPath.find_first_of(L"/\\") != std::wstring::npos : false;


	if(!IS_SILENT(opMode) && !getFTP()->CopyAskDialog(isMove, true, info))
		return Failed;

	//Remove prefix
	if(boost::istarts_with(info->destPath, L"ftp:"))
		info->destPath.erase(0, 4);

	isDestDir = true;
	//Copy - always on local disk
	if(!isMove)
	{
		return Succeeded;
	}

	//Rename of server
	if(info->FTPRename)
	{
		isDestDir = last(info->destPath) == L'/';
		return Succeeded;
	}

	FTP* other = OtherPlugin(getFTP());
	//Move to the other panel
	if(isMove && other && info->destPath == other->panel_->getCurrentDirectory())
	{
		info->FTPRename = true;
		return Succeeded;
	}

	//Single name
	if (info->destPath.find_first_of(L"/\\") == std::wstring::npos) 
	{
		isDestDir    = false;
		info->FTPRename = true;
		return Succeeded;
	}

	//Path on server
	if(last(info->destPath) == L'/')
	{
		info->FTPRename = true;
		return Succeeded;
	}

	//Path on local disk
	AddEndSlash(info->destPath, L'\\');

	return Succeeded;
}

PanelView::Result FTPFileView::getFilesInt(FARWrappers::ItemList &items, int Move, const std::wstring& destPath, int OpMode)
{
	PROCP(L"Move: " << Move << L", destPath: " << destPath << L"Mode: " << OpMode);
	FileList			filelist;
	std::wstring     DestName;
	FARWrappers::Screen scr;
	bool              isDestDir;
	DWORD            DestAttr;
	int              mTitle;
	Result			rc;
	FTPCopyInfo			ci;

	if(items.size() == 0)
		return Failed;


	if(initFtpCopyInfo(destPath, OpMode, Move, &ci, isDestDir) != Succeeded)
		return Failed;

	//Check for move to parent folder
	if(ci.FTPRename)
		if(ci.destPath == L"..") 
		{
			std::wstring s;
			s = getConnection().getCurrentDirectory();
			size_t pos = s.find_last_of(L"/\\");
			BOOST_ASSERT(pos != std::wstring::npos);
			ci.destPath.assign(s, 0, pos+1);
			isDestDir = true;
		}

	setDescriptionFlag(items.items(), static_cast<int>(items.size()));

	if(ci.FTPRename)
	{
		if(walk(items.items(), static_cast<int>(items.size()), L"", MakeList(&filelist)) != Succeeded)
			return Failed;
	}
	else
	{
		if(walk(items.items(), static_cast<int>(items.size()), L"", MakeRecursiveList(&filelist)) != Succeeded)
 			return Failed;
	}

//TODO 	if(ci.showProcessList && !getFTP()->ShowFilesList(&filelist))
// 		return Failed;

	if(!ci.FTPRename && ci.addToQueque)
	{
		BOOST_LOG(INF, L"Files added to queue [" << filelist.size() << L']');
		getFTP()->ListToQueque(filelist, ci);
		return Succeeded;
	}

	getFTP()->longBeep_.reset();

	//Calc full size
	getConnection().TrafficInfo->Init(MStatusDownload, OpMode, filelist);

	//Copy\Rename each item
	for(size_t i = 0; i < filelist.size(); i++ )
	{
		FTPFileInfo& currentFile = *filelist[i];
		std::wstring curName = currentFile.getFileName();
		FTPFileInfo			findData;
		DestAttr     = INVALID_FILE_ATTRIBUTES;

		//Rename on ftp
		if(ci.FTPRename)
		{
			if(isDestDir)
			{
				DestName = ci.destPath;
				AddEndSlash(DestName, L'/');
				DestName += LOC_SLASH;
				DestName += curName;
			} else
				DestName = ci.destPath;

			BOOST_LOG(INF, L"Rename [" << curName << L"] to [" << DestName << L"]");
			FTPFileInfoPtr fileinfo;
			if(findFile(DestName, fileinfo) && fileinfo->getFileName() == curName)
			{
				DestAttr = fileinfo->getWindowsFileAttribute();
			}
		} else
		{
			//Copy to local disk
			if (isDestDir)
				DestName = ci.destPath + LOC_SLASH + FixFileNameChars(curName, true);
			else
				DestName = FixFileNameChars(ci.destPath);

			FixLocalSlash(DestName);

			//Create directory when copy
			if(currentFile.getType() == FTPFileInfo::directory)
				continue;

			if(FRealFile(DestName.c_str(), findData))
				DestAttr = findData.getWindowsFileAttribute();
		}

//TODO 		int qq = 32;
// 		while(qq < 0x2600)
// 		{
// 			std::vector<std::wstring> v;
// 			v.push_back(L"title");
// 
// 
// 			for(int j = 0; j < 20; j++)
// 			{
// 				std::wstring s;
// 				std::wstringstream ss;
// 				ss << std::hex << qq;
// 				for(int i = 0; i < 64; ++i)
// 				{
// 					s.push_back(qq++);
// 				}
// 				ss << L' ' << s;
// 				v.push_back(ss.str());
// 			}
// 			v.push_back(L"Okk");
// 
// 			FARWrappers::message(v, 1, 0);
// 		}



		//Init current
		getConnection().TrafficInfo->initFile(currentFile, DestName);

		//Query overwrite
		switch(ci.msgCode)
		{
		case      ocOver:
		case      ocSkip:
		case    ocResume: ci.msgCode = ocNone;
			break;
		}

		if(DestAttr != INVALID_FILE_ATTRIBUTES)
		{
			ci.msgCode = getFTP()->AskOverwrite(ci.FTPRename ? MRenameTitle : MDownloadTitle, TRUE,
			                              &findData, &currentFile, ci.msgCode);

			switch( ci.msgCode )
			{
			case   ocOverAll:
			case      ocOver: break;
			case      ocSkip:
			case   ocSkipAll: getConnection().TrafficInfo->skip();
				continue;
			case    ocResume:
			case ocResumeAll: break;

			case    ocCancel: return Canceled;
			}
		}

		//Reset local attrs
		if(!ci.FTPRename && DestAttr != INVALID_FILE_ATTRIBUTES &&
			(DestAttr & FILE_ATTRIBUTE_READONLY) )
			SetFileAttributesW(DestName.c_str(), DestAttr & ~(FILE_ATTRIBUTE_READONLY));

		mTitle = MOk;

		//Do rename
		if(ci.FTPRename)
		{
			//Rename
			if(renameFile(curName, DestName))
			{
				getConnection().ConnectMessage(
					MErrRename,
					(std::wstring(L"\"") + curName + L"\" to \"" + DestName + L"\"").c_str(),
					true, 
					MOk);
				return Failed;
			} else
			{
				selFile_ = DestName;
				filelistCache_.clear();
			}
		} else
		{
			//Do download
			rc = (downloadFile(curName, DestName,
						ci.msgCode == ocResume || ci.msgCode == ocResumeAll,
						ci.asciiMode) == TRUE)? Succeeded : Failed;
			if(rc == Succeeded)
			{
				//Process description
				SetupFileTimeNDescription(OpMode, &getConnection(), DestName, &currentFile.getLastWriteTime());

				//Delete source after download
				if (Move)
				{
					if(!deleteFile(curName)) 
						mTitle = MCannotDelete;
				}
			} else
			{
				if (rc = Canceled)
					return Canceled;
				else
					//Other error
					mTitle = MCannotDownload;
			}
		}

		//Process current file finished
		//All OK
		if(mTitle == MOk || mTitle == MNone__ )
			continue;

		//Connection lost
		if (!getConnection().isConnected())
		{
			getFTP()->BackToHosts();
			getFTP()->Invalidate();
		}

		//Return error
		return Failed;
	}/*EACH FILE*/

	//Remove empty directories after file deletion complete
	if(Move && !ci.FTPRename)
		for (size_t i = filelist.size()-1; i >= 0; i--)
		{
			if(CheckForEsc(false))
				return Canceled;

			std::wstring curName = filelist[i]->getFileName();

			if(filelist[i]->getType() == FTPFileInfo::directory)
				if(removeDirectory(curName))
				{
					if(i < items.size())
					{
						items[i].Flags &= ~PPIF_SELECTED;
					}
				}
		}
	return Succeeded;
}

PanelView::PutResult FTPFileView::PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode)
{
	PROC;
	return SucceededPut;
}

PanelView::CompareResult FTPFileView::Compare(const PluginPanelItem *i1, const PluginPanelItem *i2, unsigned int Mode)
{
	PROC;

	return UseInternal;
}

bool FTPFileView::DeleteFiles(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	PROC;

	FARWrappers::Screen scr;

	BeepOnLongOperation beep;
	int res = deleteFilesInt(PanelItem, ItemsNumber, OpMode);
	filelistCache_.clear();

	if(res == Canceled || res == Succeeded)
		return true;

	//Show self error message
	std::wstring nm;
	if(ItemsNumber == 1 )
		nm = PanelItem->FindData.lpwszFileName;
	else
		nm = boost::lexical_cast<std::wstring>(ItemsNumber) + L"  files/folders";

	getConnection().ConnectMessage(MCannotDelete, nm.c_str(), true, MOk);

	return true;
}

PanelView::Result FTPFileView::MakeDirectory(std::wstring &name, int OpMode)
{
	PROCP(name << L"mode: " << OpMode);

	if(!getConnection().isConnected())
		return Failed;

	//Edit name
	if(!IS_SILENT(OpMode) && !getFTP()->EditDirectory(name, std::wstring(L""), true, false))
		return Canceled;

	if(name.empty())
		return Canceled;

	FARWrappers::Screen  scr;

	//Try to create
	if(createDirectory(name, OpMode))
		return Succeeded;

	if(IS_SILENT(OpMode))
		return Failed;

	//If connection alive - report error
	if(getConnection().cmdLineAlive())
		getConnection().ConnectMessage(MCreateDirError, L"", true, MOk);
	else
		getConnection().ConnectMessageTimeout(MConnectionLost, L"", MRetry);

	return Succeeded;
}

bool FTPFileView::ProcessEvent(int Event, void *Param)
{
	PROCP(Event);

	if(getFTP()->CurrentState == fcsClose || getFTP()->CurrentState == fcsConnecting)
		return false;

	switch(Event)
	{
	case FE_BREAK:
		break;
	case FE_CLOSE:
		getFTP()->CurrentState = fcsClose;
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
			FTPCmdBlock blocked(getFTP(), true);

			if(g_manager.opt.ShowIdle)
				IdleMessage(getMsg(MKeepAliveExec), g_manager.opt.ProcessColor);

			getConnection().keepAlive();
			IdleMessage(NULL, 0);
		}
	    break;
	case FE_COMMAND:
		return getFTP()->ExecCmdLine(reinterpret_cast<wchar_t*>(Param), false);
		break;
	default:
	    break;
	}

	return false;
}

bool FTPFileView::ProcessKey(int Key, unsigned int ControlState)
{
	PROC;

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
		getFTP()->Invalidate();
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
		getConnection().getHost().codePage_ = getFTP()->SelectTable(getConnection().getHost().codePage_);
		getFTP()->Invalidate();
		return true;
	}

	if(ControlState == PKF_CONTROL && Key=='A')
	{
		setAttributes();
		getFTP()->Invalidate();
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

		FARWrappers::getInfo().Control(getFTP(), FCTL_GETPANELINFO, &pi);

		if (pi.CurrentItem >= pi.ItemsNumber)
			return false;

		std::wstring s;
		std::wstring path = getConnection().getCurrentDirectory();
		getHost().MkUrl(s, path.c_str(), FTP_FILENAME(&pi.PanelItems[pi.CurrentItem]) );
		QuoteStr(s);
		FARWrappers::getInfo().Control(getFTP(), FCTL_INSERTCMDLINE, (void*)Unicode::toOem(s).c_str());
		return true;
	}

	//Select
	if (ControlState == 0 && Key == VK_RETURN)
	{
		const FTPFileInfoPtr fileinfo = getCurrentFile();

		if(fileinfo->getFileName() != L"." && fileinfo->getFileName() != L"..")
			if(fileinfo->getType() == FTPFileInfo::directory || 
			   fileinfo->getType() == FTPFileInfo::symlink)
			{
				if(SetDirectory((fileinfo->getType() == FTPFileInfo::directory)? 
					fileinfo->getFileName() : fileinfo->getLink(),
					0))
				{
					FARWrappers::getInfo().Control(getFTP(), FCTL_UPDATEPANEL,NULL );

					struct PanelRedrawInfo RInfo;
					RInfo.CurrentItem = RInfo.TopPanelItem = 0;
					FARWrappers::getInfo().Control(getFTP(), FCTL_REDRAWPANEL, &RInfo);
				}
				return true;
			}
	}/*ENTER*/

	//Copy/Move
	if((ControlState == 0 || ControlState == PKF_SHIFT) && Key == VK_F6)
	{
		FTP *otherPlugin = OtherPlugin(getFTP());
		int  rc;

		if(!otherPlugin && ControlState == 0 && Key == VK_F6 )
			return false;

		PluginPanelItem item;
		if(!getFTP()->getCurrentPanelItem(item))
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

			FARWrappers::getInfo().Control(getFTP(),FCTL_UPDATEPANEL,NULL);
			FARWrappers::getInfo().Control(getFTP(),FCTL_REDRAWPANEL,NULL);

			FARWrappers::Screen::fullRestore();

			FARWrappers::getInfo().Control(ANOTHER_PANEL, FCTL_UPDATEPANEL,NULL);
			FARWrappers::getInfo().Control(ANOTHER_PANEL, FCTL_REDRAWPANEL,NULL);

			FARWrappers::Screen::fullRestore();
		}
		return true;
	}

	return false;
}
bool FTPFileView::SetDirectory(const std::wstring &dir, int OpMode)
{
	PROC;


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
				getFTP()->BackToHosts();
			return false;
		}

		size_t pos = oldDir.rfind(L'/');
		selFile_.assign(oldDir, pos+1, oldDir.size()-pos);
	}

	if(!getConnection().isConnected())
		return false;

	if(getFTP()->SetDirectoryStepped(dir, false))
		return true;

	return false;
}

bool FTPFileView::readFolder(const std::wstring &path, FileListPtr& filelist)
{
	PROC;

	if(!filelistCache_.find(path, filelist))
	{
		connection_.ls(path);

		filelist.reset(new FileList);
		if(!parseFtpDirOutput(filelist))
			return false;
	}
	return true;
}

bool FTPFileView::GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber, int OpMode)
{
	PROC;
	BOOST_ASSERT(itemList_.size() == 0);

	if(!readFolder(getCurrentDirectory(), filelist_))
		return false;

	BOOST_LOG(ERR, L"start adding.");

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

	BOOST_LOG(ERR, L"end adding.");

	return true;
}

void FTPFileView::FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber)
{
	PROC;
	BOOST_ASSERT(PanelItem == itemList_.items() && ItemsNumber == itemList_.size());

	BOOST_LOG(ERR, L"free find data");

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

	getFTP()->getPanelInfo(pi);

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
	// TODO listing getCurrentFileAttribute
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

	if(hConnect->chmod(filename, octal(Mode)))
	{
	   hConnect->filelist_.clear();
	   continue;
	}

	if ( !AskCode )
	continue;

	switch( getConnection()->ConnectMessage(MCannotChmod,m, true, MCopySkip,MCopySkipAll) ) {
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

	if(h.SetHostName(str, L"", L"") && getFTP()->GetHost(MSaveFTPTitle,&h,FALSE))
		h.Write(getCurrentDirectory());
}

const FTPFileInfoPtr FTPFileView::getCurrentFile() const
{
	PluginPanelItem curItem;
	bool res = getFTP()->getCurrentPanelItem(curItem);
	BOOST_ASSERT(res);
	return findfile(curItem.UserData);
}

const FTPFileInfoPtr FTPFileView::findfile(size_t id) const
{
	BOOST_ASSERT(id >= 1 && id <= filelist_->size());
	return (*filelist_)[id-1];
}

FTPFileInfoPtr FTPFileView::findfile(size_t id)
{
	BOOST_ASSERT(id >= 1 && id <= filelist_->size());
	return (*filelist_)[id-1];
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
	getFTP()->getPanelInfo(pi);
	int ni = -1;
	int n;

	for(n = 0; n < pi.ItemsNumber; n++)
	{
		if(item == pi.PanelItems[n].FindData.lpwszFileName)
			break;
	}

	// if case sensitive item not found, then use not case sensitive search.
	if(n == pi.ItemsNumber)
	{
		for(n = 0; n < pi.ItemsNumber; n++)
		{
			if(boost::iequals(item, pi.PanelItems[n].FindData.lpwszFileName))
				break;
		}
	}
	if(n != pi.ItemsNumber)
	{
		PanelRedrawInfo pri;
		pri.CurrentItem  = n;
		pri.TopPanelItem = pi.TopPanelItem;
		FARWrappers::getInfo().Control(getFTP(), FCTL_REDRAWPANEL, &pri);
	}
	return;
}

bool FTPFileView::reread()
{
	std::wstring olddir = getCurrentDirectory();
	std::wstring curfile = getCurrentFile()->getFileName();

	//Reread
	filelistCache_.clear();
	FARWrappers::getInfo().Control(getFTP(), FCTL_UPDATEPANEL, (void*)1);

	bool rc = olddir == getCurrentDirectory();
	if(rc)
		selFile_ = curfile;

	FARWrappers::getInfo().Control(getFTP(), FCTL_REDRAWPANEL, NULL);

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

FTPFileView::Result	FTPFileView::deleteFilesInt(const PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	PROC;

	if(ItemsNumber == 0)
		return Failed;

	if(!IS_SILENT(OpMode))
	{
		const wchar_t* MsgItems[]=
		{
			getMsg(MDeleteTitle),
			getMsg(MDeleteFiles),
			getMsg(MDeleteDelete),
			getMsg(MDeleteCancel)
		};

		if(FARWrappers::message(MsgItems, 2) != 0)
			return Canceled;

		if(ItemsNumber > 1)
		{
			wchar_t buff[128];
			_snwprintf(buff, ARRAY_SIZE(buff), getMsg(MDeleteNumberOfFiles), ItemsNumber);
			MsgItems[1] = buff;
			if(FARWrappers::message(MsgItems, 2, FMSG_WARNING|FMSG_DOWN) != 0)
				return Canceled;
		}
	}

	class DeleteWalker
	{
	private:
		bool        deleteAllFolders_;
		bool        skipAll_;
		int         opMode_;
		Connection* connection_;

		bool cannotDelete(const std::wstring &filename)
		{
			if(skipAll_ == false) 
			{
				switch(connection_->ConnectMessage(MCannotDelete, filename, true, MCopySkip, MCopySkipAll)) 
				{
					/*skip*/     
					case 0: 
						return true;

					/*skip all*/ 
					case 1: 
						skipAll_ = true;
						return true;

				default: 
					return false;
				}
			} else
				return true;
		};

		bool deleteFolder(const std::wstring &filename)
		{
			const wchar_t* MsgItems[]=
			{
				getMsg(MDeleteTitle),
				getMsg(MDeleteFolder),
				filename.c_str(),
				getMsg(MDeleteGroupDelete),
				getMsg(MDeleteGroupAll),
				getMsg(MDeleteGroupCancel)
			};

			switch(FARWrappers::message(MsgItems, 3, FMSG_WARNING|FMSG_DOWN)) 
			{
				/*ESC*/
				case -1:
				/*Cancel*/
				case 2:
					return false;

				/*Del*/
				case  0:
					break;

				/*DelAll*/
				case  1:
					deleteAllFolders_ = true;
					break;
			}
			return true;
		}

	public:
		DeleteWalker(int opMode, Connection* connection)
			: deleteAllFolders_(false), skipAll_(false), opMode_(opMode), connection_(connection)
		{}

		FTPFileView::WalkerResult operator()(bool enter, FTPFileInfoPtr &info, const std::wstring& path)
		{
			if(enter)
			{
				WinAPI::setConsoleTitle(info->getFileName());
				connection_->ConnectMessage(MDeleteTitle, info->getFileName());

				if(info->getType() == FTPFileInfo::directory)
				{
					if(!deleteAllFolders_ && !IS_SILENT(opMode_))
					{
						return deleteFolder(info->getFileName())? OK : Cancel;
					}
					return OK;
				} else
				{
					std::wstring filename = path;
					if(!filename.empty())
						filename += NET_SLASH;
					filename += info->getFileName();
					if(connection_->deleteFile(filename))
					{
						return OK;
					} else
					{
						return cannotDelete(filename)? OK : Cancel;
					}
				}
			} else
			{
				std::wstring filename = path;
				if(!filename.empty())
					filename += NET_SLASH;
				filename += info->getFileName();
				if(connection_->removedir(filename)) 
				{
					return OK;
				} else
				{
					return cannotDelete(filename)? OK : Cancel;
				}
			}
		}
	};

	DeleteWalker walker(OpMode, &getConnection());

 	return walk(PanelItem, ItemsNumber, getCurrentDirectory(), walker);
}


FTP* FTPFileView::getFTP() const
{
	BOOST_ASSERT(plugin_);
	return plugin_;
}


bool FTPFileView::removeDirectory(const std::wstring &dir)
{
	if(dir == L"." || dir == L".." || dir.empty())
		return true;

	filelistCache_.clear();

	//Dir
	if(getConnection().removedir(dir))
		return true;

	//Dir+slash
	if(getConnection().removedir(dir + L'/'))
		return true;

	//Full dir
	std::wstring fulldir = getCurrentDirectory() + L'/';
	fulldir.append(dir, dir[0] == L'/', dir.size());
	if(getConnection().removedir(fulldir) != RPL_OK)
		return true;

	//Full dir+slash
	fulldir += L'/';
	if(getConnection().removedir(fulldir) != RPL_OK)
		return true;

	return false;
}

bool FTPFileView::deleteFile(const std::wstring& filename)
{  
	filelistCache_.clear();
	return getConnection().deleteFile(filename) != 0;
}

bool FTPFileView::renameFile(const std::wstring &existing, const std::wstring &New)
{
	filelistCache_.clear();
	return getConnection().renamefile(existing, New);
}


bool FTPFileView::ftpSetCurrentDirectory(const std::wstring& dir)
{
	if(dir.empty())
		return false;

	if(getConnection().cd(dir))
	{
		ftpGetCurrentDirectory();
		return true;
	}
	return false;
}

static std::wstring unDupFF(const Connection &connect, std::string str)
{
	size_t n = str.find("\xFF\xFF");
	while(n != std::string::npos)
	{
		str.replace(n, 2, 1, '\xFF');
		n = str.find("\xFF\xFF", n);
	}
	return connect.FromOEM(str);
}

bool FTPFileView::ftpGetCurrentDirectory()
{
	std::wstring	dir;

	FARWrappers::Screen scr;
	if(!getConnection().pwd())
		return false;

	std::wstring s = unDupFF(getConnection(), getConnection().GetReply());
	std::wstring::const_iterator itr = s.begin();
	Parser::skipNumber(itr, s.end());

	getConnection().curdir_ = getConnection().getHost().serverType_->parsePWD(itr, s.end());
	return !getConnection().curdir_.empty();

}

bool FTPFileView::findFile(const std::wstring& filename, FTPFileInfoPtr& fileinfo)
{
	BOOST_ASSERT(0 && "TODO");

	if(!getConnection().ls(filename))
		return false;

	FileListPtr filelist(new FileList);
	if(!parseFtpDirOutput(filelist))
		return false;

	if(filelist->size() != 1)
		return false;

	fileinfo = *(filelist->begin());

	return true;
}


FTPFileView::Result FTPFileView::downloadFile(const std::wstring& remoteFile, const std::wstring& newFile, bool reget, bool asciiMode)
{
	/* Local file always use '\' as separator.
	Regardles of remote settings.
	*/
	std::wstring NewFile = newFile;
	FixLocalSlash(NewFile);

	getConnection().setRetryCount(0);

	int rc;
	do
	{
		OperateHidden(NewFile, true);
		if((rc = downloadFileInt(remoteFile, NewFile, reget, asciiMode)) == Succeeded)
		{
			OperateHidden(NewFile, false);
			return Succeeded;
		}

		if(GetLastError() == ERROR_CANCELLED )
		{
			BOOST_LOG(INF, L"GetFileCancelled: op: " << IS_SILENT(FP_LastOpMode));
			return IS_SILENT(FP_LastOpMode) ? Canceled : Failed;
		}

		int num = getConnection().getRetryCount();
		if(g_manager.opt.RetryCount > 0 && num >= g_manager.opt.RetryCount)
			return Failed;

		getConnection().setRetryCount(num+1);

		if(!getConnection().ConnectMessageTimeout(MCannotDownload, remoteFile, MRetry))
			return Failed;

		reget = true;

		if(getConnection().keepAlive())
			continue;

		saveUsedDirNFile();
		if(!getFTP()->Connect())
			return Failed;

	} while(true);
}

bool FTPFileView::downloadFileInt(std::wstring remoteFile, const std::wstring& newFile, bool reget, bool asciiMode)
{
	PROCP(L"[" << remoteFile << L"]->[" << newFile << L"] " << (reget? L"REGET" : L"NEW") << (asciiMode? L"ASCII" : L"BIN"));
	
	//mode
	if(getConnection().settype(asciiMode? TYPE_A : TYPE_I) == false)
		return false;

	//Create directory
	boost::filesystem::wpath path = newFile;
	try
	{
		fs::create_directories(path.branch_path());
	}
	catch (fs::basic_filesystem_error<boost::filesystem::wpath>& )
	{
		return false;
	}

	//Remote file
	if(!remoteFile.empty() && remoteFile[0] != L'/')
	{
		std::wstring full_name;
		full_name = getConnection().getCurrentDirectory();
		AddEndSlash(full_name, '/');
		remoteFile = full_name + remoteFile;
	}

	//Get file
	getConnection().IOCallback = true;
	if (reget && !getConnection().isResumeSupport())
	{
		getConnection().AddCmdLine(getMsg(MResumeRestart), ldRaw);
		reget = false;
	}
	bool result;
	if(reget)
		result = getConnection().reget(remoteFile, newFile);
	else
		result = getConnection().get(remoteFile, newFile);
	getConnection().IOCallback = false;

	return result;

}

void FTPFileView::saveUsedDirNFile()
{
	getHost().url_.directory_ = getConnection().getCurrentDirectory();

	//Save current file to restore
	selFile_ = getCurrentFile()->getFileName();
}

__int64  FTPFileView::fileSize(const std::wstring &filename)
{
	if(!getConnection().sizecmd(filename))
	{
		throw std::exception("file size command is broken");
		return -1;
	} else
	{
		const std::string& line = getConnection().GetReply();
		return Parser::parseUnsignedNumber(line.begin()+4, line.end(), 
			std::numeric_limits<__int64>::max(), false); 
	}
}


void FTPFileView::setDescriptionFlag(PluginPanelItem* items, int number)
{
	for(int i = 0; i < number; ++i)
		items[i].Flags |= PPIF_PROCESSDESCR;
}
