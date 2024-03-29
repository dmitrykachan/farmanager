/*
copy.cpp

����������� ������
*/
/*
Copyright � 1996 Eugene Roshal
Copyright � 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#pragma hdrstop

#include "copy.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "flink.hpp"
#include "dialog.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "foldtree.hpp"
#include "treelist.hpp"
#include "chgprior.hpp"
#include "scantree.hpp"
#include "constitle.hpp"
#include "filefilter.hpp"
#include "fileview.hpp"
#include "TPreRedrawFunc.hpp"
#include "syslog.hpp"
#include "TaskBar.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "colormix.hpp"
#include "message.hpp"
#include "config.hpp"
#include "stddlg.hpp"
#include "fileattr.hpp"
#include "datetime.hpp"
#include "dirinfo.hpp"
#include "pathmix.hpp"
#include "drivemix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "processname.hpp"
#include "mix.hpp"
#include "DlgGuid.hpp"
#include "console.hpp"
#include "wakeful.hpp"
#include "language.hpp"
#include "manager.hpp"

/* ����� ����� �������� ������������ */
extern long WaitUserTime;
/* ��� ����, ��� �� ����� ��� �������� ������������ ������, � remaining/speed ��� */
static long OldCalcTime;

enum
{
	SDDATA_SIZE = 64*1024,
};

enum
{
	COPY_RULE_NUL    = 0x0001,
	COPY_RULE_FILES  = 0x0002,
};

ENUM(COPY_CODES)
{
	COPY_CANCEL,
	COPY_NEXT,
	COPY_NOFILTER,                              // �� ������� �������, �.�. ���� �� ������ �� �������
	COPY_FAILURE,
	COPY_FAILUREREAD,
	COPY_SUCCESS,
	COPY_SUCCESS_MOVE,
	COPY_RETRY,
};

enum COPY_FLAGS
{
	FCOPY_COPYTONUL               = 0x00000001, // ������� ����������� � NUL
	FCOPY_CURRENTONLY             = 0x00000002, // ������ �������?
	FCOPY_ONLYNEWERFILES          = 0x00000004, // Copy only newer files
	FCOPY_OVERWRITENEXT           = 0x00000008, // Overwrite all
	FCOPY_LINK                    = 0x00000010, // �������� ������
	FCOPY_MOVE                    = 0x00000040, // �������/��������������
	FCOPY_DIZREAD                 = 0x00000080, //
	FCOPY_COPYSECURITY            = 0x00000100, // [x] Copy access rights
	FCOPY_VOLMOUNT                = 0x00000400, // �������� ������������ ����
	FCOPY_STREAMSKIP              = 0x00000800, // ������
	FCOPY_STREAMALL               = 0x00001000, // ������
	FCOPY_SKIPSETATTRFLD          = 0x00002000, // ������ �� �������� ������� �������� ��� ��������� - ����� ������ Skip All
	FCOPY_COPYSYMLINKCONTENTS     = 0x00004000, // ���������� ���������� ������������� ������?
	FCOPY_COPYPARENTSECURITY      = 0x00008000, // ����������� ������������ �����, � ������ ���� �� �� �������� ����� �������
	FCOPY_LEAVESECURITY           = 0x00010000, // Move: [?] ������ �� ������ � ������� �������
	FCOPY_DECRYPTED_DESTINATION   = 0x00020000, // ��� ������������ ������ - ��������������...
	FCOPY_USESYSTEMCOPY           = 0x00040000, // ������������ ��������� ������� �����������
	FCOPY_COPYLASTTIME            = 0x10000000, // ��� ����������� � ��������� ��������� ��������������� ��� ����������.
	FCOPY_UPDATEPPANEL            = 0x80000000, // ���������� �������� ��������� ������
};

enum COPYSECURITYOPTIONS
{
	CSO_MOVE_SETCOPYSECURITY       = 0x00000001,  // Move: �� ��������� ���������� ����� "Copy access rights"?
	CSO_MOVE_SETINHERITSECURITY    = 0x00000003,  // Move: �� ��������� ���������� ����� "Inherit access rights"?
	CSO_MOVE_SESSIONSECURITY       = 0x00000004,  // Move: ��������� ��������� "access rights" ������ ������?
	CSO_COPY_SETCOPYSECURITY       = 0x00000008,  // Copy: �� ��������� ���������� ����� "Copy access rights"?
	CSO_COPY_SETINHERITSECURITY    = 0x00000018,  // Copy: �� ��������� ���������� ����� "Inherit access rights"?
	CSO_COPY_SESSIONSECURITY       = 0x00000020,  // Copy: ��������� ��������� "access rights" ������ ������?
};


size_t TotalFiles,TotalFilesToProcess;

static clock_t CopyStartTime;

static int OrigScrX,OrigScrY;

static DWORD WINAPI CopyProgressRoutine(LARGE_INTEGER TotalFileSize,
                                        LARGE_INTEGER TotalBytesTransferred,LARGE_INTEGER StreamSize,
                                        LARGE_INTEGER StreamBytesTransferred,DWORD dwStreamNumber,
                                        DWORD dwCallbackReason,HANDLE /*hSourceFile*/,HANDLE /*hDestinationFile*/,
                                        LPVOID lpData);

static unsigned __int64 TotalCopySize, TotalCopiedSize; // ����� ��������� �����������
static unsigned __int64 CurCopiedSize;                  // ������� ��������� �����������
static unsigned __int64 TotalSkippedSize;               // ����� ������ ����������� ������
static unsigned __int64 TotalCopiedSizeEx;
static size_t   CountTarget;                    // ����� �����.
static int CopySecurityCopy=-1;
static int CopySecurityMove=-1;
static bool ShowTotalCopySize;

static FileFilter *Filter;
static int UseFilter=FALSE;

static BOOL ZoomedState,IconicState;

enum enumShellCopy
{
	ID_SC_TITLE,
	ID_SC_TARGETTITLE,
	ID_SC_TARGETEDIT,
	ID_SC_SEPARATOR1,
	ID_SC_ACTITLE,
	ID_SC_ACLEAVE,
	ID_SC_ACCOPY,
	ID_SC_ACINHERIT,
	ID_SC_SEPARATOR2,
	ID_SC_COMBOTEXT,
	ID_SC_COMBO,
	ID_SC_COPYSYMLINK,
	ID_SC_MULTITARGET,
	ID_SC_SEPARATOR3,
	ID_SC_USEFILTER,
	ID_SC_SEPARATOR4,
	ID_SC_BTNCOPY,
	ID_SC_BTNTREE,
	ID_SC_BTNFILTER,
	ID_SC_BTNCANCEL,
	ID_SC_SOURCEFILENAME,
};

enum CopyMode
{
	CM_ASK,
	CM_OVERWRITE,
	CM_SKIP,
	CM_RENAME,
	CM_APPEND,
	CM_ONLYNEWER,
	CM_SEPARATOR,
	CM_ASKRO,
};

// CopyProgress start
// ����� ��� ������ � ��������� ���� ����� ������� ���� ���������� ���������� ������
class CopyProgress
{
		ConsoleTitle CopyTitle;
		IndeterminateTaskBar TB;
		wakeful W;
		SMALL_RECT Rect;
		string Bar;
		size_t BarSize;
		bool Move,Total,m_Time;
		bool BgInit,ScanBgInit;
		bool IsCancelled;
		FarColor Color;
		int Percents;
		time_check TimeCheck;
		FormatString strSrc,strDst;
		string strTime;
		LangString strFiles;
		void Flush();
		void DrawNames();
		void CreateScanBackground();
		void SetProgress(bool TotalProgress,UINT64 CompletedSize,UINT64 TotalSize);
	public:
		CopyProgress(bool Move,bool Total,bool Time);
		void CreateBackground();
		bool Cancelled() const {return IsCancelled;}
		void SetScanName(const string& Name);
		void SetNames(const string& Src,const string& Dst);
		void SetProgressValue(UINT64 CompletedSize,UINT64 TotalSize) {return SetProgress(false,CompletedSize,TotalSize);}
		void SetTotalProgressValue(UINT64 CompletedSize,UINT64 TotalSize) {return SetProgress(true,CompletedSize,TotalSize);}

		// BUGBUG
		string strTotalCopySizeText;
		time_check SecurityTimeCheck;
};

static void GetTimeText(DWORD Time,string &strTimeText)
{
	DWORD Sec=Time;
	DWORD Min=Sec/60;
	Sec-=(Min*60);
	DWORD Hour=Min/60;
	Min-=(Hour*60);
	strTimeText = FormatString() << fmt::ExactWidth(2) << fmt::FillChar(L'0') << Hour << L":" << fmt::ExactWidth(2) << fmt::FillChar(L'0') << Min << L":" << fmt::ExactWidth(2) << fmt::FillChar(L'0') << Sec;
}

void CopyProgress::Flush()
{
	if (TimeCheck)
	{
		if (!IsCancelled)
		{
			if (CheckForEscSilent())
			{
				Global->CtrlObject->Cp()->Lock();
				IsCancelled=ConfirmAbortOp()!=0;
				Global->CtrlObject->Cp()->Unlock();
			}
		}

		if (Total || (TotalFilesToProcess==1))
		{
			CopyTitle << L"{" << (Total?ToPercent(TotalCopiedSize>>8,TotalCopySize>>8):Percents) << L"%} " << MSG(Move? MCopyMovingTitle : MCopyCopyingTitle) << fmt::Flush();
		}
	}
}

CopyProgress::CopyProgress(bool Move,bool Total,bool Time):
	Rect(),
	BarSize(52),
	Move(Move),
	Total(Total),
	m_Time(Time),
	BgInit(false),
	ScanBgInit(false),
	IsCancelled(false),
	Color(colors::PaletteColorToFarColor(COL_DIALOGTEXT)),
	Percents(0),
	TimeCheck(time_check::immediate, GetRedrawTimeout()),
	SecurityTimeCheck(time_check::immediate, GetRedrawTimeout())
{
}

void CopyProgress::SetScanName(const string& Name)
{
	if (!ScanBgInit)
	{
		CreateScanBackground();
	}

	GotoXY(Rect.Left+5,Rect.Top+3);
	Global->FS << fmt::LeftAlign()<<fmt::ExactWidth(Rect.Right-Rect.Left-9)<<Name;
	Flush();
}

void CopyProgress::CreateScanBackground()
{
	Bar = make_progressbar(BarSize, 0, false, false);
	Message m(MSG_LEFTALIGN, MSG(Move? MMoveDlgTitle : MCopyDlgTitle), make_vector<string>(MSG(MCopyScanning), Bar), std::vector<string>());
	int MX1,MY1,MX2,MY2;
	m.GetMessagePosition(MX1,MY1,MX2,MY2);
	Rect.Left=MX1;
	Rect.Right=MX2;
	Rect.Top=MY1;
	Rect.Bottom=MY2;
	ScanBgInit=true;
}

void CopyProgress::CreateBackground()
{
	Bar = make_progressbar(BarSize, 0, false, false);

	std::vector<string> Items;
	const wchar_t* Title;
	string strTotalSeparator(L"\x1 ");

	if (!Total)
	{
		if (!m_Time)
		{
			Title = MSG(Move? MMoveDlgTitle : MCopyDlgTitle);
			Items = make_vector<string>(
				MSG(Move? MCopyMoving : MCopyCopying),
				L"",
				MSG(MCopyTo),
				L"",
				Bar,
				L"\x1",
				L"");
		}
		else
		{
			Title = MSG(Move? MMoveDlgTitle : MCopyDlgTitle);
			Items = make_vector<string>(
				MSG(Move? MCopyMoving : MCopyCopying),
				L"",
				MSG(MCopyTo),
				L"",
				Bar,
				L"\x1",
				L"",
				L"\x1",
				L"");
		}
	}
	else
	{
		strTotalSeparator+=MSG(MCopyDlgTotal);
		strTotalSeparator+=L": ";
		strTotalSeparator+=strTotalCopySizeText;
		strTotalSeparator+=L" ";

		if (!m_Time)
		{
			Title = MSG(Move? MMoveDlgTitle : MCopyDlgTitle);
			Items = make_vector<string>(MSG(Move? MCopyMoving : MCopyCopying),
				L"",
				MSG(MCopyTo),
				L"",
				Bar,
				strTotalSeparator,
				Bar,
				L"\x1",
				L"");
		}
		else
		{
			Title = MSG(Move? MMoveDlgTitle : MCopyDlgTitle),
			Items = make_vector<string>(MSG(Move? MCopyMoving : MCopyCopying),
				L"",
				MSG(MCopyTo),
				L"",
				Bar,
				strTotalSeparator,
				Bar,
				L"\x1",
				L"",
				L"\x1",
				L"");
		}
	}
	Message m(MSG_LEFTALIGN, Title, Items, std::vector<string>());

	int MX1,MY1,MX2,MY2;
	m.GetMessagePosition(MX1,MY1,MX2,MY2);
	Rect.Left=MX1;
	Rect.Right=MX2;
	Rect.Top=MY1;
	Rect.Bottom=MY2;
	BgInit=true;
	DrawNames();
}

void CopyProgress::DrawNames()
{
	Text(Rect.Left+5,Rect.Top+3,Color,strSrc);
	Text(Rect.Left+5,Rect.Top+5,Color,strDst);
	Text(Rect.Left+5,Rect.Top+(Total?10:8),Color,strFiles);
}

void CopyProgress::SetNames(const string& Src,const string& Dst)
{
	if (!BgInit)
	{
		CreateBackground();
	}

	if (m_Time)
	{
		if (!ShowTotalCopySize || 0 == TotalFiles)
		{
			CopyStartTime = clock();
			WaitUserTime = OldCalcTime = 0;
		}
	}

	const int NameWidth = Rect.Right-Rect.Left-9;
	string tmp(Src);
	TruncPathStr(tmp, NameWidth);
	strSrc.clear();
	strSrc<<fmt::LeftAlign()<<fmt::ExactWidth(NameWidth)<<tmp;
	tmp = Dst;
	TruncPathStr(tmp, NameWidth);
	strDst.clear();
	strDst<<fmt::LeftAlign()<<fmt::ExactWidth(NameWidth)<<tmp;

	if (Total)
	{
		strFiles = MCopyProcessedTotal;
		strFiles << TotalFiles << TotalFilesToProcess;
	}
	else
	{
		strFiles = MCopyProcessed;
		strFiles << TotalFiles;
	}

	DrawNames();
	Flush();
}

void CopyProgress::SetProgress(bool TotalProgress,UINT64 CompletedSize,UINT64 TotalSize)
{
	if (!BgInit)
	{
		CreateBackground();
	}

	UINT64 OldCompletedSize = CompletedSize;
	UINT64 OldTotalSize = TotalSize;
	CompletedSize>>=8;
	TotalSize>>=8;
	CompletedSize=std::min(CompletedSize,TotalSize);
	COORD BarCoord={static_cast<SHORT>(Rect.Left+5),static_cast<SHORT>(Rect.Top+(TotalProgress?8:6))};
	size_t BarLength = Rect.Right - Rect.Left - 9;

	Percents = ToPercent(CompletedSize, TotalSize);
	Bar = make_progressbar(BarLength, Percents, true, Total == TotalProgress);

	Text(BarCoord.X,BarCoord.Y,Color,Bar);

	if (m_Time&&(!Total||TotalProgress))
	{
		auto WorkTime = clock() - CopyStartTime;
		UINT64 SizeLeft=(OldTotalSize>OldCompletedSize)?(OldTotalSize-OldCompletedSize):0;
		long CalcTime=OldCalcTime;

		if (WaitUserTime!=-1) // -1 => ��������� � �������� �������� ������ �����
		{
			OldCalcTime=CalcTime=WorkTime-WaitUserTime;
		}

		WorkTime /= CLOCKS_PER_SEC;
		CalcTime /= CLOCKS_PER_SEC;

		if (!WorkTime)
		{
			strTime = LangString(MCopyTimeInfo) << L"        " << L"        " << L"        ";
		}
		else
		{
			if (TotalProgress)
			{
				OldCompletedSize=OldCompletedSize-TotalSkippedSize;
			}

			UINT64 CPS=CalcTime?OldCompletedSize/CalcTime:0;
			DWORD TimeLeft=static_cast<DWORD>(CPS?SizeLeft/CPS:0);
			string strSpeed;
			FileSizeToStr(strSpeed,CPS,8,COLUMN_FLOATSIZE|COLUMN_COMMAS);
			string strWorkTimeStr,strTimeLeftStr;
			GetTimeText(WorkTime,strWorkTimeStr);
			GetTimeText(TimeLeft,strTimeLeftStr);
			if(!strSpeed.empty() && strSpeed.front() == L' ' && strSpeed.back() >= L'0' && strSpeed.back() <= L'9')
			{
				strSpeed.erase(0, 1);
				strSpeed+=L" ";
			}
			;
			string tmp[3];
			tmp[0] = FormatString() << fmt::ExactWidth(8) << strWorkTimeStr;
			tmp[1] = FormatString() << fmt::ExactWidth(8) << strTimeLeftStr;
			tmp[2] = FormatString() << fmt::ExactWidth(8) << strSpeed;
			strTime = LangString(MCopyTimeInfo) << tmp[0] << tmp[1] << tmp[2];
		}

		Text(Rect.Left+5,Rect.Top+(Total?12:10),Color,strTime);
	}

	Flush();
}
// CopyProgress end



/* $ 25.05.2002 IS
 + ������ �������� � ��������� _��������_ �������, � ���������� ����
   ������������� ��������, �����
   Src="D:\Program Files\filename"
   Dest="D:\PROGRA~1\filename"
   ("D:\PROGRA~1" - �������� ��� ��� "D:\Program Files")
   ���������, ��� ����� ���� ����������, � ������ ���������,
   ��� ��� ������ (������� �� �����, ��� � � ������, � �� ������ ������
   ���� ���� � ��� ��)
 ! ����������� - "���������" ������� �� DeleteEndSlash
 ! ������� ��� ���������������� �� �������� ���� � ������
   ��������� �� ������� �����, ������ ��� ��� ����� ������ ������ ���
   ��������������, � ������� ���������� � ��� ����������� ����.
   ������ ������� ������ 1, ��� ������ ���� src=path\filename,
   dest=path\filename (������ ���������� 2 - �.�. ������ �� ������).
*/

int CmpFullNames(const string& Src,const string& Dest)
{
	string strSrcFullName, strDestFullName;

	// ������� ������ ���� � ������ ������������� ������
	// (ConvertNameToReal eliminates short names too)
	ConvertNameToReal(Src, strSrcFullName);
	ConvertNameToReal(Dest, strDestFullName);
	DeleteEndSlash(strSrcFullName);
	DeleteEndSlash(strDestFullName);

	return !StrCmpI(strSrcFullName, strDestFullName);
}

bool CheckNulOrCon(const wchar_t *Src)
{
	if (HasPathPrefix(Src))
		Src+=4;

	return (!StrCmpNI(Src,L"nul",3) || !StrCmpNI(Src,L"con",3)) && (IsSlash(Src[3])||!Src[3]);
}

string& GetParentFolder(const string& Src, string &strDest)
{
	ConvertNameToReal(Src,strDest);
	CutToSlash(strDest,true);
	return strDest;
}

int CmpFullPath(const string& Src, const string& Dest)
{
	string strSrcFullName, strDestFullName;

	GetParentFolder(Src, strSrcFullName);
	GetParentFolder(Dest, strDestFullName);
	DeleteEndSlash(strSrcFullName);
	DeleteEndSlash(strDestFullName);

	// ��������� �� �������� ����
	ConvertNameToReal(strSrcFullName, strSrcFullName);
	ConvertNameToReal(strDestFullName, strDestFullName);

	return !StrCmpI(strSrcFullName, strDestFullName);
}

static void GenerateName(string &strName,const wchar_t *Path=nullptr)
{
	if (Path&&*Path)
	{
		string strTmp=Path;
		AddEndSlash(strTmp);
		strTmp+=PointToName(strName);
		strName=strTmp;
	}

	string strExt=PointToExt(strName);
	size_t NameLength=strName.size()-strExt.size();

	// file (2).ext, file (3).ext and so on
	for (int i = 2; os::fs::exists(strName); ++i)
	{
		strName.resize(NameLength);
		strName += L" (" + std::to_wstring(i) + L")";
		strName+=strExt;
	}
}

static void PR_ShellCopyMsg();

struct CopyPreRedrawItem : public PreRedrawItem
{
	CopyPreRedrawItem():
		PreRedrawItem(PR_ShellCopyMsg),
		CP()
	{}

	CopyProgress* CP;
};

static void PR_ShellCopyMsg()
{
	if (!PreRedrawStack().empty())
	{
		auto item = dynamic_cast<CopyPreRedrawItem*>(PreRedrawStack().top());
		item->CP->CreateBackground();
	}
}

BOOL CheckAndUpdateConsole(BOOL IsChangeConsole)
{
	HWND hWnd = Console().GetWindow();
	BOOL curZoomedState=IsZoomed(hWnd);
	BOOL curIconicState=IsIconic(hWnd);

	if (ZoomedState!=curZoomedState && IconicState==curIconicState)
	{
		ZoomedState=curZoomedState;
		ChangeVideoMode(ZoomedState);
		auto Window = Global->WindowManager->GetBottomWindow();
		int LockCount=-1;

		while (Window->Locked())
		{
			LockCount++;
			Window->Unlock();
		}

		Global->WindowManager->ResizeAllWindows();
		Global->WindowManager->PluginCommit();

		while (LockCount > 0)
		{
			Window->Lock();
			LockCount--;
		}

		IsChangeConsole=TRUE;
	}

	return IsChangeConsole;
}

enum
{
	DM_CALLTREE = DM_USER+1,
	DM_SWITCHRO = DM_USER+2,
};

intptr_t ShellCopy::CopyDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2)
{
	switch (Msg)
	{
		case DN_INITDIALOG:
			Dlg->SendMessage(DM_SETCOMBOBOXEVENT,ID_SC_COMBO,ToPtr(CBET_KEY|CBET_MOUSE));
			Dlg->SendMessage(DM_SETMOUSEEVENTNOTIFY,TRUE,0);
			break;
		case DM_SWITCHRO:
		{
			FarListGetItem LGI={sizeof(FarListGetItem),CM_ASKRO};
			Dlg->SendMessage(DM_LISTGETITEM,ID_SC_COMBO,&LGI);

			if (LGI.Item.Flags&LIF_CHECKED)
				LGI.Item.Flags&=~LIF_CHECKED;
			else
				LGI.Item.Flags|=LIF_CHECKED;

			Dlg->SendMessage(DM_LISTUPDATE,ID_SC_COMBO,&LGI);
			Dlg->SendMessage(DM_REDRAW,0,0);
			return TRUE;
		}
		case DN_BTNCLICK:
		{
			if (Param1==ID_SC_USEFILTER) // "Use filter"
			{
				UseFilter=static_cast<int>(reinterpret_cast<intptr_t>(Param2));
				return TRUE;
			}

			if (Param1 == ID_SC_BTNTREE) // Tree
			{
				Dlg->SendMessage(DM_CALLTREE,0,0);
				return FALSE;
			}
			else if (Param1 == ID_SC_BTNCOPY)
			{
				Dlg->SendMessage(DM_CLOSE,ID_SC_BTNCOPY,0);
			}
			/*
			else if(Param1 == ID_SC_ONLYNEWER && ((Flags)&FCOPY_LINK))
			{
			  // ����������� ��� ����� �������� ������������ � ������ ����� :-))
			  		Dlg->SendMessage(DN_EDITCHANGE,ID_SC_TARGETEDIT,0);
			}
			*/
			else if (Param1==ID_SC_BTNFILTER) // Filter
			{
				Filter->FilterEdit();
				return TRUE;
			}

			break;
		}
		case DN_CONTROLINPUT: // �� ������ ������!
		{
			const INPUT_RECORD* record=(const INPUT_RECORD *)Param2;
			if (record->EventType==KEY_EVENT)
			{
				int key = InputRecordToKey(record);
				if (key == KEY_ALTF10 || key == KEY_RALTF10 || key == KEY_F10 || key == KEY_SHIFTF10)
				{
					AltF10=(key == KEY_ALTF10 || key == KEY_RALTF10)?1:(key == KEY_SHIFTF10?2:0);
					Dlg->SendMessage(DM_CALLTREE,AltF10,0);
					return TRUE;
				}

				if (Param1 == ID_SC_COMBO)
				{
					if (key==KEY_ENTER || key==KEY_NUMENTER || key==KEY_INS || key==KEY_NUMPAD0 || key==KEY_SPACE)
					{
						if (Dlg->SendMessage(DM_LISTGETCURPOS,ID_SC_COMBO,0)==CM_ASKRO)
							return Dlg->SendMessage(DM_SWITCHRO,0,0);
					}
				}
			}
		}
		break;

		case DN_LISTHOTKEY:
			if(Param1==ID_SC_COMBO)
			{
				if (Dlg->SendMessage(DM_LISTGETCURPOS,ID_SC_COMBO,0)==CM_ASKRO)
				{
					Dlg->SendMessage(DM_SWITCHRO,0,0);
					return TRUE;
				}
			}
			break;
		case DN_INPUT:

			if (Dlg->SendMessage(DM_GETDROPDOWNOPENED,ID_SC_COMBO,0))
			{
				auto& mer = reinterpret_cast<INPUT_RECORD*>(Param2)->Event.MouseEvent;

				if (Dlg->SendMessage(DM_LISTGETCURPOS, ID_SC_COMBO, 0) == CM_ASKRO && mer.dwButtonState && !(mer.dwEventFlags & MOUSE_MOVED))
				{
					Dlg->SendMessage(DM_SWITCHRO,0,0);
					return FALSE;
				}
			}

			break;
		case DM_CALLTREE:
		{
			/* $ 13.10.2001 IS
			   + ��� ����������������� ��������� ��������� � "������" ������� � ���
			     ������������� ������ ����� ����� � �������.
			   - ���: ��� ����������������� ��������� � "������" ������� ��
			     ���������� � �������, ���� �� �������� � �����
			     ����� �������-�����������.
			   - ���: ����������� �������� Shift-F10, ���� ������ ����� ���������
			     ���� �� �����.
			   - ���: ����������� �������� Shift-F10 ��� ����������������� -
			     ����������� �������� �������, ������ ������������ ����� ������ �������
			     � ������.
			*/
			BOOL MultiCopy=Dlg->SendMessage(DM_GETCHECK,ID_SC_MULTITARGET,0)==BSTATE_CHECKED;
			string strOldFolder = reinterpret_cast<const wchar_t*>(Dlg->SendMessage(DM_GETCONSTTEXTPTR, ID_SC_TARGETEDIT, 0));
			string strNewFolder;

			if (AltF10 == 2)
			{
				strNewFolder = strOldFolder;

				if (MultiCopy)
				{
					std::vector<string> DestList;
					split(DestList, strOldFolder, STLF_UNIQUE);

					if (!DestList.empty())
						strNewFolder = DestList.front();
				}

				if (strNewFolder.empty())
					AltF10=-1;
				else // ������� ������ ����
					DeleteEndSlash(strNewFolder);
			}

			if (AltF10 != -1)
			{
				{
					string strNewFolder2 = strNewFolder;
					FolderTree::create(strNewFolder2,
					                (AltF10==1?MODALTREE_PASSIVE:
					                 (AltF10==2?MODALTREE_FREE:
					                  MODALTREE_ACTIVE)),
					                FALSE, false);
					strNewFolder = strNewFolder2;
				}

				if (!strNewFolder.empty())
				{
					AddEndSlash(strNewFolder);

					if (MultiCopy) // �����������������
					{
						// ������� �������, ���� ��� �������� �������� �������-�����������
						if (strNewFolder.find_first_of(L";,") != string::npos)
							InsertQuote(strNewFolder);

						if (strOldFolder.size())
							strOldFolder += L";"; // ������� ����������� � ��������� ������

						strOldFolder += strNewFolder;
						strNewFolder = strOldFolder;
					}

					Dlg->SendMessage(DM_SETTEXTPTR,ID_SC_TARGETEDIT, UNSAFE_CSTR(strNewFolder));
					Dlg->SendMessage(DM_SETFOCUS,ID_SC_TARGETEDIT,0);
				}
			}

			AltF10=0;
			return TRUE;
		}
		case DN_CLOSE:
		{
			if (Param1==ID_SC_BTNCOPY)
			{
				FarListGetItem LGI={sizeof(FarListGetItem),CM_ASKRO};
				Dlg->SendMessage(DM_LISTGETITEM,ID_SC_COMBO,&LGI);

				if (LGI.Item.Flags&LIF_CHECKED)
					AskRO = true;
			}
		}
		break;

		default:
			break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

ShellCopy::ShellCopy(Panel *SrcPanel,        // �������� ������ (��������)
                     int Move,               // =1 - �������� Move
                     int Link,               // =1 - Sym/Hard Link
                     int CurrentOnly,        // =1 - ������ ������� ����, ��� ��������
                     int Ask,                // =1 - �������� ������?
                     int &ToPlugin,          // =?
                     const wchar_t* PluginDestPath,
                     bool ToSubdir):
	Flags((Move? FCOPY_MOVE : 0) | (Link? FCOPY_LINK : 0) | (CurrentOnly? FCOPY_CURRENTONLY : 0)),
	SrcPanel(SrcPanel),
	DestPanel(Global->CtrlObject->Cp()->GetAnotherPanel(SrcPanel)),
	SrcPanelMode(SrcPanel->GetMode()),
	DestPanelMode(ToPlugin? DestPanel->GetMode() : NORMAL_PANEL),
	SrcDriveType(),
	DestDriveType(),
	CopyBufferSize(Global->Opt->CMOpt.BufferSize),
	SelectedFolderNameLength(),
	RPT(RP_EXACTCOPY),
	AltF10(),
	m_CopySecurity(),
	m_FileAttr(),
	FolderPresent(),
	FilesPresent(),
	AskRO()
{
	Filter=nullptr;

	if (!(SelCount=SrcPanel->GetSelCount()))
		return;

	string strSelName;

	if (SelCount==1)
	{
		SrcPanel->GetSelName(nullptr,m_FileAttr); //????
		SrcPanel->GetSelName(&strSelName,m_FileAttr);

		if (TestParentFolderName(strSelName))
			return;
	}

	ZoomedState=IsZoomed(Console().GetWindow());
	IconicState=IsIconic(Console().GetWindow());
	// �������� ������ �������
	Filter=new FileFilter(SrcPanel, FFT_COPY);
	// $ 26.05.2001 OT ��������� ����������� ������� �� ����� �����������
	Global->CtrlObject->Cp()->Lock();
	ShowTotalCopySize=Global->Opt->CMOpt.CopyShowTotal!=0;
	int DestPlugin=ToPlugin;
	ToPlugin=FALSE;

	// ***********************************************************************
	// *** Prepare Dialog Controls
	// ***********************************************************************
	int DLG_HEIGHT=16, DLG_WIDTH=76;

	FarDialogItem CopyDlgData[]=
	{
		{DI_DOUBLEBOX,   3, 1,DLG_WIDTH-4,DLG_HEIGHT-2,0,nullptr,nullptr,0,MSG(MCopyDlgTitle)},
		{DI_TEXT,        5, 2, 0, 2,0,nullptr,nullptr,0,MSG(Link?MCMLTargetIN:MCMLTargetTO)},
		{DI_EDIT,        5, 3,70, 3,0,L"Copy",nullptr,DIF_FOCUS|DIF_HISTORY|DIF_EDITEXPAND|DIF_USELASTHISTORY|DIF_EDITPATH,L""},
		{DI_TEXT,       -1, 4, 0, 4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_TEXT,        5, 5, 0, 5,0,nullptr,nullptr,0,MSG(MCopySecurity)},
		{DI_RADIOBUTTON, 5, 5, 0, 5,0,nullptr,nullptr,DIF_GROUP,MSG(MCopySecurityLeave)},
		{DI_RADIOBUTTON, 5, 5, 0, 5,0,nullptr,nullptr,0,MSG(MCopySecurityCopy)},
		{DI_RADIOBUTTON, 5, 5, 0, 5,0,nullptr,nullptr,0,MSG(MCopySecurityInherit)},
		{DI_TEXT,       -1, 6, 0, 6,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_TEXT,        5, 7, 0, 7,0,nullptr,nullptr,0,MSG(MCopyIfFileExist)},
		{DI_COMBOBOX,   29, 7,70, 7,0,nullptr,nullptr,DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND|DIF_LISTWRAPMODE,L""},
		{DI_CHECKBOX,    5, 8, 0, 8,0,nullptr,nullptr,0,MSG(MCopySymLinkContents)},
		{DI_CHECKBOX,    5, 9, 0, 9,0,nullptr,nullptr,0,MSG(MCopyMultiActions)},
		{DI_TEXT,       -1,10, 0,10,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_CHECKBOX,    5,11, 0,11,(int)(UseFilter?BSTATE_CHECKED:BSTATE_UNCHECKED),nullptr,nullptr,DIF_AUTOMATION,MSG(MCopyUseFilter)},
		{DI_TEXT,       -1,12, 0,12,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,      0,13, 0,13,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MCopyDlgCopy)},
		{DI_BUTTON,      0,13, 0,13,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_BTNNOCLOSE,MSG(MCopyDlgTree)},
		{DI_BUTTON,      0,13, 0,13,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_BTNNOCLOSE|DIF_AUTOMATION|(UseFilter?0:DIF_DISABLE),MSG(MCopySetFilter)},
		{DI_BUTTON,      0,13, 0,13,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCopyDlgCancel)},
		{DI_TEXT,        5, 2, 0, 2,0,nullptr,nullptr,DIF_SHOWAMPERSAND,L""},
	};
	auto CopyDlg = MakeDialogItemsEx(CopyDlgData);
	CopyDlg[ID_SC_MULTITARGET].Selected=Global->Opt->CMOpt.MultiCopy;
	{
		const wchar_t *Str = MSG(MCopySecurity);
		CopyDlg[ID_SC_ACLEAVE].X1 = CopyDlg[ID_SC_ACTITLE].X1 + wcslen(Str) - (wcschr(Str, L'&')?1:0) + 1;
		Str = MSG(MCopySecurityLeave);
		CopyDlg[ID_SC_ACCOPY].X1 = CopyDlg[ID_SC_ACLEAVE].X1 + wcslen(Str) - (wcschr(Str, L'&') ? 1 : 0) + 5;
		Str = MSG(MCopySecurityCopy);
		CopyDlg[ID_SC_ACINHERIT].X1 = CopyDlg[ID_SC_ACCOPY].X1 + wcslen(Str) - (wcschr(Str, L'&') ? 1 : 0) + 5;
	}

	if (Link)
	{
		CopyDlg[ID_SC_COMBOTEXT].strData=MSG(MLinkType);
		CopyDlg[ID_SC_COPYSYMLINK].Selected=0;
		CopyDlg[ID_SC_COPYSYMLINK].Flags|=DIF_DISABLE|DIF_HIDDEN;
		m_CopySecurity=1;
	}
	else if (Move) // ������ ��� �������
	{
		CopyDlg[ID_SC_MULTITARGET].Selected = 0;
		CopyDlg[ID_SC_MULTITARGET].Flags |= DIF_DISABLE;

		//   2 - Default
		//   1 - Copy access rights
		//   0 - Inherit access rights
		m_CopySecurity=2;

		// ������� ����� "Inherit access rights"?
		// CSO_MOVE_SETINHERITSECURITY - ���������� ����
		if ((Global->Opt->CMOpt.CopySecurityOptions&CSO_MOVE_SETINHERITSECURITY) == CSO_MOVE_SETINHERITSECURITY)
			m_CopySecurity=0;
		else if (Global->Opt->CMOpt.CopySecurityOptions&CSO_MOVE_SETCOPYSECURITY)
			m_CopySecurity=1;

		// ������ ���������� �����������?
		if (CopySecurityMove != -1 && (Global->Opt->CMOpt.CopySecurityOptions&CSO_MOVE_SESSIONSECURITY))
			m_CopySecurity=CopySecurityMove;
		else
			CopySecurityMove=m_CopySecurity;
	}
	else // ������ ��� �����������
	{
		//   2 - Default
		//   1 - Copy access rights
		//   0 - Inherit access rights
		m_CopySecurity=2;

		// ������� ����� "Inherit access rights"?
		// CSO_COPY_SETINHERITSECURITY - ���������� ����
		if ((Global->Opt->CMOpt.CopySecurityOptions&CSO_COPY_SETINHERITSECURITY) == CSO_COPY_SETINHERITSECURITY)
			m_CopySecurity=0;
		else if (Global->Opt->CMOpt.CopySecurityOptions&CSO_COPY_SETCOPYSECURITY)
			m_CopySecurity=1;

		// ������ ���������� �����������?
		if (CopySecurityCopy != -1 && Global->Opt->CMOpt.CopySecurityOptions&CSO_COPY_SESSIONSECURITY)
			m_CopySecurity=CopySecurityCopy;
		else
			CopySecurityCopy=m_CopySecurity;
	}

	// ��� ������ ����������
	if (m_CopySecurity)
	{
		if (m_CopySecurity == 1)
		{
			Flags|=FCOPY_COPYSECURITY;
			CopyDlg[ID_SC_ACCOPY].Selected=1;
		}
		else
		{
			Flags|=FCOPY_LEAVESECURITY;
			CopyDlg[ID_SC_ACLEAVE].Selected=1;
		}
	}
	else
	{
		Flags&=~(FCOPY_COPYSECURITY|FCOPY_LEAVESECURITY);
		CopyDlg[ID_SC_ACINHERIT].Selected=1;
	}

	string strCopyStr;

	if (SelCount==1)
	{
		if (SrcPanel->GetType()==TREE_PANEL)
		{
			string strNewDir(strSelName);
			size_t pos;

			if (FindLastSlash(pos,strNewDir))
			{
				strNewDir.resize(pos);

				if (!pos || strNewDir[pos-1]==L':')
					strNewDir += L"\\";

				FarChDir(strNewDir);
			}
		}

		string strSelNameShort(strSelName);
		QuoteLeadingSpace(strSelNameShort);
		strCopyStr=MSG(Move?MMoveFile:(Link?MLinkFile:MCopyFile));
		TruncPathStr(strSelNameShort,static_cast<int>(CopyDlg[ID_SC_TITLE].X2-CopyDlg[ID_SC_TITLE].X1-strCopyStr.size()-7));
		strCopyStr+=L" "+strSelNameShort;

		// ���� �������� ��������� ����, �� ��������� ������������ ������
		if (!(m_FileAttr&FILE_ATTRIBUTE_DIRECTORY))
		{
			CopyDlg[ID_SC_USEFILTER].Selected=0;
			CopyDlg[ID_SC_USEFILTER].Flags|=DIF_DISABLE;
		}
	}
	else // �������� ���������!
	{

		// ��������� ����� - ��� ���������
		FormatString StrItems;
		StrItems<<SelCount;
		size_t LenItems=StrItems.size();
		LNGID NItems=MCMLItemsA;

		if (LenItems > 0)
		{
			if ((LenItems >= 2 && StrItems[LenItems-2] == '1') ||
			        StrItems[LenItems-1] >= '5' ||
			        StrItems[LenItems-1] == '0')
				NItems=MCMLItemsS;
			else if (StrItems[LenItems-1] == '1')
				NItems=MCMLItems0;
		}
		strCopyStr = LangString(Move? MMoveFiles : (Link? MLinkFiles : MCopyFiles)) << SelCount << MSG(NItems);
	}

	CopyDlg[ID_SC_SOURCEFILENAME].strData=strCopyStr;
	CopyDlg[ID_SC_TITLE].strData = MSG(Move?MMoveDlgTitle :(Link?MLinkDlgTitle:MCopyDlgTitle));
	CopyDlg[ID_SC_BTNCOPY].strData = MSG(Move?MCopyDlgRename:(Link?MCopyDlgLink:MCopyDlgCopy));

	if (DestPanelMode == PLUGIN_PANEL)
	{
		// ���� ��������������� ������ - ������, �� �������� OnlyNewer //?????
/*
		CopySecurity=2;
		CopyDlg[ID_SC_ACCOPY].Selected=0;
		CopyDlg[ID_SC_ACINHERIT].Selected=0;
		CopyDlg[ID_SC_ACLEAVE].Selected=1;
		CopyDlg[ID_SC_ACCOPY].Flags|=DIF_DISABLE;
		CopyDlg[ID_SC_ACINHERIT].Flags|=DIF_DISABLE;
		CopyDlg[ID_SC_ACLEAVE].Flags|=DIF_DISABLE;
*/
	}

	string strDestDir(DestPanel->GetCurDir());
	if(ToSubdir)
	{
		AddEndSlash(strDestDir);
		string strSubdir, strShort;
		DestPanel->GetCurName(strSubdir, strShort);
		strDestDir+=strSubdir;
	}
	string strSrcDir(SrcPanel->GetCurDir());

	if (CurrentOnly)
	{
		//   ��� ����������� ������ �������� ��� �������� ����� ��� ��� � �������, ���� ��� �������� �����������.
		CopyDlg[ID_SC_TARGETEDIT].strData = strSelName;

		if (!Move && CopyDlg[ID_SC_TARGETEDIT].strData.find_first_of(L",;") != string::npos)
		{
			// ������ ��� ������ �������
			// ������� � �������, �.�. ����� ���� �����������
			InsertQuote(Unquote(CopyDlg[ID_SC_TARGETEDIT].strData));
		}
	}
	else
	{
		switch (DestPanelMode)
		{
			case NORMAL_PANEL:
			{
				if ((strDestDir.empty() || !DestPanel->IsVisible() || !StrCmpI(strSrcDir, strDestDir)) && SelCount==1)
					CopyDlg[ID_SC_TARGETEDIT].strData = strSelName;
				else
				{
					CopyDlg[ID_SC_TARGETEDIT].strData = strDestDir;
					AddEndSlash(CopyDlg[ID_SC_TARGETEDIT].strData);
				}

				/* $ 19.07.2003 IS
				   ���� ���� �������� �����������, �� ������� �� � �������, ���� �� ��������
				   ������ ��� F5, Enter � �������, ����� ������������ ������� MultiCopy
				*/
				if (!Move && CopyDlg[ID_SC_TARGETEDIT].strData.find_first_of(L",;") != string::npos)
				{
					// ������ ��� ������ �������
					// ������� � �������, �.�. ����� ���� �����������
					InsertQuote(Unquote(CopyDlg[ID_SC_TARGETEDIT].strData));
				}

				break;
			}

			case PLUGIN_PANEL:
			{
				OpenPanelInfo Info;
				DestPanel->GetOpenPanelInfo(&Info);
				string strFormat = NullToEmpty(Info.Format);
				CopyDlg[ID_SC_TARGETEDIT].strData = strFormat+L":";

				while (CopyDlg[ID_SC_TARGETEDIT].strData.size()<2)
					CopyDlg[ID_SC_TARGETEDIT].strData += L":";

				strPluginFormat = CopyDlg[ID_SC_TARGETEDIT].strData;
				ToUpper(strPluginFormat);
				break;
			}
		}
	}

	string strInitDestDir = CopyDlg[ID_SC_TARGETEDIT].strData;
	// ��� �������
	os::FAR_FIND_DATA fd;
	SrcPanel->GetSelName(nullptr,m_FileAttr);

	bool AddSlash=false;

	while (SrcPanel->GetSelName(&strSelName,m_FileAttr,nullptr,&fd))
	{
		if (UseFilter)
		{
			if (!Filter->FileInFilter(fd, nullptr, &fd.strFileName))
				continue;
		}

		if (m_FileAttr & FILE_ATTRIBUTE_DIRECTORY)
		{
			FolderPresent=true;
			AddSlash=true;
//      break;
		}
		else
		{
			FilesPresent=true;
		}
	}

	if (Link) // ������ �� ������ ������ (���������������!)
	{
		// ���������� ����� ��� ����������� �����.
		CopyDlg[ID_SC_ACTITLE].Flags|=DIF_DISABLE|DIF_HIDDEN;
		CopyDlg[ID_SC_ACCOPY].Flags|=DIF_DISABLE|DIF_HIDDEN;
		CopyDlg[ID_SC_ACINHERIT].Flags|=DIF_DISABLE|DIF_HIDDEN;
		CopyDlg[ID_SC_ACLEAVE].Flags|=DIF_DISABLE|DIF_HIDDEN;
		CopyDlg[ID_SC_SEPARATOR2].Flags|=DIF_HIDDEN;

		for(int i=ID_SC_SEPARATOR2;i<=ID_SC_COMBO;i++)
		{
			CopyDlg[i].Y1-=2;
			CopyDlg[i].Y2-=2;
		}
		for(int i=ID_SC_MULTITARGET;i<=ID_SC_BTNCANCEL;i++)
		{
			CopyDlg[i].Y1-=3;
			CopyDlg[i].Y2-=3;
		}
		CopyDlg[ID_SC_TITLE].Y2-=3;
		DLG_HEIGHT-=3;
	}

	// ������������ ������� " to"
	CopyDlg[ID_SC_TARGETTITLE].X1=CopyDlg[ID_SC_TARGETTITLE].X2=CopyDlg[ID_SC_SOURCEFILENAME].X1+CopyDlg[ID_SC_SOURCEFILENAME].strData.size();

	/* $ 15.06.2002 IS
	   ��������� ����������� ������ - � ���� ������ ������ �� ������������,
	   �� ���������� ��� ����� ����������������. ���� ���������� ���������
	   ���������� ������ �����, �� ������� ������.
	*/
	string strCopyDlgValue;
	if (!Ask)
	{
		strCopyDlgValue = CopyDlg[ID_SC_TARGETEDIT].strData;
		split(m_DestList, InsertQuote(Unquote(strCopyDlgValue)), STLF_UNIQUE);
		if (m_DestList.empty())
			Ask=TRUE;
	}

	// ***********************************************************************
	// *** ����� � ��������� �������
	// ***********************************************************************
	if (Ask)
	{
		FarList ComboList={sizeof(FarList)};
		FarListItem LinkTypeItems[5]={},CopyModeItems[8]={};

		if (Link)
		{
			ComboList.ItemsNumber=ARRAYSIZE(LinkTypeItems);
			ComboList.Items=LinkTypeItems;
			ComboList.Items[0].Text=MSG(MLinkTypeHardlink);
			ComboList.Items[1].Text=MSG(MLinkTypeJunction);
			ComboList.Items[2].Text=MSG(MLinkTypeSymlink);
			ComboList.Items[3].Text=MSG(MLinkTypeSymlinkFile);
			ComboList.Items[4].Text=MSG(MLinkTypeSymlinkDirectory);

			if (FilesPresent)
				ComboList.Items[0].Flags|=LIF_SELECTED;
			else
				ComboList.Items[1].Flags|=LIF_SELECTED;
		}
		else
		{
			ComboList.ItemsNumber=ARRAYSIZE(CopyModeItems);
			ComboList.Items=CopyModeItems;
			ComboList.Items[CM_ASK].Text=MSG(MCopyAsk);
			ComboList.Items[CM_OVERWRITE].Text=MSG(MCopyOverwrite);
			ComboList.Items[CM_SKIP].Text=MSG(MCopySkipOvr);
			ComboList.Items[CM_RENAME].Text=MSG(MCopyRename);
			ComboList.Items[CM_APPEND].Text=MSG(MCopyAppend);
			ComboList.Items[CM_ONLYNEWER].Text=MSG(MCopyOnlyNewerFiles);
			ComboList.Items[CM_ASKRO].Text=MSG(MCopyAskRO);
			ComboList.Items[CM_ASK].Flags=LIF_SELECTED;
			ComboList.Items[CM_SEPARATOR].Flags=LIF_SEPARATOR;

			if (Global->Opt->Confirm.RO)
			{
				ComboList.Items[CM_ASKRO].Flags=LIF_CHECKED;
			}
		}

		CopyDlg[ID_SC_COMBO].ListItems=&ComboList;
		auto Dlg = Dialog::create(CopyDlg, &ShellCopy::CopyDlgProc, this);
		Dlg->SetHelp(Link?L"HardSymLink":L"CopyFiles");
		Dlg->SetId(Link?HardSymLinkId:(Move?MoveFilesId:CopyFilesId));
		Dlg->SetPosition(-1,-1,DLG_WIDTH,DLG_HEIGHT);
		Dlg->SetAutomation(ID_SC_USEFILTER,ID_SC_BTNFILTER,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
//    Dlg->Show();
		// $ 02.06.2001 IS + �������� ������ ����� � �������� �������, ���� �� �������� ������
		int DlgExitCode;

		for (;;)
		{
			Dlg->ClearDone();
			Dlg->Process();
			DlgExitCode=Dlg->GetExitCode();
			//������ �������� ������� ��� ������� ����� ����� ������ �� �������
			Filter->UpdateCurrentTime();

			if (DlgExitCode == ID_SC_BTNCOPY)
			{
				/* $ 03.08.2001 IS
				   �������� ������� �� ������� � �������� �� ������ � ����������� ��
				   ��������� ����� �����������������
				*/
				strCopyDlgValue = CopyDlg[ID_SC_TARGETEDIT].strData;
				if(!Move)
				{
					Global->Opt->CMOpt.MultiCopy=CopyDlg[ID_SC_MULTITARGET].Selected == BSTATE_CHECKED;
				}

				if (!CopyDlg[ID_SC_MULTITARGET].Selected || strCopyDlgValue.find_first_of(L",;") == string::npos) // ��������� multi*
				{
					// ������ ������ �������
					// ������� �������, ����� "������" ������ ��������������� ���
					// ����������� �� ������� ������������ � ����
					InsertQuote(Unquote(strCopyDlgValue));
				}

				split(m_DestList, strCopyDlgValue, STLF_UNIQUE);
				if (!m_DestList.empty())
				{
					// ��������� ������� ������������� �������. KM
					UseFilter=CopyDlg[ID_SC_USEFILTER].Selected;
					break;
				}
				else
				{
					Message(MSG_WARNING,1,MSG(MWarning),MSG(MCopyIncorrectTargetList), MSG(MOk));
				}
			}
			else
				break;
		}

		if (DlgExitCode == ID_SC_BTNCANCEL || DlgExitCode < 0 || (CopyDlg[ID_SC_BTNCOPY].Flags&DIF_DISABLE))
		{
			if (DestPlugin)
				ToPlugin=-1;

			return;
		}
	}

	// ***********************************************************************
	// *** ������ ���������� ������ ����� �������
	// ***********************************************************************
	Flags&=~FCOPY_COPYPARENTSECURITY;

	if (CopyDlg[ID_SC_ACCOPY].Selected)
	{
		Flags|=FCOPY_COPYSECURITY;
	}
	else if (CopyDlg[ID_SC_ACINHERIT].Selected)
	{
		Flags&=~(FCOPY_COPYSECURITY|FCOPY_LEAVESECURITY);
	}
	else
	{
		Flags|=FCOPY_LEAVESECURITY;
	}

	if (Global->Opt->CMOpt.UseSystemCopy)
		Flags|=FCOPY_USESYSTEMCOPY;
	else
		Flags&=~FCOPY_USESYSTEMCOPY;

	if (!(Flags&(FCOPY_COPYSECURITY|FCOPY_LEAVESECURITY)))
		Flags|=FCOPY_COPYPARENTSECURITY;

	m_CopySecurity=Flags&FCOPY_COPYSECURITY?1:(Flags&FCOPY_LEAVESECURITY?2:0);

	// � ����� ������ ��������� ���������� ����������� (�� ��� Link, �.�. ��� Link ��������� ��������� - "������!")
	if (!Link)
	{
		if (Move)
			CopySecurityMove=m_CopySecurity;
		else
			CopySecurityCopy=m_CopySecurity;
	}

	ReadOnlyDelMode=ReadOnlyOvrMode=OvrMode=SkipEncMode=SkipMode=SkipDeleteMode=-1;
	SkipSecurityErrors = false;

	if (Link)
	{
		switch (CopyDlg[ID_SC_COMBO].ListPos)
		{
			case 0:
				RPT=RP_HARDLINK;
				break;
			case 1:
				RPT=RP_JUNCTION;
				break;
			case 2:
				RPT=RP_SYMLINK;
				break;
			case 3:
				RPT=RP_SYMLINKFILE;
				break;
			case 4:
				RPT=RP_SYMLINKDIR;
				break;
		}
	}
	else
	{
		ReadOnlyOvrMode=AskRO?-1:1;

		switch (CopyDlg[ID_SC_COMBO].ListPos)
		{
			case CM_ASK:
				OvrMode=-1;
				break;
			case CM_OVERWRITE:
				OvrMode=1;
				break;
			case CM_SKIP:
				OvrMode=3;
				ReadOnlyOvrMode=AskRO?-1:3;
				break;
			case CM_RENAME:
				OvrMode=5;
				break;
			case CM_APPEND:
				OvrMode=7;
				break;
			case CM_ONLYNEWER:
				Flags|=FCOPY_ONLYNEWERFILES;
				break;
		}
	}

	Flags|=CopyDlg[ID_SC_COPYSYMLINK].Selected?FCOPY_COPYSYMLINKCONTENTS:0;

	if (DestPlugin && CopyDlg[ID_SC_TARGETEDIT].strData == strInitDestDir)
	{
		ToPlugin=1;
		return;
	}

	if (DestPlugin==2)
	{
		if (PluginDestPath)
			strCopyDlgValue = PluginDestPath;

		return;
	}

	if ((Global->Opt->Diz.UpdateMode==DIZ_UPDATE_IF_DISPLAYED && SrcPanel->IsDizDisplayed()) ||
	        Global->Opt->Diz.UpdateMode==DIZ_UPDATE_ALWAYS)
	{
		Global->CtrlObject->Cp()->LeftPanel->ReadDiz();
		Global->CtrlObject->Cp()->RightPanel->ReadDiz();
	}

	CopyBuffer.reset(CopyBufferSize);
	DestPanel->CloseFile();
	strDestDizPath.clear();
	SrcPanel->SaveSelection();
	// TODO: Posix - bugbug
	ReplaceSlashToBackslash(strCopyDlgValue);
	// ����� �� ���������� ����� �����������?
	// ***********************************************************************
	// **** ����� ��� ���������������� �������� ���������, ����� ����������
	// **** � �������� Copy/Move/Link
	// ***********************************************************************
	int NeedDizUpdate=FALSE;
	int NeedUpdateAPanel=FALSE;
	// ����! ������������� �������� ����������.
	// � ����������� ���� ���� ����� ������������ � ShellCopy::CheckUpdatePanel()
	Flags|=FCOPY_UPDATEPPANEL;
	/*
	   ���� ������� � �������� ����������� �����, �������� ';',
	   �� ����� ������� CopyDlgValue �� ������� MultiCopy �
	   �������� CopyFileTree ������ ���������� ���.
	*/
	{
		Flags&=~FCOPY_MOVE;
		split(m_DestList, strCopyDlgValue, STLF_UNIQUE);
		if (!m_DestList.empty())
		{
			string strNameTmp;
			// ��������� ���������� �����.
			CountTarget=m_DestList.size();
			TotalFiles=0;
			TotalCopySize=TotalCopiedSize=TotalSkippedSize=0;

			if (CountTarget > 1)
				Move=0;

			FOR_CONST_RANGE(m_DestList, i)
			{
				bool LastIteration = false;
				{
					auto j = i;
					if (++j == m_DestList.end())
						LastIteration = true;
				}
				CurCopiedSize=0;
				strNameTmp = *i;

				if ((strNameTmp.size() == 2) && IsAlpha(strNameTmp[0]) && (strNameTmp[1] == L':'))
					PrepareDiskPath(strNameTmp);

				if (CheckNulOrCon(strNameTmp.data()))
				{
					Flags|=FCOPY_COPYTONUL;
					strNameTmp = L"\\\\?\\nul\\";
				}
				else
					Flags&=~FCOPY_COPYTONUL;

				if (Flags&FCOPY_COPYTONUL)
				{
					Flags&=~FCOPY_MOVE;
					Move=0;
				}
				bool ShowCopyTime=(Global->Opt->CMOpt.CopyTimeRule&((Flags&FCOPY_COPYTONUL)?COPY_RULE_NUL:COPY_RULE_FILES))!=0;

				if (SelCount==1 || (Flags&FCOPY_COPYTONUL))
					AddSlash=false; //???


				if (LastIteration) // ����� ������ ������� ��������� � ��������� Move.
				{
					Flags|=FCOPY_COPYLASTTIME|(Move?FCOPY_MOVE:0); // ������ ��� ��������� ��������
				}

				// ���� ���������� ��������� ������ 1 � ����� ��� ���� �������, �� ������
				// ������ ���, ����� �� ����� ��� '\\'
				// ������ ��� �� ������, � ������ ����� NameTmp �� �������� ������.
				if (AddSlash && strNameTmp.find_first_of(L"*?") == string::npos)
					AddEndSlash(strNameTmp);

				if (SelCount==1 && !FolderPresent)
				{
					ShowTotalCopySize=false;
					TotalFilesToProcess=1;
				}

				if (Move) // ��� ����������� "�����" ��� �� ����������� ��� "���� �� �����"
				{
					if (CheckDisksProps(strSrcDir,strNameTmp,CHECKEDPROPS_ISSAMEDISK))
						ShowTotalCopySize=false;
					if (SelCount==1 && FolderPresent && CheckUpdateAnotherPanel(SrcPanel,strSelName))
					{
						NeedUpdateAPanel=TRUE;
					}
				}
				if (!CP)
					CP = std::make_unique<CopyProgress>(Move!=0,ShowTotalCopySize,ShowCopyTime);

				// ������� ���� ��� ����
				strDestDizPath.clear();
				Flags&=~FCOPY_DIZREAD;
				// �������� ���������
				SrcPanel->SaveSelection();
				strDestFSName.clear();
				int OldCopySymlinkContents=Flags&FCOPY_COPYSYMLINKCONTENTS;
				// ���������� - ���� ������ �����������
				// Mantis#45: ���������� �������� ����������� ������ �� ����� � NTFS �� FAT � ����� ��������� ����
				{
					string strRootDir;
					ConvertNameToFull(strNameTmp,strRootDir);
					GetPathRoot(strRootDir, strRootDir);
					DWORD FileSystemFlags=0;

					if (os::GetVolumeInformation(strRootDir,nullptr,nullptr,nullptr,&FileSystemFlags,nullptr))
						if (!(FileSystemFlags&FILE_SUPPORTS_REPARSE_POINTS))
							Flags|=FCOPY_COPYSYMLINKCONTENTS;
				}


				int I;
				{
					auto item = std::make_unique<CopyPreRedrawItem>();
					item->CP = CP.get();
					SCOPED_ACTION(TPreRedrawFuncGuard)(std::move(item));
					I=CopyFileTree(strNameTmp);
				}

				if (OldCopySymlinkContents)
					Flags|=FCOPY_COPYSYMLINKCONTENTS;
				else
					Flags&=~FCOPY_COPYSYMLINKCONTENTS;

				if (I == COPY_CANCEL)
				{
					NeedDizUpdate=TRUE;
					break;
				}

				// ���� "���� ����� � ������������" - ����������� ���������
				if (!LastIteration)
					SrcPanel->RestoreSelection();

				// ����������� � �����.
				if (!(Flags&FCOPY_COPYTONUL) && !strDestDizPath.empty())
				{
					const auto Attr = os::GetFileAttributes(DestDiz.GetDizName());
					int DestReadOnly=(Attr!=INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_READONLY));

					if (LastIteration) // ��������� ������ �� ����� ��������� Op.
						if (Move && !DestReadOnly)
							SrcPanel->FlushDiz();

					DestDiz.Flush(strDestDizPath);
				}
			}
		}
		_LOGCOPYR(else SysLog(L"Error: DestList.Set(CopyDlgValue) return FALSE"));
	}
	// ***********************************************************************
	// *** �������������� ������ ��������
	// *** ���������������/�����/��������
	// ***********************************************************************

	if (NeedDizUpdate) // ��� ����������������� ����� ���� �����, �� ��� ���
	{                 // ����� ����� ��������� ����!
		if (!(Flags&FCOPY_COPYTONUL) && !strDestDizPath.empty())
		{
			const auto Attr = os::GetFileAttributes(DestDiz.GetDizName());
			int DestReadOnly=(Attr!=INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_READONLY));

			if (Move && !DestReadOnly)
				SrcPanel->FlushDiz();

			DestDiz.Flush(strDestDizPath);
		}
	}

#if 1
	SrcPanel->Update(UPDATE_KEEP_SELECTION);

	if (SelCount==1 && !strRenamedName.empty())
		SrcPanel->GoToFile(strRenamedName);

#if 1

	if (NeedUpdateAPanel && m_FileAttr != INVALID_FILE_ATTRIBUTES && (m_FileAttr&FILE_ATTRIBUTE_DIRECTORY) && DestPanelMode != PLUGIN_PANEL)
	{
		DestPanel->SetCurDir(SrcPanel->GetCurDir(), false);
	}

#else

	if (m_FileAttr != INVALID_FILE_ATTRIBUTES && (m_FileAttr&FILE_ATTRIBUTE_DIRECTORY) && DestPanelMode != PLUGIN_PANEL)
	{
		// ���� SrcDir ���������� � DestDir...
		string strTmpDestDir;
		string strTmpSrcDir;
		DestPanel->GetCurDir(strTmpDestDir);
		SrcPanel->GetCurDir(strTmpSrcDir);

		if (CheckUpdateAnotherPanel(SrcPanel,strTmpSrcDir))
			DestPanel->SetCurDir(strTmpDestDir,false);
	}

#endif

	// �������� "��������" ������� ��������� ������
	if (Flags&FCOPY_UPDATEPPANEL)
	{
		DestPanel->SortFileList(TRUE);
		DestPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
	}

	if (SrcPanelMode == PLUGIN_PANEL)
		SrcPanel->SetPluginModified();

	Global->CtrlObject->Cp()->Redraw();
#else
	SrcPanel->Update(UPDATE_KEEP_SELECTION);

	if (SelCount==1 && strRenamedName.empty())
		SrcPanel->GoToFile(strRenamedName);

	SrcPanel->Redraw();
	DestPanel->SortFileList(TRUE);
	DestPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
	DestPanel->Redraw();
#endif
}

ShellCopy::~ShellCopy()
{
	_tran(SysLog(L"[%p] ShellCopy::~ShellCopy(), CopyBufer=%p",this,CopyBuffer));

	// $ 26.05.2001 OT ��������� ����������� �������
	Global->CtrlObject->Cp()->Unlock();
	Global->CtrlObject->Cp()->Refresh();

	delete Filter;
}

COPY_CODES ShellCopy::CopyFileTree(const string& Dest)
{
	SCOPED_ACTION(ChangePriority)(THREAD_PRIORITY_NORMAL);
	//SaveScreen SaveScr;
	DWORD DestAttr = INVALID_FILE_ATTRIBUTES;
	size_t DestMountLen = 0;
	string strSelName, strSelShortName;
	DWORD FileAttr;

	if (Dest.empty() || Dest == L".")
		return COPY_FAILURE; //????

	SetCursorType(false, 0);

	//Flags &= ~(FCOPY_STREAMSKIP|FCOPY_STREAMALL); // unused now...
	DWORD Flags0 = Flags;

	bool first = true;
	bool UseWildCards = Dest.find_first_of(L"*?") != string::npos;
	bool copy_to_null = (0 != (Flags & FCOPY_COPYTONUL));
	bool move_rename = (0 != (Flags & FCOPY_MOVE));
	bool SameDisk = false;

	if (!TotalCopySize)
	{
		CP->strTotalCopySizeText.clear();

		//  ! �� ��������� �������� ��� �������� ������
		if (ShowTotalCopySize && !(Flags&FCOPY_LINK) && !CalcTotalSize())
			return COPY_FAILURE;
	}
	else
	{
		CurCopiedSize=0;
	}

	// �������� ���� ����������� ����� ������.
	//
	SrcPanel->GetSelName(nullptr, FileAttr);
	while (SrcPanel->GetSelName(&strSelName, FileAttr, &strSelShortName))
	{
		string strDest(Dest);
		Flags = (Flags0 & ~FCOPY_DIZREAD) | (Flags & FCOPY_DIZREAD);

		bool src_abspath = IsAbsolutePath(strSelName);

		bool dst_abspath = copy_to_null || IsAbsolutePath(strDest);
		if (!dst_abspath && ((strDest.size() > 2 && strDest[1] == L':') || (!strDest.empty() && IsSlash(strDest[0]))))
		{
			ConvertNameToFull(strDest, strDest);
			dst_abspath = true;
		}

		SelectedFolderNameLength = (FileAttr & FILE_ATTRIBUTE_DIRECTORY)?(int)strSelName.size():0;
		if (UseWildCards)
			ConvertWildcards(strSelName, strDest, SelectedFolderNameLength);

		bool simple_rename = false;
		if (move_rename && first && SrcPanel->GetSelCount() == 1 && !src_abspath)
			simple_rename = PointToName(strDest) == strDest.data();

		if (!simple_rename && !dst_abspath && !IsAbsolutePath(strDest))
		{
			string tpath;
			if (!src_abspath) {
				tpath = SrcPanel->GetCurDir();
				AddEndSlash(tpath);
			}
			else {
				size_t slash_pos;
				FindLastSlash(slash_pos, strSelName);
				tpath = strSelName.substr(0, slash_pos+1);
			}
			strDest = tpath + strDest;
		}

		bool check_samedisk = false, dest_changed = false;
		if (first || strSrcDriveRoot.empty() || (src_abspath && StrCmpNI(strSelName.data(), strSrcDriveRoot.data(), strSrcDriveRoot.size())))
		{
			GetPathRoot(src_abspath ? strSelName : SrcPanel->GetCurDir(), strSrcDriveRoot);
			SrcDriveType = FAR_GetDriveType(strSrcDriveRoot);
			check_samedisk = true;
		}
		if (!copy_to_null && (first || strDestDriveRoot.empty() || StrCmpNI(strDest.data(), strDestDriveRoot.data(), strDestDriveRoot.size())))
		{
			GetPathRoot(strDest, strDestDriveRoot);
			DestDriveType = FAR_GetDriveType(strDestDriveRoot);
			DestMountLen = GetMountPointLen(strDest, strDestDriveRoot);
			check_samedisk = dest_changed = true;
		}
		if (move_rename && !copy_to_null && check_samedisk)
		{
			SameDisk = CheckDisksProps(src_abspath ? strSelName : SrcPanel->GetCurDir(), strDest, CHECKEDPROPS_ISSAMEDISK) != 0;
		}

		if (first && !copy_to_null && (dst_abspath || !src_abspath) && !UseWildCards
		 && SrcPanel->GetSelCount() > 1
		 && !IsSlash(strDest.back())
		 && !os::fs::exists(strDest))
		{
			switch (Message(FMSG_WARNING,3,MSG(MWarning),strDest.data(),MSG(MCopyDirectoryOrFile),MSG(MCopyDirectoryOrFileDirectory),MSG(MCopyDirectoryOrFileFile),MSG(MCancel)))
			{
			case 2: case -1: case -2: return COPY_CANCEL; // [Cancel]
			//case 1: break;                              // [File]
			case 0: AddEndSlash(strDest);	break;          // {Directory}
			}
		}

		if (dest_changed) // check destination drive ready
		{
			DestAttr = os::GetFileAttributes(strDest);
			if (INVALID_FILE_ATTRIBUTES == DestAttr)
			{
				auto Exists_1 = os::fs::exists(strDestDriveRoot);
				auto Exists_2 = Exists_1;
				while ( !Exists_2 && SkipMode != 2)
				{
					Global->CatchError();
					int ret = OperationFailed(strDestDriveRoot, MError, L"");
					if (ret < 0 || ret == 4)
						return COPY_CANCEL;
					else if (ret == 1)
						return COPY_NEXT;
					else if (ret == 2)
					{
						SkipMode = 2;
						return COPY_NEXT;
					}

					Exists_2 = os::fs::exists(strDestDriveRoot);
				}
				if (!Exists_1 && Exists_2)
					DestAttr = os::GetFileAttributes(strDest);
			}
		}

		size_t pos;
		if (!copy_to_null && FindLastSlash(pos,strDest) && (!DestMountLen || pos > DestMountLen))
		{
			string strNewPath = strDest.substr(0, pos);
			os::fs::file_status NewPathStatus(strNewPath);
			if (!os::fs::exists(NewPathStatus))
			{
				if (os::CreateDirectory(strNewPath,nullptr))
					TreeList::AddTreeName(strNewPath);
				else
					CreatePath(strNewPath);

				DestAttr = os::GetFileAttributes(strDest);
			}
			else if (os::fs::is_file(NewPathStatus))
			{
				Message(MSG_WARNING,1,MSG(MError),MSG(MCopyCannotCreateFolder),strNewPath.data(),MSG(MOk));
				return COPY_FAILURE;
			}
		}

		// �������� ������ �������, ���������� �� ����� (�� �� ��� ������ �������������� ����� �� ����)
		if ((DestDriveType == DRIVE_REMOTE || SrcDriveType == DRIVE_REMOTE) && StrCmpI(strSrcDriveRoot, strDestDriveRoot))
			Flags |= FCOPY_COPYSYMLINKCONTENTS;

		first = false;
		string strDestPath = strDest;

		os::FAR_FIND_DATA SrcData;
		int CopyCode = COPY_SUCCESS, KeepPathPos;
		Flags &= ~FCOPY_OVERWRITENEXT;

		KeepPathPos = (int)(size_t)(PointToName(strSelName) - strSelName.data());

		if (RPT==RP_JUNCTION || RPT==RP_SYMLINK || RPT==RP_SYMLINKFILE || RPT==RP_SYMLINKDIR)
		{
			switch (MkSymLink(strSelName, strDest, RPT))
			{
				case 2:
					break;
				case 1:

					// ������� (Ins) ��������� ���������, ALT-F6 Enter - ��������� � ����� �� �������.
					if ((!(Flags&FCOPY_CURRENTONLY)) && (Flags&FCOPY_COPYLASTTIME))
						SrcPanel->ClearLastGetSelection();

					continue;
				case 0:
					return COPY_FAILURE;
			}
		}
		else
		{
			// �������� �� �������� ;-)
			if (!os::GetFindDataEx(strSelName,SrcData))
			{
				strDestPath = strSelName;
				CP->SetNames(strSelName,strDestPath);

				if (Message(MSG_WARNING,2,MSG(MError),MSG(MCopyCannotFind),
					          strSelName.data(),MSG(MSkip),MSG(MCancel))==1)
				{
					return COPY_FAILURE;
				}

				continue;
			}
		}

		if (move_rename)
		{
			if ((UseFilter || !SameDisk) || ((SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) && (Flags&FCOPY_COPYSYMLINKCONTENTS)))
			{
				CopyCode=COPY_FAILURE;
			}
			else
			{
				do
				{
					CopyCode=ShellCopyOneFile(strSelName,SrcData,strDestPath,KeepPathPos,1);
				}
				while (CopyCode==COPY_RETRY);

				if (CopyCode==COPY_SUCCESS_MOVE)
				{
					if (!strDestDizPath.empty())
					{
						if (!strRenamedName.empty())
						{
							DestDiz.DeleteDiz(strSelName,strSelShortName);
							SrcPanel->CopyDiz(strSelName,strSelShortName,strRenamedName,strRenamedName,&DestDiz);
						}
						else
						{
							if (strCopiedName.empty())
								strCopiedName = strSelName;

							SrcPanel->CopyDiz(strSelName,strSelShortName,strCopiedName,strCopiedName,&DestDiz);
							SrcPanel->DeleteDiz(strSelName,strSelShortName);
						}
					}

					continue;
				}

				if (CopyCode==COPY_CANCEL)
					return COPY_CANCEL;

				if (CopyCode==COPY_NEXT)
				{
					TotalCopiedSize = TotalCopiedSize - CurCopiedSize + SrcData.nFileSize;
					TotalSkippedSize = TotalSkippedSize + SrcData.nFileSize - CurCopiedSize;
					continue;
				}

				if (!(Flags&FCOPY_MOVE) || CopyCode==COPY_FAILURE)
					Flags|=FCOPY_OVERWRITENEXT;
			}
		}

		if (!(Flags&FCOPY_MOVE) || CopyCode==COPY_FAILURE)
		{
			string strCopyDest=strDest;

			do
			{
				CopyCode=ShellCopyOneFile(strSelName,SrcData,strCopyDest,KeepPathPos,0);
			}
			while (CopyCode==COPY_RETRY);

			Flags&=~FCOPY_OVERWRITENEXT;

			if (CopyCode==COPY_CANCEL)
				return COPY_CANCEL;

			if (CopyCode!=COPY_SUCCESS)
			{
				if (CopyCode != COPY_NOFILTER) //????
					TotalCopiedSize = TotalCopiedSize - CurCopiedSize +  SrcData.nFileSize;

				if (CopyCode == COPY_NEXT)
					TotalSkippedSize = TotalSkippedSize +  SrcData.nFileSize - CurCopiedSize;

				continue;
			}
		}

		if (CopyCode==COPY_SUCCESS && !(Flags&FCOPY_COPYTONUL) && !strDestDizPath.empty())
		{
			if (strCopiedName.empty())
				strCopiedName = strSelName;

			SrcPanel->CopyDiz(strSelName,strSelShortName,strCopiedName,strCopiedName,&DestDiz);
		}

		// Mantis#44 - ������ ������ ��� ����������� ������ �� �����
		// ���� ������� (��� ����� ���������� �������) - �������� ���������� ����������...
		if (RPT!=RP_SYMLINKFILE && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			      (
			          !(SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) ||
			          ((SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) && (Flags&FCOPY_COPYSYMLINKCONTENTS))
			      )
			  )
		{
			int SubCopyCode;
			string strSubName;
			string strFullName;
			ScanTree ScTree(true, true, Flags & FCOPY_COPYSYMLINKCONTENTS);
			strSubName = strSelName;
			strSubName += L"\\";

			if (DestAttr==INVALID_FILE_ATTRIBUTES)
				KeepPathPos=(int)strSubName.size();

			int NeedRename=!((SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) && (Flags&FCOPY_COPYSYMLINKCONTENTS) && (Flags&FCOPY_MOVE));
			ScTree.SetFindPath(strSubName,L"*",FSCANTREE_FILESFIRST);

			while (ScTree.GetNextName(&SrcData,strFullName))
			{
				if (UseFilter && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					// ������ ���������� ������� ������������ - ���� ������� ������� �
					// ������� ��� ������������, �� ������� ���������� � ��� � �� ���
					// ����������.
					if (!Filter->FileInFilter(SrcData, nullptr, &strFullName))
					{
						ScTree.SkipDir();
						continue;
					}
				}
				{
					int AttemptToMove=FALSE;

					if ((Flags&FCOPY_MOVE) && SameDisk && !(SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						AttemptToMove=TRUE;
						int Ret=COPY_SUCCESS;
						string strCopyDest=strDest;

						do
						{
							Ret=ShellCopyOneFile(strFullName,SrcData,strCopyDest,KeepPathPos,NeedRename);
						}
						while (Ret==COPY_RETRY);

						switch (Ret) // 1
						{
							case COPY_CANCEL:
								return COPY_CANCEL;
							case COPY_NEXT:
							{
								TotalCopiedSize = TotalCopiedSize - CurCopiedSize + SrcData.nFileSize;
								TotalSkippedSize = TotalSkippedSize + SrcData.nFileSize - CurCopiedSize;
								continue;
							}
							case COPY_SUCCESS_MOVE:
							{
								continue;
							}
							case COPY_SUCCESS:

								if (!NeedRename) // ������� ��� ����������� ����������� �������� � ������ "���������� ���������� ���..."
								{
									TotalCopiedSize = TotalCopiedSize - CurCopiedSize + SrcData.nFileSize;
									TotalSkippedSize = TotalSkippedSize + SrcData.nFileSize - CurCopiedSize;
									continue;     // ...  �.�. �� ��� �� ������, � �����������, �� ���, �� ���� �������� �������� � ���� ������
								}
						}
					}

					int SaveOvrMode=OvrMode;

					if (AttemptToMove)
						OvrMode=1;

					string strCopyDest=strDest;

					do
					{
						SubCopyCode=ShellCopyOneFile(strFullName,SrcData,strCopyDest,KeepPathPos,0);
					}
					while (SubCopyCode==COPY_RETRY);

					if (AttemptToMove)
						OvrMode=SaveOvrMode;
				}

				if (SubCopyCode==COPY_CANCEL)
					return COPY_CANCEL;

				if (SubCopyCode==COPY_NEXT)
				{
					TotalCopiedSize = TotalCopiedSize - CurCopiedSize + SrcData.nFileSize;
					TotalSkippedSize = TotalSkippedSize + SrcData.nFileSize - CurCopiedSize;
				}

				if (SubCopyCode==COPY_SUCCESS)
				{
					if (Flags&FCOPY_MOVE)
					{
						if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							if (ScTree.IsDirSearchDone() ||
								      ((SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && !(Flags&FCOPY_COPYSYMLINKCONTENTS)))
							{
								if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
									os::SetFileAttributes(strFullName,FILE_ATTRIBUTE_NORMAL);

								if (os::RemoveDirectory(strFullName))
									TreeList::DelTreeName(strFullName);
							}
						}
						// ����� ����� �������� �� FSCANTREE_INSIDEJUNCTION, �����
						// ��� ������� ����� �������� �����, ��� ������ �����������!
						else if (!ScTree.InsideJunction())
						{
							if (DeleteAfterMove(strFullName,SrcData.dwFileAttributes)==COPY_CANCEL)
								return COPY_CANCEL;
						}
					}
				}
			}

			if ((Flags&FCOPY_MOVE) && CopyCode==COPY_SUCCESS)
			{
				if (FileAttr & FILE_ATTRIBUTE_READONLY)
					os::SetFileAttributes(strSelName,FILE_ATTRIBUTE_NORMAL);

				if (os::RemoveDirectory(strSelName))
				{
					TreeList::DelTreeName(strSelName);

					if (!strDestDizPath.empty())
						SrcPanel->DeleteDiz(strSelName,strSelShortName);
				}
			}
		}
		else if ((Flags&FCOPY_MOVE) && CopyCode==COPY_SUCCESS)
		{
			int DeleteCode;

			if ((DeleteCode=DeleteAfterMove(strSelName,FileAttr))==COPY_CANCEL)
				return COPY_CANCEL;

			if (DeleteCode==COPY_SUCCESS && !strDestDizPath.empty())
				SrcPanel->DeleteDiz(strSelName,strSelShortName);
		}

		if ((!(Flags&FCOPY_CURRENTONLY)) && (Flags&FCOPY_COPYLASTTIME))
		{
			SrcPanel->ClearLastGetSelection();
		}
	}

	return COPY_SUCCESS; //COPY_SUCCESS_MOVE???
}



// ��������� ����������� �������. ������� ����� �������� �������� ���� �� �����. ���������� ASAP

COPY_CODES ShellCopy::ShellCopyOneFile(
    const string& Src,
    const os::FAR_FIND_DATA &SrcData,
    string &strDest,
    int KeepPathPos,
    int Rename
)
{
	CurCopiedSize = 0; // �������� ������� ��������

	if (CP->Cancelled())
	{
		return COPY_CANCEL;
	}

	if (UseFilter)
	{
		if (!Filter->FileInFilter(SrcData, nullptr, &Src))
			return COPY_NOFILTER;
	}

	string strDestPath = strDest;

	DWORD DestAttr=INVALID_FILE_ATTRIBUTES;

	os::FAR_FIND_DATA DestData;
	if (!(Flags&FCOPY_COPYTONUL))
	{
		if (os::GetFindDataEx(strDestPath,DestData))
			DestAttr=DestData.dwFileAttributes;
	}

	int SameName=0, Append=0;

	if (!(Flags&FCOPY_COPYTONUL) && DestAttr!=INVALID_FILE_ATTRIBUTES && (DestAttr & FILE_ATTRIBUTE_DIRECTORY))
	{
		if(SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			int CmpCode=CmpFullNames(Src,strDestPath);

			if(CmpCode && SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT && RPT==RP_EXACTCOPY && !(Flags&FCOPY_COPYSYMLINKCONTENTS))
			{
				CmpCode = 0;
			}

			if (CmpCode==1) // TODO: error check
			{
				SameName=1;

				if (Rename)
				{
					CmpCode=!StrCmp(PointToName(Src),PointToName(strDestPath));
				}

				if (CmpCode==1)
				{
					Message(MSG_WARNING, MSG(MError),
						make_vector<string>(MSG(MCannotCopyFolderToItself1), Src, MSG(MCannotCopyFolderToItself2)),
						make_vector<string>(MSG(MOk)),
						L"ErrCopyItSelf");
					return COPY_CANCEL;
				}
			}
		}

		if (!SameName)
		{
			int Length=(int)strDestPath.size();

			if (!IsSlash(strDestPath[Length-1]) && strDestPath[Length-1]!=L':')
				strDestPath += L"\\";

			const wchar_t *PathPtr=Src.data()+KeepPathPos;

			if (*PathPtr && !KeepPathPos && PathPtr[1]==L':')
				PathPtr+=2;

			if (IsSlash(*PathPtr))
				PathPtr++;

			strDestPath += PathPtr;

			if (!os::GetFindDataEx(strDestPath,DestData))
				DestAttr=INVALID_FILE_ATTRIBUTES;
			else
				DestAttr=DestData.dwFileAttributes;
		}
	}

	if (!(Flags&FCOPY_COPYTONUL) && StrCmpI(strDestPath.data(),L"prn"))
		SetDestDizPath(strDestPath);

	CP->SetProgressValue(0,0);
	CP->SetNames(Src,strDestPath);

	int IsSetSecuty=FALSE;

	if (!(Flags&FCOPY_COPYTONUL))
	{
		// �������� ���������� ��������� �� ������
		switch (CheckStreams(Src,strDestPath))
		{
			case COPY_NEXT:
				return COPY_NEXT;
			case COPY_CANCEL:
				return COPY_CANCEL;
			default:
				break;
		}

		bool dir = (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		bool rpt = (SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
		bool cpc = (Flags & FCOPY_COPYSYMLINKCONTENTS) != 0;
		if (!dir && rpt && RPT==RP_EXACTCOPY && !cpc)
		{
			bool spf = (SrcData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0;
			if (spf)
				cpc = true; // ???
			else
			{
				DWORD rpTag = 0;
				string strRptInfo;
				GetReparsePointInfo(Src, strRptInfo, &rpTag);
				cpc = (rpTag != IO_REPARSE_TAG_SYMLINK) && (rpTag != IO_REPARSE_TAG_MOUNT_POINT);
			}
			if (cpc)
				Flags |= FCOPY_COPYSYMLINKCONTENTS;
		}

		if (dir || (rpt && RPT==RP_EXACTCOPY && !cpc))
		{
			if (!Rename)
				strCopiedName = PointToName(strDestPath);

			if (DestAttr!=INVALID_FILE_ATTRIBUTES)
			{
				if ((DestAttr & FILE_ATTRIBUTE_DIRECTORY) && !SameName)
				{
					DWORD SetAttr=SrcData.dwFileAttributes;

					if (IsDriveTypeCDROM(SrcDriveType) && (SetAttr & FILE_ATTRIBUTE_READONLY))
						SetAttr&=~FILE_ATTRIBUTE_READONLY;

					if (SetAttr!=DestAttr)
						ShellSetAttr(strDestPath,SetAttr);

					string strSrcFullName;
					ConvertNameToFull(Src,strSrcFullName);
					return (strDestPath == strSrcFullName)? COPY_NEXT : COPY_SUCCESS;
				}

				int Type=os::GetFileTypeByName(strDestPath);

				if (Type==FILE_TYPE_CHAR || Type==FILE_TYPE_PIPE)
					return Rename? COPY_NEXT : COPY_SUCCESS;
			}

			if (Rename)
			{
				string strSrcFullName,strDestFullName;
				ConvertNameToFull(Src,strSrcFullName);
				os::FAR_SECURITY_DESCRIPTOR sd;

				// ��� Move ��� ���������� ������ ������� ��������, ����� �������� ��� ���������
				if (!(Flags&(FCOPY_COPYSECURITY|FCOPY_LEAVESECURITY)))
				{
					IsSetSecuty=FALSE;

					if (CmpFullPath(Src,strDest)) // � �������� ������ �������� ������ �� ������
						IsSetSecuty=FALSE;
					else if (!os::fs::exists(strDest)) // ���� �������� ���...
					{
						// ...�������� ��������� ��������
						if (GetSecurity(GetParentFolder(strDest,strDestFullName), sd))
							IsSetSecuty=TRUE;
					}
					else if (GetSecurity(strDest,sd)) // ����� �������� ��������� Dest`�
						IsSetSecuty=TRUE;
				}

				// �������� �������������, ���� �� �������
				for (;;)
				{
					BOOL SuccessMove=os::MoveFile(Src,strDestPath);

					if (SuccessMove)
					{
						if (IsSetSecuty)// && !strcmp(DestFSName,"NTFS"))
							SetRecursiveSecurity(strDestPath,sd);

						if (PointToName(strDestPath)==strDestPath.data())
							strRenamedName = strDestPath;
						else
							strCopiedName = PointToName(strDestPath);

						ConvertNameToFull(strDest, strDestFullName);
						TreeList::RenTreeName(strSrcFullName,strDestFullName);
						return SameName? COPY_NEXT : COPY_SUCCESS_MOVE;
					}
					else
					{
						Global->CatchError();
						int MsgCode = Message(MSG_WARNING|MSG_ERRORTYPE,3,MSG(MError),
						                      MSG(MCopyCannotRenameFolder),Src.data(),MSG(MCopyRetry),
						                      MSG(MCopyIgnore),MSG(MCopyCancel));

						switch (MsgCode)
						{
							case 0:  continue;
							case 1:
							{
								int CopySecurity = Flags&FCOPY_COPYSECURITY;
								os::FAR_SECURITY_DESCRIPTOR tmpsd;

								if ((CopySecurity) && !GetSecurity(Src,tmpsd))
									CopySecurity = FALSE;
								SECURITY_ATTRIBUTES TmpSecAttr  ={sizeof(TmpSecAttr), tmpsd.get(), FALSE};
								if (os::CreateDirectory(strDestPath,CopySecurity?&TmpSecAttr:nullptr))
								{
									if (PointToName(strDestPath)==strDestPath.data())
										strRenamedName = strDestPath;
									else
										strCopiedName = PointToName(strDestPath);

									TreeList::AddTreeName(strDestPath);
									return COPY_SUCCESS;
								}
							}
							default:
								return COPY_CANCEL;
						} /* switch */
					} /* else */
				} /* while */
			} // if (Rename)

			os::FAR_SECURITY_DESCRIPTOR sd;

			if ((Flags&FCOPY_COPYSECURITY) && !GetSecurity(Src,sd))
				return COPY_CANCEL;
			SECURITY_ATTRIBUTES SecAttr = {sizeof(SecAttr), sd.get(), FALSE};
			if (RPT!=RP_SYMLINKFILE && SrcData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				while (!os::CreateDirectoryEx(
					// CreateDirectoryEx preserves reparse points,
					// so we shouldn't use template when copying with content
					((SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) && (Flags&FCOPY_COPYSYMLINKCONTENTS))? L"" : Src,
					strDestPath,(Flags&FCOPY_COPYSECURITY) ? &SecAttr:nullptr))
				{
					Global->CatchError();
					int MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,3,MSG(MError),
					                MSG(MCopyCannotCreateFolder),strDestPath.data(),MSG(MCopyRetry),
					                MSG(MCopySkip),MSG(MCopyCancel));

					if (MsgCode)
						return (MsgCode==-2 || MsgCode==2)? COPY_CANCEL : COPY_NEXT;
				}

				DWORD SetAttr=SrcData.dwFileAttributes;

				if (IsDriveTypeCDROM(SrcDriveType) && (SetAttr & FILE_ATTRIBUTE_READONLY))
					SetAttr&=~FILE_ATTRIBUTE_READONLY;

				if ((SetAttr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
				{
					// �� ����� ���������� ����������, ���� ������� � �������
					// � ������������ FILE_ATTRIBUTE_ENCRYPTED (� �� ��� ����� ��������� ����� CreateDirectory)
					// �.�. ���������� ������ ���.
					if (os::fs::file_status(strDestPath).check(FILE_ATTRIBUTE_ENCRYPTED))
						SetAttr&=~FILE_ATTRIBUTE_COMPRESSED;

					if (SetAttr&FILE_ATTRIBUTE_COMPRESSED)
					{
						for (;;)
						{
							int MsgCode=ESetFileCompression(strDestPath,1,0,SkipMode);

							if (MsgCode)
							{
								if (MsgCode == SETATTR_RET_SKIP)
									Flags|=FCOPY_SKIPSETATTRFLD;
								else if (MsgCode == SETATTR_RET_SKIPALL)
								{
									Flags|=FCOPY_SKIPSETATTRFLD;
									SkipMode=SETATTR_RET_SKIP;
								}

								break;
							}

							if (MsgCode != SETATTR_RET_OK)
								return (MsgCode==SETATTR_RET_SKIP || MsgCode==SETATTR_RET_SKIPALL) ? COPY_NEXT:COPY_CANCEL;
						}
					}

					while (!ShellSetAttr(strDestPath,SetAttr))
					{
						Global->CatchError();
						int MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,4,MSG(MError),
						                MSG(MCopyCannotChangeFolderAttr),strDestPath.data(),
						                MSG(MCopyRetry),MSG(MCopySkip),MSG(MCopySkipAll),MSG(MCopyCancel));

						if (MsgCode)
						{
							if (MsgCode==1)
								break;

							if (MsgCode==2)
							{
								Flags|=FCOPY_SKIPSETATTRFLD;
								break;
							}

							os::RemoveDirectory(strDestPath);
							return (MsgCode==-2 || MsgCode==3)? COPY_CANCEL : COPY_NEXT;
						}
					}
				}
				else if (!(Flags & FCOPY_SKIPSETATTRFLD) && ((SetAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
				{
					while (!ShellSetAttr(strDestPath,SetAttr))
					{
						Global->CatchError();
						int MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,4,MSG(MError),
						                MSG(MCopyCannotChangeFolderAttr),strDestPath.data(),
						                MSG(MCopyRetry),MSG(MCopySkip),MSG(MCopySkipAll),MSG(MCopyCancel));

						if (MsgCode)
						{
							if (MsgCode==1)
								break;

							if (MsgCode==2)
							{
								Flags|=FCOPY_SKIPSETATTRFLD;
								break;
							}

							os::RemoveDirectory(strDestPath);
							return (MsgCode==-2 || MsgCode==3)? COPY_CANCEL : COPY_NEXT;
						}
					}
				}
			}

			// [ ] Copy contents of symbolic links
			// For file symbolic links only!!!
			// Directory symbolic links and junction points are handled by CreateDirectoryEx.
			if (!dir && rpt && !cpc && RPT==RP_EXACTCOPY)
			{
				switch (MkSymLink(Src, strDestPath, RPT))
				{
					case 2:
						return COPY_CANCEL;
					case 1:
						break;
					case 0:
						return COPY_FAILURE;
				}
			}

			TreeList::AddTreeName(strDestPath);
			return COPY_SUCCESS;
		}

		if (DestAttr!=INVALID_FILE_ATTRIBUTES && !(DestAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (SrcData.nFileSize==DestData.nFileSize)
			{
				int CmpCode=CmpFullNames(Src,strDestPath);

				if(CmpCode && SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT && RPT==RP_EXACTCOPY && !(Flags&FCOPY_COPYSYMLINKCONTENTS))
				{
					CmpCode = 0;
				}

				if (CmpCode==1) // TODO: error check
				{
					SameName=1;

					if (Rename)
					{
						CmpCode=!StrCmp(PointToName(Src),PointToName(strDestPath));
					}

					if (CmpCode==1 && !Rename)
					{
						string qSrc(Src);
						Message(MSG_WARNING,1,MSG(MError),MSG(MCannotCopyFileToItself1),
							    QuoteLeadingSpace(qSrc).data(),MSG(MCannotCopyFileToItself2),MSG(MOk));
						return COPY_CANCEL;
					}
				}
			}

			int RetCode=COPY_CANCEL;
			string strNewName;

			if (!AskOverwrite(SrcData,Src,strDestPath,DestAttr,SameName,Rename,((Flags&FCOPY_LINK)?0:1),Append,strNewName,RetCode))
			{
				return (COPY_CODES)RetCode;
			}

			if (RetCode==COPY_RETRY)
			{
				strDest=strNewName;

				if (CutToSlash(strNewName) && !os::fs::exists(strNewName))
				{
					CreatePath(strNewName);
				}

				return COPY_RETRY;
			}
		}
	}
	else
	{
		if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			return COPY_SUCCESS;
		}
	}

	bool NWFS_Attr = Global->Opt->Nowell.MoveRO && strDestFSName == L"NWFS";
	{
		for (;;)
		{
			int CopyCode=0;
			unsigned __int64 SaveTotalSize=TotalCopiedSize;

			if (!(Flags&FCOPY_COPYTONUL) && Rename)
			{
				int MoveCode=FALSE,AskDelete;

				if (strDestFSName == L"NWFS" && !Append &&
				        DestAttr!=INVALID_FILE_ATTRIBUTES && !SameName)
				{
					os::DeleteFile(strDestPath); //BUGBUG
				}

				if (!Append)
				{
					string strSrcFullName;
					ConvertNameToFull(Src,strSrcFullName);

					if (NWFS_Attr)
						os::SetFileAttributes(strSrcFullName,SrcData.dwFileAttributes&(~FILE_ATTRIBUTE_READONLY));

					os::FAR_SECURITY_DESCRIPTOR sd;
					IsSetSecuty=FALSE;

					// ��� Move ��� ���������� ������ ������� ��������, ����� �������� ��� ���������
					if (Rename && !(Flags&(FCOPY_COPYSECURITY|FCOPY_LEAVESECURITY)))
					{
						if (CmpFullPath(Src,strDest)) // � �������� ������ �������� ������ �� ������
							IsSetSecuty=FALSE;
						else if (!os::fs::exists(strDest)) // ���� �������� ���...
						{
							string strDestFullName;

							// ...�������� ��������� ��������
							if (GetSecurity(GetParentFolder(strDest,strDestFullName), sd))
								IsSetSecuty=TRUE;
						}
						else if (GetSecurity(strDest, sd)) // ����� �������� ��������� Dest`�
							IsSetSecuty=TRUE;
					}

					if (strDestFSName == L"NWFS")
						MoveCode=os::MoveFile(strSrcFullName,strDestPath);
					else
						MoveCode=os::MoveFileEx(strSrcFullName,strDestPath,SameName ? MOVEFILE_COPY_ALLOWED:MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING);

					if (!MoveCode)
					{
						int MoveLastError=GetLastError();

						if (NWFS_Attr)
							os::SetFileAttributes(strSrcFullName,SrcData.dwFileAttributes);

						if (MoveLastError==ERROR_NOT_SAME_DEVICE)
							return COPY_FAILURE;

						SetLastError(MoveLastError);
						Global->CatchError();
					}
					else
					{
						if (IsSetSecuty)
							SetSecurity(strDestPath, sd);
					}

					if (NWFS_Attr)
						os::SetFileAttributes(strDestPath,SrcData.dwFileAttributes);

					if (ShowTotalCopySize && MoveCode)
					{
						TotalCopiedSize+=SrcData.nFileSize;
						CP->SetTotalProgressValue(TotalCopiedSize,TotalCopySize);
					}

					AskDelete=0;
				}
				else
				{
					do
					{
						DWORD Attr=INVALID_FILE_ATTRIBUTES;
						CopyCode=ShellCopyFile(Src,SrcData,strDestPath,Attr,Append);
					}
					while (CopyCode==COPY_RETRY);

					switch (CopyCode)
					{
						case COPY_SUCCESS:
							MoveCode=TRUE;
							break;
						case COPY_FAILUREREAD:
						case COPY_FAILURE:
							MoveCode=FALSE;
							break;
						case COPY_CANCEL:
							return COPY_CANCEL;
						case COPY_NEXT:
							return COPY_NEXT;
					}

					AskDelete=1;
				}

				if (MoveCode)
				{
					if (DestAttr==INVALID_FILE_ATTRIBUTES || !(DestAttr & FILE_ATTRIBUTE_DIRECTORY))
					{
						if (PointToName(strDestPath)==strDestPath.data())
							strRenamedName = strDestPath;
						else
							strCopiedName = PointToName(strDestPath);
					}

					if (IsDriveTypeCDROM(SrcDriveType) && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
						ShellSetAttr(strDestPath,SrcData.dwFileAttributes & (~FILE_ATTRIBUTE_READONLY));

					TotalFiles++;

					if (AskDelete && DeleteAfterMove(Src,SrcData.dwFileAttributes)==COPY_CANCEL)
						return COPY_CANCEL;

					return COPY_SUCCESS_MOVE;
				}
			}
			else
			{
				do
				{
					CopyCode=ShellCopyFile(Src,SrcData,strDestPath,DestAttr,Append);
				}
				while (CopyCode==COPY_RETRY);

				if (CopyCode==COPY_SUCCESS)
				{
					strCopiedName = PointToName(strDestPath);

					if (!(Flags&FCOPY_COPYTONUL))
					{
						if (IsDriveTypeCDROM(SrcDriveType) && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
							ShellSetAttr(strDestPath,SrcData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);

						if (DestAttr!=INVALID_FILE_ATTRIBUTES && !StrCmpI(strCopiedName, DestData.strFileName) &&
						        strCopiedName != DestData.strFileName)
							os::MoveFile(strDestPath,strDestPath); //???
					}

					TotalFiles++;

					if (DestAttr!=INVALID_FILE_ATTRIBUTES && Append)
						os::SetFileAttributes(strDestPath,DestAttr);

					return COPY_SUCCESS;
				}
				else if (CopyCode==COPY_CANCEL || CopyCode==COPY_NEXT)
				{
					if (DestAttr!=INVALID_FILE_ATTRIBUTES && Append)
						os::SetFileAttributes(strDestPath,DestAttr);

					return (COPY_CODES)CopyCode;
				}

				if (DestAttr!=INVALID_FILE_ATTRIBUTES && Append)
					os::SetFileAttributes(strDestPath,DestAttr);
			}

			//????
			if (CopyCode == COPY_FAILUREREAD)
				return COPY_FAILURE;

			//????
			string strMsg1, strMsg2;
			LNGID MsgMCannot=(Flags&FCOPY_LINK) ? MCannotLink: (Flags&FCOPY_MOVE) ? MCannotMove: MCannotCopy;
			strMsg1 = Src;
			strMsg2 = strDestPath;
			InsertQuote(strMsg1);
			InsertQuote(strMsg2);

			int MsgCode;
			if ((SrcData.dwFileAttributes&FILE_ATTRIBUTE_ENCRYPTED))
			{
				if (SkipEncMode != -1)
				{
					MsgCode = SkipEncMode;

					if (SkipEncMode == 1)
						Flags |= FCOPY_DECRYPTED_DESTINATION;
				}
				else
				{
					// Better to set it always, just in case
					//if (GetLastError() == ERROR_ACCESS_DENIED)
					{
						SetLastError(ERROR_ENCRYPTION_FAILED);
					}
					Global->CatchError();

					MsgCode = Message(MSG_WARNING | MSG_ERRORTYPE, 5, MSG(MError),
						MSG(MsgMCannot),
						strMsg1.data(),
						MSG(MCannotCopyTo),
						strMsg2.data(),
						MSG(MCopyDecrypt),
						MSG(MCopyDecryptAll),
						MSG(MCopySkip),
						MSG(MCopySkipAll),
						MSG(MCopyCancel));
				}
				switch (MsgCode)
				{
				case 1:
					SkipEncMode = 1;
				case 0:
					Flags |= FCOPY_DECRYPTED_DESTINATION;
					break;

				case 3:
					SkipEncMode = 3;
				case 2:
					return COPY_NEXT;

				case -1:
				case -2:
				case 4:
					return COPY_CANCEL;
				}
			}
			else
			{
				if (SkipMode!=-1)
					MsgCode=SkipMode;
				else
				{
					Global->CatchError();
					MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,4,MSG(MError),
									MSG(MsgMCannot),
									strMsg1.data(),
									MSG(MCannotCopyTo),
									strMsg2.data(),
									MSG(MCopyRetry),MSG(MCopySkip),
									MSG(MCopySkipAll),MSG(MCopyCancel));
				}

				switch (MsgCode)
				{
				case  1:
					return COPY_NEXT;
				case  2:
					SkipMode=1;
					return COPY_NEXT;
				case -1:
				case -2:
				case  3:
					return COPY_CANCEL;
				}
			}
			TotalCopiedSize=SaveTotalSize;
			int RetCode = COPY_CANCEL;
			string strNewName;

			if (!AskOverwrite(SrcData,Src,strDestPath,DestAttr,SameName,Rename,((Flags&FCOPY_LINK)?0:1),Append,strNewName,RetCode))
				return (COPY_CODES)RetCode;

			if (RetCode==COPY_RETRY)
			{
				strDest=strNewName;

				if (CutToSlash(strNewName) && !os::fs::exists(strNewName))
				{
					CreatePath(strNewName);
				}

				return COPY_RETRY;
			}
		}
	}
}


// �������� ���������� ��������� �� ������
COPY_CODES ShellCopy::CheckStreams(const string& Src,const string& DestPath)
{

#if 0
	int AscStreams=(Flags&FCOPY_STREAMSKIP)?2:((Flags&FCOPY_STREAMALL)?0:1);

	if (!(Flags&FCOPY_USESYSTEMCOPY) && AscStreams)
	{
		int CountStreams=EnumNTFSStreams(Src,nullptr,nullptr);

		if (CountStreams > 1 ||
		        (CountStreams >= 1 && (GetFileAttributes(Src)&FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
		{
			if (AscStreams == 2)
			{
				return COPY_NEXT);
			}

			SetMessageHelp("WarnCopyStream");
			//char SrcFullName[NM];
			//ConvertNameToFull(Src,SrcFullName, sizeof(SrcFullName));
			//TruncPathStr(SrcFullName,ScrX-16);
			int MsgCode=Message(MSG_WARNING,5,MSG(MWarning),
			                    MSG(MCopyStream1),
			                    MSG(CanCreateHardLinks(DestPath,nullptr)?MCopyStream2:MCopyStream3),
			                    MSG(MCopyStream4),"\1",//SrcFullName,"\1",
			                    MSG(MCopyResume),MSG(MCopyOverwriteAll),MSG(MCopySkipOvr),MSG(MCopySkipAllOvr),MSG(MCopyCancelOvr));

			switch (MsgCode)
			{
				case 0: break;
				case 1: Flags|=FCOPY_STREAMALL; break;
				case 2: return COPY_NEXT);
				case 3: Flags|=FCOPY_STREAMSKIP; return COPY_NEXT);
				default:
					return COPY_CANCEL;
			}
		}
	}

#endif
	return COPY_SUCCESS;
}

int ShellCopy::DeleteAfterMove(const string& Name,DWORD Attr)
{
	string FullName;
	ConvertNameToFull(Name, FullName);
	if (Attr & FILE_ATTRIBUTE_READONLY)
	{
		int MsgCode;

		if (!Global->Opt->Confirm.RO)
			ReadOnlyDelMode=1;

		if (ReadOnlyDelMode!=-1)
			MsgCode=ReadOnlyDelMode;
		else
			MsgCode=Message(MSG_WARNING,5,MSG(MWarning),
			                MSG(MCopyFileRO),FullName.data(),MSG(MCopyAskDelete),
			                MSG(MCopyDeleteRO),MSG(MCopyDeleteAllRO),
			                MSG(MCopySkipRO),MSG(MCopySkipAllRO),MSG(MCopyCancelRO));

		switch (MsgCode)
		{
			case 1:
				ReadOnlyDelMode=1;
				break;
			case 2:
				return COPY_NEXT;
			case 3:
				ReadOnlyDelMode=3;
				return COPY_NEXT;
			case -1:
			case -2:
			case 4:
				return COPY_CANCEL;
		}

		os::SetFileAttributes(FullName,FILE_ATTRIBUTE_NORMAL);
	}

	while ((Attr&FILE_ATTRIBUTE_DIRECTORY)?!os::RemoveDirectory(FullName):!os::DeleteFile(FullName))
	{
		Global->CatchError();
		int MsgCode;

		if (SkipDeleteMode!=-1)
			MsgCode=SkipDeleteMode;
		else
			MsgCode=OperationFailed(FullName, MError, MSG(MCannotDeleteFile));

		switch (MsgCode)
		{
			case 1:
				return COPY_NEXT;
			case 2:
				SkipDeleteMode=1;
				return COPY_NEXT;
			case -1:
			case -2:
			case 3:
				return COPY_CANCEL;
		}
	}

	return COPY_SUCCESS;
}



int ShellCopy::ShellCopyFile(const string& SrcName,const os::FAR_FIND_DATA &SrcData,
                             string &strDestName,DWORD &DestAttr,int Append)
{
	OrigScrX=ScrX;
	OrigScrY=ScrY;

	if ((Flags&FCOPY_LINK))
	{
		if (RPT==RP_HARDLINK)
		{
			os::DeleteFile(strDestName); //BUGBUG
			return MkHardLink(SrcName,strDestName)? COPY_SUCCESS : COPY_FAILURE;
		}
		else
		{
			return MkSymLink(SrcName, strDestName, RPT)? COPY_SUCCESS : COPY_FAILURE;
		}
	}

	if ((SrcData.dwFileAttributes&FILE_ATTRIBUTE_ENCRYPTED) &&
	        !CheckDisksProps(SrcName,strDestName,CHECKEDPROPS_ISDST_ENCRYPTION)
	   )
	{
		int MsgCode;

		if (SkipEncMode!=-1)
		{
			MsgCode=SkipEncMode;

			if (SkipEncMode == 1)
				Flags|=FCOPY_DECRYPTED_DESTINATION;
		}
		else
		{
			string strSrcName(SrcName);
			InsertQuote(strSrcName);
			MsgCode = Message(MSG_WARNING, MSG(MWarning),
				make_vector<string>(MSG(MCopyEncryptWarn1), strSrcName, MSG(MCopyEncryptWarn2), MSG(MCopyEncryptWarn3)),
				make_vector<string>(MSG(MCopyIgnore), MSG(MCopyIgnoreAll), MSG(MCopyCancel)),
				L"WarnCopyEncrypt");
		}

		switch (MsgCode)
		{
			case  0:
				_LOGCOPYR(SysLog(L"return COPY_NEXT -> %d",__LINE__));
				Flags|=FCOPY_DECRYPTED_DESTINATION;
				break;//return COPY_NEXT;
			case  1:
				SkipEncMode=1;
				Flags|=FCOPY_DECRYPTED_DESTINATION;
				_LOGCOPYR(SysLog(L"return COPY_NEXT -> %d",__LINE__));
				break;//return COPY_NEXT;
			case -1:
			case -2:
			case  2:
				_LOGCOPYR(SysLog(L"return COPY_CANCEL -> %d",__LINE__));
				return COPY_CANCEL;
		}
	}

	if ((Flags & FCOPY_USESYSTEMCOPY) && !Append)
	{
		if (!(SrcData.dwFileAttributes&FILE_ATTRIBUTE_ENCRYPTED) ||
		        ((SrcData.dwFileAttributes&FILE_ATTRIBUTE_ENCRYPTED) &&
		        (IsWindowsXPOrGreater() || !(Flags&(FCOPY_DECRYPTED_DESTINATION))))
		   )
		{
			if (!Global->Opt->CMOpt.CopyOpened)
			{
				os::fs::file SrcFile;
				if (!SrcFile.Open(SrcName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN))
				{
					_LOGCOPYR(SysLog(L"return COPY_FAILURE -> %d if (SrcHandle==INVALID_HANDLE_VALUE)",__LINE__));
					return COPY_FAILURE;
				}

				SrcFile.Close();
			}

			//_LOGCOPYR(SysLog(L"call ShellSystemCopy('%s','%s',%p)",SrcName,DestName,SrcData));
			return ShellSystemCopy(SrcName,strDestName,SrcData);
		}
	}

	os::FAR_SECURITY_DESCRIPTOR sd;
	if ((Flags&FCOPY_COPYSECURITY) && !GetSecurity(SrcName,sd))
		return COPY_CANCEL;

	int OpenMode=FILE_SHARE_READ;

	if (Global->Opt->CMOpt.CopyOpened)
		OpenMode|=FILE_SHARE_WRITE;

	os::fs::file_walker SrcFile;
	bool Opened = SrcFile.Open(SrcName, GENERIC_READ, OpenMode, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);

	if (!Opened && Global->Opt->CMOpt.CopyOpened)
	{
		if (GetLastError() == ERROR_SHARING_VIOLATION)
		{
			Opened = SrcFile.Open(SrcName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
		}
	}

	if (!Opened)
	{
		return COPY_FAILURE;
	}

	os::fs::file DestFile;
	uint64_t AppendPos=0;
	DWORD flags_attrs=0;

	bool CopySparse=false;

	if (!(Flags&FCOPY_COPYTONUL))
	{
		//if (DestAttr!=INVALID_FILE_ATTRIBUTES && !Append) //��� ��� ������ ����������� ������ ����������
		//api::DeleteFile(DestName);
		SECURITY_ATTRIBUTES SecAttr = {sizeof(SecAttr), sd.get(), FALSE};
		flags_attrs = SrcData.dwFileAttributes&(~((Flags&(FCOPY_DECRYPTED_DESTINATION))?FILE_ATTRIBUTE_ENCRYPTED|FILE_FLAG_SEQUENTIAL_SCAN:FILE_FLAG_SEQUENTIAL_SCAN));
		bool DstOpened = DestFile.Open(strDestName, GENERIC_WRITE, FILE_SHARE_READ, (Flags&FCOPY_COPYSECURITY) ? &SecAttr:nullptr, (Append ? OPEN_EXISTING:CREATE_ALWAYS), flags_attrs);
		Flags&=~FCOPY_DECRYPTED_DESTINATION;

		if (!DstOpened)
		{
			_LOGCOPYR(DWORD LastError=GetLastError();)
			SrcFile.Close();
			_LOGCOPYR(SysLog(L"return COPY_FAILURE -> %d CreateFile=-1, LastError=%d (0x%08X)",__LINE__,LastError,LastError));
			return COPY_FAILURE;
		}

		string strDriveRoot;
		GetPathRoot(strDestName,strDriveRoot);

		if (SrcData.dwFileAttributes&FILE_ATTRIBUTE_SPARSE_FILE)
		{
			DWORD VolFlags=0;
			if(os::GetVolumeInformation(strDriveRoot,nullptr,nullptr,nullptr,&VolFlags,nullptr))
			{
				if(VolFlags&FILE_SUPPORTS_SPARSE_FILES)
				{
					DWORD Temp;
					if (DestFile.IoControl(FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &Temp))
					{
						CopySparse=true;
					}
				}
			}
		}

		if (Append)
		{
			if (!DestFile.SetPointer(0,&AppendPos,FILE_END))
			{
				SrcFile.Close();
				DestFile.SetEnd();
				DestFile.Close();
				return COPY_FAILURE;
			}
		}

		// ���� ����� � �������� ������� - ����� �����.
		UINT64 FreeBytes=0;
		if (os::GetDiskSize(strDriveRoot,nullptr,nullptr,&FreeBytes))
		{
			if (FreeBytes>SrcData.nFileSize)
			{
				const auto CurPtr = DestFile.GetPointer();
				DestFile.SetPointer(SrcData.nFileSize, nullptr, FILE_CURRENT);
				DestFile.SetEnd();
				DestFile.SetPointer(CurPtr, nullptr, FILE_BEGIN);
			}
		}
	}

	CP->SetProgressValue(0,0);

	if(SrcFile.InitWalk(CopyBufferSize))
	{
		bool AbortOp = false;

		do
		{
			BOOL IsChangeConsole=OrigScrX != ScrX || OrigScrY != ScrY;

			if (CP->Cancelled())
			{
				AbortOp=true;
			}

			IsChangeConsole=CheckAndUpdateConsole(IsChangeConsole);

			if (IsChangeConsole)
			{
				OrigScrX=ScrX;
				OrigScrY=ScrY;
				PR_ShellCopyMsg();
			}

			CP->SetProgressValue(CurCopiedSize,SrcData.nFileSize);

			if (ShowTotalCopySize)
			{
				CP->SetTotalProgressValue(TotalCopiedSize,TotalCopySize);
			}

			if (AbortOp)
			{
				SrcFile.Close();

				if (!(Flags&FCOPY_COPYTONUL))
				{
					if (Append)
					{
						DestFile.SetPointer(AppendPos,nullptr,FILE_BEGIN);
					}

					DestFile.SetEnd();
					DestFile.Close();

					if (!Append)
					{
						os::SetFileAttributes(strDestName,FILE_ATTRIBUTE_NORMAL);
						os::DeleteFile(strDestName); //BUGBUG
					}
				}

				return COPY_CANCEL;
			}

			size_t BytesRead, BytesWritten;
			while (!SrcFile.Read(CopyBuffer.get(), SrcFile.GetChunkSize(), BytesRead))
			{
				Global->CatchError();
				int MsgCode = Message(MSG_WARNING|MSG_ERRORTYPE,2,MSG(MError),
										MSG(MCopyReadError),SrcName.data(),
										MSG(MRetry),MSG(MCancel));
				PR_ShellCopyMsg();

				if (!MsgCode)
					continue;

				DWORD LastError=GetLastError();
				SrcFile.Close();

				if (!(Flags&FCOPY_COPYTONUL))
				{
					if (Append)
					{
						DestFile.SetPointer(AppendPos,nullptr,FILE_BEGIN);
					}

					DestFile.SetEnd();
					DestFile.Close();

					if (!Append)
					{
						os::SetFileAttributes(strDestName,FILE_ATTRIBUTE_NORMAL);
						os::DeleteFile(strDestName); //BUGBUG
					}
				}

				CP->SetProgressValue(0,0);
				SetLastError(LastError);
				Global->CatchError();
				CurCopiedSize = 0; // �������� ������� ��������
				return COPY_FAILURE;
			}

			if (!BytesRead)
			{
				break;
			}

			if (!(Flags&FCOPY_COPYTONUL))
			{
				DestFile.SetPointer(SrcFile.GetChunkOffset() + (Append? AppendPos : 0), nullptr, FILE_BEGIN);
				while (!DestFile.Write(CopyBuffer.get(),BytesRead,BytesWritten,nullptr))
				{
					DWORD LastError=GetLastError();
					Global->CatchError();
					int Split=FALSE,SplitCancelled=FALSE,SplitSkipped=FALSE;

					if ((LastError==ERROR_DISK_FULL || LastError==ERROR_HANDLE_DISK_FULL) &&
						strDestName.size() > 1 && strDestName[1]==L':')
					{
						string strDriveRoot;
						GetPathRoot(strDestName,strDriveRoot);
						UINT64 FreeSize=0;

						if (os::GetDiskSize(strDriveRoot,nullptr,nullptr,&FreeSize))
						{
							if (FreeSize<BytesRead &&
								DestFile.Write(CopyBuffer.get(),(DWORD)FreeSize,BytesWritten,nullptr) &&
								SrcFile.SetPointer(FreeSize-BytesRead,nullptr,FILE_CURRENT))
							{
								DestFile.Close();
								int MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE, MSG(MError),
									make_vector(strDestName),
									make_vector<string>(MSG(MSplit), MSG(MSkip), MSG(MRetry), MSG(MCancel)),
									L"CopyFiles");
								PR_ShellCopyMsg();

								if (MsgCode==2)
								{
									SrcFile.Close();

									if (!Append)
									{
										os::SetFileAttributes(strDestName,FILE_ATTRIBUTE_NORMAL);
										os::DeleteFile(strDestName); //BUGBUG
									}

									return COPY_FAILURE;
								}

								if (!MsgCode)
								{
									Split=TRUE;

									for (;;)
									{
										if (os::GetDiskSize(strDriveRoot,nullptr,nullptr,&FreeSize))
										{
											if (FreeSize<BytesRead)
											{
												int MsgCode2 = Message(MSG_WARNING,2,MSG(MWarning),
													MSG(MCopyErrorDiskFull),strDestName.data(),
													MSG(MRetry),MSG(MCancel));
												PR_ShellCopyMsg();

												if (MsgCode2)
												{
													Split=FALSE;
													SplitCancelled=TRUE;
												}
												else
													continue;
											}
										}
										break;
									}
								}

								if (MsgCode==1)
									SplitSkipped=TRUE;

								if (MsgCode==-1 || MsgCode==3)
									SplitCancelled=TRUE;
							}
						}
					}

					if (Split)
					{
						const auto FilePtr = SrcFile.GetPointer();
						os::FAR_FIND_DATA SplitData=SrcData;
						SplitData.nFileSize-=FilePtr;
						int RetCode = COPY_CANCEL;
						string strNewName;

						if (!AskOverwrite(SplitData, SrcName, strDestName, INVALID_FILE_ATTRIBUTES, FALSE, (Flags&FCOPY_MOVE) != 0, (Flags&FCOPY_LINK) == 0, Append, strNewName, RetCode))
						{
							SrcFile.Close();
							return COPY_CANCEL;
						}

						if (RetCode==COPY_RETRY)
						{
							strDestName=strNewName;

							if (CutToSlash(strNewName) && !os::fs::exists(strNewName))
							{
								CreatePath(strNewName);
							}

							return COPY_RETRY;
						}

						string strDestDir = strDestName;

						if (CutToSlash(strDestDir,true))
							CreatePath(strDestDir);


						if (!DestFile.Open(strDestName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, (Append ? OPEN_EXISTING:CREATE_ALWAYS), SrcData.dwFileAttributes|FILE_FLAG_SEQUENTIAL_SCAN) || (Append && !DestFile.SetPointer(0,nullptr,FILE_END)))
						{
							SrcFile.Close();
							DestFile.Close();
							return COPY_FAILURE;
						}
					}
					else
					{
						if (!SplitCancelled && !SplitSkipped &&
							!Message(MSG_WARNING|MSG_ERRORTYPE,2,MSG(MError),
							MSG(MCopyWriteError),strDestName.data(),MSG(MRetry),MSG(MCancel)))
						{
							continue;
						}

						SrcFile.Close();

						if (Append)
						{
							DestFile.SetPointer(AppendPos,nullptr,FILE_BEGIN);
						}

						DestFile.SetEnd();
						DestFile.Close();

						if (!Append)
						{
							os::SetFileAttributes(strDestName,FILE_ATTRIBUTE_NORMAL);
							os::DeleteFile(strDestName); //BUGBUG
						}

						CP->SetProgressValue(0,0);
						SetLastError(LastError);

						if (SplitSkipped)
							return COPY_NEXT;

						return SplitCancelled? COPY_CANCEL : COPY_FAILURE;
					}

					break;
				}
			}
			else
			{
				BytesWritten=BytesRead; // �� ������� ���������� ���������� ���������� ����
			}

			if (ShowTotalCopySize)
				TotalCopiedSize-=CurCopiedSize;

			CurCopiedSize = SrcFile.GetChunkOffset() + SrcFile.GetChunkSize();

			if (ShowTotalCopySize)
				TotalCopiedSize+=CurCopiedSize;

			CP->SetProgressValue(CurCopiedSize,SrcData.nFileSize);

			if (ShowTotalCopySize)
			{
				CP->SetTotalProgressValue(TotalCopiedSize,TotalCopySize);
			}
		}
		while(SrcFile.Step());
	}

	SrcFile.Close();

	if (!(Flags&FCOPY_COPYTONUL))
	{
		DestFile.SetTime(nullptr, nullptr, &SrcData.ftLastWriteTime, nullptr);

		if (CopySparse)
		{
			INT64 Pos=SrcData.nFileSize;

			if (Append)
				Pos+=AppendPos;

			DestFile.SetPointer(Pos,nullptr,FILE_BEGIN);
			DestFile.SetEnd();
		}

		DestFile.Close();
		// TODO: ����� ������� Compressed???
		Flags&=~FCOPY_DECRYPTED_DESTINATION;

		if (!IsWindowsVistaOrGreater() && IsWindowsServer()) // WS2003-Share SetFileTime BUG
		{
			string strRoot;
			GetPathRoot(strDestName, strRoot);
			int DriveType = FAR_GetDriveType(strRoot, 0);
			if (DriveType == DRIVE_REMOTE)
			{
				if (DestFile.Open(strDestName,GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,flags_attrs))
				{
					DestFile.SetTime(nullptr, nullptr, &SrcData.ftLastWriteTime, nullptr);
					DestFile.Close();
				}
			}
		}
	}

	return COPY_SUCCESS;
}

void ShellCopy::SetDestDizPath(const string& DestPath)
{
	if (!(Flags&FCOPY_DIZREAD))
	{
		ConvertNameToFull(DestPath, strDestDizPath);
		CutToSlash(strDestDizPath);

		if (strDestDizPath.empty())
			strDestDizPath = L".";

		if ((Global->Opt->Diz.UpdateMode==DIZ_UPDATE_IF_DISPLAYED && !SrcPanel->IsDizDisplayed()) ||
		        Global->Opt->Diz.UpdateMode==DIZ_NOT_UPDATE)
			strDestDizPath.clear();

		if (!strDestDizPath.empty())
			DestDiz.Read(strDestDizPath);

		Flags|=FCOPY_DIZREAD;
	}
}

enum WarnDlgItems
{
	WDLG_BORDER,
	WDLG_TEXT,
	WDLG_FILENAME,
	WDLG_SEPARATOR,
	WDLG_SRCFILEBTN,
	WDLG_DSTFILEBTN,
	WDLG_SEPARATOR2,
	WDLG_CHECKBOX,
	WDLG_SEPARATOR3,
	WDLG_OVERWRITE,
	WDLG_SKIP,
	WDLG_RENAME,
	WDLG_APPEND,
	WDLG_CANCEL,
};

enum
{
 DM_OPENVIEWER = DM_USER+33,
};


struct file_names_for_overwrite_dialog
{
	const string* Src;
	string* Dest;
	string* DestPath;
};

intptr_t ShellCopy::WarnDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2)
{
	switch (Msg)
	{
		case DM_OPENVIEWER:
		{
			auto WFN = reinterpret_cast<file_names_for_overwrite_dialog*>(Dlg->SendMessage(DM_GETDLGDATA, 0, 0));

			if (WFN)
			{
				LPCWSTR ViewName=nullptr;
				switch (Param1)
				{
					case WDLG_SRCFILEBTN:
						ViewName=WFN->Src->data();
						break;
					case WDLG_DSTFILEBTN:
						ViewName=WFN->Dest->data();
						break;
				}

				NamesList List;
				List.AddName(*WFN->Src);
				List.AddName(*WFN->Dest);
				List.SetCurName(*(Param1 == WDLG_SRCFILEBTN? WFN->Src : WFN->Dest));

				auto Viewer = FileViewer::create(ViewName, FALSE, FALSE, TRUE, -1, nullptr, &List, false);
				Global->WindowManager->ExecuteModal(Viewer);
				Global->WindowManager->ProcessKey(Manager::Key(KEY_CONSOLE_BUFFER_RESIZE));
			}
		}
		break;
		case DN_CTLCOLORDLGITEM:
		{
			if (Param1==WDLG_FILENAME)
			{
				FarColor Color=colors::PaletteColorToFarColor(COL_WARNDIALOGTEXT);
				FarDialogItemColors* Colors = static_cast<FarDialogItemColors*>(Param2);
				Colors->Colors[0] = Color;
				Colors->Colors[2] = Color;
			}
		}
		break;
		case DN_BTNCLICK:
		{
			switch (Param1)
			{
				case WDLG_SRCFILEBTN:
				case WDLG_DSTFILEBTN:
					Dlg->SendMessage(DM_OPENVIEWER,Param1,0);
					break;
				case WDLG_RENAME:
				{
					auto WFN = reinterpret_cast<file_names_for_overwrite_dialog*>(Dlg->SendMessage(DM_GETDLGDATA, 0, 0));
					string strDestName = *WFN->Dest;
					GenerateName(strDestName, WFN->DestPath->data());

					if (Dlg->SendMessage(DM_GETCHECK,WDLG_CHECKBOX,0)==BSTATE_UNCHECKED)
					{
						int All=BSTATE_UNCHECKED;

						if (GetString(MSG(MCopyRenameTitle),MSG(MCopyRenameText),nullptr,strDestName.data(), *WFN->Dest,L"CopyAskOverwrite",FIB_BUTTONS|FIB_NOAMPERSAND|FIB_EXPANDENV|FIB_CHECKBOX,&All,MSG(MCopyRememberChoice)))
						{
							if (All!=BSTATE_UNCHECKED)
							{
								*WFN->DestPath = *WFN->Dest;
								CutToSlash(*WFN->DestPath);
							}

							Dlg->SendMessage(DM_SETCHECK,WDLG_CHECKBOX,ToPtr(All));
						}
						else
						{
							return TRUE;
						}
					}
					else
					{
						*WFN->Dest=strDestName;
					}
				}
				break;
			}
		}
		break;
		case DN_CONTROLINPUT:
		{
			auto record = static_cast<const INPUT_RECORD*>(Param2);
			if (record->EventType==KEY_EVENT)
			{
				int key = InputRecordToKey(record);
				if ((Param1==WDLG_SRCFILEBTN || Param1==WDLG_DSTFILEBTN) && key==KEY_F3)
				{
					Dlg->SendMessage(DM_OPENVIEWER,Param1,0);
				}
			}
		}
		break;

		default:
			break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

int ShellCopy::AskOverwrite(const os::FAR_FIND_DATA &SrcData,
                            const string& SrcName,
                            const string& DestName, DWORD DestAttr,
                            int SameName,int Rename,int AskAppend,
                            int &Append,string &strNewName,int &RetCode)
{
	enum
	{
		WARN_DLG_HEIGHT=13,
		WARN_DLG_WIDTH=76,
	};
	string qDst(DestName);
	QuoteLeadingSpace(qDst);
	FarDialogItem WarnCopyDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,WARN_DLG_WIDTH-4,WARN_DLG_HEIGHT-2,0,nullptr,nullptr,0,MSG(MWarning)},
		{DI_TEXT,5,2,WARN_DLG_WIDTH-6,2,0,nullptr,nullptr,DIF_CENTERTEXT,MSG(MCopyFileExist)},
		{DI_EDIT,5,3,WARN_DLG_WIDTH-6,3,0,nullptr,nullptr,DIF_READONLY,qDst.data()},
		{DI_TEXT,-1,4,0,4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,5,5,WARN_DLG_WIDTH-6,5,0,nullptr,nullptr,DIF_BTNNOCLOSE|DIF_NOBRACKETS,L""},
		{DI_BUTTON,5,6,WARN_DLG_WIDTH-6,6,0,nullptr,nullptr,DIF_BTNNOCLOSE|DIF_NOBRACKETS,L""},
		{DI_TEXT,-1,7,0,7,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_CHECKBOX,5,8,0,8,0,nullptr,nullptr,DIF_FOCUS,MSG(MCopyRememberChoice)},
		{DI_TEXT,-1,9,0,9,0,nullptr,nullptr,DIF_SEPARATOR,L""},

		{DI_BUTTON,0,10,0,10,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MCopyOverwrite)},
		{DI_BUTTON,0,10,0,10,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCopySkipOvr)},
		{DI_BUTTON,0,10,0,10,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCopyRename)},
		{DI_BUTTON,0,10,0,10,0,nullptr,nullptr,DIF_CENTERGROUP|(AskAppend?0:(DIF_DISABLE|DIF_HIDDEN)),MSG(MCopyAppend)},
		{DI_BUTTON,0,10,0,10,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCopyCancelOvr)},
	};
	os::FAR_FIND_DATA DestData;
	int DestDataFilled=FALSE;
	Append=FALSE;

	if ((Flags&FCOPY_COPYTONUL))
	{
		RetCode=COPY_NEXT;
		return TRUE;
	}

	if (DestAttr==INVALID_FILE_ATTRIBUTES)
		if ((DestAttr=os::GetFileAttributes(DestName))==INVALID_FILE_ATTRIBUTES)
			return TRUE;

	if (DestAttr & FILE_ATTRIBUTE_DIRECTORY)
		return TRUE;

	string strDestName = DestName;

	{
		int MsgCode = OvrMode;

		if (OvrMode == -1)
		{
			int Type;

			if ((!Global->Opt->Confirm.Copy && !Rename) || (!Global->Opt->Confirm.Move && Rename) ||
				SameName || (Type = os::GetFileTypeByName(DestName)) == FILE_TYPE_CHAR ||
				Type == FILE_TYPE_PIPE || (Flags&FCOPY_OVERWRITENEXT))
				MsgCode = 1;
			else
			{
				DestData.Clear();
				os::GetFindDataEx(DestName, DestData);
				DestDataFilled = TRUE;

				if ((Flags&FCOPY_ONLYNEWERFILES))
				{
					// ������� �����
					if (DestData.ftLastWriteTime < SrcData.ftLastWriteTime)
						MsgCode = 0;
					else
						MsgCode = 2;
				}
				else
				{
					FormatString strSrcFileStr, strDestFileStr;
					unsigned __int64 SrcSize = SrcData.nFileSize;
					FILETIME SrcLastWriteTime = SrcData.ftLastWriteTime;
					if (Flags&FCOPY_COPYSYMLINKCONTENTS && SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)
					{
						string RealName;
						ConvertNameToReal(SrcName, RealName);
						os::FAR_FIND_DATA FindData;
						os::GetFindDataEx(RealName, FindData);
						SrcSize = FindData.nFileSize;
						SrcLastWriteTime = FindData.ftLastWriteTime;

					}
					FormatString strSrcSizeText;
					strSrcSizeText << SrcSize;
					unsigned __int64 DestSize = DestData.nFileSize;
					FormatString strDestSizeText;
					strDestSizeText << DestSize;
					string strDateText, strTimeText;
					ConvertDate(SrcLastWriteTime, strDateText, strTimeText, 8, FALSE, FALSE, TRUE);
					strSrcFileStr << fmt::LeftAlign() << fmt::MinWidth(17) << MSG(MCopySource) << L" " << fmt::ExactWidth(25) << strSrcSizeText << L" " << strDateText << L" " << strTimeText;
					ConvertDate(DestData.ftLastWriteTime, strDateText, strTimeText, 8, FALSE, FALSE, TRUE);
					strDestFileStr << fmt::LeftAlign() << fmt::MinWidth(17) << MSG(MCopyDest) << L" " << fmt::ExactWidth(25) << strDestSizeText << L" " << strDateText << L" " << strTimeText;

					WarnCopyDlgData[WDLG_SRCFILEBTN].Data = strSrcFileStr.data();
					WarnCopyDlgData[WDLG_DSTFILEBTN].Data = strDestFileStr.data();
					auto WarnCopyDlg = MakeDialogItemsEx(WarnCopyDlgData);
					string strFullSrcName;
					ConvertNameToFull(SrcName, strFullSrcName);
					file_names_for_overwrite_dialog WFN = { &strFullSrcName, &strDestName, &strRenamedFilesPath };
					auto WarnDlg = Dialog::create(WarnCopyDlg, &ShellCopy::WarnDlgProc, &WFN);
					WarnDlg->SetDialogMode(DMODE_WARNINGSTYLE);
					WarnDlg->SetPosition(-1, -1, WARN_DLG_WIDTH, WARN_DLG_HEIGHT);
					WarnDlg->SetHelp(L"CopyAskOverwrite");
					WarnDlg->SetId(CopyOverwriteId);
					WarnDlg->Process();

					switch (WarnDlg->GetExitCode())
					{
					case WDLG_OVERWRITE:
						MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 1 : 0;
						break;
					case WDLG_SKIP:
						MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 3 : 2;
						break;
					case WDLG_RENAME:
						MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 5 : 4;
						break;
					case WDLG_APPEND:
						MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 7 : 6;
						break;
					case -1:
					case -2:
					case WDLG_CANCEL:
						MsgCode = 8;
						break;
					}
				}
			}
		}

		switch (MsgCode)
		{
		case 1:
			OvrMode = 1;
		case 0:
			break;
		case 3:
			OvrMode = 2;
		case 2:
			RetCode = COPY_NEXT;
			return FALSE;
		case 5:
			OvrMode = 5;
			GenerateName(strDestName, strRenamedFilesPath.data());
		case 4:
			RetCode = COPY_RETRY;
			strNewName = strDestName;
			break;
		case 7:
			OvrMode = 6;
		case 6:
			Append = TRUE;
			break;
		case -1:
		case -2:
		case 8:
			RetCode = COPY_CANCEL;
			return FALSE;
		}
	}

	if (RetCode!=COPY_RETRY)
	{
		if ((DestAttr & FILE_ATTRIBUTE_READONLY) && !(Flags&FCOPY_OVERWRITENEXT))
		{
			int MsgCode=0;

			if (!SameName)
			{
				if (ReadOnlyOvrMode!=-1)
				{
					MsgCode=ReadOnlyOvrMode;
				}
				else
				{
					if (!DestDataFilled)
					{
						DestData.Clear();
						os::GetFindDataEx(DestName,DestData);
					}

					string strDateText,strTimeText;
					FormatString strSrcFileStr, strDestFileStr;
					unsigned __int64 SrcSize = SrcData.nFileSize;
					FormatString strSrcSizeText;
					strSrcSizeText<<SrcSize;
					unsigned __int64 DestSize = DestData.nFileSize;
					FormatString strDestSizeText;
					strDestSizeText<<DestSize;
					ConvertDate(SrcData.ftLastWriteTime,strDateText,strTimeText,8,FALSE,FALSE,TRUE);
					strSrcFileStr<<fmt::LeftAlign()<<fmt::MinWidth(17)<<MSG(MCopySource)<<L" "<<fmt::ExactWidth(25)<<strSrcSizeText<<L" "<<strDateText<<L" "<<strTimeText;
					ConvertDate(DestData.ftLastWriteTime,strDateText,strTimeText,8,FALSE,FALSE,TRUE);
					strDestFileStr<<fmt::LeftAlign()<<fmt::MinWidth(17)<<MSG(MCopyDest)<<L" "<<fmt::ExactWidth(25)<<strDestSizeText<<L" "<<strDateText<<L" "<<strTimeText;
					WarnCopyDlgData[WDLG_SRCFILEBTN].Data=strSrcFileStr.data();
					WarnCopyDlgData[WDLG_DSTFILEBTN].Data=strDestFileStr.data();
					WarnCopyDlgData[WDLG_TEXT].Data=MSG(MCopyFileRO);
					WarnCopyDlgData[WDLG_OVERWRITE].Data=MSG(Append?MCopyAppend:MCopyOverwrite);
					WarnCopyDlgData[WDLG_RENAME].Type=DI_TEXT;
					WarnCopyDlgData[WDLG_RENAME].Data=L"";
					WarnCopyDlgData[WDLG_APPEND].Type=DI_TEXT;
					WarnCopyDlgData[WDLG_APPEND].Data=L"";
					auto WarnCopyDlg = MakeDialogItemsEx(WarnCopyDlgData);
					string strSrcName;
					ConvertNameToFull(SrcData.strFileName,strSrcName);
					file_names_for_overwrite_dialog WFN[] = { &strSrcName, &strDestName };
					auto WarnDlg = Dialog::create(WarnCopyDlg, &ShellCopy::WarnDlgProc, &WFN);
					WarnDlg->SetDialogMode(DMODE_WARNINGSTYLE);
					WarnDlg->SetPosition(-1,-1,WARN_DLG_WIDTH,WARN_DLG_HEIGHT);
					WarnDlg->SetHelp(L"CopyFiles");
					WarnDlg->SetId(CopyReadOnlyId);
					WarnDlg->Process();

					switch (WarnDlg->GetExitCode())
					{
						case WDLG_OVERWRITE:
							MsgCode=WarnCopyDlg[WDLG_CHECKBOX].Selected?1:0;
							break;
						case WDLG_SKIP:
							MsgCode=WarnCopyDlg[WDLG_CHECKBOX].Selected?3:2;
							break;
						case -1:
						case -2:
						case WDLG_CANCEL:
							MsgCode=8;
							break;
					}
				}
			}

			switch (MsgCode)
			{
				case 1:
					ReadOnlyOvrMode=1;
				case 0:
					break;
				case 3:
					ReadOnlyOvrMode=2;
				case 2:
					RetCode=COPY_NEXT;
					return FALSE;
				case -1:
				case -2:
				case 8:
					RetCode=COPY_CANCEL;
					return FALSE;
			}
		}

		if (!SameName && (DestAttr & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)))
			os::SetFileAttributes(DestName,FILE_ATTRIBUTE_NORMAL);
	}

	return TRUE;
}



bool ShellCopy::GetSecurity(const string& FileName, os::FAR_SECURITY_DESCRIPTOR& sd)
{
	if (!os::GetFileSecurity(NTPath(FileName), DACL_SECURITY_INFORMATION, sd))
	{
		if (!SkipSecurityErrors)
		{
			Global->CatchError();
			switch (Message(MSG_WARNING | MSG_ERRORTYPE, 3, MSG(MError),
				MSG(MCannotGetSecurity),
				FileName.data(),
				MSG(MSkip), MSG(MCopySkipAll), MSG(MCancel)))
			{
			case 0:
				break;

			case 1:
				SkipSecurityErrors = true;
				break;

			default:
				return false;
			}
		}
	}
	return true;
}


bool ShellCopy::SetSecurity(const string& FileName, const os::FAR_SECURITY_DESCRIPTOR& sd)
{
	if (!os::SetFileSecurity(NTPath(FileName), DACL_SECURITY_INFORMATION, sd))
	{
		if (!SkipSecurityErrors)
		{
			Global->CatchError();
			switch (Message(MSG_WARNING | MSG_ERRORTYPE, 3, MSG(MError),
				MSG(MCannotSetSecurity),
				FileName.data(),
				MSG(MSkip), MSG(MCopySkipAll), MSG(MCancel)))
			{
			case 0:
				break;

			case 1:
				SkipSecurityErrors = true;
				break;

			default:
				return false;
			}
		}
	}
	return true;
}

static BOOL ShellCopySecuryMsg(const CopyProgress* CP, const string& Name)
{
	if (Name.empty() || CP->SecurityTimeCheck)
	{
		static int Width=30;
		int WidthTemp;
		if (!Name.empty())
		{
			WidthTemp=std::max(static_cast<int>(Name.size()),30);
		}
		else
			Width=WidthTemp=30;

		WidthTemp=std::min(WidthTemp, ScrX/2);
		Width=std::max(Width,WidthTemp);

		string strOutFileName = Name; //??? nullptr ???
		TruncPathStr(strOutFileName,Width);
		CenterStr(strOutFileName, strOutFileName,Width+4);
		Message(0,0,MSG(MMoveDlgTitle),MSG(MCopyPrepareSecury),strOutFileName.data());

		if (CP->Cancelled())
		{
			return FALSE;
		}
	}

	//BUGBUG, not used
	/*
	if (!PreRedrawStack().empty())
	{
		auto item = dynamic_cast<CopyPreRedrawItem*>(PreRedrawStack().top());
		item->name = Name;
	}
	*/
	return TRUE;
}


int ShellCopy::SetRecursiveSecurity(const string& FileName,const os::FAR_SECURITY_DESCRIPTOR& sd)
{
	if (SetSecurity(FileName, sd))
	{
		if (os::fs::is_directory(FileName))
		{
			ScanTree ScTree(true, true, Flags & FCOPY_COPYSYMLINKCONTENTS);
			ScTree.SetFindPath(FileName,L"*",FSCANTREE_FILESFIRST);

			string strFullName;
			os::FAR_FIND_DATA SrcData;
			while (ScTree.GetNextName(&SrcData,strFullName))
			{
				if (!ShellCopySecuryMsg(CP.get(), strFullName))
					break;

				if (!SetSecurity(strFullName, sd))
				{
					return FALSE;
				}
			}
		}

		return TRUE;
	}

	return FALSE;
}



int ShellCopy::ShellSystemCopy(const string& SrcName,const string& DestName,const os::FAR_FIND_DATA &SrcData)
{
	os::FAR_SECURITY_DESCRIPTOR sd;

	if ((Flags&FCOPY_COPYSECURITY) && !GetSecurity(SrcName, sd))
		return COPY_CANCEL;

	CP->SetNames(SrcName,DestName);
	CP->SetProgressValue(0,0);
	TotalCopiedSizeEx=TotalCopiedSize;
	if (!os::CopyFileEx(SrcName, DestName, CopyProgressRoutine, CP.get(), nullptr, Flags&FCOPY_DECRYPTED_DESTINATION? COPY_FILE_ALLOW_DECRYPTED_DESTINATION : 0))
	{
		Flags&=~FCOPY_DECRYPTED_DESTINATION;
		return (GetLastError() == ERROR_REQUEST_ABORTED)? COPY_CANCEL : COPY_FAILURE;
	}

	Flags&=~FCOPY_DECRYPTED_DESTINATION;

	if ((Flags&FCOPY_COPYSECURITY) && !SetSecurity(DestName, sd))
		return COPY_CANCEL;

	return COPY_SUCCESS;
}

DWORD WINAPI CopyProgressRoutine(LARGE_INTEGER TotalFileSize,
                                 LARGE_INTEGER TotalBytesTransferred,LARGE_INTEGER StreamSize,
                                 LARGE_INTEGER StreamBytesTransferred,DWORD dwStreamNumber,
                                 DWORD dwCallbackReason,HANDLE hSourceFile,HANDLE hDestinationFile,
                                 LPVOID lpData)
{
	// // _LOGCOPYR(CleverSysLog clv(L"CopyProgressRoutine"));
	// // _LOGCOPYR(SysLog(L"dwStreamNumber=%d",dwStreamNumber));
	bool Abort = false;
	auto CP = static_cast<CopyProgress*>(lpData);
	if (CP->Cancelled())
	{
		// // _LOGCOPYR(SysLog(L"2='%s'/0x%08X  3='%s'/0x%08X  Flags=0x%08X",(char*)PreRedrawParam.Param2,PreRedrawParam.Param2,(char*)PreRedrawParam.Param3,PreRedrawParam.Param3,PreRedrawParam.Flags));
		Abort=true;
	}

	if (CheckAndUpdateConsole(OrigScrX!=ScrX || OrigScrY!=ScrY))
	{
		OrigScrX=ScrX;
		OrigScrY=ScrY;
		PR_ShellCopyMsg();
	}

	CurCopiedSize = TotalBytesTransferred.QuadPart;
	CP->SetProgressValue(TotalBytesTransferred.QuadPart,TotalFileSize.QuadPart);

	//fix total size
	if(dwStreamNumber == 1)
	{
		TotalCopySize -= StreamSize.QuadPart;
		TotalCopySize += TotalFileSize.QuadPart;
	}

	if (ShowTotalCopySize)
	{
		TotalCopiedSize=TotalCopiedSizeEx+CurCopiedSize;
		CP->SetTotalProgressValue(TotalCopiedSize,TotalCopySize);
	}

	return Abort?PROGRESS_CANCEL:PROGRESS_CONTINUE;
}

bool ShellCopy::CalcTotalSize()
{
	string strSelName, strSelShortName;
	DWORD FileAttr;
	unsigned __int64 FileSize;
	// ��� �������
	os::FAR_FIND_DATA fd;

	auto item = std::make_unique<CopyPreRedrawItem>();
	item->CP = CP.get();
	SCOPED_ACTION(TPreRedrawFuncGuard)(std::move(item));

	TotalCopySize=CurCopiedSize=0;
	TotalFilesToProcess = 0;
	SrcPanel->GetSelName(nullptr,FileAttr);

	while (SrcPanel->GetSelName(&strSelName,FileAttr,&strSelShortName,&fd))
	{
		if ((FileAttr&FILE_ATTRIBUTE_REPARSE_POINT) && !(Flags&FCOPY_COPYSYMLINKCONTENTS))
			continue;

		if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
		{
			{
				DirInfoData Data = {};
				CP->SetScanName(strSelName);
				int __Ret = GetDirInfo(L"", strSelName, Data, getdirinfo_infinite_delay, Filter, (Flags&FCOPY_COPYSYMLINKCONTENTS? GETDIRINFO_SCANSYMLINK : 0) | (UseFilter? GETDIRINFO_USEFILTER : 0));
				FileSize = Data.FileSize;
				if (__Ret <= 0)
				{
					ShowTotalCopySize=false;
					return false;
				}

				if (Data.FileCount > 0)
				{
					TotalCopySize+=FileSize;
					TotalFilesToProcess += Data.FileCount;
				}
			}
		}
		else
		{
			//  ���������� ���������� ������
			if (UseFilter)
			{
				if (!Filter->FileInFilter(fd, nullptr, &fd.strFileName))
					continue;
			}

			FileSize = SrcPanel->GetLastSelectedSize();

			if (FileSize != (unsigned __int64)-1)
			{
				TotalCopySize+=FileSize;
				TotalFilesToProcess++;
			}
		}
	}

	// INFO: ��� ��� ��������, ����� "����� = ����� ������ * ���������� �����"
	TotalCopySize *= CountTarget;
	TotalFilesToProcess *= CountTarget;
	InsertCommas(TotalCopySize,CP->strTotalCopySizeText);
	return true;
}

/*
  �������� ������ SetFileAttributes() ���
  ����������� ����������� ���������
*/
bool ShellCopy::ShellSetAttr(const string& Dest, DWORD Attr)
{
	string strRoot;
	ConvertNameToFull(Dest,strRoot);
	GetPathRoot(strRoot,strRoot);

	if (!os::fs::exists(strRoot)) // �������, ����� ������� ����, �� ��� � �������
	{
		// ... � ���� ������ �������� AS IS
		ConvertNameToFull(Dest,strRoot);
		GetPathRoot(strRoot,strRoot);

		if (!os::fs::exists(strRoot))
		{
			return false;
		}
	}

	DWORD FileSystemFlagsDst=0;
	int GetInfoSuccess=os::GetVolumeInformation(strRoot,nullptr,nullptr,nullptr,&FileSystemFlagsDst,nullptr);

	if (GetInfoSuccess)
	{
		if (!(FileSystemFlagsDst&FILE_FILE_COMPRESSION))
		{
			Attr&=~FILE_ATTRIBUTE_COMPRESSED;
		}

		if (!(FileSystemFlagsDst&FILE_SUPPORTS_ENCRYPTION))
		{
			Attr&=~FILE_ATTRIBUTE_ENCRYPTED;
		}
	}

	if (!os::SetFileAttributes(Dest,Attr))
	{
		return false;
	}

	if ((Attr&FILE_ATTRIBUTE_COMPRESSED) && !(Attr&FILE_ATTRIBUTE_ENCRYPTED))
	{
		int Ret=ESetFileCompression(Dest,1,Attr&(~FILE_ATTRIBUTE_COMPRESSED),SkipMode);

		if (Ret==SETATTR_RET_ERROR)
		{
			return false;
		}
		else if (Ret==SETATTR_RET_SKIPALL)
		{
			this->SkipMode=SETATTR_RET_SKIP;
		}
	}

	// ��� �����������/�������� ���������� FILE_ATTRIBUTE_ENCRYPTED
	// ��� ��������, ���� �� ����
	if (GetInfoSuccess && FileSystemFlagsDst&FILE_SUPPORTS_ENCRYPTION && Attr&FILE_ATTRIBUTE_ENCRYPTED && Attr&FILE_ATTRIBUTE_DIRECTORY)
	{
		int Ret=ESetFileEncryption(Dest,1,0,SkipMode);

		if (Ret==SETATTR_RET_ERROR)
		{
			return false;
		}
		else if (Ret==SETATTR_RET_SKIPALL)
		{
			SkipMode=SETATTR_RET_SKIP;
		}
	}

	return true;
}
