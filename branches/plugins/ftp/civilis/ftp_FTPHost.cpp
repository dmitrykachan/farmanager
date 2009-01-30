#include "stdafx.h"

#pragma warning( disable : 240)
#pragma warning( disable : 4100)
#include "utils/sregexp.h"

#include "ftp_Int.h"
#include "farwrapper/dialog.h"

#define FTPHOST_DVERSION_WSTR			 L"1.2740"

//---------------------------------------------------------------------------------
typedef bool (*HexToPassword_cb)(const std::wstring& hexstr, std::wstring &password);

static BYTE HexToNum(wchar_t Hex)
{
	if ( Hex >= '0' && Hex <= '9' )
		return (BYTE)(Hex-'0');

	return (BYTE)(std::toupper(Hex, std::locale())-'A'+10);
}


static inline unsigned char lobyte(short n)
{
	return n & 0xFF;
}

static inline unsigned char hibyte(short n)
{
	return (n >> 8) & 0xFF;
}

static inline char tohex(int n)
{
	BOOST_ASSERT(n >=0 && n <= 15);
	return static_cast<char>((n < 10)? '0' + n : 'a' + n - 10);
}

std::wstring PasswordToHex(const std::wstring& password)
{  
	std::vector<unsigned char> pwd = MakeCryptPassword(password);

	std::wstring res = L"hex:";
	res.reserve(4 + pwd.size());
	std::vector<unsigned char>::const_iterator itr = pwd.begin();
	while(itr != pwd.end())
	{
		res.push_back(tohex((*itr >> 4) & 0xF));
		res.push_back(tohex(*itr & 0xF));
		++itr;
	}
	return res;
}

bool HexToPassword_OLD(const std::wstring& hexstr, std::wstring &password)
{
	if(hexstr.size() <= 4 && (hexstr.size() & 1))
		return false;
	if(hexstr.find(L"hex:", 0, 4) == std::wstring::npos)
		return false;

	std::wstring::const_iterator itr = hexstr.begin()+4;
	password = L"";
	while(itr != hexstr.end())
	{
		int n = HexToNum(*itr)<<4;
		++itr;
		n += HexToNum(*itr);
		password.push_back(static_cast<wchar_t>(n));
		++itr;
	}
	return true;
}

bool HexToPassword_2740(const std::wstring& hexstr, std::wstring &password)
{  
	if(hexstr.size() <= 4 && (hexstr.size() & 1))
		return false;

	if(hexstr.find(L"hex:", 0, 4) == std::wstring::npos)
		return false;

	std::vector<unsigned char> pwd;
	std::wstring::const_iterator itr = hexstr.begin()+4;
	while(itr != hexstr.end())
	{
		int n = HexToNum(*itr)<<4;
		++itr;
		n += HexToNum(*itr);
		pwd.push_back(static_cast<unsigned char>(n));
		++itr;
	}

	password = DecryptPassword(pwd);

	return TRUE;
}

void FTPHost::Init()
{
	PROCP(this);

	Folder		= false;
	hostDescription_ = L"";
	codePage_	= CP_OEMCP;

	url_.clear();

	AskLogin = PassiveMode = UseFirewall = AsciiMode = ExtCmdView = ProcessCmd =
		ExtList = FFDup = UndupFF = SendAllo = false;

	listCMD_	= L"LIST -la";

	ExtCmdView   = g_manager.opt.ExtCmdView;
	IOBuffSize   = g_manager.opt.IOBuffSize;
	PassiveMode  = g_manager.opt.PassiveMode;
	ProcessCmd   = g_manager.opt.ProcessCmd;
	UseFirewall  = g_manager.opt.firewall[0] != 0;
	FFDup        = g_manager.opt.FFDup;
	UndupFF      = g_manager.opt.UndupFF;
	serverType_   = ServerList::instance().create(L"");
}

void FTPHost::MkUrl(std::wstring &str, std::wstring Path, const std::wstring &nm)
{
	url_.directory_.swap(Path);
	url_.directory_ = Path;
	str = url_.toString(is_flag(g_manager.opt.PwdSecurity, SEC_PLAINPWD));
	url_.directory_.swap(Path);

	if (!nm.empty()) 
		str += L"/" + nm;

	FixFTPSlash( str );
}

class CopyAndCorrectBadCharacters: boost::noncopyable
{
private:
	std::wstring&	str_;
	bool			prevBad_;
	const wchar_t*	badCharacters_;
public:
	CopyAndCorrectBadCharacters(std::wstring &s, const wchar_t *badChars)
		: str_(s), prevBad_(false), badCharacters_(badChars)
	{}

	void operator()(wchar_t c)
	{
		if((c < 0x20) || wcschr(badCharacters_, c) != 0)
		{
			if(!prevBad_)
			{
				str_.push_back(L'_');
				prevBad_ = true;
			}
		} else
		{
			str_.push_back(c);
			prevBad_ = false;
		}
	}
};

void FTPHost::MkINIFile()
{
	iniFilename_ = L"";
	std::wstring fullhostname = url_.toString(false, false);
	iniFilename_.reserve(fullhostname.size());

	//Correct bad characters and add possible to DestName
	CopyAndCorrectBadCharacters f = CopyAndCorrectBadCharacters(iniFilename_, Folder? L":/\\\"\'|><^*?" : L":/\\.@\"\'|><^*?");
	std::for_each<std::wstring::const_iterator, CopyAndCorrectBadCharacters &>(fullhostname.begin(), fullhostname.end(), f);
	if(!Folder)
		iniFilename_ += L".ftp";
}

std::wostream& operator << (std::wostream& os, FTPHost& h)
{
	os << "user: " << h.url_.username_ << "\n";
	os << "pass: " << h.url_.password_ << "\n";
	os << "host: " << h.url_.Host_ << "\n";
	os << "port: " << h.url_.port_ << "\n";
	os << "dir : " << h.url_.directory_ << "\n";
	return os;
}

bool FTPUrl_::parse(const std::wstring &s)
{
	using namespace static_regex;

	typedef cs<c<'h'>, c<'H'> > h_;
	typedef cs<c<'t'>, c<'T'> > t_;
	typedef cs<c<'p'>, c<'P'> > p_;
	typedef cs<c<'f'>, c<'F'> > f_;

	typedef seq<h_, t_, t_, p_> http_;
	typedef seq<f_, t_, p_> ftp_;
	typedef alt<http_, ftp_> protocol_name;
	typedef seq<c<':'>, c<'/'>, c<'/'> > protocol_delimiter;

	typedef plus<alpha> string;

	enum
	{
		user = 1,
		pass,
		hst,
		port,
		dir,
		count
	};

	// ftp://user:password@host:port/dir
	typedef seq<
				opt<seq<protocol_name, protocol_delimiter> >, // skip http:// or ftp://
				opt<
					seq<
						mark<user, plus<not_cc<':', '@'> > >, // user
						opt< seq< 
								c<':'>, 
								mark<pass, star<not_cc<'@', '@'> > > // password
							> >, 
				c<'@'> 
				> 
		>, 
		mark<hst, plus<not_cc<':', '/'> > >, // host
		opt<mark<port, seq<c<':'>, plus<digit> > > >, // port
		opt<mark<dir, seq<c<'/'>, plus<dot> > > > // dir
	> ftpurl;

	username_	= L"";
	password_	= L"";
	Host_		= L"";
	directory_	= L"";
	port_		= 0;

	typedef match_array<std::wstring::const_iterator, count> MC;
	std::wstring::const_iterator itr;
	MC mc;
	std::wstring::const_iterator it = s.begin();
	if(match<ftpurl>(it, s.end(), mc))
	{
		for(MC::iterator i = mc.begin(); i != mc.end(); ++i)
		{
			switch(i->id())
			{
			case user:	username_.assign(i->begin(), i->end()); break;
			case pass:	password_.assign(i->begin(), i->end()); break;
			case hst:	Host_.assign(i->begin(), i->end()); break;
			case port:	itr = i->begin(); port_ = Parser::parseUnsignedNumber<size_t>(itr, i->end(), false); break;
			case dir:	directory_.assign(i->begin(), i->end()); break;

			}
		}
		return true;
	}
	return false;
}


std::wstring FTPUrl_::toString(bool insertPassword, bool useServiceName) const
{
	bool defLogin =	username_ == L"anonymous" && password_ == g_manager.opt.defaultPassword_;
	
	std::wstring str;
	str.reserve(6+username_.size()+password_.size()+2+Host_.size()+directory_.size());
	
	if(useServiceName)
		str += L"ftp://";

	if(!defLogin && !username_.empty()) 
	{
		str += username_;
		if(insertPassword && !password_.empty())
			str += L":" + password_;
		str += L"@";
	}
	str += Host_;

	if(!directory_.empty())
	{
		if(directory_[0] != NET_SLASH)
			str += NET_SLASH;
		str += directory_;
	}

	return str;
}

std::wstring FTPUrl_::getUrl(bool useServerName) const
{
	std::wstring str;
	str.reserve(Host_.size() + directory_.size() + 8);
	
	if(useServerName)
		str += L"ftp://";

	str += Host_;

	if(!directory_.empty())
	{
		if(directory_[0] != NET_SLASH)
			str += NET_SLASH;
		str += directory_;
	}

	return str;
}


bool FTPHost::SetHostName(const std::wstring& hnm, const std::wstring &usr, const std::wstring &pwd)
{
	if(!url_.parse(hnm))
		return false;

	if(!usr.empty())
		url_.username_ = usr;
	if(!pwd.empty())
		url_.password_ = pwd;

	return true;
}


WinAPI::RegKey FTPHost::findFreeKey(const std::wstring &path, std::wstring &name)
{
	std::wstring key;
	int  n = 0;
	WinAPI::RegKey r(g_manager.getRegKey(), path.c_str());

	while(true)
	{
		key = L"Item" + boost::lexical_cast<std::wstring>(n);
		if(!r.isSubKeyExists(key.c_str()))
			break;
		++n;
	}
	name = key;
	return WinAPI::RegKey::create(r, key.c_str());
}

std::wstring DecryptPassword_old(const std::vector<unsigned char> &crypt);


bool FTPHost::Read(const std::wstring &keyName, const std::wstring &path)
{  
	PROCP(keyName << L", " << path);
	std::vector<unsigned char>	psw;

	WinAPI::RegKey r;

	try
	{
		r.open(g_manager.getRegKey(), (HostRegName + path + keyName).c_str());

		setRegName(keyName);
		setRegPath(path);
		Folder = r.get(L"Folder", 0);
		if(Folder) 
		{
			url_.clear();
			hostDescription_	= r.get(L"Description", L"");
			url_.Host_			= r.get(L"HostName",	keyName);
			MkINIFile();
			return true;
		}

		url_.Host_ = r.get(L"HostName", L"");
		if(url_.Host_.empty())
			return false;

		url_.username_ = r.get(L"User", L"");
		try
		{
			r.get(L"Passwordw", &psw);
			if(!psw.empty())
				url_.password_ = DecryptPassword(psw);
		}
		catch(WinAPI::RegKey::exception&)
		{
			try
			{
				r.get(L"Password", &psw);
				url_.password_ = DecryptPassword_old(psw);
			}
			catch(WinAPI::RegKey::exception&)
			{
				return false;				
			}
		}

		hostDescription_	= r.get(L"Description",		L"");
		url_.directory_		= r.get(L"LocalDir",        L"");
		codePage_			= r.get(L"Table",			defaultCodePage);
		ProcessCmd			= r.get(L"ProcessCmd",		TRUE);
		AskLogin			= r.get(L"AskLogin",		FALSE);
		PassiveMode			= r.get(L"PassiveMode",		FALSE);
		UseFirewall			= r.get(L"UseFirewall",		FALSE);
		AsciiMode			= r.get(L"AsciiMode",		FALSE);
		ExtCmdView			= r.get(L"ExtCmdView",		g_manager.opt.ExtCmdView);
		ExtList				= r.get(L"ExtList",			FALSE);
		serverType_			= ServerList::instance().create(
							  r.get(L"ServerType",		g_manager.opt.defaultServerType_->getName()));
		listCMD_			= r.get(L"ListCMD",			L"LIST -la");
		IOBuffSize			= r.get(L"IOBuffSize",      g_manager.opt.IOBuffSize);
		FFDup				= r.get(L"FFDup",           g_manager.opt.FFDup);
		UndupFF				= r.get(L"UndupFF",         g_manager.opt.UndupFF);
		SendAllo			= r.get(L"SendAllo",        FALSE);

		IOBuffSize = std::max(FTR_MINBUFFSIZE,IOBuffSize);
		MkINIFile();

	}
	catch (WinAPI::RegKey::exception&)
	{
		return false;
	}

	return true;
}

bool FTPHost::Write()
{  
	WinAPI::RegKey r;
	if(getRegName().empty())
	{
		std::wstring name;
		r = findFreeKey(HostRegName + getRegPath(), name);
		setRegName(name);
	}
	else
	{
		r.open(g_manager.getRegKey(), (HostRegName + getRegPath() + getRegName()).c_str());
	}
	
	if(!Folder)
	{
		std::vector<unsigned char> psw;

		if(!url_.password_.empty())
			psw = MakeCryptPassword(url_.password_);

		std::wstring s = serverType_->getName();

		r.set(L"User",			url_.username_);
		r.set(L"Passwordw",		psw);
		r.set(L"LocalDir",		url_.directory_);
		r.set(L"Table",			codePage_);
		r.set(L"AskLogin",		AskLogin   );
		r.set(L"PassiveMode",	PassiveMode);
		r.set(L"UseFirewall",	UseFirewall);
		r.set(L"AsciiMode",		AsciiMode  );
		r.set(L"ExtCmdView",	ExtCmdView );
		r.set(L"ExtList",		ExtList );
		r.set(L"ServerType",	serverType_->getName());
		r.set(L"ListCMD",		listCMD_);
		r.set(L"ProcessCmd",	ProcessCmd );
		r.set(L"IOBuffSize",	IOBuffSize );
		r.set(L"FFDup",			FFDup);
		r.set(L"UndupFF",		UndupFF);
		r.set(L"SendAllo",		SendAllo);
	}
	r.set(L"HostName",			url_.Host_);
	r.set(L"Description",		hostDescription_);
	r.set(L"Folder",			Folder);

	return true;
}


class Profile
{
public:
	Profile(const std::wstring &filename, const std::wstring &section = L"")
		: filename_(filename), section_(section)
	{}

	void setSection(const std::wstring &section)
	{
		section_ = section;
	}

	bool setString(const std::wstring& name, const std::wstring& value)
	{
		return setString(name.c_str(), value.c_str());
	}

	bool setString(const wchar_t* name, const wchar_t* value)
	{
		BOOST_ASSERT(!filename_.empty() && !section_.empty());
		return WritePrivateProfileStringW(section_.c_str(), name, value, filename_.c_str());
	}

	std::wstring getString(const std::wstring& name, const std::wstring& def)
	{
		return getString(name.c_str(), def.c_str());
	}

	std::wstring getString(const wchar_t* name, const wchar_t* def)
	{
		BOOST_ASSERT(!filename_.empty() && !section_.empty());

		DWORD size = 256, n;
		boost::scoped_array<wchar_t> buff(new wchar_t[size]);
		while((n = GetPrivateProfileStringW(section_.c_str(),
				name, def, buff.get(), size, filename_.c_str())) == size-1)
		{
			size *= 2;
			buff.reset(new wchar_t[size]);
		}
		return std::wstring(buff.get(), buff.get()+n);
	}

	int getInt(const wchar_t* name, int def)
	{
		return GetPrivateProfileIntW(section_.c_str(), name, def, filename_.c_str());
	}

	int getInt(const std::wstring &name, int def)
	{
		return getInt(name.c_str(), def);
	}

	bool setInt(const wchar_t* name, int value)
	{
		return WritePrivateProfileStringW(section_.c_str(), name, 
			boost::lexical_cast<std::wstring>(value).c_str(), filename_.c_str());
	}

	bool setInt(const std::wstring &name, int value)
	{
		return setInt(name.c_str(), value);		
	}

private:
	std::wstring filename_;
	std::wstring section_;
};


bool FTPHost::ReadINI(const std::wstring &nm)
{  
	std::wstring hex;

	Init();

	Profile p(nm, L"FarFTP");

	BOOST_ASSERT(0 && "not realized");
	HexToPassword_cb DecodeProc = NULL;
	if(p.getString(L"Version", L"") == FTPHOST_DVERSION_WSTR)
	{
		DecodeProc = HexToPassword_2740;
	} else
		DecodeProc = HexToPassword_OLD;

	url_.Host_ = p.getString(L"Host", L"");
	if(url_.Host_.empty())
		return false;
	url_.username_ = p.getString(L"User", L"");
	hex = p.getString(L"Password", L"");

	if(!DecodeProc(hex, url_.password_))
		return false;

	hostDescription_ = p.getString(L"Description", L"");


	AskLogin		= p.getInt(L"AskLogin", 0);
	AsciiMode		= p.getInt(L"AsciiMode", 0);
	PassiveMode		= p.getInt(L"PassiveMode", g_manager.opt.PassiveMode);
	UseFirewall		= p.getInt(L"UseFirewall", !g_manager.opt.firewall.empty());
	ExtCmdView		= p.getInt(L"ExtCmdView", g_manager.opt.ExtCmdView);
	ExtList			= p.getInt(L"ExtList", FALSE);
	std::wstring serverName = p.getString(L"ServerType", L"");
	serverType_		= ServerList::instance().create(serverName);
	ProcessCmd		= p.getInt(L"ProcessCmd", TRUE);
	listCMD_		= p.getString(L"ListCMD", listCMD_);
	IOBuffSize		= p.getInt(L"IOBuffSize", g_manager.opt.IOBuffSize);
	FFDup			= p.getInt(L"FFDup", g_manager.opt.FFDup);
	UndupFF			= p.getInt(L"UndupFF", g_manager.opt.UndupFF);
	SendAllo		= p.getInt(L"SendAllo", FALSE);
	codePage_		= p.getInt(L"CharTable", defaultCodePage);
	IOBuffSize		= std::max(FTR_MINBUFFSIZE, IOBuffSize);

	return TRUE;
}


bool writeString(const wchar_t *filename, const wchar_t* name, const wchar_t* value)
{
	return WritePrivateProfileStringW(L"FarFTP", name, value,  filename);
}

bool writeInt(const wchar_t *filename, const wchar_t* name, int value)
{
	return WritePrivateProfileStringW(L"FarFTP", name, 
		boost::lexical_cast<std::wstring>(value).c_str(), filename);
}

bool FTPHost::WriteINI(const std::wstring & nm) const
{  
	BOOL res;

	//CreateDirectory
	boost::filesystem::wpath path = nm;
	boost::filesystem::create_directories(path.branch_path());

	Profile p(nm, L"FarFTP");

	std::wstring hexPassword = PasswordToHex(url_.password_);
 	res = 
 		p.setString(L"Version",		FTPHOST_DVERSION_WSTR) &&
		p.setString(L"Host",		url_.Host_) &&
		p.setString(L"User",		url_.username_) &&
		p.setString(L"Password",	hexPassword) &&
		p.setString(L"LocalDir",	url_.directory_) &&
		p.setString(L"Description",	hostDescription_) &&
		p.setInt   (L"AskLogin",	AskLogin) &&
		p.setInt   (L"AsciiMode",	AsciiMode) &&
		p.setInt   (L"PassiveMode",	PassiveMode) &&
		p.setInt   (L"UseFirewall",	UseFirewall) &&
		p.setInt   (L"ExtCmdView",	ExtCmdView) &&
		p.setInt   (L"ExtList",		ExtList) &&
		p.setString(L"ServerType",	serverType_->getName()) &&
		p.setString(L"ListCMD",		listCMD_) &&
		p.setInt   (L"IOBuffSize",	IOBuffSize) &&
		p.setInt   (L"FFDup",		FFDup) &&
		p.setInt   (L"UndupFF",		UndupFF) &&
		p.setInt   (L"SendAllo",	SendAllo) &&
		p.setInt   (L"ProcessCmd",	ProcessCmd) &&
		p.setInt   (L"CharTable",	codePage_);
	return res;
}


std::vector<unsigned char> MakeCryptPassword(const std::wstring &src)
{  
	std::vector<unsigned char>::iterator itr;
	std::wstring::const_iterator it;
	clock_t	Random = clock();
	unsigned char	XorMask;

	std::vector<unsigned char> crypt;

	if(src.empty())
		return crypt;

	crypt.resize(2+src.size()*2);
	itr = crypt.begin();
	*itr = (unsigned char)((Random & 0xFF) |0x80);
	++itr;
	*itr = (unsigned char)((Random>>8)|0x80);
	++itr;
	XorMask  = (crypt[0]^crypt[1]) | 80;
	it		= src.begin();
	while(it != src.end())
	{
		*itr		= lobyte(*it) ^ XorMask;
		*(itr+1)	= hibyte(*it) ^ *itr;
		itr += 2; ++it;
	}
	return crypt;
}

std::wstring DecryptPassword_old(const std::vector<unsigned char> &crypt)
{  
	BYTE XorMask = (crypt[0]^crypt[1]) | 80;
	int  n;
	std::wstring res;
	const int maxPwdLen = 150;  //max crypted pwd length

	if (crypt.size() > 2)
		for(n = 2; n < maxPwdLen; n++)
		{
			wchar_t c = crypt[n] ^ XorMask;
			if(c == 0 || c == XorMask)
				break;
			res.push_back(c);
		}
	return res;
}

std::wstring DecryptPassword(const std::vector<unsigned char> &crypt)
{
	std::wstring uncrypted;
	if(crypt.size() > 3)
	{
		uncrypted.reserve((crypt.size()-2)/2);
		unsigned char XorMask = (crypt[0]^crypt[1]) | 80;
		std::vector<unsigned char>::const_iterator itr = crypt.begin()+2;
		while(itr != crypt.end())
		{
			int lo = *itr ^ XorMask;
			int hi = *(itr+1) ^ *itr;
			itr += 2;
			uncrypted.push_back(static_cast<wchar_t>(lo + (hi << 8)));
		}
	}
	return uncrypted;
}

