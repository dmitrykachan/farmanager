#include "stdafx.h"
#include "ftpfileview.h"
#include "Utils/uniconverts.h"
#include "ftp_Ftp.h"
#include "ftp_Int.h"
#include "farwrapper/dialog.h"
#include "progress.h"
#include "farwrapper/menu.h"


static std::wstring FDigit(__int64 val)
{
	return Utils::FDigit(val, g_manager.opt.dDelimit? g_manager.opt.dDelimiter : 0);
}

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

static void FileDescriptionToHostCoding(int OpMode,Connection *hConnect, std::wstring nm)
{
	if(!is_flag(OpMode,OPM_DESCR))
		return;

	FILE *SrcFile = _wfopen(nm.c_str(), L"r+b");
	if(!SrcFile)
		return;

	int   FileSize = (int)Fsize(SrcFile);
	boost::scoped_array<BYTE> Buf(new BYTE[FileSize*3+1]);
	size_t   ReadSize = fread(Buf.get(), 1,FileSize,SrcFile);
	size_t WriteSize = ReadSize; // TODO hConnect->FromOEM( Buf,ReadSize,sizeof(BYTE)*FileSize*3+1 );
	fflush(SrcFile);
	fseek(SrcFile,0,SEEK_SET);
	fflush(SrcFile);
	fwrite(Buf.get(),1,WriteSize,SrcFile);
	fflush(SrcFile);
	fclose(SrcFile);
}

static void FileDescriptionToLocalCoding(int OpMode, Connection *hConnect, const std::wstring &nm, const FILETIME &tm)
{
	if(!is_flag(OpMode,OPM_DESCR))
		return;

	HANDLE SrcFile = CreateFileW(nm.c_str(), GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL );
	DWORD  FileSize;

	if(SrcFile == INVALID_HANDLE_VALUE)
		return;

	if((FileSize = GetFileSize(SrcFile,NULL)) != INVALID_FILE_SIZE)
	{
		boost::scoped_array<unsigned char> buf(new unsigned char[FileSize]);
		ReadFile(SrcFile, buf.get(),FileSize,&FileSize,NULL);
		BOOST_ASSERT(0 && "TODO");
		// TODO DWORD WriteSize = hConnect->toOEM( Buf,FileSize );
		DWORD WriteSize = FileSize;
		SetFilePointer(SrcFile, 0, NULL, FILE_BEGIN);
		WriteFile(SrcFile, buf.get(),WriteSize,&WriteSize,NULL );
		SetEndOfFile(SrcFile);
	}
	SetFileTime(SrcFile,NULL,NULL, &tm);
	CloseHandle(SrcFile);
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

bool FTPFileView::parseFtpDirOutput(const std::wstring& path, FileListPtr &filelist)
{
	PROC;

	ServerTypePtr server = getHost()->serverType_;
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

	filelistCache_.add(path, filelist);

	return true;
}

PanelView::Result FTPFileView::GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const std::wstring &destPath, int OpMode)
{
	//Copy received items to internal data and use copy
	// because FAR delete it on next FCTL_GETPANELINFO
	FARWrappers::ItemList FilesList(PanelItem, ItemsNumber);

	return GetFiles(FilesList, Move, destPath, OpMode);
}

PanelView::Result FTPFileView::GetFiles(FARWrappers::ItemList &list, int Move, const std::wstring &destPath, int OpMode)
{
	Result rc = getFilesInt(list, Move, destPath, OpMode);

	FtpCmdBlock(&getConnection(), false);
	if(rc != Canceled)
		getFTP()->LongBeepEnd(true);

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

	info->asciiMode       = getHost()->AsciiMode;
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
	FTPFileView::WalkerResult operator()(bool enter, FTPFileInfoPtr info, const std::wstring& path)
	{
		if(enter)
		{
			FTPFileInfoPtr p(new FTPFileInfo(*info));
			if(!path.empty())
				p->setFileName(path + NET_SLASH + p->getFileName());
			list_->push_back(p);
		}
		return FTPFileView::OK;
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
	FTPFileView::WalkerResult operator()(bool enter, FTPFileInfoPtr info, const std::wstring& path)
	{
		if(enter && info->getType() == FTPFileInfo::directory)
		{
			FTPFileInfoPtr p(new FTPFileInfo(*info));
			if(!path.empty())
				p->setFileName(path + NET_SLASH + p->getFileName());
			list_->push_back(p);
			return FTPFileView::Skip;
		}
		return FTPFileView::OK;
	}
};
template<typename Pred>
PanelView::Result walkInt(FTPFileView& fileview, FTPFileInfoPtr &file, const std::wstring &path, Pred pred)
{
	PanelView::Result res = PanelView::Succeeded;
	if(file->getType() == FTPFileInfo::directory)
	{
		FTPFileView::WalkerResult cbstatus = pred(true, file, path);
		if(cbstatus == FTPFileView::Cancel)
			return PanelView::Canceled;
		if(cbstatus == FTPFileView::OK)
		{
			boost::shared_ptr<FileList> list(new FileList);
			std::wstring subpath = path;
			if(!subpath.empty())
				subpath += NET_SLASH;
			subpath += file->getFileName();
			if(fileview.readFolder(subpath, list))
			{
				FileList::iterator itr = list->begin();
				while(res == PanelView::Succeeded && itr != list->end())
				{
					if((*itr)->getFileName() != L"..")
						res = walkInt(fileview, *itr, subpath, pred);
					++itr;
				}
				cbstatus = pred(false, file, path);
				if(cbstatus == FTPFileView::Cancel)
					return PanelView::Canceled;
			}
		}
	} else
	{
		if(pred(true, file, path) == FTPFileView::Cancel)
			return PanelView::Canceled;
	}
	return PanelView::Succeeded;
}

template<typename Pred>
PanelView::Result walk(FTPFileView& fileview, const PluginPanelItem* panelItems, int itemsNumber, const std::wstring &path, Pred pred)
{
	PanelView::Result res = PanelView::Succeeded;
	for(int i = 0; res && i < itemsNumber; ++i)
	{
		res = walkInt(fileview, fileview.getFile(panelItems[i].UserData), path, pred);
	}
	return res;
}

template<typename Pred>
PanelView::Result walk(FTPFileView& fileview, PluginPanelItem** const pPanelItems, int itemsNumber, const std::wstring &path, Pred pred)
{
	PanelView::Result res = PanelView::Succeeded;
	for(int i = 0; res && i < itemsNumber; ++i)
	{
		res = walkInt(fileview, fileview.getFile(pPanelItems[i]->UserData), path, pred);
	}
	return res;
}

PanelView::Result FTPFileView::getFilesInt(FARWrappers::ItemList &items, int Move, const std::wstring& destPath, int OpMode)
{
	PROCP(L"Move: " << Move << L", destPath: " << destPath << L"Mode: " << OpMode);
	FileList			filelist;
	std::wstring		DestName;
	FARWrappers::Screen scr;
	bool				isDestDir;
	DWORD				DestAttr;
	int					mTitle;
	Result				rc;
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
		Result res = walk(*this, items.items(), static_cast<int>(items.size()), L"", MakeList(&filelist));
		if(res != Succeeded)
			return res;
	}
	else
	{
		Result res = walk(*this, items.items(), static_cast<int>(items.size()), L"", MakeRecursiveList(&filelist));
		if(res != Succeeded)
 			return res;
	}

 	if(ci.showProcessList && !showFilesList(filelist))
 		return Failed;

	if(!ci.FTPRename && ci.addToQueque)
	{
		BOOST_LOG(INF, L"Files added to queue [" << filelist.size() << L']');
		getFTP()->ListToQueque(filelist, ci);
		return Succeeded;
	}

	getFTP()->longBeep_.reset();

	//Calc full size
	FTPProgress trafficInfo(MStatusDownload, OpMode, filelist, FTPProgress::ShowProgress);

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

		//Init current
		trafficInfo.initFile(&getConnection(), currentFile, DestName);

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
			WinAPI::Stopwatch watch;
			ci.msgCode = getFTP()->AskOverwrite(ci.FTPRename ? MRenameTitle : MDownloadTitle, TRUE,
			                              findData, currentFile, ci.msgCode);

			switch( ci.msgCode )
			{
			case   ocOverAll:
			case      ocOver: break;
			case      ocSkip:
			case   ocSkipAll: 
				trafficInfo.skip();
				trafficInfo.Waiting(watch.getPeriod());
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
				resetFileCache();
			}
		} else
		{
			//Do download
			rc = downloadFile(curName, DestName,
						ci.msgCode == ocResume || ci.msgCode == ocResumeAll,
						ci.asciiMode, trafficInfo);
			if(rc == Succeeded)
			{
				//Process description
				FileDescriptionToLocalCoding(OpMode, &getConnection(), DestName, currentFile.getLastWriteTime());

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

PanelView::Result FTPFileView::PutFiles(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode)
{
	PROC;

	Result rc = putFilesInt(panelItems, itemsNumber, Move, OpMode);
	FtpCmdBlock(&getConnection(), false);
	resetFileCache();

	if(rc == Failed || rc == Canceled)
		getFTP()->LongBeepEnd(true);

	BOOST_LOG(INF, L"rc" << rc);
	return rc;
}


class MakeLocalList: boost::noncopyable
{
private:
	FileList *list_;
public:
	MakeLocalList(FileList* list)
		: list_(list)
	{
		BOOST_ASSERT(list);
	}
	FTPFileView::WalkerResult operator()(bool enter, WIN32_FIND_DATAW file, const std::wstring& path)
	{
		if(enter)
		{
			FTPFileInfoPtr fileinfo = FTPFileInfoPtr(new FTPFileInfo);
			std::wstring filename;
			if(path.empty())
				fileinfo->setFileName(file.cFileName);
			else
				fileinfo->setFileName(path + LOC_SLASH + file.cFileName);
			fileinfo->setFileSize(file.nFileSizeLow + (static_cast<__int64>(file.nFileSizeHigh) << 32));
			fileinfo->setWindowsAttribute(file.dwFileAttributes);

			list_->push_back(fileinfo);
		}
		return FTPFileView::OK;
	}
};

template<typename Pred>
PanelView::Result localWalk(WIN32_FIND_DATAW &file, const std::wstring &path, Pred& pred)
{
	if(is_flag(file.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		FTPFileView::WalkerResult cbstatus = pred(true, file, path);
		if(cbstatus == FTPFileView::Cancel)
			return FTPFileView::Canceled;
		if(cbstatus == FTPFileView::OK)
		{
			std::wstring subpath = path;
			if(!subpath.empty())
				subpath += LOC_SLASH;
			subpath += file.cFileName;

			WIN32_FIND_DATAW findFile;
			WinAPI::SmartHandle hFind(WinAPI::SmartHandle(FindFirstFileW((subpath + L"/*").c_str(), &findFile), FindClose));

			if(hFind.get() != INVALID_HANDLE_VALUE)
			{
				PanelView::Result res = PanelView::Succeeded;

				do
				{
					if(wcscmp(findFile.cFileName, L"..") == 0 || wcscmp(findFile.cFileName, L".") == 0)
						continue;
					res = localWalk(findFile, subpath, pred);
				} while(res == PanelView::Succeeded && FindNextFileW(hFind.get(), &findFile));
			}
			cbstatus = pred(false, file, path);
			if(cbstatus == FTPFileView::Cancel)
				return PanelView::Canceled;
		}
	} else
	{
		if(pred(true, file, path) == FTPFileView::Cancel)
			return PanelView::Canceled;
	}
	return PanelView::Succeeded;
}

template<typename Pred>
PanelView::Result localWalk(PluginPanelItem* panelItems, int itemsNumber, const std::wstring &path, Pred& pred)
{
	PanelView::Result res = PanelView::Succeeded;
	WIN32_FIND_DATAW f;
	for(int i = 0; res && i < itemsNumber; ++i)
	{
		FARWrappers::convertFindData(f, panelItems[i].FindData);
		res = localWalk(f, path, pred);
	}
	return res;
}

template<typename Pred>
PanelView::Result localWalk(PluginPanelItem** const panelItems, int itemsNumber, const std::wstring &path, Pred& pred)
{
	PanelView::Result res = PanelView::Succeeded;
	WIN32_FIND_DATAW f;
	for(int i = 0; res && i < itemsNumber; ++i)
	{
		FARWrappers::convertFindData(f, panelItems[i]->FindData);
		res = localWalk(f, path, pred);
	}
	return res;
}

static int findPanelItem(PluginPanelItem* panelItems, int itemsNumber, const std::wstring &name)
{
	for(int j = 0; j < itemsNumber; j++)
	{
		if(name == panelItems[j].FindData.lpwszFileName)
			return j;
	}
	return -1;
}

PanelView::Result FTPFileView::putFilesInt(PluginPanelItem* panelItems, int itemsNumber, int Move, int OpMode)
{
	FARWrappers::Screen scr;
	FTPCopyInfo       ci;

	if(itemsNumber == 0)
		return Succeeded;

	ci.destPath  = getConnection().getCurrentDirectory();

	ci.asciiMode       = getConnection().getHost()->AsciiMode;
	ci.addToQueque     = false;
	ci.msgCode         = is_flag(OpMode, OPM_FIND) ? ocOverAll : ocNone;
	ci.showProcessList = false;
	ci.download        = false;
	ci.uploadLowCase   = g_manager.opt.UploadLowCase;

	if(g_manager.opt.ShowUploadDialog && !IS_SILENT(OpMode))
	{
		if(!getFTP()->CopyAskDialog(Move, false, &ci))
			return Canceled;
	}

	AddEndSlash(ci.destPath, L'/');

	FileList filelist;
	Result res = localWalk(panelItems, itemsNumber, L"", MakeLocalList(&filelist));
	if(res != Succeeded)
		return res;
	if(ci.showProcessList && !showFilesList(filelist))
		return Canceled;

	if(ci.addToQueque)
	{
		BOOST_LOG(INF, L"Files added to queue");
		getFTP()->ListToQueque(filelist, ci);
		return Succeeded;
	}

	getFTP()->longBeep_.reset();

	FTPProgress trafficInfo(MStatusUpload, OpMode, filelist, FTPProgress::ShowProgress);

	for(size_t i = 0; i < filelist.size(); ++i)
	{
		if(!filelist[i]) // skip non checked
			continue;
		const FTPFileInfo& fileinfo = *filelist[i];
		std::wstring curName = fileinfo.getFileName();
		FixFTPSlash(curName);

		if(ci.uploadLowCase && !is_flag(fileinfo.getWindowsFileAttribute(), FILE_ATTRIBUTE_DIRECTORY))
		{
			size_t n = curName.find_last_of(L'/');
			if(n == std::wstring::npos)
				boost::to_lower(curName);
			else
				boost::to_lower(std::make_pair(curName.begin()+n+1, curName.end()));
		}
		std::wstring destName = ci.destPath + curName;

		if(is_flag(fileinfo.getWindowsFileAttribute(), FILE_ATTRIBUTE_DIRECTORY))
		{
			if(createDirectory(curName, OpMode))
				if(g_manager.opt.UpdateDescriptions)
				{
					int index = findPanelItem(panelItems, itemsNumber, curName);
					if(index != -1)
						panelItems[index].Flags |= PPIF_PROCESSDESCR;
				}
			continue;
		}

		trafficInfo.initFile(&getConnection(), fileinfo, destName);
		FTPFileInfoPtr destFileInfo = FTPFileInfoPtr(new FTPFileInfo);
		bool isDestFileExists = findFile(destName, destFileInfo, true);
		// TODO add filesize checking if !isFileExist
		// TODO add processing of connection lost => return false;

		if(isDestFileExists)
		{
			if(ci.msgCode == ocOver || ci.msgCode == ocSkip || ci.msgCode == ocResume)
				ci.msgCode = ocNone;
			
			FTPFileInfo fff;
			
			WinAPI::Stopwatch watch;
			ci.msgCode  = getFTP()->AskOverwrite(MUploadTitle, false, *destFileInfo, *filelist[i], ci.msgCode);
			switch(ci.msgCode)
			{
				case   ocOverAll:
				case      ocOver:
					break;

				case      ocSkip:
				case   ocSkipAll:
					trafficInfo.skip();
					trafficInfo.Waiting(watch.getPeriod());
					continue;
				case    ocResume:
				case ocResumeAll:
					break;

				case    ocCancel:
					return Canceled;
			}
		}
		//Upload
		Result rc = putFile(curName, destName, 
			isDestFileExists? (ci.msgCode == ocResume || ci.msgCode == ocResumeAll) : false, ci.asciiMode, trafficInfo);
		
		if(rc == Succeeded)
		{
			if(g_manager.opt.UpdateDescriptions)
			{
				int index = findPanelItem(panelItems, itemsNumber, curName);
				if(index != -1)
					panelItems[index].Flags |= PPIF_PROCESSDESCR;
			}
			FileDescriptionToHostCoding(OpMode, &getConnection(), curName);

			//Move source
			if(Move)
			{
				SetFileAttributesW(curName.c_str(), 0);
				::_wremove(curName.c_str());
			}
		} else
		{
			return rc;
		}
	}
	
	if(Move)
		for(size_t i = 0; i < filelist.size(); ++i)
		{
			if(!filelist[i]) // skip non checked
				continue;
			const FTPFileInfo& fileinfo = *filelist[i];

			if(CheckForEsc(false))
				return Canceled;

			if(is_flag(fileinfo.getWindowsFileAttribute(), FILE_ATTRIBUTE_DIRECTORY))
				if(RemoveDirectoryW(fileinfo.getFileName().c_str()))
				{
					int index = findPanelItem(panelItems, itemsNumber, fileinfo.getFileName());
					if(index != -1)
						panelItems[index].Flags &=~ PPIF_SELECTED;
						if (g_manager.opt.UpdateDescriptions)
							panelItems[index].Flags |= PPIF_PROCESSDESCR;
				}
		}
	return Succeeded;
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
	resetFileCache();

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
	{
		resetFileCache();
		return Succeeded;
	}

	if(IS_SILENT(OpMode))
		return Failed;

	//If connection alive - report error
	if(getConnection().isConnected())
		getConnection().ConnectMessage(MCreateDirError, L"", true, MOk);
	else
		getConnection().ConnectMessageTimeout(MConnectionLost, L"", MRetry);

	return Succeeded;
}

bool FTPFileView::ProcessEvent(int Event, void *Param)
{
	PROCP(Event);

//TODO	if(getFTP()->CurrentState == fcsClose || getFTP()->CurrentState == fcsConnecting)
//		return false;

	switch(Event)
	{
	case FE_BREAK:
		break;
	case FE_CLOSE:
		break;
	case FE_REDRAW:
		if(!selFile_.empty())
		{
			const std::wstring s = selFile_;
			selFile_.clear();  // prevent the recursion update
			selectPanelItem(s);
		}
	    break;
	case FE_IDLE:
		if(getConnection().isConnected() && getConnection().getKeepStopWatch().isTimeout())
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
		getConnection().getHost()->codePage_ = getFTP()->SelectTable(getConnection().getHost()->codePage_);
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
		resetFileCache();
		return false;
	}

	//Drop full name
	if(ControlState==PKF_CONTROL && Key=='F')
	{
		FARWrappers::PanelInfoAuto pi(getFTP(), false);

		if (pi.CurrentItem >= pi.ItemsNumber)
			return false;

		std::wstring s;
		std::wstring path = getConnection().getCurrentDirectory();
		getHost()->MkUrl(s, path.c_str(), FTP_FILENAME(&pi.PanelItems[pi.CurrentItem]) );
		QuoteStr(s);
		FARWrappers::getInfo().Control(getFTP(), FCTL_INSERTCMDLINE, (void*)(s.c_str()));
		return true;
	}

	//Select
	if (ControlState == 0 && Key == VK_RETURN)
	{
		const FTPFileInfoPtr fileinfo = getCurrentFile();

		if(FARWrappers::getInfo().Control(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, 0) != 0)
			return false; // the command line is not empty therefore do not change the dir

		if(fileinfo->getFileName() != L"." && fileinfo->getFileName() != L"..")
			if(fileinfo->getType() == FTPFileInfo::directory || 
			   fileinfo->getType() == FTPFileInfo::symlink)
			{
				if(SetDirectory((fileinfo->getType() == FTPFileInfo::directory)? 
					fileinfo->getFileName() : fileinfo->getLink(),
					0))
				{
					FARWrappers::getInfo().Control(getFTP(), FCTL_UPDATEPANEL,NULL );

					PanelRedrawInfo RInfo;
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

			FARWrappers::getInfo().Control(PANEL_PASSIVE, FCTL_UPDATEPANEL,NULL);
			FARWrappers::getInfo().Control(PANEL_PASSIVE, FCTL_REDRAWPANEL,NULL);

			FARWrappers::Screen::fullRestore();
		}
		return true;
	}

	return false;
}

bool FTPFileView::SetDirectoryStepped(const std::wstring &Dir, bool update)
{
	//Empty dir
	if (Dir.empty())
		return true;

	//Try direct change
	if(ftpSetCurrentDirectory(Dir))
	{
		if(update)
			getFTP()->Invalidate();
		return true;
	}

	//dir name contains only one name
	if(Dir.find(L'/') == std::wstring::npos)
	{
		//Cancel
		return false;
	}

	//Cancel changes in automatic mode
	if ( IS_SILENT(FP_LastOpMode) )
		return false;

	//Ask user
	{  static const wchar_t* MsgItems[] = {
		getMsg(MAttention),
		getMsg(MAskDir1),
		NULL,
		getMsg(MAskDir3),
		getMsg(MYes), getMsg(MNo) };

		MsgItems[2] = Dir.c_str();
		if (FARWrappers::message(MsgItems, 2, FMSG_WARNING|FMSG_LEFTALIGN) != 0 )
		{
			SetLastError( ERROR_CANCELLED );
			return false;
		}
	}

	//Try change to directory one-by-one
	std::wstring str;
	std::wstring::const_iterator itr = Dir.begin();
	std::wstring::const_iterator m; 

	do
	{
		if(*itr == L'/')
		{
			++itr;
			if(itr == Dir.end()) 
				break;
			str = L"/";
		} else
		{
			m = std::find(itr, Dir.end(), L'/');
			if(m != Dir.end())
			{
				str.assign(Dir.begin(), m);
				itr = m + 1;
			} else
				str = Dir;
		}

		BOOST_LOG(INF, L"Dir: [" << str << L"]");
		if (!ftpSetCurrentDirectory(str))
			return false;

		if (update)
			getFTP()->Invalidate();
	} while(m != Dir.end());

	return true;
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

	if(SetDirectoryStepped(dir, false))
		return true;

	return false;
}

bool FTPFileView::readFolder(const std::wstring &path, FileListPtr& filelist, bool useCache)
{
	PROC;

	if(!useCache || !filelistCache_.find(path, filelist))
	{
		connection_.ls(path);

		filelist.reset(new FileList);
		if(!parseFtpDirOutput(path, filelist))
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
		i.CustomColumnData[FTP_COL_MODE]= const_cast<wchar_t*>(file.getSecondAttribute().c_str());
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


const FtpHostPtr& FTPFileView::getHost() const
{
	return getConnection().getHost();
}

FtpHostPtr& FTPFileView::getHost()
{
	return getConnection().getHost();
}

void FTPFileView::copyNamesToClipboard()
{  
	std::wstring	FullName, CopyData;
	size_t			CopySize;
	int n;

	FARWrappers::PanelInfoAuto	pi(getFTP(), false);

	getHost()->MkUrl(FullName, getConnection().getCurrentDirectory(), L"");

	CopySize = (FullName.size() + 1 + 2 + 2)*pi.SelectedItemsNumber; // slash +	quotes and /r/n
	for(CopySize = n = 0; n < pi.SelectedItemsNumber; n++)
		CopySize += FTP_FILENAME(pi.SelectedItems[n]).size();

	CopyData.reserve(CopySize);

	for ( n = 0; n < pi.SelectedItemsNumber; n++ ) 
	{
		std::wstring s;
		s = FullName;
		AddEndSlash(s, L'/');
		s += FTP_FILENAME(pi.SelectedItems[n]);

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
	std::wstring  str;

	getHost()->url_.directory_ = getConnection().getCurrentDirectory();

	if(getFTP()->EditHostDlg(MSaveFTPTitle, getHost(), FALSE))
		getHost()->Write();

}

const FTPFileInfoPtr FTPFileView::getCurrentFile() const
{
	PluginPanelItem curItem;
	bool res = getFTP()->getCurrentPanelItem(curItem);
	BOOST_ASSERT(res);
	return getFile(curItem.UserData);
}

const FTPFileInfoPtr FTPFileView::getFile(size_t id) const
{
	BOOST_ASSERT(id >= 1 && id <= filelist_->size());
	return (*filelist_)[id-1];
}

FTPFileInfoPtr FTPFileView::getFile(size_t id)
{
	BOOST_ASSERT(id >= 1 && id <= filelist_->size());
	return (*filelist_)[id-1];
}

const std::wstring FTPFileView::getCurrentDirectory() const
{
	return getConnection().getCurrentDirectory();
}

void FTPFileView::setCurrentDirectory(const std::wstring& dir)
{
	getConnection().setCurrentDirectory(dir);
}

void FTPFileView::selectPanelItem(const std::wstring &item) const
{
	FARWrappers::PanelInfoAuto pi(getFTP(), false);
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
	resetFileCache();
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
			if(getConnection().makedir(name) == Connection::Done)
				break;
		}

		//Try relative path with end slash
		name	+= L'/';
		if(getConnection().makedir(name) == Connection::Done)
			break;

		//Try absolute path
		name = getConnection().getCurrentDirectory() + L'/';
		name.append(dir.begin() + (dir[0] == L'/'), dir.end());
		if(!iswspace(last)) 
		{
			if(getConnection().makedir(name) == Connection::Done)
				break;
		}

		//Try absolute path with end slash
		name	+= L'/';
		if(getConnection().makedir(name) == Connection::Done)
			break;

		return false;
	}while(0);

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
					if(connection_->deleteFile(filename) == Connection::Done)
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
				if(connection_->removedir(filename) == Connection::Done)
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

 	return walk(*this, PanelItem, ItemsNumber, getCurrentDirectory(), walker);
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

	resetFileCache();

	//Dir
	if(getConnection().removedir(dir) == Connection::Done)
		return true;

	//Dir+slash
	if(getConnection().removedir(dir + L'/') == Connection::Done)
		return true;

	//Full dir
	std::wstring fulldir = getCurrentDirectory() + L'/';
	fulldir.append(dir, dir[0] == L'/', dir.size());
	if(getConnection().removedir(fulldir) == Connection::Done)
		return true;

	//Full dir+slash
	fulldir += L'/';
	if(getConnection().removedir(fulldir)  == Connection::Done)
		return true;

	return false;
}

bool FTPFileView::deleteFile(const std::wstring& filename)
{  
	resetFileCache();
	return getConnection().deleteFile(filename) == Connection::Done;
}

bool FTPFileView::renameFile(const std::wstring &existing, const std::wstring &New)
{
	resetFileCache();
	return getConnection().renamefile(existing, New) == Connection::Done;
}


bool FTPFileView::ftpSetCurrentDirectory(const std::wstring& dir)
{
	if(dir.empty())
		return false;

	if(getConnection().cd(dir) == Connection::Done)
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
	if(getConnection().pwd() != Connection::Done)
		return false;

 	std::wstring s = unDupFF(getConnection(), getConnection().getReply());
	std::wstring::const_iterator itr = s.begin();

	const std::wstring &curdir = getConnection().getHost()->serverType_->parsePWD(itr, s.end());
	setCurrentDirectory(curdir);
	return !curdir.empty();

}

bool FTPFileView::findFile(const std::wstring& filename, FTPFileInfoPtr& fileinfo, bool useCache)
{
	FileListPtr filelist(new FileList);

	std::wstring path = getNetPathBranch(filename); 
	if(!readFolder(path, filelist, useCache))
		return false;

	std::wstring name = getName(filename);

	FileList::const_iterator i = filelist->begin();
	while(i != filelist->end())
	{
		if(name == (*i)->getFileName())
		{
			fileinfo = *i;
			return true;
		}
		++i;
	}
	return false;
}


FTPFileView::Result FTPFileView::downloadFile(const std::wstring& remoteFile, const std::wstring& newFile, bool reget, bool asciiMode, FTPProgress& trafficInfo)
{
	/* Local file always use '\' as separator.
	Regardles of remote settings.
	*/
	std::wstring NewFile = newFile;
	FixLocalSlash(NewFile);

	getConnection().setRetryCount(0);

	Result rc;
	do
	{
		OperateHidden(NewFile, true);
		rc = downloadFileInt(remoteFile, NewFile, reget, asciiMode, trafficInfo);
		if(rc == Succeeded)
		{
			OperateHidden(NewFile, false);
			return Succeeded;
		}

		if(rc == Canceled)
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

		if(getConnection().isConnected())
			continue;

		saveUsedDirNFile();
		if(!getFTP()->Connect(getConnection().getHost()))
			return Failed;

	} while(true);
}

static PanelView::Result ConnectionResultToPanelResult(Connection::Result res)
{
	if(res == Connection::Done)
		return PanelView::Succeeded;
	if(res == Connection::Cancel)
		return PanelView::Canceled;
	return PanelView::Failed;
}


FTPFileView::Result FTPFileView::downloadFileInt(std::wstring remoteFile, const std::wstring& newFile, bool reget, bool asciiMode, FTPProgress& trafficInfo)
{
	PROCP(L"[" << remoteFile << L"]->[" << newFile << L"] " << (reget? L"REGET" : L"NEW") << (asciiMode? L"ASCII" : L"BIN"));
	
	//Create directory
	boost::filesystem::wpath path = newFile;
	try
	{
		fs::create_directories(path.branch_path());
	}
	catch (fs::basic_filesystem_error<boost::filesystem::wpath>& )
	{
		return Failed;
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
	Result result;
	if(reget)
		result = ConnectionResultToPanelResult(getConnection().getit(remoteFile, newFile, 1, L"a+", trafficInfo));
	else
		result = ConnectionResultToPanelResult(getConnection().getit(remoteFile, newFile, 0, L"w", trafficInfo));
	getConnection().IOCallback = false;

	return result;
}

void FTPFileView::saveUsedDirNFile()
{
	getHost()->url_.directory_ = getConnection().getCurrentDirectory();

	//Save current file to restore
	selFile_ = getCurrentFile()->getFileName();
}

void FTPFileView::setDescriptionFlag(PluginPanelItem* items, int number)
{
	for(int i = 0; i < number; ++i)
		items[i].Flags |= PPIF_PROCESSDESCR;
}

void resetNotChecked(FileList filelist, const FARWrappers::Menu& menu)
{
	BOOST_ASSERT(filelist.size() == menu.size());
	for(size_t i = 0; i < filelist.size(); ++i)
		if(!menu.isChecked(i))
			filelist[i].reset();
}

bool FTPFileView::showFilesList(FileList& filelist)
{
	if(filelist.empty())
		return false;

	FARWrappers::Menu menu(FMENU_SHOWAMPERSAND);
	menu.reserve(filelist.size());

	size_t width = 0;
	size_t sizeWidth = 0;
	size_t countWidth = 0;
	for(size_t i = 0; i < filelist.size(); ++i)
	{
		const FTPFileInfo& f = *filelist[i];
		const std::wstring& name = f.getFileName();
		width = std::max(width, 
			getName(name).size() + 2*std::count(name.begin(), name.end(), LOC_SLASH));
		int q = FDigit(123).size();
		sizeWidth = std::max(sizeWidth, FDigit(f.getFileSize()).size());
		if(is_flag(f.getWindowsFileAttribute(), FILE_ATTRIBUTE_DIRECTORY))
		{
			countWidth = 1;
		}
	}
	width = std::min(std::max(60u, width), WinAPI::getConsoleWidth()-8u);
	width -= sizeWidth + countWidth + 2;

	std::wstring str;
	__int64 totalSize = 0;
	size_t  totalCount = 0;
	for(size_t i = 0; i < filelist.size(); ++i)
	{
		const FTPFileInfo& f = *filelist[i];

		if(!is_flag(f.getWindowsFileAttribute(), FILE_ATTRIBUTE_DIRECTORY))
		{
			totalSize += f.getFileSize();
			totalCount++;
		}
		const std::wstring& fullname = f.getFileName();

		str.assign(2*std::count(fullname.begin(), fullname.end(), LOC_SLASH), L' ');
		std::wstring name = getName(fullname);
		if(str.size() + name.size() > width)
		{
			str.append(name, 0, width - str.size()-1);
			str += FAR_SBMENU_CHAR;
		} else
		{
			str += name;
			str.resize(width, L' ');
		}

		if(sizeWidth)
		{
			std::wstring sizeStr = FDigit(f.getFileSize());
			str += FAR_VERT_CHAR + std::wstring(sizeWidth-sizeStr.size(), L' ') + sizeStr;
		}
		if(countWidth)
		{
			str += FAR_VERT_CHAR;
			if(is_flag(f.getWindowsFileAttribute(), FILE_ATTRIBUTE_DIRECTORY))
			{
				std::wstring countStr = FDigit(1);
				str += std::wstring(countWidth-countStr.size(), L' ') + countStr;
			} else
				str += std::wstring(countWidth, L' ');
		}

		menu.addItem(str);
		menu.setCheck(i);
	}

	menu.setBottom(getMsg(MListFooter));
	menu.setHelpTopic(L"FTPFilesList");
	const int keys[] = {VK_INSERT, VK_F2, 0};
	menu.setBreakKeys(keys);

	do
	{
		__int64 selectedSize = 0;
		size_t selectedCount = 0;
		for(size_t i = 0; i < menu.size(); i++)
		{
			const FTPFileInfo& f = *filelist[i];

			if(menu.isChecked(i) && !is_flag(f.getWindowsFileAttribute(), FILE_ATTRIBUTE_DIRECTORY))
			{
				selectedCount++;
				selectedSize += f.getFileSize();
			}
		}
		std::wstring title = getMsg(MListTitle);
		title += L" (";
		title += FDigit(selectedSize);
		if(totalSize != selectedSize)
		{
			title += L'{';
			title += FDigit(totalSize);
			title += L'}';
		}
		title += L'/';
		title += FDigit(selectedCount);
		if(selectedCount != totalCount)
		{
			title += L'{';
			title += FDigit(totalCount);
			title += L'}';
		}
		title += L')';
		menu.setTitle(title);

		int n = menu.show();
		if(n == -1) // ESC presses
			return false;

		switch(menu.getBreakIndex())
		{
		case -1: // Enter is pressed
			resetNotChecked(filelist, menu);
			return true;
		case 0:  // Insert is pressed
			{
				bool newvalue = !menu.isChecked(n);
				menu.setCheck(n, newvalue);
				if(is_flag(filelist[n]->getWindowsFileAttribute(), FILE_ATTRIBUTE_DIRECTORY))
				{
					const std::wstring& name = filelist[n]->getFileName();
					int level = std::count(name.begin(), name.end(), LOC_SLASH);
					size_t i;
					for(i = n+1; i < menu.size(); ++i)
					{
						const std::wstring& nam = filelist[i]->getFileName();
						int lvl = std::count(nam.begin(), nam.end(), LOC_SLASH);
						if(lvl <= level)
							break;
						menu.setCheck(i, newvalue);
					}
					menu.select(i);
				} else
				{
					if(static_cast<size_t>(n)+1 < menu.size())
						menu.select(static_cast<size_t>(n)+1);
				}
				break;
			}

		case 1:  // F2 is pressed
			resetNotChecked(filelist, menu);
			saveList(filelist);
			return true;
		default:
			BOOST_ASSERT(0);
		}

	} while(true);
}

static std::wstring getOtherPath()
{
	FARWrappers::PanelInfoAuto api(false, true);

	if(api.PanelType == PTYPE_FILEPANEL && !api.Plugin)
		return api.lpwszCurDir;

	FARWrappers::PanelInfoAuto pi(true, true);
	if(pi.PanelType == PTYPE_FILEPANEL && !pi.Plugin)
		return pi.lpwszCurDir;

	FARWrappers::message(MFLErrCReate, MFLErrGetInfo, FMSG_WARNING);
	return L"";
}


void FTPFileView::saveList(FileList& filelist)
{
	BOOST_ASSERT(0 && "TODO");

	g_manager.opt.sli.path = getOtherPath();
	AddEndSlash(g_manager.opt.sli.path, LOC_SLASH);
	g_manager.opt.sli.path += L"ftplist.lst";
	/*
	if ( !AskSaveList(&g_manager.opt.sli) )
	return;

	FILE *f = _wfopen(g_manager.opt.sli.path.c_str(), g_manager.opt.sli.Append ? L"a" : L"w" );
	if ( !f )
	{
	FARWrappers::message(MFLErrCReate, getMsg(MFLErrCReate), FMSG_ERRORTYPE | FMSG_WARNING)
	return;
	}

	PluginPanelItem* p;
	size_t           n;
	int              level;
	char             str[1024+2],
	BasePath[1024+2];
	std::string		CurrentUrlPath;

	SNprintf( BasePath, sizeof(BasePath),
	"%s%s%s%s",
	g_manager.opt.sli.AddPrefix ? "ftp://" : "",
	g_manager.opt.sli.AddPasswordAndUser ? Message( "%s:%s@",hConnect->userName_, hConnect->UserPassword ) : "",
	hConnect->hostname,
	hConnect->curdir_.c_str() );
	TAddEndSlash( BasePath,'/' );

	if ( g_manager.opt.sli.ListType == sltTree )
	fprintf( f,"BASE: \"%s\"\n",BasePath );

	for( n = 0; n < il->Count(); n++ )
	{
	p = il->Item(n);

	if (p->Reserved[0] == UINT_MAX)
	continue;

	//URLS --------------------------------------
	if ( g_manager.opt.sli.ListType == sltUrlList ) {
	if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
	continue;

	FixFTPSlash( FTP_FILENAME(p) );
	SNprintf( str,sizeof(str),"%s%s",BasePath,FTP_FILENAME(p) );
	if ( g_manager.opt.sli.Quote ) QuoteStr( str );

	fprintf( f,"%s\n",str );
	} else
	//TREE --------------------------------------
	if ( g_manager.opt.sli.ListType == sltTree ) {
	TStrCpy( str, Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() );

	FixFTPSlash( str );
	for( m = str,level = 0;
	(m=strchr(m,'/')) != NULL;
	m++,level++ );

	fprintf( f,"%*c", level*2+2, ' ' );
	m = strrchr( str,'/' );

	if (m) m++; else m = str;

	fprintf( f,"%c%s",
	is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ? '/' : ' ', m );

	if ( g_manager.opt.sli.Size ) {
	level = Max( 1, g_manager.opt.sli.RightBound - 10 - level*2 - 2 - 1 - (int)strlen(m) );
	fprintf( f,"%*c",level,' ' );

	if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
	fprintf( f,"<DIR>" );
	else
	fprintf( f,"%10I64u",
	((__int64)p->FindData.nFileSizeHigh) * UINT_MAX + ((__int64)p->FindData.nFileSizeLow) );
	}
	fprintf( f,"\n" );

	} else
	//GROUPS ------------------------------------
	if ( g_manager.opt.sli.ListType == sltGroup )
	{
	if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
	continue;

	FixFTPSlash( FTP_FILENAME(p) );
	SNprintf( str, sizeof(str), "%s%s", BasePath, FTP_FILENAME(p) );
	if ( !is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
	*strrchr( str,'/' ) = 0;

	if(!boost::iequals(CurrentUrlPath, str))
	{
	CurrentUrlPath = str;
	fprintf( f,"\n[%s]\n", CurrentUrlPath.c_str());
	}
	TStrCpy( str, Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() );
	FixFTPSlash( str );
	m = strrchr(str,'/');
	if (m) m++; else m = str;
	fprintf( f," %s", m );

	if ( g_manager.opt.sli.Size ) {
	level = Max( 1, g_manager.opt.sli.RightBound - 10 - (int)strlen(m) - 1 );
	fprintf( f,"%*c%10I64u",
	level,' ',
	((__int64)p->FindData.nFileSizeHigh) * UINT_MAX + ((__int64)p->FindData.nFileSizeLow) );
	}
	fprintf( f,"\n" );
	}
	}
	fclose(f);

	const wchar_t* itms[] = { getMsg(MFLDoneTitle), getMsg(MFLFile), g_manager.opt.sli.path, getMsg(MFLDone), getMsg(MOk) };
	FMessage( FMSG_LEFTALIGN,NULL,itms,5,1 );
	*/
}

PanelView::Result FTPFileView::putFile(const std::wstring &localFile, const std::wstring &remoteFile, bool Reput, bool AsciiMode, FTPProgress& trafficInfo)
{
	getConnection().setRetryCount(0);
	do
	{
		std::wstring cmd = ((getConnection().sunique) ? g_manager.opt.cmdPutUniq : g_manager.opt.cmdStor); //STORE or PUT
		Result rc = ConnectionResultToPanelResult(getConnection().sendrequest(cmd, localFile, remoteFile.empty()? localFile : remoteFile, AsciiMode, trafficInfo));
			
		if(rc == Succeeded)
			return Succeeded;

		if(rc == Canceled)
			return IS_SILENT(FP_LastOpMode)? Canceled : Failed;

		int num = getConnection().getRetryCount();
		if ( g_manager.opt.RetryCount > 0 && num >= g_manager.opt.RetryCount )
			return Failed;

		getConnection().setRetryCount(num+1);

		if (!getConnection().ConnectMessageTimeout(MCannotUpload, remoteFile, MRetry))
			return Failed;

		Reput = true;

		if(getConnection().isConnected())
			continue;

		if ( !getFTP()->Connect(getConnection().getHost()) )
			return Failed;
	} while(true);
}


void FTPFileView::resetFileCache()
{
	filelistCache_.clear();
}

const wchar_t* FTPFileView::InsertSelectedToQueue(bool download)
{
	FTPCopyInfo ci;

	if(getFTP()->FTPMode())
		return getMsg(MQErrUploadHosts);

	FARWrappers::PanelInfoAuto activepi(true, false);
	FARWrappers::PanelInfoAuto passivepi(false, false);

	FARWrappers::PanelInfoAuto* pi = download? &activepi : &passivepi;

	if(pi->SelectedItemsNumber <= 0 ||
		pi->SelectedItemsNumber == 1 && !is_flag(pi->SelectedItems[0]->Flags, PPIF_SELECTED))
		return getMsg(MErrNoSelection);

	if(passivepi.PanelType != PTYPE_FILEPANEL || passivepi.Plugin)
		return getMsg(MErrNotFiles);

	Result   res;
	FileList filelist;

	if(download)
	{
		res = walk(*this, pi->SelectedItems, pi->SelectedItemsNumber, L"", MakeList(&filelist));
	} else
	{
		res = localWalk(pi->SelectedItems, pi->SelectedItemsNumber, L"", MakeLocalList(&filelist));
	}

	FARWrappers::Screen::fullRestore();
	if(res != Succeeded)
		return (res == Canceled)? NULL : getMsg(MErrExpandList);

	ci.download = download;
	if(download)
		ci.destPath = passivepi.lpwszCurDir;
	else
		ci.destPath = getCurrentDirectory();

	getFTP()->ListToQueque(filelist, ci);

	return NULL;
}

void FTPFileView::setSelectedFile(const std::wstring& filename)
{
	selFile_ = filename;
}