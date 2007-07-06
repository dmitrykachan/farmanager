#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

//------------------------------------------------------------------------
void ShowHostError( FTPUrl* p )
  {
    SayMsg( p->Error.c_str() );
}

//------------------------------------------------------------------------
void FTP::UrlInit(FTPUrl* p)
{
      p->Host     = Host;
      p->Download = FALSE;
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
		p->SrcPath = p->Host.directory_;

		p->Host.directory_ = L"";
	}

	if (p->Host.Host_.empty())
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

BOOL FTP::EditUrlItem( FTPUrl* p )
  {
static FP_DECL_DIALOG( InitItems )
   /*00*/    FDI_CONTROL( DI_DOUBLEBOX, 3, 1,72,12, 0, FMSG(MUrlItem) )

   /*01*/      FDI_LABEL( 5, 2,    FMSG(MCopyFrom) )
   /*02*/   FDI_HISTEDIT( 5, 3,70, "FTPUrl" )
   /*03*/      FDI_LABEL( 5, 4,    FMSG(MCopyTo) )
   /*04*/   FDI_HISTEDIT( 5, 5,70, "FTPUrl" )
   /*05*/      FDI_LABEL( 5, 6,    FMSG(MFileName) )
   /*06*/   FDI_HISTEDIT( 5, 7,70, "FTPFileName" )

   /*07*/      FDI_HLINE( 3, 8 )

   /*08*/      FDI_CHECK( 5, 9,    FMSG(MUDownlioad) )

   /*09*/      FDI_HLINE( 3,10 )
   /*10*/    FDI_GBUTTON( 0,11,    FMSG(MUHost) )
   /*11*/ FDI_GDEFBUTTON( 0,11,    FMSG(MOk) )
   /*12*/    FDI_GBUTTON( 0,11,    FMSG(MCancel) )
   /*13*/    FDI_GBUTTON( 0,11,    FMSG(MUError) )
FP_END_DIALOG

     FarDialogItem  DialogItems[ FP_DIALOG_SIZE(InitItems) ];

//Create items
     FP_InitDialogItems( InitItems,DialogItems );

     PreFill( p );
   do{
//Set flags
		//From
		Utils::safe_strcpy(DialogItems[2].Data, WinAPI::toOEM(p->SrcPath));
		//To
		Utils::safe_strcpy(DialogItems[4].Data, WinAPI::toOEM(p->DestPath));
	    //Name
		Utils::safe_strcpy(DialogItems[6].Data, WinAPI::toOEM(p->fileName_));

     //Flags
     DialogItems[ 8].Selected = p->Download;
     if ( !p->Error.Length() )
       SET_FLAG( DialogItems[13].Flags,DIF_DISABLE );

//Dialog
     do{
       int rc = FDialog( 76,14,"FTPQueueItemEdit",DialogItems,FP_DIALOG_SIZE(InitItems) );
       if ( rc == -1 || rc == 12 ) return FALSE;                               else
       if ( rc == 11 )             break;                                      else
       if ( rc == 10 )             GetHost( MEditFtpTitle, &p->Host, FALSE );  else
       if ( rc == 13 )             ShowHostError( p );
     }while(1);

//Get paras
		//From
		p->SrcPath = WinAPI::fromOEM(DialogItems[2].Data);
		//To
		p->DestPath = WinAPI::fromOEM(DialogItems[4].Data);
		//Name
		p->fileName_ = WinAPI::fromOEM(DialogItems[6].Data);

     //Flags
     p->Download = DialogItems[ 8].Selected;

//Form
     if ( !PreFill(p) )
       continue;

     return TRUE;
   }while(1);
}
