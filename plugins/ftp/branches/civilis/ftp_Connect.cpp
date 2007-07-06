#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

cmd cmdtabdata[] = {
  /*00*/{ "account",    1,  1, 0 },  //  [...]
  /*01*/{ "append",     1,  1, 2 },  //  <local> [<remote>]
  /*02*/{ "ascii",      1,  1, 0 },  //
  /*03*/{ "binary",     1,  1, 0 },  //
  /*04*/{ "bye",        0,  0, 0 },  //
  /*05*/{ "cd",         1,  1, 1 },  //  <path>
  /*06*/{ "cdup",       1,  1, 0 },  //
  /*07*/{ "chmod",      1,  1, 2 },  //  <file> <mode>
  /*08*/{ "close",      1,  1, 0 },  //
  /*09*/{ "delete",     1,  1, 1 },  //  <file>
  /*10*/{ "dir",        1,  1, 0 },  //  [<path>]
  /*11*/{ "disconnect", 1,  1, 0 },  //
  /*12*/{ "get",        1,  1, 1 },  //  <remote> [<local>]
  /*13*/{ "idle",       1,  1, 1 },  //  <time>
  /*14*/{ "image",      1,  1, 0 },  //
  /*15*/{ "ls",         1,  1, 0 },  //  [<path>]
  /*16*/{ "mkdir",      1,  1, 1 },  //  <dir>
  /*17*/{ "modtime",    1,  1, 1 },  //  <file> (get last modification time)
  /*18*/{ "newer",      1,  1, 1 },  //  <remote> [<local>] (get files only if new)
  /*19*/{ "nlist",      1,  1, 0 },  //  [<path>]
  /*20*/{ "open",       0,  1, 1 },  //  <site> [<port> [<user> [<pwd>]]]
  /*21*/{ "proxy",      0,  1, 1 },  //  <command> [<cmd params>] (exec cmd into proxy mode; tempror switch to proxy if not yet)
  /*22*/{ "sendport",   0,  0, 0 },  //
  /*23*/{ "put",        1,  1, 1 },  //  <local> [<remote>]
  /*24*/{ "pwd",        1,  1, 0 },  //
  /*25*/{ "quit",       0,  0, 0 },  //
  /*26*/{ "quote",      1,  1, 1 },  //  <command> [...]
  /*27*/{ "recv",       1,  1, 1 },  //  <remote> [<local>]
  /*28*/{ "reget",      1,  1, 1 },  //  <remote> [<local>]
  /*29*/{ "rstatus",    1,  1, 0 },  //  [<command>]
  /*30*/{ "rhelp",      1,  1, 0 },  //  [<command>]
  /*31*/{ "rename",     1,  1, 2 },  //  <old name> <new name>
  /*32*/{ "reset",      1,  1, 0 },  //  (read one server reply string)
  /*33*/{ "restart",    1,  1, 1 },  //  <restart point> (set internal restart offset)
  /*34*/{ "rmdir",      1,  1, 1 },  //  <dirname>
  /*35*/{ "runique",    0,  1, 0 },  //  (set internal `runique` flag)
  /*36*/{ "send",       1,  1, 1 },  //  <local> [<remote>]
  /*37*/{ "site",       1,  1, 1 },  //  <command> [...]
  /*38*/{ "size",       1,  1, 1 },  //  <file>
  /*39*/{ "system",     1,  1, 0 },  //  (exec SYST command)
  /*40*/{ "sunique",    0,  1, 0 },  //  (set internal `sunique` flag)
  /*41*/{ "user",       1,  1, 1 },  //  <user> [<pwd>] [<account command>]
  /*42*/{ "umask",      1,  1, 1 },  //  <umask>
  { 0 },
};

Connection::Connection()
  {
    SocketError      = (int)INVALID_SOCKET;
    sendport         = -1;
    cmd_peer         = INVALID_SOCKET;
    data_peer        = INVALID_SOCKET;
    LastUsedTableNum = 1;
    TrafficInfo      = new FTPProgress;
    CurrentState     = fcsNormal;
    Breakable        = TRUE;
}

Connection::~Connection()
{  PROC(( "Connection::~Connection","%p",this ))

  int LastError = GetLastError();

  ResetOutput();
  CacheReset();
  AbortAllRequest(0);
  SetLastError(LastError);
  CloseIOBuff();

  delete TrafficInfo;
  CloseCmdBuff();
}

void Connection::ExecCmdTab(struct cmd *c,int argc,char *argv[])
{  
	PROC(( "ExecCmdTab","%d [%s,%s,%s]",argc,(argc>=1)?argv[0]:"nil",(argc>=2)?argv[1]:"nil",(argc>=3)?argv[2]:"nil" ));

     int I;
     for ( I=0; I < ARRAY_SIZE(cmdtabdata); I++ )
       if (c==&cmdtabdata[I]) {
         switch(I) 
		 {
           case  0: account(argc,argv);  break;

           case  1: // TODO if (argc>2) argv[2] = FromOEMDup( argv[2] );
                    put(argc,argv);
                 break;

           case  2: setascii();          break;

           case  3: setbinary();         break;

           case  4: quit();              break;

           case  5: // TODO argv[1] = FromOEMDup( argv[1] );
                    cd(argc,argv);
                 break;

           case  6: cdup();              break;

           case  7: do_chmod(argc,argv); break;

           case  8: disconnect();        break;

           case  9: // TODO argv[1] = FromOEMDup( argv[1] );
                    deleteFile(argc,argv);
                 break;

           case 10: // TODO if ( argc > 1 )
                    //  argv[1] = FromOEMDup( argv[1] );
                    ls(argc,argv);
                  break;

           case 11: disconnect();
                 break;

           case 12: // argv[1] = FromOEMDup( argv[1] );
                    get(argc,argv);
                  break;

           case 13: idle(argc,argv); break;

           case 14: setbinary();     break;

           case 15: // TODO if (argc>1) argv[1] = FromOEMDup( argv[1] );
                    ls(argc,argv);
                 break;

           case 16: // TODO argv[1] = FromOEMDup( argv[1] );
                    makedir(argc,argv);
                 break;

           case 17: modtime(argc,argv); break;

           case 18: newer(argc,argv); break;

           case 19: // TODO if (argc>1) argv[1] = FromOEMDup( argv[1] );
                    ls(argc,argv);
                 break;

           case 20: setpeer(argc,argv); break;

           case 21: doproxy(argc,argv); break;

           case 22: setport(); break;

           case 23: // TODO if (argc>2) argv[2] = FromOEMDup( argv[2] );
                    put(argc,argv);
                  break;

           case 24: pwd(); break;

           case 25: quit(); break;

           case 26: quote(argc,argv); break;

           case 27: // TODO argv[1] = FromOEMDup( argv[1] );
                    get(argc,argv);
                 break;

           case 28: // TODO argv[1] = FromOEMDup( argv[1] );
                    reget(argc,argv);
                 break;

           case 29: rmtstatus(argc,argv); break;

           case 30: rmthelp(argc,argv); break;

           case 31: // TODO if (argc>1) argv[1] = FromOEMDup( argv[1] );
                    // TODO if (argc>2) argv[2] = FromOEMDup( argv[2],1 );
                    renamefile(argc,argv);
                 break;

           case 32: reset(); break;

           case 33: restart(argc,argv); break;

           case 34: // TODO argv[1] = FromOEMDup( argv[1] );
                    removedir(argc,argv);
                 break;

           case 35: setrunique(); break;

           case 36: // TODO if (argc>2) argv[2] = FromOEMDup( argv[2] );
                    put(argc,argv);
                 break;

           case 37: site(argc,argv); break;

           case 38: // TODO argv[1] = FromOEMDup( argv[1] );
                    sizecmd(argc,argv);
                 break;

           case 39: syst(); break;

           case 40: setsunique(); break;

           case 41: user(argc,argv); break;

           case 42: do_umask(argc,argv); break;
         }
         break;
       }
}

void Connection::GetState( ConnectionState* p )
  {
     p->Inited     = TRUE;
     p->Blocked    = CmdVisible;
     p->RetryCount = RetryCount;
     p->TableNum   = codePage_;
     p->Passive    = Host.PassiveMode;
     p->Object     = TrafficInfo->Object;

     TrafficInfo->Object = NULL;
}

void Connection::SetState( ConnectionState* p )
  {
    if ( !p->Inited )
      return;

    CmdVisible           = p->Blocked;
    RetryCount           = p->RetryCount;
    codePage_             = p->TableNum;
    Host.PassiveMode     = p->Passive;
    TrafficInfo->Object  = p->Object;
    TrafficInfo->SetConnection( this );
}

void Connection::InitData( FTPHost* p,int blocked /*=TRUE,FALSE,-1*/ )
  {
    Host       = *p;
    Host.Size  = sizeof(Host);
    CmdVisible = TRUE;

    if ( blocked != -1 )
      CmdVisible = blocked == FALSE;
}

void Connection::AddOutput(BYTE *Data,int Size)
{
	if(Size==0) return;
	output_ += Unicode::utf8ToUtf16(std::string(Data, Data+Size));
}

void Connection::ResetOutput()
{
	output_ = L"";
}


void Connection::CacheReset()
{
  for (int I=0;I<sizeof(ListCache)/sizeof(ListCache[0]);I++) {
    if (ListCache[I].Listing)
      _Del( ListCache[I].Listing );
    ListCache[I].Listing=NULL;
    ListCache[I].ListingSize=0;
  }
  ListCachePos=0;
}


int Connection::CacheGet()
{
	if(DirFile[0]) 
	{
		HANDLE f = Fopen(DirFile, "r");
		if(f) 
		{
			ResetOutput();
			int size = (DWORD)Fsize( f );
			std::vector<char> v(size);
			Fread(f, &output_[0], size);
			Fclose(f);

			return TRUE;
		}
	}

	for(size_t I=0; I < ListCache.size(); I++)
		if(ListCache[I].ListingSize > 0 &&
			curdir_ == Unicode::utf8ToUtf16(ListCache[I].DirName) )
		{
			ResetOutput();
			output_ = Unicode::utf8ToUtf16(ListCache[I].Listing);
			return TRUE;
		}

	return FALSE;
}


void Connection::CacheAdd()
{
  if ( ListCache[ListCachePos].Listing )
    _Del( ListCache[ListCachePos].Listing );

  ListCache[ListCachePos].ListingSize = 0;
  ListCache[ListCachePos].Listing     = (char*)_Alloc(output_.size()+1);

  if ( ListCache[ListCachePos].Listing ==NULL )
    return;

  ListCache[ListCachePos].ListingSize = output_.size();
  MemCpy( ListCache[ListCachePos].Listing, Unicode::utf16ToUtf8(output_).c_str(), output_.size()); 
  ListCache[ListCachePos].Listing[output_.size()] = 0;
  StrCpy( ListCache[ListCachePos].DirName, Unicode::utf16ToUtf8(curdir_).c_str(), sizeof(ListCache[ListCachePos].DirName) );

  ListCachePos++;
  if ( ListCachePos >= sizeof(ListCache)/sizeof(ListCache[0]) )
    ListCachePos=0;
}

/* Returns if error happen
*/
BOOL Connection::GetExitCode()
{
	static struct 
	{
		int Code;
		int WCode;
	} FtpErrCodes[] = 
	{
		{ 202, ERROR_CALL_NOT_IMPLEMENTED },
		{ 421, ERROR_INTERNET_CONNECTION_ABORTED },
		{ 450, ERROR_IO_INCOMPLETE },
		{ 451, 0 },
		{ 452, ERROR_DISK_FULL },
		{ 500, ERROR_BAD_COMMAND },
		{ 501, ERROR_BAD_COMMAND },
		{ 502, ERROR_CALL_NOT_IMPLEMENTED },
		{ 503, ERROR_BAD_COMMAND },
		{ 504, ERROR_CALL_NOT_IMPLEMENTED },
		{ 530, ERROR_INTERNET_LOGIN_FAILURE },
		{ 532, ERROR_ACCESS_DENIED },
		{ 550, ERROR_ACCESS_DENIED },
		{ 551, 0 },
		{ 552, ERROR_DISK_FULL },
		{ 553, 0 }
	};

	if (ErrorCode != ERROR_SUCCESS)
	{
		SetLastError(ErrorCode);
		return FALSE;
	}

	if (code == RPL_ERROR || code == RPL_TRANSFERERROR )
	{
		//SetLastError(ErrorCode);
		return FALSE;
	}

	for (int I=0; I < sizeof(FtpErrCodes)/sizeof(FtpErrCodes[0]); I++)
		if(FtpErrCodes[I].Code == code)
		{
			if ( FtpErrCodes[I].WCode )
				SetLastError( FtpErrCodes[I].WCode );
			return FALSE;
		}

	return TRUE;
}

int encode_UTF8(WCHAR *ws, int wsz, char *utf8s, int sz)
{
    int i = 0, iw = 0;
    char *s;

    s = utf8s;
    while(*ws && i<sz && iw<wsz) {
        if(*ws <= 0x007F) {                 /* 1 octet */
            *s++ = (char)*ws;
            i++;
        }
        else if(*ws <= 0x07FF) {            /* 2 octets */
            *s++ = 0xC0 | ((*ws >> 6) & 0x1F);
            *s++ = 0x80 | (*ws & 0x3F);
            i+=2;
        }
        else if(*ws <= 0xFFFF) {            /* 3 octets */
            *s++ = 0xE0 | ((*ws >> 12) & 0x0F);
            *s++ = 0x80 | ((*ws >> 6) & 0x3F);
            *s++ = 0x80 | (*ws & 0x3F);
            i+=3;
        }
        else {                              /* >= 4 octets -- not fit in UCS-2 */
            *s++ = '_';
            i++;
        }
        ++ws;
        ++iw;
    }
    iw = i;
    while (iw < sz)
    {
      *s++ = 0;
      iw++;
    }
    return i;
}

std::wstring Connection::FromOEM(const std::string &str)
{
	return WinAPI::toWide(str, codePage_);

}

std::string Connection::toOEM(const std::wstring &str)
{  
	return WinAPI::fromWide(str, codePage_);
}
