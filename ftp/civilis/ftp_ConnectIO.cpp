#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

Connection::Result Connection::reset()
{
	BOOST_ASSERT(0 && "Not implemented");
	return Done;
}

void Connection::AbortAllRequest(int BrkFlag)
{
	if (BrkFlag)
		brk_flag = TRUE;
	else 
	{
		ftpclient_.close();
	}
}


Connection::Result Connection::commandOem(const std::string &str, bool isQuit)
{  
	ConnectMessage(MLast);

	AddCmdLineOem(str, ldOut);
	error_code code = ftpclient_.sendCommandOem(str);
	getKeepStopWatch().reset();

	if(FTPClient::isCanceled(code))
		return Cancel;
	if(FTPClient::isComplete(code))
		return Done;
	return Error;
	/* TODO надо куда нить засунуть обработку если конекшен упал
	{
		BOOST_LOG(ERR, L"!command(" << FromOEM(str) << L"): " << e.whatw());
		AddCmdLine(e.whatw(), ldRaw);
		if(ConnectMessageTimeout(MConnectionLost, getHost()->url_.Host_, true))
			getPlugin()->FullConnect(getHost());
		else
		{
			getPlugin()->BackToHosts();
			getPlugin()->Invalidate();
			lostpeer();
			return RPL_ERROR;
		}
	}
	*/
}

Connection::Result Connection::command(const std::wstring &str, bool isQuit)
{
	return commandOem(toOEM(str), isQuit);
}

void Connection::setHost(const boost::shared_ptr<FTPHost> &host)
{
	pHost_ = host;
}

boost::shared_ptr<FTPHost>& Connection::getHost()
{
	BOOST_ASSERT(pHost_);
	return pHost_;
}

const boost::shared_ptr<FTPHost>& Connection::getHost() const
{
	BOOST_ASSERT(pHost_);
	return pHost_;
}

std::wstring Connection::getSystemInfo()
{  
	if(systemInfo_.empty())
	{
		FARWrappers::Screen scr;
		if(syst() == Done)
		{
			std::string tmp = ftpclient_.getReply();
			size_t n;
			n = tmp.find_first_of("\r\n");
			if(n != std::string::npos)
				tmp.resize(n);

			if(tmp.size() > 3 && isdigit(tmp[0]) && isdigit(tmp[1]) && isdigit(tmp[2]))
				systemInfo_ = FromOEM(std::string(tmp.begin()+4, tmp.end()));
			else
				systemInfo_ = FromOEM(tmp);
		} else
		{
			systemInfo_ = L"System info is undefined";
		}
	}
	return systemInfo_;
}


__int64  Connection::fileSize(const std::wstring &filename)
{
	if(sizecmd(filename) != Done)
	{
		throw std::exception("file size command is broken");
		return -1;
	} else
	{
		const std::string& line = ftpclient_.getReply();
		return Parser::parseUnsignedNumber(line.begin()+4, line.end(), 
			std::numeric_limits<__int64>::max(), false); 
	}
}

