/*
filelist.cpp

�������� ������ - ����� �������
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
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

#include "filelist.hpp"
#include "global.hpp"
#include "fn.hpp"
#include "flink.hpp"
#include "keys.hpp"
#include "macroopcode.hpp"
#include "lang.hpp"
#include "ctrlobj.hpp"
#include "filefilter.hpp"
#include "dialog.hpp"
#include "vmenu.hpp"
#include "cmdline.hpp"
#include "manager.hpp"
#include "filepanels.hpp"
#include "help.hpp"
#include "fileedit.hpp"
#include "namelist.hpp"
#include "fileview.hpp"
#include "copy.hpp"
#include "history.hpp"
#include "qview.hpp"
#include "rdrwdsk.hpp"
#include "plognmn.hpp"
#include "scrbuf.hpp"
#include "CFileMask.hpp"

extern struct PanelViewSettings ViewSettingsArray[];

static int _cdecl SortList(const void *el1,const void *el2);
int _cdecl SortSearchList(const void *el1,const void *el2);

static int ListSortMode,ListSortOrder,ListSortGroups,ListSelectedFirst;
static int ListPanelMode,ListCaseSensitive,ListNumericSort;
static HANDLE hSortPlugin;

enum SELECT_MODES {SELECT_INVERT,SELECT_INVERTALL,SELECT_ADD,SELECT_REMOVE,
    SELECT_ADDEXT,SELECT_REMOVEEXT,SELECT_ADDNAME,SELECT_REMOVENAME};

FileList::FileList()
{
  _OT(SysLog(L"[%p] FileList::FileList()", this));
  {
    const wchar_t *data=UMSG(MPanelBracketsForLongName);
    if(StrLength(data)>1)
    {
      *openBracket=data[0];
      *closeBracket=data[1];
    }
    else
    {
      *openBracket=L'{';
      *closeBracket=L'}';
    }
    openBracket[1]=closeBracket[1]=0;
  }

  Type=FILE_PANEL;
  FarGetCurDir(strCurDir);
  hPlugin=INVALID_HANDLE_VALUE;
  Filter=NULL;
  ListData=NULL;
  FileCount=0;
  UpperFolderTopFile=0;
  CurTopFile=CurFile=0;
  LastCurFile=-1;
  ShowShortNames=0;
  MouseSelection=0;
  SortMode=BY_NAME;
  SortOrder=1;
  SortGroups=0;
  SelectedFirst=0;
  ViewMode=VIEW_3;
  ViewSettings=ViewSettingsArray[ViewMode];
  NumericSort=0;
  Columns=PreparePanelView(&ViewSettings);
  Height=0;
  PluginsStack=NULL;
  PluginsStackSize=0;
  ShiftSelection=-1;
  hListChange=INVALID_HANDLE_VALUE;
  SelFileCount=0;
  SelFileSize=0;
  TotalFileCount=0;
  TotalFileSize=0;
  FreeDiskSize=0;
  ReturnCurrentFile=FALSE;
  LastUpdateTime=0;
  PluginCommand=-1;
  DataToDeleteCount=0;
  PrevDataStack=NULL;
  PrevDataStackSize=0;
  LeftPos=0;
  UpdateRequired=FALSE;
  AccessTimeUpdateRequired=FALSE;
  DizRead=FALSE;
  InternalProcessKey=FALSE;
  GetSelPosition = 0;
  Is_FS_NTFS=FALSE;
}


FileList::~FileList()
{
  _OT(SysLog(L"[%p] FileList::~FileList()", this));
  CloseChangeNotification();
  for (int I=0;I < PrevDataStackSize; I++)
  {
    DeleteListData(PrevDataStack[I]->PrevListData,PrevDataStack[I]->PrevFileCount);
    delete PrevDataStack[I];
  }
  if(PrevDataStack) xf_free (PrevDataStack);
  DeleteListData(ListData,FileCount);
  if (PanelMode==PLUGIN_PANEL)
    while (PopPlugin(FALSE))
      ;
   delete Filter;
}


void FileList::DeleteListData(struct FileListItem **(&ListData),long &FileCount)
{
  if (ListData==NULL)
    return;

  struct FileListItem *CurPtr;
  for (int I=0;I<FileCount;I++)
  {
    CurPtr = ListData[I];
    if (CurPtr->CustomColumnNumber>0 && CurPtr->CustomColumnData!=NULL)
    {
      for (int J=0; J < CurPtr->CustomColumnNumber; J++)
        delete[] CurPtr->CustomColumnData[J];
      delete[] CurPtr->CustomColumnData;
    }
    if (CurPtr->UserFlags & PPIF_USERDATA)
      xf_free((void *)CurPtr->UserData);
    if (CurPtr->DizText && CurPtr->DeleteDiz)
      delete[] CurPtr->DizText;

    delete CurPtr; //!!!
  }
  xf_free(ListData);
  ListData=NULL;
  FileCount=0;
}




void FileList::Up(int Count)
{
  CurFile-=Count;
  if(CurFile < 0)
    CurFile=0;
  ShowFileList(TRUE);
}


void FileList::Down(int Count)
{
  CurFile+=Count;
  if(CurFile >= FileCount)
    CurFile=FileCount-1;
  ShowFileList(TRUE);
}

void FileList::Scroll(int Count)
{
  CurTopFile+=Count;
  if (Count<0)
    Up(-Count);
  else
    Down(Count);
}

void FileList::CorrectPosition()
{
  if (FileCount==0)
  {
    CurFile=CurTopFile=0;
    return;
  }
  if (CurTopFile+Columns*Height>FileCount)
    CurTopFile=FileCount-Columns*Height;
  if (CurFile<0)
    CurFile=0;
  if (CurFile > FileCount-1)
    CurFile=FileCount-1;
  if (CurTopFile<0)
    CurTopFile=0;
  if (CurTopFile > FileCount-1)
    CurTopFile=FileCount-1;
  if (CurFile<CurTopFile)
    CurTopFile=CurFile;
  if (CurFile>CurTopFile+Columns*Height-1)
    CurTopFile=CurFile-Columns*Height+1;
}


void FileList::SortFileList(int KeepPosition)
{
  if (FileCount>1)
  {
    string strCurName;

    if (SortMode==BY_DIZ)
      ReadDiz();

    ListSortMode=SortMode;
    ListSortOrder=SortOrder;
    ListSortGroups=SortGroups;
    ListSelectedFirst=SelectedFirst;
    ListPanelMode=PanelMode;
    ListCaseSensitive=ViewSettingsArray[ViewMode].CaseSensitiveSort;
    //ListCaseSensitive=ViewSettings.CaseSensitiveSort;
    ListNumericSort=NumericSort;


    if (KeepPosition)
      strCurName = ListData[CurFile]->strName;

    hSortPlugin=(PanelMode==PLUGIN_PANEL) ? hPlugin:NULL;

    far_qsort((void *)ListData,FileCount,sizeof(*ListData),SortList);
    if (KeepPosition)
      GoToFile(strCurName);
  }
}

#if defined(__BORLANDC__)
#pragma intrinsic strcmp
#endif

int _cdecl SortList(const void *el1,const void *el2)
{
  int RetCode;
  __int64 RetCode64;
  const wchar_t *ChPtr1,*ChPtr2;
  struct FileListItem *SPtr1,*SPtr2;
  SPtr1=((FileListItem **)el1)[0];
  SPtr2=((FileListItem **)el2)[0];

  const wchar_t *Name1=PointToName(SPtr1->strName);
  const wchar_t *Name2=PointToName(SPtr2->strName);

  if (Name1[0]==L'.' && Name1[1]==L'.' && Name1[2]==0)
    return(-1);
  if (Name2[0]==L'.' && Name2[1]==L'.' && Name2[2]==0)
    return(1);

  if (ListSortMode==UNSORTED)
  {
    if (ListSelectedFirst && SPtr1->Selected!=SPtr2->Selected)
      return(SPtr1->Selected>SPtr2->Selected ? -1:1);
    return((SPtr1->Position>SPtr2->Position) ? ListSortOrder:-ListSortOrder);
  }

  if ((SPtr1->FileAttr & FA_DIREC) < (SPtr2->FileAttr & FA_DIREC))
    return(1);
  if ((SPtr1->FileAttr & FA_DIREC) > (SPtr2->FileAttr & FA_DIREC))
    return(-1);
  if (ListSelectedFirst && SPtr1->Selected!=SPtr2->Selected)
    return(SPtr1->Selected>SPtr2->Selected ? -1:1);
  if (ListSortGroups && (ListSortMode==BY_NAME || ListSortMode==BY_EXT) &&
      SPtr1->SortGroup!=SPtr2->SortGroup)
    return(SPtr1->SortGroup<SPtr2->SortGroup ? -1:1);

  if (hSortPlugin!=NULL)
  {
    DWORD SaveFlags1,SaveFlags2;
    SaveFlags1=SPtr1->UserFlags;
    SaveFlags2=SPtr2->UserFlags;
    SPtr1->UserFlags=SPtr2->UserFlags=0;
    PluginPanelItem pi1,pi2;
    FileList::FileListToPluginItem(SPtr1,&pi1);
    FileList::FileListToPluginItem(SPtr2,&pi2);
    SPtr1->UserFlags=SaveFlags1;
    SPtr2->UserFlags=SaveFlags2;
    int RetCode=CtrlObject->Plugins.Compare(hSortPlugin,&pi1,&pi2,ListSortMode+(SM_UNSORTED-UNSORTED));
    if (RetCode==-3)
      hSortPlugin=NULL;
    else
      if (RetCode!=-2)
        return(ListSortOrder*RetCode);
  }

  // �� ��������� �������� � ������ "�� ����������" (�����������!)
  if(!(ListSortMode == BY_EXT && !Opt.SortFolderExt && (SPtr1->FileAttr & FA_DIREC)))
  {
    switch(ListSortMode)
    {
      case BY_NAME:
        break;

      case BY_EXT:
        ChPtr1=wcsrchr(*Name1 ? Name1+1:Name1,L'.');
        ChPtr2=wcsrchr(*Name2 ? Name2+1:Name2,L'.');
        if (ChPtr1==NULL && ChPtr2==NULL)
          break;
        if (ChPtr1==NULL)
          return(-ListSortOrder);
        if (ChPtr2==NULL)
          return(ListSortOrder);
        if (*(ChPtr1+1)==L'.')
          return(-ListSortOrder);
        if (*(ChPtr2+1)==L'.')
          return(ListSortOrder);
        RetCode=ListSortOrder*StrCmpI(ChPtr1+1,ChPtr2+1);
        if(RetCode)
          return RetCode;
        break;

      case BY_MTIME:
        if((RetCode64=*(__int64*)&SPtr1->WriteTime - *(__int64*)&SPtr2->WriteTime) == 0)
          break;
        return -ListSortOrder*(RetCode64<0?-1:1);

      case BY_CTIME:
        if((RetCode64=*(__int64*)&SPtr1->CreationTime - *(__int64*)&SPtr2->CreationTime) == 0)
          break;
        return -ListSortOrder*(RetCode64<0?-1:1);

      case BY_ATIME:
        if((RetCode64=*(__int64*)&SPtr1->AccessTime - *(__int64*)&SPtr2->AccessTime) == 0)
          break;
        return -ListSortOrder*(RetCode64<0?-1:1);

      case BY_SIZE:
        return((SPtr1->UnpSize > SPtr2->UnpSize) ? -ListSortOrder : ListSortOrder);

      case BY_DIZ:
        if (SPtr1->DizText==NULL)
          if (SPtr2->DizText==NULL)
            break;
          else
            return(ListSortOrder);
        if (SPtr2->DizText==NULL)
          return(-ListSortOrder);
        RetCode=ListSortOrder*StrCmpI(SPtr1->DizText,SPtr2->DizText);
        if(RetCode)
          return RetCode;
        break;

      case BY_OWNER:
        RetCode=ListSortOrder*StrCmpI(SPtr1->strOwner,SPtr2->strOwner);
        if(RetCode)
          return RetCode;
        break;

      case BY_COMPRESSEDSIZE:
        return((SPtr1->PackSize > SPtr2->PackSize) ? -ListSortOrder : ListSortOrder);

      case BY_NUMLINKS:
        if (SPtr1->NumberOfLinks==SPtr2->NumberOfLinks)
          break;
        return((SPtr1->NumberOfLinks > SPtr2->NumberOfLinks) ? -ListSortOrder : ListSortOrder);
    }
  }

  int NameCmp;

  if(!ListNumericSort)
    NameCmp=ListCaseSensitive?StrCmp(Name1,Name2):StrCmpI(Name1,Name2);
  else
    NameCmp=ListCaseSensitive?NumStrCmp(Name1,Name2):NumStrCmpI(Name1,Name2);
  NameCmp*=ListSortOrder;
  if (NameCmp==0)
    NameCmp=SPtr1->Position>SPtr2->Position ? ListSortOrder:-ListSortOrder;

  return(NameCmp);
}


int _cdecl SortSearchList(const void *el1,const void *el2)
{
  struct FileListItem **SPtr1,**SPtr2;
  SPtr1=(struct FileListItem **)el1;
  SPtr2=(struct FileListItem **)el2;

  return StrCmp(SPtr1[0]->strName,SPtr2[0]->strName);
//  return NumStrcmp(SPtr1->Name,SPtr2->Name);
}
#if defined(__BORLANDC__)
#pragma intrinsic -strcmp
#endif

void FileList::SetFocus()
{
  Panel::SetFocus();
  /* $ 07.04.2002 KM
    ! ������ ��������� ������� ���� ������ �����, �����
      �� ��� ������� ����������� ���� �������. � ������
      ������ ��� �������� ����� ������ � ������� ��������
      ��������� ���������.
  */
  if (!IsRedrawFramesInProcess)
    SetTitle();
}

int FileList::SendKeyToPlugin(DWORD Key,BOOL Pred)
{
  _ALGO(CleverSysLog clv(L"FileList::SendKeyToPlugin()"));
  _ALGO(SysLog(L"Key=%u (0x%08X) Pred=%d",Key,Key,Pred));
  if (PanelMode==PLUGIN_PANEL &&
      (CtrlObject->Macro.IsRecording() == MACROMODE_RECORDING_COMMON || CtrlObject->Macro.IsExecuting() == MACROMODE_EXECUTING_COMMON || CtrlObject->Macro.GetCurRecord(NULL,NULL) == MACROMODE_NOMACRO)
     )
  {
    int VirtKey,ControlState;
    if (TranslateKeyToVK(Key,VirtKey,ControlState))
    {
      _ALGO(SysLog(L"call Plugins.ProcessKey() {"));
      int ProcessCode=CtrlObject->Plugins.ProcessKey(hPlugin,VirtKey|(Pred?PKF_PREPROCESS:0),ControlState);
      _ALGO(SysLog(L"} ProcessCode=%d",ProcessCode));
      ProcessPluginCommand();
      if (ProcessCode)
        return(TRUE);
    }
  }
  return FALSE;
}

__int64 FileList::VMProcess(int OpCode,void *vParam,__int64 iParam)
{
  switch(OpCode)
  {
    case MCODE_C_ROOTFOLDER:
    {
      if (PanelMode==PLUGIN_PANEL)
      {
        struct OpenPluginInfo Info;
        CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
        return (__int64)(*NullToEmpty(Info.CurDir)==0);
      }
      else
      {
        if(!IsLocalRootPath(strCurDir))
        {
          string strDriveRoot;
          GetPathRoot(strCurDir, strDriveRoot);
          return (__int64)(!StrCmpI(strCurDir, strDriveRoot));
        }
        return _i64(1);
      }
    }
    case MCODE_C_EOF:
      return (__int64)(CurFile == FileCount-1);
    case MCODE_C_BOF:
      return (__int64)(CurFile==0);
    case MCODE_C_SELECTED:
      return (__int64)(GetRealSelCount()>1);
    case MCODE_V_ITEMCOUNT:
      return (__int64)(FileCount);
    case MCODE_V_CURPOS:
      return (__int64)(CurFile+1);
  }

  return _i64(0);
}

int FileList::ProcessKey(int Key)
{
  struct FileListItem *CurPtr=NULL;
  int N;
  int CmdLength=CtrlObject->CmdLine->GetLength();

  if (IsVisible())
  {
    if(!InternalProcessKey)
      if ((!(Key==KEY_ENTER||Key==KEY_NUMENTER) && !(Key==KEY_SHIFTENTER||Key==KEY_SHIFTNUMENTER)) || CmdLength==0)
        if(SendKeyToPlugin(Key))
          return TRUE;
  }
  else
  {
    // �� �������, ������� �������� ��� ���������� �������:
    switch(Key)
    {
      case KEY_CTRLF:
      case KEY_CTRLALTF:
      case KEY_CTRLENTER:
      case KEY_CTRLNUMENTER:
      case KEY_CTRLBRACKET:
      case KEY_CTRLBACKBRACKET:
      case KEY_CTRLSHIFTBRACKET:
      case KEY_CTRLSHIFTBACKBRACKET:
      case KEY_CTRL|KEY_SEMICOLON:
      case KEY_CTRL|KEY_ALT|KEY_SEMICOLON:
      case KEY_CTRLALTBRACKET:
      case KEY_CTRLALTBACKBRACKET:
      case KEY_ALTSHIFTBRACKET:
      case KEY_ALTSHIFTBACKBRACKET:
        break;

      case KEY_CTRLG:
      case KEY_SHIFTF4:
      case KEY_F7:
      case KEY_CTRLH:
      case KEY_ALTSHIFTF9:
      case KEY_CTRLN:
        break;

      // ��� �������, ����, ���� Ctrl-F ��������, �� � ��� ������ :-)
/*
      case KEY_CTRLINS:
      case KEY_CTRLSHIFTINS:
      case KEY_CTRLALTINS:
      case KEY_ALTSHIFTINS:
        break;
*/
      default:
        return(FALSE);
    }
  }

  if (!ShiftPressed && ShiftSelection!=-1)
  {
    if (SelectedFirst)
    {
      SortFileList(TRUE);
      ShowFileList(TRUE);
    }
    ShiftSelection=-1;
  }

  if (!InternalProcessKey && (Key>=KEY_RCTRL0 && Key<=KEY_RCTRL9 ||
      Key>=KEY_CTRLSHIFT0 && Key<=KEY_CTRLSHIFT9))
  {

    string strShortcutFolder;
    int SizeFolderNameShortcut=GetShortcutFolderSize(Key);
    //char *ShortcutFolder=NULL;
    string strPluginModule,strPluginFile,strPluginData;
    if (PanelMode==PLUGIN_PANEL)
    {
      PluginHandle *ph = (PluginHandle*)hPlugin;
      strPluginModule = ph->pPlugin->GetModuleName();

      struct OpenPluginInfo Info;
      CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);

      strPluginFile = NullToEmpty(Info.HostFile);
      strShortcutFolder = NullToEmpty(Info.CurDir);
      strPluginData = NullToEmpty(Info.ShortcutData);
    }
    else
    {
      strPluginModule=strPluginFile=strPluginData=L"";
      strShortcutFolder = strCurDir;
    }
    if (SaveFolderShortcut(Key,&strShortcutFolder,&strPluginModule,&strPluginFile,&strPluginData))
    {
      return(TRUE);
    }

      if (GetShortcutFolder(Key,&strShortcutFolder,&strPluginModule,&strPluginFile,&strPluginData))
      {
        if( !strPluginModule.IsEmpty() )
        {
          if( !strPluginFile.IsEmpty() )
          {
            switch(CheckShortcutFolder(&strPluginFile,0,TRUE))
            {
              case 0:
  //              return FALSE;
              case -1:
                return TRUE;
            }
            /* ������������ ������� BugZ#50 */
            string strRealDir;

            strRealDir = strPluginFile;

            if(CutToSlash(strRealDir))
            {
              SetCurDir(strRealDir,TRUE);
              GoToFile(PointToName(strPluginFile));
              // ������ ����.��������.
              if(PrevDataStackSize>0)
              {
                for(--PrevDataStackSize;PrevDataStackSize > 0;PrevDataStackSize--)
                  DeleteListData(PrevDataStack[PrevDataStackSize]->PrevListData,PrevDataStack[PrevDataStackSize]->PrevFileCount);
              }
            }
            /**/

            OpenFilePlugin(strPluginFile,FALSE);
            if ( !strShortcutFolder.IsEmpty() )
              SetCurDir(strShortcutFolder,FALSE);
            Show();
          }
          else
          {
            switch(CheckShortcutFolder(NULL,0,TRUE))
            {
              case 0:
  //              return FALSE;
              case -1:
                return TRUE;
            }
            for (int I=0;I<CtrlObject->Plugins.PluginsCount;I++)
            {
              Plugin *pPlugin = CtrlObject->Plugins.PluginsData[I];

              if ( StrCmpI(pPlugin->GetModuleName(),strPluginModule)==0)
              {
                if (pPlugin->HasOpenPlugin())
                {
                  HANDLE hNewPlugin=CtrlObject->Plugins.OpenPlugin(pPlugin,OPEN_SHORTCUT,(INT_PTR)(const wchar_t *)strPluginData);
                  if (hNewPlugin!=INVALID_HANDLE_VALUE)
                  {
                    int CurFocus=GetFocus();
                    Panel *NewPanel=CtrlObject->Cp()->ChangePanel(this,FILE_PANEL,TRUE,TRUE);
                    NewPanel->SetPluginMode(hNewPlugin,L"",CurFocus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible());

                    if (!strShortcutFolder.IsEmpty())
                      CtrlObject->Plugins.SetDirectory(hNewPlugin,strShortcutFolder,0);
                    NewPanel->Update(0);
                    NewPanel->Show();
                  }
                }
                break;
              }
            }
            /*
            if(I == CtrlObject->Plugins.PluginsCount)
            {
              char Target[NM*2];
              xstrncpy(Target, PluginModule, sizeof(Target)-1);
              TruncPathStr(Target, ScrX-16);
              Message (MSG_WARNING | MSG_ERRORTYPE, 1, MSG(MError), Target, MSG (MNeedNearPath), MSG(MOk))
            }
            */
          }
          return(TRUE);
        }
        switch(CheckShortcutFolder(&strShortcutFolder,SizeFolderNameShortcut,FALSE))
        {
          case 0:
  //          return FALSE;
          case -1:
            return TRUE;
        }
        SetCurDir(strShortcutFolder,TRUE);
        Show();
        return(TRUE);
      }
  }

  /* $ 27.08.2002 SVS
      [*] � ������ � ����� �������� Shift-Left/Right ���������� �������
          Shift-PgUp/PgDn.
  */
  if(Columns==1 && CmdLength==0)
  {
    if(Key == KEY_SHIFTLEFT || Key == KEY_SHIFTNUMPAD4)
      Key=KEY_SHIFTPGUP;
    else if(Key == KEY_SHIFTRIGHT || Key == KEY_SHIFTNUMPAD6)
      Key=KEY_SHIFTPGDN;
  }

  switch(Key)
  {
    case KEY_F1:
    {
      _ALGO(CleverSysLog clv(L"F1"));
      _ALGO(SysLog(L"%s, FileCount=%d",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount));
      if (PanelMode==PLUGIN_PANEL && PluginPanelHelp(hPlugin))
        return(TRUE);
      return(FALSE);
    }

    case KEY_ALTSHIFTF9:
    {
      PluginHandle *ph = (PluginHandle*)hPlugin;

      if (PanelMode==PLUGIN_PANEL)
        CtrlObject->Plugins.ConfigureCurrent(ph->pPlugin, 0);
      else
        CtrlObject->Plugins.Configure();
      return TRUE;
    }

    case KEY_SHIFTSUBTRACT:
    {
      SaveSelection();
      ClearSelection();
      Redraw();
      return(TRUE);
    }

    case KEY_SHIFTADD:
    {
      SaveSelection();
      {
        struct FileListItem *CurPtr;
        for (int I=0; I < FileCount; I++)
        {
          CurPtr = ListData[I];
          if ((CurPtr->FileAttr & FA_DIREC)==0 || Opt.SelectFolders)
            Select(CurPtr,1);
        }
      }
      if (SelectedFirst)
        SortFileList(TRUE);
      Redraw();
      return(TRUE);
    }

    case KEY_ADD:
      SelectFiles(SELECT_ADD);
      return(TRUE);

    case KEY_SUBTRACT:
      SelectFiles(SELECT_REMOVE);
      return(TRUE);

    case KEY_CTRLADD:
      SelectFiles(SELECT_ADDEXT);
      return(TRUE);

    case KEY_CTRLSUBTRACT:
      SelectFiles(SELECT_REMOVEEXT);
      return(TRUE);

    case KEY_ALTADD:
      SelectFiles(SELECT_ADDNAME);
      return(TRUE);

    case KEY_ALTSUBTRACT:
      SelectFiles(SELECT_REMOVENAME);
      return(TRUE);

    case KEY_MULTIPLY:
      SelectFiles(SELECT_INVERT);
      return(TRUE);

    case KEY_CTRLMULTIPLY:
      SelectFiles(SELECT_INVERTALL);
      return(TRUE);

    case KEY_ALTLEFT:     // ��������� ������� ���� � ��������
    case KEY_ALTHOME:     // ��������� ������� ���� � �������� - � ������
      LeftPos=(Key == KEY_ALTHOME)?-0x7fff:LeftPos-1;
      Redraw();
      return(TRUE);

    case KEY_ALTRIGHT:    // ��������� ������� ���� � ��������
    case KEY_ALTEND:     // ��������� ������� ���� � �������� - � �����
      LeftPos=(Key == KEY_ALTEND)?0x7fff:LeftPos+1;
      Redraw();
      return(TRUE);

    case KEY_CTRLINS:      case KEY_CTRLNUMPAD0:
      if (CmdLength>0)
        return(FALSE);
    case KEY_CTRLSHIFTINS: case KEY_CTRLSHIFTNUMPAD0:  // ���������� �����
    case KEY_CTRLALTINS:   case KEY_CTRLALTNUMPAD0:    // ���������� UNC-�����
    case KEY_ALTSHIFTINS:                              // ���������� ������ �����

      //if (FileCount>0 && SetCurPath()) // ?????
      SetCurPath ();

      CopyNames(Key == KEY_CTRLALTINS || Key == KEY_ALTSHIFTINS || Key == KEY_CTRLALTNUMPAD0,
                (Key&(KEY_CTRL|KEY_ALT))==(KEY_CTRL|KEY_ALT));
      return(TRUE);

    /* $ 14.02.2001 VVM
      + Ctrl: ��������� ��� ����� � ��������� ������.
      + CtrlAlt: ��������� UNC-��� ����� � ��������� ������ */
    case KEY_CTRL|KEY_SEMICOLON:
    case KEY_CTRL|KEY_ALT|KEY_SEMICOLON:
    {
      int NewKey = KEY_CTRLF;
      if (Key & KEY_ALT)
        NewKey|=KEY_ALT;

      Panel *SrcPanel = CtrlObject->Cp()->GetAnotherPanel(CtrlObject->Cp()->ActivePanel);
      int OldState = SrcPanel->IsVisible();
      SrcPanel->SetVisible(1);
      SrcPanel->ProcessKey(NewKey);
      SrcPanel->SetVisible(OldState);

      SetCurPath();
      return(TRUE);
    }

    case KEY_CTRLNUMENTER:
    case KEY_CTRLSHIFTNUMENTER:
    case KEY_CTRLENTER:
    case KEY_CTRLSHIFTENTER:
    case KEY_CTRLJ:
    case KEY_CTRLF:
    case KEY_CTRLALTF:  // 29.01.2001 VVM + �� CTRL+ALT+F � ��������� ������ ������������ UNC-��� �������� �����.
    {
      if (FileCount>0 && SetCurPath())
      {
        string strFileName;
        if(Key==KEY_CTRLSHIFTENTER || Key==KEY_CTRLSHIFTNUMENTER)
          _MakePath1(Key,strFileName, L" ");
        else
        {
          int CurrentPath=FALSE;
          CurPtr=ListData[CurFile];

          if ( ShowShortNames && !CurPtr->strShortName.IsEmpty() )
            strFileName = CurPtr->strShortName;
          else
            strFileName = CurPtr->strName;

          if (TestParentFolderName(strFileName))
          {
            if (PanelMode==PLUGIN_PANEL)
              strFileName=L"";
            else
              strFileName.SetLength(1); // "."

            if(Key!=KEY_CTRLALTF)
              Key=KEY_CTRLF;
            CurrentPath=TRUE;
          }
          if (Key==KEY_CTRLF || Key==KEY_CTRLALTF)
          {
            struct OpenPluginInfo Info={0};
            if (PanelMode==PLUGIN_PANEL)
            {
              CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
            }
            if (PanelMode!=PLUGIN_PANEL || (Info.Flags & OPIF_REALNAMES))
              CreateFullPathName(CurPtr->strName,CurPtr->strShortName,CurPtr->FileAttr, strFileName, Key==KEY_CTRLALTF);
            else
            {
              string strFullName = NullToEmpty(Info.CurDir);

              if (Opt.PanelCtrlFRule && ViewSettings.FolderUpperCase)
                strFullName.Upper ();

              if ( !strFullName.IsEmpty() )
                AddEndSlash(strFullName,0);

              if(Opt.PanelCtrlFRule)
              {
                /* $ 13.10.2000 tran
                  �� Ctrl-f ��� ������ �������� �������� �� ������ */
                if ( ViewSettings.FileLowerCase && !(CurPtr->FileAttr & FA_DIREC))
                  strFileName.Lower ();
                if (ViewSettings.FileUpperToLowerCase)
                  if (!(CurPtr->FileAttr & FA_DIREC) && !IsCaseMixed(strFileName))
                     strFileName.Lower();
              }
              strFullName += strFileName;
              strFileName = strFullName;
            }
          }
          if (CurrentPath)
            AddEndSlash(strFileName);

          // ������� ������ �������!
          if(PanelMode==PLUGIN_PANEL && Opt.SubstPluginPrefix && !(Key == KEY_CTRLENTER || Key == KEY_CTRLNUMENTER || Key == KEY_CTRLJ))
          {
            string strPrefix;
            /* $ 19.11.2001 IS ����������� �� �������� :) */
            if(*AddPluginPrefix((FileList *)CtrlObject->Cp()->ActivePanel,strPrefix))
            {
                strPrefix += strFileName;
                strFileName = strPrefix;
            }
          }
          if(Opt.QuotedName&QUOTEDNAME_INSERT)
            QuoteSpace(strFileName);
          strFileName += L" ";
        }
        CtrlObject->CmdLine->InsertString(strFileName);
      }
      return(TRUE);
    }

    case KEY_CTRLALTBRACKET:       // �������� ������� (UNC) ���� �� ����� ������
    case KEY_CTRLALTBACKBRACKET:   // �������� ������� (UNC) ���� �� ������ ������
    case KEY_ALTSHIFTBRACKET:      // �������� ������� (UNC) ���� �� �������� ������
    case KEY_ALTSHIFTBACKBRACKET:  // �������� ������� (UNC) ���� �� ��������� ������
    case KEY_CTRLBRACKET:          // �������� ���� �� ����� ������
    case KEY_CTRLBACKBRACKET:      // �������� ���� �� ������ ������
    case KEY_CTRLSHIFTBRACKET:     // �������� ���� �� �������� ������
    case KEY_CTRLSHIFTBACKBRACKET: // �������� ���� �� ��������� ������
    {
      string strPanelDir;
      if(_MakePath1(Key,strPanelDir, L""))
        CtrlObject->CmdLine->InsertString(strPanelDir);
      return(TRUE);
    }

    case KEY_CTRLA:
    {
      _ALGO(CleverSysLog clv(L"Ctrl-A"));
      if (FileCount>0 && SetCurPath())
      {
        ShellSetFileAttributes(this);
        Show();
      }
      return(TRUE);
    }

    case KEY_CTRLG:
    {
      _ALGO(CleverSysLog clv(L"Ctrl-G"));
      if (PanelMode!=PLUGIN_PANEL ||
          CtrlObject->Plugins.UseFarCommand(hPlugin,PLUGIN_FAROTHER))
        if (FileCount>0 && ApplyCommand())
        {
          Update(UPDATE_KEEP_SELECTION);
          Redraw();
          Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
          AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
          AnotherPanel->Redraw();
        }
      return(TRUE);
    }

    case KEY_CTRLZ:
      if (FileCount>0 && PanelMode==NORMAL_PANEL && SetCurPath())
        DescribeFiles();
      return(TRUE);

    case KEY_CTRLH:
    {
      Opt.ShowHidden=!Opt.ShowHidden;
      Update(UPDATE_KEEP_SELECTION);
      Redraw();
      Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
      AnotherPanel->Update(UPDATE_KEEP_SELECTION);//|UPDATE_SECONDARY);
      AnotherPanel->Redraw();
      return(TRUE);
    }

    case KEY_CTRLM:
    {
      RestoreSelection();
      return(TRUE);
    }

    case KEY_CTRLR:
    {
      Update(UPDATE_KEEP_SELECTION);
      Redraw();
      {
        Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
        if (AnotherPanel->GetType()!=FILE_PANEL)
        {
          AnotherPanel->SetCurDir(strCurDir,FALSE);
          AnotherPanel->Redraw();
        }
      }
      break;
    }

    case KEY_CTRLN:
    {
      ShowShortNames=!ShowShortNames;
      Redraw();
      return(TRUE);
    }

    case KEY_NUMENTER:
    case KEY_SHIFTNUMENTER:
    case KEY_ENTER:
    case KEY_SHIFTENTER:
    {
      _ALGO(CleverSysLog clv(L"Enter/Shift-Enter"));
      _ALGO(SysLog(L"%s, FileCount=%d Key=%s",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount,_FARKEY_ToName(Key)));
      if (CmdLength>0 || FileCount==0)
      {
        // ��� ������� ������� ���� � �� ������ ������... �������� ��� ������
        if(MButtonPressed && CmdLength>0)
        {
          CtrlObject->CmdLine->ProcessKey(Key);
          return(TRUE);
        }
        break;
      }
      ProcessEnter(1,Key==KEY_SHIFTENTER||Key==KEY_SHIFTNUMENTER);
      return(TRUE);
    }

    case KEY_CTRLBACKSLASH:
    {
      _ALGO(CleverSysLog clv(L"Ctrl-\\"));
      _ALGO(SysLog(L"%s, FileCount=%d",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount));
      BOOL NeedChangeDir=TRUE;
      if (PanelMode==PLUGIN_PANEL)// && *PluginsStack[PluginsStackSize-1].HostFile)
      {
        int CheckFullScreen=IsFullScreen();
        struct OpenPluginInfo Info;
        CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
        if (!Info.CurDir || *Info.CurDir == 0)
        {
          ChangeDir(L"..");
          NeedChangeDir=FALSE;
          if(CheckFullScreen)
            CtrlObject->Cp()->GetAnotherPanel(this)->Show();
        }
      }
      if(NeedChangeDir)
        ChangeDir(L"\\");
      Show();
      return(TRUE);
    }

    case KEY_SHIFTF1:
    {
      _ALGO(CleverSysLog clv(L"Shift-F1"));
      _ALGO(SysLog(L"%s, FileCount=%d",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount));
      if (FileCount>0 && PanelMode!=PLUGIN_PANEL && SetCurPath())
        PluginPutFilesToNew();
      return(TRUE);
    }

    case KEY_SHIFTF2:
    {
      _ALGO(CleverSysLog clv(L"Shift-F2"));
      _ALGO(SysLog(L"%s, FileCount=%d",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount));
      if (FileCount>0 && SetCurPath())
      {
        if (PanelMode==PLUGIN_PANEL)
        {
          struct OpenPluginInfo Info;
          CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
          if (Info.HostFile!=NULL && *Info.HostFile!=0)
            ProcessKey(KEY_F5);
          else
            if ( (Info.Flags & OPIF_REALNAMES) == OPIF_REALNAMES )
              PluginHostGetFiles();

          return(TRUE);
        }
        PluginHostGetFiles();
      }
      return(TRUE);
    }

    case KEY_SHIFTF3:
    {
      _ALGO(CleverSysLog clv(L"Shift-F3"));
      _ALGO(SysLog(L"%s, FileCount=%d",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount));
      ProcessHostFile();
      return(TRUE);
    }

    case KEY_F3:
    case KEY_NUMPAD5:      case KEY_SHIFTNUMPAD5:
    case KEY_ALTF3:
    case KEY_CTRLSHIFTF3:
    case KEY_F4:
    case KEY_ALTF4:
    case KEY_SHIFTF4:
    case KEY_CTRLSHIFTF4:
    {
      _ALGO(CleverSysLog clv(L"Edit/View"));
      _ALGO(SysLog(L"%s, FileCount=%d Key=%s",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount,_FARKEY_ToName(Key)));

      struct OpenPluginInfo Info;
      BOOL RefreshedPanel=TRUE;

      if(PanelMode==PLUGIN_PANEL)
        CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
      else
        memset(&Info,0,sizeof(Info));


      if (Key == KEY_NUMPAD5 || Key == KEY_SHIFTNUMPAD5)
        Key=KEY_F3;
      if ((Key==KEY_SHIFTF4 || FileCount>0) && SetCurPath())
      {
        int Edit=(Key==KEY_F4 || Key==KEY_ALTF4 || Key==KEY_SHIFTF4 || Key==KEY_CTRLSHIFTF4);
        BOOL Modaling=FALSE; ///
        int UploadFile=TRUE;
        string strPluginData;
        string strFileName;
        string strShortFileName;

        int PluginMode=PanelMode==PLUGIN_PANEL &&
            !CtrlObject->Plugins.UseFarCommand(hPlugin,PLUGIN_FARGETFILE);

        if (PluginMode)
        {
          if(Info.Flags & OPIF_REALNAMES)
            PluginMode=FALSE;
          else
            strPluginData.Format (L"<%s:%s>",NullToEmpty(Info.HostFile),NullToEmpty(Info.CurDir));
        }

        if(!PluginMode)
          strPluginData=L"";

        int codepage = CP_AUTODETECT;

        if (Key==KEY_SHIFTF4)
        {
          static string strLastFileName=L"";

          do{

            if ( !dlgOpenEditor (strLastFileName, codepage) )
            	return FALSE;

            /*if (!GetString(UMSG(MEditTitle),
                           UMSG(MFileToEdit),
                           L"NewEdit",
                           strLastFileName,
                           strLastFileName,
                           512, //BUGBUG
                           L"Editor",
                           FIB_BUTTONS|FIB_EXPANDENV|FIB_EDITPATH|FIB_ENABLEEMPTY))
              return(FALSE);*/

            if( !strLastFileName.IsEmpty() )
            {
              strFileName = strLastFileName;

              RemoveTrailingSpaces(strFileName);
              Unquote(strFileName);
              ConvertNameToShort(strFileName,strShortFileName);

              if (PathMayBeAbsolute(strFileName))
              {
                PluginMode=FALSE;
              }

              {
                // �������� ���� � �����
                wchar_t *lpwszStart=strFileName.GetBuffer ();

                wchar_t *Ptr = wcsrchr(lpwszStart, L'\\');
                if(Ptr && Ptr != lpwszStart)
                {
                  *Ptr=0;
                  DWORD CheckFAttr=GetFileAttributesW(lpwszStart);
                  if(CheckFAttr == (DWORD)-1)
                  {
                    SetMessageHelp(L"WarnEditorPath");
                    if (Message(MSG_WARNING,2,UMSG(MWarning),
                                UMSG(MEditNewPath1),
                                UMSG(MEditNewPath2),
                                UMSG(MEditNewPath3),
                                UMSG(MHYes),UMSG(MHNo))!=0)

                      return(FALSE);
                  }
                  *Ptr=L'\\';
                }

                strFileName.ReleaseBuffer ();
              }
            }
            else if(PluginMode) // ������ ��� ����� � ������ ������� �� �����������!
            {
              SetMessageHelp(L"WarnEditorPluginName");
              if (Message(MSG_WARNING,2,UMSG(MWarning),
                          UMSG(MEditNewPlugin1),
                          UMSG(MEditNewPath3),UMSG(MCancel))!=0)

                return(FALSE);
            }
            else
              strFileName = UMSG(MNewFileName);

          } while(strFileName.IsEmpty());

        }
        else
        {
          CurPtr=ListData[CurFile];

          if (CurPtr->FileAttr & FA_DIREC)
          {
            if (Edit)
              return ProcessKey(KEY_CTRLA);

            CountDirSize(Info.Flags);
            return(TRUE);
          }

          strFileName = CurPtr->strName;

          if ( !CurPtr->strShortName.IsEmpty () )
            strShortFileName = CurPtr->strShortName;
          else
            strShortFileName = CurPtr->strName;
        }

        string strTempDir, strTempName;

        int UploadFailed=FALSE, NewFile=FALSE;

        if (PluginMode)
        {
          if(!FarMkTempEx(strTempDir))
            return(TRUE);

          CreateDirectoryW(strTempDir,NULL);
          strTempName.Format (L"%s\\%s",(const wchar_t*)strTempDir,(const wchar_t*)PointToName(strFileName));
          if (Key==KEY_SHIFTF4)
          {
            int Pos=FindFile(strFileName);
            if (Pos!=-1)
              CurPtr=ListData[Pos];
            else
            {
              NewFile=TRUE;
              strFileName = strTempName;
            }
          }

          if (!NewFile)
          {
            struct PluginPanelItem PanelItem;
            FileListToPluginItem(CurPtr,&PanelItem);
            if (!CtrlObject->Plugins.GetFile(hPlugin,&PanelItem,strTempDir,strFileName,OPM_SILENT|(Edit ? OPM_EDIT:OPM_VIEW)))
            {
              apiRemoveDirectory(strTempDir);
              return(TRUE);
            }
          }

          ConvertNameToShort(strFileName,strShortFileName);
        }

        /* $ 08.04.2002 IS
           ����, ��������� � ���, ��� ����� ������� ����, ������� ��������� ��
           ������. ���� ���� ������� �� ���������� ������, �� DeleteViewedFile
           ������ ��� ����� false, �.�. ���������� ����� ��� ��� ������.
        */
        bool DeleteViewedFile=PluginMode && !Edit;
        if ( !strFileName.IsEmpty () )
          if (Edit)
          {
            int EnableExternal=((Key==KEY_F4 || Key==KEY_SHIFTF4) && Opt.EdOpt.UseExternalEditor ||
                Key==KEY_ALTF4 && !Opt.EdOpt.UseExternalEditor) && !Opt.strExternalEditor.IsEmpty();
            /* $ 02.08.2001 IS ���������� ���������� ��� alt-f4 */
            BOOL Processed=FALSE;
            if(Key==KEY_ALTF4 &&
               ProcessLocalFileTypes(strFileName,strShortFileName,FILETYPE_ALTEDIT,
               PluginMode))
               Processed=TRUE;
            else if(Key==KEY_F4 &&
               ProcessLocalFileTypes(strFileName,strShortFileName,FILETYPE_EDIT,
               PluginMode))
               Processed=TRUE;

            if (!Processed || Key==KEY_CTRLSHIFTF4)
            {
              if (EnableExternal)
                ProcessExternal(Opt.strExternalEditor,strFileName,strShortFileName,PluginMode);
              else if (PluginMode)
              {
                RefreshedPanel=FrameManager->GetCurrentFrame()->GetType()==MODALTYPE_EDITOR?FALSE:TRUE;

                FileEditor ShellEditor (strFileName,codepage,(Key==KEY_SHIFTF4?FFILEEDIT_CANNEWFILE:0)|FFILEEDIT_DISABLEHISTORY,-1,-1,strPluginData);
                ShellEditor.SetDynamicallyBorn(false);
                FrameManager->EnterModalEV();
                FrameManager->ExecuteModal();//OT
                FrameManager->ExitModalEV();
                /* $ 24.11.2001 IS
                     ���� �� ������� ����� ����, �� �� �����, ��������� ��
                     ��� ���, ��� ����� ������� ��� �� ������ �������.
                */
                UploadFile=ShellEditor.IsFileChanged() || NewFile;
                Modaling=TRUE;///
              }
              else
              {
                NamesList EditList;
                if (!PluginMode)
                {
                  for (int I=0;I<FileCount;I++)
                    if ((ListData[I]->FileAttr & FA_DIREC)==0)
                      EditList.AddName(ListData[I]->strName, ListData[I]->strShortName);

                  EditList.SetCurDir(strCurDir);
                  EditList.SetCurName(strFileName);
                }

                FileEditor *ShellEditor=new FileEditor(strFileName,codepage,(Key==KEY_SHIFTF4?FFILEEDIT_CANNEWFILE:0)|FFILEEDIT_ENABLEF6);
                /* TODO: BUG BUG BUG!!! �������� � ���, ��� ����...
                ���� ���� ������ - �� ShellEditor � ������ �������� �� ��������, ���������� ������ ������
                */
#if 0

                if(ShellEditor->GetExitCode() == XC_OPEN_ERROR) // � ������ ���� � ��������!!!
                  delete ShellEditor;
                else
                {
                  ShellEditor->SetNamesList (&EditList);
                  FrameManager->ExecuteModal();//OT
                }
#else
                ShellEditor->SetNamesList (&EditList);
                FrameManager->ExecuteModal();//OT
#endif
              }
            }

            if (PluginMode && UploadFile)
            {
              struct PluginPanelItem PanelItem;
              string strSaveDir;

              FarGetCurDir(strSaveDir);

              if (GetFileAttributesW(strTempName)==0xffffffff)
              {
                string strFindName;
                string strPath;

                strPath = strTempName;

                CutToSlash(strPath, false);

                strFindName = strPath+L"*";

                HANDLE FindHandle;
                FAR_FIND_DATA_EX FindData;

                bool Done=((FindHandle=apiFindFirstFile(strFindName,&FindData))==INVALID_HANDLE_VALUE);
                while (!Done)
                {
                  if ((FindData.dwFileAttributes & FA_DIREC)==0)
                  {
                    strTempName = strPath+FindData.strFileName;
                    break;
                  }
                  Done=!apiFindNextFile(FindHandle,&FindData);
                }
                FindClose(FindHandle);
              }

              if (FileNameToPluginItem(strTempName,&PanelItem))
              {
                int PutCode=CtrlObject->Plugins.PutFiles(hPlugin,&PanelItem,1,FALSE,OPM_EDIT);
                if (PutCode==1 || PutCode==2)
                  SetPluginModified();
                if (PutCode==0)
                  UploadFailed=TRUE;
              }

              FarChDir(strSaveDir);
            }
          }
          else
          {
            int EnableExternal=(Key==KEY_F3 && Opt.ViOpt.UseExternalViewer ||
                                Key==KEY_ALTF3 && !Opt.ViOpt.UseExternalViewer) &&
                                !Opt.strExternalViewer.IsEmpty();
            /* $ 02.08.2001 IS ���������� ���������� ��� alt-f3 */
            BOOL Processed=FALSE;
            if(Key==KEY_ALTF3 &&
               ProcessLocalFileTypes(strFileName,strShortFileName,FILETYPE_ALTVIEW,PluginMode))
               Processed=TRUE;
            else if(Key==KEY_F3 &&
               ProcessLocalFileTypes(strFileName,strShortFileName,FILETYPE_VIEW,PluginMode))
               Processed=TRUE;

            if (!Processed || Key==KEY_CTRLSHIFTF3)
              if (EnableExternal)
                ProcessExternal(Opt.strExternalViewer,strFileName,strShortFileName,PluginMode);
              else
              {
                NamesList ViewList;
                if (!PluginMode)
                {
                  for (int I=0;I<FileCount;I++)
                    if ((ListData[I]->FileAttr & FA_DIREC)==0)
                      ViewList.AddName(ListData[I]->strName,ListData[I]->strShortName);

                  ViewList.SetCurDir(strCurDir);
                  ViewList.SetCurName(strFileName);
                }
                FileViewer *ShellViewer=new FileViewer(strFileName, TRUE,PluginMode,PluginMode,-1,strPluginData,&ViewList);

                /* $ 08.04.2002 IS
                   ������� DeleteViewedFile, �.�. ���������� ����� ��� ��� ������
                */
                if (PluginMode)
                {
                  ShellViewer->SetTempViewName(strFileName);
                  DeleteViewedFile=false;
                }
                Modaling=FALSE;
              }
          }
        /* $ 08.04.2002 IS
             ��� �����, ������� ���������� �� ���������� ������, ������ ��
             �������������, �.�. ����� �� ���� ����������� ���
        */
        if (PluginMode)
        {
          if (UploadFailed)
            Message(MSG_WARNING,1,UMSG(MError),UMSG(MCannotSaveFile),
                    UMSG(MTextSavedToTemp),strFileName,UMSG(MOk));
          else if(Edit || DeleteViewedFile)
            // ������� ���� ������ ��� ������ ������� ��� � ��������� ��� ��
            // ������� ������, �.�. ���������� ����� ������� ���� ���
            DeleteFileWithFolder(strFileName);
        }
        if (Modaling && (Edit || IsColumnDisplayed(ADATE_COLUMN)) && RefreshedPanel)
        {
          //if (!PluginMode || UploadFile)
          {
            Update(UPDATE_KEEP_SELECTION);
            Redraw();
            Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
            if (AnotherPanel->GetMode()==NORMAL_PANEL)
            {
              AnotherPanel->Update(UPDATE_KEEP_SELECTION);
              AnotherPanel->Redraw();
            }
          }
//          else
//            SetTitle();
        }
        else
          if (PanelMode==NORMAL_PANEL)
            AccessTimeUpdateRequired=TRUE;
      }
      /* $ 15.07.2000 tran
         � ��� �� �������� ����������� �������
         ������ ��� ���� viewer, editor ����� ��� ������� ������������
         */
//      CtrlObject->Cp()->Redraw();
      return(TRUE);
    }

    case KEY_F5:
    case KEY_F6:
    case KEY_ALTF6:
    case KEY_DRAGCOPY:
    case KEY_DRAGMOVE:
    {
      _ALGO(CleverSysLog clv(L"F5/F6/Alt-F6/DragCopy/DragMove"));
      _ALGO(SysLog(L"%s, FileCount=%d Key=%s",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount,_FARKEY_ToName(Key)));
      if (FileCount>0 && SetCurPath())
        ProcessCopyKeys(Key);
      return(TRUE);
    }

    case KEY_ALTF5:  // ������ ��������/��������� �����/��
    {
      _ALGO(CleverSysLog clv(L"Alt-F5"));
      _ALGO(SysLog(L"%s, FileCount=%d",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount));
      // $ 11.03.2001 VVM - ������ ����� pman ������ �� �������� �������.
      if ((PanelMode!=PLUGIN_PANEL) && (Opt.UsePrintManager && CtrlObject->Plugins.FindPlugin(SYSID_PRINTMANAGER)))
         CtrlObject->Plugins.CallPlugin(SYSID_PRINTMANAGER,OPEN_FILEPANEL,0); // printman
      else
        if (FileCount>0 && SetCurPath())
          PrintFiles(this);
      return(TRUE);
    }

    case KEY_SHIFTF5:
    case KEY_SHIFTF6:
    {
      _ALGO(CleverSysLog clv(L"Shift-F5/Shift-F6"));
      _ALGO(SysLog(L"%s, FileCount=%d Key=%s",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount,_FARKEY_ToName(Key)));
      if (FileCount>0 && SetCurPath())
      {
        int OldFileCount=FileCount,OldCurFile=CurFile;
        int OldSelection=ListData[CurFile]->Selected;
        int ToPlugin=0;
        int RealName=PanelMode!=PLUGIN_PANEL;
        ReturnCurrentFile=TRUE;

        if (PanelMode==PLUGIN_PANEL)
        {
          struct OpenPluginInfo Info;
          CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
          RealName=Info.Flags&OPIF_REALNAMES;
        }

        if (RealName)
        {
          ShellCopy ShCopy(this,Key==KEY_SHIFTF6,FALSE,TRUE,TRUE,ToPlugin,NULL);
        }
        else
        {
          ProcessCopyKeys(Key==KEY_SHIFTF5 ? KEY_F5:KEY_F6);
        }
        ReturnCurrentFile=FALSE;
        if (Key!=KEY_SHIFTF5 && FileCount==OldFileCount &&
            CurFile==OldCurFile && OldSelection!=ListData[CurFile]->Selected)
        {
          Select(ListData[CurFile],OldSelection);
          Redraw();
        }
      }
      return(TRUE);
    }

    case KEY_F7:
    {
      _ALGO(CleverSysLog clv(L"F7"));
      _ALGO(SysLog(L"%s, FileCount=%d",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount));
      if (SetCurPath())
      {
        if (PanelMode==PLUGIN_PANEL && !CtrlObject->Plugins.UseFarCommand(hPlugin,PLUGIN_FARMAKEDIRECTORY))
        {
          string strDirName;
          int MakeCode=CtrlObject->Plugins.MakeDirectory(hPlugin,strDirName,0);
          if (!MakeCode)
            Message(MSG_DOWN|MSG_WARNING|MSG_ERRORTYPE,1,UMSG(MError),UMSG(MCannotCreateFolder),strDirName,UMSG(MOk));
          Update(UPDATE_KEEP_SELECTION);
          if (MakeCode==1)
            GoToFile(PointToName(strDirName));
          Redraw();
          Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
          /* $ 07.09.2001 VVM
            ! �������� �������� ������ � ���������� �� ����� ������� */
//          AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
//          AnotherPanel->Redraw();

          if (AnotherPanel->GetType()!=FILE_PANEL)
          {
            AnotherPanel->SetCurDir(strCurDir,FALSE);
            AnotherPanel->Redraw();
          }
        }
        else
          ShellMakeDir(this);
      }
      return(TRUE);
    }

    case KEY_F8:
    case KEY_SHIFTDEL:
    case KEY_SHIFTF8:
    case KEY_SHIFTNUMDEL:
    case KEY_SHIFTDECIMAL:
    case KEY_ALTNUMDEL:
    case KEY_ALTDECIMAL:
    case KEY_ALTDEL:
    {
      _ALGO(CleverSysLog clv(L"F8/Shift-F8/Shift-Del/Alt-Del"));
      _ALGO(SysLog(L"%s, FileCount=%d, Key=%s",(PanelMode==PLUGIN_PANEL?"PluginPanel":"FilePanel"),FileCount,_FARKEY_ToName(Key)));
      if (FileCount>0 && SetCurPath())
      {
        if (Key==KEY_SHIFTF8)
          ReturnCurrentFile=TRUE;
        if (PanelMode==PLUGIN_PANEL &&
            !CtrlObject->Plugins.UseFarCommand(hPlugin,PLUGIN_FARDELETEFILES))
          PluginDelete();
        else
        {
          int SaveOpt=Opt.DeleteToRecycleBin;
          if (Key==KEY_SHIFTDEL || Key==KEY_SHIFTNUMDEL || Key==KEY_SHIFTDECIMAL)
            Opt.DeleteToRecycleBin=0;
          ShellDelete(this,Key==KEY_ALTDEL||Key==KEY_ALTNUMDEL||Key==KEY_ALTDECIMAL);
          Opt.DeleteToRecycleBin=SaveOpt;
        }
        if (Key==KEY_SHIFTF8)
          ReturnCurrentFile=FALSE;
      }
      return(TRUE);
    }

    // $ 26.07.2001 VVM  � ������ ������� ������ �� 1
    case KEY_MSWHEEL_UP:
    case (KEY_MSWHEEL_UP | KEY_ALT):
      Scroll(Key & KEY_ALT?-1:-Opt.MsWheelDelta);
      return(TRUE);

    case KEY_MSWHEEL_DOWN:
    case (KEY_MSWHEEL_DOWN | KEY_ALT):
      Scroll(Key & KEY_ALT?1:Opt.MsWheelDelta);
      return(TRUE);

    case KEY_MSWHEEL_LEFT:
    case (KEY_MSWHEEL_LEFT | KEY_ALT):
    {
      int Roll = Key & KEY_ALT?1:Opt.MsHWheelDelta;
      for (int i=0; i<Roll; i++)
        ProcessKey(KEY_LEFT);
      return TRUE;
    }

    case KEY_MSWHEEL_RIGHT:
    case (KEY_MSWHEEL_RIGHT | KEY_ALT):
    {
      int Roll = Key & KEY_ALT?1:Opt.MsHWheelDelta;
      for (int i=0; i<Roll; i++)
        ProcessKey(KEY_RIGHT);
      return TRUE;
    }

    case KEY_HOME:         case KEY_NUMPAD7:
      Up(0x7fffff);
      return(TRUE);

    case KEY_END:          case KEY_NUMPAD1:
      Down(0x7fffff);
      return(TRUE);

    case KEY_UP:           case KEY_NUMPAD8:
      Up(1);
      return(TRUE);

    case KEY_DOWN:         case KEY_NUMPAD2:
      Down(1);
      return(TRUE);

    case KEY_PGUP:         case KEY_NUMPAD9:
      N=Columns*Height-1;
      CurTopFile-=N;
      Up(N);
      return(TRUE);

    case KEY_PGDN:         case KEY_NUMPAD3:
      N=Columns*Height-1;
      CurTopFile+=N;
      Down(N);
      return(TRUE);

    case KEY_LEFT:         case KEY_NUMPAD4:
      if ( (Columns==1 && Opt.ShellRightLeftArrowsRule == 1) || Columns>1 || CmdLength==0 )
      {
        if (CurTopFile>=Height && CurFile-CurTopFile<Height)
          CurTopFile-=Height;
        Up(Height);
        return(TRUE);
      }
      return(FALSE);

    case KEY_RIGHT:        case KEY_NUMPAD6:
      if ( (Columns==1 && Opt.ShellRightLeftArrowsRule == 1) || Columns>1 || CmdLength==0 )
      {
        if (CurFile+Height<FileCount && CurFile-CurTopFile>=(Columns-1)*(Height))
          CurTopFile+=Height;
        Down(Height);
        return(TRUE);
      }
      return(FALSE);
    /* $ 25.04.2001 DJ
       ����������� Shift-������� ��� Selected files first: ������ ����������
       ���� ���
    */
    case KEY_SHIFTHOME:    case KEY_SHIFTNUMPAD7:
    {
      InternalProcessKey++;
      Lock ();
      while (CurFile>0)
        ProcessKey(KEY_SHIFTUP);
      ProcessKey(KEY_SHIFTUP);
      InternalProcessKey--;
      Unlock ();
      if (SelectedFirst)
        SortFileList(TRUE);
      ShowFileList(TRUE);
      return(TRUE);
    }

    case KEY_SHIFTEND:     case KEY_SHIFTNUMPAD1:
    {
      InternalProcessKey++;
      Lock ();
      while (CurFile<FileCount-1)
        ProcessKey(KEY_SHIFTDOWN);
      ProcessKey(KEY_SHIFTDOWN);
      InternalProcessKey--;
      Unlock ();
      if (SelectedFirst)
        SortFileList(TRUE);
      ShowFileList(TRUE);
      return(TRUE);
    }

    case KEY_SHIFTPGUP:    case KEY_SHIFTNUMPAD9:
    case KEY_SHIFTPGDN:    case KEY_SHIFTNUMPAD3:
    {
      N=Columns*Height-1;
      InternalProcessKey++;
      Lock ();
      while (N--)
        ProcessKey(Key==KEY_SHIFTPGUP||Key==KEY_SHIFTNUMPAD9? KEY_SHIFTUP:KEY_SHIFTDOWN);
      InternalProcessKey--;
      Unlock ();
      if (SelectedFirst)
        SortFileList(TRUE);
      ShowFileList(TRUE);
      return(TRUE);
    }

    case KEY_SHIFTLEFT:    case KEY_SHIFTNUMPAD4:
    case KEY_SHIFTRIGHT:   case KEY_SHIFTNUMPAD6:
    {
      if (FileCount==0)
        return(TRUE);
      if (Columns>1)
      {
        int N=Height;
        InternalProcessKey++;
        Lock ();
        while (N--)
          ProcessKey(Key==KEY_SHIFTLEFT || Key==KEY_SHIFTNUMPAD4? KEY_SHIFTUP:KEY_SHIFTDOWN);
        Select(ListData[CurFile],ShiftSelection);
        if (SelectedFirst)
          SortFileList(TRUE);
        InternalProcessKey--;
        Unlock ();
        if (SelectedFirst)
          SortFileList(TRUE);
        ShowFileList(TRUE);
        return(TRUE);
      }
      return(FALSE);
    }

    case KEY_SHIFTUP:      case KEY_SHIFTNUMPAD8:
    case KEY_SHIFTDOWN:    case KEY_SHIFTNUMPAD2:
    {
      if (FileCount==0)
        return(TRUE);
      CurPtr=ListData[CurFile];
      if (ShiftSelection==-1)
      {
        // .. is never selected
        if (CurFile < FileCount-1 && TestParentFolderName(CurPtr->strName))
          ShiftSelection = !ListData [CurFile+1]->Selected;
        else
          ShiftSelection=!CurPtr->Selected;
      }
      Select(CurPtr,ShiftSelection);
      if (Key==KEY_SHIFTUP || Key == KEY_SHIFTNUMPAD8)
        Up(1);
      else
        Down(1);
      if (SelectedFirst && !InternalProcessKey)
        SortFileList(TRUE);
      ShowFileList(TRUE);
      return(TRUE);
    }

    case KEY_INS:          case KEY_NUMPAD0:
    {
      if (FileCount==0)
        return(TRUE);
      CurPtr=ListData[CurFile];
      Select(CurPtr,!CurPtr->Selected);
      Down(1);
      if (SelectedFirst)
        SortFileList(TRUE);
      ShowFileList(TRUE);
      return(TRUE);
    }

    case KEY_CTRLF3:
      SetSortMode(BY_NAME);
      return(TRUE);

    case KEY_CTRLF4:
      SetSortMode(BY_EXT);
      return(TRUE);

    case KEY_CTRLF5:
      SetSortMode(BY_MTIME);
      return(TRUE);

    case KEY_CTRLF6:
      SetSortMode(BY_SIZE);
      return(TRUE);

    case KEY_CTRLF7:
      SetSortMode(UNSORTED);
      return(TRUE);

    case KEY_CTRLF8:
      SetSortMode(BY_CTIME);
      return(TRUE);

    case KEY_CTRLF9:
      SetSortMode(BY_ATIME);
      return(TRUE);

    case KEY_CTRLF10:
      SetSortMode(BY_DIZ);
      return(TRUE);

    case KEY_CTRLF11:
      SetSortMode(BY_OWNER);
      return(TRUE);

    case KEY_CTRLF12:
      SelectSortMode();
      return(TRUE);

    case KEY_SHIFTF11:
      SortGroups=!SortGroups;
      if (SortGroups)
        ReadSortGroups();
      SortFileList(TRUE);
      Show();
      return(TRUE);

    case KEY_SHIFTF12:
      SelectedFirst=!SelectedFirst;
      SortFileList(TRUE);
      Show();
      return(TRUE);

    case KEY_CTRLPGUP:     case KEY_CTRLNUMPAD9:
      /* $ 25.12.2001 DJ
         � ���� �� ����� ��������������, ���� ChangeDir() ������ ����������
         ������? ���������, �� this, � �������� ������. ������ ��� this
         ��� ���������!
      */
      ChangeDir(L"..");
      /* $ 24.04.2001 IS
           ����������������� ������ ����� ������.
      */
      {
        Panel *NewActivePanel = CtrlObject->Cp()->ActivePanel;
        NewActivePanel->SetViewMode(NewActivePanel->GetViewMode());
        NewActivePanel->Show();
      }
      return(TRUE);

    case KEY_CTRLPGDN:     case KEY_CTRLNUMPAD3:
      ProcessEnter(0,0);
      return(TRUE);

    default:
      if((Key>=KEY_ALT_BASE+0x01 && Key<=KEY_ALT_BASE+65535 ||
          Key>=KEY_ALTSHIFT_BASE+0x01 && Key<=KEY_ALTSHIFT_BASE+65535) &&
         Key != KEY_ALTBS && Key != (KEY_ALTBS|KEY_SHIFT)
        )
      {
        //_SVS(SysLog(L">FastFind: Key=%s",_FARKEY_ToName(Key)));
        // ������������ ��� ����� ������ �������, �.�. WaitInFastFind
        // � ��� ����� ��� ����� ����.
        static const char Code[]=")!@#$%^&*(";
        if(Key >= KEY_ALTSHIFT0 && Key <= KEY_ALTSHIFT9)
          Key=(DWORD)Code[Key-KEY_ALTSHIFT0];
        else if((Key&(~(KEY_ALT+KEY_SHIFT))) == '/')
          Key='?';
        else if(Key == KEY_ALTSHIFT+'-')
          Key='_';
        else if(Key == KEY_ALTSHIFT+'=')
          Key='+';
        //_SVS(SysLog(L"<FastFind: Key=%s",_FARKEY_ToName(Key)));
        FastFind(Key);
      }
      else
        break;
      return(TRUE);
  }
  return(FALSE);
}


void FileList::Select(struct FileListItem *SelPtr,int Selection)
{
  if (!TestParentFolderName(SelPtr->strName) && SelPtr->Selected!=Selection)
    if ((SelPtr->Selected=Selection)!=0)
    {
      SelFileCount++;
      SelFileSize += SelPtr->UnpSize;
    }
    else
    {
       SelFileCount--;
       SelFileSize -= SelPtr->UnpSize;
    }
}


void FileList::ProcessEnter(int EnableExec,int SeparateWindow)
{
  struct FileListItem *CurPtr;
  string strFileName, strShortFileName;
  const wchar_t *ExtPtr;
  if (CurFile>=FileCount)
    return;

  CurPtr=ListData[CurFile];

  strFileName = CurPtr->strName;

  if ( !CurPtr->strShortName.IsEmpty () )
    strShortFileName = CurPtr->strShortName;
  else
    strShortFileName = CurPtr->strName;

  if (CurPtr->FileAttr & FA_DIREC)
  {
    BOOL IsRealName=FALSE;
    if(PanelMode==PLUGIN_PANEL)
    {
      struct OpenPluginInfo Info;
      CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
      IsRealName=Info.Flags&OPIF_REALNAMES;
    }

    // Shift-Enter �� �������� �������� ���������
    if((PanelMode!=PLUGIN_PANEL || IsRealName) && SeparateWindow)
    {
      string strFullPath;
      if(!PathMayBeAbsolute(CurPtr->strName))
      {
        strFullPath = strCurDir;
        AddEndSlash(strFullPath);
        /* 23.08.2001 VVM
          ! SHIFT+ENTER �� ".." ����������� ��� �������� ��������, � �� ������������� */
        if (!TestParentFolderName(CurPtr->strName))
          strFullPath += CurPtr->strName;
      }
      else
      {
        strFullPath = CurPtr->strName;
      }
      QuoteSpace(strFullPath);

      Execute(strFullPath,FALSE,SeparateWindow?2:0,TRUE,CurPtr->FileAttr&FA_DIREC);
    }
    else
    {
      /* $ 09.04.2001 SVS
         �� ��������������, ���� ChangeDir ������� ������
      */
      BOOL res=FALSE;
      int CheckFullScreen=IsFullScreen();
      if (PanelMode==PLUGIN_PANEL || wcschr(CurPtr->strName,L'?')==NULL ||
          CurPtr->strShortName.IsEmpty() )
      {
        res=ChangeDir(CurPtr->strName);
      }
      else
        res=ChangeDir(CurPtr->strShortName);
//      if(res)
      if(CheckFullScreen)
      {
        CtrlObject->Cp()->GetAnotherPanel(this)->Show();
      }
      CtrlObject->Cp()->ActivePanel->Show();
    }
  }
  else
  {
    int PluginMode=PanelMode==PLUGIN_PANEL &&
        !CtrlObject->Plugins.UseFarCommand(hPlugin,PLUGIN_FARGETFILE);
    if (PluginMode)
    {
      string strTempDir;
      if(!FarMkTempEx(strTempDir))
        return;
      CreateDirectoryW(strTempDir,NULL);
      struct PluginPanelItem PanelItem;
      FileListToPluginItem(CurPtr,&PanelItem);

      if (!CtrlObject->Plugins.GetFile(hPlugin,&PanelItem,strTempDir,strFileName,OPM_SILENT|OPM_VIEW))
      {
        apiRemoveDirectory(strTempDir);
        return;
      }

      ConvertNameToShort(strFileName,strShortFileName);
    }

    if (EnableExec && SetCurPath() && !SeparateWindow &&
        ProcessLocalFileTypes(strFileName,strShortFileName,FILETYPE_EXEC,PluginMode)) //?? is was var!
    {
      if (PluginMode)
        DeleteFileWithFolder(strFileName);
      return;
    }

    ExtPtr=wcsrchr(strFileName,L'.');
    int ExeType=FALSE,BatType=FALSE;
    if (ExtPtr!=NULL)
    {
      ExeType=StrCmpI(ExtPtr,L".exe")==0 || StrCmpI(ExtPtr,L".com")==0;
      BatType=IsBatchExtType(ExtPtr);
    }
    if (EnableExec && (ExeType || BatType))
    {
      QuoteSpace(strFileName);
      if (!(Opt.ExcludeCmdHistory&EXCLUDECMDHISTORY_NOTPANEL) && !PluginMode) //AN
        CtrlObject->CmdHistory->AddToHistory(strFileName);

      int DirectRun=(strCurDir.At(0)==L'\\' && strCurDir.At(1)==L'\\' && ExeType);

      CtrlObject->CmdLine->ExecString(strFileName,PluginMode,SeparateWindow,DirectRun);
      if (PluginMode)
        DeleteFileWithFolder(strFileName);
    }
    else
      if (SetCurPath())
      {
        HANDLE hOpen=NULL;
        /* $ 02.08.2001 IS ���������� ���������� ��� ctrl-pgdn */

        if(!EnableExec &&     // �� ��������� � �� � ��������� ����,
           !SeparateWindow && // ������������� ��� Ctrl-PgDn
           ProcessLocalFileTypes(strFileName,strShortFileName,FILETYPE_ALTEXEC,
           PluginMode)
          )
        {
          if (PluginMode)
          {
            DeleteFileWithFolder(strFileName);
          }
          return;
        }
        if (SeparateWindow || (hOpen=OpenFilePlugin(strFileName,TRUE))==INVALID_HANDLE_VALUE ||
            hOpen==(HANDLE)-2)
        {
          if (EnableExec && hOpen!=(HANDLE)-2)
            if (SeparateWindow || Opt.UseRegisteredTypes)
              ProcessGlobalFileTypes(strFileName,PluginMode);
          if (PluginMode)
          {
            DeleteFileWithFolder(strFileName);
          }
        }
        return;
      }
  }
}


void FileList::SetCurDir(const wchar_t *NewDir,int ClosePlugin)
{
  if (ClosePlugin && PanelMode==PLUGIN_PANEL)
  {
    while (1)
    {
      if (ProcessPluginEvent(FE_CLOSE,NULL))
        return;
      if (!PopPlugin(TRUE))
        break;
    }
    CtrlObject->Cp()->RedrawKeyBar();
  }
  /* $ 20.07.2001 VVM
    ! ��������� �� �������� ������ */
  if ((NewDir) && (*NewDir))
  {
    ChangeDir(NewDir);
  }
}

BOOL FileList::ChangeDir(const wchar_t *NewDir,BOOL IsUpdated)
{
  Panel *AnotherPanel;
  string strFindDir, strSetDir;

  strSetDir = NewDir;
    /* $ 28.08.2007 YJH
      + � �������� ������ ����� �� GetFileAttributes("..") ��� ���������� �
        ����� UNC ����. ���������� �������� � ������ */
  if (PanelMode!=PLUGIN_PANEL && !StrCmp(strSetDir, L"..") &&
      !StrCmpN(strCurDir, L"\\\\?\\", 4) && strCurDir.At(4) &&
      !StrCmp(&strCurDir[5], L":\\"))
  {
    strSetDir = (const wchar_t*)strCurDir+4;
  }
  PrepareDiskPath(strSetDir);

  if ( !TestParentFolderName(strSetDir) && StrCmp(strSetDir,L"\\")!=0)
    UpperFolderTopFile=CurTopFile;

  if (SelFileCount>0)
    ClearSelection();

  int PluginClosed=FALSE,GoToPanelFile=FALSE;
  if (PanelMode==PLUGIN_PANEL)
  {
    struct OpenPluginInfo Info;
    CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);

    /* $ 16.01.2002 VVM
      + ���� � ������� ��� OPIF_REALNAMES, �� ������� ����� �� ������� � ������ */

    string strInfoCurDir;
    string strInfoFormat;

    strInfoCurDir = NullToEmpty(Info.CurDir);
    strInfoFormat = NullToEmpty(Info.Format);

    CtrlObject->FolderHistory->AddToHistory(strInfoCurDir,strInfoFormat,1,
                               (Info.Flags & OPIF_REALNAMES)?0:(Opt.SavePluginFoldersHistory?0:1));

    /* $ 25.04.01 DJ
       ��� ������� SetDirectory �� ���������� ���������
    */
    BOOL SetDirectorySuccess = TRUE;
    int UpperFolder=TestParentFolderName(strSetDir);
    if (UpperFolder && *NullToEmpty(Info.CurDir)==0)
    {
      if (ProcessPluginEvent(FE_CLOSE,NULL))
        return(TRUE);
      PluginClosed=TRUE;

      strFindDir = NullToEmpty(Info.HostFile);
      if ( strFindDir.IsEmpty() && (Info.Flags & OPIF_REALNAMES) && CurFile<FileCount)
      {
        strFindDir = ListData[CurFile]->strName;
        GoToPanelFile=TRUE;
      }
      PopPlugin(TRUE);
      Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
      if (AnotherPanel->GetType()==INFO_PANEL)
        AnotherPanel->Redraw();
    }
    else
    {
      strFindDir = NullToEmpty(Info.CurDir);
      SetDirectorySuccess=CtrlObject->Plugins.SetDirectory(hPlugin,strSetDir,0);
    }
    ProcessPluginCommand();
    if (SetDirectorySuccess)
      Update(0);
    else
      Update(UPDATE_KEEP_SELECTION);
    if (PluginClosed && PrevDataStackSize>0)
    {
      PrevDataStackSize--;
      if (PrevDataStack[PrevDataStackSize]->PrevFileCount>0)
      {
        MoveSelection(ListData,FileCount,PrevDataStack[PrevDataStackSize]->PrevListData,PrevDataStack[PrevDataStackSize]->PrevFileCount);
        UpperFolderTopFile = PrevDataStack[PrevDataStackSize]->PrevTopFile;
        if (!GoToPanelFile)
          strFindDir = PrevDataStack[PrevDataStackSize]->strPrevName;

        DeleteListData(PrevDataStack[PrevDataStackSize]->PrevListData,PrevDataStack[PrevDataStackSize]->PrevFileCount);
        if (ListSelectedFirst)
          SortFileList(FALSE);
        else if (FileCount>0)
          SortFileList(TRUE);
      }
    }

    if (UpperFolder)
    {
      long Pos=FindFile(PointToName(strFindDir));
      if (Pos!=-1)
        CurFile=Pos;
      else
        GoToFile(strFindDir);
      CurTopFile=UpperFolderTopFile;
      UpperFolderTopFile=0;
      CorrectPosition();
    }
    /* $ 26.04.2001 DJ
       ������� ��� ������� ��������� ��� ������� SetDirectory
    */
    else if (SetDirectorySuccess)
      CurFile=CurTopFile=0;
    return(TRUE);
  }
  else
  {
    string strFullNewDir;

    ConvertNameToFull(strSetDir, strFullNewDir);

    if ( StrCmpI(strFullNewDir, strCurDir)!=0)
      CtrlObject->FolderHistory->AddToHistory(strCurDir,NULL,0);

    if(TestParentFolderName(strSetDir))
    {
      string strRootDir, strTempDir;

      strTempDir = strCurDir;

      AddEndSlash(strTempDir);
      GetPathRoot(strTempDir, strRootDir);

      if((strCurDir.At(0) == L'\\' && strCurDir.At(1) == L'\\' && StrCmp(strTempDir,strRootDir)==0) ||
         (strCurDir.At(1) == L':'  && strCurDir[2] == L'\\' && strCurDir.At(3)==0))
      {
        string strDirName;
        strDirName = strCurDir;

        AddEndSlash(strDirName);

        if(Opt.PgUpChangeDisk &&
          (FAR_GetDriveType(strDirName) != DRIVE_REMOTE ||
           !CtrlObject->Plugins.FindPlugin(SYSID_NETWORK)))
        {
          CtrlObject->Cp()->ActivePanel->ChangeDisk();
          return TRUE;
        }

        string strNewCurDir;

        strNewCurDir = strCurDir;
        if(strNewCurDir.At(1) == L':')
        {
          wchar_t Letter=strNewCurDir.At(0);
          DriveLocalToRemoteName(DRIVE_REMOTE,Letter,strNewCurDir);
        }

        if( !strNewCurDir.IsEmpty() ) // �������� - ����� �� ������� ���������� RemoteName
        {
          const wchar_t *PtrS1=wcschr((const wchar_t*)strNewCurDir+2,L'\\');
          if(PtrS1 && !wcschr(PtrS1+1,L'\\'))
          {
            if(CtrlObject->Plugins.CallPlugin(SYSID_NETWORK,OPEN_FILEPANEL,(void*)(const wchar_t *)strNewCurDir)) // NetWork Plugin :-)
              return(FALSE);
          }
        }
      }
    }
  }

  strFindDir = PointToName(strCurDir);

  if ( strSetDir.IsEmpty() || strSetDir.At(1) != L':' || strSetDir.At(2) != L'\\')
    FarChDir(strCurDir);

  /* $ 26.04.2001 DJ
     ���������, ������� �� ������� �������, � ��������� � KEEP_SELECTION,
     ���� �� �������
  */
  int UpdateFlags = 0;

  // ...����� ����� � ������ cd //host/share
  if(WinVer.dwPlatformId != VER_PLATFORM_WIN32_NT &&
    strSetDir.At(0) == L'/' && strSetDir.At(1) == L'/')
  {
    wchar_t *Ptr=strSetDir.GetBuffer();
    while(*Ptr)
    {
      if(*Ptr == L'/')
        *Ptr=L'\\';
      ++Ptr;
    }

    strSetDir.ReleaseBuffer ();
  }

  if(PanelMode!=PLUGIN_PANEL && !StrCmp(strSetDir,L"\\"))
  {
#if 1    // ���� ��������� 0, �� ��� ����� ��������� � ������ ���� �����, ������� ��������� �� �������� �������
    GetPathRootOne(strCurDir,strSetDir);
#else
    GetPathRoot(strCurDir,strSetDir);
    if(!StrCmpN(SetDir,L"\\\\?\\Volume{",11)) // ������, ����� ��� ����������� �� NTFS � �������� ��������, �� ����� �� �����.
      GetPathRootOne(CurDir,SetDir);
#endif
  }

  if (!FarChDir(strSetDir))
  {
    /* $ 03.11.2001 IS
         ������ ��� ���������� ��������
    */
    string strTarget;
    strTarget = strSetDir;
    TruncPathStr(strTarget, ScrX-16);
    Message(MSG_WARNING | MSG_ERRORTYPE, 1, UMSG (MError), strTarget, UMSG (MOk));
    UpdateFlags = UPDATE_KEEP_SELECTION;
  }
  /* $ 28.04.2001 IS
       ������������� "�� ������ ������".
       � �� ����, ������ ���� ���������� ������ � ����, �� ���� ����, ������ ��
       ��� ������-���� ������ ���������. �������� ����� ������� RTFM. ���� ���
       ��������: chdir, setdisk, SetCurrentDirectory � ���������� ���������

  */
  /*else {
    if (isalpha(SetDir[0]) && SetDir[1]==':')
    {
      int CurDisk=toupper(SetDir[0])-'A';
      setdisk(CurDisk);
    }
  }*/
  FarGetCurDir(strCurDir);

  if(!IsUpdated)
    return(TRUE);

  Update(UpdateFlags);

  if (TestParentFolderName(strSetDir))
  {
    GoToFile(strFindDir);
    CurTopFile=UpperFolderTopFile;
    UpperFolderTopFile=0;
    CorrectPosition();
  }
  else if (UpdateFlags != UPDATE_KEEP_SELECTION)
    CurFile=CurTopFile=0;

  if (GetFocus())
  {
    CtrlObject->CmdLine->SetCurDir(strCurDir);
    CtrlObject->CmdLine->Show();
  }
  AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
  if (AnotherPanel->GetType()!=FILE_PANEL)
  {
    AnotherPanel->SetCurDir(strCurDir,FALSE);
    AnotherPanel->Redraw();
  }
  if (PanelMode==PLUGIN_PANEL)
    CtrlObject->Cp()->RedrawKeyBar();
  return(TRUE);

}


int FileList::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
  struct FileListItem *CurPtr;
  int RetCode;

  if (IsVisible() && Opt.ShowColumnTitles && MouseEvent->dwEventFlags==0 &&
      MouseEvent->dwMousePosition.Y==Y1+1 &&
      MouseEvent->dwMousePosition.X>X1 && MouseEvent->dwMousePosition.X<X1+3)
  {
    if (MouseEvent->dwButtonState)
      if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
        ChangeDisk();
      else
        SelectSortMode();
    return(TRUE);
  }

  if (IsVisible() && Opt.ShowPanelScrollbar && MouseX==X2 &&
      (MouseEvent->dwButtonState & 1) && !IsDragging())
  {
    int ScrollY=Y1+1+Opt.ShowColumnTitles;
    if (MouseY==ScrollY)
    {
      while (IsMouseButtonPressed())
        ProcessKey(KEY_UP);
      SetFocus();
      return(TRUE);
    }
    if (MouseY==ScrollY+Height-1)
    {
      while (IsMouseButtonPressed())
        ProcessKey(KEY_DOWN);
      SetFocus();
      return(TRUE);
    }
    if (MouseY>ScrollY && MouseY<ScrollY+Height-1 && Height>2)
    {
      CurFile=(FileCount-1)*(MouseY-ScrollY)/(Height-2);
      ShowFileList(TRUE);
      SetFocus();
      return(TRUE);
    }
  }

  /* $ 21.08.2001 VVM
    + ������� ������� ������� ������ �� ����� */
  /* $ 17.12.2001 IS
    ! ��������� �� ������� - �����������
  */
  if ((MouseEvent->dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED)
      && (MouseEvent->dwEventFlags != MOUSE_MOVED)
      && Opt.PanelMiddleClickRule)
  {
    int Key = KEY_ENTER;
    if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
      Key |= KEY_SHIFT;
    if (MouseEvent->dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
      Key |= KEY_CTRL;
    if (MouseEvent->dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
      Key |= KEY_ALT;
    ProcessKey(Key);
    return(TRUE);
  }

  if (Panel::PanelProcessMouse(MouseEvent,RetCode))
    return(RetCode);

  if (MouseEvent->dwMousePosition.Y>Y1+Opt.ShowColumnTitles &&
      MouseEvent->dwMousePosition.Y<Y2-2*Opt.ShowPanelStatus)
  {
    SetFocus();
    if (FileCount==0)
      return(TRUE);
    MoveToMouse(MouseEvent);
    CurPtr=ListData[CurFile];

    if ((MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) &&
        MouseEvent->dwEventFlags==DOUBLE_CLICK)
    {
      if (PanelMode==PLUGIN_PANEL)
      {
        FlushInputBuffer(); // !!!
        int ProcessCode=CtrlObject->Plugins.ProcessKey(hPlugin,VK_RETURN,ShiftPressed ? PKF_SHIFT:0);
        ProcessPluginCommand();
        if (ProcessCode)
          return(TRUE);
      }
      /*$ 21.02.2001 SKV
        ���� ������ DOUBLE_CLICK ��� ���������������� ���
        �������� �����, �� ������ �� ����������������.
        ���������� ���.
        �� ���� ��� ���������� DOUBLE_CLICK, �����
        ������� �����������...
        �� �� �� �������� Fast=TRUE...
        ����� �� ������ ���� ��.
      */
      ShowFileList(TRUE);
      FlushInputBuffer();
      ProcessEnter(1,ShiftPressed!=0);
      return(TRUE);
    }
    else
    {
      /* $ 11.09.2000 SVS
         Bug #17: �������� ��� �������, ��� ������� ��������� �����.
      */
      if ((MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED) && !IsEmpty)
      {
        if (MouseEvent->dwEventFlags==0)
          MouseSelection=!CurPtr->Selected;
        Select(CurPtr,MouseSelection);
        if (SelectedFirst)
          SortFileList(TRUE);
      }
    }
    ShowFileList(TRUE);
    return(TRUE);
  }
  if (MouseEvent->dwMousePosition.Y<=Y1+1)
  {
    SetFocus();
    if (FileCount==0)
      return(TRUE);
    while (IsMouseButtonPressed() && MouseY<=Y1+1)
    {
      Up(1);
      if (RButtonPressed)
      {
        CurPtr=ListData[CurFile];
        Select(CurPtr,MouseSelection);
      }
    }
    if (SelectedFirst)
      SortFileList(TRUE);
    return(TRUE);
  }
  if (MouseEvent->dwMousePosition.Y>=Y2-2)
  {
    SetFocus();
    if (FileCount==0)
      return(TRUE);
    while (IsMouseButtonPressed() && MouseY>=Y2-2)
    {
      Down(1);
      if (RButtonPressed)
      {
        CurPtr=ListData[CurFile];
        Select(CurPtr,MouseSelection);
      }
    }
    if (SelectedFirst)
      SortFileList(TRUE);
    return(TRUE);
  }
  return(FALSE);
}


/* $ 12.09.2000 SVS
  + ������������ ��������� ��� ������ ������� ���� �� ������ ������
*/
void FileList::MoveToMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
  int CurColumn=1,ColumnsWidth,I;
  int PanelX=MouseEvent->dwMousePosition.X-X1-1;

  int Level = 0;

  for (ColumnsWidth=I=0;I<ViewSettings.ColumnCount;I++)
  {
    if ( Level == ColumnsInGlobal )
    {
      CurColumn++;
      Level = 0;
    }
    ColumnsWidth+=ViewSettings.ColumnWidth[I];
    if (ColumnsWidth>=PanelX)
      break;
    ColumnsWidth++;
    Level++;
  }
//  if (CurColumn==0)
//    CurColumn=1;
  int OldCurFile=CurFile;
  CurFile=CurTopFile+MouseEvent->dwMousePosition.Y-Y1-1-Opt.ShowColumnTitles;
  if (CurColumn>1)
    CurFile+=(CurColumn-1)*Height;
  CorrectPosition();
  /* $ 11.09.2000 SVS
     Bug #17: �������� �� ��������� ������ �������.
  */
  if(Opt.PanelRightClickRule == 1)
    IsEmpty=((CurColumn-1)*Height > FileCount);
  else if(Opt.PanelRightClickRule == 2 &&
          (MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED) &&
          ((CurColumn-1)*Height > FileCount))
  {
    CurFile=OldCurFile;
    IsEmpty=TRUE;
  }
  else
    IsEmpty=FALSE;
}

void FileList::SetViewMode(int ViewMode)
{
  int CurFullScreen=IsFullScreen();
  int OldOwner=IsColumnDisplayed(OWNER_COLUMN);
  int OldPacked=IsColumnDisplayed(PACKED_COLUMN);
  int OldNumLink=IsColumnDisplayed(NUMLINK_COLUMN);
  int OldDiz=IsColumnDisplayed(DIZ_COLUMN);
  int OldCaseSensitiveSort=ViewSettings.CaseSensitiveSort;
  int OldNumericSort=NumericSort;
  PrepareViewSettings(ViewMode,NULL);
  int NewOwner=IsColumnDisplayed(OWNER_COLUMN);
  int NewPacked=IsColumnDisplayed(PACKED_COLUMN);
  int NewNumLink=IsColumnDisplayed(NUMLINK_COLUMN);
  int NewDiz=IsColumnDisplayed(DIZ_COLUMN);
  int NewAccessTime=IsColumnDisplayed(ADATE_COLUMN);
  int NewCaseSensitiveSort=ViewSettings.CaseSensitiveSort;
  int NewNumericSort=NumericSort;
  int ResortRequired=FALSE;

  string strDriveRoot;
  DWORD FileSystemFlags;
  GetPathRoot(strCurDir,strDriveRoot);
  if (NewPacked && apiGetVolumeInformation (strDriveRoot,NULL,NULL,NULL,&FileSystemFlags,NULL))
    if ((FileSystemFlags & FS_FILE_COMPRESSION)==0)
      NewPacked=FALSE;

  if (FileCount>0 && PanelMode!=PLUGIN_PANEL &&
      (!OldOwner && NewOwner || !OldPacked && NewPacked ||
       !OldNumLink && NewNumLink ||
       AccessTimeUpdateRequired && NewAccessTime))
    Update(UPDATE_KEEP_SELECTION);
  else
    if (OldCaseSensitiveSort!=NewCaseSensitiveSort || OldNumericSort!=NewNumericSort) //????
      ResortRequired=TRUE;

  if (!OldDiz && NewDiz)
    ReadDiz();

  if (ViewSettings.FullScreen && !CurFullScreen)
  {
    if (Y2>0)
      SetPosition(0,Y1,ScrX,Y2);
    FileList::ViewMode=ViewMode;
  }
  else
    if (!ViewSettings.FullScreen && CurFullScreen)
    {
      if (Y2>0)
        if (this==CtrlObject->Cp()->LeftPanel)
          SetPosition(0,Y1,ScrX/2-Opt.WidthDecrement,Y2);
        else
          SetPosition(ScrX/2+1-Opt.WidthDecrement,Y1,ScrX,Y2);
      FileList::ViewMode=ViewMode;
    }
    else
    {
      FileList::ViewMode=ViewMode;
      FrameManager->RefreshFrame();
    }

  if (PanelMode==PLUGIN_PANEL)
  {
    string strColumnTypes,strColumnWidths;

//    SetScreenPosition();
    ViewSettingsToText(ViewSettings.ColumnType,ViewSettings.ColumnWidth,
        ViewSettings.ColumnCount,strColumnTypes,strColumnWidths);
    ProcessPluginEvent(FE_CHANGEVIEWMODE,(void*)(const wchar_t*)strColumnTypes);
  }

  if (ResortRequired)
  {
    SortFileList(TRUE);
    ShowFileList(TRUE);
    Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
    if (AnotherPanel->GetType()==TREE_PANEL)
      AnotherPanel->Redraw();
  }
}


void FileList::SetSortMode(int SortMode)
{
  if (SortMode==FileList::SortMode && Opt.ReverseSort)
    SortOrder=-SortOrder;
  else
    SortOrder=1;
  FileList::SortMode=SortMode;
  if (FileCount>0)
    SortFileList(TRUE);
  FrameManager->RefreshFrame();
}

int FileList::GoToFile(long idxItem)
{
  if ((DWORD)idxItem < (DWORD)FileCount)
  {
    CurFile=idxItem;
    CorrectPosition();
    return TRUE;
  }
  return FALSE;
}

int FileList::GoToFile(const wchar_t *Name,BOOL OnlyPartName)
{
  return GoToFile(FindFile(Name,OnlyPartName));
}


long FileList::FindFile(const wchar_t *Name,BOOL OnlyPartName)
{
  long I;
  struct FileListItem *CurPtr;

  for (I=0; I < FileCount; I++)
  {
    CurPtr = ListData[I];
    const wchar_t *CurPtrName=CurPtr->strName;

    if(OnlyPartName)
      CurPtrName=PointToName(CurPtr->strName);

    if (StrCmp(Name,CurPtrName)==0)
      return I;
    if (StrCmpI(Name,CurPtrName)==0)
      return I;
  }
  return -1;
}

long FileList::FindFirst(const wchar_t *Name)
{
  return FindNext(0,Name);
}

long FileList::FindNext(int StartPos, const wchar_t *Name)
{
  int I;
  struct FileListItem *CurPtr;

  if((DWORD)StartPos < (DWORD)FileCount)
    for(I=StartPos; I < FileCount; I++)
    {
      CurPtr = ListData[I];
      const wchar_t *CurPtrName=CurPtr->strName;
      if (CmpName(Name,CurPtrName,TRUE))
        if (!TestParentFolderName(CurPtrName))
          return I;
    }
  return -1;
}


int FileList::IsSelected(const wchar_t *Name)
{
  long Pos=FindFile(Name);
  return(Pos!=-1 && (ListData[Pos]->Selected || SelFileCount==0 && Pos==CurFile));
}


// $ 02.08.2000 IG  Wish.Mix #21 - ��� ������� '/' ��� '\' � QuickSerach ��������� �� ����������
int FileList::FindPartName(const wchar_t *Name,int Next,int Direct,int ExcludeSets)
{
  int I;
  struct FileListItem *CurPtr;

  int DirFind = 0;
  int Length = StrLength(Name);

  string strMask;

  strMask = Name;

  if ( Length > 0 && (Name[Length-1] == L'/' || Name[Length-1] == L'\\') )
  {
    DirFind = 1;
    strMask.SetLength(strMask.GetLength ()-1);
  }

  strMask += L"*";

  if(ExcludeSets)
  {
    ReplaceStrings(strMask,L"[",L"<[%>",-1,1);
    ReplaceStrings(strMask,L"]",L"[]]",-1,1);
    ReplaceStrings(strMask,L"<[%>",L"[[]",-1,1);
  }

  for (I=CurFile+(Next?Direct:0); I >= 0 && I < FileCount; I+=Direct)
  {
    CurPtr = ListData[I];
    CmpNameSearchMode=(I==CurFile);
    if (CmpName(strMask,CurPtr->strName,TRUE))
      if (!TestParentFolderName(CurPtr->strName))
        if (!DirFind || (CurPtr->FileAttr & FA_DIREC))
        {
          CmpNameSearchMode=FALSE;
          CurFile=I;
          CurTopFile=CurFile-(Y2-Y1)/2;
          ShowFileList(TRUE);
          return(TRUE);
        }
  }

  CmpNameSearchMode=FALSE;

  for(I=(Direct > 0)?0:FileCount-1; (Direct > 0) ? I < CurFile:I > CurFile; I+=Direct)
  {
    CurPtr = ListData[I];
    if (CmpName(strMask,CurPtr->strName,TRUE))
      if (!TestParentFolderName(CurPtr->strName))
        if (!DirFind || (CurPtr->FileAttr & FA_DIREC))
        {
          CurFile=I;
          CurTopFile=CurFile-(Y2-Y1)/2;
          ShowFileList(TRUE);
          return(TRUE);
        }
  }

  return(FALSE);
}


int FileList::GetSelCount()
{
  if (FileCount==0)
    return(0);
  if (SelFileCount==0 || ReturnCurrentFile)
    return(1);
  return(SelFileCount);
}

int FileList::GetRealSelCount()
{
  if (FileCount==0)
    return(0);
  return(SelFileCount);
}


int FileList::GetSelName(string *strName,int &FileAttr,string *strShortName,FAR_FIND_DATA_EX *fd)
{
  if ( strName==NULL )
  {
    GetSelPosition=0;
    LastSelPosition=-1;
    return(TRUE);
  }


  if (SelFileCount==0 || ReturnCurrentFile)
  {
    if (GetSelPosition==0 && CurFile<FileCount)
    {
      GetSelPosition=1;

      *strName = ListData[CurFile]->strName;
      if ( strShortName!=NULL )
      {
        *strShortName = ListData[CurFile]->strShortName;

        if ( strShortName->IsEmpty() )
          *strShortName = *strName;
      }
      FileAttr=ListData[CurFile]->FileAttr;
      LastSelPosition=CurFile;

      if (fd)
      {
        fd->dwFileAttributes=ListData[CurFile]->FileAttr;
        fd->ftCreationTime=ListData[CurFile]->CreationTime;
        fd->ftLastAccessTime=ListData[CurFile]->AccessTime;
        fd->ftLastWriteTime=ListData[CurFile]->WriteTime;
        fd->nFileSize=ListData[CurFile]->UnpSize;
        fd->nPackSize=ListData[CurFile]->PackSize;

        fd->strFileName = ListData[CurFile]->strName;
        fd->strAlternateFileName = ListData[CurFile]->strShortName;
      }

      return(TRUE);
    }
    else
      return(FALSE);
  }

  while (GetSelPosition<FileCount)
    if (ListData[GetSelPosition++]->Selected)
    {
      *strName = ListData[GetSelPosition-1]->strName;
      if ( strShortName!=NULL )
      {
        *strShortName = ListData[GetSelPosition-1]->strShortName;
        if ( strShortName->IsEmpty() )
          *strShortName = *strName;
      }
      FileAttr=ListData[GetSelPosition-1]->FileAttr;
      LastSelPosition=GetSelPosition-1;

      if (fd)
      {
        fd->dwFileAttributes=ListData[GetSelPosition-1]->FileAttr;
        fd->ftCreationTime=ListData[GetSelPosition-1]->CreationTime;
        fd->ftLastAccessTime=ListData[GetSelPosition-1]->AccessTime;
        fd->ftLastWriteTime=ListData[GetSelPosition-1]->WriteTime;
        fd->nFileSize=ListData[GetSelPosition-1]->UnpSize;
        fd->nPackSize=ListData[GetSelPosition-1]->PackSize;
        fd->strFileName = ListData[GetSelPosition-1]->strName;
        fd->strAlternateFileName = ListData[GetSelPosition-1]->strShortName;
      }

      return(TRUE);
    }
  return(FALSE);
}


void FileList::ClearLastGetSelection()
{
  if (LastSelPosition>=0 && LastSelPosition<FileCount)
    Select(ListData[LastSelPosition],0);
}


void FileList::UngetSelName()
{
  GetSelPosition=LastSelPosition;
}


unsigned __int64 FileList::GetLastSelectedSize ()
{
  if (LastSelPosition>=0 && LastSelPosition<FileCount)
    return ListData[LastSelPosition]->UnpSize;

  return (unsigned __int64)(-1);
}


int FileList::GetLastSelectedItem(struct FileListItem *LastItem)
{
  if (LastSelPosition>=0 && LastSelPosition<FileCount)
  {
    *LastItem=*ListData[LastSelPosition];
    return(TRUE);
  }
  return(FALSE);
}


/*int FileList::GetCurName(char *Name,char *ShortName)
{
  if (FileCount==0)
  {
    *Name=*ShortName=0;
    return(FALSE);
  }
  strcpy(Name,ListData[CurFile]->Name);
  strcpy(ShortName,ListData[CurFile]->ShortName);
  if (*ShortName==0)
    strcpy(ShortName,Name);
  return(TRUE);
}
*/
int FileList::GetCurName(string &strName, string &strShortName)
{
  if (FileCount==0)
  {
    strName = L"";
    strShortName = L"";
    return(FALSE);
  }

  strName = ListData[CurFile]->strName;
  strShortName = ListData[CurFile]->strShortName;

  if ( strShortName.IsEmpty() )
    strShortName = strName;
  return(TRUE);
}


/*int FileList::GetCurBaseName(char *Name,char *ShortName)
{
  *Name=*ShortName=0;
  if (FileCount==0)
    return(FALSE);
  if(PanelMode==PLUGIN_PANEL && PluginsStack) // ��� ��������
  {
    // ����� ����� ������ (��� ���������)
    strcpy(Name,PointToName(NullToEmpty(PluginsStack->HostFile)));
  }
  else if(PanelMode==NORMAL_PANEL)
  {
    strcpy(Name,ListData[CurFile].Name);
    strcpy(ShortName,ListData[CurFile].ShortName);
  }

  if (*ShortName==0)
    strcpy(ShortName,Name);
  return(TRUE);
}*/

int FileList::GetCurBaseName(string &strName, string &strShortName)
{
  if (FileCount==0)
  {
    strName = L"";
    strShortName = L"";
    return(FALSE);
  }

  if(PanelMode==PLUGIN_PANEL && PluginsStack) // ��� ��������
  {
    strName = PointToName(PluginsStack[0]->strHostFile);
  }
  else if(PanelMode==NORMAL_PANEL)
  {
    strName = ListData[CurFile]->strName;
    strShortName = ListData[CurFile]->strShortName;
  }

  if ( strShortName.IsEmpty() )
    strShortName = strName;
  return(TRUE);
}

extern void add_char (string &str, wchar_t c); //BUGBUG

void FileList::SelectFiles(int Mode)
{
  CFileMask FileMask; // ����� ��� ������ � �������
  const wchar_t *HistoryName=L"Masks";
  static struct DialogDataEx SelectDlgData[]=
  {
    DI_DOUBLEBOX,3,1,51,5,0,0,0,0,L"",
    DI_EDIT,5,2,49,2,1,(DWORD_PTR)HistoryName,DIF_HISTORY,0,L"",
    DI_TEXT,0,3,0,3,0,0,DIF_SEPARATOR,0,L"",
    DI_BUTTON,0,4,0,4,0,0,DIF_CENTERGROUP,1,(const wchar_t *)MOk,
    DI_BUTTON,0,4,0,4,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MSelectFilter,
    DI_BUTTON,0,4,0,4,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MCancel,
  };
  MakeDialogItemsEx(SelectDlgData,SelectDlg);

  FileFilter Filter(this,FFT_SELECT);
  bool bUseFilter = false;

  struct FileListItem *CurPtr;
  static string strPrevMask=L"*.*";
  /* $ 20.05.2002 IS
     ��� ��������� �����, ���� �������� � ������ ����� �� ������,
     ����� ������ ���������� ������ � ����� ��� ����������� ����� � ������,
     ����� �������� ����� ������������� ���������� ������ - ��� ���������,
     ��������� CmpName.
  */
  string strMask=L"*.*", strRawMask;
  int Selection=0,I;
  bool WrapBrackets=false; // ������� � ���, ��� ����� ����� ��.������ � ������

  if (CurFile>=FileCount)
    return;

  int RawSelection=FALSE;
  if (PanelMode==PLUGIN_PANEL)
  {
    struct OpenPluginInfo Info;
    CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
    RawSelection=(Info.Flags & OPIF_RAWSELECTION);
  }

  CurPtr=ListData[CurFile];
  string strCurName=(ShowShortNames && !CurPtr->strShortName.IsEmpty() ? CurPtr->strShortName:CurPtr->strName);

  if (Mode==SELECT_ADDEXT || Mode==SELECT_REMOVEEXT)
  {
    const wchar_t *DotPtr=wcsrchr(strCurName,L'.');
    if (DotPtr!=NULL)
    {
      // ����� ��� ������, ��� ���������� ����� ��������� �������-�����������
      strRawMask.Format (L"\"*.%s\"", DotPtr+1);
      WrapBrackets=true;
    }
    else
      strMask = L"*.";
    Mode=(Mode==SELECT_ADDEXT) ? SELECT_ADD:SELECT_REMOVE;
  }
  else
  {
    if (Mode==SELECT_ADDNAME || Mode==SELECT_REMOVENAME)
    {
      // ����� ��� ������, ��� ��� ����� ��������� �������-�����������
      strRawMask.Format (L"\"%s", (const wchar_t*)strCurName);

      wchar_t *DotPtr = strRawMask.GetBuffer ((int)strRawMask.GetLength()+4);

      DotPtr=wcsrchr(DotPtr,L'.');

      if (DotPtr!=NULL)
      {
        wcscpy(DotPtr,L".*\"");
        strRawMask.ReleaseBuffer ();
      }
      else
      {
        strRawMask.ReleaseBuffer ();
        strRawMask += ".*\"";
      }
      WrapBrackets=true;
      Mode=(Mode==SELECT_ADDNAME) ? SELECT_ADD:SELECT_REMOVE;
    }
    else
    {
      if (Mode==SELECT_ADD || Mode==SELECT_REMOVE)
      {
        SelectDlg[1].strData = strPrevMask;
        if (Mode==SELECT_ADD)
          SelectDlg[0].strData = UMSG(MSelectTitle);
        else
          SelectDlg[0].strData = UMSG(MUnselectTitle);
        {
          Dialog Dlg(SelectDlg,sizeof(SelectDlg)/sizeof(SelectDlg[0]));
          Dlg.SetHelp(L"SelectFiles");
          Dlg.SetPosition(-1,-1,55,7);
          for(;;)
          {
            Dlg.ClearDone();
            Dlg.Process();
            if (Dlg.GetExitCode()==4 && Filter.FilterEdit())
            {
              bUseFilter = true;
              break;
            }
            if (Dlg.GetExitCode()!=3)
              return;
            strMask = SelectDlg[1].strData;
            if(FileMask.Set(strMask, 0)) // �������� �������� ������������� ����� �� ������
            {
              strPrevMask = strMask;
              break;
            }
          }
        }
      }
    }
  }
  SaveSelection();

  if(!bUseFilter && WrapBrackets) // ������� ��.������ � ������, ����� ��������
  {                               // ��������������� �����
    const wchar_t *src=(const wchar_t*)strRawMask;
    strMask=(const wchar_t*)L"";
    while ( *src )
    {
      if(*src==L']' || *src==L'[')
      {
        add_char (strMask, L'[');
        add_char (strMask, *src);
        add_char (strMask, L']');
      }
      else
        add_char (strMask, *src);

      src++;
    }
  }

  if(bUseFilter || FileMask.Set(strMask, FMF_SILENT)) // ������������ ����� ������ � ��������
  {                                                // ������ � ����������� �� ������ ����������
    for (I=0; I < FileCount; I++)
    {
      CurPtr=ListData[I];
      int Match=FALSE;
      if (Mode==SELECT_INVERT || Mode==SELECT_INVERTALL)
        Match=TRUE;
      else
      {
        if (bUseFilter)
          Match=Filter.FileInFilter(CurPtr);
        else
          Match=FileMask.Compare((ShowShortNames && !CurPtr->strShortName.IsEmpty() ? CurPtr->strShortName:CurPtr->strName));
      }

      if (Match)
      {
        switch(Mode)
        {
          case SELECT_ADD:
            Selection=1;
            break;
          case SELECT_REMOVE:
            Selection=0;
            break;
          case SELECT_INVERT:
          case SELECT_INVERTALL:
            Selection=!CurPtr->Selected;
            break;
        }
        if (bUseFilter || (CurPtr->FileAttr & FA_DIREC)==0 || Opt.SelectFolders ||
            Selection==0 || RawSelection || Mode==SELECT_INVERTALL)
          Select(CurPtr,Selection);
      }
    }
  }

  if (SelectedFirst)
    SortFileList(TRUE);
  ShowFileList(TRUE);
}

void FileList::UpdateViewPanel()
{
  Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
  if (FileCount>0 && AnotherPanel->IsVisible() &&
      AnotherPanel->GetType()==QVIEW_PANEL && SetCurPath())
  {
    QuickView *ViewPanel=(QuickView *)AnotherPanel;
    struct FileListItem *CurPtr=ListData[CurFile];
    if (PanelMode!=PLUGIN_PANEL ||
        CtrlObject->Plugins.UseFarCommand(hPlugin,PLUGIN_FARGETFILE))
    {
      if (TestParentFolderName(CurPtr->strName))
        ViewPanel->ShowFile(strCurDir,FALSE,NULL);
      else
        ViewPanel->ShowFile(CurPtr->strName,FALSE,NULL);
    }
    else
      if ((CurPtr->FileAttr & FA_DIREC)==0)
      {
        string strTempDir,strFileName;
        strFileName = CurPtr->strName;
        if(!FarMkTempEx(strTempDir))
          return;
        CreateDirectoryW(strTempDir,NULL);
        struct PluginPanelItem PanelItem;
        FileListToPluginItem(CurPtr,&PanelItem);
        if (!CtrlObject->Plugins.GetFile(hPlugin,&PanelItem,strTempDir,strFileName,OPM_SILENT|OPM_VIEW|OPM_QUICKVIEW))
        {
          ViewPanel->ShowFile(NULL,FALSE,NULL);
          apiRemoveDirectory(strTempDir);
          return;
        }
        ViewPanel->ShowFile(strFileName,TRUE,NULL);
      }
      else
        if (!TestParentFolderName(CurPtr->strName))
          ViewPanel->ShowFile(CurPtr->strName,FALSE,hPlugin);
        else
          ViewPanel->ShowFile(NULL,FALSE,NULL);

    SetTitle();
  }
}


void FileList::CompareDir()
{
  FileList *Another=(FileList *)CtrlObject->Cp()->GetAnotherPanel(this);
  int I,J;
  if (Another->GetType()!=FILE_PANEL || !Another->IsVisible())
  {
    Message(MSG_WARNING,1,UMSG(MCompareTitle),UMSG(MCompareFilePanelsRequired1),
            UMSG(MCompareFilePanelsRequired2),UMSG(MOk));
    return;
  }

  ScrBuf.Flush();

  // ��������� ������� ��������� � ����� �������
  ClearSelection();
  Another->ClearSelection();

  struct FileListItem *CurPtr, *AnotherCurPtr;
  string strTempName1, strTempName2;

  const wchar_t *PtrTempName1, *PtrTempName2;
  BOOL OpifRealnames1=FALSE, OpifRealnames2=FALSE;

  // �������� ���, ����� ��������� �� �������� ������
  for (I=0; I < FileCount; I++)
  {
    CurPtr = ListData[I];
    if((CurPtr->FileAttr & FA_DIREC)==0)
      Select(CurPtr,TRUE);
  }

  // �������� ���, ����� ��������� �� ��������� ������
  for (J=0; J < Another->FileCount; J++)
  {
    AnotherCurPtr = Another->ListData[J];
    if((AnotherCurPtr->FileAttr & FA_DIREC)==0)
      Another->Select(AnotherCurPtr,TRUE);
  }

  int CompareFatTime=FALSE;
  if (PanelMode==PLUGIN_PANEL)
  {
    struct OpenPluginInfo Info;
    CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
    if (Info.Flags & OPIF_COMPAREFATTIME)
      CompareFatTime=TRUE;
    OpifRealnames1=Info.Flags & OPIF_REALNAMES;
  }
  if (Another->PanelMode==PLUGIN_PANEL && !CompareFatTime)
  {
    struct OpenPluginInfo Info;
    CtrlObject->Plugins.GetOpenPluginInfo(Another->hPlugin,&Info);
    if (Info.Flags & OPIF_COMPAREFATTIME)
      CompareFatTime=TRUE;
    OpifRealnames2=Info.Flags & OPIF_REALNAMES;
  }

  if (PanelMode==NORMAL_PANEL && Another->PanelMode==NORMAL_PANEL)
  {
    string strFileSystemName1, strFileSystemName2;
    string strRoot1, strRoot2;

    GetPathRoot(strCurDir, strRoot1);
    GetPathRoot(Another->strCurDir, strRoot2);

    if (apiGetVolumeInformation (strRoot1,NULL,NULL,NULL,NULL,&strFileSystemName1) &&
        apiGetVolumeInformation (strRoot2,NULL,NULL,NULL,NULL,&strFileSystemName2))
      if (StrCmpI(strFileSystemName1,strFileSystemName2)!=0)
        CompareFatTime=TRUE;
  }

  // ������ ������ ���� �� ������ ���������
  // ������ ������� �������� ������...
  for (I=0; I < FileCount; I++)
  {
    CurPtr = ListData[I];
    // ...���������� � ��������� ��������� ������...
    for (J=0; J < Another->FileCount; J++)
    {
      AnotherCurPtr = Another->ListData[J];
      int Cmp=0;
#if 0
      PtrTempName1=CurPtr->Name;
      PtrTempName2=AnotherCurPtr->Name;

      int fp1=strpbrk(CurPtr->Name,":\\/")!=NULL;
      int fp2=strpbrk(AnotherCurPtr->Name,":\\/")!=NULL;

      if(fp1 && !fp2 && strcmp(PtrTempName2,".."))
      {
        UnicodeToAnsi (Another->strCurDir, TempName2); //BUGBUG
        AddEndSlash(TempName2);
        strncat(TempName2,AnotherCurPtr->Name,sizeof(TempName2)-1);
        PtrTempName2=TempName2;
      }
      else if(!fp1 && fp2 && strcmp(PtrTempName1,".."))
      {
        strcpy(TempName1,CurDir);
        AddEndSlash(TempName1);
        strncat(TempName1,CurPtr->Name,sizeof(TempName1)-1);
        PtrTempName1=TempName1;
      }

      if(OpifRealnames1 || OpifRealnames2)
      {
        PtrTempName1=PointToName(CurPtr->Name);
        PtrTempName2=PointToName(AnotherCurPtr->Name);
      }
#else
      PtrTempName1=PointToName(CurPtr->strName);
      PtrTempName2=PointToName(AnotherCurPtr->strName);
#endif

      if (StrCmpI(PtrTempName1,PtrTempName2)==0)
      //if (LocalStricmp(CurPtr->Name,AnotherCurPtr->Name)==0)
      {
        if (CompareFatTime)
        {
          WORD DosDate,DosTime,AnotherDosDate,AnotherDosTime;
          FileTimeToDosDateTime(&CurPtr->WriteTime,&DosDate,&DosTime);
          FileTimeToDosDateTime(&AnotherCurPtr->WriteTime,&AnotherDosDate,&AnotherDosTime);
          DWORD FullDosTime,AnotherFullDosTime;
          FullDosTime=((DWORD)DosDate<<16)+DosTime;
          AnotherFullDosTime=((DWORD)AnotherDosDate<<16)+AnotherDosTime;
          int D=FullDosTime-AnotherFullDosTime;
          if (D>=-1 && D<=1)
            Cmp=0;
          else
            Cmp=(FullDosTime<AnotherFullDosTime) ? -1:1;
        }
        else
        {
          __int64 RetCompare=*(__int64*)&CurPtr->WriteTime - *(__int64*)&AnotherCurPtr->WriteTime;
          Cmp=!RetCompare?0:(RetCompare > 0?1:-1);
        }

        if (Cmp==0 && (CurPtr->UnpSize != AnotherCurPtr->UnpSize) )
          continue;

        if (Cmp < 1 && CurPtr->Selected)
          Select(CurPtr,0);

        if (Cmp > -1 && AnotherCurPtr->Selected)
          Another->Select(AnotherCurPtr,0);

        if (Another->PanelMode!=PLUGIN_PANEL)
          break;
      }
    }
  }

  if (SelectedFirst)
    SortFileList(TRUE);

  Redraw();
  Another->Redraw();
  if (SelFileCount==0 && Another->SelFileCount==0)
    Message(0,1,UMSG(MCompareTitle),UMSG(MCompareSameFolders1),UMSG(MCompareSameFolders2),UMSG(MOk));
}

void FileList::CopyNames(int FillPathName,int UNC)
{
  struct OpenPluginInfo Info;
  wchar_t *CopyData=NULL;
  long DataSize=0;
  string strSelName, strSelShortName, strQuotedName;
  int FileAttr;

  if (PanelMode==PLUGIN_PANEL)
  {
    CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
  }

  GetSelName(NULL,FileAttr);
  while (GetSelName(&strSelName,FileAttr,&strSelShortName))
  {
    if (DataSize>0)
    {
      wcscat(CopyData+DataSize,L"\r\n");
      DataSize+=2;
    }
    strQuotedName = (ShowShortNames && !strSelShortName.IsEmpty()) ? strSelShortName:strSelName;
    if(FillPathName)
    {

      if (PanelMode!=PLUGIN_PANEL || (Info.Flags & OPIF_REALNAMES))
      {
        /* $ 14.02.2002 IS
           ".." � ������� �������� ���������� ��� ��� �������� ��������
        */
        if(TestParentFolderName(strQuotedName) && TestParentFolderName(strSelShortName))
        {
					strQuotedName.SetLength(1);
					strSelShortName.SetLength(1);
        }
        if(!CreateFullPathName(strQuotedName,strSelShortName,FileAttr,strQuotedName,UNC))
        {
          if (CopyData)
          {
            xf_free(CopyData);
            CopyData=NULL;
          }
          break;
        }
      }
      else
      {
        string strFullName = NullToEmpty(Info.CurDir);

        if (Opt.PanelCtrlFRule && ViewSettings.FolderUpperCase)
          strFullName.Upper();

        if ( !strFullName.IsEmpty() )
          AddEndSlash(strFullName);

        if(Opt.PanelCtrlFRule)
        {
          // ��� ������ �������� �������� �� ������
          if (ViewSettings.FileLowerCase && !(FileAttr & FA_DIREC))
            strQuotedName.Lower();
          if (ViewSettings.FileUpperToLowerCase)
            if (!(FileAttr & FA_DIREC) && !IsCaseMixed(strQuotedName))
               strQuotedName.Lower();
        }
        strFullName += strQuotedName;
        strQuotedName = strFullName;
        // ������� ������ �������!
        if(PanelMode==PLUGIN_PANEL && Opt.SubstPluginPrefix)
        {
          string strPrefix;
          /* $ 19.11.2001 IS ����������� �� �������� :) */
          if(*AddPluginPrefix((FileList *)CtrlObject->Cp()->ActivePanel,strPrefix))
          {
            strPrefix += strQuotedName;
            strQuotedName = strPrefix;
          }
        }
      }
    }
    if(Opt.QuotedName&QUOTEDNAME_CLIPBOARD)
      QuoteSpace(strQuotedName);
    int Length=(int)strQuotedName.GetLength();
    wchar_t *NewPtr=(wchar_t *)xf_realloc(CopyData, (DataSize+Length+3)*sizeof (wchar_t));
    if (NewPtr==NULL)
    {
    	if (CopyData)
    	{
        xf_free(CopyData);
        CopyData=NULL;
      }
      break;
    }
    CopyData=NewPtr;
    CopyData[DataSize]=0;

    wcscpy (CopyData+DataSize, strQuotedName);
    DataSize+=Length;
  }

  CopyToClipboard(CopyData);
  xf_free(CopyData);
}

string &FileList::CreateFullPathName(const wchar_t *Name, const wchar_t *ShortName,DWORD FileAttr, string &strDest, int UNC,int ShortNameAsIs)
{
  //wchar_t *NamePtr;
  wchar_t Chr=0;

  string strFileName;
  string strTemp;

  strFileName = strDest;

  const wchar_t *ShortNameLastSlash=wcsrchr(ShortName, L'\\'), *NameLastSlash=wcsrchr(Name, L'\\');

  if (NULL==ShortNameLastSlash && NULL==NameLastSlash)
    ConvertNameToFull(strFileName, strFileName);
  else

  if(ShowShortNames)
  {
    strTemp = Name;

    wchar_t *lpwszTemp = strTemp.GetBuffer ();

    if(NameLastSlash)
      lpwszTemp[1+NameLastSlash-Name]=0;

    strTemp.ReleaseBuffer();

    const wchar_t *NamePtr = wcsrchr(strFileName, L'\\');

    if(NamePtr != NULL)
      NamePtr++;
    else
      NamePtr=strFileName;

    strTemp += NameLastSlash?NameLastSlash+1:Name; //??? NamePtr??? BUGBUG
    strFileName = strTemp;
  }
  if (ShowShortNames && ShortNameAsIs)
    ConvertNameToShort(strFileName,strFileName);

  /* $ 29.01.2001 VVM
    + �� CTRL+ALT+F � ��������� ������ ������������ UNC-��� �������� �����. */
  if (UNC)
  {
    // ��������� �� ��� �������� �������
    string strFileSystemName;
    GetPathRoot(strFileName,strTemp);

    if(!apiGetVolumeInformation (strTemp,NULL,NULL,NULL,NULL,&strFileSystemName))
      strFileSystemName=L"";


    UNIVERSAL_NAME_INFOW uni;
    DWORD uniSize = sizeof(uni);
    // ��������� WNetGetUniversalName ��� ���� ������, ������ �� ��� Novell`�
    if (StrCmpI(strFileSystemName,L"NWFS") != 0 &&
        WNetGetUniversalNameW(strFileName, UNIVERSAL_NAME_INFO_LEVEL,&uni, &uniSize) == NOERROR)
    {
        strFileName = uni.lpUniversalName;
    }
    else if(strFileName.At(1) == L':')
    {
      // BugZ#449 - �������� ������ CtrlAltF � ��������� Novell DS
      // �����, ���� �� ���������� �������� UniversalName � ���� ���
      // ��������� ���� - �������� ��� ��� ���� ������ ������


      /*if(*DriveLocalToRemoteName(DRIVE_UNKNOWN,*FileName,Temp) != 0)
      {
        if((NamePtr=wcschr(strFileName, L'/')) == NULL)
          NamePtr=wcschr(strFileName, L'\\');
        if(NamePtr != NULL)
        {
          AddEndSlashW(strTemp);

          NamePtr++;
          strTemp += NamePtr;
        }

        strFileName = strTemp;
      }*/ //BUGBUG
    }

    ConvertNameToReal(strFileName,strFileName);
  } /* if */

  // $ 20.10.2000 SVS ������� ���� Ctrl-F ������������!
  if(Opt.PanelCtrlFRule)
  {
    /* $ 13.10.2000 tran
      �� Ctrl-f ��� ������ �������� �������� �� ������ */
    if (ViewSettings.FolderUpperCase)
    {
      if ( FileAttr & FA_DIREC )
        strFileName.Upper();
      else
      {
          wchar_t *lpwszFileName = strFileName.GetBuffer();
          wchar_t *NamePtr=(wchar_t *)wcsrchr(lpwszFileName,L'\\');

          if(NamePtr != NULL)
          {
            Chr=*NamePtr;
            *NamePtr=0;
          }

          CharUpperW (lpwszFileName);

          if(NamePtr)
            *NamePtr=Chr;

          strFileName.ReleaseBuffer();
      }
    }
    if (ViewSettings.FileUpperToLowerCase)
      if (!(FileAttr & FA_DIREC) && wcsrchr(strFileName,L'\\') && !IsCaseMixed(wcsrchr(strFileName,L'\\')))
      {
          wchar_t *lpwszFileName = strFileName.GetBuffer();

          lpwszFileName = wcsrchr (lpwszFileName, L'\\');
          CharLowerW (lpwszFileName);

          strFileName.ReleaseBuffer();
      }
    if ( ViewSettings.FileLowerCase && wcsrchr(strFileName,L'\\') && !(FileAttr & FA_DIREC))
    {
        wchar_t *lpwszFileName = strFileName.GetBuffer();

        lpwszFileName = wcsrchr (lpwszFileName, L'\\');
        CharLowerW (lpwszFileName);

        strFileName.ReleaseBuffer();
    }
  }

  strDest = strFileName;

  return strDest;
}


void FileList::SetTitle()
{
  if (GetFocus() || CtrlObject->Cp()->GetAnotherPanel(this)->GetType()!=FILE_PANEL)
  {
    string strTitleDir = L"{";
    if (PanelMode==PLUGIN_PANEL)
    {
      struct OpenPluginInfo Info;

      CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);

      string strPluginTitle = NullToEmpty(Info.PanelTitle);

      RemoveExternalSpaces(strPluginTitle);

      strTitleDir += strPluginTitle;
    }
    else
      strTitleDir += strCurDir;

    strTitleDir += L"}";

    strLastFarTitle = strTitleDir; //BUGBUG;
    SetFarTitle(strTitleDir);
  }
}


void FileList::ClearSelection()
{
  struct FileListItem *CurPtr;
  for (int I=0; I < FileCount; I++)
  {
    CurPtr = ListData[I];
    Select(CurPtr,0);
  }

  if (SelectedFirst)
    SortFileList(TRUE);
}


void FileList::SaveSelection()
{
  struct FileListItem *CurPtr;
  for (int I=0; I < FileCount; I++)
  {
    CurPtr = ListData[I];
    CurPtr->PrevSelected=CurPtr->Selected;
  }
}


void FileList::RestoreSelection()
{
  struct FileListItem *CurPtr;
  for (int I=0; I < FileCount; I++)
  {
    CurPtr = ListData[I];
    int NewSelection=CurPtr->PrevSelected;
    CurPtr->PrevSelected=CurPtr->Selected;
    Select(CurPtr,NewSelection);
  }
  if (SelectedFirst)
    SortFileList(TRUE);
  Redraw();
}



int FileList::GetFileName(string &strName,int Pos,int &FileAttr)
{
  if (Pos>=FileCount)
    return(FALSE);

  strName = ListData[Pos]->strName;

  FileAttr=ListData[Pos]->FileAttr;
  return(TRUE);
}


int FileList::GetCurrentPos()
{
  return(CurFile);
}


void FileList::EditFilter()
{
  if (Filter==NULL)
    Filter=new FileFilter(this,FFT_PANEL);
  Filter->FilterEdit();
}


void FileList::SelectSortMode()
{
  struct MenuDataEx SortMenu[]=
  {
   /* 00 */(const wchar_t *)MMenuSortByName,LIF_SELECTED,KEY_CTRLF3,
   /* 01 */(const wchar_t *)MMenuSortByExt,0,KEY_CTRLF4,
   /* 02 */(const wchar_t *)MMenuSortByModification,0,KEY_CTRLF5,
   /* 03 */(const wchar_t *)MMenuSortBySize,0,KEY_CTRLF6,
   /* 04 */(const wchar_t *)MMenuUnsorted,0,KEY_CTRLF7,
   /* 05 */(const wchar_t *)MMenuSortByCreation,0,KEY_CTRLF8,
   /* 06 */(const wchar_t *)MMenuSortByAccess,0,KEY_CTRLF9,
   /* 07 */(const wchar_t *)MMenuSortByDiz,0,KEY_CTRLF10,
   /* 08 */(const wchar_t *)MMenuSortByOwner,0,KEY_CTRLF11,
   /* 09 */(const wchar_t *)MMenuSortByCompressedSize,0,0,
   /* 10 */(const wchar_t *)MMenuSortByNumLinks,0,0,
   /* 11 */L"",LIF_SEPARATOR,0,
   /* 12 */(const wchar_t *)MMenuSortUseNumeric,0,0,
   /* 13 */(const wchar_t *)MMenuSortUseGroups,0,KEY_SHIFTF11,
   /* 14 */(const wchar_t *)MMenuSortSelectedFirst,0,KEY_SHIFTF12,
  };

  static int SortModes[]={BY_NAME,   BY_EXT,    BY_MTIME,
                          BY_SIZE,   UNSORTED,  BY_CTIME,
                          BY_ATIME,  BY_DIZ,    BY_OWNER,
                          BY_COMPRESSEDSIZE,BY_NUMLINKS};

  for (int I=0;I<sizeof(SortModes)/sizeof(SortModes[0]);I++)
    if (SortMode==SortModes[I])
    {
      SortMenu[I].SetCheck(SortOrder==1 ? L'+':L'-');
      break;
    }

  int SG=GetSortGroups();
  SortMenu[12].SetCheck(NumericSort);
  SortMenu[13].SetCheck(SG);
  SortMenu[14].SetCheck(SelectedFirst);

  int SortCode;
  {
    VMenu SortModeMenu(UMSG(MMenuSortTitle),SortMenu,sizeof(SortMenu)/sizeof(SortMenu[0]),0);

    SortModeMenu.SetHelp(L"PanelCmdSort");
    SortModeMenu.SetPosition(X1+4,-1,0,0);
    SortModeMenu.SetFlags(VMENU_WRAPMODE);
    SortModeMenu.Process();
    if ((SortCode=SortModeMenu.Modal::GetExitCode())<0)
      return;
  }
  if (SortCode<sizeof(SortModes)/sizeof(SortModes[0]))
    SetSortMode(SortModes[SortCode]);
  else
    switch(SortCode)
    {
      case 13:
        ProcessKey(KEY_SHIFTF11);
        break;
      case 14:
        ProcessKey(KEY_SHIFTF12);
        break;
      case 12:
        NumericSort=NumericSort?0:1;
        Update(UPDATE_KEEP_SELECTION);
        Redraw();
        Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
        AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
        AnotherPanel->Redraw();
        break;
    }
}


void FileList::DeleteDiz(const wchar_t *Name, const wchar_t *ShortName)
{
  if (PanelMode==NORMAL_PANEL)
    Diz.DeleteDiz(Name,ShortName);
}


void FileList::FlushDiz()
{
  if (PanelMode==NORMAL_PANEL)
    Diz.Flush(strCurDir);
}


void FileList::GetDizName(string &strDizName)
{
  if (PanelMode==NORMAL_PANEL)
    Diz.GetDizName(strDizName);
}


void FileList::CopyDiz(const wchar_t *Name, const wchar_t *ShortName,const wchar_t *DestName,
                       const wchar_t *DestShortName,DizList *DestDiz)
{
  Diz.CopyDiz(Name,ShortName,DestName,DestShortName,DestDiz);
}


void FileList::DescribeFiles()
{
  string strSelName, strSelShortName;
  int FileAttr,DizCount=0;

  ReadDiz();

  SaveSelection();
  GetSelName(NULL,FileAttr);
  Panel* AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
  int AnotherType=AnotherPanel->GetType();

  while (GetSelName(&strSelName,FileAttr,&strSelShortName))
  {
    string strDizText, strMsg, strTruncMsg, strQuotedName;
    const wchar_t *PrevText;
    PrevText=Diz.GetDizTextAddr(strSelName,strSelShortName,GetLastSelectedSize());
    strQuotedName = strSelName;
    QuoteSpaceOnly(strQuotedName);
    strMsg.Format (UMSG(MEnterDescription),(const wchar_t*)strQuotedName);
    strTruncMsg.Format (L"%.65s",(const wchar_t*)strMsg);
    /* $ 09.08.2000 SVS
       ��� Ctrl-Z ������� ����� ���������� ��������!
    */
    if (!GetString(UMSG(MDescribeFiles),strTruncMsg,L"DizText",
                   PrevText!=NULL ? PrevText:L"",strDizText,1024,
                   L"FileDiz",FIB_ENABLEEMPTY|(!DizCount?FIB_NOUSELASTHISTORY:0)|FIB_BUTTONS))
      break;

    DizCount++;
    if ( strDizText.IsEmpty() )
      Diz.DeleteDiz(strSelName,strSelShortName);
    else
    {
      string strDizLine;
      strDizLine.Format (L"%-*s %s",Opt.Diz.StartPos>1 ? Opt.Diz.StartPos-2:0, (const wchar_t*)strQuotedName, (const wchar_t*)strDizText);
      Diz.AddDiz(strSelName,strSelShortName,strDizLine);
    }
    ClearLastGetSelection();
    // BugZ#442 - Deselection is late when making file descriptions
    FlushDiz();
    // BugZ#863 - ��� �������������� ������ ������������ ��� �� ����������� �� ����
    //if (AnotherType==QVIEW_PANEL) continue; //TODO ???
    if (AnotherType==INFO_PANEL) AnotherPanel->Update(UIC_UPDATE_NORMAL);
    Update(UPDATE_KEEP_SELECTION);
    Redraw();
  }
  /*if (DizCount>0)
  {
    FlushDiz();
    Update(UPDATE_KEEP_SELECTION);
    Redraw();
    Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
    AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
    AnotherPanel->Redraw();
  }*/
}


void FileList::SetReturnCurrentFile(int Mode)
{
  ReturnCurrentFile=Mode;
}


bool FileList::ApplyCommand()
{
  static string strPrevCommand;
  string strCommand;

  if (!GetString(UMSG(MAskApplyCommandTitle),UMSG(MAskApplyCommand),L"ApplyCmd",strPrevCommand,strCommand,260, L"ApplyCmd",FIB_BUTTONS) || !SetCurPath())
    return false;

  strPrevCommand = strCommand;
  string strSelName, strSelShortName;
  int FileAttr;
  int RdrwDskt=CtrlObject->MainKeyBar->IsVisible();

  RedrawDesktop Redraw(TRUE);
  SaveSelection();

  GetSelName(NULL,FileAttr);
  while (GetSelName(&strSelName,FileAttr,&strSelShortName) && !CheckForEsc())
  {
    string strConvertedCommand;
    string strListName, strAnotherListName;
    string strShortListName, strAnotherShortListName;

    strConvertedCommand = strCommand;

    {
      int PreserveLFN=SubstFileName(strConvertedCommand,strSelName,strSelShortName,&strListName,&strAnotherListName,&strShortListName, &strAnotherShortListName);
      PreserveLongName PreserveName(strSelShortName,PreserveLFN);

      Execute(strConvertedCommand,FALSE,FALSE);

      ClearLastGetSelection();
    }
  }
  /*$ 23.07.2001 SKV
    ��� �� �� �������� ��������� ������ ������.
  */
  if(RdrwDskt)
  {
    ScrBuf.Scroll(1);
    ScrBuf.Flush();
  }
  return true;
}


void FileList::CountDirSize(DWORD PluginFlags)
{
  unsigned long DirCount,DirFileCount,ClusterSize;;
  unsigned __int64 FileSize,CompressedFileSize,RealFileSize;
  unsigned long SelDirCount=0;
  struct FileListItem *CurPtr;
  int I;

  /* $ 09.11.2000 OT
    F3 �� ".." � ��������
  */
  if ( PanelMode==PLUGIN_PANEL && !CurFile && TestParentFolderName(ListData[0]->strName))
  {
    struct FileListItem *DoubleDotDir = NULL;
    if (SelFileCount)
    {
      DoubleDotDir = ListData[0];
      for (I=0; I < FileCount; I++)
      {
        CurPtr = ListData[I];
        if (CurPtr->Selected && (CurPtr->FileAttr & FA_DIREC))
        {
          DoubleDotDir = NULL;
          break;
        }
      }
    }
    else
    {
      DoubleDotDir = ListData[0];
    }

    if (DoubleDotDir)
    {
      DoubleDotDir->ShowFolderSize=1;
      DoubleDotDir->UnpSize     = 0;
      DoubleDotDir->PackSize    = 0;
      for (I=1; I < FileCount; I++)
      {
        CurPtr = ListData[I];
        if (CurPtr->FileAttr & FA_DIREC)
        {
          if (GetPluginDirInfo(hPlugin,CurPtr->strName,DirCount,DirFileCount,FileSize,CompressedFileSize))
          {
            DoubleDotDir->UnpSize += FileSize;
            DoubleDotDir->PackSize += CompressedFileSize;
          }
        }
        else
        {
          DoubleDotDir->UnpSize     += CurPtr->UnpSize;
          DoubleDotDir->PackSize    += CurPtr->PackSize;
        }
      }
    }
  }

  for (I=0; I < FileCount; I++)
  {
    CurPtr = ListData[I];
    if (CurPtr->Selected && (CurPtr->FileAttr & FA_DIREC))
    {
      SelDirCount++;
      if (PanelMode==PLUGIN_PANEL && !(PluginFlags & OPIF_REALNAMES) &&
          GetPluginDirInfo(hPlugin,CurPtr->strName,DirCount,DirFileCount,FileSize,CompressedFileSize)
        ||
          (PanelMode!=PLUGIN_PANEL || (PluginFlags & OPIF_REALNAMES)) &&
          GetDirInfo(UMSG(MDirInfoViewTitle),
                     CurPtr->strName,
                     DirCount,DirFileCount,FileSize,
                     CompressedFileSize,RealFileSize, ClusterSize,0,Filter,GETDIRINFO_DONTREDRAWFRAME|GETDIRINFO_SCANSYMLINKDEF)==1)
      {
        SelFileSize -= CurPtr->UnpSize;
        SelFileSize += FileSize;
        CurPtr->UnpSize = FileSize;
        CurPtr->PackSize = CompressedFileSize;
        CurPtr->ShowFolderSize=1;
      }
      else
        break;
    }
  }

  CurPtr=ListData[CurFile];

  if (SelDirCount==0)
  {
    if (PanelMode==PLUGIN_PANEL && !(PluginFlags & OPIF_REALNAMES) &&
        GetPluginDirInfo(hPlugin,CurPtr->strName,DirCount,DirFileCount,FileSize,CompressedFileSize)
      ||
        (PanelMode!=PLUGIN_PANEL || (PluginFlags & OPIF_REALNAMES)) &&
        GetDirInfo(UMSG(MDirInfoViewTitle),
                   TestParentFolderName(CurPtr->strName) ? L".":CurPtr->strName,
                   DirCount,
                   DirFileCount,FileSize,CompressedFileSize,RealFileSize,ClusterSize,0,Filter,GETDIRINFO_DONTREDRAWFRAME|GETDIRINFO_SCANSYMLINKDEF)==1)
    {
      CurPtr->UnpSize = FileSize;
      CurPtr->PackSize = CompressedFileSize;
      CurPtr->ShowFolderSize=1;
    }
  }

  SortFileList(TRUE);
  ShowFileList(TRUE);
  CtrlObject->Cp()->Redraw();
  CreateChangeNotification(TRUE);
}


int FileList::GetPrevViewMode()
{
  if (PanelMode==PLUGIN_PANEL && PluginsStackSize>0)
    return(PluginsStack[0]->PrevViewMode);
  else
    return(ViewMode);
}


int FileList::GetPrevSortMode()
{
  if (PanelMode==PLUGIN_PANEL && PluginsStackSize>0)
    return(PluginsStack[0]->PrevSortMode);
  else
    return(SortMode);
}


int FileList::GetPrevSortOrder()
{
  if (PanelMode==PLUGIN_PANEL && PluginsStackSize>0)
    return(PluginsStack[0]->PrevSortOrder);
  else
    return(SortOrder);
}

int FileList::GetPrevNumericSort()
{
  if (PanelMode==PLUGIN_PANEL && PluginsStackSize>0)
    return(PluginsStack[0]->PrevNumericSort);
  else
    return(NumericSort);
}


HANDLE FileList::OpenFilePlugin(const wchar_t *FileName,int PushPrev)
{
  if (!PushPrev && PanelMode==PLUGIN_PANEL)
    while (1)
    {
      if (ProcessPluginEvent(FE_CLOSE,NULL))
        return((HANDLE)-2);
      if (!PopPlugin(TRUE))
        break;
    }
  HANDLE hNewPlugin=OpenPluginForFile(FileName);
  if (hNewPlugin!=INVALID_HANDLE_VALUE && hNewPlugin!=(HANDLE)-2)
  {
    if (PushPrev)
    {

      PrevDataStack=(PrevDataItem **)xf_realloc(PrevDataStack,(PrevDataStackSize+1)*sizeof(PrevDataItem*));

      PrevDataStack[PrevDataStackSize] = new PrevDataItem;

      PrevDataStack[PrevDataStackSize]->PrevListData=ListData;
      PrevDataStack[PrevDataStackSize]->PrevFileCount=FileCount;
      PrevDataStack[PrevDataStackSize]->PrevTopFile = CurTopFile;
      PrevDataStack[PrevDataStackSize]->strPrevName = FileName;
      PrevDataStackSize++;
      ListData=NULL;
      FileCount=0;
    }

    BOOL WasFullscreen = IsFullScreen();

    SetPluginMode(hNewPlugin,FileName);  // SendOnFocus??? true???
    PanelMode=PLUGIN_PANEL;
    UpperFolderTopFile=CurTopFile;
    CurFile=0;
    Update(0);
    Redraw();
    Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
    if ( (AnotherPanel->GetType()==INFO_PANEL) || WasFullscreen )
      AnotherPanel->Redraw();
  }
  return(hNewPlugin);
}


void FileList::ProcessCopyKeys(int Key)
{
  if (FileCount>0)
  {
    int Drag=Key==KEY_DRAGCOPY || Key==KEY_DRAGMOVE;
    int Ask=!Drag || Opt.Confirm.Drag;
    int Move=(Key==KEY_F6 || Key==KEY_DRAGMOVE);
    int AnotherDir=FALSE;
    Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
    if (AnotherPanel->GetType()==FILE_PANEL)
    {
      FileList *AnotherFilePanel=(FileList *)AnotherPanel;
      if (AnotherFilePanel->FileCount>0 &&
          (AnotherFilePanel->ListData[AnotherFilePanel->CurFile]->FileAttr & FA_DIREC) &&
          !TestParentFolderName(AnotherFilePanel->ListData[AnotherFilePanel->CurFile]->strName))
      {
        AnotherDir=TRUE;
        if (Drag)
        {
          AnotherPanel->ProcessKey(KEY_ENTER);
          SetCurPath();
        }
      }
    }
    if (PanelMode==PLUGIN_PANEL && !CtrlObject->Plugins.UseFarCommand(hPlugin,PLUGIN_FARGETFILES))
    {
      if (Key!=KEY_ALTF6)
      {
        string strPluginDestPath;
        int ToPlugin=FALSE;
        if (AnotherPanel->GetMode()==PLUGIN_PANEL && AnotherPanel->IsVisible() &&
            !CtrlObject->Plugins.UseFarCommand(AnotherPanel->GetPluginHandle(),PLUGIN_FARPUTFILES))
        {
          ToPlugin=2;
          ShellCopy ShCopy(this,Move,FALSE,FALSE,Ask,ToPlugin,strPluginDestPath);
        }
        if (ToPlugin!=-1)
          if (ToPlugin)
            PluginToPluginFiles(Move);
          else
          {
            string strDestPath;
            if ( !strPluginDestPath.IsEmpty() )
              strDestPath = strPluginDestPath;
            else
            {
              AnotherPanel->GetCurDir(strDestPath);
              if(!AnotherPanel->IsVisible())
              {
                struct OpenPluginInfo Info;
                CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
                if (Info.HostFile!=NULL && *Info.HostFile!=0)
                {
                  wchar_t *ExtPtr;
                  strDestPath = PointToName(Info.HostFile);

                  ExtPtr = strDestPath.GetBuffer();

                  if ( (ExtPtr=wcsrchr(ExtPtr, L'.')) != NULL )
                      *ExtPtr = 0;

                  strDestPath.ReleaseBuffer();
                }
              }
            }

            PluginGetFiles(strDestPath,Move);
          }
      }
    }
    else
    {
      int ToPlugin=AnotherPanel->GetMode()==PLUGIN_PANEL &&
                    AnotherPanel->IsVisible() && Key!=KEY_ALTF6 &&
                    !CtrlObject->Plugins.UseFarCommand(AnotherPanel->GetPluginHandle(),PLUGIN_FARPUTFILES);

      if(Key != KEY_ALTF6 ||
        (Key == KEY_ALTF6 && WinVer.dwPlatformId==VER_PLATFORM_WIN32_NT))
      {
         ShellCopy ShCopy(this,Move,Key==KEY_ALTF6,FALSE,Ask,ToPlugin,NULL);
      }

      if (ToPlugin==1)
        PluginPutFilesToAnother(Move,AnotherPanel);
    }
    if (AnotherDir && Drag)
      AnotherPanel->ProcessKey(KEY_ENTER);
  }
}

void FileList::SetSelectedFirstMode(int Mode)
{
  SelectedFirst=Mode;
  SortFileList(TRUE);
}

void FileList::ChangeSortOrder(int NewOrder)
{
  Panel::ChangeSortOrder(NewOrder);
  SortFileList(TRUE);
  Show();
}

BOOL FileList::UpdateKeyBar()
{
  KeyBar *KB=CtrlObject->MainKeyBar;

  KB->SetAllGroup (KBL_MAIN, MF1, 12);
  KB->SetAllGroup (KBL_SHIFT, MShiftF1, 12);
  KB->SetAllGroup (KBL_ALT, MAltF1, 12);
  KB->SetAllGroup (KBL_CTRL, MCtrlF1, 12);
  KB->SetAllGroup (KBL_CTRLSHIFT, MCtrlShiftF1, 12);
  KB->SetAllGroup (KBL_CTRLALT, MCtrlAltF1, 12);
  KB->SetAllGroup (KBL_ALTSHIFT, MAltShiftF1, 12);

  if(WinVer.dwPlatformId != VER_PLATFORM_WIN32_NT)
    KB->Change(KBL_ALT,L"",6-1);

  KB->ReadRegGroup(L"Shell",Opt.strLanguage);
  KB->SetAllRegGroup();

  if (GetMode() == PLUGIN_PANEL)
  {
    struct OpenPluginInfo Info;
    GetOpenPluginInfo(&Info);
    if (Info.KeyBar)
    {
      KB->Set((const wchar_t **)Info.KeyBar->Titles,12);
      KB->SetShift((const wchar_t **)Info.KeyBar->ShiftTitles,12);
      KB->SetAlt((const wchar_t **)Info.KeyBar->AltTitles,12);
      KB->SetCtrl((const wchar_t **)Info.KeyBar->CtrlTitles,12);
      if(Info.StructSize >= sizeof(struct OpenPluginInfo))
      {
        KB->SetCtrlShift((const wchar_t **)Info.KeyBar->CtrlShiftTitles,12);
        KB->SetAltShift((const wchar_t **)Info.KeyBar->AltShiftTitles,12);
        KB->SetCtrlAlt((const wchar_t **)Info.KeyBar->CtrlAltTitles,12);
      }
    }
  }

  return TRUE;
}

int FileList::PluginPanelHelp(HANDLE hPlugin)
{
  string strPath, strFileName, strStartTopic;

  PluginHandle *ph = (PluginHandle*)hPlugin;

  strPath = ph->pPlugin->GetModuleName();

  CutToSlash(strPath);

  int nCodePage = CP_OEMCP;

  FILE *HelpFile=Language::OpenLangFile(strPath,HelpFileMask,Opt.strHelpLanguage,strFileName, nCodePage);
  if (HelpFile==NULL)
    return(FALSE);
  fclose(HelpFile);
  strStartTopic.Format(HelpFormatLink,(const wchar_t*)strPath,L"Contents");
  Help PanelHelp(strStartTopic);
  return(TRUE);
}

/* $ 19.11.2001 IS
     ��� �������� ������� � ��������� ������� �������� �������� �� ���������
*/
string &FileList::AddPluginPrefix(FileList *SrcPanel,string &strPrefix)
{
	strPrefix = L"";
	if(Opt.SubstPluginPrefix && SrcPanel->GetMode()==PLUGIN_PANEL)
	{
		OpenPluginInfo Info;
		PluginHandle *ph = (PluginHandle*)SrcPanel->hPlugin;

		CtrlObject->Plugins.GetOpenPluginInfo(ph,&Info);

		if(!(Info.Flags & OPIF_REALNAMES))
		{
			PluginInfo PInfo;

			CtrlObject->Plugins.GetPluginInfo(ph->pPlugin, &PInfo);

			if(PInfo.CommandPrefix && *PInfo.CommandPrefix)
			{
				strPrefix = PInfo.CommandPrefix;

				wchar_t *Ptr=strPrefix.GetBuffer ();

				Ptr = wcschr(Ptr, L':');

				if(Ptr)
				{
					*++Ptr=0;
					strPrefix.ReleaseBuffer();
				}
				else
				{
					strPrefix.ReleaseBuffer();
					strPrefix += L":";
				}
			}
		}
	}
	return strPrefix;
}


void FileList::IfGoHome(wchar_t Drive)
{
	string strTmpCurDir;
	string strFName;

	if (apiGetModuleFileName (NULL, strFName))
	{
		strFName.SetLength(3);

		// ������� ��������� ������!!!
		/*
			������? - ������ - ���� �������� ������� (��� ���������
			�������) - �������� ���� � �����������!
		*/
		Panel *Another=CtrlObject->Cp()->GetAnotherPanel (this);
		if (Another->GetMode() != PLUGIN_PANEL)
		{
			Another->GetCurDir(strTmpCurDir);
			if (strTmpCurDir.At(0) == Drive && strTmpCurDir.At(1) == L':')
				Another->SetCurDir(strFName, FALSE);
		}

		if (GetMode() != PLUGIN_PANEL)
		{
			GetCurDir(strTmpCurDir);
			if (strTmpCurDir.At(0) == Drive && strTmpCurDir.At(1) == L':')
				SetCurDir(strFName, FALSE); // ��������� � ������ ����� � far.exe
		}
	}
}


BOOL FileList::GetItem(int Index,void *Dest)
{
  if(Index == -1 || Index == -2)
    Index=GetCurrentPos();
  if((DWORD)Index >= (DWORD)FileCount)
    return FALSE;
  *((FileListItem *)Dest)=*ListData[Index];
  return TRUE;
}
