#include "stdafx.h"

#pragma hdrstop

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "ftp_Int.h"

#include "panelview.h"
#include "farwrapper/menu.h"
#include "utils/uniconverts.h"
#include "farwrapper/message.h"
#include "progress.h"

int		FP_LastOpMode    = 0;

FTPPluginManager g_manager;

int           SocketInitializeError = 0;

typedef void (_cdecl *AbortProc)(void);

AbortProc     ExitProc;

FTP *WINAPI OtherPlugin( FTP *p )
{
	if ( !p )
		return p;

	size_t n = g_manager.getPlugin(p);
	BOOST_ASSERT(n == 0 || n == 1);
	return g_manager.getPlugin(1-n);
}

size_t WINAPI PluginPanelNumber( FTP *p )
{
	return g_manager.getPlugin(p)+1;
}


class FPOpMode
{
private:
	int		opMode_;
public:
	FPOpMode( int mode ) { opMode_ = FP_LastOpMode; FP_LastOpMode = mode; }
	~FPOpMode()          { FP_LastOpMode = opMode_; }
};



void _cdecl CloseUp()
{
	FTP::backups_.eraseAll();

	Connection::cleanupWSA();

	if(ExitProc)
		ExitProc();
}

void FTPPluginManager::addWait(size_t tm)
{
	for(size_t n = 0; n < FTPPanels_.size(); n++)
		if(FTPPanels_[n] && FTPPanels_[n]->getConnection().IOCallback)
			FTPPanels_[n]->getConnection().TrafficInfo->Waiting(tm);
}


void FTPPluginManager::addPlugin(FTP *ftp)
{
	for(size_t i = 0; i < FTPPanels_.size(); i++)
		if(FTPPanels_[i] == 0)
		{
			FTPPanels_[i] = ftp;
			return;
		}

	BOOST_ASSERT(!"More then two plugins in a time !!");
}

size_t FTPPluginManager::getPlugin(FTP *ftp)
{
	for(size_t i = 0; i < FTPPanels_.size(); i++)
		if(FTPPanels_[i] == ftp)
			return i;
	BOOST_ASSERT(0 && "Invalid pointer to FTP");
	return 0;
}

FTP* FTPPluginManager::getPlugin(size_t n)
{
	return FTPPanels_[n];
}


void FTPPluginManager::removePlugin(FTP *ftp)
{
	size_t i = getPlugin(ftp);

	// TODO WTF: move 2.

	const wchar_t* rejectReason;
	if(FTP::backups_.find(ftp))
	{
		ftp->SetBackupMode();
		return;
	}

	const wchar_t* itms[] =
	{
		getMsg(MRejectTitle),
		getMsg(MRejectCanNot),
		NULL,
		getMsg(MRejectAsk1),
		getMsg(MRejectAsk2),
		getMsg(MRejectIgnore), getMsg(MRejectSite)
	};

	do{
		if ( (rejectReason = ftp->CloseQuery()) == NULL ) break;

		itms[2] = rejectReason;
		if(FARWrappers::message(itms, 2, FMSG_LEFTALIGN|FMSG_WARNING, L"CloseQueryReject") != 1)
			break;

		FTP::backups_.add(ftp);
		if(!FTP::backups_.find(ftp))
		{
			FARWrappers::message(MRejectFail);
			break;
		}

		return;
	}while(0);
	
	delete ftp;
	FTPPanels_[i] = 0;
}


static AbortProc CTAbortProc = NULL;

AbortProc WINAPI AtExit( AbortProc p )
{
	AbortProc old = CTAbortProc;
	CTAbortProc = p;
	return old;
}

extern "C"
{
void WINAPI ExitFARW( void )
{
	if(CTAbortProc)
	{
		CTAbortProc();
		CTAbortProc = NULL;
	}
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	PROCP(Info);

	FARWrappers::setStandartMessages(MYes, MNo, MOk, MAttention);
	FARWrappers::setStartupInfo(*Info);
	ExitProc = AtExit( CloseUp );

	g_manager.openRegKey(std::wstring(Info->RootKey) + L'\\' + L"ftp_debug");
	g_manager.readCfg();
	LogCmd(L"FTP plugin loaded", ldInt);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{  
	PROCP(Info);

	static const wchar_t* PluginMenuStrings[1];
	static const wchar_t* PluginCfgStrings[1];

	if(g_manager.opt.AddToDisksMenu)
	{
		static std::vector<std::wstring> DiskStrings;

		DiskStrings.clear();
		DiskStrings.reserve(1 + FTP::backups_.size());
		DiskStrings.push_back(getMsg(MFtpDiskMenu));

		FTPHost* p;
		size_t	uLen = 0, hLen = 0;
		std::wstring	str;

		Backup::const_iterator itr = FTP::backups_.begin();
		while(itr != FTP::backups_.end())
		{
			if((*itr)->FTPMode())
			{
				p = &(*itr)->chost_;
				uLen = std::max(uLen, p->url_.username_.size());
				hLen = std::max(hLen, p->url_.fullhostname_.size());
			}
			++itr;
		}

		itr = FTP::backups_.begin();
		int i = 0;
		while(itr != FTP::backups_.end())
		{
			str = (*itr)->panel_->getCurrentDirectory();
			std::wstring disk = L"FTP: ";
			if((*itr)->FTPMode())
			{
				p = &(*itr)->chost_;
				disk += p->url_.username_ + std::wstring(uLen+1-p->url_.username_.size(), L' ') +
					p->url_.fullhostname_ + std::wstring(hLen+1-p->url_.fullhostname_.size(), L' '); // FTP: %-*s %-*s %s"
			}
			DiskStrings.push_back(disk + boost::lexical_cast<std::wstring>(++i) + str);
			++itr;
		}

		static std::vector<int>				DiskMenuNumbers;
		DiskMenuNumbers.clear();
		DiskMenuNumbers.reserve(DiskStrings.size());
		DiskMenuNumbers.push_back(g_manager.opt.DisksMenuDigit);
		DiskMenuNumbers.insert(DiskMenuNumbers.end(), DiskStrings.size()-1, 0);
		
		static std::vector<const wchar_t*> DiskMenuStrings;
		DiskMenuStrings.clear();
		DiskMenuStrings.reserve(DiskStrings.size());
		for(size_t i = 0; i < DiskStrings.size(); ++i)
			DiskMenuStrings.push_back(DiskStrings[i].c_str());

		Info->DiskMenuStrings           = &DiskMenuStrings[0];
		Info->DiskMenuNumbers           = &DiskMenuNumbers[0];
		Info->DiskMenuStringsNumber     = static_cast<int>(DiskMenuStrings.size());
	} else
	{
		Info->DiskMenuStrings           = 0;
		Info->DiskMenuNumbers           = 0;
		Info->DiskMenuStringsNumber     = 0;
	}

	PluginMenuStrings[0] = getMsg(MFtpMenu);
	PluginCfgStrings[0]  = getMsg(MFtpMenu);

	Info->StructSize                = sizeof(*Info);
	Info->Flags                     = 0;

	Info->PluginMenuStrings         = PluginMenuStrings;
	Info->PluginMenuStringsNumber   = g_manager.opt.AddToPluginsMenu ? (sizeof(PluginMenuStrings)/sizeof(PluginMenuStrings[0])):0;

	Info->PluginConfigStrings       = PluginCfgStrings;
	Info->PluginConfigStringsNumber = sizeof(PluginCfgStrings)/sizeof(PluginCfgStrings[0]);

	Info->CommandPrefix             = FTP_CMDPREFIX;
}

int WINAPI ConfigureW(int ItemNumber)
{
	PROCP(ItemNumber);

	BOOST_ASSERT(ItemNumber == 0);
	return Config();
}

HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item)
{  
	PROCP(OpenFrom << L"," << static_cast<int>(Item));
	FTP *Ftp;

	if(Item == 0 || Item > static_cast<int>(FTP::backups_.size()))
		Ftp = new FTP;
	else
	{
		Ftp = FTP::backups_.get(Item-1);
		Ftp->SetActiveMode();
	}

	g_manager.addPlugin(Ftp);

	Ftp->Call();
	BOOST_LOG(INF, L"FTP handle: " << Ftp);
	do{
		if (OpenFrom==OPEN_SHORTCUT)
		{
			if (!Ftp->ProcessShortcutLine((char *)Item))
				break;
			Ftp->End();
		} else
			if (OpenFrom==OPEN_COMMANDLINE)
			{
				if (!Ftp->ProcessCommandLine(reinterpret_cast<wchar_t*>(Item)))
					break;
			}

		Ftp->End();
		return (HANDLE)Ftp;
	}while(0);

	g_manager.removePlugin(Ftp);

	return INVALID_HANDLE_VALUE;
}

void WINAPI ClosePluginW(HANDLE hPlugin)
{
	PROCP(hPlugin);
	FTP *p = (FTP*)hPlugin;

	g_manager.removePlugin(p);
}

int WINAPI GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	PROCP(hPlugin);
	FPOpMode _op(OpMode);
	FTP*     p = (FTP*)hPlugin;

	p->Call();
	FARWrappers::Screen src;
	int rc = p->panel_->GetFindData(pPanelItem, pItemsNumber, OpMode);
	p->End(rc);

	return rc;
}

void WINAPI FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	FTP*     p = (FTP*)hPlugin;

	p->Call();
	p->panel_->FreeFindData(PanelItem,ItemsNumber);
	p->End();
}

void WINAPI GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info)
{
	FTP*     p = (FTP*)hPlugin;

	p->Call();
	p->GetOpenPluginInfo(Info);
	p->End();
}


int WINAPI GetMinFarVersionW(void)
{
	return MAKEFARVERSION(1,80,267);
}


int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t* Dir,int OpMode)
	{
	FPOpMode _op(OpMode);
	FTP*     p = (FTP*)hPlugin;

	p->Call();
	int rc = p->panel_->SetDirectory(Dir, OpMode);
	p->End(rc);

	return rc;
}

int WINAPI GetFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t *DestPath,int OpMode)
{
	FPOpMode _op(OpMode);
	FTP*     p = (FTP*)hPlugin;
	if ( !p || !DestPath || !DestPath[0] )
		return FALSE;

	p->Call();
	PanelItem->Description = 0;
	int rc = p->panel_->GetFiles(PanelItem, ItemsNumber, Move, std::wstring(DestPath), OpMode);
	p->End(rc);
	return rc;
}

int WINAPI PutFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode)
{
	FPOpMode _op(OpMode);
	FTP*     p = (FTP*)hPlugin;

	p->Call();
	int rc = p->panel_->PutFiles(PanelItem,ItemsNumber,Move,OpMode);
	p->End(rc);
	return rc;
}

int WINAPI DeleteFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
{
	FPOpMode _op(OpMode);
	FTP*     p = (FTP*)hPlugin;

	p->Call();
	int rc = p->panel_->DeleteFiles(PanelItem, ItemsNumber, OpMode);
	p->End(rc);
	return rc;
}

int WINAPI MakeDirectoryW(HANDLE hPlugin, char* name,int OpMode)
{  
	FPOpMode _op(OpMode);

	if ( !hPlugin )
		return FALSE;

	FTP*     p = (FTP*)hPlugin;

	p->Call();
	std::wstring s = Unicode::fromOem(name);
	int rc = p->panel_->MakeDirectory(s, OpMode);
	p->End(rc);

	return rc;
}

int WINAPI ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState)
{  
	PROCP(hPlugin << L", " << std::hex << Key << L", " << ControlState);
	FTP*     p = (FTP*)hPlugin;
	p->Call();
    int rc = p->ProcessKey(Key,ControlState);
	p->End(rc);

	return rc;
}

int WINAPI ProcessEventW(HANDLE hPlugin,int Event,void *Param)
{
	FTP*     p = (FTP*)hPlugin;

	static const wchar_t* evts[] = {L"CHANGEVIEWMODE", L"REDRAW", L"IDLE", L"CLOSE", L"BREAK", L"COMMAND" };
	PROCP(hPlugin << L", " << ((Event < ARRAY_SIZE(evts)) ? evts[Event] : L"<unk>") << 
		L"[" << Param << L"]" );

	p->Call();
		int rc = p->panel_->ProcessEvent(Event,Param);
	p->End(rc);
	return rc;
}

int WINAPI CompareW( HANDLE hPlugin,const PluginPanelItem *i,const PluginPanelItem *i1,unsigned int Mode )
{
	FTP*     p = (FTP*)hPlugin;
	return p->panel_->Compare(i, i1, Mode);
}

} // extern c

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID ptr )
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		BOOST_LOG_INIT((L"[" >> boost::logging::level >> L"],"
			>> boost::logging::time >> L" "
			>> boost::logging::filename >> L"("
			>> boost::logging::line >> L"),"
			>> boost::logging::trace
			>> boost::logging::eol), // log format
			WRN);                      // log level

		BOOST_LOG_ADD_OUTPUT_STREAM(new boost::logging::wdebugstream);
	}

	if(reason == DLL_PROCESS_DETACH)
		ExitFARW();

	return true;
}
