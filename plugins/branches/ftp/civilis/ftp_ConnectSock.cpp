#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

int Connection::fgetcSocket(TCPSocket &s,DWORD tm)
{
	char buffer;
	int len = 1;

	nb_recv(s, &buffer, len, tm);

	return buffer;
}

void Connection::fputsSocket(TCPSocket &s, const std::string &str)
{
	std::string buf;

	BOOST_LOG(INF, L">>>>>>>>>>>>>>>>>>>>>>>>>>>>>[" << FromOEM(str) << L"]");

	if(str.empty())
		return;

	AddCmdLine(str, ldOut);

	if(!getHost().FFDup)
	{
		nb_send(s, str.c_str(), str.size(), 0);
		return;
	}

	std::string::const_iterator itr = str.begin();
	while(itr != str.end())
	{
		buf.push_back(*itr);
		if(*itr == '\xFF')
			buf.push_back('\xFF');
		++itr;
	}

	nb_send(s, buf.c_str(), buf.size(), 0);
}
