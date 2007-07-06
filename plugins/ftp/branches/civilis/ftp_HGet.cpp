#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

int FTP::GetHostFiles( struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move, std::wstring& DestPath,int OpMode)
{
	PROC(( "FTP::GetHostFiles","%d [%s] %s %08X",ItemsNumber,DestPath.c_str(),Move?"MOVE":"COPY",OpMode ));

	static FP_DECL_DIALOG( InitItems )
		/*00*/		FDI_CONTROL	( DI_DOUBLEBOX, 3, 1,72,6, 0, NULL )
		/*01*/		FDI_LABEL	( 5, 2,   NULL )
		/*02*/		FDI_EDIT	(  5, 3,70 )
		/*03*/		FDI_HLINE	( 3, 4 )
		/*06*/		FDI_GDEFBUTTON( 0, 5,   FMSG(MCopy) )
		/*07*/		FDI_GBUTTON	( 0, 5,   FMSG(MCancel) )
		FP_END_DIALOG

	FarDialogItem    DialogItems[FP_DIALOG_SIZE(InitItems)];
	FP_SizeItemList  il;

	if ( !IS_SILENT(OpMode) )
	{
		if (Move)
		{
			InitItems[0].Text = FMSG(MMoveHostTitle);
			InitItems[1].Text = FMSG(MMoveHostTo);
		} else
		{
			InitItems[0].Text = FMSG(MCopyHostTitle);
			InitItems[1].Text = FMSG(MCopyHostTo);
		}
		FP_InitDialogItems( InitItems,DialogItems );
		Utils::safe_strcpy(DialogItems[2].Data, Unicode::toOem(DestPath).c_str());

		int AskCode = FDialog( 76,8,"FTPCmd",DialogItems,FP_DIALOG_SIZE(InitItems) );

		if ( AskCode != 4 )
			return -1;

		DestPath = Unicode::fromOem(DialogItems[2].Data);
	}

	if (DestPath.empty() )
		return -1;

	if ( !ExpandList(PanelItem,ItemsNumber,&il,TRUE) )
		return 0;

	int OverwriteAll = FALSE,
		SkipAll      = FALSE,
		Rename       = FALSE;
	size_t      n;
	const FTPHost* p;
	FTPHost  h;
	std::wstring  CheckKey;
	std::wstring  DestName;

	if (DestPath == L"..")
	{
		if (hostsPath_.empty())
			return 0;
		else
		{
			CheckKey = getPathBranch(hostsPath_);
			Rename = true;
		}
	} else
	{
		CheckKey = hostsPath_;
		if (DestPath.find_first_of(L":\\") == std::wstring::npos)
		{
			Rename = true;
		} else
			if(g_manager.getRegKey().get((CheckKey + L"\\Folder").c_str(), 0))
			{
				AddEndSlash(CheckKey,'\\' );
				Rename=TRUE;
			}

			/*
			if (DestPath.find_first_of(L":\\") == std::wstring::npos) //&& 
			// TODO !!!!			!FP_CheckRegKey(CheckKey))
			{
			Rename=TRUE;
			} else
			if (FP_GetRegKey(CheckKey.c_str(), L"Folder", 0))
			{
			AddEndSlash(CheckKey,'\\' );
			Rename=TRUE;
			}
			*/
	}

	AddEndSlash( DestPath, '\\' );

	//Rename
	if (Rename) {
		for( n=0; n < il.Count(); n++ ) {
			p = findhost(il.List[n].UserData);
			if ( !p ) continue;

			//Check for folders
			if ( p->Folder ) {
				SayMsg( FMSG(MCanNotMoveFolder) );
				return TRUE;
			}

			h.Assign(p);
// TODO			h.regPath_ = L"";

			if(!h.Write(this, CheckKey))
				return FALSE;

			if ( Move && !p->Folder)
			{
				// TODO        FP_DeleteRegKey(p->regKey_);
				if ( n < ItemsNumber ) PanelItem[n].Flags &= ~PPIF_SELECTED;
			}
		}
	}//Rename
	else
		//INI
		for( n = 0; n < il.Count(); n++ ) {

			p = findhost(il.List[n].UserData);
			if (!p) continue;

			if ( p->Folder ) {
				continue;
			}

			DestName = p->MkINIFile(hostsPath_, DestPath);

			DWORD DestAttr=GetFileAttributesW(DestName.c_str());

			if ( !IS_SILENT(OpMode) &&
				!OverwriteAll &&
				DestAttr != 0xFFFFFFFF ) {

					if (SkipAll)
						continue;

					CONSTSTR MsgItems[] = {
						FMSG( Move ? MMoveHostTitle:MCopyHostTitle ),
						FMSG( IS_FLAG(DestAttr,FILE_ATTRIBUTE_READONLY) ? MAlreadyExistRO : MAlreadyExist ),
						Unicode::toOem(DestName).c_str(),
						FMSG( MAskOverwrite ),
						/*0*/FMSG(MOverwrite),
						/*1*/FMSG(MOverwriteAll),
						/*2*/FMSG(MCopySkip),
						/*3*/FMSG(MCopySkipAll),
						/*4*/FMSG(MCopyCancel) };

						int MsgCode = FMessage( FMSG_WARNING,NULL,MsgItems,sizeof(MsgItems)/sizeof(MsgItems[0]),5 );

						switch(MsgCode) {
		  case 1:
			  OverwriteAll=TRUE;
			  break;
		  case 3:
			  SkipAll=TRUE;
		  case 2:
			  continue;
		  case -1:
		  case 4:
			  return(	-1);
						}
			}
			int WriteFailed = FALSE;
			if ( DestAttr!=0xFFFFFFFF ) {
				if ( !DeleteFileW(DestName.c_str()) )
					if ( !SetFileAttributesW(DestName.c_str(),FILE_ATTRIBUTE_NORMAL) && !DeleteFileW(DestName.c_str()) )
						WriteFailed=TRUE;
			}

			if ( !WriteFailed )
			{
				if ( !p->WriteINI(DestName) )
				{
					WriteFailed=TRUE;
					DeleteFileW(DestName.c_str());
				} else
					if (Move)
			  {
				  BOOST_ASSERT(0 && "TODO");
				  //	TODO			  FP_DeleteRegKey(p->regKey_);
			  }
			}

			if (WriteFailed) {
				std::string &s = Unicode::toOem(DestName);
				CONSTSTR MsgItems[] = {
					FMSG(MError),
					FMSG(MCannotCopyHost),
					s.c_str(),
					FMSG(MOk)
				};
				FMessage( FMSG_WARNING|FMSG_DOWN|FMSG_ERRORTYPE,NULL,MsgItems,sizeof(MsgItems)/sizeof(MsgItems[0]),1 );
				return(0);
			}
		}//INI

		if (Move)
			for (n = il.Count()-1; n >= 0; n-- ) {

				if (CheckForEsc(FALSE))
					return -1;

				p = findhost(il.List[n].UserData);
				if ( p && p->Folder )
				{
					BOOST_ASSERT(0 && "TODO");
					// TODO				FP_DeleteRegKey(p->regKey_);
					if ( n < ItemsNumber )
						PanelItem[n].Flags &= ~PPIF_SELECTED;
				}
			}
			return 1;
}
