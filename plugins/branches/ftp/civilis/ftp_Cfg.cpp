#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"
#include "utils/regkey.h"
#include <boost/algorithm/string/trim.hpp>


int TrimLen( char *m )
  {  int len = 0;
     while( *m && isspace(*m) )  m++;
     while( *m && !isspace(*m) ) len++,m++;
 return len;
}


std::string getAsciiString(WinAPI::RegKey& r, const wchar_t* name, const char* defaultValue)
{
	std::wstring wval;
	std::string val;
	try
	{
		r.get(name, &wval);
		val = Unicode::utf16ToAscii(wval);
	}
	catch(WinAPI::RegKey::exception& )
	{
		val = defaultValue;
	}
	boost::algorithm::trim_left(val);
	boost::algorithm::trim_right(val);
	return val;
}

void FTPPluginManager::InvalidateAll()
{
	for(size_t i =0; i < FTPPanels_.size(); i++)
		if(FTPPanels_[i])
			FTPPanels_[i]->Invalidate();
}


void FTPPluginManager::readCfg()
{
	int  val;

	opt.AddToDisksMenu		= regkey_.get(L"AddToDisksMenu",     1);
	opt.AddToPluginsMenu	= regkey_.get(L"AddToPluginsMenu",   1);
	opt.DisksMenuDigit		= regkey_.get(L"DisksMenuDigit",     2);
	opt.ReadDescriptions	= regkey_.get(L"ReadDescriptions",   0);
	opt.UploadLowCase		= regkey_.get(L"UploadLowCase",      0);
	opt.ShowUploadDialog	= regkey_.get(L"ShowUploadDialog",   1);
	opt.ResumeDefault		= regkey_.get(L"ResumeDefault",      0);
	opt.UpdateDescriptions	= regkey_.get(L"UpdateDescriptions", 0);
	opt.PassiveMode			= regkey_.get(L"PassiveMode",        0);
	opt.DescriptionNames	= regkey_.get(L"DescriptionNames", L"00_index.txt,0index,0index.txt");
	opt.firewall			= regkey_.get(L"Firewall", L"");

	std::vector<unsigned char> pswd;
	try
	{
		regkey_.get(L"DefaultPassword", &pswd);
		opt.defaultPassword_ = DecryptPassword(pswd);
	}
	catch(WinAPI::RegKey::exception& )
	{
	}

	//JM
	opt.CmdLength			=       std::max(5, std::min(FP_ConHeight()-5,regkey_.get(L"CmdLength",7 )));
	opt.CmdLine				=       std::max(10, std::min(static_cast<int>(FP_ConWidth())-9,regkey_.get(L"CmdLine",70 )));
	opt.IOBuffSize			=       std::max(FTR_MINBUFFSIZE, regkey_.get(L"IOBuffSize",512));
	opt.dDelimit			=       regkey_.get(L"DigitDelimit",       TRUE );
	opt.dDelimiter			= (wchar_t)regkey_.get(L"DigitDelimiter",     0 );
	opt.WaitTimeout			=       regkey_.get(L"WaitTimeout",        30 );
	opt.AskAbort			=       regkey_.get(L"AskAbort",           TRUE );
	opt.WaitIdle			=       regkey_.get(L"WaitIdle",           1 );
	opt.CmdLogLimit			=       regkey_.get(L"CmdLogLimit",        100 );
	opt.ShowIdle			=       regkey_.get(L"ShowIdle",           TRUE);
	opt.TimeoutRetry		=       regkey_.get(L"TimeoutRetry",       FALSE );
	opt.RetryCount			=       regkey_.get(L"RetryCount",         0 );
	opt.LogOutput			=       regkey_.get(L"LogOutput",          FALSE );
	opt._ShowPassword		=       regkey_.get(L"ShowPassword",       FALSE );
	opt.IdleColor			=       regkey_.get(L"IdleColor",          FAR_COLOR(fccCYAN,fccBLUE) );
	opt.IdleMode			=       regkey_.get(L"IdleMode",           IDLE_CONSOLE );
	opt.LongBeepTimeout		=       regkey_.get(L"BeepTimeout",        30 );
	opt.KeepAlive			=       regkey_.get(L"KeepAlive",          60 );
	opt.IdleShowPeriod		=       regkey_.get(L"IdleShowPeriod",     700 );
	opt.IdleStartPeriod		=       regkey_.get(L"IdleStartPeriod",    4000 );
	opt.AskLoginFail		=       regkey_.get(L"AskLoginFail",       TRUE );
	opt.ExtCmdView			=       regkey_.get(L"ExtCmdView",         TRUE );
	opt.AutoAnonymous		=       regkey_.get(L"AutoAnonymous",      TRUE );
	opt.CloseDots			=       regkey_.get(L"CloseDots",          TRUE );
	opt.QuoteClipboardNames	=       regkey_.get(L"QuoteClipboardNames",TRUE );
	opt.SetHiddenOnAbort	=       regkey_.get(L"SetHiddenOnAbort",   FALSE );
	opt.PwdSecurity			=       regkey_.get(L"PwdSecurity",        0 );
	opt.WaitCounter			=       regkey_.get(L"WaitCounter",        0 );
	opt.RetryTimeout		=       regkey_.get(L"RetryTimeout",       10 );
	opt.DoNotExpandErrors	=       regkey_.get(L"DoNotExpandErrors",  FALSE );
	opt.TruncateLogFile		=       regkey_.get(L"TruncateLogFile",    FALSE );
	opt.ServerType			=       regkey_.get(L"ServerType",         FTP_TYPE_DETECT );
	opt.UseBackups			=       regkey_.get(L"UseBackups",         TRUE );
	opt.ProcessCmd			=       regkey_.get(L"ProcessCmd",         TRUE );
	opt.FFDup				=       regkey_.get(L"FFDup",              FALSE );
	opt.UndupFF				=       regkey_.get(L"UndupFF",            FALSE );
	opt.ShowSilentProgress	=       regkey_.get(L"ShowSilentProgress", FALSE );
	opt.InvalidSymbols		=		regkey_.get(L"InvalidSymbols",     L"<>|?*\"");
	opt.CorrectedSymbols	=		regkey_.get(L"CorrectedSymbols",   L"()!__\'");
	size_t n = std::min(opt.InvalidSymbols.size(), opt.CorrectedSymbols.size());
	opt.InvalidSymbols.resize(n);
	opt.CorrectedSymbols.resize(n);

	opt.PluginColumnMode	=		regkey_.get(L"PluginColumnMode", -1);
	if(opt.PluginColumnMode < 0 || opt.PluginColumnMode >= 10)
		opt.PluginColumnMode = -1;

	opt.CmdLogFile			= regkey_.get(L"CmdLogFile", L"");

	//Queue
	opt.RestoreState		= regkey_.get(L"QueueRestoreState",    TRUE);
	opt.RemoveCompleted		= regkey_.get(L"QueueRemoveCompleted", TRUE );
	opt.sli.AddPrefix		= regkey_.get(L"AddPrefix",          TRUE );
	opt.sli.AddPasswordAndUser = regkey_.get(L"AddPasswordAndUser", TRUE );
	opt.sli.Quote			= regkey_.get(L"Quote",              TRUE );
	opt.sli.Size			= regkey_.get(L"Size",               TRUE );
	opt.sli.RightBound		= regkey_.get(L"RightBound",         80 );
	opt.sli.ListType		= (sliTypes)regkey_.get(L"ListType",           sltUrlList );

	//Formats
	opt.fmtDateFormat		= getAsciiString(regkey_, L"ServerDateFormat", "%*s %04d%02d%02d%02d%02d%02d");

	//CMD`s
	opt.cmdPut		= getAsciiString(regkey_, L"xcmdPUT", "STOR");
	opt.cmdAppe		= getAsciiString(regkey_, L"xcmdAPPE", "APPE");
	opt.cmdStor		= getAsciiString(regkey_, L"xcmdSTOR", "STOR");
	opt.cmdPutUniq	= getAsciiString(regkey_, L"xcmdSTOU", "STOU");
	opt.cmdPasv_	= getAsciiString(regkey_, L"xcmdPASV", "PASV");
	opt.cmdPort		= getAsciiString(regkey_, L"xcmdPORT", "PORT");
	opt.cmdMDTM		= getAsciiString(regkey_, L"xcmdMDTM", "MDTM");
	opt.cmdRetr		= getAsciiString(regkey_, L"xcmdRETR", "RETR");
	opt.cmdRest_	= getAsciiString(regkey_, L"xcmdREST", "REST");
	opt.cmdAllo		= getAsciiString(regkey_, L"xcmdALLO", "ALLO");
	opt.cmdCwd		= getAsciiString(regkey_, L"xcmdCWD", "CWD");
	opt.cmdXCwd		= getAsciiString(regkey_, L"xcmdXCWD", "XCWD");
	opt.cmdDel		= getAsciiString(regkey_, L"xcmdDELE", "DELE");
	opt.cmdRen		= getAsciiString(regkey_, L"xcmdRNFR", "RNFR");
	opt.cmdRenTo	= getAsciiString(regkey_, L"xcmdRNTO", "RNTO");
	opt.cmdList		= getAsciiString(regkey_, L"xcmdLIST", "LIST");
	opt.cmdNList	= getAsciiString(regkey_, L"xcmdNLIST", "NLIST");
	opt.cmdUser_	= getAsciiString(regkey_, L"xcmdUSER", "USER");
	opt.cmdPass_	= getAsciiString(regkey_, L"xcmdPASS", "PASS");
	opt.cmdAcct_	= getAsciiString(regkey_, L"xcmdACCT", "ACCT");
	opt.cmdPwd		= getAsciiString(regkey_, L"xcmdPWD", "PWD");
	opt.cmdXPwd		= getAsciiString(regkey_, L"xcmdXPWD", "XPWD");
	opt.cmdMkd		= getAsciiString(regkey_, L"xcmdMKD", "MKD");
	opt.cmdXMkd		= getAsciiString(regkey_, L"xcmdXMKD", "XMKD");
	opt.cmdRmd		= getAsciiString(regkey_, L"xcmdRMD", "RMD");
	opt.cmdXRmd		= getAsciiString(regkey_, L"xcmdXRMD", "XRMD");
	opt.cmdSite		= getAsciiString(regkey_, L"xcmdSITE", "SITE");
	opt.cmdChmod	= getAsciiString(regkey_, L"xcmdCHMOD", "CHMOD");
	opt.cmdUmask	= getAsciiString(regkey_, L"xcmdUMASK", "UMASK");
	opt.cmdIdle		= getAsciiString(regkey_, L"xcmdIDLE", "IDLE");
	opt.cmdHelp		= getAsciiString(regkey_, L"xcmdHELP", "HELP");
	opt.cmdQuit		= getAsciiString(regkey_, L"xcmdQUIT", "QUIT");
	opt.cmdCDUp		= getAsciiString(regkey_, L"xcmdCDUP", "CDUP");
	opt.cmdXCDUp	= getAsciiString(regkey_, L"xcmdXCUP", "XCUP");
	opt.cmdSyst		= getAsciiString(regkey_, L"xcmdSYST", "SYST");
	opt.cmdSize		= getAsciiString(regkey_, L"xcmdSIZE", "SIZE");
	opt.cmdStat		= getAsciiString(regkey_, L"xcmdSTAT", "STAT");

	//ProcessColor
	val = (int)FP_Info->AdvControl( FP_Info->ModuleNumber,ACTL_GETCOLOR,(void*)COL_DIALOGBOX );
	opt.ProcessColor = regkey_.get(L"ProcessColor", val);

	//dDelimit && dDelimiter
	if(opt.dDelimit && opt.dDelimiter == 0)
	{
		wchar_t buffer[8];
		if(GetLocaleInfoW(GetThreadLocale(), LOCALE_STHOUSAND, buffer, sizeof(buffer)))
		{
			opt.dDelimiter = buffer[0];
		} else
			opt.dDelimiter = L'.';
	}
}

//------------------------------------------------------------------------
void DECLSPEC WriteCfg( void )
{
//Dialog
/* TODO
  FP_SetRegKey("AddToDisksMenu",       g_manager.opt.AddToDisksMenu );
  FP_SetRegKey("DisksMenuDigit",       g_manager.opt.DisksMenuDigit );
  FP_SetRegKey("AddToPluginsMenu",     g_manager.opt.AddToPluginsMenu );
  FP_SetRegKey("PluginColumnMode",     g_manager.opt.PluginColumnMode );
  FP_SetRegKey("ReadDescriptions",     g_manager.opt.ReadDescriptions );
  FP_SetRegKey("UpdateDescriptions",   g_manager.opt.UpdateDescriptions );
  FP_SetRegKey("UploadLowCase",        g_manager.opt.UploadLowCase );
  FP_SetRegKey("ShowUploadDialog",     g_manager.opt.ShowUploadDialog );
  FP_SetRegKey("ResumeDefault",        g_manager.opt.ResumeDefault );
  FP_SetRegKey("WaitTimeout",          g_manager.opt.WaitTimeout );
  FP_SetRegKey("ShowIdle",             g_manager.opt.ShowIdle );
  FP_SetRegKey("IdleColor",            g_manager.opt.IdleColor );
  FP_SetRegKey("IdleMode",             g_manager.opt.IdleMode );

  FP_SetRegKey("KeepAlive",            g_manager.opt.KeepAlive );
  FP_SetRegKey("IdleShowPeriod",       g_manager.opt.IdleShowPeriod );
  FP_SetRegKey("IdleStartPeriod",      g_manager.opt.IdleStartPeriod );
  FP_SetRegKey("TimeoutRetry",         g_manager.opt.TimeoutRetry );
  FP_SetRegKey("RetryCount",           g_manager.opt.RetryCount );
  FP_SetRegKey("BeepTimeout",          g_manager.opt.LongBeepTimeout  );
  FP_SetRegKey("AskAbort",             g_manager.opt.AskAbort );
  FP_SetRegKey("WaitIdle",             g_manager.opt.WaitIdle );
  FP_SetRegKey("DigitDelimit",         g_manager.opt.dDelimit );
  FP_SetRegKey("DigitDelimiter",       (int)g_manager.opt.dDelimiter );
  FP_SetRegKey("ExtCmdView",           g_manager.opt.ExtCmdView );
  FP_SetRegKey("CmdLine",              g_manager.opt.CmdLine );
  FP_SetRegKey("CmdLength",            g_manager.opt.CmdLength );
  FP_SetRegKey("IOBuffSize",           g_manager.opt.IOBuffSize );
  FP_SetRegKey("ProcessColor",         g_manager.opt.ProcessColor );
  FP_SetRegKey("ServerType",           g_manager.opt.ServerType );
  FP_SetRegKey("ProcessCmd",           g_manager.opt.ProcessCmd );

  FP_SetRegKey("QueueRestoreState",    g_manager.opt.RestoreState );
  FP_SetRegKey("QueueRemoveCompleted", g_manager.opt.RemoveCompleted );

  FP_SetRegKey(L"DescriptionNames",		g_manager.opt.DescriptionNames );

  std::vector<unsigned char> CryptedPassword = MakeCryptPassword(g_manager.opt.defaultPassword_);
// TODO  FP_SetRegKey( "DefaultPassword", CryptedPassword,sizeof(CryptedPassword)-1 );

  FP_SetRegKey( "Firewall",            g_manager.opt.Firewall );

  FP_SetRegKey( "CmdLogLimit",         g_manager.opt.CmdLogLimit );
  FP_SetRegKey( "CmdLogFile",          g_manager.opt.CmdLogFile );
  FP_SetRegKey( "LogOutput",           g_manager.opt.LogOutput );
  FP_SetRegKey( "PassiveMode",         g_manager.opt.PassiveMode );

  FP_SetRegKey("AutoAnonymous",        g_manager.opt.AutoAnonymous      );
  FP_SetRegKey("CloseDots",            g_manager.opt.CloseDots          );
  FP_SetRegKey("QuoteClipboardNames",  g_manager.opt.QuoteClipboardNames);
  FP_SetRegKey("SetHiddenOnAbort",     g_manager.opt.SetHiddenOnAbort );
  FP_SetRegKey("PwdSecurity",          g_manager.opt.PwdSecurity        );
  FP_SetRegKey("WaitCounter",          g_manager.opt.WaitCounter        );
  FP_SetRegKey("RetryTimeout",         g_manager.opt.RetryTimeout       );
  FP_SetRegKey("DoNotExpandErrors",    g_manager.opt.DoNotExpandErrors  );
  FP_SetRegKey("TruncateLogFile",      g_manager.opt.TruncateLogFile    );
  FP_SetRegKey("ServerType",           g_manager.opt.ServerType         );
  FP_SetRegKey("UseBackups",           g_manager.opt.UseBackups         );
  FP_SetRegKey("ProcessCmd",           g_manager.opt.ProcessCmd         );
  FP_SetRegKey("FFDup",                g_manager.opt.FFDup              );
  FP_SetRegKey("UndupFF",              g_manager.opt.UndupFF );
  FP_SetRegKey("ShowSilentProgress",   g_manager.opt.ShowSilentProgress );
  FP_SetRegKey("AskLoginFail",         g_manager.opt.AskLoginFail       );
  FP_SetRegKey( "InvalidSymbols",      g_manager.opt.InvalidSymbols     );
  FP_SetRegKey( "CorrectedSymbols",    g_manager.opt.CorrectedSymbols   );

//Queue
  FP_SetRegKey("QueueRestoreState",    g_manager.opt.RestoreState       );
  FP_SetRegKey("QueueRemoveCompleted", g_manager.opt.RemoveCompleted    );
  FP_SetRegKey("AddPrefix",            g_manager.opt.sli.AddPrefix          );
  FP_SetRegKey("AddPasswordAndUser",   g_manager.opt.sli.AddPasswordAndUser );
  FP_SetRegKey("Quote",                g_manager.opt.sli.Quote              );
  FP_SetRegKey("Size",                 g_manager.opt.sli.Size               );
  FP_SetRegKey("RightBound",           g_manager.opt.sli.RightBound         );
  FP_SetRegKey("ListType",             g_manager.opt.sli.ListType           );
*/
}
//------------------------------------------------------------------------
void ExtendedConfig( void )
{
static FP_DECL_DIALOG( InitItems )
   /*00*/    FDI_CONTROL( DI_DOUBLEBOX, 3, 1,72,17, 0, FMSG(METitle) )

   /*01*/      FDI_CHECK( 5, 2,    FMSG(MDupFF) )
   /*02*/      FDI_CHECK( 5, 3,    FMSG(MUndupFF) )
   /*03*/      FDI_CHECK( 5, 4,    FMSG(MEShowProgress) )
   /*04*/      FDI_CHECK( 5, 5,    FMSG(MEBackup) )
   /*05*/      FDI_CHECK( 5, 6,    FMSG(MESendCmd) )
   /*06*/      FDI_CHECK( 5, 7,    FMSG(MEDontError) )
   /*07*/      FDI_CHECK( 5, 8,    FMSG(MEAskLoginAtFail) )
   /*08*/      FDI_CHECK( 5, 9,    FMSG(MEAutoAn) )
   /*09*/      FDI_CHECK( 5,10,    FMSG(MECloseDots) )
   /*10*/      FDI_CHECK( 5,11,    FMSG(MQuoteClipboardNames) )
   /*11*/      FDI_CHECK( 5,12,    FMSG(MSetHiddenOnAbort) )

   /*12*/      FDI_HLINE( 3,15 )

   /*13*/ FDI_GDEFBUTTON( 0,16,    FMSG(MOk) )
   /*14*/    FDI_GBUTTON( 0,16,    FMSG(MCancel) )
FP_END_DIALOG

enum {
  dDupFF        = 1,
  dUndupFF      = 2,
  dShowProgress = 3,
  dBackup       = 4,
  dSendCmd      = 5,
  dDontErr      = 6,
  dAskFail      = 7,
  dAutoAnn      = 8,
  dCloseDots    = 9,
  dQuoteCN      = 10,
  dSetHiddenOnAbort = 11,

  dOk           = 13
};

  int           rc;
  FarDialogItem DialogItems[ FP_DIALOG_SIZE(InitItems) ];

  FP_InitDialogItems( InitItems,DialogItems );

  DialogItems[dDupFF].Selected        = g_manager.opt.FFDup;
  DialogItems[dUndupFF].Selected      = g_manager.opt.UndupFF;
  DialogItems[dShowProgress].Selected = g_manager.opt.ShowSilentProgress;
  DialogItems[dBackup].Selected       = g_manager.opt.UseBackups;
  DialogItems[dSendCmd].Selected      = g_manager.opt.ProcessCmd;
  DialogItems[dDontErr].Selected      = g_manager.opt.DoNotExpandErrors;
  DialogItems[dAskFail].Selected      = g_manager.opt.AskLoginFail;
  DialogItems[dAutoAnn].Selected      = g_manager.opt.AutoAnonymous;
  DialogItems[dCloseDots].Selected    = g_manager.opt.CloseDots;
  DialogItems[dQuoteCN].Selected      = g_manager.opt.QuoteClipboardNames;
  DialogItems[dSetHiddenOnAbort].Selected  = g_manager.opt.SetHiddenOnAbort;

  do{
    rc = FDialog( 76,19,"FTPExtGlobal",DialogItems,FP_DIALOG_SIZE(InitItems) );
    if ( rc == -1 )
      return;

    if ( rc != dOk )
      return;

    break;
  }while( 1 );

  g_manager.opt.FFDup              = DialogItems[dDupFF].Selected;
  g_manager.opt.UndupFF            = DialogItems[dUndupFF].Selected;
  g_manager.opt.ShowSilentProgress = DialogItems[dShowProgress].Selected;
  g_manager.opt.UseBackups         = DialogItems[dBackup].Selected;
  g_manager.opt.ProcessCmd         = DialogItems[dSendCmd].Selected;
  g_manager.opt.DoNotExpandErrors  = DialogItems[dDontErr].Selected;
  g_manager.opt.AskLoginFail       = DialogItems[dAskFail].Selected;
  g_manager.opt.AutoAnonymous      = DialogItems[dAutoAnn].Selected;
  g_manager.opt.CloseDots          = DialogItems[dCloseDots].Selected;
  g_manager.opt.QuoteClipboardNames= DialogItems[dQuoteCN].Selected;
  g_manager.opt.SetHiddenOnAbort   = DialogItems[dSetHiddenOnAbort].Selected;
}

//------------------------------------------------------------------------
static FLngColorDialog ColorLangs = {
 /*MTitle*/    FMSG(MColorTitle),
 /*MFore*/     FMSG(MColorFore),
 /*MBk*/       FMSG(MColorBk),
 /*MSet*/      FMSG(MColorColorSet),
 /*MCancel*/   FMSG(MCancel),
 /*MText*/     FMSG(MColorText)
};

template<size_t size>
void intToStr(char (&str)[size], int n)
{
	snprintf(str, size, "%d", n);
}

int DECLSPEC Config( void )
{
static FP_DECL_DIALOG( InitItems )
   /*00*/   FDI_CONTROL( DI_DOUBLEBOX, 3, 1,72,21, 0, FMSG(MConfigTitle) )

   /*01*/     FDI_CHECK(  5, 2,   FMSG(MConfigAddToDisksMenu) )   //Add to Disks menu
   /*02*/   FDI_FIXEDIT( 35, 2,37 )
   /*03*/     FDI_CHECK(  5, 3,   FMSG(MConfigAddToPluginsMenu) ) //Add to Plugins menu
   /*04*/     FDI_LABEL(  9, 4,   FMSG(MHostsMode) )              //Hosts panel mode
   /*05*/   FDI_FIXEDIT(  5, 4, 7 )
   /*06*/     FDI_CHECK(  5, 5,   FMSG(MConfigReadDiz) )          //Read descriptions
   /*07*/     FDI_CHECK(  5, 6,   FMSG(MConfigUpdateDiz) )        //Update descriptions
   /*08*/     FDI_CHECK(  5, 7,   FMSG(MConfigUploadLowCase) )    //Upload upper in lowercase
   /*09*/     FDI_CHECK(  5, 8,   FMSG(MConfigUploadDialog) )     //Show upload options dialog
   /*10*/     FDI_CHECK(  5, 9,   FMSG(MConfigDefaultResume) )    //Default button is 'Resume'
   /*11*/     FDI_CHECK(  5,10,   FMSG(MAskAbort) )               //Confirm abort
   /*12*/     FDI_CHECK(  5,11,   FMSG(MShowIdle) )               //Show idle
   /*13*/    FDI_BUTTON( 21,11,   FMSG(MColor) )                  //Color
   /*14*/FDI_STARTRADIO( 32,11,   FMSG(MScreen) )                 //( ) 1.Screen
   /*15*/     FDI_RADIO( 44,11,   FMSG(MCaption) )                //( ) 2.Caption
   /*16*/     FDI_RADIO( 58,11,   FMSG(MBoth) )                   //( ) 3.Both

   /*17*/     FDI_CHECK( 40, 2,   FMSG(MKeepAlive) )              //Keepalive packet
   /*18*/      FDI_EDIT( 67, 2,70 )
   /*19*/     FDI_LABEL( 71, 2,   FMSG(MSec) )                    //s
   /*20*/     FDI_CHECK( 40, 3,   FMSG(MAutoRetry) )              //AutoRetry
   /*21*/      FDI_EDIT( 67, 3,70 )
   /*22*/     FDI_CHECK( 40, 4,   FMSG(MLongOp) )                 //Long operation beep
   /*23*/      FDI_EDIT( 67, 4,70 )
   /*24*/     FDI_LABEL( 71, 4,   FMSG(MSec) )                    //s
   /*25*/     FDI_LABEL( 40, 5,   FMSG(MWaitTimeout) )            //Server reply timeout (s)
   /*26*/      FDI_EDIT( 67, 5,70 )
   /*27*/     FDI_CHECK( 40, 6,   FMSG(MDigitDelimit) )           //Digits grouping symbol
   /*28*/      FDI_EDIT( 69, 6,70 )
   /*29*/     FDI_CHECK( 40, 7,   FMSG(MExtWindow) )              //Show FTP command log
   /*30*/     FDI_LABEL( 40, 8,   FMSG(MExtSize) )                //Log window size
   /*31*/      FDI_EDIT( 60, 8,63 )
   /*32*/     FDI_LABEL( 64, 8,   " x " )
   /*33*/      FDI_EDIT( 67, 8,70 )
   /*34*/     FDI_LABEL( 40, 9,   FMSG(MHostIOSize) )             //I/O buffer size
   /*35*/      FDI_EDIT( 60, 9,70 )
   /*36*/     FDI_LABEL( 40,10,   FMSG(MSilentText) )             //Alert text
   /*37*/    FDI_BUTTON( 60,10,   FMSG(MColor) )

   /*38*/     FDI_HLINE(  5,12 )

   /*39*/     FDI_LABEL(  5,13,   FMSG(MConfigDizNames) )         //Dis names
   /*40*/      FDI_EDIT(  5,14,70 )
   /*41*/     FDI_LABEL(  5,15,   FMSG(MConfigDefPassword) )      //Def pass
   /*42*/   FDI_PSWEDIT(  5,16,34 )
   /*43*/     FDI_LABEL(  5,17,   FMSG(MConfigFirewall) )         //Firewall
   /*44*/      FDI_EDIT(  5,18,34 )
   /*45*/     FDI_LABEL( 40,15,   FMSG(MLogFilename) )            //Log filename
   /*46*/      FDI_EDIT( 66,15,70 )
   /*47*/     FDI_LABEL( 71,15,   FMSG(MKBytes) )
   /*48*/      FDI_EDIT( 40,16,70 )
   /*49*/     FDI_CHECK( 40,17,   FMSG(MLogDir) )                 //Log DIR contents
   /*50*/     FDI_CHECK( 40,18,   FMSG(MConfigPassiveMode) )

   /*51*/     FDI_HLINE(  5,19 )

   /*52*/FDI_GDEFBUTTON(  0,20,   FMSG(MOk) )
   /*53*/   FDI_GBUTTON(  0,20,   FMSG(MCancel) )
   /*54*/   FDI_GBUTTON(  0,20,   FMSG(MExtOpt) )
FP_END_DIALOG

#define CFG_ADDDISK       1
#define CFG_DIGIT         2
#define CFG_ADDPLUGINS    3
#define CFG_HOSTMODE      5
#define CFG_READDIZ       6
#define CFG_UPDDIZ        7
#define CFG_UPCASE        8
#define CFG_SHOWUP        9
#define CFG_RESDEF        10
#define CFG_ASKABORT      11
#define CFG_SHOWIDLE      12
#define CFG_IDLECOLOR     13
#define CFG_IDLE_SCREEN   14
#define CFG_IDLE_CAPTION  15
#define CFG_IDLE_BOTH     16

#define CFG_KEEPALIVE     17
#define CFG_KEEPTIME      18
#define CFG_AUTOR         20
#define CFG_AUTORTIME     21
#define CFG_LONGOP        22
#define CFG_LONGOPTIME    23
#define CFG_WAITTIMEOUT   26
#define CFG_DIGDEL        27
#define CFG_DIGCHAR       28
#define CFG_EXT           29
#define CFG_EXT_W         31
#define CFG_EXT_H         33
#define CFG_BUFFSIZE      35
#define CFG_SILENT        37

#define CFG_DESC          40
#define CFG_PASS          42
#define CFG_FIRE          44
#define CFG_LOGLIMIT      46
#define CFG_LOGFILE       48
#define CFG_LOGDIR        49
#define CFG_PASV          50

#define CFG_OK            52
#define CFG_CANCEL        53
#define CFG_EXTBTN        54

     FarDialogItem DialogItems[ FP_DIALOG_SIZE(InitItems) ];
     int           IdleColor    = g_manager.opt.IdleColor,
                   ProcessColor = g_manager.opt.ProcessColor;
     int           rc;

	 FP_InitDialogItems( InitItems,DialogItems );

	 DialogItems[CFG_ADDDISK].Selected		= g_manager.opt.AddToDisksMenu;
	 intToStr(DialogItems[CFG_DIGIT].Data, g_manager.opt.DisksMenuDigit);
	 DialogItems[CFG_ADDPLUGINS].Selected	= g_manager.opt.AddToPluginsMenu;
	 intToStr(DialogItems[CFG_HOSTMODE].Data, g_manager.opt.PluginColumnMode);
	 DialogItems[CFG_READDIZ].Selected		= g_manager.opt.ReadDescriptions;
	 DialogItems[CFG_UPDDIZ].Selected		= g_manager.opt.UpdateDescriptions;
	 DialogItems[CFG_UPCASE].Selected		= g_manager.opt.UploadLowCase;
	 DialogItems[CFG_SHOWUP].Selected		= g_manager.opt.ShowUploadDialog;
	 DialogItems[CFG_RESDEF].Selected		= g_manager.opt.ResumeDefault;
	 intToStr(DialogItems[CFG_WAITTIMEOUT].Data, g_manager.opt.WaitTimeout);
	 DialogItems[CFG_SHOWIDLE].Selected		= g_manager.opt.ShowIdle;
	 DialogItems[CFG_IDLE_SCREEN].Selected	= g_manager.opt.IdleMode == IDLE_CONSOLE;
	 DialogItems[CFG_IDLE_CAPTION].Selected	= g_manager.opt.IdleMode == IDLE_CAPTION;
	 DialogItems[CFG_IDLE_BOTH].Selected	= g_manager.opt.IdleMode == (IDLE_CONSOLE|IDLE_CAPTION);

	 DialogItems[CFG_KEEPALIVE].Selected	= g_manager.opt.KeepAlive != 0;
	 intToStr(DialogItems[CFG_KEEPTIME].Data, g_manager.opt.KeepAlive);
	 DialogItems[CFG_AUTOR].Selected		= g_manager.opt.TimeoutRetry;
	 intToStr(DialogItems[CFG_AUTORTIME].Data, g_manager.opt.RetryCount);
	 DialogItems[CFG_LONGOP].Selected		= g_manager.opt.LongBeepTimeout != 0;
	 intToStr(DialogItems[CFG_LONGOPTIME].Data, g_manager.opt.LongBeepTimeout );
	 DialogItems[CFG_ASKABORT].Selected		= g_manager.opt.AskAbort;
	 DialogItems[CFG_DIGDEL].Selected		= g_manager.opt.dDelimit;
	 sprintf(DialogItems[CFG_DIGCHAR].Data, "%c", g_manager.opt.dDelimiter );
	 DialogItems[CFG_EXT].Selected			= g_manager.opt.ExtCmdView;
	 intToStr(DialogItems[CFG_EXT_W].Data, g_manager.opt.CmdLine );
	 intToStr(DialogItems[CFG_EXT_H].Data, g_manager.opt.CmdLength );
	 Size2Str(DialogItems[CFG_BUFFSIZE].Data, g_manager.opt.IOBuffSize);
	 Utils::safe_strcpy(DialogItems[CFG_DESC].Data, Unicode::toOem(g_manager.opt.DescriptionNames));
	 Utils::safe_strcpy(DialogItems[CFG_PASS].Data, Unicode::toOem(g_manager.opt.defaultPassword_));
	 Utils::safe_strcpy(DialogItems[CFG_FIRE].Data, Unicode::toOem(g_manager.opt.firewall));
	 intToStr(DialogItems[CFG_LOGLIMIT].Data, g_manager.opt.CmdLogLimit);
	 Utils::safe_strcpy(DialogItems[CFG_LOGFILE].Data, Unicode::toOem(g_manager.opt.CmdLogFile));
	 DialogItems[CFG_LOGDIR].Selected      = g_manager.opt.LogOutput;
	 DialogItems[CFG_PASV].Selected        = g_manager.opt.PassiveMode;

  do{
    rc = FDialog( 76,23,"Config",DialogItems,FP_DIALOG_SIZE(InitItems) );
    if ( rc == CFG_OK )
      break;

    if ( rc == -1 || rc == CFG_CANCEL )
      return FALSE;

    if ( rc == CFG_IDLECOLOR )
      IdleColor = FP_GetColorDialog( IdleColor,&ColorLangs,NULL );
     else
    if ( rc == CFG_SILENT )
      ProcessColor = FP_GetColorDialog( ProcessColor,&ColorLangs,NULL );
     else
    if ( rc == CFG_EXTBTN )
      ExtendedConfig();

  }while(1);

//Set to OPT
  g_manager.opt.IdleColor          = IdleColor;
  g_manager.opt.ProcessColor       = ProcessColor;
  g_manager.opt.AddToDisksMenu     = DialogItems[CFG_ADDDISK].Selected;
  g_manager.opt.DisksMenuDigit     = atoi(DialogItems[CFG_DIGIT].Data);
  g_manager.opt.AddToPluginsMenu   = DialogItems[CFG_ADDPLUGINS].Selected;
  g_manager.opt.PluginColumnMode   = atoi( DialogItems[CFG_HOSTMODE].Data );
  g_manager.opt.ReadDescriptions   = DialogItems[CFG_READDIZ].Selected;
  g_manager.opt.UpdateDescriptions = DialogItems[CFG_UPDDIZ].Selected;
  g_manager.opt.UploadLowCase      = DialogItems[CFG_UPCASE].Selected;
  g_manager.opt.ShowUploadDialog   = DialogItems[CFG_SHOWUP].Selected;
  g_manager.opt.ResumeDefault      = DialogItems[CFG_RESDEF].Selected;
  g_manager.opt.WaitTimeout        = atoi( DialogItems[CFG_WAITTIMEOUT].Data );
  g_manager.opt.ShowIdle           = DialogItems[CFG_SHOWIDLE].Selected;

  if ( DialogItems[CFG_IDLE_SCREEN].Selected )  g_manager.opt.IdleMode = IDLE_CONSOLE; else
  if ( DialogItems[CFG_IDLE_CAPTION].Selected ) g_manager.opt.IdleMode = IDLE_CAPTION; else
    g_manager.opt.IdleMode = IDLE_CONSOLE | IDLE_CAPTION;

  if ( DialogItems[CFG_KEEPALIVE].Selected )
    g_manager.opt.KeepAlive = atoi( DialogItems[CFG_KEEPTIME].Data );
   else
    g_manager.opt.KeepAlive = 0;

  g_manager.opt.TimeoutRetry = DialogItems[CFG_AUTOR].Selected;
  g_manager.opt.RetryCount   = atoi( DialogItems[CFG_AUTORTIME].Data );

  if ( DialogItems[CFG_LONGOP].Selected )
    g_manager.opt.LongBeepTimeout = atoi( DialogItems[CFG_LONGOPTIME].Data );
   else
    g_manager.opt.LongBeepTimeout = 0;

  g_manager.opt.AskAbort			= DialogItems[CFG_ASKABORT].Selected;
  g_manager.opt.dDelimit			= DialogItems[CFG_DIGDEL].Selected;
  g_manager.opt.dDelimiter			= DialogItems[CFG_DIGCHAR].Data[0];
  g_manager.opt.ExtCmdView			= DialogItems[CFG_EXT].Selected;

  g_manager.opt.CmdLine				= std::max( 10,Min(static_cast<int>(FP_ConWidth())-9,atoi(DialogItems[CFG_EXT_W].Data)) );
  g_manager.opt.CmdLength			= std::max( 5,Min(FP_ConHeight()-5,atoi(DialogItems[CFG_EXT_H].Data)) );
  g_manager.opt.IOBuffSize			= std::max( (DWORD)FTR_MINBUFFSIZE,Str2Size(DialogItems[CFG_BUFFSIZE].Data) );

  g_manager.opt.DescriptionNames	= Unicode::fromOem(DialogItems[CFG_DESC].Data);
  g_manager.opt.defaultPassword_	= Unicode::fromOem(DialogItems[CFG_PASS].Data);
  g_manager.opt.firewall			= Unicode::fromOem(DialogItems[CFG_FIRE].Data);
  g_manager.opt.CmdLogLimit			= atoi(DialogItems[CFG_LOGLIMIT].Data);
  g_manager.opt.CmdLogFile			= Unicode::fromOem(DialogItems[CFG_LOGFILE].Data);
  g_manager.opt.LogOutput			= DialogItems[CFG_LOGDIR].Selected;
  g_manager.opt.PassiveMode			= DialogItems[CFG_PASV].Selected;

//Write to REG
  WriteCfg();

  g_manager.InvalidateAll();

 return TRUE;
}
