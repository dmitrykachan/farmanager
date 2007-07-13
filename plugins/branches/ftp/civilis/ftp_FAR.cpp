#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"
#include <mem.inc>

const char*		DECLSPEC FP_GetPluginName( void )			{ return "Ftp.dll"; }
const wchar_t*	DECLSPEC FP_GetPluginNameW( void )			{ return L"Ftp.dll"; }

CONSTSTR DECLSPEC FP_GetPluginLogName( void )      { return "farftp.log"; }
BOOL     DECLSPEC FP_PluginStartup( DWORD Reason ) { return TRUE; }


FTPPluginManager g_manager;


//FTP          *FTPPanels[3] = { 0 };
FTP          *LastUsedPlugin = NULL;
BOOL          SocketStartup  = FALSE;
int           SocketInitializeError = 0;
AbortProc     ExitProc;
char          DialogEditBuffer[ DIALOG_EDIT_SIZE ];

boost::array<std::string, 1+FTP_MAXBACKUPS>	DiskStrings;
boost::array<const char*, 1+FTP_MAXBACKUPS> DiskMenuStrings;
int           DiskMenuNumbers[ 1+FTP_MAXBACKUPS ];

FTP *DECLSPEC OtherPlugin( FTP *p )
{
	if ( !p )
		return p;

	size_t n = g_manager.getPlugin(p);
	return g_manager.getPlugin(1-n);
}

size_t DECLSPEC PluginPanelNumber( FTP *p )
{
	return g_manager.getPlugin(p)+1;
}

size_t DECLSPEC PluginUsed()
{
	return PluginPanelNumber( LastUsedPlugin );
}

//------------------------------------------------------------------------
void RTL_CALLBACK CloseUp( void )
  {  int n;

     if ( FTP::BackupCount ) {
       Log(( "CloseUp.FreeBackups" ));
       for( n = 0; n < FTP::BackupCount; n++ )
         delete FTP::Backups[n];
       FTP::BackupCount = NULL;
     }

     if ( SocketStartup ) {
       Log(( "CloseUp.CloseWSA" ));
       WSACleanup();
     }

     if ( ExitProc )
       ExitProc();
}

void FTPPluginManager::addWait(time_t tm)
{
	for(size_t n = 0; n < FTPPanels_.size(); n++)
		if(FTPPanels_[n] && FTPPanels_[n]->hConnect &&
			FTPPanels_[n]->hConnect->IOCallback)
			FTPPanels_[n]->hConnect->TrafficInfo->Waiting(tm);
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

	CONSTSTR rejectReason;
	if(ftp->isBackup())
	{
		ftp->SetBackupMode();
		return;
	}

	CONSTSTR itms[] =
	{
		FMSG(MRejectTitle),
		FMSG(MRejectCanNot),
		NULL,
		FMSG(MRejectAsk1),
		FMSG(MRejectAsk2),
		FMSG(MRejectIgnore), FMSG(MRejectSite)
	};

	do{
		if ( (rejectReason = ftp->CloseQuery()) == NULL ) break;

		itms[2] = rejectReason;
		if(FMessage( FMSG_LEFTALIGN|FMSG_WARNING,"CloseQueryReject",itms,ARRAY_SIZE(itms),2) != 1)
			break;

		ftp->AddToBackup();
		if(!ftp->isBackup())
		{
			SayMsg(FMSG(MRejectFail));
			break;
		}

		return;
	}while(0);
	
	delete ftp;
}

//------------------------------------------------------------------------
FAR_EXTERN void FAR_DECLSPEC ExitFAR( void )
  {
     //FP_Info = NULL; FP_GetMsg( "temp" );
     CallAtExit();
}

FAR_EXTERN void FAR_DECLSPEC SetStartupInfo(const struct PluginStartupInfo *Info)
{
	LastUsedPlugin = NULL;
	FP_SetStartupInfo(Info, L"FTP_debug");

	ExitProc = AtExit( CloseUp );

	memset(DiskMenuNumbers, 0, sizeof(DiskMenuNumbers) );
	g_manager.openRegKey(g_PluginRootKey);
	g_manager.readCfg();
	LogCmd( "FTP plugin loaded", ldInt);
}

FAR_EXTERN void FAR_DECLSPEC GetPluginInfo(struct PluginInfo *Info)
{  
	LastUsedPlugin = NULL;
	PROC(("GetPluginInfo","%p",Info))

	static CONSTSTR PluginMenuStrings[1];
	static CONSTSTR PluginCfgStrings[1];
	static char     MenuString[ FAR_MAX_PATHSIZE ];
	static char     CfgString[ FAR_MAX_PATHSIZE ];

	SNprintf( MenuString,     sizeof(MenuString),     "%s", FP_GetMsg(MFtpMenu) );
	DiskStrings[0] = FP_GetMsg(MFtpDiskMenu);
	SNprintf( CfgString,      sizeof(CfgString),      "%s", FP_GetMsg(MFtpMenu) );

	FTPHost* p;
	int      n;
	size_t	uLen = 0,
			hLen = 0;
	std::string	str;
	FTP*     ftp;

	for( n = 0; n < FTP::BackupCount; n++ ) 
	{
		ftp = FTP::Backups[n];
		if ( !ftp->FTPMode() )
			continue;
		p = &ftp->Host;
		uLen = std::max(uLen, p->username_.size());
		hLen = std::max(hLen, p->hostname_.size());
	}

	for( n = 0; n < FTP::BackupCount; n++ ) {
		ftp = FTP::Backups[n];
		str = Unicode::toOem(ftp->GetCurPath());
		std::string disk = "FTP: ";
		if ( ftp->FTPMode() )
		{
			p = &ftp->Host;
			disk += Unicode::toOem(p->username_) + std::string(uLen+1-p->username_.size(), ' ') +
					Unicode::toOem(p->hostname_) + std::string(hLen+1-p->hostname_.size(), ' '); // FTP: %-*s %-*s %s"
		}
		DiskStrings[1+n] = disk + str;
	}

	DiskMenuNumbers[0]   = g_manager.opt.DisksMenuDigit;
	PluginMenuStrings[0] = MenuString;
	PluginCfgStrings[0]  = CfgString;

	Info->StructSize                = sizeof(*Info);
	Info->Flags                     = 0;


	//

	
	for(size_t i = 0; i < DiskMenuStrings.size(); ++i)
	{
		DiskMenuStrings[i] = DiskStrings[i].c_str();
	}

	
	Info->DiskMenuStrings           = &DiskMenuStrings[0];
	


	Info->DiskMenuNumbers           = DiskMenuNumbers;
	Info->DiskMenuStringsNumber     = g_manager.opt.AddToDisksMenu ? (1+FTP::BackupCount) : 0;

	Info->PluginMenuStrings         = PluginMenuStrings;
	Info->PluginMenuStringsNumber   = g_manager.opt.AddToPluginsMenu ? (sizeof(PluginMenuStrings)/sizeof(PluginMenuStrings[0])):0;

	Info->PluginConfigStrings       = PluginCfgStrings;
	Info->PluginConfigStringsNumber = sizeof(PluginCfgStrings)/sizeof(PluginCfgStrings[0]);

	Info->CommandPrefix             = Unicode::toOem(FTP_CMDPREFIX).c_str();
}

FAR_EXTERN int FAR_DECLSPEC Configure(int ItemNumber)
  {  LastUsedPlugin = NULL;
     PROC(("Configure","%d",ItemNumber))

     switch(ItemNumber) {
       case 0: if ( !Config() )
                 return FALSE;
     }

     //Update panels

  return TRUE;
}

FAR_EXTERN HANDLE FAR_DECLSPEC OpenPlugin(int OpenFrom,INT_PTR Item)
{  
	LastUsedPlugin = NULL;
	PROC(("OpenPlugin","%d,%d",OpenFrom,Item))
	FTP *Ftp;

	if ( Item == 0 || Item > FTP::BackupCount )
		Ftp = new FTP;
	else
	{
		Ftp = FTP::Backups[ Item-1 ];
		Ftp->SetActiveMode();
	}

	g_manager.addPlugin(Ftp);

	Ftp->Call();
	Log(( "FTP handle: %p",Ftp ));
	do{
		if (OpenFrom==OPEN_SHORTCUT)
		{
			if (!Ftp->ProcessShortcutLine((char *)Item))
				break;
			Ftp->End();
		} else
			if (OpenFrom==OPEN_COMMANDLINE)
			{
				if (!Ftp->ProcessCommandLine((char *)Item))
					break;
			}

			Ftp->End();
			return (HANDLE)Ftp;
	}while(0);

	g_manager.removePlugin(Ftp);

	return INVALID_HANDLE_VALUE;
}

FAR_EXTERN void FAR_DECLSPEC ClosePlugin(HANDLE hPlugin)
{
	FTP *p = (FTP*)hPlugin;

	PROC(("ClosePlugin","%p",hPlugin));
	LastUsedPlugin = p;
	g_manager.removePlugin(p);
}

FAR_EXTERN int FAR_DECLSPEC GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	FPOpMode _op(OpMode);
	FTP*     p = (FTP*)hPlugin;

	p->Call();
	int rc = p->GetFindData(pPanelItem,pItemsNumber,OpMode);
	p->End(rc);

	return rc;
}

FAR_EXTERN void FAR_DECLSPEC FreeFindData(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
  {  FTP*     p = (FTP*)hPlugin;

    p->Call();
      p->FreeFindData(PanelItem,ItemsNumber);
    p->End();
}

FAR_EXTERN void FAR_DECLSPEC GetOpenPluginInfo(HANDLE hPlugin,struct OpenPluginInfo *Info)
  {  FTP*     p = (FTP*)hPlugin;

   p->Call();
     p->GetOpenPluginInfo(Info);
   p->End();
}

FAR_EXTERN int FAR_DECLSPEC SetDirectory(HANDLE hPlugin,CONSTSTR Dir,int OpMode)
{  FPOpMode _op(OpMode);
   FTP*     p = (FTP*)hPlugin;

   p->Call();
   int rc = p->SetDirectoryFAR(Unicode::fromOem(Dir),OpMode);
   p->End(rc);

 return rc;
}

FAR_EXTERN int FAR_DECLSPEC GetFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,char *DestPath,int OpMode)
{  FPOpMode _op(OpMode);
   FTP*     p = (FTP*)hPlugin;
   if ( !p || !DestPath || !DestPath[0] )
     return FALSE;

   p->Call();
   int rc = p->GetFiles(PanelItem,ItemsNumber,Move, Unicode::fromOem(DestPath),OpMode);
   p->End(rc);
  return rc;
}

FAR_EXTERN int FAR_DECLSPEC PutFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode)
  {  FPOpMode _op(OpMode);
     FTP*     p = (FTP*)hPlugin;

   p->Call();
     int rc = p->PutFiles(PanelItem,ItemsNumber,Move,OpMode);
   p->End(rc);
  return rc;
}

FAR_EXTERN int FAR_DECLSPEC DeleteFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
  {  FPOpMode _op(OpMode);
     FTP*     p = (FTP*)hPlugin;

   p->Call();
     int rc = p->DeleteFiles(PanelItem,ItemsNumber,OpMode);
   p->End(rc);
 return rc;
}

FAR_EXTERN int FAR_DECLSPEC MakeDirectory(HANDLE hPlugin, char* name,int OpMode)
{  
	FPOpMode _op(OpMode);

	if ( !hPlugin )
		return FALSE;

	FTP*     p = (FTP*)hPlugin;

	p->Call();
	std::wstring s = Unicode::fromOem(name);
	int rc = p->MakeDirectory(s,OpMode);
	p->End(rc);

	return rc;
}

FAR_EXTERN int FAR_DECLSPEC ProcessKey(HANDLE hPlugin,int Key,unsigned int ControlState)
{  
	FTP*     p = (FTP*)hPlugin;
	p->Call();
    int rc = p->ProcessKey(Key,ControlState);
	p->End(rc);

	return rc;
}

FAR_EXTERN int FAR_DECLSPEC ProcessEvent(HANDLE hPlugin,int Event,void *Param)
  {  FTP*     p = (FTP*)hPlugin;
     LastUsedPlugin = p;

#if defined(__FILELOG__)
   static CONSTSTR evts[] = { "CHANGEVIEWMODE", "REDRAW", "IDLE", "CLOSE", "BREAK", "COMMAND" };
     PROC(( "FAR.ProcessEvent","%p,%s[%08X]",
            hPlugin,
            (Event < ARRAY_SIZE(evts)) ? evts[Event] : Message("<unk>%d",Event),Param))
#endif

   p->Call();
     int rc = p->ProcessEvent(Event,Param);
   p->End(rc);
 return rc;
}

FAR_EXTERN int FAR_DECLSPEC Compare( HANDLE hPlugin,const PluginPanelItem *i,const PluginPanelItem *i1,unsigned int Mode )
  {
     if ( Mode == SM_UNSORTED )
       return -2;

//	 BOOST_ASSERT(0 && "TODO");

	 FTP*     ftp = (FTP*)hPlugin;

     const FTPHost* p  = ftp->findhost(i->UserData),
             *p1 = ftp->findhost(i1->UserData);
     int      n;

     if ( !i || !i1 || !p || !p1 )
       return -2;

     switch( Mode ) 
	 {
       case   SM_EXT: n = !boost::algorithm::iequals(p->directory_, p1->directory_);           break;
       case SM_DESCR: n = !boost::algorithm::iequals(p->hostDescription_,p1->hostDescription_); break;
	   case SM_OWNER: n = !boost::algorithm::iequals(p->username_, p1->username_);           break;

       case SM_MTIME:
       case SM_CTIME:
       case SM_ATIME: n = (int)CompareFileTime( &p1->LastWrite, &p->LastWrite );
                   break;

             default: n = p->Host_ != p1->Host_;
                   break;
     }

    do{
     if (n) break;

     n = p->Host_ != p1->Host_;
     if (n) break;

	 n = !boost::algorithm::iequals(p->username_, p1->username_);
     if (n) break;

	 n = !boost::iequals(p->hostDescription_, p1->hostDescription_);
     if (n) break;
   }while( 0 );

   if (n)
     return (n>0)?1:(-1);
    else
     return 0;
}
