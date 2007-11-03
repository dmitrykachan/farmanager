#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"
#include <fcntl.h>


bool Connection::Init(const std::wstring& Host, size_t Port, const std::wstring& User, const std::wstring& Password, FTP* ftp)
{  
	FARWrappers::Screen scr;
	PROCP(L"host: " << Host << L"user: " << User << L"password: " << Password);

	_set_fmode(_O_BINARY); // This causes an error somewhere.
	SetLastError( ERROR_SUCCESS );

	setPlugin(ftp);

	/* Set up defaults for FTP. */
	type = TYPE_A;
	cpend = 0;  /* no pending replies */
	setConnected(false);
	portnum		= -1;
	brk_flag	= false;
	restart_point = 0;
	IOCallback	= false;

	keepAliveStopWatch_.setTimeout(g_manager.opt.KeepAlive*1000);

	if(setpeer_(Host, Port, User, Password) == false)
	{
		BOOST_LOG(INF, L"!setpeer");
		return false;
	}
	BOOST_LOG(INF, L"OK");

	return true;
}

bool Connection::hookup(const std::wstring& host, int port)
{
	FARWrappers::Screen scr;
	SOCKET   sock = INVALID_SOCKET;

	memset( &hisctladdr, 0, sizeof(hisctladdr) );
	hisctladdr.sin_port = port;

	ConnectMessage(MResolving, host);
	in_addr addr = TCPSocket::resolveName(toOEM(host).c_str());

	cmd_socket_.create();
	ConnectMessage(MWaitingForConnect, host);
	if(nb_connect(cmd_socket_, addr, port) == false)
		return false;

	myctladdr = cmd_socket_.getlocalname();

	//Read startup message from server
	ConnectMessage(MWaitingForResponse);
	if(isComplete(getreply()))
	{
		portnum = port;
		return true;
	}
	cmd_socket_.close();
	return false;
}

bool Connection::login(const std::wstring& user, const std::wstring& password, const std::wstring& account)
{
	int   n;

	ConnectMessage(MSendingName);
	userName_		= user.empty()? L"anonymous" : user;
	userPassword_	= password.empty()? g_manager.opt.defaultPassword_ : password;

	n = command(g_manager.opt.cmdUser_ + L' ' + userName_);
	if(!isContinue(n))
		return false;

	ConnectMessage(MPasswordName);
	n = command(g_manager.opt.cmdPass_ + L' ' + userPassword_);

	if(isContinue(n))
		n = command(g_manager.opt.cmdAcct_ + L' ' + account);
	if(!isComplete(n))
		return false;
	LoginComplete = true;

	return true;
}

void Connection::CheckResume()
{  
	bool res = getHost().AsciiMode? setascii() : setbinary();
	if(!res)
		return;
	if(command(g_manager.opt.cmdRest_ + L" 0") == RequestedFileActionPendingFurtherInformation)
	{
		resumeSupport_ = true;
	}
}

bool Connection::dataconn(in_addr data_addr, int port)
{
	PROC;

	if (brk_flag)
		return false;

	if(!getHost().PassiveMode)
	{
		TCPSocket s;
		sockaddr_in from;
		int			fromlen = sizeof(from);

		nb_accept(data_peer_, s, from);
		BOOST_LOG(INF, L"SOCK: accepted data");
		s.swap(data_peer_);
		return true;
	} else
	{
		if(!data_peer_.isCreated())
		{
			InternalError();
			return false;
		}

		if(brk_flag || !nb_connect(data_peer_, data_addr, port))
		{
			data_peer_.close();
			return false;
		}
		return true;
	}
}

/*
 * Need to start a listen on the data channel
 * before we send the command, otherwise the
 * server's connect may fail.
 */
bool Connection::initDataConnection(in_addr &data_addr, int &port)
{
	if(brk_flag)
		return false;

	data_addr = myctladdr.sin_addr;
	if(sendport_)
		port = 0; /* let system pick one */
	else
		port = ntohs(myctladdr.sin_port);

	data_peer_.create();

	if(sendport_ == false)
	{
		int on = 1;
		data_peer_.setSockOpt(SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
		// close socket if not ok;
	}

	if(!getHost().PassiveMode || sendport_ == false)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr = data_addr;
		addr.sin_port = htons(port);
		
		data_peer_.bind(&addr);
		addr = data_peer_.getlocalname();
		
		data_addr	= addr.sin_addr;
		port		= ntohs(addr.sin_port);
		data_peer_.listen(1);
	}

	if(sendport_)
	{
		if(!getHost().PassiveMode)
		{
			return doport(data_addr, port);
		} else
		{
			int pasvResult = command(g_manager.opt.cmdPasv_);
			std::string pasvReply = GetReply();
			size_t ofs;
			if(pasvResult != EnteringPassiveMode || (ofs = pasvReply.find('(')) == std::string::npos)
			{
				AddCmdLine(L"Incorrect port reply", ldRaw);
				return false;
			}

			unsigned a1, a2, a3, a4, p1, p2;

#define TOP(a) ((a) & ~0xFF)
			if(sscanf(pasvReply.c_str()+ofs, "(%u,%u,%u,%u,%u,%u)", &a1, &a2, &a3, &a4, &p1, &p2) !=6 ||
				TOP(a1) || TOP(a2) || TOP(a3) || TOP(a4) || TOP(p1) || TOP(p2) )
			{
				AddCmdLine(L"Incorrect port reply", ldRaw);
				return false;
			}

			data_addr.s_addr = (a4<<24) | (a3<<16) | (a2<<8) | a1;
			port			 = (p1<<8) | p2;
			if(!data_addr.s_addr || data_addr.s_addr == INADDR_ANY)
				return true;
			if(data_addr.s_addr != hisctladdr.sin_addr.s_addr)
			{
				std::wstring msg = L'[' + WinAPI::fromOEM(inet_ntoa(hisctladdr.sin_addr)) +
					L" -> " + WinAPI::fromOEM(inet_ntoa(data_addr)) + L']';
				ConnectMessage(msg);
			}
		}
		return true;
	}

	return true;
}
