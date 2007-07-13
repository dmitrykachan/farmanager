#ifndef __MY_JM
#define __MY_JM

class FTP;


const int defaultCodePage = CP_OEMCP;

extern const wchar_t* ftp_sig;
extern const wchar_t* http_sig;
extern const wchar_t* ftp_sig_end;
extern const wchar_t* http_sig_end;


//------------------------------------------------------------------------
//ftp_FTPHost.cpp
struct FTPHost : public FTPHostPlugin
{
	std::wstring	username_;
	std::wstring	password_;
	std::wstring	hostname_;
	std::wstring	directory_;
	size_t			port_;
	std::wstring	Host_;

//Reg
	bool			Folder;
	std::wstring	hostDescription_;
	FILETIME		LastWrite;
	int				codePage_;

	WinAPI::RegKey	regKey_;

	void			Init( void );
	void			Assign(const FTPHost* p);
	void			MkUrl(std::wstring &str, const std::wstring &Path, const std::wstring &nm, bool sPwd = false);
	std::wstring	MkINIFile(const std::wstring &path, const std::wstring &destPath) const;
	bool			Cmp(FTPHost* p);
	BOOL			CmpConnected( FTPHost* p );
	void			FindFreeKey(WinAPI::RegKey &r);

	bool			SetHostName(const std::wstring& hnm, const std::wstring &usr, const std::wstring &pwd);
	bool			parseFtpUrl(const std::wstring &s);

	bool			Read(const std::wstring &keyName);
	bool			Write(FTP* ftp, const std::wstring &nm);
	bool			ReadINI(const std::wstring &nm);
	bool			WriteINI(const std::wstring &nm) const;

	static bool		CheckHost(const std::wstring &path, const std::wstring &name);
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

#endif
