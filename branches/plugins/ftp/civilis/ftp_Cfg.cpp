#include "stdafx.h"

#include "ftp_Int.h"
#include "utils/regkey.h"
#include <boost/algorithm/string/trim.hpp>
#include "farwrapper/dialog.h"
#include "farwrapper/stddialogs.h"
#include "utils/uniconverts.h"

#include "../common/unicode/farcolor.hpp"

std::string getAsciiString(WinAPI::RegKey& r, const wchar_t* name, const char* defaultValue)
{
	std::string val = Unicode::utf16ToAscii(r.get(name, Unicode::fromOem(defaultValue)));
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

	opt.CmdCount			=       std::max(5, std::min(WinAPI::getConsoleHeight()-5,regkey_.get(L"CmdLength",7 )));
	opt.CmdLine				=       std::max(40, std::min(WinAPI::getConsoleWidth()-9,regkey_.get(L"CmdLine",60 )));
	opt.IOBuffSize			=       std::max(FTR_MINBUFFSIZE, regkey_.get(L"IOBuffSize", 4096));
	opt.dDelimit			=       regkey_.get(L"DigitDelimit",       TRUE );
	opt.dDelimiter			= (wchar_t)regkey_.get(L"DigitDelimiter",     0 );
	opt.WaitTimeout			=       regkey_.get(L"WaitTimeout",        30 );
	opt.AskAbort			=       regkey_.get(L"AskAbort",           TRUE );
	opt.CmdLogLimit			=       regkey_.get(L"CmdLogLimit",        100 );
	opt.ShowIdle			=       regkey_.get(L"ShowIdle",           TRUE);
	opt.TimeoutRetry		=       regkey_.get(L"TimeoutRetry",       FALSE );
	opt.RetryCount			=       regkey_.get(L"RetryCount",         0 );
	opt.LogOutput			=       regkey_.get(L"LogOutput",          FALSE );
	opt.showPassword_		=       regkey_.get(L"ShowPassword",       FALSE );
	opt.IdleColor			=       regkey_.get(L"IdleColor",          FARWrappers::makeColor(FARWrappers::fccCYAN, FARWrappers::fccBLUE) );
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
	opt.RetryTimeout		=       regkey_.get(L"RetryTimeout",       10 );
	opt.DoNotExpandErrors	=       regkey_.get(L"DoNotExpandErrors",  FALSE );
	opt.TruncateLogFile		=       regkey_.get(L"TruncateLogFile",    FALSE );
	opt.defaultServerType_	= ServerList::instance().create(
									regkey_.get(L"ServerType",		   L""));
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
	opt.fmtDateFormat		= regkey_.get(L"ServerDateFormat", L"%*s %04d%02d%02d%02d%02d%02d");

	//CMD`s
	opt.cmdPut		= regkey_.get(L"xcmdPUT",	L"STOR");
	opt.cmdAppe		= regkey_.get(L"xcmdAPPE",	L"APPE");
	opt.cmdStor		= regkey_.get(L"xcmdSTOR",	L"STOR");
	opt.cmdPutUniq	= regkey_.get(L"xcmdSTOU",	L"STOU");
	opt.cmdPasv_	= regkey_.get(L"xcmdPASV",	L"PASV");
	opt.cmdPort		= regkey_.get(L"xcmdPORT",	L"PORT");
	opt.cmdMDTM		= regkey_.get(L"xcmdMDTM",	L"MDTM");
	opt.cmdRetr		= regkey_.get(L"xcmdRETR",	L"RETR");
	opt.cmdRest_	= regkey_.get(L"xcmdREST",	L"REST");
	opt.cmdAllo		= regkey_.get(L"xcmdALLO",	L"ALLO");
	opt.cmdCwd		= regkey_.get(L"xcmdCWD",	L"CWD");
	opt.cmdXCwd		= regkey_.get(L"xcmdXCWD",	L"XCWD");
	opt.cmdDel		= regkey_.get(L"xcmdDELE",	L"DELE");
	opt.cmdRen		= regkey_.get(L"xcmdRNFR",	L"RNFR");
	opt.cmdRenTo	= regkey_.get(L"xcmdRNTO",	L"RNTO");
	opt.cmdList		= regkey_.get(L"xcmdLIST",	L"LIST");
	opt.cmdNList	= regkey_.get(L"xcmdNLIST",	L"NLIST");
	opt.cmdUser_	= regkey_.get(L"xcmdUSER",	L"USER");
	opt.cmdPass_	= regkey_.get(L"xcmdPASS",	L"PASS");
	opt.cmdAcct_	= regkey_.get(L"xcmdACCT",	L"ACCT");
	opt.cmdPwd		= regkey_.get(L"xcmdPWD",	L"PWD");
	opt.cmdXPwd		= regkey_.get(L"xcmdXPWD",	L"XPWD");
	opt.cmdMkd		= regkey_.get(L"xcmdMKD",	L"MKD");
	opt.cmdXMkd		= regkey_.get(L"xcmdXMKD",	L"XMKD");
	opt.cmdRmd		= regkey_.get(L"xcmdRMD",	L"RMD");
	opt.cmdXRmd		= regkey_.get(L"xcmdXRMD",	L"XRMD");
	opt.cmdSite		= regkey_.get(L"xcmdSITE",	L"SITE");
	opt.cmdChmod	= regkey_.get(L"xcmdCHMOD", L"CHMOD");
	opt.cmdUmask	= regkey_.get(L"xcmdUMASK", L"UMASK");
	opt.cmdIdle		= regkey_.get(L"xcmdIDLE",	L"IDLE");
	opt.cmdHelp		= regkey_.get(L"xcmdHELP",	L"HELP");
	opt.cmdQuit		= regkey_.get(L"xcmdQUIT",	L"QUIT");
	opt.cmdCDUp		= regkey_.get(L"xcmdCDUP",	L"CDUP");
	opt.cmdXCDUp	= regkey_.get(L"xcmdXCUP",	L"XCUP");
	opt.cmdSyst		= regkey_.get(L"xcmdSYST",	L"SYST");
	opt.cmdSize		= regkey_.get(L"xcmdSIZE",	L"SIZE");
	opt.cmdStat		= regkey_.get(L"xcmdSTAT",	L"STAT");

	//ProcessColor
	val = (int)FARWrappers::getInfo().AdvControl(FARWrappers::getModuleNumber(), ACTL_GETCOLOR,(void*)COL_DIALOGBOX );
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
void FTPPluginManager::WriteCfg( void )
{
	//Dialog
	regkey_.set(L"AddToDisksMenu",       g_manager.opt.AddToDisksMenu );
	regkey_.set(L"DisksMenuDigit",       g_manager.opt.DisksMenuDigit );
	regkey_.set(L"AddToPluginsMenu",     g_manager.opt.AddToPluginsMenu );
	regkey_.set(L"PluginColumnMode",     g_manager.opt.PluginColumnMode );
	regkey_.set(L"ReadDescriptions",     g_manager.opt.ReadDescriptions );
	regkey_.set(L"UpdateDescriptions",   g_manager.opt.UpdateDescriptions );
	regkey_.set(L"UploadLowCase",        g_manager.opt.UploadLowCase );
	regkey_.set(L"ShowUploadDialog",     g_manager.opt.ShowUploadDialog );
	regkey_.set(L"ResumeDefault",        g_manager.opt.ResumeDefault );
	regkey_.set(L"WaitTimeout",          g_manager.opt.WaitTimeout );
	regkey_.set(L"ShowIdle",             g_manager.opt.ShowIdle );
	regkey_.set(L"IdleColor",            g_manager.opt.IdleColor );
	regkey_.set(L"IdleMode",             g_manager.opt.IdleMode );

	regkey_.set(L"KeepAlive",            g_manager.opt.KeepAlive );
	regkey_.set(L"IdleShowPeriod",       g_manager.opt.IdleShowPeriod );
	regkey_.set(L"IdleStartPeriod",      g_manager.opt.IdleStartPeriod );
	regkey_.set(L"TimeoutRetry",         g_manager.opt.TimeoutRetry );
	regkey_.set(L"RetryCount",           g_manager.opt.RetryCount );
	regkey_.set(L"BeepTimeout",          g_manager.opt.LongBeepTimeout  );
	regkey_.set(L"AskAbort",             g_manager.opt.AskAbort );
	regkey_.set(L"DigitDelimit",         g_manager.opt.dDelimit );
	regkey_.set(L"DigitDelimiter",       (int)g_manager.opt.dDelimiter );
	regkey_.set(L"ExtCmdView",           g_manager.opt.ExtCmdView );
	regkey_.set(L"CmdLine",              g_manager.opt.CmdLine );
	regkey_.set(L"CmdLength",            g_manager.opt.CmdCount );
	regkey_.set(L"IOBuffSize",           g_manager.opt.IOBuffSize );
	regkey_.set(L"ProcessColor",         g_manager.opt.ProcessColor );
	regkey_.set(L"ServerType",           g_manager.opt.defaultServerType_->getName());
	regkey_.set(L"ProcessCmd",           g_manager.opt.ProcessCmd );

	regkey_.set(L"QueueRestoreState",    g_manager.opt.RestoreState );
	regkey_.set(L"QueueRemoveCompleted", g_manager.opt.RemoveCompleted );

	regkey_.set(L"DescriptionNames",		g_manager.opt.DescriptionNames );

	std::vector<unsigned char> CryptedPassword = MakeCryptPassword(g_manager.opt.defaultPassword_);
	regkey_.set(L"DefaultPassword", CryptedPassword);

	regkey_.set(L"Firewall",            g_manager.opt.firewall);

	regkey_.set(L"CmdLogLimit",         g_manager.opt.CmdLogLimit );
	regkey_.set(L"CmdLogFile",          g_manager.opt.CmdLogFile );
	regkey_.set(L"LogOutput",           g_manager.opt.LogOutput );
	regkey_.set(L"PassiveMode",         g_manager.opt.PassiveMode );

	regkey_.set(L"AutoAnonymous",        g_manager.opt.AutoAnonymous      );
	regkey_.set(L"CloseDots",            g_manager.opt.CloseDots          );
	regkey_.set(L"QuoteClipboardNames",  g_manager.opt.QuoteClipboardNames);
	regkey_.set(L"SetHiddenOnAbort",     g_manager.opt.SetHiddenOnAbort );
	regkey_.set(L"PwdSecurity",          g_manager.opt.PwdSecurity        );
	regkey_.set(L"RetryTimeout",         g_manager.opt.RetryTimeout       );
	regkey_.set(L"DoNotExpandErrors",    g_manager.opt.DoNotExpandErrors  );
	regkey_.set(L"TruncateLogFile",      g_manager.opt.TruncateLogFile    );
	regkey_.set(L"UseBackups",           g_manager.opt.UseBackups         );
	regkey_.set(L"ProcessCmd",           g_manager.opt.ProcessCmd         );
	regkey_.set(L"FFDup",                g_manager.opt.FFDup              );
	regkey_.set(L"UndupFF",              g_manager.opt.UndupFF );
	regkey_.set(L"ShowSilentProgress",   g_manager.opt.ShowSilentProgress );
	regkey_.set(L"AskLoginFail",         g_manager.opt.AskLoginFail       );
	regkey_.set(L"InvalidSymbols",		g_manager.opt.InvalidSymbols     );
	regkey_.set(L"CorrectedSymbols",    g_manager.opt.CorrectedSymbols   );

	//Queue
	regkey_.set(L"QueueRestoreState",    g_manager.opt.RestoreState       );
	regkey_.set(L"QueueRemoveCompleted", g_manager.opt.RemoveCompleted    );
	regkey_.set(L"AddPrefix",            g_manager.opt.sli.AddPrefix          );
	regkey_.set(L"AddPasswordAndUser",   g_manager.opt.sli.AddPasswordAndUser );
	regkey_.set(L"Quote",                g_manager.opt.sli.Quote              );
	regkey_.set(L"Size",                 g_manager.opt.sli.Size               );
	regkey_.set(L"RightBound",           g_manager.opt.sli.RightBound         );
	regkey_.set(L"ListType",             g_manager.opt.sli.ListType           );
}

void ExtendedConfig( void )
{
	FARWrappers::Dialog dlg;

	Options &o = g_manager.opt;

	dlg.addDoublebox( 3, 1, 72, 17,								getMsg(METitle))->
		addCheckbox	( 5, 2,		&o.FFDup,						getMsg(MDupFF))->
		addCheckbox	( 5, 3,		&o.UndupFF,						getMsg(MUndupFF))->
		addCheckbox	( 5, 4,		&o.ShowSilentProgress,			getMsg(MEShowProgress))->
		addCheckbox	( 5, 5,		&o.UseBackups,					getMsg(MEBackup))->
		addCheckbox	( 5, 6,		&o.ProcessCmd,					getMsg(MESendCmd))->
		addCheckbox	( 5, 7,		&o.DoNotExpandErrors,			getMsg(MEDontError))->
		addCheckbox	( 5, 8,		&o.AskLoginFail,				getMsg(MEAskLoginAtFail))->
		addCheckbox	( 5, 9,		&o.AutoAnonymous,				getMsg(MEAutoAn))->
		addCheckbox	( 5,10,		&o.CloseDots,					getMsg(MECloseDots))->
		addCheckbox	( 5,11,		&o.QuoteClipboardNames,			getMsg(MQuoteClipboardNames))->
		addCheckbox	( 5,12,		&o.SetHiddenOnAbort,			getMsg(MSetHiddenOnAbort))->
		addHLine	( 3,15)->
		addDefaultButton(0, 16,	1,								getMsg(MOk), DIF_CENTERGROUP)->
		addButton(0, 16,		-1,								getMsg(MCancel), DIF_CENTERGROUP);

	dlg.show(76, 19);
	return;

 }

int WINAPI Config( void )
{
	static FARWrappers::FLngColorDialog ColorLangs =
	{
		/*MTitle*/    getMsg(MColorTitle),
		/*MFore*/     getMsg(MColorFore),
		/*MBk*/       getMsg(MColorBk),
		/*MSet*/      getMsg(MColorColorSet),
		/*MCancel*/   getMsg(MCancel),
		/*MText*/     getMsg(MColorText)
	};

	FARWrappers::Dialog dlg;

	Options& o = g_manager.opt;
	enum
	{
		idIdleColor = 1,
		idSilentColor,
		idOk,
		idExtended
	};

	int idleScreen = 0, idleCaption = 0, idleBoth = 0;
	int keepAlive  = o.KeepAlive != 0;
	int longBeep  = o.LongBeepTimeout != 0;
	switch(o.IdleMode)
	{
	case IDLE_CAPTION:	idleCaption	= 1; break;
	case IDLE_CONSOLE:	idleScreen	= 1; break;
	case IDLE_BOTH:		idleBoth	= 1; break;
	default: BOOST_ASSERT(	0);
	}

	std::wstring delimiter;
	delimiter += o.dDelimiter;

	dlg.addDoublebox	( 3, 1, 72, 21,						getMsg(MConfigTitle))->
		addCheckbox		( 5, 2,		&o.AddToDisksMenu,		getMsg(MConfigAddToDisksMenu))->
		addFixEditorInt	(35, 2, 37,	&o.DisksMenuDigit)->
		addCheckbox		( 5, 3,		&o.AddToDisksMenu,		getMsg(MConfigAddToPluginsMenu))->
		addLabel		( 9, 4,								getMsg(MHostsMode))->
		addFixEditorInt	( 5, 4, 7,	&o.PluginColumnMode)->
		addCheckbox		( 5, 5,		&o.ReadDescriptions,	getMsg(MConfigReadDiz))->
		addCheckbox		( 5, 6,		&o.UpdateDescriptions,	getMsg(MConfigUpdateDiz))->
		addCheckbox		( 5, 7,		&o.UploadLowCase,		getMsg(MConfigUploadLowCase))->
		addCheckbox		( 5, 8,		&o.ShowUploadDialog,	getMsg(MConfigUploadDialog))->
		addCheckbox		( 5, 9,		&o.ResumeDefault,		getMsg(MConfigDefaultResume))->
		addCheckbox		( 5,10,		&o.AskAbort,			getMsg(MAskAbort))->
		addCheckbox		( 5,11,		&o.ShowIdle,			getMsg(MShowIdle))->
		addButton		(21,11,		idIdleColor,			getMsg(MColor))->
		addRadioButtonStart(32,11,	&idleScreen,			getMsg(MScreen))->
		addRadioButton	(44,11,		&idleCaption,			getMsg(MCaption))->
		addRadioButton	(58,11,		&idleBoth,				getMsg(MBoth))->
		addCheckbox		(40, 2,		&keepAlive,				getMsg(MKeepAlive))->
		addEditorInt	(67, 2, 70, &o.KeepAlive)->
		addLabel		(71, 2,								getMsg(MSec))->
		addCheckbox		(40, 3,		&o.TimeoutRetry,		getMsg(MAutoRetry))->
		addEditorInt	(67, 3, 70, &o.RetryTimeout)->
		addCheckbox		(40, 4,		&longBeep,				getMsg(MLongOp))->
		addEditorInt	(67, 4, 70, &o.LongBeepTimeout)->
		addLabel		(71, 4,								getMsg(MSec))->
		addLabel		(40, 5,								getMsg(MWaitTimeout))->
		addEditorInt	(67, 5, 70, &o.WaitTimeout)->
		addCheckbox		(40, 6,		&o.dDelimit,			getMsg(MDigitDelimit))->
		addEditor		(69, 6, 70, &delimiter)->
		addCheckbox		(40, 7,		&o.ExtCmdView,			getMsg(MExtWindow))->
		addLabel		(40, 8,								getMsg(MExtSize))->
		addEditorInt	(60, 8, 63, &o.CmdLine)->
		addLabel		(64, 8,								L" x ")->
		addEditorInt	(67, 8, 70, &o.CmdCount)->
		addLabel		(40, 9,								getMsg(MHostIOSize))->
		addEditorInt	(60, 9, 70, &o.IOBuffSize)->
		addLabel		(40,10,								getMsg(MSilentText))->
		addButton		(60,10,		idSilentColor,			getMsg(MColor))->
		addHLine		( 5,12)->
		addLabel		( 5,13,								getMsg(MConfigDizNames))->
		addEditor		( 5,14, 70, &o.DescriptionNames)->
		addLabel		( 5,15,								getMsg(MConfigDefPassword))->
		addPasswordEditor(5,16, 34, &o.defaultPassword_)->
		addLabel		( 5,17,								getMsg(MConfigFirewall))->
		addEditor		( 5,18, 34, &o.firewall)->
		addLabel		(40,15,								getMsg(MLogFilename))->
		addEditorInt	(66,15,70,	&o.CmdLogLimit)->
		addLabel		(71,15,								getMsg(MKBytes))->
		addEditor		(40,16,70,	&o.CmdLogFile)->
		addCheckbox		(40, 17,	&o.LogOutput,			getMsg(MLogDir))->
		addCheckbox		(40, 18,	&o.PassiveMode,			getMsg(MConfigPassiveMode))->
		addHLine		( 5, 19)->
		addDefaultButton(0, 20,		idOk,					getMsg(MOk), DIF_CENTERGROUP)->
		addButton		(0, 20,		-1,						getMsg(MCancel), DIF_CENTERGROUP)->
		addButton		(0, 20,		idExtended,				getMsg(MExtOpt), DIF_CENTERGROUP);

	while(true)
	{
		int rc = dlg.show(76, 23);
		if(rc == -1)
			return false;

		o.IdleMode = idleScreen? IDLE_CONSOLE : (idleCaption? IDLE_CAPTION : IDLE_BOTH);
		if(!keepAlive)
			g_manager.opt.KeepAlive = 0;
		o.dDelimiter	= delimiter.empty()? L' ' : delimiter[0];
		o.CmdLine		= std::max(10, std::min(WinAPI::getConsoleWidth()-9, o.CmdLine));
		o.CmdCount		= std::max(5,  std::min(WinAPI::getConsoleHeight()-5, o.CmdCount));
		o.IOBuffSize	= std::max(FTR_MINBUFFSIZE, o.IOBuffSize);

		switch(rc)
		{
		case idIdleColor:
			o.IdleColor = FARWrappers::ShowColorColorDialog(o.IdleColor, &ColorLangs, L"");
			break;
		case idSilentColor:
			o.ProcessColor = FARWrappers::ShowColorColorDialog(o.ProcessColor,&ColorLangs, L"");
			break;
		case idExtended:
			ExtendedConfig();
			break;
		case idOk:
			g_manager.WriteCfg();
			g_manager.InvalidateAll();
			return true;
		default:
			BOOST_ASSERT(0);
			return false;
		}
	}
}
