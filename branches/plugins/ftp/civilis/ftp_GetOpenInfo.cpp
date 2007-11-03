#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

void SetTitles(wchar_t *cols[], const wchar_t* fmt, int cn)
{
	int n = 0;

	if (!fmt || *fmt == 0 || !cn )
		return;

	do
	{
		if(*fmt == L'C' || *fmt == L'c' )
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
		if ( n >= cn )
			return;

		while( 1 )
		{
			if ( *fmt == 0 )
				return;

			if ( *fmt == L',' )
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
		panelTitle_	= std::wstring(L" FTP: ") + chost_.url_.username_ + L'@' + 
						chost_.url_.Host_ + pi->CurDir;

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
		Format = L"//" + chost_.url_.Host_ + L"/";

	Format += pi->CurDir + ( *pi->CurDir == '/' || *pi->CurDir == '\\' );

	pi->Format = Format.c_str();
	//---------------- INFO LINES
	static struct InfoPanelLine InfoLines[7];
	
	memset( InfoLines, 0, sizeof(InfoLines) );
	//Client
	Utils::safe_wcscpy(InfoLines[0].Text, getMsg(MFtpInfoFTPClient));
	InfoLines[0].Separator = TRUE;

	Utils::safe_wcscpy(InfoLines[1].Text, getMsg(MFtpInfoHostName));
	Utils::safe_wcscpy(InfoLines[1].Data, chost_.url_.fullhostname_);

	Utils::safe_wcscpy(InfoLines[2].Text, getMsg(MFtpInfoHostDescr));
	Utils::safe_wcscpy(InfoLines[2].Data, chost_.hostDescription_);

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
		DecodeCmdLine + '0'
		1
		*/
		ShortcutData = L"FTP:" + chost_.url_.Host_ + 
			L'\x1' + static_cast<wchar_t>(chost_.AskLogin+3) + 
					 static_cast<wchar_t>(chost_.AsciiMode+3) +
					 static_cast<wchar_t>(chost_.PassiveMode+3) +
					 static_cast<wchar_t>(chost_.UseFirewall+3) +
					 chost_.serverType_->getName() +
			L'\x1' + boost::lexical_cast<std::wstring>(chost_.codePage_) +
			L'\x1' + chost_.url_.username_ +
			L'\x1' + chost_.url_.password_ +
			L'\x1' + static_cast<wchar_t>(chost_.ExtCmdView+3) + 
			         boost::lexical_cast<std::wstring>(chost_.IOBuffSize) +
			L'\x1' + static_cast<wchar_t>('0'+chost_.FFDup) +
					 static_cast<wchar_t>('0'+chost_.DecodeCmdLine) +
			L'\x1';
	}
	pi->ShortcutData = ShortcutData.c_str();

	//---------------- PANEL MODES
	//HOSTST
	if (ShowHosts)
	{
		static struct PanelMode PanelModesArray[10];
		static wchar_t *ColumnTitles[4] = { NULL };
		static wchar_t *ColumnTitles2[4] = { NULL };
		static std::wstring Mode, ModeSz, ModeSz2;
		size_t      dizLen = 0,
			nmLen  = 0,
			hLen   = 0,
			hstLen = 0;
		size_t		n,num;
		const FTPHost* p;

		std::fill_n(reinterpret_cast<char*>(PanelModesArray), sizeof(PanelModesArray), 0);
		if ( !thisPInfo.PanelItems)
			getPanelInfo(thisPInfo);

		for (n = 0; static_cast<int>(n) < thisPInfo.ItemsNumber; n++ )
		{
			p = hostPanel_.findhost(thisPInfo.PanelItems[n].UserData);
			if (!p) continue;
			dizLen = std::max(dizLen,p->hostDescription_.size());
			nmLen  = std::max(nmLen, p->url_.username_.size());
			hLen   = std::max(hLen,  p->url_.directory_.size());
			hstLen = std::max(hstLen,p->url_.Host_.size());
		}
		ColumnTitles[0] = const_cast<wchar_t*>(getMsg(MHostColumn));

		//==1
		PanelModesArray[1].ColumnTypes   = L"C0";
		PanelModesArray[1].ColumnWidths  = L"0";

		//==2
		num = 1;
		n   = (thisPInfo.PanelRect.right-thisPInfo.PanelRect.left)/2;

		//HOST
		Mode = L"C0";
		ModeSz = hLen || nmLen || dizLen ? boost::lexical_cast<std::wstring>(hstLen) : L"0";

		//HOME
		if(hLen)
		{
			Mode += L",C1";
			if(hLen < n && (nmLen || dizLen))
			{
				ModeSz += L',' + boost::lexical_cast<std::wstring>(hLen);
				n -= hLen;
			} else
				ModeSz += L",0";
			ColumnTitles[num++] = const_cast<wchar_t*>(getMsg(MHomeColumn));
		}

		//UNAME
		if (nmLen)
		{
			Mode += L",C2";
			if(nmLen < n && dizLen )
				ModeSz += L',' + boost::lexical_cast<std::wstring>(nmLen);
			else
				ModeSz += L",0";
			ColumnTitles[num++] = const_cast<wchar_t*>(getMsg(MUserColumn));
		}

		//DIZ
		if(dizLen)
		{
			Mode += L",Z";
			ModeSz += L",0";
			ColumnTitles[num] = const_cast<wchar_t*>(getMsg(MDescColumn));
		}

		PanelModesArray[2].ColumnTypes   = const_cast<wchar_t*>(Mode.c_str());
		PanelModesArray[2].ColumnWidths  = const_cast<wchar_t*>(ModeSz.c_str());
		PanelModesArray[2].ColumnTitles  = ColumnTitles;

		ColumnTitles2[0] = const_cast<wchar_t*>(getMsg(MHostColumn));
		ColumnTitles2[1] = const_cast<wchar_t*>(getMsg(MDescColumn));

		if (!dizLen)
		{
			PanelModesArray[3].ColumnTypes   = L"C0";
			PanelModesArray[3].ColumnWidths  = L"0";
		} else
		{
			PanelModesArray[3].ColumnTypes   = L"C0,Z";
			PanelModesArray[3].ColumnWidths  = const_cast<wchar_t*>(ModeSz2.c_str());
			ModeSz2 = boost::lexical_cast<std::wstring>(
				std::min( (size_t)(thisPInfo.PanelRect.right-thisPInfo.PanelRect.left)/2,hstLen)
				) + L",0";
		}
		PanelModesArray[3].ColumnTitles  = ColumnTitles2;

		pi->PanelModesArray  = PanelModesArray;
		pi->PanelModesNumber = sizeof(PanelModesArray)/sizeof(PanelModesArray[0]);
		pi->StartPanelMode   = 0;

		for ( n = 1; n <= 3; n++ ) {
			PanelModesArray[n].StatusColumnTypes   = PanelModesArray[n].ColumnTypes;
			PanelModesArray[n].StatusColumnWidths  = PanelModesArray[n].ColumnWidths;
		}

	} else {
		//FTP
		static struct PanelMode PanelModesArray[10];
		memset( PanelModesArray, 0, sizeof(PanelModesArray) );

		static wchar_t *ColumnTitles[10];
		SetTitles( ColumnTitles, getMsg(MColumn9), ARRAY_SIZE(ColumnTitles) );

		PanelModesArray[9].ColumnTypes  = const_cast<wchar_t*>(getMsg(MColumn9));
		PanelModesArray[9].ColumnWidths = const_cast<wchar_t*>(getMsg(MSizes9));
		PanelModesArray[9].ColumnTitles = ColumnTitles;
		PanelModesArray[9].FullScreen   = boost::lexical_cast<int>(getMsg(MFullScreen9));

		static wchar_t *ColumnTitles1[10];
		SetTitles( ColumnTitles1, getMsg(MColumn0), ARRAY_SIZE(ColumnTitles) );

		PanelModesArray[0].ColumnTypes  = const_cast<wchar_t*>(getMsg(MColumn0));
		PanelModesArray[0].ColumnWidths = const_cast<wchar_t*>(getMsg(MSizes0));
		PanelModesArray[0].ColumnTitles = ColumnTitles1;
		PanelModesArray[0].FullScreen   = boost::lexical_cast<int>(getMsg(MFullScreen0));

		pi->PanelModesArray  = PanelModesArray;
		pi->PanelModesNumber = sizeof(PanelModesArray)/sizeof(PanelModesArray[0]);
	}

	//---------------- KEYBAR
	static struct KeyBarTitles KeyBar;
	memset(&KeyBar,0,sizeof(KeyBar));

	KeyBar.ShiftTitles[1-1] = L"";
	KeyBar.ShiftTitles[2-1] = L"";
	KeyBar.ShiftTitles[3-1] = L"";

	KeyBar.AltTitles[6-1]   = const_cast<wchar_t*>(getMsg(MAltF6));

	if ( ShowHosts ) {
		KeyBar.ShiftTitles[1-1] = const_cast<wchar_t*>(getMsg(MShiftF1));
		KeyBar.ShiftTitles[4-1] = ShowHosts ? const_cast<wchar_t*>(getMsg(MShiftF4)):NULL;
	} else {
		KeyBar.ShiftTitles[1-1] = const_cast<wchar_t*>(getMsg(MShiftF1));
		KeyBar.ShiftTitles[7-1] = const_cast<wchar_t*>(getMsg(MShiftF7));
	}
	pi->KeyBar=&KeyBar;

	//---------------- RESTORE SCREEN
	if(CurrentState != fcsExpandList &&
		!IS_SILENT(FP_LastOpMode) )
		FARWrappers::Screen::fullRestore();

	//Back
	inside--;
}
