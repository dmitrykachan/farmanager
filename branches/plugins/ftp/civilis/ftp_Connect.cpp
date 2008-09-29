#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"
#include "utils/uniconverts.h"
#include "progress.h"


void CommandsList::init()
{
	add(L"account", &Connection::account);
 	add(L"append",	&Connection::appended);
 	add(L"ascii",	&Connection::setascii);
 	add(L"binary",	&Connection::setbinary);
	add(L"bye",		&Connection::disconnect, false, false);
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
	add(L"open",	&Connection::setpeer);
	add(L"sendport",&Connection::setport, false, false);
	add(L"put",		&Connection::put);
	add(L"pwd",		&Connection::pwd);
	add(L"quit",	&Connection::disconnect, false, false);
	add(L"quote",	&Connection::quote);
	add(L"recv",	&Connection::get);
	add(L"reget",	&Connection::reget);
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

#pragma warning(disable:4355)
Connection::Connection()
:	ftpclient_(this)
{
	sunique			= false;
	runique			= false;
	sendport_		= true;
    breakable_		= true;
	retryCount_		= 0;
}
#pragma warning(default:4355)


Connection::~Connection()
{
	AbortAllRequest(0);
}

void Connection::InitData(const boost::shared_ptr<FTPHost> &p)
{
	setHost(p);
    CmdVisible = true;
}

void appendFromOem(std::wstring &output, const char* data, int size, int codePage)
{

}

void Connection::AddOutput(const char *data, int size)
{
	if(size == 0)
		return;

	boost::scoped_array<wchar_t> result(new wchar_t[size]);
	int res = ::MultiByteToWideChar(getHost()->codePage_, 0, data, size, result.get(), size);
	BOOST_ASSERT(res > 0);
	if(res == 0)
	{
		throw std::exception((std::string("MultiByteToWideChar fails: ") + boost::lexical_cast<std::string>(res)).c_str());
	}
	output_.append(result.get(), size);
}

void Connection::ResetOutput()
{
	output_.clear();
}

std::wstring Connection::FromOEM(const std::string &str) const
{
	return WinAPI::toWide(str, getHost()->codePage_);
}

std::string Connection::toOEM(const std::wstring &str) const
{  
	return WinAPI::fromWide(str, getHost()->codePage_);
}

bool Connection::displayWaitingMessage(size_t n)
{
	if(g_manager.opt.ShowIdle)
	{
		if(n*FTPClient::timerInterval_ >= 500)
		{
			std::wstring message = currentProcessMessage_;
			message.resize(message.size()+n, '.');
			IdleMessage(message.c_str(), g_manager.opt.ProcessColor);
		}
	}
	return !CheckForEsc(true);	
}