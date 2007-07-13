#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

//------------------------------------------------------------------------
CONSTSTR UType2Str( sliTypes tp )
  {
    switch( tp ) {
      case sltUrlList: return "URLS_LIST";
      case    sltTree: return "URLS_TREE";
      case   sltGroup: return "URLS_GROUP";
              default: HAbort( "Url type not supported" );
                       return NULL;
    }
}
sliTypes Str2UType( CONSTSTR s )
  {
    if ( StrCmpI( s,"URLS_LIST" ) == 0 ) return sltUrlList;
    if ( StrCmpI( s,"URLS_TREE" ) == 0 ) return sltTree;
    if ( StrCmpI( s,"URLS_GROUP" ) == 0 ) return sltGroup;
 return sltNone;
}
//------------------------------------------------------------------------
CONSTSTR GetOtherPath( char *path )
  {  PanelInfo pi;

     do{
       if ( FP_Info->Control( INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELSHORTINFO, &pi ) &&
            pi.PanelType == PTYPE_FILEPANEL &&
            !pi.Plugin )
         break;

       if ( FP_Info->Control( INVALID_HANDLE_VALUE, FCTL_GETPANELSHORTINFO, &pi ) &&
            pi.PanelType == PTYPE_FILEPANEL &&
            !pi.Plugin )
         break;

       return FMSG(MFLErrGetInfo);
     }while(0);

     StrCpy( path, pi.CurDir, FAR_MAX_PATHSIZE );
     AddEndSlash( path,'\\',FAR_MAX_PATHSIZE );
 return NULL;
}

void SayOutError( CONSTSTR m,int tp = 0 )
  {  CONSTSTR itms[] = { FMSG(MFLErrCReate), NULL, FMSG(MOk) };
     itms[1] = m;
     FMessage( tp + FMSG_WARNING,NULL,itms,3,1 );
}

void FTP::SaveList( FP_SizeItemList* il )
{  
	CONSTSTR m;

     if ( (m=GetOtherPath(g_manager.opt.sli.path)) != NULL ) {
       SayOutError( m );
       return;
     }

     StrCat( g_manager.opt.sli.path,"ftplist.lst" );

     if ( !AskSaveList(&g_manager.opt.sli) )
       return;

     FILE *f = fopen( g_manager.opt.sli.path, g_manager.opt.sli.Append ? "a" : "w" );
     if ( !f ) {
       SayOutError( FMSG(MFLErrCReate),FMSG_ERRORTYPE );
       return;
     }

     PPluginPanelItem p;
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

     for( n = 0; n < il->Count(); n++ ) {
       p = il->Item(n);

       if ( p->FindData.dwReserved1 == MAX_DWORD )
         continue;

     //URLS --------------------------------------
       if ( g_manager.opt.sli.ListType == sltUrlList ) {
         if ( IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
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
                  IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ? '/' : ' ', m );

         if ( g_manager.opt.sli.Size ) {
           level = Max( 1, g_manager.opt.sli.RightBound - 10 - level*2 - 2 - 1 - (int)strlen(m) );
           fprintf( f,"%*c",level,' ' );

           if ( IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
             fprintf( f,"<DIR>" );
            else
             fprintf( f,"%10I64u",
                      ((__int64)p->FindData.nFileSizeHigh) * MAX_DWORD + ((__int64)p->FindData.nFileSizeLow) );
         }
         fprintf( f,"\n" );

       } else
     //GROUPS ------------------------------------
       if ( g_manager.opt.sli.ListType == sltGroup )
	   {
         if ( IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
           continue;

         FixFTPSlash( FTP_FILENAME(p) );
         SNprintf( str, sizeof(str), "%s%s", BasePath, FTP_FILENAME(p) );
         if ( !IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
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
                    ((__int64)p->FindData.nFileSizeHigh) * MAX_DWORD + ((__int64)p->FindData.nFileSizeLow) );
         }
         fprintf( f,"\n" );
       }
     }
     fclose(f);

     CONSTSTR itms[] = { FMSG(MFLDoneTitle), FMSG(MFLFile), g_manager.opt.sli.path, FMSG(MFLDone), FMSG(MOk) };
     FMessage( FMSG_LEFTALIGN,NULL,itms,5,1 );
}
//------------------------------------------------------------------------
#define MNUM( v ) ( ((int*)(v).Text)[ 128/sizeof(int) - 1] )
#define MSZ       (128-sizeof(int))

BOOL FTP::ShowFilesList( FP_SizeItemList* il )
{
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
       p->FindData.dwReserved1 = 0;

	   w = std::max(w,strlen(PointToName(Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str())) + (int)p->NumberOfLinks + 1 );
       cn++;

	   MNUM(mi[i++]) = static_cast<int>(n);
     }
	 w = std::min(std::max(static_cast<size_t>(60), std::min(w, MSZ)), FP_ConWidth()-8 );

     if ( !cn ) return FALSE;

     //Calc length of size and count digits
     int szSize = 0,
         szCount = 0;

     for ( n = 0; n < cn; n++ ) {
       p = il->Item( MNUM(mi[n]) );
       FDigit( str,
               ((__int64)p->FindData.nFileSizeHigh) * ((__int64)MAX_DWORD) + ((__int64)p->FindData.nFileSizeLow),
               -1 );
       szSize = Max( strLen(str),szSize );

       if ( IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ) {
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
                 ((__int64)p->FindData.nFileSizeHigh) * ((__int64)MAX_DWORD) + ((__int64)p->FindData.nFileSizeLow),
                 -1 );
         m += Sprintf( m,"%*s",szSize,str );
       }

       if ( szCount ) {
         *(m++) = ' ';
         *(m++) = FAR_VERT_CHAR;
         *(m++) = ' ';
         if ( IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) )
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
         mi[n].Checked = il->Items()[ MNUM(mi[n]) ].FindData.dwReserved1 != MAX_DWORD;

       //Title
       __int64 tsz = 0,tcn = 0;
       for ( n = 0; n < cn; n++ ) {
         p = il->Item( MNUM(mi[n]) );
         if ( p->FindData.dwReserved1 != MAX_DWORD &&
              !IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ) {
           tsz += ((__int64)p->FindData.nFileSizeHigh) * ((__int64)MAX_DWORD) +
                  ((__int64)p->FindData.nFileSizeLow);
           tcn++;
         }
       }

       StrCpy( str,FP_GetMsg(FMSG(MListTitle)),sizeof(str) );
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
       n = FP_Info->Menu( FP_Info->ModuleNumber,-1,-1,0,FMENU_SHOWAMPERSAND,
                          str,
                          FP_GetMsg(FMSG(MListFooter)),
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
         /*INS*/
         case 0: //Current
                 p = il->Item( MNUM(mi[num]) );
                 //Next item
                 n = num+1;
                 //Switch selected
                 set = p->FindData.dwReserved1 != MAX_DWORD;
                 p->FindData.dwReserved1 = set ? MAX_DWORD : 0;

                 //Switch all nested
                 if ( IS_FLAG(p->FindData.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ) {
					 i = StrSlashCount( Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() );
                   for ( ; n < cn; n++ ) {
                     p = il->Item( MNUM(mi[n]) );
                     if ( StrSlashCount( Unicode::utf16ToUtf8(FTP_FILENAME(p)).c_str() ) <= i )
                       break;
                     p->FindData.dwReserved1 = set ? MAX_DWORD : 0;
                   }
                 }
                 //INS-moves-down
                 if ( n < cn ) {
                   mi[num].Selected = FALSE;
                   mi[n].Selected   = TRUE;
                   num = n;
                 }
              break;
         /*F2*/
         case 1: SaveList( il );
              break;
       }

     }while( 1 );

     _Del(mi);
     if ( !num )
       return FALSE;

 return TRUE;
}
