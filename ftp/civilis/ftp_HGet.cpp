#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "farwrapper/dialog.h"

int FTP::GetHostFiles( struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move, std::wstring& DestPath,int OpMode)
{
	PROCP(ItemsNumber << L" [" << DestPath << L"] " << (Move? L"MOVE " : L"COPY ") << OpMode);

	FARWrappers::ItemList  il;

	if(!IS_SILENT(OpMode))
	{
		FARWrappers::Dialog dlg(L"FTPCmd");
		dlg.addDoublebox( 3, 1,72,6,	Move? getMsg(MMoveHostTitle) : getMsg(MCopyHostTitle))->
			addLabel	( 5, 2,			Move? getMsg(MMoveHostTo) : getMsg(MCopyHostTo))->
			addEditor	( 5, 3,70,		&DestPath)->
			addHLine	( 3, 4)->
			addDefaultButton(0, 5, 1,	getMsg(MCopy))->
			addButton	( 0, 5, -1,		getMsg(MCancel));

		if(dlg.show(76, 8) == -1)
			return -1;
	}

	if (DestPath.empty() )
		return -1;

	BOOST_ASSERT(0 && "TODO");
//	if ( !ExpandList(PanelItem,ItemsNumber,&il,TRUE) )
//		return 0;

	int OverwriteAll = FALSE,
		SkipAll      = FALSE,
		Rename       = FALSE;
	size_t      n;
	const FTPHost* p;
//	FTPHost  h;
	std::wstring  CheckKey;
	std::wstring  DestName;

	if (DestPath == L"..")
	{
		if(panel_->getCurrentDirectory().empty())
			return 0;
		else
		{
			CheckKey = getPathBranch(panel_->getCurrentDirectory());
			Rename = true;
		}
	} else
	{
		CheckKey = panel_->getCurrentDirectory();
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
	if(Rename)
	{
		for( n=0; n < il.size(); n++ )
		{
			p = hostPanel_.findhost(il[n].UserData);
			if(!p)
				continue;

			//Check for folders
			if ( p->Folder )
			{
				FARWrappers::message(MCanNotMoveFolder);
				return TRUE;
			}

			FTPHost h;
			h.Assign(p);
// TODO			h.regPath_ = L"";

			BOOST_ASSERT(0 && "Todo or check (write)");
			if(!h.Write(CheckKey))
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
		for( n = 0; n < il.size(); n++ ) {

			p = hostPanel_.findhost(il[n].UserData);
			if (!p) continue;

			if ( p->Folder )
				continue;

			DestName = DestPath + L'\\' + p->getIniFilename();

			DWORD DestAttr=GetFileAttributesW(DestName.c_str());

			if ( !IS_SILENT(OpMode) &&
				!OverwriteAll &&
				DestAttr != 0xFFFFFFFF )
			{

					if (SkipAll)
						continue;

					const wchar_t* MsgItems[] =
					{
						getMsg( Move ? MMoveHostTitle:MCopyHostTitle ),
						getMsg( is_flag(DestAttr,FILE_ATTRIBUTE_READONLY) ? MAlreadyExistRO : MAlreadyExist ),
						DestName.c_str(),
						getMsg( MAskOverwrite ),
						/*0*/getMsg(MOverwrite),
						/*1*/getMsg(MOverwriteAll),
						/*2*/getMsg(MCopySkip),
						/*3*/getMsg(MCopySkipAll),
						/*4*/getMsg(MCopyCancel)
					};

					int MsgCode = FARWrappers::message(MsgItems, 5, FMSG_WARNING);

					switch(MsgCode)
					{
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

			if (WriteFailed)
			{
				const wchar_t* MsgItems[] =
				{
					getMsg(MError),
					getMsg(MCannotCopyHost),
					DestName.c_str(),
					getMsg(MOk)
				};
				FARWrappers::message(MsgItems, 1, FMSG_WARNING|FMSG_DOWN|FMSG_ERRORTYPE);
				return(0);
			}
		}//INI

		if (Move)
			for (n = il.size()-1; n >= 0; n-- ) {

				if (CheckForEsc(FALSE))
					return -1;

				p = hostPanel_.findhost(il[n].UserData);
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
