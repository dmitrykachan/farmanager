#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"
#include "progress.h"

Connection::Result Connection::recvrequest(const std::wstring& cmd, const std::wstring &local, const std::wstring &remote, wchar_t *mode, FTPProgress& trafficInfo)
{  
	//??SaveConsoleTitle _title;

	Result res = recvrequestINT(cmd, local, remote, mode, trafficInfo);
	IdleMessage(NULL, 0);
	return res;
}

Connection::Result Connection::recvrequestINT(const std::wstring& cmd, const std::wstring &local, const std::wstring &remote, wchar_t *mode, FTPProgress& trafficInfo)
{
	boost::shared_ptr<void> fout;
	BOOL             oldBrk = getBreakable();

	BOOST_ASSERT(!remote.empty());

	if(!isResumeSupport())
	{
		BOOST_ASSERT(restart_point == 0);
	}

	if(local != g_MemoryFile)
	{
		fout = boost::shared_ptr<void>(Fopen(local.c_str(), mode, g_manager.opt.SetHiddenOnAbort ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL), Fclose);
		BOOST_LOG(INF, L"recv file [" << getHost()->IOBuffSize << L"] \"" << local << L"\"=" << fout.get());
		if(!fout.get())
		{
			BOOST_LOG(ERR, L"!Fopen [" << mode << L"] " << WinAPI::getStringError());
			if (!ConnectMessage( MErrorOpenFile, local, true, MRetry))
				return Cancel;
			return Error;
		}

		if(restart_point)
		{
			if(!Fmove(fout.get(), restart_point))
			{
				if(!ConnectMessage(MErrorPosition, local, true, MRetry))
					return Cancel;
				return Error;
			}
		}
		trafficInfo.resume(restart_point);
	}

	if(!initDataConnection())
	{
		BOOST_LOG(INF, L"!initconn");
		return Error;
	}

	if(cmd == g_manager.opt.cmdRetr)
	{
		if(getHost()->AsciiMode)
			setascii();
		else
			setbinary();
		if(restart_point)
		{
			Result result = command(g_manager.opt.cmdRest_ + L" " + boost::lexical_cast<std::wstring>(restart_point));
			if(result != Done)
			{
				AddCmdLine(L"REST command failed", ldRaw);
				return Error;
			}
		}
	} else
		setascii();

	if(getHost()->PassiveMode)
	{
		if(!ftpclient_.establishPassiveDataConnection())
			return Error;
	}

	error_code code = ftpclient_.sendCommand(cmd + L" " + remote, false);
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

	WinAPI::Stopwatch stopwatch(500);
	setBreakable(false);

	WinAPI::Stopwatch timer;
	BOOST_LOG(ERR, L"start downloading");

	error_code downloadResult;
	if(fout.get())
		downloadResult = ftpclient_.downloadFile(boost::bind(&Connection::downloadToFile, this, fout.get(), boost::ref(trafficInfo), _1, _2));
	else
		downloadResult = ftpclient_.downloadFile(boost::bind(&Connection::downloadToBufferHandler, this, boost::ref(trafficInfo), _1, _2));

	while(FTPClient::isPrelim(code))
		code = ftpclient_.readOutput();

	// TODO обрабока если ошибка возникла...
	BOOST_LOG(ERR, L"end downloading. Downloading time is " << timer.getPeriod() << L". Data size is " << output_.size() << L". Reserved size is " << output_.capacity());

	trafficInfo.refresh(this, 0);

	setBreakable(oldBrk);

	if(FTPClient::isComplete(downloadResult))
		return Done;
	if(FTPClient::isCanceled(downloadResult))
		return Cancel;
	return Error;
}


error_code Connection::downloadToBufferHandler(FTPProgress& trafficInfo, const std::vector<char> &buffer, size_t bytes_transferred)
{
	AddOutput(&buffer[0], bytes_transferred);
	trafficInfo.refresh(this, bytes_transferred);
	return error_code();
}

error_code Connection::downloadToFile(HANDLE file, FTPProgress& trafficInfo, const std::vector<char> &buffer, size_t bytes_transferred)
{
	error_code code;
	DWORD res;
	if(WriteFile(file, &buffer[0], bytes_transferred, &res, NULL) != FALSE)
		code = error_code(GetLastError(), asio::error::get_system_category());
	trafficInfo.refresh(this, res);
	return code;
}
