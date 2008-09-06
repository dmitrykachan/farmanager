#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"
#include "utils/strutils.h"
#include "utils/uniconverts.h"
#include "servertype.h"

#include "farwrapper/menu.h"

const char quote = '\x1';

std::string ftpQuote(const std::string& str)
{
	return quote + str + quote;
}

std::string ftpQuote(const std::wstring& str)
{
	return quote + Unicode::toOem(str) + quote;
}

bool Connection::getBreakable()
{
	return breakable_;
}

void Connection::setBreakable(bool value)
{
	breakable_ = value;
}

int FtpCmdBlock(Connection *hConnect, bool block)
{
	int rc = -1;

	if (!hConnect)
		return -1;

	rc = hConnect->CmdVisible == false;

	hConnect->CmdVisible = block == false;
	return rc;
}



std::wstring octal(int n)
{
	wchar_t buffer[16];
	_snwprintf(buffer, sizeof(buffer)/sizeof(*buffer), L"%o", n);
	return buffer;
}

boost::shared_ptr<ServerType> FTP::SelectServerType(boost::shared_ptr<ServerType> defType)
{
	FARWrappers::Menu menu(FMENU_AUTOHIGHLIGHT);

	ServerList &list = ServerList::instance();
	menu.reserve(list.size());
	menu.setTitle(getMsg(MTableTitle));

	ServerList::const_iterator itr = list.begin();

	std::vector<std::string> vec;

	class MaxNameSize: public std::binary_function<ServerTypePtr, ServerTypePtr, bool>
	{
	public:
		bool operator()(const ServerTypePtr &l, ServerTypePtr &r)
		{
			return l->getName().size() < r->getName().size();
		}
	};

	size_t maxNameLengh = std::max_element(list.begin(), list.end(), MaxNameSize())
		->get()->getName().size();

	while(itr != list.end()) 
	{
		std::wstring name = (*itr)->getName();
		size_t nspaces = maxNameLengh - name.size();
		name += std::wstring(nspaces, L' ');
		name += FAR_VERT_CHAR + (*itr)->getDescription();
		menu.addItem(name);
		++itr;
	}
	BOOST_ASSERT(list.find(defType->getName()) != list.end());
	menu.select(list.find(defType->getName()) - list.begin());
	int rc = menu.show();

	return (rc == -1)? defType : list[rc];
}


const std::wstring& Connection::getCurrentDirectory() const
{
	BOOST_ASSERT(!curdir_.empty());
	return curdir_;
}

void Connection::setCurrentDirectory(const std::wstring& curdir)
{
	curdir_ = curdir;
}

void Connection::keepAlive()
{
	if(isConnected())
		pwd();
}