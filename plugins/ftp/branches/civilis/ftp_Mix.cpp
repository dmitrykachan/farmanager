#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

FTPCopyInfo::FTPCopyInfo( void )
  {
     asciiMode       = FALSE;
     ShowProcessList = FALSE;
     AddToQueque     = FALSE;
     MsgCode         = ocNone;
     Download        = FALSE;
     UploadLowCase   = FALSE;
     FTPRename       = FALSE;
}

//------------------------------------------------------------------------
bool DECLSPEC IsAbsolutePath(const std::wstring &path)
{
	return path.size() > 3 &&
         path[1] == ':' && path[2] == SLASH_CHAR;
}
//------------------------------------------------------------------------
const char quotes[] = " \"%,;[]";

// TODO remove
void DECLSPEC QuoteStr( char *str )
  {  char buff[ 1024 ],*m,*src;

    if ( StrChr(str,quotes) == NULL )
      return;

    m = buff;
    src = str;
    *m++ = '\"';
    for( int n = 0; n < sizeof(buff)-3 && *src; n++ )
      if ( *src == '\"' ) {
        *m++ = '\"';
        *m++ = '\"';
        n++;
        src++;
      } else
        *m++ = *src++;

    *m++ = '\"';
    *m = 0;
    strcpy( str,buff );
}

void DECLSPEC QuoteStr(std::wstring& str)
{  
	if(str.find_first_of(Unicode::utf8ToUtf16(quotes)) == std::wstring::npos)
		return;

	str.insert(str.begin(), L'\"');
	std::wstring::iterator itr = str.begin()+1;
	while((itr = std::find(itr, str.end(), L'\"')) != str.end())
	{
		++itr;
		itr = str.insert(itr, L'\"');
	}
}

// TODO remove
void DECLSPEC QuoteStr( String& str )
  {  String  buff;

    if ( str.Chr(quotes) == -1 )
      return;

    buff.Add( '\"' );
    for( int n = 0; n < str.Length(); n++ )
      if ( str[n] == '\"' ) {
        buff.Add( '\"' );
        buff.Add( '\"' );
      } else
        buff.Add( str[n] );

    buff.Add( '\"' );
    str = buff;
}

//------------------------------------------------------------------------
#define SIZE_M 1024*1024
#define SIZE_K 1024

void DECLSPEC Size2Str( char *buff,DWORD sz )
  {  char   letter = 0;
     double size = (double)sz;
     size_t    rc;

     if ( size >= SIZE_M ) {
       size /= SIZE_M;
       letter = 'M';
     } else
     if ( size >= SIZE_K ) {
       size /= SIZE_K;
       letter = 'K';
     }

     if ( !letter ) {
       Sprintf( buff,"%d",sz );
       return;
     }

     Sprintf( buff,"%f",size );
     rc = strlen(buff);
     if ( !rc || strchr(buff,'.') == NULL )
       return;

     for ( rc--; rc && buff[rc] == '0'; rc-- );
     if ( buff[rc] != '.' )
       rc++;
     buff[rc]   = letter;
     buff[rc+1] = 0;
}

DWORD DECLSPEC Str2Size( char *str )
{  
	size_t    rc = strlen( str );
	double sz;
	char   letter;

	if ( !rc )
		return 0;

	rc--;
	if ( str[rc] == 'k' || str[rc] == 'K' )
		letter = 'K';
	else
		if ( str[rc] == 'm' || str[rc] == 'M' )
			letter = 'M';
		else
			letter = 0;

	if ( letter )
		str[rc] = 0;

	sz = atof( str );

	if ( letter == 'K' ) sz *= SIZE_K; else
		if ( letter == 'M' ) sz *= SIZE_M;

	return (DWORD)sz;
}
//------------------------------------------------------------------------
size_t DECLSPEC StrSlashCount(CONSTSTR m)
{
	size_t cn = 0;

	if ( m )
		for( ; *m; m++ )
			if ( *m == '/' || *m == '\\' )
				cn++;

	return cn;
}
//------------------------------------------------------------------------
BOOL DECLSPEC FTestOpen( CONSTSTR nm )
  {  HANDLE f;
     BOOL   rc;

     f = CreateFile( nm, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
     rc = f &&
          f != INVALID_HANDLE_VALUE &&
          GetFileType(f) == FILE_TYPE_DISK;

     CloseHandle( f );

 return rc;
}

BOOL DECLSPEC FRealFile( CONSTSTR nm,WIN32_FIND_DATA* fd )
  {  HANDLE f;
     BOOL   rc;

     f = CreateFile( nm, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
     rc = f &&
          f != INVALID_HANDLE_VALUE &&
          GetFileType(f) == FILE_TYPE_DISK;

     if ( rc && fd ) {
       StrCpy( fd->cFileName, nm );
       fd->dwFileAttributes = GetFileAttributes(nm);
       fd->nFileSizeLow     = GetFileSize( f, &fd->nFileSizeHigh );
       GetFileTime( f, &fd->ftCreationTime, &fd->ftLastAccessTime, &fd->ftLastWriteTime );
     }

     CloseHandle( f );

 return rc;
}

__int64 DECLSPEC Fsize( CONSTSTR nm )
  {  HANDLE f;
     DWORD lo,hi;

     f = CreateFile( nm, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
     if ( !f || f == INVALID_HANDLE_VALUE )
       return 0;
     lo = GetFileSize( f,&hi );
     CloseHandle( f );

     if ( lo == MAX_DWORD )
       return 0;
      else
       return ((__int64)hi) * ((__int64)MAX_DWORD) + ((__int64)lo);
}

__int64 DECLSPEC Fsize( HANDLE File )
  {  DWORD low,hi;

     low = GetFileSize( File,&hi );
     if ( low == MAX_DWORD )
       return 0;
      else
       return ((__int64)hi) * ((__int64)MAX_DWORD) + ((__int64)low);
}

BOOL DECLSPEC Fmove( HANDLE file,__int64 restart )
  {  LONG lo = (LONG)( restart % ((__int64)MAX_DWORD) ),
          hi = (LONG)( restart / ((__int64)MAX_DWORD) );

    if ( SetFilePointer( file,lo,&hi,FILE_BEGIN ) == 0xFFFFFFFF &&
         GetLastError() != NO_ERROR )
      return FALSE;

 return TRUE;
}

void DECLSPEC Fclose( HANDLE file )
  {
    if ( file ) {
      SetEndOfFile( file );
      CloseHandle( file );
    }
}

BOOL DECLSPEC Ftrunc( HANDLE h,DWORD move )
  {
     if ( move != FILE_CURRENT )
       if ( SetFilePointer(h,0,NULL,move) == 0xFFFFFFFF )
         return FALSE;

 return SetEndOfFile(h);
}

HANDLE DECLSPEC Fopen( CONSTSTR nm,CONSTSTR mode /*R|W|A[+]*/, DWORD attr )
  {  BOOL   rd  = toupper(mode[0]) == 'R';
     HANDLE h;

     if ( rd )
       h = CreateFile( nm, GENERIC_READ,
                       FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, attr, NULL );
      else
       h = CreateFile( nm, GENERIC_WRITE,
                       FILE_SHARE_READ, NULL, OPEN_ALWAYS, attr, NULL );

     if ( !h ||
          h == INVALID_HANDLE_VALUE )
       return NULL;

     do{
       if ( toupper(mode[0]) == 'A' || mode[1] == '+' )
         if ( SetFilePointer(h,0,NULL,FILE_END) == 0xFFFFFFFF )
           break;

       if ( !rd )
         SetEndOfFile(h);  //Ignore SetEndOfFile result in case of use with CON, NUL and others

       return h;
     }while(0);

     CloseHandle(h);
 return NULL;
}

int DECLSPEC Fwrite( HANDLE File,LPCVOID Buff, size_t Size)
{  
	DWORD res;
	return WriteFile(File,Buff,(DWORD)Size,&res,NULL) ? ((int)res) : (-1);
}

int DECLSPEC Fread( HANDLE File,LPVOID Buff,int Size )
  {  DWORD res;

 return ReadFile(File,Buff,(DWORD)Size,&res,NULL) ? ((int)res) : (-1);
}

void DMessage(const std::wstring &str, bool full, int color, int y)
{
	std::wstring err;

	if(full)
	{
		err = str;

		size_t len = err.size();
		size_t w = FP_ConWidth()-4;

		if(len < w )
			err.insert(err.end(), w - len, L' ');
		FP_Info->Text(2, y, color, Unicode::toOem(err).c_str());
	} else
		FP_Info->Text(2, y, color, Unicode::toOem(str).c_str());
}

void DECLSPEC IdleMessage(CONSTSTR str, int color)
{
	static HANDLE hScreen;

	//Clear
	if ( !str ) {
		if ( hScreen ) {
			FP_Info->RestoreScreen(hScreen);
			hScreen = NULL;
		}
		return;
	}

	//Draw
	std::wstring msg = Unicode::fromOem(FP_GetMsg(str));
	if(IS_FLAG(g_manager.opt.IdleMode, IDLE_CAPTION))
		WinAPI::setConsoleTitle(msg);

	if ( IS_FLAG(g_manager.opt.IdleMode,IDLE_CONSOLE) )
	{
		DWORD    er  = GetLastError();
		BOOL     err = er != ERROR_CANCELLED &&
			er != ERROR_SUCCESS &&
			er != ERROR_NO_MORE_FILES;

		if ( !hScreen )
			hScreen = FP_Info->SaveScreen(0,0, static_cast<int>(FP_ConWidth()),2);

		DMessage(msg, err, color, 0);
		if(err)
			DMessage(Unicode::fromOem(FIO_ERROR), err, color, 1);

		FP_Info->Text(0, 0, 0, NULL);
	}
}

int DECLSPEC FMessage( unsigned int Flags,CONSTSTR HelpTopic,CONSTSTR *Items,int ItemsNumber,int ButtonsNumber )
{
	time_t b = time(NULL);
	BOOL   delayed;
	int    rc;

	rc = FP_Message( Flags, HelpTopic, Items, ItemsNumber, ButtonsNumber, &delayed);
	if(delayed)
		g_manager.addWait(time(NULL)-b);

	return rc;
}


int DECLSPEC FMessage(unsigned int Flags, CONSTSTR HelpTopic, const std::vector<std::string> Items, int ButtonsNumber)
{
	time_t b = time(NULL);
	BOOL   delayed;
	int    rc;

	static size_t CMsgWidth  = 0;
	static size_t CMsgHeight = 0;

	//Check width and set repaint flag
	size_t  width = 0;

	//Array of lines - check if lines are message id
	std::vector<std::string>::const_iterator itr = Items.begin();
	while(itr != Items.end())
	{
		width = std::max(width, itr->size());
		++itr;
	}

	//Calc if message need to be redrawn with smaller dimentions
	if (!CMsgWidth || width < CMsgWidth ||
		!CMsgHeight|| Items.size() < CMsgHeight)
			// Need restore bk
			if ( !ButtonsNumber &&           // No buttons
				(Flags & FMSG_MB_MASK) == 0 ) // No FAR buttons
				FP_Screen::RestoreWithoutNotes();

	boost::scoped_array<const char*> v(new const char* [Items.size()]);
	for(size_t i = 0; i < Items.size(); ++i)
	{
		v[i] = Items[i].c_str();
	}
	rc = FP_Info->Message( FP_Info->ModuleNumber, Flags, HelpTopic,
			v.get(), static_cast<int>(Items.size()), ButtonsNumber);

	delayed = ButtonsNumber || (Flags&FMSG_MB_MASK) != 0;

	CMsgWidth  = width;
	CMsgHeight = Items.size();

	if(delayed)
		g_manager.addWait(time(NULL)-b);

	return rc;
}

int DECLSPEC FDialog( int X2,int Y2,CONSTSTR HelpTopic,struct FarDialogItem *Item,int ItemsNumber )
{
	time_t b = time(NULL);
	int    rc;

	rc = FP_Info->Dialog(FP_Info->ModuleNumber,-1,-1,X2,Y2,HelpTopic,Item,ItemsNumber );

	g_manager.addWait(time(NULL)-b);

	return rc;
}

int DECLSPEC FDialogEx( int X2,int Y2,CONSTSTR HelpTopic,struct FarDialogItem *Item,int ItemsNumber,DWORD Flags,FARWINDOWPROC DlgProc,long Param )
{
	time_t b = time(NULL);
	int    rc;

	if ( DlgProc == (FARWINDOWPROC)MAX_WORD ) // TODO
		DlgProc = FP_Info->DefDlgProc;

	rc = FP_Info->DialogEx(FP_Info->ModuleNumber,-1,-1,X2,Y2,HelpTopic,Item,ItemsNumber,0,Flags,DlgProc,Param );

	g_manager.addWait(time(NULL)-b);

	return rc;
}

void DECLSPEC AddEndSlash( String& p, char slash )
{
    if ( !p.Length() ) return;

    if ( !slash )
      slash = p.Chr('\\') ? '\\' : '/';

    if ( p[p.Length()-1] != slash )
      p.Add( slash );
}

void DECLSPEC AddEndSlash(std::wstring& s, wchar_t slash)
{
	if(s.size() == 0)
		return;

	if(slash == 0)
		slash = s.find(L'\\') != std::wstring::npos ? '\\' : '/';

	if(*s.rbegin() != slash)
		s.push_back(slash);
}



void DECLSPEC AddEndSlash( char *Path,char slash, size_t ssz )
  {  size_t Length;

     if ( !Path || !Path[0] ) return;

     Length = strLen(Path)-1;
     if ( Length <= 0 || Length >= ssz ) return;

     if ( !slash )
       slash = strchr(Path,'\\') ? '\\' : '/';

     if ( Path[Length] != slash ) {
       Path[Length+1] = slash;
       Path[Length+2] = 0;
     }
}

void DECLSPEC DelEndSlash( String& p,char shash )
{  
	int len;

	if ( (len=p.Length()-1) >= 0 &&
		p[len] == shash )
		p.SetLength( len );
}

void DECLSPEC DelEndSlash( char *Path,char shash )
{  
	int len;

	if ( Path && (len=strLen(Path)-1) >= 0 &&
		Path[len] == shash )
		Path[len] = 0;
}

void DECLSPEC DelEndSlash(std::wstring& s, char slash)
{  
	BOOST_ASSERT("TODO test");
	if(!s.empty() && *s.rbegin() == slash)
	{
		s.resize(s.size()-1);
	}
}


char* DECLSPEC TruncStr(char *Str,int MaxLength)
{
  int Length;
  if ((Length=strLen(Str))>MaxLength)
    if ( MaxLength>3 ) {
      char *TmpStr=new char[MaxLength+5];
      Sprintf(TmpStr,"...%s",Str+Length-MaxLength+3);
      StrCpy(Str,TmpStr);
      delete[] TmpStr;
    } else
      Str[MaxLength]=0;
  return(Str);
}

const char *DECLSPEC PointToName(const char *Path)
{  
	const char *NamePtr = Path;

	while( *Path ) 
	{
		if (*Path=='\\' || *Path=='/' || *Path==':')
			NamePtr=Path+1;
		Path++;
	}

	return NamePtr;
}


std::wstring DECLSPEC getName(const std::wstring &path)
{  
	std::wstring::size_type n = path.find_last_of(L"\\/:");
	if(n == std::wstring::npos)
		return path;
	else
		return std::wstring(path.begin()+n+1, path.end());
}

BOOL DECLSPEC CheckForEsc( BOOL isConnection,BOOL IgnoreSilent )
{  
	WORD  ESCCode = VK_ESCAPE;
	BOOL  rc;

	if ( !IgnoreSilent && IS_FLAG(FP_LastOpMode,OPM_FIND) )
		return FALSE;

	rc = CheckForKeyPressed( &ESCCode,1 );
	if ( !rc )
		return FALSE;

	rc = !g_manager.opt.AskAbort ||
		AskYesNo( FMSG( isConnection ? MTerminateConnection : MTerminateOp ) );

	if ( rc ) {
		Log(( "ESC: cancel detected" ));
	}

	return rc;
}

int DECLSPEC IsCaseMixed(const char *Str)
{
  char AnsiStr[1024];
  OemToChar(Str,AnsiStr);
  while (*Str && !IsCharAlpha(*Str))
    Str++;
  int Case=IsCharLower( *Str );
  while (*(Str++))
    if (IsCharAlpha(*Str) && IsCharLower(*Str) != Case)
      return(TRUE);
  return(FALSE);
}

void DECLSPEC LocalLower(char *Str)
{
  OemToChar(Str,Str);
  CharLower(Str);
  CharToOem(Str,Str);
}

BOOL DECLSPEC IsDirExist( CONSTSTR nm )
  {  WIN32_FIND_DATA wfd;
     HANDLE          h;
     int             l;
     BOOL            res;
     char            str[ FAR_MAX_PATHSIZE ];

     StrCpy( str,nm );
     if ( (l=strLen(str)-1) > 0 && str[l] == SLASH_CHAR ) str[l] = 0;

     h = FindFirstFile( str,&wfd );
     if ( h == INVALID_HANDLE_VALUE ) return FALSE;
     res = IS_FLAG(wfd.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY);
     FindClose(h);
 return res;
}

void DECLSPEC FixFTPSlash(std::wstring& s )
{
	std::wstring::iterator i = s.begin();

	while((i = std::find(i, s.end(), L'\\')) != s.end())
		*i = L'/';
}


void DECLSPEC FixFTPSlash( String& s ) { FixFTPSlash( (char*)s.c_str() ); }
void DECLSPEC FixFTPSlash( char *s )
  {
    if (!s) return;
    for ( ; *s; s++ )
      if ( *s == '\\' ) *s = '/';
}

void DECLSPEC FixLocalSlash( String& s ) { FixLocalSlash( (char*)s.c_str() ); }
void DECLSPEC FixLocalSlash( char *s )
  {
    if (!s) return;
    for ( ; *s; s++ )
      if ( *s == '/' ) *s = '\\';
}

void DECLSPEC FixLocalSlash(std::wstring &path)
{
	std::replace(path.begin(), path.end(), L'/', L'\\');
}



std::wstring getPathBranch(const std::wstring &s)
{
	size_t n = s.rfind('\\');
	if(n == std::wstring::npos || n == 0)
		return L"";
	else
		return std::wstring(s, 0, n);
}


std::wstring getPathLast(const std::wstring &s)
{
	size_t n = s.rfind('\\');
	if(n == std::wstring::npos || n == 0)
		return s;
	else
		return std::wstring(s.begin()+n+1, s.end());
}

#ifdef CONFIG_TEST
BOOST_AUTO_TEST_CASE(testPathProcedures)
{
	BOOST_CHECK(getPathBranch(L"")		== L"");
	BOOST_CHECK(getPathBranch(L"\\")	== L"");
	BOOST_CHECK(getPathBranch(L"abc")	== L"");
	BOOST_CHECK(getPathBranch(L"abc\\def")== L"abc");
	BOOST_CHECK(getPathBranch(L"abc\\\\def")== L"abc\\");
	BOOST_CHECK(getPathBranch(L"abc\\def\\")== L"abc\\def");


	BOOST_CHECK(getPathLast(L"")		== L"");
	BOOST_CHECK(getPathLast(L"\\")		== L"\\");
	BOOST_CHECK(getPathLast(L"abc")		== L"abc");
	BOOST_CHECK(getPathLast(L"abc\\def")== L"def");
	BOOST_CHECK(getPathLast(L"abc\\\\def")== L"def");
	BOOST_CHECK(getPathLast(L"abc\\def\\")== L"");
}

#endif