#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"
#include <fcntl.h>


Connection::Result Connection::Init(FTP* ftp)
{  
	FARWrappers::Screen scr;
	std::wstring Host		= getHost()->url_.Host_;
	std::wstring& User		= getHost()->url_.username_;
	std::wstring& Password	= getHost()->url_.password_;
	size_t Port				= getHost()->url_.port_;

	if(!GetLoginData(User, Password, getHost()->AskLogin))
		return Cancel;

	_set_fmode(_O_BINARY); // This causes an error somewhere.
	SetLastError( ERROR_SUCCESS );

	setPlugin(ftp);

	/* Set up defaults for FTP. */
	type_ = TYPE_NONE;
	brk_flag	= false;
	restart_point = 0;
	IOCallback	= false;

	keepAliveStopWatch_.setTimeout(g_manager.opt.KeepAlive*1000);

	return setpeer_(Host, Port, User, Password, g_manager.opt.AskLoginFail);
}


error_code Connection::login(const std::wstring& user, const std::wstring& password, const std::wstring& account)
{
	ConnectMessage(MSendingName);
	userName_		= user.empty()? L"anonymous" : user;
	userPassword_	= password.empty()? g_manager.opt.defaultPassword_ : password;

	return ftpclient_.login(userName_, userPassword_, account);
}

void Connection::CheckResume()
{  
	Result res = getHost()->AsciiMode? setascii() : setbinary();
	if(res != Done)
		return;
	if(command(g_manager.opt.cmdRest_ + L" 0") == RequestedFileActionPendingFurtherInformation)
	{
		resumeSupport_ = true;
	}
}

/*
 * Need to start a listen on the data channel
 * before we send the command, otherwise the
 * server's connect may fail.
 */
bool Connection::initDataConnection()
{
	if(brk_flag)
		return false;

	ftpclient_.prepareDataConnection(getHost()->PassiveMode);
	return true;
}
