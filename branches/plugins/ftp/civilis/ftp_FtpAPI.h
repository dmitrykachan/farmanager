#ifndef __FAR_PLUGIN_FTP_IEFTP
#define __FAR_PLUGIN_FTP_IEFTP

bool	FtpChmod(Connection *hConnect, const std::wstring &filename, DWORD Mode);
int    FtpCmdBlock( Connection *hConnect,int block /*TRUE,FALSE,-1*/ );
int    FtpConnectMessage( Connection *hConnect,int Msg, const wchar_t* HostName,int BtnMsg = MNone__,int btn1 = MNone__, int btn2 = MNone__ );
bool   FtpDeleteFile(Connection *hConnect, const std::wstring& filename);
bool   FtpGetFtpDirectory(Connection *Connect);
bool   FtpFindFirstFile(Connection *hConnect, const std::wstring &searchFile, FTPFileInfo* lpFindFileData, bool *ResetCache );
bool   FtpGetFile(Connection *hConnect, const std::wstring& remoteFile, const std::wstring& newFile, bool reget, int asciiMode);
int    FtpGetRetryCount( Connection *hConnect );
bool   FtpIsResume( Connection *hConnect );
bool   FtpPutFile(Connection *Connect, const std::wstring &localFile, std::wstring remoteFile, bool Reput, int AsciiMode);
bool   FtpRemoveDirectory(Connection *hConnect, const std::wstring &dir);
BOOL   FtpRenameFile(Connection *hConnect, const std::wstring &existing, const std::wstring &New);
bool   FtpSetCurrentDirectory(Connection *hConnect, const std::wstring &dir);
void   FtpSetRetryCount( Connection *hConnect,int cn );
void	FtpSystemInfo(Connection *hConnect);
__int64  FtpFileSize(Connection *Connect, const std::wstring &fnm);

class FTPConnectionBreakable
{
private:
	Connection	*hConnect_;
	bool		breakable_;
public:
	FTPConnectionBreakable(Connection *cn, int breakable)
	{
		hConnect_	= cn;
		breakable_	= cn->getBreakable();
		cn->setBreakable(breakable);
		BOOST_LOG(INF, L"ESC: FTPBreakable " << breakable_ << L" -> " << breakable_);
	}
	~FTPConnectionBreakable()
	{
		hConnect_->setBreakable(breakable_);
		BOOST_LOG(INF, L"ESC: FTPBreakable restore to " << breakable_);
	}
};

#endif
