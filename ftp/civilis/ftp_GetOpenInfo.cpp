#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "panelview.h"

static void SetTitles(wchar_t *cols[], const wchar_t* fmt, int cn)
{
	int n = 0;

	if (!fmt || *fmt == 0 || !cn )
		return;

	do
	{
		if(*fmt == L'C' || *fmt == L'c')
		{
			fmt++;
			switch(*fmt)
			{
				case L'0': cols[n] = const_cast<wchar_t*>(getMsg(MFileMode)); break;
				case L'1': cols[n] = const_cast<wchar_t*>(getMsg(MLink)); break;
				default: cols[n] = L"<unk>";
			}
		} else
			cols[n] = NULL;

		n++;
		if(n >= cn)
			return;

		while(1)
		{
			if(*fmt == 0)
				return;

			if(*fmt == L',')
			{
				fmt++;
				break;
			}
			fmt++;
		}
	}while( 1 );
}

void FTP::GetOpenPluginInfo(struct OpenPluginInfo *pi)
{  
	PROCP((ShowHosts? L"HOSTS": L"FTP") << L"cc:"  << CallLevel 
		<< " pi:" << pi << L"hC");
	PanelInfo thisPInfo = { 0 };
	static int inside = 0;

	inside++;
	memset( pi, 0, sizeof(*pi) );
	pi->StructSize = sizeof(*pi);

	//---------------- FLAGS
	if(ShowHosts)
		pi->Flags = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING;
	else
		pi->Flags = OPIF_ADDDOTS | OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING |	OPIF_SHOWPRESERVECASE;

	//---------------- HOST, CURDIR
	pi->HostFile = NULL;
	static std::wstring curPath;

	curPath	= panel_->getCurrentDirectory();
	pi->CurDir = curPath.c_str();


	//---------------- TITLE
	pi->PanelTitle = panelTitle_.c_str();

	if(ShowHosts)
		panelTitle_ = std::wstring(L" FTP: ") + pi->CurDir;
	else
		panelTitle_	= std::wstring(L" FTP: ") + getConnection().getHost()->url_.username_ + L'@' + 
						getConnection().getHost()->url_.Host_ + pi->CurDir;

	if(inside > 1)
	{
		inside--;
		return;
	}

	//---------------- FORMAT
	static std::wstring Format;

	if ( ShowHosts )
		Format = L"//Hosts/"; // TODO
	else
		Format = L"//" + getConnection().getHost()->url_.Host_ + L"/";

	Format += pi->CurDir + ( *pi->CurDir == '/' || *pi->CurDir == '\\' );

	pi->Format = Format.c_str();
	//---------------- INFO LINES
	static struct InfoPanelLine InfoLines[7];
	
	memset( InfoLines, 0, sizeof(InfoLines) );
	//Client
	Utils::safe_wcscpy(InfoLines[0].Text, getMsg(MFtpInfoFTPClient));
	InfoLines[0].Separator = TRUE;

	Utils::safe_wcscpy(InfoLines[1].Text, getMsg(MFtpInfoHostName));
	if(!ShowHosts)
		Utils::safe_wcscpy(InfoLines[1].Data, getConnection().getHost()->url_.toString());

	Utils::safe_wcscpy(InfoLines[2].Text, getMsg(MFtpInfoHostDescr));
	if(!ShowHosts)
		Utils::safe_wcscpy(InfoLines[2].Data, getConnection().getHost()->hostDescription_);

	Utils::safe_wcscpy(InfoLines[3].Text, getMsg(MFtpInfoHostType));
	if(getConnection().isConnected())
	{
		Utils::safe_wcscpy(InfoLines[2].Data, getConnection().getSystemInfo());
	}
	else
		InfoLines[3].Data[0] = 0;

	//Titles
	Utils::safe_wcscpy(InfoLines[4].Text, getMsg(MFtpInfoFtpTitle));
	InfoLines[4].Separator = TRUE;

	InfoLines[5].Text[0] = 0;
	if (getConnection().isConnected())
		Utils::safe_wcscpy(InfoLines[5].Data, getConnection().GetStartReply());
	else
		InfoLines[5].Data[0] = 0;

	wchar_t* m = wcspbrk(InfoLines[5].Data, L"\n\r"); if (m) *m = 0;

	Utils::safe_wcscpy(InfoLines[6].Text, getMsg(MResmResume));
	if(getConnection().isConnected())
		Utils::safe_wcscpy(InfoLines[6].Data, getMsg(getConnection().isResumeSupport()? MResmSupport:MResmNotSupport));
	else
		Utils::safe_wcscpy(InfoLines[6].Data, getMsg(MResmNotConnected));

	pi->InfoLines       = InfoLines;
	pi->InfoLinesNumber = 7;

	if(!g_manager.opt.ReadDescriptions)
	{
		pi->DescrFilesNumber = 0;
		pi->DescrFiles = 0;
	} else
	{
		std::vector<wchar_t> descrFilesString(
			g_manager.opt.DescriptionNames.begin(), 
			g_manager.opt.DescriptionNames.end());
		descrFilesString.push_back(0);

		static std::vector<const wchar_t*> descriptionFiles;
		descriptionFiles.clear();
		std::vector<wchar_t>::iterator itr = descrFilesString.begin();
		while(itr != descrFilesString.end())
		{
			Parser::skipSpaces(itr, descrFilesString.end());
			if(itr == descrFilesString.end())
				break;
			descriptionFiles.push_back(&(*itr));
			itr = std::find(itr, descrFilesString.end(), L',');
			if(itr != descrFilesString.end())
			{
				*itr = 0;
				++itr;
			}
		}

		pi->DescrFiles = &descriptionFiles[0];
		pi->DescrFilesNumber = static_cast<int>(descriptionFiles.size());
	}

	//---------------- SHORTCUT
	static std::wstring ShortcutData;
	if(ShowHosts) 
	{
		/*
		HOSTSTS
		Hostspath
		*/
		ShortcutData = L"HOST:" + panel_->getCurrentDirectory();
	} else {
		/*
		FTP
		Host
		1
		AskLogin    + 3
		AsciiMode   + 3
		PassiveMode + 3
		UseFirewall + 3
		HostTable
		1
		User
		1
		Password
		1
		ExtCmdView + 3
		IOBuffSize (atoi)
		1
		FFDup + '0'
		1
		*/
		ShortcutData = L"FTP:" + getConnection().getHost()->url_.Host_ + 
			L'\x1' + static_cast<wchar_t>(getConnection().getHost()->AskLogin+3) + 
					 static_cast<wchar_t>(getConnection().getHost()->AsciiMode+3) +
					 static_cast<wchar_t>(getConnection().getHost()->PassiveMode+3) +
					 static_cast<wchar_t>(getConnection().getHost()->UseFirewall+3) +
					 getConnection().getHost()->serverType_->getName() +
			L'\x1' + boost::lexical_cast<std::wstring>(getConnection().getHost()->codePage_) +
			L'\x1' + getConnection().getHost()->url_.username_ +
			L'\x1' + getConnection().getHost()->url_.password_ +
			L'\x1' + static_cast<wchar_t>(getConnection().getHost()->ExtCmdView+3) + 
			         boost::lexical_cast<std::wstring>(getConnection().getHost()->IOBuffSize) +
			L'\x1' + static_cast<wchar_t>('0'+getConnection().getHost()->FFDup) +
			L'\x1';
	}
	pi->ShortcutData = ShortcutData.c_str();

	//---------------- PANEL MODES
	//HOSTST
	if (ShowHosts)
	{
		static struct PanelMode PanelModesArray[10] = {0};
		static wchar_t *ColumnTitles[4] = { NULL };
		static wchar_t *ColumnTitles2[4] = { NULL };
		static std::wstring Mode, ModeSz, ModeSz2;
		size_t      descrLen = 0,
			usrLen  = 0,
			dirLen   = 0,
			hstLen = 0;

		if(!thisPInfo.PanelItems)
			getPanelInfo(thisPInfo);

		for(int n = 0; n < thisPInfo.ItemsNumber; n++ )
		{
			if(thisPInfo.PanelItems[n].UserData != HostView::ParentDirHostID)
			{
				const FtpHostPtr& p = hostPanel_.findhost(thisPInfo.PanelItems[n].UserData);
				if (!p) continue;
				descrLen	= std::max(descrLen, p->hostDescription_.size());
				usrLen		= std::max(usrLen,   p->url_.username_.size());
				dirLen		= std::max(dirLen,   p->url_.directory_.size());
				hstLen		= std::max(hstLen,   p->url_.Host_.size());
			}
		}
		ColumnTitles[0] = const_cast<wchar_t*>(getMsg(MHostColumn));

		//==1
		PanelModesArray[1].ColumnTypes   = L"C0";
		PanelModesArray[1].ColumnWidths  = L"0";

		//==2
		size_t num = 1;
		size_t n   = (thisPInfo.PanelRect.right-thisPInfo.PanelRect.left)/2;
		size_t columnCount = (usrLen? 1 : 0) + (dirLen? 1 : 0) + (descrLen? 1 : 0);
		if(columnCount != 0)
		{
			size_t sumLen = hstLen + usrLen + dirLen + descrLen + columnCount;
			size_t panelWidth = thisPInfo.PanelRect.right-thisPInfo.PanelRect.left;
			if(sumLen > panelWidth)
			{
				hstLen = std::min(panelWidth/2, hstLen);
				size_t columnWidth = (panelWidth-hstLen-columnCount)/columnCount;
				usrLen	= std::min(usrLen, columnWidth);
				dirLen	= std::min(dirLen, columnWidth);
				descrLen= std::min(descrLen, columnWidth);
			}
		}

		//HOST
		Mode = L"C0";
		ModeSz = L"0";

		//HOME
		if(dirLen)
		{
			Mode += L",C1";
			ModeSz += L',' + boost::lexical_cast<std::wstring>(dirLen);
			ColumnTitles[num++] = const_cast<wchar_t*>(getMsg(MHomeColumn));
		}

		//UNAME
		if (usrLen)
		{
			Mode += L",C2";
			ModeSz += L',' + boost::lexical_cast<std::wstring>(usrLen);
			ColumnTitles[num++] = const_cast<wchar_t*>(getMsg(MUserColumn));
		}

		//DIZ
		if(descrLen)
		{
			Mode += L",Z";
			ModeSz += L',' + boost::lexical_cast<std::wstring>(descrLen);
			ColumnTitles[num] = const_cast<wchar_t*>(getMsg(MDescColumn));
		}

		PanelModesArray[2].ColumnTypes   = const_cast<wchar_t*>(Mode.c_str());
		PanelModesArray[2].ColumnWidths  = const_cast<wchar_t*>(ModeSz.c_str());
		PanelModesArray[2].ColumnTitles  = ColumnTitles;

		ColumnTitles2[0] = const_cast<wchar_t*>(getMsg(MHostColumn));
		ColumnTitles2[1] = const_cast<wchar_t*>(getMsg(MDescColumn));

		if (!descrLen)
		{
			PanelModesArray[3].ColumnTypes   = L"C0";
			PanelModesArray[3].ColumnWidths  = L"0";
		} else
		{
			PanelModesArray[3].ColumnTypes   = L"C0,Z";
			PanelModesArray[3].ColumnWidths  = const_cast<wchar_t*>(ModeSz2.c_str());
			ModeSz2 = boost::lexical_cast<std::wstring>(
				std::min( static_cast<size_t>(thisPInfo.PanelRect.right-thisPInfo.PanelRect.left)/2,hstLen)
				) + L",0";
		}
		PanelModesArray[3].ColumnTitles  = ColumnTitles2;

		pi->PanelModesArray  = PanelModesArray;
		pi->PanelModesNumber = sizeof(PanelModesArray)/sizeof(PanelModesArray[0]);
		pi->StartPanelMode   = 0;

		for ( n = 1; n <=3; n++ )
		{
			PanelModesArray[n].StatusColumnTypes   = PanelModesArray[n].ColumnTypes;
			PanelModesArray[n].StatusColumnWidths  = PanelModesArray[n].ColumnWidths;
		}

	} else
	{
		//FTP
		static struct PanelMode PanelModesArray[10] = {0};

		static wchar_t *ColumnTitles[10];
		SetTitles(ColumnTitles, getMsg(MColumn9), ARRAY_SIZE(ColumnTitles));

		PanelModesArray[9].ColumnTypes  = const_cast<wchar_t*>(getMsg(MColumn9));
		PanelModesArray[9].ColumnWidths = const_cast<wchar_t*>(getMsg(MSizes9));
		PanelModesArray[9].ColumnTitles = ColumnTitles;
		try
		{
			PanelModesArray[9].FullScreen= boost::lexical_cast<int>(getMsg(MFullScreen9));
		}
		catch (boost::bad_lexical_cast &)
		{
			PanelModesArray[9].FullScreen=0;
		}

		static wchar_t *ColumnTitles1[10];
		SetTitles(ColumnTitles1, getMsg(MColumn0), ARRAY_SIZE(ColumnTitles));

		PanelModesArray[0].ColumnTypes  = const_cast<wchar_t*>(getMsg(MColumn0));
		PanelModesArray[0].ColumnWidths = const_cast<wchar_t*>(getMsg(MSizes0));
		PanelModesArray[0].ColumnTitles = ColumnTitles1;
		try
		{
		PanelModesArray[0].FullScreen   = boost::lexical_cast<int>(getMsg(MFullScreen0));
		}
		catch (boost::bad_lexical_cast &)
		{
			PanelModesArray[9].FullScreen=0;
		}

		pi->PanelModesArray  = PanelModesArray;
		pi->PanelModesNumber = ARRAY_SIZE(PanelModesArray);
	}

	//---------------- KEYBAR
	static struct KeyBarTitles KeyBar;
	memset(&KeyBar,0,sizeof(KeyBar));

	KeyBar.ShiftTitles[1-1] = L"";
	KeyBar.ShiftTitles[2-1] = L"";
	KeyBar.ShiftTitles[3-1] = L"";

	KeyBar.AltTitles[6-1]   = const_cast<wchar_t*>(getMsg(MAltF6));

		KeyBar.ShiftTitles[1-1] = const_cast<wchar_t*>(getMsg(MShiftF1));
	if(ShowHosts)
		KeyBar.ShiftTitles[4-1] = const_cast<wchar_t*>(getMsg(MShiftF4));
	else
		KeyBar.ShiftTitles[7-1] = const_cast<wchar_t*>(getMsg(MShiftF7));
	pi->KeyBar=&KeyBar;

//TODO	if(CurrentState != fcsExpandList && !IS_SILENT(FP_LastOpMode))
		FARWrappers::Screen::fullRestore();

	//Back
	inside--;
}
