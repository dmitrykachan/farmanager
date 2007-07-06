#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

bool SayNotReadedTerminates(const std::wstring& fnm, BOOL& SkipAll)
{
	std::string s = Unicode::toOem(fnm);
	CONSTSTR MsgItems[] =
	{
		FMSG(MError),
		FMSG(MCannotCopyHost),
		s.c_str(),
		/*0*/FMSG(MCopySkip),
		/*1*/FMSG(MCopySkipAll),
		/*2*/FMSG(MCopyCancel)
	};

	if ( SkipAll )
		return false;

	switch( FMessage( FMSG_WARNING|FMSG_DOWN|FMSG_ERRORTYPE, NULL, MsgItems, ARRAY_SIZE(MsgItems), 3 ) )
	{
		case  1: SkipAll = false;
		case  0: break;
		default: return false;
	}

	return false;
}

int FTP::PutHostsFiles( struct PluginPanelItem *PanelItem, size_t ItemsNumber,int Move,int OpMode )
{
	BOOL            SkipAll = FALSE;
	std::wstring	DestPath, SrcFile, DestName, CurName;
	DWORD           SrcAttr;
	size_t			n;
	FTPHost         h;
	FP_SizeItemList il;

	DestPath = hostsPath_ + L'\\';

	if (!ExpandList(PanelItem,ItemsNumber,&il,FALSE))
		return 0;

	for(n = 0; n < il.Count(); n++ )
	{

		if (CheckForEsc(FALSE))
			return -1;

		h.Init();
		CurName = FTP_FILENAME(&il.List[n]);

		SrcAttr = GetFileAttributesW(CurName.c_str());
		if (SrcAttr==0xFFFFFFFF)
			continue;

		if ( IS_FLAG(SrcAttr,FILE_ATTRIBUTE_DIRECTORY) )
		{
			h.Folder = TRUE;
			h.Host_ = CurName.c_str();
			h.Write(this, hostsPath_);
			continue;
		}

		if(CurName.find(L'\\') == std::wstring::npos)
		{
			SrcFile = L".\\" + CurName;
			DestName = DestPath;
		} else
		{
			SrcFile = CurName;
			DestName = DestPath + CurName;
			size_t n = DestName.find('\\');
			BOOST_ASSERT(n != std::wstring::npos);
			DestName.resize(n);
		}

		if(!h.ReadINI(SrcFile))
		{
			if ( !IS_SILENT(OpMode) )
				if ( SayNotReadedTerminates(SrcFile, SkipAll))
					return -1;
			continue;
		}

		Log(( "Write new host [%s] -> [%s]", SrcFile.c_str(), DestName.c_str() ));
		h.Write(this, DestName);

		if (Move)
		{
			SetFileAttributesW(CurName.c_str(),0);
			DeleteFileW(CurName.c_str());
		}

		if ( n < ItemsNumber )
			PanelItem[n].Flags &= ~PPIF_SELECTED;
	}

	if (Move) {
		for (n = il.Count()-1; n >= 0; n-- ) {
			if ( CheckForEsc(FALSE) )
				return -1;

			if ( IS_FLAG(il.List[n].FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
				if ( RemoveDirectoryW( FTP_FILENAME(&il.List[n]).c_str() ))
					if ( n < ItemsNumber )
						PanelItem[n].Flags &= ~PPIF_SELECTED;
		}
	}

	return 1;
}
