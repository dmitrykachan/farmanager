#ifndef __FAR_PLUGIN_FTP_IEFTP
#define __FAR_PLUGIN_FTP_IEFTP

int    FtpCmdBlock( Connection *hConnect,int block /*TRUE,FALSE,-1*/ );
bool   FtpFindFirstFile(Connection *hConnect, const std::wstring &searchFile, FTPFileInfo* lpFindFileData, bool *ResetCache );
bool   FtpPutFile(Connection *Connect, const std::wstring &localFile, std::wstring remoteFile, bool Reput, int AsciiMode);
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
