#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

int Connection::SetType(int type)
{
	switch ( type )
	{
		case TYPE_A: return setascii();
		case TYPE_I: return setbinary();
		case TYPE_E: return setebcdic();
	}

	return RPL_ERROR;
}

bool Connection::reset()
{
	BOOST_ASSERT(0 && "Not implemented");
	return true;
}

void Connection::AbortAllRequest(int BrkFlag)
{
	if (BrkFlag)
		brk_flag = TRUE;
	else 
	{
		data_peer_.close();
		cmd_socket_.close();
	}
}

int Connection::command(const std::string &str, bool isQuit)
{  
	ConnectMessage(MLast);

	while(true)
	{
		try
		{
			fputsSocket(cmd_socket_, (str + "\r\n").c_str());
			getKeepStopWatch().reset();
			return getreply();
		}
		catch(ConnectionError& e)
		{
			BOOST_LOG(ERR, L"!command(" << FromOEM(str) << L"): " << FromOEM(e.what()));
			AddCmdLine(WinAPI::fromOEM(e.what()), ldRaw);
			if(ConnectMessageTimeout(MConnectionLost, getHost().url_.Host_, true))
				getPlugin()->FullConnect();
			else
			{
				getPlugin()->BackToHosts();
				getPlugin()->Invalidate();
				lostpeer();
				return RPL_ERROR;
			}
		}
	}
}


int Connection::command(const std::wstring &str, bool isQuit)
{
	return command(toOEM(str), isQuit);
}

void Connection::getline(std::string &str, DWORD tm)
{
	int c;
	bool wasCarriageReturn = false;
	str = "";
	char buffer[3] = {cffIAC};

	do
	{
		c = fgetcSocket(cmd_socket_, tm);
		if(str.empty()) 
		{
			// handle telnet commands
			if(c == cffIAC) 
			{
				switch (c = fgetcSocket(cmd_socket_, tm))
				{
				case cffWILL:
				case cffWONT:
					c = fgetcSocket(cmd_socket_, tm);
					buffer[1] = cffDONT;
					buffer[2] = c;
					nb_send(cmd_socket_, buffer, 3, 0);
					break;
				case cffDO:
				case cffDONT:
					c = fgetcSocket(cmd_socket_, tm);
					buffer[1] = cffWONT;
					buffer[2] = c;
					nb_send(cmd_socket_, buffer, 3, 0);
					break;
				default:
					break;
				}
				continue;
			}
		}
		if(wasCarriageReturn)
		{
			if(c != '\n')
				throw std::exception("invalid replay");
			return;
		}
		if(c == '\r')
			wasCarriageReturn = true;
		else
			str.push_back(c);
	} while(true);
}

int Connection::getreply(DWORD tm)
{
	int		originalcode = 0;
	bool	continuation = false;
	bool	isLastLine;
	std::string line;
	int		code;

	replyString_ = "";
	do
	{
		getline(line, tm);
		code = 0;
		isLastLine = false;
		if(line.size() >= 4)
		{
			int i;
			for(i = 0; i < 3; ++i)
			{
				if(isdigit(line[i]))
					code = code * 10 + line[i] - '0';
				else
					break;
			}
			if(i == 3)
			{
				if(line[3] == ' ')
					isLastLine = true;
				else
					if(line[3] == '-')
						continuation = true;
			}
		}

		if(replyString_.empty())
		{
			// parsing of the first line
			if(!isLastLine && !continuation)
				throw std::exception("incorrect reply");
			originalcode = code;
		} else
			replyString_ += '\n';

		replyString_ += line;

	} while(!(isLastLine && (!continuation || code == originalcode)));

	AddCmdLine(replyString_, ldIn);

	cpend = !isPrelim(originalcode);
	return originalcode;
}

void Connection::setHost(FTPHost* host)
{
	pHost_ = host;
}

FTPHost& Connection::getHost()
{
	BOOST_ASSERT(pHost_);
	return *pHost_;
}

const FTPHost& Connection::getHost() const
{
	BOOST_ASSERT(pHost_);
	return *pHost_;
}