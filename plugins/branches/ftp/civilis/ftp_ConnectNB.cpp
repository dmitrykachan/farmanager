#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

bool Connection::nb_connect(TCPSocket &peer, in_addr &ipaddr, int port, DWORD tm)
{
	peer.connect(ipaddr, port);
    return true;
}

void Connection::displayWaitingMessage(size_t curTime, size_t timeout)
{
	std::wstringstream ws;
	ws << getMsg(MWaiting) << L" (";
	ws	<< curTime/1000 << getMsg(MSeconds) << L")";
	if(!getBreakable() && timeout != INFINITE)
		ws << L": " << std::setw(2) << (curTime*100+timeout-1)/timeout << L'%';

	IdleMessage(ws.str().c_str(), g_manager.opt.IdleColor);
}


int Connection::nb_recv(TCPSocket &peer, char* buf, int len, DWORD tm)
{
	WinAPI::Stopwatch stopwatch;
	if(tm == 0)
		tm = g_manager.opt.WaitTimeout*1000;
	if(peer.read(buf, &len, g_manager.opt.IdleStartPeriod) == false)
	{
		while(!peer.read(buf, &len, g_manager.opt.IdleShowPeriod) && 
			   (tm == INFINITE || stopwatch.getPeriod() < tm))
			displayWaitingMessage(stopwatch.getPeriod(), tm);
		if(stopwatch.getPeriod() >= tm)
			throw ReplyTimeOut;
	}
	return len;
}

void Connection::nb_send(TCPSocket &peer, const char* buf, size_t len, int flags, DWORD tm)
{
	WinAPI::Stopwatch stopwatch;
	if(tm == 0)
		tm = g_manager.opt.WaitTimeout*1000;
	if(peer.write(buf, len, g_manager.opt.IdleStartPeriod) == false)
	{
		while(!peer.write(buf, len, g_manager.opt.IdleShowPeriod) && 
			(tm == INFINITE || stopwatch.getPeriod() < tm))
			displayWaitingMessage(stopwatch.getPeriod(), tm);
		if(stopwatch.getPeriod() >= tm)
			throw ReplyTimeOut;
	}
}

void Connection::nb_accept(TCPSocket& listenPeer, TCPSocket& s, sockaddr_in &from, DWORD tm)
{
	WinAPI::Stopwatch stopwatch;
	if(tm == 0)
		tm = g_manager.opt.WaitTimeout*1000;
	if(listenPeer.accept(&from, s, tm) == false)
	{
		while(!listenPeer.accept(&from, s, g_manager.opt.IdleShowPeriod) && 
			(tm == INFINITE || stopwatch.getPeriod() < tm))
			displayWaitingMessage(stopwatch.getPeriod(), tm);
		if(stopwatch.getPeriod() >= tm)
			throw ReplyTimeOut;
	}
}