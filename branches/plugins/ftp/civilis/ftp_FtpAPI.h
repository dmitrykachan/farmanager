#ifndef __FAR_PLUGIN_FTP_IEFTP
#define __FAR_PLUGIN_FTP_IEFTP

int    FtpCmdBlock( Connection *hConnect, bool block);

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
