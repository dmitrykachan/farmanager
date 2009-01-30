#ifndef __MY_JM
#define __MY_JM

class FTP;
#include "servertype.h"

const int defaultCodePage = CP_OEMCP;

struct FTPUrl_
{
	std::wstring	username_;
	std::wstring	password_;
	std::wstring	directory_;
	size_t			port_;
	std::wstring	Host_;

	FTPUrl_()
		: port_(0)
	{};

	void clear()
	{
		username_ = password_ =  directory_ = Host_ = L"";
		port_ = 0;
	}

	bool compare(FTPUrl_ &url)
	{
		return	boost::algorithm::iequals(Host_, url.Host_) &&
			username_ == url.username_ && password_ == url.password_;
	}

	bool parse(const std::wstring& s);
	std::wstring toString(bool insertPassword = false, bool useServiceName = true) const;
	std::wstring getUrl(bool useServerName = false) const;
};



//------------------------------------------------------------------------
//ftp_FTPHost.cpp
struct FTPHost: public boost::noncopyable
{
	BOOL    AskLogin;
	BOOL    PassiveMode;
	BOOL    UseFirewall;
	BOOL    AsciiMode;
	BOOL    ExtCmdView;
	BOOL    ProcessCmd;
	int		IOBuffSize;                    // Size of buffer used to send|recv data
	BOOL    ExtList;                       // Use extended list command
	std::wstring listCMD_;                 // Extended list command
	ServerTypePtr   serverType_;           // Type of server
	BOOL    FFDup;                         // Duplicate FF char on string sent to server
	BOOL    UndupFF;                       // Remove FF duplicate from PWD
	BOOL    SendAllo;                      // Send allo before upload

	FTPUrl_			url_;
	bool			Folder;
	std::wstring	hostDescription_;
	FILETIME		lastEditTime_;
	int				codePage_;

	void			Init();
	void			MkUrl(std::wstring &str, std::wstring Path, const std::wstring &nm);
	void			MkINIFile();
	WinAPI::RegKey	findFreeKey(const std::wstring &path, std::wstring &name);

	bool			SetHostName(const std::wstring& hnm, const std::wstring &usr, const std::wstring &pwd);

	bool			Read(const std::wstring &keyName, const std::wstring &path);
	bool			Write(/*const std::wstring &path*/);
	bool			ReadINI(const std::wstring &nm);
	bool			WriteINI(const std::wstring &nm) const;

	const std::wstring&	getRegName() const
	{
		return regName_;
	}

	const std::wstring&	getRegPath() const
	{
		return regKeyPath_;
	}

	void			setRegName(const std::wstring &name)
	{
		regName_ = name;
	}

	void			setRegPath(const std::wstring& path)
	{
		regKeyPath_ = path;
	}

	const std::wstring&	getIniFilename() const
	{
		return iniFilename_;
	}

private:
	std::wstring	regKeyPath_;
	std::wstring	iniFilename_;
	std::wstring	regName_;
};

typedef boost::shared_ptr<FTPHost> FtpHostPtr;

class FTPCmdBlock: boost::noncopyable
{
    int   hVis;  /*TRUE, FALSE, -1*/
    FTP  *handle_;
public:
    FTPCmdBlock(FTP *c,int block = -1);
    ~FTPCmdBlock();

    void Block(int block);
    void Reset();
};

const wchar_t NET_SLASH = L'/';
const wchar_t LOC_SLASH = L'\\';

namespace
{
	const wchar_t* NET_SLASH_STR = L"/";
	const wchar_t* LOC_SLASH_STR = L"\\";
}


#endif
