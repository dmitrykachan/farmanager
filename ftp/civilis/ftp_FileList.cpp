#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

const wchar_t* GetOtherPath(std::wstring &path)
{
	PanelInfo pi;

	do
	{
		if(FARWrappers::getShortPanelInfo(pi, false) && pi.PanelType == PTYPE_FILEPANEL && !pi.Plugin)
			break;

		if(FARWrappers::getShortPanelInfo(pi) && pi.PanelType == PTYPE_FILEPANEL && !pi.Plugin)
			break;

		return getMsg(MFLErrGetInfo);
	} while(0);

	path = pi.lpwszCurDir;
	AddEndSlash(path, '\\');
	return NULL;
}

void SayOutError(const wchar_t* m, int tp = 0)
{
	 FARWrappers::message(MFLErrCReate, m, tp | FMSG_WARNING);
}

void FTP::SaveList(FARWrappers::ItemList* il)
{
	BOOST_ASSERT(0 && "TODO");

/*
	const wchar_t* m;

	if ((m = GetOtherPath(g_manager.opt.sli.path)) != NULL )
	{
		SayOutError( m );
		return;
	}

	g_manager.opt.sli.path = L"ftplist.lst";

	if ( !AskSaveList(&g_manager.opt.sli) )
		return;

	FILE *f = _wfopen(g_manager.opt.sli.path.c_str(), g_manager.opt.sli.Append ? L"a" : L"w" );
	if ( !f )
	{
		SayOutError( getMsg(MFLErrCReate),FMSG_ERRORTYPE );
		return;
	}

	PluginPanelItem* p;
	size_t           n;
	int              level;
	char             str[1024+2],
		BasePath[1024+2];
	std::string		CurrentUrlPath;

	SNprintf( BasePath, sizeof(BasePath),
		"%s%s%s%s",
		g_manager.opt.sli.AddPrefix ? "ftp://" : "",
		g_manager.opt.sli.AddPasswordAndUser ? Message( "%s:%s@",hConnect->userName_, hConnect->UserPassword ) : "",
		hConnect->hostname,
		hConnect->curdir_.c_str() );
	TAddEndSlash( BasePath,'/' );

	if ( g_manager.opt.sli.ListType == sltTree )
		fprintf( f,"BASE: \"%s\"\n",BasePath );

	for( n = 0; n < il->Count(); n++ )
	{
		p = il->Item(n);

		if (p->Reserved[0] == UINT_MAX)
			continue;

		//URLS --------------------------------------
		if ( g_manager.opt.sli.ListType == sltUrlList ) {
			if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
				continue;

			FixFTPSlash( FTP_FILENAME(p) );
			SNprintf( str,sizeof(str),"%s%s",BasePath,FTP_FILENAME(p) );
			if ( g_manager.opt.sli.Quote ) QuoteStr( str );

			fprintf( f,"%s\n",str );
		} else
			//TREE --------------------------------------
			if ( g_manager.opt.sli.ListType == sltTree ) {
				TStrCpy( str, Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() );

				FixFTPSlash( str );
				for( m = str,level = 0;
					(m=strchr(m,'/')) != NULL;
					m++,level++ );

				fprintf( f,"%*c", level*2+2, ' ' );
				m = strrchr( str,'/' );

				if (m) m++; else m = str;

				fprintf( f,"%c%s",
					is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ? '/' : ' ', m );

				if ( g_manager.opt.sli.Size ) {
					level = Max( 1, g_manager.opt.sli.RightBound - 10 - level*2 - 2 - 1 - (int)strlen(m) );
					fprintf( f,"%*c",level,' ' );

					if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
						fprintf( f,"<DIR>" );
					else
						fprintf( f,"%10I64u",
						((__int64)p->FindData.nFileSizeHigh) * UINT_MAX + ((__int64)p->FindData.nFileSizeLow) );
				}
				fprintf( f,"\n" );

			} else
				//GROUPS ------------------------------------
				if ( g_manager.opt.sli.ListType == sltGroup )
				{
					if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
						continue;

					FixFTPSlash( FTP_FILENAME(p) );
					SNprintf( str, sizeof(str), "%s%s", BasePath, FTP_FILENAME(p) );
					if ( !is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
						*strrchr( str,'/' ) = 0;

					if(!boost::iequals(CurrentUrlPath, str))
					{
						CurrentUrlPath = str;
						fprintf( f,"\n[%s]\n", CurrentUrlPath.c_str());
					}
					TStrCpy( str, Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() );
					FixFTPSlash( str );
					m = strrchr(str,'/');
					if (m) m++; else m = str;
					fprintf( f," %s", m );

					if ( g_manager.opt.sli.Size ) {
						level = Max( 1, g_manager.opt.sli.RightBound - 10 - (int)strlen(m) - 1 );
						fprintf( f,"%*c%10I64u",
							level,' ',
							((__int64)p->FindData.nFileSizeHigh) * UINT_MAX + ((__int64)p->FindData.nFileSizeLow) );
					}
					fprintf( f,"\n" );
				}
	}
	fclose(f);

	const wchar_t* itms[] = { getMsg(MFLDoneTitle), getMsg(MFLFile), g_manager.opt.sli.path, getMsg(MFLDone), getMsg(MOk) };
	FMessage( FMSG_LEFTALIGN,NULL,itms,5,1 );
*/
}
//------------------------------------------------------------------------
#define MNUM( v ) ( ((int*)(v).Text)[ 128/sizeof(int) - 1] )
#define MSZ       (128-sizeof(int))

BOOL FTP::ShowFilesList(FARWrappers::ItemList* il)
{
	BOOST_ASSERT(0 && "TODO");
	return true;
	/*

	size_t			n,i,cn,w, num;
	int              Breaks[] = { VK_INSERT, VK_F2, 0 },
		BNumber;
	char             str[ 500 ];
	PluginPanelItem *p;
	FarMenuItem     *mi = NULL;
	char            *m;
	const char      *nm;

	if ( !il || !il->Count() )
		return FALSE;

	//Create|Recreate
	mi = (FarMenuItem *)_Realloc( mi,static_cast<DWORD>(il->Count()*sizeof(FarMenuItem)));
	MemSet( mi, 0, il->Count()*sizeof(FarMenuItem) );

	//Scan number of items
	w = cn = 0;
	for ( i = n = 0; n < il->Count(); n++ ) {
		p = il->Item(n);
		p->NumberOfLinks        = static_cast<int>(StrSlashCount( Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str()));
		p->Reserved[0] = 0;

		w = std::max(w,strlen(PointToName(Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str())) + (int)p->NumberOfLinks + 1 );
		cn++;

		MNUM(mi[i++]) = static_cast<int>(n);
	}
	w = std::min(std::max(static_cast<size_t>(60), std::min(w, MSZ)), FP_ConWidth()-8 );

	if ( !cn ) return FALSE;

	//Calc length of size and count digits
	int szSize = 0,
		szCount = 0;

	for ( n = 0; n < cn; n++ )
	{
		p = il->Item( MNUM(mi[n]) );
		FDigit(str,
			(p->FindData.nFileSize,
			-1 );
		szSize = Max( strLen(str),szSize );

		if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ) {
			FDigit( str,p->FindData.dwReserved0,-1 );
			szCount = Max( strLen(str),szCount );
		}
	}

	//Filename width
	w -= szSize + szCount + 6;

	//Set menu item text
	for ( n = 0; n < cn; n++ ) {
		p = il->Item( MNUM(mi[n]) );

		m = mi[n].Text;
		for ( i = 0; i < (int)p->NumberOfLinks; i++ ) {
			*(m++) = ' ';
			*(m++) = ' ';
		}

		nm = PointToName( Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() );
		for ( i = (int)(m - mi[n].Text); i < w; i++ )
			if ( *nm )
				*(m++) = *(nm++);
			else
				*(m++) = ' ';

		*(m++) = (*nm) ? FAR_SBMENU_CHAR : ' ';

		if ( szSize ) {
			*(m++) = ' ';
			*(m++) = FAR_VERT_CHAR;
			*(m++) = ' ';
			FDigit( str,
				((__int64)p->FindData.nFileSizeHigh) * ((__int64)UINT_MAX) + ((__int64)p->FindData.nFileSizeLow),
				-1 );
			m += Sprintf( m,"%*s",szSize,str );
		}

		if ( szCount ) {
			*(m++) = ' ';
			*(m++) = FAR_VERT_CHAR;
			*(m++) = ' ';
			if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
				FDigit( str,p->FindData.dwReserved0,-1 );
			else
				str[0] = 0;
			m += Sprintf( m,"%*s",szCount,str );
		}

		*m = 0;
	}

	num = -1;
	do{
		//Set selected
		for ( n = 0; n < cn; n++ )
			mi[n].Checked = il->Items()[ MNUM(mi[n]) ].FindData.dwReserved1 != UINT_MAX;

		//Title
		__int64 tsz = 0,tcn = 0;
		for ( n = 0; n < cn; n++ ) {
			p = il->Item( MNUM(mi[n]) );
			if ( p->FindData.dwReserved1 != UINT_MAX &&
				!is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ) {
					tsz += ((__int64)p->FindData.nFileSizeHigh) * ((__int64)UINT_MAX) +
						((__int64)p->FindData.nFileSizeLow);
					tcn++;
			}
		}

		StrCpy( str,getMsg(getMsg(MListTitle)),sizeof(str) );
		StrCat( str," (",sizeof(str) );
		StrCat( str,FDigit(NULL,tsz,-1),sizeof(str) );
		if ( il->TotalFullSize != tsz ) {
			StrCat( str,"{",sizeof(str) );
			StrCat( str,FDigit(NULL,il->TotalFullSize,-1),sizeof(str) );
			StrCat( str,"}",sizeof(str) );
		}
		StrCat( str,"/",sizeof(str) );
		StrCat( str,FDigit(NULL,tcn,-1),sizeof(str) );
		if ( tcn != il->TotalFiles ) {
			StrCat( str,"{",sizeof(str) );
			StrCat( str,FDigit(NULL,il->TotalFiles,-1),sizeof(str) );
			StrCat( str,"}",sizeof(str) );
		}
		StrCat( str,")",sizeof(str) );

		//Menu
		n = FARWrappers::getInfo().Menu( FP_Info->ModuleNumber,-1,-1,0,FMENU_SHOWAMPERSAND,
			str,
			getMsg(getMsg(MListFooter)),
			"FTPFilesList", Breaks, &BNumber, mi, static_cast<int>(cn));
		//key ESC
		if ( n == -1 )       { num = FALSE; break; }
		//key Enter
		if ( BNumber == -1 ) { num = TRUE; break; }

		//Set selected
		if ( num != -1 ) mi[num].Selected = FALSE;
		num = n;
		mi[num].Selected = TRUE;

		//Process keys
		bool set;
		switch( BNumber ) {
			/ *INS* /
		 case 0: //Current
			 p = il->Item( MNUM(mi[num]) );
			 //Next item
			 n = num+1;
			 //Switch selected
			 set = p->FindData.dwReserved1 != UINT_MAX;
			 p->FindData.dwReserved1 = set ? UINT_MAX : 0;

			 //Switch all nested
			 if ( is_flag(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ) {
				 i = StrSlashCount( Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() );
				 for ( ; n < cn; n++ ) {
					 p = il->Item( MNUM(mi[n]) );
					 if ( StrSlashCount( Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() ) <= i )
						 break;
					 p->FindData.dwReserved1 = set ? UINT_MAX : 0;
				 }
			 }
			 //INS-moves-down
			 if ( n < cn ) {
				 mi[num].Selected = FALSE;
				 mi[n].Selected   = TRUE;
				 num = n;
			 }
			 break;
			 / *F2* /
		 case 1: SaveList( il );
			 break;
		}

	}while( 1 );

	_Del(mi);
	if ( !num )
		return FALSE;

	return TRUE;
*/
}

