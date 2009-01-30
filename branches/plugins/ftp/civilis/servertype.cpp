#include "stdafx.h"
#include "servertype.h"
#include "utils\parserprimitive.h"

std::locale defaultLocale_;

class VxWorksServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"VxWorks";}
	virtual std::wstring	getDescription() const	{ return L"VxWorks unix style listing"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"Tornado-VxWorks") != std::wstring::npos &&
				str.find(L"UNIX") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new VxWorksServer());
	}
};

class UnixServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"Unix";}
	virtual std::wstring	getDescription() const	{ return L"Classic unix style listing"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"MACOS Peter's Server") != std::wstring::npos 
			|| str.find(L"MACOS WebSTAR FTP") != std::wstring::npos 
			|| str.find(L"UNIX") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new UnixServer());
	}
};


class QNXServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"QNX";}
	virtual std::wstring	getDescription() const	{ return L"QNX style listing"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"QNX") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new QNXServer());
	}
};


class WindowsServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"Windows";}
	virtual std::wstring	getDescription() const	{ return L"dir-like listing"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"Windows_NT") != std::wstring::npos 
			|| str.find(L"Windows_CE") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new WindowsServer());
	}
};

class VMSServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"VMS";}
	virtual std::wstring	getDescription() const	{ return L"VMS system"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"VMS") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new VMSServer());
	}
private:
	void					setFileName(const const_iterator &itr, const const_iterator &itr_end, FTPFileInfo &fileinfo) const;
	entry					getEntry(const_iterator &itr, const const_iterator &itr_end) const;
};


class CMSServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"CMS";}
	virtual std::wstring	getDescription() const	{ return L"CMS plain files list"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"VM/CMS") != std::wstring::npos
			|| str.find(L"VM ") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new CMSServer());
	}
};

 // EPLF: see http://pobox.com/~djb/proto/eplf.txt
class EPLFServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"EPLF";}
	virtual std::wstring	getDescription() const	{ return L"Comma delimited files list"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"UNIX") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new EPLFServer());
	}
};

class MacOsTCPServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"Mac TCP";}
	virtual std::wstring	getDescription() const	{ return L"MAC-OS TCP/Connect II"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"MAC-OS TCP/Connect II") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new MacOsTCPServer());
	}
};

class Os2Server: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"OS/2";}
	virtual std::wstring	getDescription() const	{ return L"OS/2 native format"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"OS/2") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new Os2Server());
	}
};

class KomutServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"Komut";}
	virtual std::wstring	getDescription() const	{ return L"File listing generated by some comutation servers"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &/*str*/) const
	{
		return false;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new KomutServer());
	}
private:
	void parseKomutDateTime(const_iterator &itr, const const_iterator &itr_end, 
							WinAPI::FileTime &ft, bool skipspaces = true) const;
};

class NetwareServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"Netware";}
	virtual std::wstring	getDescription() const	{ return L"Netware file server"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"NETWARE") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new NetwareServer());
	}
};

class VxWorksDosServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"VxWorksDos";}
	virtual std::wstring	getDescription() const	{ return L"VxWorks DOS stype listing"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"VxWorks") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new VxWorksDosServer());
	}
};

class PCTPCServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"PC/TPC";}
	virtual std::wstring	getDescription() const	{ return L"PC/TPC v 2.11 ftpsrv.exe"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const;
	virtual bool			acceptServerInfo(const std::wstring &str) const
	{
		return str.find(L"PC/TCP ") != std::wstring::npos;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new PCTPCServer());
	}
};

class AutodetectServer: public ServerType
{
public:
	virtual std::wstring	getName() const			{ return L"Autodetect";}
	virtual std::wstring	getDescription() const	{ return L"Autodetect"; }
	virtual void			parseListingEntry(const_iterator itr, 
		const const_iterator &itr_end,
		FTPFileInfo& fileinfo) const
	{
		ServerList& list = ServerList::instance();
		ServerList::const_iterator i = list.begin();
		while(i != list.end())
		{
			if(typeid(*i) == typeid(this))
				continue;
			if((*i)->isValidEntry(itr, itr_end))
			{
				try
				{
					(*i)->parseListingEntry(itr, itr_end, fileinfo);
					return;
				}
				catch (std::exception &e)
				{
					e;
				}
			}
			++i;
		}
		if(i == list.end())
		{
			throw std::exception("parser for this entry is not found");
		}
	}
	virtual bool			acceptServerInfo(const std::wstring &/*str*/) const
	{
		BOOST_ASSERT(false && "acceptServerInfo is not realized for AutodetectServer");
		return false;
	}
	virtual ServerTypePtr clone() const
	{
		return boost::shared_ptr<ServerType>(new AutodetectServer());
	}
};

static bool oneof(wchar_t test, const wchar_t* valid)
{
	test = std::tolower(test, defaultLocale_);
	while(*valid)
	{
		if(test == *valid)
			return true;
		++valid;
	}
	return false;
}

static bool verifyUnixAttribute(const std::wstring &u)
{
	return u.size() == 10 
		&& oneof(u[0], L"-dcbpl")
		&& oneof(u[1], L"-r")
		&& oneof(u[2], L"-w")
		&& oneof(u[3], L"-xs")
		&& oneof(u[4], L"-r")
		&& oneof(u[5], L"-w")
		&& oneof(u[6], L"-xs")
		&& oneof(u[7], L"-r")
		&& oneof(u[8], L"-w")
		&& oneof(u[9], L"-xt");
}

static void parseUnixAttribute(ServerType::const_iterator& itr, const ServerType::const_iterator &itr_end, FTPFileInfo &fileinfo)
{
	const int	UnixModeSize = 10;
	Parser::check(itr_end - itr >= UnixModeSize, "the unix attribute is too small");
	const std::wstring attribute(itr, itr+UnixModeSize);
	itr += UnixModeSize;
	Parser::check(verifyUnixAttribute(attribute), "the unix attribute string is not valid");
	fileinfo.setSecondAttribute(attribute);

	switch(std::tolower(attribute[0], defaultLocale_))
	{
	case '-': fileinfo.setType(FTPFileInfo::file); break;
	case 'd': fileinfo.setType(FTPFileInfo::directory); break;
	case 'l': fileinfo.setType(FTPFileInfo::symlink); break;
	case 'b': 
	case 'c': fileinfo.setType(FTPFileInfo::specialdevice); break;
	default:
		Parser::check(false, "unknown type");
	}
}


void VxWorksServer::parseListingEntry(const_iterator itr, 
									  const const_iterator &itr_end,
									  FTPFileInfo& fileinfo) const
{
	using namespace Parser;
	std::wstring::const_iterator itr2;

	skipSpaces(itr, itr_end);
	parseUnixAttribute(itr, itr_end, fileinfo);
	skipSpaces(itr, itr_end);
	skipNumber(itr, itr_end);

	itr2 = itr;
	skipWord(itr, itr_end, false);
	fileinfo.setOwner(itr2, itr);
	skipSpaces(itr, itr_end);

	fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));
	skipWord(itr, itr_end);
	Month mon = parseMonth(itr, itr_end);
	int day = parseUnsignedNumberRange(itr, itr_end, 1, 31);

	WinAPI::FileTime time;
	parseTime(itr, itr_end, time);

	int year = parseUnsignedNumberRange(itr, itr_end, 1900, 2100);
	time.setDate(year, mon, day);
	fileinfo.setLastWriteTime(time);

	fileinfo.setFileName(itr, itr_end, false);
}

void UnixServer::parseListingEntry(const_iterator itr, 
									  const const_iterator &itr_end,
									  FTPFileInfo& fileinfo) const
{
	std::wstring::const_iterator itr2;
	using namespace Parser;

	skipSpaces(itr, itr_end);
	if(std::isdigit(*itr, defaultLocale_))
		skipNumber(itr, itr_end);

	parseUnixAttribute(itr, itr_end, fileinfo);
	checkString(itr, itr_end, "+");
	skipSpaces(itr, itr_end);
	skipNumber(itr, itr_end);

	itr2 = itr;
	skipWord(itr, itr_end, false);
	fileinfo.setOwner(itr2, itr);
	skipSpaces(itr, itr_end);

	// skip @
	checkString(itr, itr_end, L"@ ");
	skipSpaces(itr, itr_end);

	itr2 = itr;
	skipWord(itr, itr_end, false);
	fileinfo.setGroup(itr2, itr);
	skipSpaces(itr, itr_end);

	// skip @
	checkString(itr, itr_end, L"@ ");
	skipSpaces(itr, itr_end);

	if(fileinfo.getType() == FTPFileInfo::specialdevice)
	{
		// Special devices do not have size. But they have
		// device number fields. Skip it.
		skipNumber(itr, itr_end);
		skipString(itr, itr_end, ",");
		skipNumber(itr, itr_end);

	} else
	{
		fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));
	}

	WinAPI::FileTime ft;
	parseUnixDateTime(itr, itr_end, ft);

	fileinfo.setLastWriteTime(ft);
	fileinfo.setFileName(itr, itr_end, fileinfo.getType() == FTPFileInfo::symlink);
}

void QNXServer::parseListingEntry(const_iterator itr, 
								   const const_iterator &itr_end,
								   FTPFileInfo& fileinfo) const
{
	std::wstring::const_iterator itr2;
	using namespace Parser;

	skipSpaces(itr, itr_end);

	parseUnixAttribute(itr, itr_end, fileinfo);
	checkString(itr, itr_end, "+");
	skipSpaces(itr, itr_end);
	skipNumber(itr, itr_end);

	itr2 = itr;
	skipWord(itr, itr_end, false);
	fileinfo.setOwner(itr2, itr);
	skipSpaces(itr, itr_end);

	if(fileinfo.getType() == FTPFileInfo::specialdevice)
	{
		// Special devices do not have size. But they have
		// device number fields. Skip it.
		skipNumber(itr, itr_end);
		skipString(itr, itr_end, ",");
		skipNumber(itr, itr_end);

	} else
	{
		fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));
	}

	WinAPI::FileTime ft;
	parseUnixDateTime(itr, itr_end, ft);

	fileinfo.setLastWriteTime(ft);
	fileinfo.setFileName(itr, itr_end, fileinfo.getType() == FTPFileInfo::symlink);
}

void WindowsServer::parseListingEntry(const_iterator itr, 
									  const const_iterator &itr_end,
									  FTPFileInfo& fileinfo) const
{
	using namespace Parser;

	skipSpaces(itr, itr_end);
	WinAPI::FileTime ft;
	parseWindowsDate(itr, itr_end, ft);
	parseTime(itr, itr_end, ft);
	fileinfo.setLastWriteTime(ft);
	if(checkString(itr, itr_end, L"<DIR>"))
	{
		fileinfo.setType(FTPFileInfo::directory);
	} else
	{
		fileinfo.setType(FTPFileInfo::file);
		fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));
	}
	skipSpaces(itr, itr_end);
	fileinfo.setFileName(itr, itr_end, fileinfo.getType() == FTPFileInfo::symlink);
}

VMSServer::entry VMSServer::getEntry(const_iterator &itr, const const_iterator &itr_end) const
{
	entry e = ServerType::getEntry(itr, itr_end);
	if(itr != itr_end && *itr == L' ')
	{
		entry ee = ServerType::getEntry(itr, itr_end);
		e.second = ee.second;
	}
	return e;
}



static void parseVMSAttribute(VMSServer::const_iterator itr, 
									  const VMSServer::const_iterator &itr_end)
{
	static const boost::array<wchar_t, 4> attributes = {L'r', L'w', L'e', L'd'};
	boost::array<wchar_t, 4>::const_iterator i;
	while(itr != itr_end)
	{
		i = std::find(attributes.begin(), attributes.end(), 
			std::tolower(*itr, defaultLocale_));
		Parser::check(i != attributes.end(), "invalid VMS attribute");
		++itr;
	}
}

void VMSServer::setFileName(const const_iterator &itr, const const_iterator &itr_end, FTPFileInfo &fileinfo) const
{
	const_iterator i = itr_end;
	static const wchar_t dir[] = L".dir";
	const int dir_len = sizeof(dir)/sizeof(*dir)-1;
	fileinfo.setType(FTPFileInfo::file);

	if(std::distance(itr, itr_end) > dir_len)
	{
		i -= dir_len;
		const wchar_t* p = dir;
		while(i != itr_end)
		{
			if(std::tolower(*i, defaultLocale_) != *p)
				break;
			++i; ++p;
		}
		if(i == itr_end)
		{
			fileinfo.setType(FTPFileInfo::directory);
			i = itr_end - dir_len;
		} else
			i = itr_end;
	}
	fileinfo.setFileName(itr, i);
}

void VMSServer::parseListingEntry(const_iterator itr, 
									  const const_iterator &itr_end,
									  FTPFileInfo& fileinfo) const
{
	using namespace Parser;
	const_iterator itr2, itr3;
	skipSpaces(itr, itr_end);
	itr2 = itr;
	skipNSpaces(itr2, itr_end);
	itr3 = std::find(std::reverse_iterator<const_iterator>(itr2), 
					 std::reverse_iterator<const_iterator>(itr), L';').base();
	if(itr3 != itr)
	{
		setFileName(itr, itr3-1, fileinfo);
		skipNumber(itr3, itr2, false); // check that a number is followed after ';' 
	} else
	{
		setFileName(itr, itr2, fileinfo);
	}
	itr = itr2;
	skipSpaces(itr, itr_end);
	FTPFileInfo::FileSize size = parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end);
	if(checkString(itr, itr_end, L"/"))
		size = parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end);
	fileinfo.setFileSize(size*512);

	int		day = parseUnsignedNumberRange(itr, itr_end, 1, 31, false);
	skipString(itr, itr_end, L"-");
	Month	mon = parseMonth(itr, itr_end, false);
	skipString(itr, itr_end, L"-");
	int		year = parseUnsignedNumberRange(itr, itr_end, 1900, 2099);
	WinAPI::FileTime ft;
	ft.setDate(year, mon, day);
	parseTime(itr, itr_end, ft);
	fileinfo.setLastWriteTime(ft);

	// owner, group
	skipString(itr, itr_end, L"[");
	itr2 = std::find(itr, itr_end, L']');
	itr3 = std::find(itr, itr2, L',');
	if(itr3 != itr2)
	{
		fileinfo.setOwner(itr, itr3);
		fileinfo.setGroup(itr3+1, itr2);
	} else
		fileinfo.setOwner(itr, itr2);
	itr = itr2+1;
	skipSpaces(itr, itr_end);

	// attribute
	skipString(itr, itr_end, L"(");
	const_iterator attr_itr = itr;
	for(int i = 0; i < 4; i++)
	{
		itr2 = std::find(itr, itr_end, (i == 3)? L')' : L',');
		check(itr2 != itr_end, "VMS attribute is incorrect");
		parseVMSAttribute(itr, itr2);
		itr = itr2 + 1;
	}
	fileinfo.setSecondAttribute(attr_itr, itr2);
}

static bool isOctalDigit(wchar_t c)
{
	return c >= L'0' && c <= '7';
}

static void parseEPLFAttribute(ServerType::const_iterator &itr, const ServerType::const_iterator itr_end, FTPFileInfo& fileinfo)
{
	Parser::check(itr+3 == itr_end && 
		isOctalDigit(*itr) && isOctalDigit(*(itr+1)) && isOctalDigit((*itr+2)),
		"the octal number is expected");
	fileinfo.setSecondAttribute(itr, itr_end);
}


// Easily Parsed LIST Format:  http://cr.yp.to/ftp/list/eplf.html
void EPLFServer::parseListingEntry(const_iterator itr, 
									  const const_iterator &itr_end,
									  FTPFileInfo& fileinfo) const
{
	Parser::check(itr != itr_end && *itr == L'+', "Invalid entry. The '+' should be in the begin of the line.");

	const_iterator itr_tab = find(itr, itr_end, L'\t');
	Parser::check(itr_tab != itr_end, "Invalid entry. The '\\t' is not found.");

	fileinfo.setLastWriteTime(WinAPI::FileTime());
	++itr;
	while(itr != itr_tab)
	{
		const wchar_t info = *itr;
		++itr;
		switch(info)
		{
			// date
		case L'm':
			{
				size_t n = Parser::parseUnsignedNumber<size_t>(itr, itr_tab, false);
				time_t time = n;

				struct tm *t = gmtime(&time);
				if(t == 0)
				{
					fileinfo.setLastWriteTime(WinAPI::FileTime());
				} else
				{
					fileinfo.setLastWriteTime(
						WinAPI::FileTime(t->tm_year+1900, t->tm_mon+1,
										t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec));
				}
			}
			break;
		case L'/':
			fileinfo.setType(FTPFileInfo::directory);
			break;
		case L'r':
			fileinfo.setType(FTPFileInfo::file);
		    break;
		case L's':
			fileinfo.setFileSize(Parser::parseUnsignedNumber<__int64>(itr, itr_tab, false));
		    break;
		case L'i':
			itr = find(itr, itr_tab, ',');
			break;
		case L'u':
			if(itr != itr_tab)
			{
				Parser::check(*itr == 'p', "Invalid entry. The fact is unknown.");
				++itr;
				const_iterator attr_itr = itr;
				Parser::skipNumber(itr, itr_tab);
				fileinfo.setSecondAttribute(attr_itr, itr);
			}
			break;
		default:
			Parser::check(false, "Invalid entry. The fact is unknown.");
		}
		if(itr != itr_tab)
		{
			Parser::check(*itr == L',', "Invalid entry. The ',' is missing.");
			++itr;
		}
	}

	Parser::check(fileinfo.getType() != FTPFileInfo::undefined, "The type of entry is not set");
	fileinfo.setFileName(itr_tab+1, itr_end);
	fileinfo.setLastAccessTime(fileinfo.getLastWriteTime());
	fileinfo.setCreationTime(fileinfo.getLastWriteTime());
}

void CMSServer::parseListingEntry(const_iterator itr, 
									  const const_iterator &itr_end,
									  FTPFileInfo& fileinfo) const
{
	fileinfo.setFileName(itr, itr_end);
}

void MacOsTCPServer::parseListingEntry(const_iterator itr, 
									  const const_iterator &itr_end,
									  FTPFileInfo& fileinfo) const
{
	Parser::check(itr != itr_end, "Empty entry");
	const_iterator itr_e = itr_end;
	if(*(itr_e-1) == L'/')
	{
		--itr_e;
		fileinfo.setType(FTPFileInfo::directory);
	} else
		fileinfo.setType(FTPFileInfo::file);
	fileinfo.setFileName(itr, itr_e);
}

void VxWorksDosServer::parseListingEntry(const_iterator itr, 
									   const const_iterator &itr_end,
									   FTPFileInfo& fileinfo) const
{
	using namespace Parser;
	const_iterator itr2;
	skipSpaces(itr, itr_end);
	fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));
	Month mon = parseMonth(itr, itr_end, false);
	skipString(itr, itr_end, L"-", false);
	int day = parseUnsignedNumberRange(itr, itr_end, 1, 31, false);
	skipString(itr, itr_end, L"-", false);
	int year = parseUnsignedNumberRange(itr, itr_end, 1900, 2099);

	WinAPI::FileTime ft;
	ft.setDate(year, mon, day);
	parseTime(itr, itr_end, ft);
	fileinfo.setLastWriteTime(ft);

	itr2 = itr;
	skipNSpaces(itr, itr_end);
	fileinfo.setFileName(itr2, itr);
	skipSpaces(itr, itr_end);
	if(itr == itr_end)
	{
		fileinfo.setType(FTPFileInfo::file);
		return;
	}
	skipString(itr, itr_end, L"<DIR>");
	fileinfo.setType(FTPFileInfo::directory);
}

void Os2Server::parseListingEntry(const_iterator itr, 
									   const const_iterator &itr_end,
									   FTPFileInfo& fileinfo) const
{
	using namespace Parser;
	skipSpaces(itr, itr_end);
	fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));
	int attribute;
	parseOs2Attribute(itr, itr_end, attribute);
	if(checkString(itr, itr_end, L"DIR"))
	{
		fileinfo.setType(FTPFileInfo::directory);
		skipSpaces(itr, itr_end);
	}
	else
		fileinfo.setType(FTPFileInfo::file);
	WinAPI::FileTime ft;
	parseWindowsDate(itr, itr_end, ft);
	parseTime(itr, itr_end, ft);
	fileinfo.setLastWriteTime(ft);
	fileinfo.setFileName(itr, itr_end, false);
}

void KomutServer::parseKomutDateTime(const_iterator &itr, const const_iterator &itr_end, 
								   WinAPI::FileTime &ft, bool skipspaces) const
{
	using namespace Parser;
	static const wchar_t FF[] = L"FF.FF:FF FF.FF.FFFF";
	static const wchar_t ZZ[] = L"00.00:00 00.00.0000";
	const int datatime_len = sizeof(FF)/(sizeof(*FF))-1;
	static WinAPI::FileTime zeroTime = WinAPI::FileTime(1900, 1, 1, 0, 0, 0);
	
	check(std::distance(itr, itr_end) >= datatime_len, "invalid data&time");

	if(std::equal(itr, itr+datatime_len, FF) || 
	   std::equal(itr, itr+datatime_len, ZZ))
	{
		ft = zeroTime;
		itr += datatime_len;
		if(skipspaces)
			skipSpaces(itr, itr_end);
		return;
	}

	int hour = parseUnsignedNumberRange(itr, itr_end, 0, 23, false);
	skipString(itr, itr_end, L".");
	int minute = parseUnsignedNumberRange(itr, itr_end, 0, 59, false);
	skipString(itr, itr_end, L":");
	int second = parseUnsignedNumberRange(itr, itr_end, 0, 59);
	ft.setTime(hour, minute, second);

	int day = parseUnsignedNumberRange(itr, itr_end, 1, 31, false);
	skipString(itr, itr_end, L".");
	int month = parseUnsignedNumberRange(itr, itr_end, 1, 12, false);
	skipString(itr, itr_end, L".");
	int year = parseUnsignedNumberRange(itr, itr_end, 1900, 2099, skipspaces);
	ft.setDate(year, month, day);
}


void KomutServer::parseListingEntry(const_iterator itr, 
									   const const_iterator &itr_end,
									   FTPFileInfo& fileinfo) const
{
	using namespace Parser;
	const_iterator itr2;
	skipSpaces(itr, itr_end);
	WinAPI::FileTime ft;
	parseKomutDateTime(itr, itr_end, ft);
	fileinfo.setLastWriteTime(ft);
	parseKomutDateTime(itr, itr_end, ft);
	fileinfo.setLastAccessTime(ft);

	fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end)*512);
	itr2 = itr_end;
	if(std::distance(itr, itr2) >= 4 && equalIgnoreCase(itr2-4, itr2, ".dir"))
	{
		itr2 -= 4;
		fileinfo.setType(FTPFileInfo::directory);
	} else
		fileinfo.setType(FTPFileInfo::file);

	fileinfo.setFileName(itr, itr2, false);
}

void NetwareServer::parseListingEntry(const_iterator itr, 
									   const const_iterator &itr_end,
									   FTPFileInfo& fileinfo) const
{
	using namespace Parser;
	std::wstring::const_iterator itr2;

	skipSpaces(itr, itr_end);
	WinAPI::FileTime ft;
	if(*itr == L'-')
		fileinfo.setType(FTPFileInfo::file);
	else if(std::tolower(*itr, defaultLocale_) == L'd')
		fileinfo.setType(FTPFileInfo::directory);
	else
		check(true, "'-' or 'd' are expected");
	++itr;
	skipSpaces(itr, itr_end);
	check(*itr == L'[', "'[' are expected");
	++itr;
	itr = std::find(itr, itr_end, L']');
	check(itr != itr_end, "']' not found");
	++itr;
	skipSpaces(itr, itr_end);

	int dummy;
	checkUnsignedNumber(itr, itr_end, dummy);
	if(!std::isdigit(*itr, defaultLocale_))
	{
		itr2 = itr;
		skipWord(itr, itr_end, false);
		fileinfo.setOwner(itr2, itr);
		skipSpaces(itr, itr_end);
	}
	fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));

	Month mon = parseMonth(itr, itr_end);
	int day = parseUnsignedNumberRange(itr, itr_end, 1, 31);
	itr2 = itr;
	int year = parseUnsignedNumber<int>(itr, itr_end);
	WinAPI::FileTime time;
	if(year < 1900)
	{
		parseTime(itr2, itr_end, time);
		itr = itr2;
		year = WinAPI::FileTime::getCurrentSystemTime().wYear;
	} else
		time.setTime(0, 0, 0);

	time.setDate(year, mon, day);
	fileinfo.setLastWriteTime(time);
	fileinfo.setFileName(itr, itr_end, false);
}

void PCTPCServer::parseListingEntry(const_iterator itr, 
									   const const_iterator &itr_end,
									   FTPFileInfo& fileinfo) const
{
	using namespace Parser;
	skipSpaces(itr, itr_end);
	if(checkString(itr, itr_end, L"<DIR>", false))
	{
		fileinfo.setType(FTPFileInfo::directory);
		skipSpaces(itr, itr_end);
	} else
	{
		fileinfo.setType(FTPFileInfo::file);
		fileinfo.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));
	}
	const_iterator itr2 = std::find(itr, itr_end, L' ');
	fileinfo.setFileName(itr, itr2);
	itr = itr2;

	skipSpaces(itr, itr_end);
	skipWord(itr, itr_end);
	Month mon = parseMonth(itr, itr_end);
	int	  day = parseUnsignedNumberRange(itr, itr_end, 1, 31);

	WinAPI::FileTime ft;
	parseTime(itr, itr_end, ft);
	int   year = parseUnsignedNumberRange(itr, itr_end, 1900, 2099);
	ft.setDate(year, mon, day);
	fileinfo.setLastWriteTime(ft);
}



void ServerList::addServer(ServerType* type)
{
	list_.push_back(boost::shared_ptr<ServerType>(type));
}


ServerList::ServerList()
{
	addServer(new VxWorksServer);
	addServer(new VxWorksDosServer);
	addServer(new QNXServer);
	addServer(new UnixServer);
	addServer(new WindowsServer);
	addServer(new VMSServer);
	addServer(new EPLFServer);
	addServer(new CMSServer);
	addServer(new MacOsTCPServer);
	addServer(new Os2Server);
	addServer(new KomutServer);
	addServer(new NetwareServer);
	addServer(new PCTPCServer);
	addServer(new AutodetectServer);
}

static bool updateTime(FTPFileInfo& fileinfo)
{
	WinAPI::FileTime ft = fileinfo.getLastWriteTime();
	if(ft.empty())
		return false;
	if(fileinfo.getCreationTime().empty())
		fileinfo.setCreationTime(ft);
	if(fileinfo.getLastAccessTime().empty())
		fileinfo.setLastAccessTime(ft);
	return true;
}

void parseListing(const std::wstring &listing, boost::shared_ptr<ServerType> pServer, 
				  std::vector<FTPFileInfo>& files)
{
	static FTPFileInfo emptyFileInfo;
	std::wstring::const_iterator itr = listing.begin();
	std::wstring::const_iterator itr_end;
	ServerType::entry entry;

	files.clear();
	do
	{
		FTPFileInfo fileinfo;

		entry = pServer->getEntry(itr, listing.end());
		if(entry.first == entry.second)
			break;

		try
		{
			fileinfo.clear();
			pServer->parseListingEntry(entry.first, entry.second, fileinfo);
			updateTime(fileinfo);
		}
		catch (std::exception &e)
		{
			e;
#ifdef CONFIG_TEST
			BOOST_CHECK_MESSAGE(false, std::string(e.what())+" : " + std::string(entry.first, entry.second));
#endif
			fileinfo.setType(FTPFileInfo::undefined);
		}
		files.push_back(fileinfo);
	} while(true);
}


ServerType::entry ServerType::getEntry(const_iterator &itr, const const_iterator &itr_end) const
{
	const_iterator first = itr, last;
	const wchar_t entrySeparator[] = L"\n\r";
	itr = last = std::find_first_of(itr, itr_end, entrySeparator, entrySeparator + 2);

	if(itr != itr_end)
	{
		++itr;

		while(itr != itr_end && (*itr == '\n' || *itr == '\r'))
			++itr;
	}
	return std::make_pair(first, last);
}

ServerTypePtr ServerList::create(const std::wstring &name)
{
	const_iterator itr = find(name);
	if(itr == end())
	{
		return boost::shared_ptr<ServerType>(new AutodetectServer);
	}
	return (*itr)->clone();
}

bool ServerList::isAutodetect(const ServerTypePtr& server)
{
	return typeid(*server.get()) == typeid(AutodetectServer);
}

ServerTypePtr ServerList::autodetect(const std::wstring &serverInfo)
{
	ServerList& list = ServerList::instance();
	ServerList::const_iterator itr = list.begin();
	while(itr != list.end())
	{
		// skip autodetect
		if(typeid(itr->get()) != typeid(AutodetectServer))
		{
			if((*itr)->acceptServerInfo(serverInfo))
				return (*itr)->clone();
		}
		++itr;
	}
	return boost::shared_ptr<ServerType>(new AutodetectServer());
}


bool ServerType::isValidEntry(const const_iterator &itr, const const_iterator &itr_end) const
{
	//Check contains skipped text
	static boost::array<std::wstring, 6> skippedMsg = 
	{ {
		L"data connection",
			L"transfer complete",
			L"bytes received",
			L"DEVICE:[",
			L"Total of ",
			L"total "
	} };

	class Check
	{
	private:
		const const_iterator &first_;
		const const_iterator &last_;
	public:
		Check(const const_iterator &first, const const_iterator &last)
			: first_(first), last_(last)
		{}
		bool operator()(const std::wstring &mess)
		{
			return search(first_, last_, mess.begin(), mess.end()) != last_;
		}
	}check(itr, itr_end);
	return std::find_if(skippedMsg.begin(), skippedMsg.end(), check) == skippedMsg.end();
}
