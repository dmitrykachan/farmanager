#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"
#include "utils/uniconverts.h"

void CommandsList::init()
{
	add(L"account", &Connection::account);
	add(L"append",	&Connection::appended);
	add(L"ascii",	&Connection::setascii);
	add(L"binary",	&Connection::setbinary);
	add(L"bye",		&Connection::quit, false, false);
	add(L"cd",		&Connection::cd);
	add(L"cdup",	&Connection::cdup);
	add(L"chmod",	&Connection::chmod);
	add(L"close",	&Connection::disconnect);
	add(L"delete",	&Connection::deleteFile);
	add(L"dir",		&Connection::ls);
	add(L"disconnect", &Connection::disconnect);
	add(L"get",		&Connection::get);
	add(L"idle",	&Connection::idle);
	add(L"image",	&Connection::setbinary);
	add(L"ls",		&Connection::ls);
	add(L"mkdir",	&Connection::makedir);
	add(L"modtime",	&Connection::modtime);
	add(L"newer",	&Connection::newer);
	add(L"nlist",	&Connection::nlist);
 	add(L"open",	&Connection::setpeer); // 20
	add(L"sendport",&Connection::setport, false, false);
	add(L"put",		&Connection::put);
	add(L"pwd",		&Connection::pwd);
	add(L"quit",	&Connection::quit, false, false);
	add(L"quote",	&Connection::quote);
	add(L"recv",	&Connection::get); // 27
	add(L"reget",	&Connection::reget); // 28
	add(L"rstatus",	&Connection::rmtstatus);
	add(L"rhelp",	&Connection::rmthelp);
	add(L"rename",	&Connection::renamefile);
	add(L"reset",	&Connection::reset);
	add(L"restart",	&Connection::restart);
	add(L"rmdir",	&Connection::removedir);
	add(L"runique",	&Connection::setrunique);
	add(L"send",	&Connection::put);
	add(L"site",	&Connection::site);
	add(L"size",	&Connection::sizecmd);
	add(L"system",	&Connection::syst);
	add(L"sunique",	&Connection::setsunique);
	add(L"user",	&Connection::user);
	add(L"umask",	&Connection::do_umask);
}


Connection::Connection()
{
    sendport_		= true;
    LastUsedTableNum = 1;
    TrafficInfo		= new FTPProgress;
    CurrentState	= fcsNormal;
    breakable_		= true;
	connected_		= false;
}

Connection::~Connection()
{
	PROCP(L"this " << this);

	int LastError = GetLastError();

	ResetOutput();
	AbortAllRequest(0);
	SetLastError(LastError);

	delete TrafficInfo;
	CloseCmdBuff();
}

void Connection::GetState( ConnectionState* p )
{
     p->Inited     = TRUE;
     p->Blocked    = CmdVisible;
     p->RetryCount = RetryCount;
     p->Passive    = getHost().PassiveMode;
     p->Object     = TrafficInfo->Object;

     TrafficInfo->Object = NULL;
}

void Connection::SetState( ConnectionState* p )
{
    if ( !p->Inited )
      return;

    CmdVisible				= p->Blocked;
    RetryCount				= p->RetryCount;
    getHost().PassiveMode	= p->Passive;
    TrafficInfo->Object		= p->Object;
    TrafficInfo->SetConnection( this );
}

void Connection::InitData( FTPHost* p,int blocked /*=TRUE,FALSE,-1*/ )
{
	setHost(p);
    CmdVisible = true;

    if(blocked != -1)
      CmdVisible = blocked == false;
}

void Connection::AddOutput(BYTE *Data,int Size)
{
	if(Size == 0)
		return;
	output_ += Unicode::utf8ToUtf16(std::string(Data, Data+Size));
}

void Connection::ResetOutput()
{
	output_ = L"";
}

std::wstring Connection::FromOEM(const std::string &str) const
{
	return WinAPI::toWide(str, getHost().codePage_);

}

std::string Connection::toOEM(const std::wstring &str) const
{  
	return WinAPI::fromWide(str, getHost().codePage_);
}
