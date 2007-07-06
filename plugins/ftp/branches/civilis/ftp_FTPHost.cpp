#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

#define FTPHOST_DVERSION                 2740
#define FTPHOST_DVERSION_STR			 "1.2740"
#define FTPHOST_DVERSION_WSTR			 L"1.2740"
#define FTPHOST_DVERSION_SERVERTYPE      (FTPHOST_DVERSION + sizeof(WORD))
#define FTPHOST_DVERSION_SERVERTYPE_CODE (FTPHOST_DVERSION_SERVERTYPE + sizeof(BOOL))

#define FTPHOST_VERSION_LAST             FTPHOST_DVERSION_SERVERTYPE_CODE

#define FTPHOST_VERSION      Message("1.%d", FTPHOST_VERSION_LAST )

#undef PROC
#undef Log
#if 0
  #define Log(v)  INProc::Say v
  #define PROC(v) INProc _inp v ;
#else
  #define PROC(v)
  #define Log(v)
#endif

//---------------------------------------------------------------------------------
typedef bool (*HexToPassword_cb)(const std::wstring& hexstr, std::wstring &password);

BYTE HexToNum(wchar_t Hex)
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
	return (n < 10)? '0' + n : 'a' + n - 10;
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
		password.push_back(n);
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
		pwd.push_back(n);
		++itr;
	}

	password = DecryptPassword(pwd);

	return TRUE;
}

void FTPHost::Init( void )
{
	Log(( "FTPHost::Init %p",this ));

	Size		= sizeof(*this);

	Folder		= false;
	hostname_	= L"";
	username_	= L"";
	password_	= L"";
	hostDescription_ = L"";
	codePage_	= CP_OEMCP;

	Host_		= L"";
	directory_	= L"";

	AskLogin = PassiveMode = UseFirewall = AsciiMode = ExtCmdView = ProcessCmd =
		CodeCmd = ExtList = FFDup = UndupFF = DecodeCmdLine = SendAllo = UseStartSpaces = false;

	listCMD_	= L"LIST -la";

	ExtCmdView   = g_manager.opt.ExtCmdView;
	IOBuffSize   = g_manager.opt.IOBuffSize;
	PassiveMode  = g_manager.opt.PassiveMode;
	ProcessCmd   = g_manager.opt.ProcessCmd;
	UseFirewall  = g_manager.opt.firewall[0] != 0;
	FFDup        = g_manager.opt.FFDup;
	UndupFF      = g_manager.opt.UndupFF;
	DecodeCmdLine= TRUE;
	serverType_   = ServerList::instance().create(L"");
}

void FTPHost::Assign(const FTPHost* p )
{
	Assert( p );
	Assert( p != this );
	*this = *p;
}

bool FTPHost::Cmp(FTPHost* p)
{
	return regKey_ == p->regKey_;
}

BOOL FTPHost::CmpConnected(FTPHost* p)
{
	return	boost::algorithm::iequals(p->Host_, Host_) &&
			p->username_ == username_ && p->password_ == password_;
}

void FTPHost::MkUrl(std::wstring &str, const std::wstring &Path, const std::wstring &nm, bool sPwd )
{  
	bool defPara =	username_ == L"anonymous" &&
					password_ == g_manager.opt.defaultPassword_ == 0;
	if ( !defPara && !username_.empty() ) 
	{
		str = std::wstring(L"ftp://") + username_;
		if(!password_.empty())
			str += L":" + ((sPwd || IS_FLAG(g_manager.opt.PwdSecurity,SEC_PLAINPWD)) ? 
								password_ : L"");
		str += L"@" + Host_;
	} else
		str = std::wstring(L"ftp://") + Host_;

	if(!Path.empty())
	{
		if ( Path[0] != '/' )
			str += '/';
		str += Path;
	}

	if (!nm.empty()) 
	{
		size_t len = str.size()-1;
		if (len >= 0 && str[len] != '/' && str[len] != '\\' )
			str += L'/';
		str += nm;
	}

	FixFTPSlash( str );
}

class CopyAndCorrectBadCharacters
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

std::wstring FTPHost::MkINIFile(const std::wstring &path,
							   const std::wstring &destPath) const
{
	std::wstring destName = destPath;
	if(!path.empty())
	{
		std::wstring::const_iterator itr = path.begin();
		while(itr != path.end() && (*itr == '\\' || *itr == '/'))
			++itr;
		AddEndSlash(destName, '\\');
		// Add from "Hosts\Folder\Item0" "Folder\Item0" part
// TODO		destName += std::wstring(regKey_.begin()+6, regKey_.end());

		// Remove trailing "\Item0"
		size_t m = destName.rfind('\\');
		if(m != std::wstring::npos)
			destName.resize(m);
	}
	AddEndSlash(destName, '\\');

	//Correct bad characters and add possible to DestName
	BOOST_ASSERT(!hostname_.empty());
	std::for_each(hostname_.begin(), 
		hostname_.end(), 
		CopyAndCorrectBadCharacters(destName, Folder? L":/\\\"\'|><^*?" : L":/\\.@\"\'|><^*?"));
	if(!Folder)
	{
		destName += L".ftp";
	}
	return destName;
}

bool FTPHost::CheckHost(const std::wstring &path, const std::wstring &name)
{
// TODO	return FP_CheckRegKey(MkHost(path,name));
	return false;
}

#pragma  warning(disable: 4503)

std::wostream& operator << (std::wostream& os, FTPHost& h)
{
	os << "user: " << h.username_ << "\n";
	os << "pass: " << h.password_ << "\n";
	os << "host: " << h.hostname_ << "\n";
	os << "port: " << h.port_ << "\n";
	os << "dir : " << h.directory_ << "\n";
	return os;
}

bool FTPHost::parseFtpUrl(const std::wstring &s)
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
		opt< seq< c<':'>, 
		mark<pass, star<not_cc<'@', '@'> > > > 
		>, // password
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
	hostname_	= s;

	typedef match_array<std::wstring::const_iterator, count> MC;
	std::wstring::const_iterator itr;
	MC mc;
	if(match<ftpurl>(s.begin(), s.end(), mc))
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


bool FTPHost::SetHostName(const std::wstring& hnm, const std::wstring &usr, const std::wstring &pwd)
{
	parseFtpUrl(hnm);

	if(!usr.empty())
		username_ = usr;
	if(!pwd.empty())
		password_ = pwd;

	return false;
}


void AddPath(std::wstring &buff, const std::wstring &path)
{
    if(!path.empty())
	{
		AddEndSlash(buff, L'\\');
		std::wstring::const_iterator itr = path.begin();
		while(*itr == L'\\' || *itr == L'/' ) ++itr;
		buff += std::wstring(itr, path.end());
    }
    DelEndSlash(buff, '\\');
}

void FTPHost::FindFreeKey(WinAPI::RegKey &regKey)
{
	std::wstring key;
	int  n;
	WinAPI::RegKey r;

	while(true)
	{
		key = L"Item" + boost::lexical_cast<std::wstring>(n);
		if(!regKey.isSubKeyExists(key.c_str()))
			break;
		++n;
	}
	regKey.open(regKey, key.c_str());
}
//---------------------------------------------------------------------------------
bool FTPHost::Read(const std::wstring &keyName)
{  
	PROC(( "FTPHost::Read","%s",nm ));
	std::wstring	usr;
	std::vector<unsigned char>	psw;
	std::wstring	hnm;
	std::wstring	pwd;

	Folder = regKey_.get(L"Folder", 0);
	if(Folder) 
	{
		username_	= L"";
		password_	= L"";
		directory_	= L"";
		hostDescription_ = regKey_.get(L"Description", L"");
		Host_		= keyName;
		hostname_	= Host_;
		return true;
	}

	hnm = regKey_.get(L"HostName", L"");
	if(hnm.empty())
		return false;

	usr = regKey_.get(L"User", L"");
	regKey_.get(L"Password", &psw);

	if(!psw.empty())
		pwd = DecryptPassword_old(psw);

	SetHostName(hnm, usr, pwd);

	hostDescription_	= regKey_.get(L"Description",	L"");
	codePage_			= regKey_.get(L"Table",			defaultCodePage);
	ProcessCmd			= regKey_.get(L"ProcessCmd",		TRUE);
	AskLogin			= regKey_.get(L"AskLogin",			FALSE);
	PassiveMode			= regKey_.get(L"PassiveMode",		FALSE);
	UseFirewall			= regKey_.get(L"UseFirewall",		FALSE);
	AsciiMode			= regKey_.get(L"AsciiMode",			FALSE);
	ExtCmdView			= regKey_.get(L"ExtCmdView",		g_manager.opt.ExtCmdView);
	ExtList				= regKey_.get(L"ExtList",			FALSE);
	std::wstring serverName = regKey_.get(L"ServerType", L"");
	serverType_			= ServerList::instance().create(serverName);
	CodeCmd				= regKey_.get(L"CodeCmd",			TRUE );
	listCMD_			= regKey_.get(L"ListCMD",			L"LIST -la");
	IOBuffSize			= regKey_.get(L"IOBuffSize",        g_manager.opt.IOBuffSize);
	FFDup				= regKey_.get(L"FFDup",             g_manager.opt.FFDup);
	UndupFF				= regKey_.get(L"UndupFF",           g_manager.opt.UndupFF);
	DecodeCmdLine		= regKey_.get(L"DecodeCmdLine",     TRUE);
	SendAllo			= regKey_.get(L"SendAllo",          FALSE);
	UseStartSpaces		= regKey_.get(L"UseStartSpaces",	TRUE);

	IOBuffSize = std::max(FTR_MINBUFFSIZE,IOBuffSize);

	return TRUE;
}

bool FTPHost::Write(FTP* ftp, const std::wstring &nm)
{  
	PROC(( "FTPHost::Write","%s",nm ))

	Log(( "RegKey=[%s]", host.c_str() ));


	std::wstring key = HostRegName + nm;

	BOOST_ASSERT(0 && "TO check");
	
	WinAPI::RegKey r(g_manager.getRegKey(), key.c_str());
	r.deleteSubKey(Host_.c_str());

	if(Folder)
		r.open(r, hostname_.c_str());
	else
	{
		FindFreeKey(r);
	}
	
	if ( !Folder )
	{
		std::vector<unsigned char> psw;

		if(!password_.empty())
			psw = MakeCryptPassword(password_);

		std::wstring s = serverType_->getName();

		regKey_.set(L"HostName",		hostname_);
		regKey_.set(L"User",			username_);
		regKey_.set(L"Passwordw",		psw);
		regKey_.set(L"Table",			codePage_);
		regKey_.set(L"AskLogin",		AskLogin   );
		regKey_.set(L"PassiveMode",		PassiveMode);
		regKey_.set(L"UseFirewall",		UseFirewall);
		regKey_.set(L"AsciiMode",		AsciiMode  );
		regKey_.set(L"ExtCmdView",		ExtCmdView );
		regKey_.set(L"ExtList",			ExtList );
		regKey_.set(L"ServerType",		serverType_->getName());
		regKey_.set(L"ListCMD",			listCMD_);
		regKey_.set(L"ProcessCmd",		ProcessCmd );
		regKey_.set(L"CodeCmd",			CodeCmd );
		regKey_.set(L"IOBuffSize",		IOBuffSize );
		regKey_.set(L"FFDup",			FFDup);
		regKey_.set(L"UndupFF",			UndupFF);
		regKey_.set(L"DecodeCmdLine",	DecodeCmdLine);
		regKey_.set(L"SendAllo",		SendAllo);
		regKey_.set(L"UseStartSpaces",	UseStartSpaces);
	}
	regKey_.set(L"Description", hostDescription_);
	regKey_.set(L"Folder",		Folder);

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
	std::wstring hst;
	std::wstring usr;
	std::wstring pwd;


	Init();

	Profile p(nm, L"FarFTP");

	BOOST_ASSERT(0 && "not realized");
	HexToPassword_cb DecodeProc = NULL;
	if(p.getString(L"Version", L"") == FTPHOST_DVERSION_WSTR)
	{
		DecodeProc = HexToPassword_2740;
	} else
		DecodeProc = HexToPassword_OLD;

	hst = p.getString(L"Url", L"");
	if(hst.empty())
		return false;
	usr = p.getString(L"User", L"");
	hex = p.getString(L"Password", L"");

	if(!DecodeProc(hex,pwd) || !SetHostName(hst,usr,pwd))
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
	CodeCmd			= p.getInt(L"CodeCmd", TRUE);
	listCMD_		= p.getString(L"ListCMD", listCMD_);
	IOBuffSize		= p.getInt(L"IOBuffSize", g_manager.opt.IOBuffSize);
	FFDup			= p.getInt(L"FFDup", g_manager.opt.FFDup);
	UndupFF			= p.getInt(L"UndupFF", g_manager.opt.UndupFF);
	DecodeCmdLine	= p.getInt(L"DecodeCmdLine", TRUE);
	SendAllo		= p.getInt(L"SendAllo", FALSE);
	UseStartSpaces	= p.getInt(L"UseStartSpaces", true);
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

	std::wstring pass = L"Test“ÂÒÚﬂ";
	std::vector<unsigned char> pwd = MakeCryptPassword(pass);
	std::wstring unpwd = DecryptPassword(pwd);
	BOOST_ASSERT(pass == unpwd);

	//CreateDirectory
	boost::filesystem::wpath path = nm;
	boost::filesystem::create_directories(path.branch_path());

	Profile p(nm, L"FarFTP");

	const wchar_t* filename = nm.c_str();
	std::wstring hexPassword = PasswordToHex(password_);
	res =
		p.setString(L"Version",		FTPHOST_DVERSION_WSTR) &&
		p.setString(L"Url",			hostname_) &&
		p.setString(L"User",		username_) &&
		p.setString(L"Password",	hexPassword) &&
		p.setString(L"Description",	hostDescription_) &&
		p.setInt   (L"AskLogin",	AskLogin) &&
		p.setInt   (L"AsciiMode",	AsciiMode) &&
		p.setInt   (L"PassiveMode",	PassiveMode) &&
		p.setInt   (L"UseFirewall",	UseFirewall) &&
		p.setInt   (L"ExtCmdView",	ExtCmdView) &&
		p.setInt   (L"ExtList",		ExtList) &&
		p.setString(L"ServerType",	serverType_->getName()) &&
		p.setInt   (L"CodeCmd",		CodeCmd) &&
		p.setString(L"ListCMD",		listCMD_) &&
		p.setInt   (L"IOBuffSize",	IOBuffSize) &&
		p.setInt   (L"FFDup",		FFDup) &&
		p.setInt   (L"UndupFF",		UndupFF) &&
		p.setInt   (L"DecodeCmdLine",	DecodeCmdLine) &&
		p.setInt   (L"SendAllo",	SendAllo) &&
		p.setInt   (L"UseStartSpaces",UseStartSpaces) &&
		p.setInt   (L"ProcessCmd",	ProcessCmd) &&
		p.setInt   (L"CharTable",	codePage_);
	return res;
}


std::vector<unsigned char> MakeCryptPassword(const std::wstring &src)
{  
	BOOST_ASSERT("TEST");
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

	if (crypt.size() > 2)
		for(n = 2; n < FTP_PWD_LEN; n++)
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
			uncrypted.push_back(lo + (hi << 8));
		}
	}
	return uncrypted;
}