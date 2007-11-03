#ifndef __FAR_PLUGIN_CONNECT_CONNECTIONS
#define __FAR_PLUGIN_CONNECT_CONNECTIONS


#include "ftp_Lang.h"
#include "ftp_Cfg.h"
#include "ftp_var.h"
#include "socket.h"

class Connection;
class FTPProgress;

const wchar_t quoteMark = L'\x1';

struct ConnectionState
{

  BOOL      Inited;
  int       Blocked;
  int       RetryCount;
  int       Passive;
  HANDLE    Object;

  ConnectionState( void ) { Inited = FALSE; }
};

class Command
{
public:
	typedef bool (Connection::* Func0)();
	typedef bool (Connection::* Func1)(const std::wstring& s1);
	typedef bool (Connection::* Func2)(const std::wstring& s1, const std::wstring& s2);
	typedef bool (Connection::* Func3)(const std::wstring& s1, const std::wstring& s2, const std::wstring& s3);
	typedef bool (Connection::* Func4)(const std::wstring& s1, const std::wstring& s2, const std::wstring& s3, const std::wstring& s4);
	typedef bool (Connection::* Func5)(const std::wstring& s1, const std::wstring& s2, const std::wstring& s3, const std::wstring& s4, const std::wstring& s5);
	typedef bool (Connection::* FuncV)(const std::vector<std::wstring> &params);

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

	int execute(Connection &conn, const std::vector<std::wstring>& params)
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
			return RPL_ERROR;
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



class Connection
{
public:
	void		setHost(FTPHost* host);
	FTPHost&	getHost();
	const FTPHost& getHost() const;

    bool		account(const std::vector<std::wstring> &params);
    bool		dataconn(in_addr data_addr, int port);
    bool		do_umask(const std::wstring &mask);
    int			empty_(struct fd_set *mask, int sec);
    bool		getit(const std::wstring &remote, const std::wstring& local, int restartit, wchar_t *mode);
    int			getreply(DWORD tm = 0);
	void		getline(std::string &str, DWORD tm = 0);
	bool		hookup(const std::wstring& host, int port);
    bool		initDataConnection(in_addr &data_addr, int &port);
	bool		login(const std::wstring& user, const std::wstring& password, const std::wstring& accout = L"");
    void		lostpeer();
	bool		makedir(const std::wstring &dir);
	bool		doport(in_addr data_addr, int port);
	bool		modtime(const std::wstring& file);
    bool		newer(const std::wstring &remote, const std::wstring& local);
	bool		put(const std::wstring &local, const std::wstring& remote);
    bool		pwd();
    bool		quit();
	bool		idle(const std::wstring &time);

    bool		quote(const std::vector<std::wstring> &params);
    int			recvrequestINT(const std::wstring& cmd, const std::wstring &local, const std::wstring &remote, wchar_t *mode );
	int			recvrequest(const std::wstring& cmd, const std::wstring &local, const std::wstring &remote, wchar_t *mode );
    bool		reget(const std::wstring &remote, const std::wstring& local);
	bool		removedir(const std::wstring &dir);
	bool		renamefile(const std::wstring& oldfilename, const std::wstring& newfilename);
    bool		reset();
    bool		restart(const std::wstring& point);
    bool		rmthelp(const std::wstring &c);
    bool		rmtstatus(const std::wstring &c);
	void		sendrequestINT(const std::wstring &cmd, const std::wstring &local, const std::wstring &remote);
	void		sendrequest(const std::wstring &cmd, const std::wstring &local, const std::wstring &remote);
    bool		setpeer(const std::wstring& site, const std::wstring& port, const std::wstring &user, const std::wstring &pwd);
	bool		setpeer_(const std::wstring& site, size_t port, const std::wstring &user, const std::wstring &pwd);

	static bool	startupWSA();
	static void cleanupWSA();

    bool		setport();
    bool		setrunique();
    bool		setsunique();

	bool		appended(const std::wstring &local, const std::wstring &remote);
	bool		setascii();
    bool		setbinary();
	bool		cd(const std::wstring &dir);
	bool		cdup();
	bool		chmod(const std::wstring& file, const std::wstring &mode);
	bool		disconnect();
	bool		deleteFile(const std::wstring &filename);
	bool		ls(const std::wstring &path);
	bool		get(const std::wstring &remote, const std::wstring& local);
	bool		nlist(const std::wstring &path);

    bool		setebcdic();
    bool		settype(ftTypes Mode);

    bool		site(const std::vector<std::wstring>& params);
	bool		sizecmd(const std::wstring& filename);
    bool		syst();
	bool		user(const std::wstring& usr, const std::wstring& pwd, const std::wstring &accountcmd);

    bool		nb_connect	(TCPSocket &peer, in_addr &ipaddr, int port, DWORD tm = 0);
	int 		nb_recv		(TCPSocket &peer, char* buf, int len, DWORD tm = 0);
    void		nb_send		(TCPSocket &peer, const char* buf, size_t len, int flags, DWORD tm = 0);
	void		nb_accept	(TCPSocket& listenPeer, TCPSocket& s, sockaddr_in &from, DWORD tm = 0);


    int			fgetcSocket(TCPSocket &s, DWORD tm = 0);
	void		fputsSocket(TCPSocket &s, const std::string &format);

    int			SetType(int type);
    void		ResetOutput();
    void		AddOutput(BYTE *Data,int Size);


	void		setBreakable(bool value);
	bool		getBreakable();

	bool		keepAlive();

    sockaddr_in   hisctladdr;
    BOOL          brk_flag;
    sockaddr_in   myctladdr;
    __int64       restart_point;
    int           portnum;

    /* Lot's of options... */
    /*
     * Options and other state info.
     */
    bool			sendport_;   /* use PORT cmd for each data connection */
    int           sunique;    /* store files on server with unique name */
    int           runique;    /* store local files with unique name */
	std::wstring	pasv_;   /* passive port for proxy data connection */

    ftTypes       type;          /* file transfer type */
    int           stru;          /* file transfer structure */
    
    int           cpend;         /* flag: if != 0, then pending server reply */

	std::wstring			userName_;
	std::wstring			userPassword_;
	std::wstring			output_;
    int           LastUsedTableNum;
    TCPSocket		data_peer_;
	std::wstring	curdir_;
    int           ErrorCode;
    bool          breakable_;

	bool	cmdLineAlive()
	{
		return isConnected() && cmd_socket_.isCreated();
	}

	bool	isConnected()
	{
		return connected_;
	}

	std::wstring getSystemInfo() const
	{
		return systemInfo_;
	}

	void setSystemInfo(const std::wstring& info)
	{
		systemInfo_ = info;
	}

	const std::wstring& getCurrentDirectory() const;

	static bool isComplete(int code)
	{
		if(code == RPL_ERROR)
			return false;
		return getFirstDigit(code) == FTP_RESULT_CODE_COMPLETE;
	}

	static bool isContinue(int code)
	{
		if(code == RPL_ERROR)
			return false;
		return getFirstDigit(code) == FTP_RESULT_CODE_CONTINUE;
	}

	static bool isPrelim(int code)
	{
		if(code == RPL_ERROR)
			return false;
		return getFirstDigit(code) == FTP_RESULT_CODE_PRELIM;
	}

	static bool isBadCommand(int code)
	{
		if(code == RPL_ERROR)
			return false;
		return getFirstDigit(code) == FTP_RESULT_CODE_BADCOMMAND;
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
	FTPHost*		pHost_;
	static	bool	socketStartup_;

	TCPSocket		cmd_socket_;
	bool			connected_;
	std::wstring	systemInfo_;

	void	setConnected(bool val = true)
	{
		connected_ = val;
	}

	static	int	getFirstDigit(int code)
	{
		code /= 100;
		BOOST_ASSERT(code >= 1 && code <= 9);
		return code;
	}

	std::wstring		lastMsg_;
	std::wstring		startReply_;
	std::string			replyString_;
	std::deque<std::wstring> cmdBuff_;

	std::vector<std::wstring> cmdMsg_;
	boost::scoped_array<char> IOBuff;
	WinAPI::Stopwatch	idleStopWatch_;

	static CommandsList		commandsList_;
	WinAPI::Stopwatch		keepAliveStopWatch_;

	FTP*			plugin_;
	bool			resumeSupport_;
	int				retryCount_;

public:
    FTPCurrentStates CurrentState;
	std::wstring	DirFile;
	FTPProgress*   TrafficInfo;
    BOOL           CmdVisible;
    bool           IOCallback;

	BOOL           LoginComplete;

private:
    void           InternalError( void );
    void           CloseCmdBuff( void );
	void			pushCmdLine(const std::wstring &src, CMDOutputDir direction);
    bool			SendAbort(TCPSocket &din);
	void			displayWaitingMessage(size_t curTime, size_t timeout);
	int				ConnectMessage(std::wstring message, bool messageTime,
									const std::wstring &title, const std::wstring &bottom, 
									bool showcmds, bool error,
									int button1 = MNone__, int button2 = MNone__, int button3 = MNone__);

  public:
    Connection();
    ~Connection();

    void           InitData( FTPHost* p,int blocked /*=TRUE,FALSE,-1*/ );
    void           InitCmdBuff();
    void           InitIOBuff( void );
	bool           Init(const std::wstring& Host, size_t Port, const std::wstring& User, const std::wstring& Password, FTP* ftp);

	int            command(const std::string &str, bool isQuit = false);
	int            command(const std::wstring &str, bool isQuit = false);
	int            ProcessCommand(const std::wstring& line);
    void           CheckResume( void );
    void           AbortAllRequest(int brkFlag);

	std::string		GetReply() const			{ return replyString_; }
	std::wstring	GetStartReply() const		{ return startReply_; }

	std::wstring	FromOEM(const std::string &str) const;
	std::string		toOEM(const std::wstring &str) const;

    void           GetState( ConnectionState* p );
    void           SetState( ConnectionState* p );

    int				GetErrorCode( void )           { return ErrorCode; }

	void			AddCmdLine(const std::string& str, CMDOutputDir direction = ldOut);
	void			AddCmdLine(const std::wstring &str, CMDOutputDir direction = ldOut);

	int				ConnectMessage(int Msg = MNone__, std::wstring title = L"", bool error = false, int BtnMsg = MNone__,int btn1 = MNone__, int btn2 = MNone__ );
	int				ConnectMessage(std::wstring msg, std::wstring title = L"", bool error = false, int BtnMsg = MNone__,int btn1 = MNone__, int btn2 = MNone__ );
	bool			ConnectMessageTimeout(int Msg, const std::wstring& hostname, int BtnMsg);
};


#endif
