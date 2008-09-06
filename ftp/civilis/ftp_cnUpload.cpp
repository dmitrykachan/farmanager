#include "stdafx.h"
#include "ftp_Int.h"
#include "progress.h"

Connection::Result Connection::sendrequest(const std::wstring &cmd, const std::wstring &local, const std::wstring &remote, bool asciiMode, FTPProgress& trafficInfo)
{  //??SaveConsoleTitle _title;

     Result res = sendrequestINT(cmd, local, remote, asciiMode, trafficInfo);
     IdleMessage(NULL, 0);

	 return res;
}

Connection::Result Connection::sendrequestINT(const std::wstring &cmd, const std::wstring &local, const std::wstring &remote, bool asciiMode, FTPProgress& trafficInfo)
{
	PROCP(L"cmd: " << cmd << L"local: " << local << L"remote: " << remote);

	if(asciiMode)
		setascii();
	else
		setbinary();

	if(type_ == TYPE_A)
	{
		BOOST_ASSERT(restart_point == 0);
	}
	if(restart_point != 0)
	{
		BOOST_ASSERT(resumeSupport_);
		BOOST_ASSERT(cmd != g_manager.opt.cmdAppe);
	}

	WIN32_FIND_DATAW	ffi;
	bool				oldBrk = getBreakable();
	__int64				fsz;

	HANDLE ff = FindFirstFileW(local.c_str(), &ffi);
	if(ff == INVALID_HANDLE_VALUE)
	{
		AddCmdLine(L"File not found");
		return Error;
	}
	FindClose(ff);

	if(is_flag(ffi.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		BOOST_LOG(INF, "local is directory: " << local);
		AddCmdLine(getMsg(MNotPlainFile));
		return Error;
	}

	boost::shared_ptr<void> fin(Fopen(local.c_str(), L"r"), Fclose);
	if(fin.get() == 0)
	{
		BOOST_LOG(INF, L"!open local");
		AddCmdLine(getMsg(MErrorOpenFile));
		return Error;
	}
	fsz = Fsize(fin.get());

	if(restart_point && fsz == restart_point)
	{
		AddCmdLine(getMsg(MFileComplete));
		return Error;
	}

	if(!initDataConnection())
	{
		BOOST_LOG(INF, L"!initconn");
		return Error;
	}

	if(getHost()->SendAllo)
	{
		if(cmd != g_manager.opt.cmdAppe)
		{
			BOOST_LOG(INF, L"ALLO " << fsz);
			if(command(g_manager.opt.cmdAllo + boost::lexical_cast<std::wstring>(fsz)) == Done)
			{
				BOOST_LOG(INF, "!allo");
				AddCmdLine(L"Can not send the ALLO command");
				return Error;
			}
		}
	}

	if(getHost()->AsciiMode)
		setascii();
	else
		setbinary();

	if(restart_point)
	{
		BOOST_LOG(INF, L"restart_point " << restart_point);

		if(!Fmove(fin.get(), restart_point))
		{
			BOOST_LOG(INF, L"!setfilepointer: " << restart_point);
			AddCmdLine(getMsg(MErrorPosition));
			return Error;
		}

		if(command(g_manager.opt.cmdRest_ + L" " + boost::lexical_cast<std::wstring>(restart_point)) != Done)
			return Error;

		trafficInfo.resume(restart_point);
	}

	if(getHost()->PassiveMode)
	{
		if(!ftpclient_.establishPassiveDataConnection())
			return Error;
	}
	std::wstring s = cmd;
	if(!remote.empty())
		s += L' ' + remote;
	error_code code = ftpclient_.sendCommand(s, false);
	if(FTPClient::isComplete(code) == false && FTPClient::isPrelim(code) == false)
		return Error;

	if(!getHost()->PassiveMode)
	{
		if(!ftpclient_.establishActiveDataConnection())
		{
			BOOST_LOG(ERR, "cannot establish the data connection");
			return Error;
		}
	}

	BOOST_LOG(INF, L"Uploading " << local << L"->" << remote << L" from " << restart_point);
	FTPConnectionBreakable _brk( this, false);

	error_code uploadResult;
	uploadResult = ftpclient_.uploadFile(boost::bind(&Connection::uploadFromFile, this, fin.get(), boost::ref(trafficInfo), _1, _2));

	while(FTPClient::isPrelim(code))
		code = ftpclient_.readOutput();

	setBreakable(oldBrk);
	if(FTPClient::isComplete(uploadResult))
		return Done;
	if(FTPClient::isCanceled(uploadResult))
		return Cancel;
	return Error;
}

error_code Connection::uploadFromFile(HANDLE file, FTPProgress& trafficInfo, std::vector<char> &buffer, size_t* readbytes)
{
	error_code code;
	BOOST_ASSERT(readbytes);
	DWORD res;
	if(ReadFile(file, &buffer[0], buffer.size(), &res, NULL) != FALSE)
		code = error_code(GetLastError(), asio::error::get_system_category());
	trafficInfo.refresh(this, res);
	*readbytes = res;
	return code;
}


