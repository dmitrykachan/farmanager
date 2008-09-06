#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "farwrapper/dialog.h"
#include "farwrapper/menu.h"

bool WINAPI AskSaveList(SaveListInfo* sli)
{
	FARWrappers::Dialog dlg(L"FTPSaveList");
	boost::array<int, 3> listType = {{0}};
	listType[sli->ListType] = true;

	dlg.addDoublebox	( 3, 1,56,14,					getMsg(MFLTitle))->
		addLabel		( 4, 2,							getMsg(MFLSaveTo))->
		addEditor		( 4, 3,54,	&sli->path, 0,		L"SaveListHistory")->
		addHLine		( 1, 4)->

		addRadioButtonStart(4,5,	&listType[0],		getMsg(MFLUrls))->
		addRadioButton	( 4, 6,		&listType[1],		getMsg(MFLTree))->
		addRadioButton	(32, 5,		&listType[2],		getMsg(MFLGroups))->
		addHLine		( 1, 7)->
		addCheckbox		( 4, 8,		&sli->AddPrefix,	getMsg(MFLPrefix))->
		addCheckbox		( 4, 9,		&sli->AddPasswordAndUser,		getMsg(MFLPass))->
		addCheckbox		( 4,10,		&sli->Quote,		getMsg(MFLQuote))->
		addCheckbox		( 4,11,		&sli->Size,			getMsg(MFLSizes))->

		addLabel		(28,11,							getMsg(MFLSizeBound))->
		addEditorInt	(44,11,47,	&sli->RightBound)->
		addHLine		( 1,12)->

		addCheckbox		(32, 8,		&sli->Append,		getMsg(MFAppendList))->

		addDefaultButton(0, 13,		0,					getMsg(MFtpSave), DIF_CENTERGROUP)->
		addButton		(0, 13,		-1,					getMsg(MDownloadCancel), DIF_CENTERGROUP);

	if(dlg.show(60, 16) == -1)
		return false;

	sli->ListType = listType[0]? sltUrlList : (listType[1]? sltTree : sltGroup);
	sli->RightBound = std::max(30, sli->RightBound);

	return true;
}

bool FTP::CopyAskDialog(bool Move, bool Download, FTPCopyInfo* ci)
{
	FARWrappers::Dialog dlg(L"FTPCmd");

	std::wstring IncludeMask = L"*";
	std::wstring ExcludeMask = L"";

	ci->showProcessList = ci->addToQueque = ci->FTPRename = false;

	bool doNotScan = false;

	boost::array<int, 4> msg = {{0}};
	switch(ci->msgCode)
	{
	case ocOverAll:
		msg[1] = true;
		break;
	case ocSkipAll:
		msg[2] = true;
	    break;
	case ocResumeAll:
		msg[3] = true;
	    break;
	default:
		msg[0] = true;
	}

	dlg.addDoublebox	( 3, 1,72,17,						Move ? getMsg(MRenameTitle) : getMsg(MDownloadTitle))->
		addLabel		( 5, 2,								ci->download? getMsg(MDownloadTo) : getMsg(MUploadTo))->
		addEditor		( 5, 3,70, &ci->destPath, 0,		ci->download? FTP_GETHISTORY : FTP_PUTHISTORY)->

		addLabel		( 5, 4,								getMsg(MInclude))->
		addEditor		( 5, 5,70,	&IncludeMask, 0,		FTP_INCHISTORY)->

		addLabel		( 5, 6,								getMsg(MExclude))->
		addEditor		( 5, 7,70,	&IncludeMask, 0,		FTP_EXCHISTORY)->
		addHLine		( 3, 8)->

		addCheckbox		( 5, 9,		&ci->asciiMode,			getMsg(MDownloadAscii))->
		addCheckbox		( 5,10,		&doNotScan,				getMsg(MDoNotScan), 0, DIF_DISABLE)->
		addCheckbox		( 5,11,		&ci->showProcessList,	getMsg(MSelectFromList))->
		addCheckbox		( 5,12,		&ci->addToQueque,		getMsg(MAddQueue), 0, Move? DIF_DISABLE : 0)->
		addCheckbox		( 5,13,		&ci->uploadLowCase,		getMsg(MConfigUploadLowCase), 0, Download? DIF_DISABLE : 0)->
		addCheckbox		( 5,14,		&ci->FTPRename,			getMsg(MDownloadOnServer), (!Download || !Move)? DIF_DISABLE : 0)->

		addLabel		(40, 9,								getMsg(MDefOverwrite))->
		addRadioButtonStart(41,10,	&msg[0],				getMsg(MOverAsk))->
		addRadioButton	(41,11,		&msg[1],				getMsg(MOverOver))->
		addRadioButton	(41,12,		&msg[2],				getMsg(MOverSkip))->
		addRadioButton	(41,13,		&msg[3],				getMsg(MOverResume), 0, (!FtpFilePanel_.getConnection().isResumeSupport() && Download)? DIF_DISABLE : 0) ->
		addHLine		( 3,15)->

		addDefaultButton( 0,16,		0,			ci->download? (Move ? getMsg(MRenameTitle) : getMsg(MDownload)) 
															: getMsg(MUpload), DIF_CENTERGROUP)->
		addButton		(0, 16,		0,						getMsg(MDownloadCancel), DIF_CENTERGROUP);

	if(dlg.show(76, 19) == -1)
		return false;

	longBeep_.reset();

	if(ci->destPath.empty())
		return false;

	boost::trim(IncludeMask);
	boost::trim(ExcludeMask);

	ci->msgCode = msg[1]? ocOverAll : (msg[2]? ocSkipAll : (msg[3]? ocResumeAll: ci->msgCode));

	if(ci->destPath.size() >= 4 &&
		boost::istarts_with(ci->destPath, L"FTP:"))
		ci->destPath.erase(0, FTP_CMDPREFIX_SIZE+1);

	return true;
}

void QueryExHostOptions(FTP* ftp, FtpHostPtr& p)
{
	FARWrappers::Dialog dlg(L"FTPExtHost");

	dlg.addDoublebox( 3, 1,72,17,				getMsg(MEHTitle))->
		addCheckbox	( 5, 2, &p->FFDup,			getMsg(MDupFF))->
		addCheckbox	( 5, 3, &p->UndupFF,		getMsg(MUndupFF))->
		addCheckbox	( 5, 5, &p->SendAllo,		getMsg(MSendAllo))->
		addCheckbox	( 5, 6, &p->UseStartSpaces,	getMsg(MUseStartSpaces))->
		addHLine	( 3,15)->
		addDefaultButton( 0,16, 0,				getMsg(MOk), DIF_CENTERGROUP)->
		addButton		( 0,16, -1,				getMsg(MCancel), DIF_CENTERGROUP);

	
	dlg.show(76, 19);
	ftp->longBeep_.reset();
}

bool FTP::EditHostDlg(int title, FtpHostPtr& p, bool ToDescription)
{
	FARWrappers::Dialog dlg(L"FTPConnect");

	enum
	{
		idTable = 1, idServer, idExtOpt, idSave, idConnect, idHost, idDescription
	};
	std::wstring fullhostname = p->url_.getUrl(true);

	dlg.addDoublebox( 3, 1,72,17,							getMsg(title))->
		addLabel	( 5, 2,									getMsg(MFtpName))->
		addEditor	( 7, 3,70,		&fullhostname, idHost,	FTP_HOSTHISTORY)->
		addHLine	( 4, 4)->
		addLabel	( 5, 5,									getMsg(MUser))->
		addEditor	(18, 5,70,		&p->url_.username_, 0,	FTP_USERHISTORY)->
		addLabel	( 5, 6,									getMsg(MPassword))->
		addEditor	(18, 6,70,		&p->url_.password_)-> // TODO FDI_PSWEDIT
		addLabel	( 5, 7,									getMsg(MHostDescr))->
		addEditor	(18, 7,70,		&p->hostDescription_, idDescription)->
		addHLine	( 3, 8)->
		addCheckbox	( 5, 9,			&p->AskLogin,			getMsg(MAskLogin))->
		addCheckbox	( 5,10,			&p->AsciiMode,			getMsg(MAsciiMode))->
		addCheckbox	( 5,11,			&p->PassiveMode,		getMsg(MPassiveMode))->
//		addCheckbox	( 5,12,			&p->UseFirewall,		getMsg(MUseFirewall))->

		addCheckbox	(40, 9,			&p->ExtCmdView,			getMsg(MExtWindow))->
		addCheckbox	(40,10,			&p->ExtList,			getMsg(MExtList))->
		addEditor	(44,11,59,		&p->listCMD_)->
		addLabel	(40,12,									getMsg(MHostIOSize))->
		addEditorInt(60,12,70,		&p->IOBuffSize)->
		addHLine	( 3,14)->
		addButton	(0,15,			idTable,				getMsg(MFtpSelectTable), DIF_CENTERGROUP)->
		addButton	(0,15,			idServer,				getMsg(MServerType), DIF_CENTERGROUP)->
		addButton	(0,15,			idExtOpt,				getMsg(MExtOpt), DIF_CENTERGROUP)->
		addDefaultButton(0,16,		idSave,					getMsg(MFtpSave), DIF_CENTERGROUP)->
		addButton	(0,16,			idConnect,				getMsg(MFtpConnect), DIF_CENTERGROUP)->
		addButton	(0,16,			-1,						getMsg(MCancel), DIF_CENTERGROUP);

	if(ToDescription)
		dlg.find(ToDescription? idDescription : idHost).setFocus();

	int rc;
	do 
	{
		rc = dlg.show(76, 19);

		longBeep_.reset();

		dlg.find(idHost).setFocus();
		switch(rc)
		{
		case -1:
			return false;
		case idTable:
			p->codePage_ = SelectTable(p->codePage_);
			continue;
		case idServer:
			p->serverType_ = SelectServerType(p->serverType_);
			continue;
		case idExtOpt:
			QueryExHostOptions(this, p);
			continue;
		}
		boost::trim(fullhostname);
		if(fullhostname.empty())
			continue;
		if(fullhostname[0] == L'\"' && FARWrappers::getFSF()->Unquote)
		{
			std::vector<wchar_t> v(fullhostname.begin(), fullhostname.end());
			v.push_back(0);
			FARWrappers::getFSF()->Unquote(&v[0]);
			fullhostname.assign(v.begin(), v.end()-1);
			if(fullhostname.empty())
				continue;
		}

		if(rc == idSave || rc == idConnect)
			break;
	} while(1);

	p->IOBuffSize  = std::max(FTR_MINBUFFSIZE, p->IOBuffSize);
	p->SetHostName(fullhostname, std::wstring(p->url_.username_), std::wstring(p->url_.password_));
	SYSTEMTIME currentTime;
	GetSystemTime(&currentTime);
	SystemTimeToFileTime(&currentTime, &p->lastEditTime_); 

	if(rc == idConnect)
	{
		FullConnect(p);
		return false;
	}
	return true;
}

bool FTP::EditDirectory(std::wstring& Name, std::wstring &Desc, bool newDir, bool hostMode)
{
	FARWrappers::Dialog dlg;

	dlg.addLabel	(5, 2,							getMsg(MMkdirName))->
		addEditor	(5, 3, 70, &Name, 0, FTP_FOLDHISTORY, newDir? 0 : DIF_DISABLE)->
		addLabel	(5, 4,							getMsg(MHostDescr), 0, hostMode? 0 : DIF_DISABLE)->
		addEditor	(5, 5, 70, &Desc, 0, 0, hostMode? 0 : DIF_DISABLE)->
		addDoublebox(3, 1, 72, 8, newDir ?			getMsg(MMkdirTitle) : getMsg(MChDir))->
		addButton	(0, 7, 0,						getMsg(MOk), DIF_CENTERGROUP)->
		addButton	(1, 7, -1,						getMsg(MCancel), DIF_CENTERGROUP)->
		addHLine	(4, 6);

	return dlg.show(76,10) != -1;
}

UINT FTP::SelectTable(UINT codePage)
{
	static const wchar_t* strings[] =
	{
		L"OEM",
		L"Windows-1251",
		L"Dos-866",
		L"KOI-R",
		L"UTF-8",
	};

	FARWrappers::Menu menu(strings, false, FMENU_AUTOHIGHLIGHT);
	menu.setTitle(getMsg(MTableTitle));

	typedef boost::array<unsigned int, sizeof(strings)/sizeof(*strings)> CodepageArray;
	static const CodepageArray tables =
	{{
		CP_OEMCP,
		1251,
		866,
		20866,
		CP_UTF8
	}};

	size_t index = std::find(tables.begin(), tables.end(), codePage) - tables.begin();
	BOOST_ASSERT(index < tables.size());

	menu.select(index);
	int exitCode = menu.show();

	if(exitCode < 0)
		return codePage;

	return tables[exitCode];
}

bool WINAPI GetLoginData(std::wstring &username, std::wstring &password, bool forceAsk)
{
	if(!username.empty() && password.empty())
		forceAsk = true;
	if(!forceAsk)
		return true;

	//Default user
	if(username.empty() && g_manager.opt.AutoAnonymous)
		username = L"anonymous";

	if(password.empty() && boost::algorithm::to_lower_copy(username) == L"anonymous")
		password = g_manager.opt.defaultPassword_;

	FARWrappers::Dialog dlg(L"FTPConnect");

	enum
	{
		idUser = 1, idPassword, idOk
	};

	dlg.addDoublebox( 3, 1,72,8,				getMsg(MLoginInfo))->
		addLabel	( 5, 2,						getMsg(MUserName))->
		addEditor	( 5, 3,70,	&username,		idUser, FTP_USERHISTORY)->
		addLabel	( 5, 4,						getMsg(MUserPassword))->
		addPasswordEditor( 5, 5,70,	&password,  idPassword)->
		addHLine	( 5, 6)->
		addDefaultButton( 0, 7, idOk,			getMsg(MOk), DIF_CENTERGROUP)->
		addButton	( 0, 7,    -1,				getMsg(MCancel), DIF_CENTERGROUP);

    if(username.empty() || !password.empty())
		dlg.find(idUser).setFocus();
     else
		 dlg.find(idPassword).setFocus();

	do
	{
		int rc = dlg.show(76, 10);
		if(rc == -1)
		{
			SetLastError(ERROR_CANCELLED);
			return false;
		}

		//Empty user name
		if(username.empty())
			continue;
		return true;
	}while(1);
}


bool SayNotReadedTerminates(const std::wstring& fnm, bool& SkipAll)
{
	const wchar_t* MsgItems[] =
	{
		getMsg(MError),
		getMsg(MCannotCopyHost),
		fnm.c_str(),
		/*0*/getMsg(MCopySkip),
		/*1*/getMsg(MCopySkipAll),
		/*2*/getMsg(MCopyCancel)
	};

	if(SkipAll)
		return false;

	switch(FARWrappers::message(MsgItems, 3, FMSG_WARNING|FMSG_DOWN|FMSG_ERRORTYPE))
	{
	case  1: SkipAll = false;
	case  0: break;
	default: return false;
	}

	return false;
}
