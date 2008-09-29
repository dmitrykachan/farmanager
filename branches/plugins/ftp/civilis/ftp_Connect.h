#ifndef __FAR_PLUGIN_CONNECT_CONNECTIONS
#define __FAR_PLUGIN_CONNECT_CONNECTIONS


#include "ftp_Lang.h"
#include "ftp_Cfg.h"
#include "ftp_var.h"
#include "FTPClient.h"
#include "progress.h"

class FTPProgress;

const wchar_t quoteMark = L'\x1';

class CommandsList;

class Connection
{
public:
	enum Result
	{
		Done, Error = -2, Cancel = -3
	};
	void		setHost(const FtpHostPtr &host);
	FtpHostPtr& getHost();
	const FtpHostPtr& getHost() const;


    int			empty_(struct fd_set *mask, int sec);
    Result		getit(const std::wstring &remote, const std::wstring& local, int restartit, wchar_t *mode, FTPProgress& trafficInfo);
	void		getline(std::string &str, DWORD tm = 0);
    bool		initDataConnection();
	error_code	login(const std::wstring& user, const std::wstring& password, const std::wstring& accout = L"");
    void		lostpeer();

    Result		recvrequestINT(const std::wstring& cmd, const std::wstring &local, const std::wstring &remote, wchar_t *mode, FTPProgress& trafficInfo);
	Result		recvrequest(const std::wstring& cmd, const std::wstring &local, const std::wstring &remote, wchar_t *mode, FTPProgress& trafficInfo);
    Result		reget(const std::wstring &remote, const std::wstring& local);
	Result		sendrequestINT(const std::wstring &cmd, const std::wstring &local, const std::wstring &remote, bool asciiMode, FTPProgress& trafficInfo);
	Result		sendrequest(const std::wstring &cmd, const std::wstring &local, const std::wstring &remote, bool asciiMode, FTPProgress& trafficInfo);
	Result		setpeer_(const std::wstring& site, size_t port, std::wstring &user, std::wstring &pwd, bool reaskPassword);


	Result		account(const std::vector<std::wstring> &params);
	Result		appended(const std::wstring &local, const std::wstring &remote);
	Result		setascii();
    Result		setbinary();
	Result		settype(ftTypes Mode);
	Result		cd(const std::wstring &dir);
	Result		cdup();
	Result		chmod(const std::wstring& file, const std::wstring &mode);
	Result		disconnect();
	Result		deleteFile(const std::wstring &filename);
	Result		ls(const std::wstring &path);
	Result		get(const std::wstring &remote, const std::wstring& local);
	Result		nlist(const std::wstring &path);
	Result		idle(const std::wstring &time);
	Result		makedir(const std::wstring &dir);
	Result		modtime(const std::wstring& file);
	Result		newer(const std::wstring &remote, const std::wstring& local);
	Result		setport();
	Result		setpeer(const std::wstring& site, const std::wstring& port, const std::wstring &user, const std::wstring &pwd);
	Result		put(const std::wstring &local, const std::wstring& remote);
	Result		pwd();
	Result		quote(const std::vector<std::wstring> &params);
	Result		rmthelp(const std::wstring &c);
	Result		rmtstatus(const std::wstring &c);
	Result		renamefile(const std::wstring& oldfilename, const std::wstring& newfilename);
	Result		reset();
	Result		restart(const std::wstring& point);
	Result		removedir(const std::wstring &dir);
	Result		setrunique();
	Result		setsunique();
	Result		site(const std::vector<std::wstring>& params);
	Result		sizecmd(const std::wstring& filename);
	Result		syst();
	Result		do_umask(const std::wstring &mask);
	Result		user(const std::wstring& usr, const std::wstring& pwd, const std::wstring &accountcmd);

    void		ResetOutput();
    void		AddOutput(const char *data, int size);

	std::wstring getSystemInfo();
	__int64		fileSize(const std::wstring &filename);

	void		setBreakable(bool value);
	bool		getBreakable();

	void		keepAlive();

    BOOL          brk_flag;
    __int64       restart_point;

    bool			sendport_;   /* use PORT cmd for each data connection */
    bool           sunique;    /* store files on server with unique name */
    bool           runique;    /* store local files with unique name */

    ftTypes       type_;          /* file transfer type */
    
	std::wstring			userName_;
	std::wstring			userPassword_;
	std::wstring			output_;
    bool          breakable_;

	bool	isConnected() const
	{
		return ftpclient_.isConnected();
	}

	const std::wstring& getCurrentDirectory() const;
	void				setCurrentDirectory(const std::wstring& curdir);
	const std::string&  getReply() const
	{
		return ftpclient_.getReply();
	}
	std::wstring  getReplyW() const
	{
		return FromOEM(ftpclient_.getReply());
	}

	FTP*	getPlugin()
	{
		return plugin_;
	}

	void setPlugin(FTP* ftp)
	{
		plugin_ = ftp;
	}

	WinAPI::Stopwatch& getKeepStopWatch()
	{
		return keepAliveStopWatch_;
	}

	bool isResumeSupport() const
	{
		return resumeSupport_;
	}

	int getRetryCount() const
	{
		return retryCount_;
	}

	void setRetryCount(int count)
	{
		retryCount_ = count;
	}

private:
	boost::shared_ptr<FTPHost>	pHost_;

	std::wstring	systemInfo_;

	static	int	getFirstDigit(int code)
	{
		code /= 100;
		BOOST_ASSERT(code >= 1 && code <= 9);
		return code;
	}

	std::wstring		lastMsg_;
	std::wstring		startReply_;
	std::vector<std::wstring> cmdBuff_;

	std::vector<std::wstring> cmdMsg_;
	WinAPI::Stopwatch	idleStopWatch_;

	static std::auto_ptr<CommandsList>	commandsList_;
	WinAPI::Stopwatch		keepAliveStopWatch_;

	FTP*			plugin_;
	bool			resumeSupport_;
	int				retryCount_;
	std::wstring	curdir_;

	FTPClient		ftpclient_;
	std::wstring	currentProcessMessage_;

public:
//	FTPProgress		trafficInfo_;
    bool			CmdVisible;
    bool           IOCallback;

private:
	void			pushCmdLine(const std::wstring &src, CMDOutputDir direction);
	int				ConnectMessage(std::wstring message, bool messageTime,
									const std::wstring &title, const std::wstring &bottom, 
									bool showcmds, bool error,
									int button1 = MNone__, int button2 = MNone__, int button3 = MNone__);
	Result			commandOem(const std::string &str, bool isQuit = false);

  public:
    Connection();
    ~Connection();

    void           InitData(const boost::shared_ptr<FTPHost> &p);
    void           InitCmdBuff();
	Result			Init(FTP* ftp);
	Result			command(const std::wstring &str, bool isQuit = false);

	Result			ProcessCommand(const std::wstring& line);
    void           CheckResume( void );
    void           AbortAllRequest(int brkFlag);

	std::wstring	GetStartReply() const		{ return startReply_; }

	std::wstring	FromOEM(const std::string &str) const;
	std::string		toOEM(const std::wstring &str) const;

	void			AddCmdLineOem(const std::string& str, CMDOutputDir direction = ldOut);
	void			AddCmdLine(const std::wstring &str, CMDOutputDir direction = ldOut);

	int				ConnectMessage(int Msg = MNone__, std::wstring title = L"", bool error = false, int BtnMsg = MNone__,int btn1 = MNone__, int btn2 = MNone__ );
	int				ConnectMessage(std::wstring msg, std::wstring title = L"", bool error = false, int BtnMsg = MNone__,int btn1 = MNone__, int btn2 = MNone__ );
	bool			ConnectMessageTimeout(int Msg, const std::wstring& hostname, int BtnMsg);

	error_code      downloadToBufferHandler(FTPProgress& trafficInfo, const std::vector<char> &buffer, size_t bytes_transferred);
	error_code		downloadToFile(HANDLE file, FTPProgress& trafficInfo, const std::vector<char> &buffer, size_t bytes_transferred);
	error_code		uploadFromFile(HANDLE file, FTPProgress& trafficInfo, std::vector<char> &buffer, size_t* readbytes);
	bool			displayWaitingMessage(size_t n);

};

class Command
{
public:
	typedef Connection::Result (Connection::* Func0)();
	typedef Connection::Result (Connection::* Func1)(const std::wstring& s1);
	typedef Connection::Result (Connection::* Func2)(const std::wstring& s1, const std::wstring& s2);
	typedef Connection::Result (Connection::* Func3)(const std::wstring& s1, const std::wstring& s2, const std::wstring& s3);
	typedef Connection::Result (Connection::* Func4)(const std::wstring& s1, const std::wstring& s2, const std::wstring& s3, const std::wstring& s4);
	typedef Connection::Result (Connection::* Func5)(const std::wstring& s1, const std::wstring& s2, const std::wstring& s3, const std::wstring& s4, const std::wstring& s5);
	typedef Connection::Result (Connection::* FuncV)(const std::vector<std::wstring> &params);

	Command()
		: argsCount_(NotInitialized)
	{}

	Command(Func0& f, bool mustConnect = true, bool proxy = true)
		: mustConnect_(mustConnect), proxy_(proxy),  argsCount_(0)
	{
		func.func0_ = f;
	}

	Command(Func1& f, bool mustConnect = true, bool proxy = true)
		: mustConnect_(mustConnect), proxy_(proxy),  argsCount_(1)
	{
		func.func1_ = f;
	}

	Command(Func2& f, bool mustConnect = true, bool proxy = true)
		: mustConnect_(mustConnect), proxy_(proxy),  argsCount_(2)
	{
		func.func2_ = f;
	}

	Command(Func3& f, bool mustConnect = true, bool proxy = true)
		: mustConnect_(mustConnect), proxy_(proxy),  argsCount_(3)
	{
		func.func3_ = f;
	}

	Command(Func4& f, bool mustConnect = true, bool proxy = true)
		: mustConnect_(mustConnect), proxy_(proxy),  argsCount_(4)
	{
		func.func4_ = f;
	}

	Command(Func5& f, bool mustConnect = true, bool proxy = true)
		: mustConnect_(mustConnect), proxy_(proxy),  argsCount_(5)
	{
		func.func5_ = f;
	}

	Command(FuncV& f, bool mustConnect = true, bool proxy = true)
		: mustConnect_(mustConnect), proxy_(proxy),  argsCount_(AnyParameters)
	{
		func.funcv_ = f;
	}

	bool getMustConnect() const
	{
		BOOST_ASSERT(argsCount_ != NotInitialized);
		return mustConnect_;
	}

	bool getProxy() const
	{
		BOOST_ASSERT(argsCount_ != NotInitialized);
		return proxy_;
	}

	Connection::Result execute(Connection &conn, const std::vector<std::wstring>& params)
	{
		BOOST_ASSERT(argsCount_ != NotInitialized);
		switch(argsCount_)
		{
		case 0: return (conn.*func.func0_)();
		case 1: return (conn.*func.func1_)(params[1]);
		case 2: return (conn.*func.func2_)(params[1], params[2]);
		case 3: return (conn.*func.func3_)(params[1], params[2], params[3]);
		case 4: return (conn.*func.func4_)(params[1], params[2], params[3], params[4]);
		case 5: return (conn.*func.func5_)(params[1], params[2], params[3], params[4], params[5]);
		case AnyParameters:
			return (conn.*func.funcv_)(params);
		default:
			BOOST_ASSERT(0);
			return Connection::Error;
		}
	}

private:
	enum 
	{
		AnyParameters = -1, NotInitialized = -2
	};
	bool			mustConnect_;
	bool			proxy_;
	int				argsCount_;
	union			Func
	{
		Func0		func0_;
		Func1		func1_;
		Func2		func2_;
		Func3		func3_;
		Func4		func4_;
		Func5		func5_;
		FuncV		funcv_;
	}				func;
};

class CommandsList
{
public:
	CommandsList()
	{
		init();
	}

	bool find(const std::wstring &name, Command& cmd)
	{
		const_iterator itr = list_.find(name);
		if(itr != list_.end())
		{
			cmd = itr->second;
			return true;
		}
		return false;
	}

private:
	void		add(const std::wstring name, const Command& c)
	{
		list_.insert(std::make_pair(name, c));
	}

	template<typename Func>
	void		add(const std::wstring name, Func f, bool mustConnect = true, bool proxy = true)
	{
		list_.insert(std::make_pair(name, Command(f, mustConnect, proxy)));
	}
	void		init();
	typedef std::map<std::wstring, Command> List;
	typedef List::const_iterator	const_iterator;
	typedef List::iterator			iterator;
	List	list_;
};

#endif
