#include "stdafx.h"

#include "ftp_Int.h"
#include "farwrapper/dialog.h"

void ShowHostError( FTPUrl* p )
{
    FARWrappers::message(p->Error);
}

void FTP::UrlInit(FTPUrl* p)
{
//TODO HOST ASSIGN      p->Host     = chost_;
      p->Download = false;
      p->Next     = NULL;
}

FTPUrl* FTP::UrlItem( int num, FTPUrl* *prev )
  {  FTPUrl* p,*p1;

     if (prev) *prev = NULL;
     for( p1 = NULL,p = UrlsList;
          p && num > 0;
          p1 = p, p = p->Next )
          num--;

     if ( num == 0 ) {
       if (prev) *prev = p1;
       return p;
     }

 return NULL;
}

void FTP::DeleteUrlItem( FTPUrl* p, FTPUrl* prev )
  {
    if ( !p ) return;

    if (prev) prev->Next = p->Next;
    if (UrlsList == p) UrlsList = p->Next;
    if (UrlsTail == p) UrlsTail = prev;
    delete p;
    QuequeSize--;
}

//------------------------------------------------------------------------
BOOL PreFill(FTPUrl* p)
{  
	wchar_t  ch;

	boost::algorithm::to_lower(p->SrcPath);
	if((p->SrcPath == L"http://") ||
		p->SrcPath == L"ftp://")
		p->Download = true;

	if (p->Download && p->SrcPath[0] != L'/' )
	{
		p->Host.SetHostName(p->SrcPath.c_str(), NULL, NULL);
		p->SrcPath = p->Host.url_.directory_;

		p->Host.url_.directory_ = L"";
	}

	if (p->Host.url_.Host_.empty())
		return FALSE;

	if (p->Download)
	{
		ch = '/';
	} else 
	{
		FixLocalSlash(p->SrcPath);
		ch = '\\';
	}

	if(p->fileName_.find_first_of(L"\\/") != std::wstring::npos)
	{
		AddEndSlash(p->SrcPath, ch);
		p->SrcPath += p->fileName_;
		p->fileName_ = L"";

		if (!p->Download)
			FixLocalSlash(p->SrcPath);
	}

	if(p->fileName_.empty())
	{
		size_t num = p->SrcPath.rfind(ch);
		if(num == std::wstring::npos)
			return false;

		p->fileName_.assign(p->SrcPath.begin()+num+1, p->SrcPath.end());
		p->SrcPath.resize(num);
	}
	return true;
}

bool FTP::EditUrlItem( FTPUrl* p )
{
	FARWrappers::Dialog dlg(L"FTPQueueItemEdit");

	enum
	{
		idHost, idOk, idError
	};

	dlg.addDoublebox( 3, 1,72,12,				getMsg(MUrlItem))->
		addLabel	( 5, 2,						getMsg(MCopyFrom))->
		addEditor	( 5, 3,70,	&p->SrcPath,  0, L"FTPUrl")->
		addLabel	( 5, 4,						getMsg(MCopyTo))->
		addEditor	( 5, 5,70,	&p->DestPath, 0, L"FTPUrl")->
		addLabel	( 5, 6,						getMsg(MFileName))->
		addEditor	( 5, 7,70,	&p->fileName_, 0, L"FTPFileName")->
		addHLine	( 3, 8)->
		addCheckbox	( 5, 9,		&p->Download,	getMsg(MUDownlioad))->
		addHLine	( 3,10)->
		addButton	( 0,11,		idHost,			getMsg(MUHost))->
		addDefaultButton( 0,11,	idOk,			getMsg(MOk))->
		addButton	( 0,11,		-1,				getMsg(MCancel))->
		addButton	( 0,11,		idError,		getMsg(MUError));

	PreFill(p);
	do
	{
		if(p->Error.empty())
			dlg.find(idError).disable();
		int rc;
		do
		{
			rc = dlg.show(76, 14);

			switch(rc)
			{
			case -1:
				return false;
			case idHost:
				EditHostDlg(MEditFtpTitle, FtpHostPtr(&p->Host), false);
				break;
			case idError:
				ShowHostError(p);
			    break;
			}
		} while(rc != idOk);

		//Form
		if(!PreFill(p))
			continue;

		return true;
	} while(1);
}
