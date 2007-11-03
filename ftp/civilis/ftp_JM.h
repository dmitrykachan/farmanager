#ifndef __MY_JM
#define __MY_JM

class FTP;
#include "servertype.h"

const int defaultCodePage = CP_OEMCP;

struct FTPUrl_
{
	std::wstring	username_;
	std::wstring	password_;
	std::wstring	fullhostname_;
	std::wstring	directory_;
	size_t			port_;
	std::wstring	Host_;

	FTPUrl_()
		: port_(0)
	{};

	void clear()
	{
		username_ = password_ = fullhostname_ = directory_ = Host_ = L"";
		port_ = 0;
	}

	bool parse(const std::wstring& s);
	std::wstring toString(bool insertPassword = false) const;
};



//------------------------------------------------------------------------
//ftp_FTPHost.cpp
struct FTPHost// : public FTPHostPlugin
{
	// FTPHostPlugin
	BOOL    AskLogin;
	BOOL    PassiveMode;
	BOOL    UseFirewall;
	BOOL    AsciiMode;
	BOOL    ExtCmdView;
	BOOL    ProcessCmd;
	BOOL    CodeCmd;
	int		IOBuffSize;                    // Size of buffer used to send|recv data
	BOOL    ExtList;                       // Use extended list command
	std::wstring listCMD_;                 // Extended list command
	ServerTypePtr   serverType_;           // Type of server
	BOOL    FFDup;                         // Duplicate FF char on string sent to server
	BOOL    UndupFF;                       // Remove FF duplicate from PWD
	BOOL    DecodeCmdLine;                 // Decode OEM cmd line chars to hosts code page
	BOOL    SendAllo;                      // Send allo before upload
	BOOL    UseStartSpaces;					// Ignore spaces from start of file name

	FTPUrl_			url_;
//Reg
	bool			Folder;
	std::wstring	hostDescription_;
	FILETIME		LastWrite;
	int				codePage_;

	void			Init( void );
	void			Assign(const FTPHost* p);
	void			MkUrl(std::wstring &str, const std::wstring &Path, const std::wstring &nm, bool sPwd = false);
	void			MkINIFile();
	BOOL			CmpConnected( FTPHost* p );
	WinAPI::RegKey	findFreeKey(const std::wstring &path, std::wstring &name);

	bool			SetHostName(const std::wstring& hnm, const std::wstring &usr, const std::wstring &pwd);

	bool Read(const std::wstring &keyName, const std::wstring &path);
	bool			Write(const std::wstring &path);
	bool			ReadINI(const std::wstring &nm);
	bool			WriteINI(const std::wstring &nm) const;

	std::wstring	regName_;
	std::wstring	getRegName() const
	{
		return regName_;
	}

	void			setRegName(const std::wstring &name)
	{
		regName_ = name;
	}

	const std::wstring&	getIniFilename() const
	{
		return iniFilename_;
	}

private:
	std::wstring	regKeyPath_;
	std::wstring	iniFilename_;
};

//------------------------------------------------------------------------
//ftp_FTPBlock.cpp
class FTPCmdBlock
{

    int   hVis;  /*TRUE, FALSE, -1*/
    FTP  *Handle;
  public:
    FTPCmdBlock( FTP *c,int block = -1 );
    ~FTPCmdBlock();

    void Block( int block );
    void Reset( void );
};

const wchar_t NET_SLASH = L'/';
const wchar_t LOC_SLASH = L'\\';

namespace
{
	const wchar_t* NET_SLASH_STR = L"/";
	const wchar_t* LOC_SLASH_STR = L"\\";
}


#endif
