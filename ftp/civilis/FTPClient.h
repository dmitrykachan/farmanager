#pragma once

namespace asio=boost::asio;
namespace ip=boost::asio::ip;
using ip::tcp;

typedef boost::system::error_code error_code;

class Connection;

class FTPReplyCodeCategory: public boost::system::error_category
{
public:
	enum Error
	{
		InvalidReply = 800, SocketError, WriteError, Canceled, Timeout, ReadError
	};

	static const FTPReplyCodeCategory& getCategory()
	{
		static const FTPReplyCodeCategory ftp_reply_code_const;
		return ftp_reply_code_const;
	}

	const char* name() const
	{
		return "FTP Reply Code";
	}

	std::string message(int ev) const
	{
		switch(ev)
		{
		case Canceled:
			return "the task was canceled by user";
		case Timeout:
			return "timeout is occured";
		default:
			return boost::lexical_cast<std::string>(ev);
		}
	}

	boost::system::error_condition default_error_condition(int ev) const
	{
		return boost::system::error_condition(ev, getCategory());
	}
};

class FTPClient: boost::noncopyable
{
public:
	FTPClient(Connection* conn);

	error_code connect(const std::string &host, int port = defaultPort);

	bool isConnected() const
	{
		BOOST_ASSERT(cmdsocket_.is_open() == connected_);
		return connected_;
	}

	void sendAbort();

	static bool isComplete(const error_code& code)
	{
		BOOST_ASSERT(code.category() == FTPReplyCodeCategory::getCategory());
		return code.value() / 100 == 2;
	}
	static bool isPrelim(const error_code& code)
	{
		BOOST_ASSERT(code.category() == FTPReplyCodeCategory::getCategory());
		return code.value() / 100 == 1;
	}

	static bool isCanceled(const error_code& code)
	{
		BOOST_ASSERT(code.category() == FTPReplyCodeCategory::getCategory());
		return code.value() == FTPReplyCodeCategory::Canceled;
	}

	error_code		login(const std::wstring &user, const std::wstring &password, const std::wstring &account);
	error_code		sendCommandOem(const std::string& command, bool waitFullProcess = true);
	error_code		sendCommand(const std::wstring& command, bool waitFullProcess = true, bool dontDisplay = false);
	error_code      readOutput();
	const std::string& getReply() const;
	void			close();
	void            setBufferSize(size_t size);

	bool			prepareDataConnection(bool passive);
	bool			establishActiveDataConnection();
	bool			establishPassiveDataConnection();

	typedef boost::function<error_code (std::vector<char>&, size_t*)> UploadHandler;

	error_code		downloadFile(boost::function<error_code (const std::vector<char>&, size_t)> handler);
	error_code		uploadFile(UploadHandler handler);
	static const int		timerInterval_ = 500;
private:
	

	void addMessage(int msg);

	void handle_resolve(const error_code& err, tcp::resolver::iterator endpoint_iterator);
	void handleConnect(const error_code& err, tcp::resolver::iterator endpoint_iterator);
	void handleTimer(const error_code& e);
	void handleWriteRequest(const error_code& err);
	void handleRead(const error_code& err, size_t bytes_transferred);
	void handleAccept(const error_code& err);
	void handlePassiveConnect(const error_code& err);
	void handleDownload(const error_code& err, size_t bytes_transferred, boost::function<error_code (const std::vector<char>&, size_t)> handler);
	void handleUpload(const error_code& err, size_t bytes_transferred, UploadHandler handler);
	

	void startTimer(const boost::function<void()> &cancelOnTimerExpired, bool reStart = true);
	void read();
	void setLastError(const error_code& err);
	void errorHandler(const error_code& err);
	bool sendPort(tcp::endpoint endpoint);
	bool sendPasv();

	static error_code replyCode(int code)
	{
		return error_code(code, FTPReplyCodeCategory::getCategory());
	}

	Connection* getConnection() const;

	asio::io_service		io_service_;
	tcp::socket				cmdsocket_;
	tcp::socket				datasocket_;
	tcp::acceptor			acceptor_;
	tcp::endpoint			pasvEndpoint_;
	asio::deadline_timer	timer_;
	tcp::resolver			resolver_;
	size_t					timerCount_;
	static const int        timeForTimeout = 3000;
	static const int        defaultPort = 0;
	error_code				lastErrorCode_;
	std::string				message_;
	boost::asio::streambuf	reply_;
	std::string				replyString_;
	boost::function<void()>	cancelOnTimerExpired_;
	bool					connected_;
	Connection* const		connection_;
	std::vector<char>		iobuffer_;
};

