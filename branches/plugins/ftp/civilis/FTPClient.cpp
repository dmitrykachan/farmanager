#include "stdafx.h"
#include "FTPClient.h"
#include "ftp_Lang.h"
#include "ftp_JM.h"
#include "ftp_Connect.h"
#include "ftp_int.h"
#include "utils/uniconverts.h"

static int getCode(const error_code& code)
{
	return code.value();
}

static bool isContinue(const error_code& code)
{
	BOOST_ASSERT(code.category() == FTPReplyCodeCategory::getCategory());
	return code.value() / 100 == 3;
}

static bool isTransient(const error_code& code)
{
	BOOST_ASSERT(code.category() == FTPReplyCodeCategory::getCategory());
	return code.value() / 100 == 4;
}

static bool isBadCommand(const error_code& code)
{
	BOOST_ASSERT(code.category() == FTPReplyCodeCategory::getCategory());
	return code.value() / 100 == 5;
}


FTPClient::FTPClient(Connection* conn)
:	cmdsocket_(io_service_),
	datasocket_(io_service_),
	acceptor_(io_service_),
	timer_(io_service_),
	resolver_(io_service_),
	reply_(2048),
	connected_(false),
	connection_(conn)
{
	setBufferSize(4096);
}

error_code FTPClient::connect(const std::string &host, int port)
{
	tcp::resolver resolver(io_service_);
	std::string portStr;
	if(port != defaultPort)
		portStr = boost::lexical_cast<std::string>(port);
	else
		portStr = "FTP";
	tcp::resolver::query query(host, portStr);

	io_service_.reset();
	cmdsocket_.close();

	addMessage(MResolving);
	lastErrorCode_.clear();
	resolver_.async_resolve(query,
		boost::bind(&FTPClient::handle_resolve, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::iterator));

	startTimer(boost::bind(&tcp::resolver::cancel, &resolver_));
	io_service_.run();

	connected_ = isComplete(lastErrorCode_);
	if(!connected_)
		close();

	return lastErrorCode_;
}

error_code FTPClient::sendCommandOem(const std::string& command, bool waitFullProcess)
{
	lastErrorCode_.clear();
	message_ = command + "\r\n";
	io_service_.reset();

	boost::asio::async_write(cmdsocket_, asio::buffer(message_),
		boost::bind(&FTPClient::handleWriteRequest, this,
		boost::asio::placeholders::error));
	startTimer(boost::bind(&FTPClient::close, this));

	io_service_.run();

	if(waitFullProcess && isPrelim(lastErrorCode_))
	{
		do
		{
			readOutput();
		} while(isPrelim(lastErrorCode_));
	}
	return lastErrorCode_;
}

error_code FTPClient::sendCommand(const std::wstring& command, bool waitFullProcess, bool dontDisplay)
{
	connection_->getKeepStopWatch().reset();
	if(!dontDisplay)
		connection_->AddCmdLine(command, ldOut);
	return sendCommandOem(connection_->toOEM(command), waitFullProcess);
}
	
void FTPClient::addMessage(int /*msg*/)
{
}

void FTPClient::handle_resolve(const error_code& err, tcp::resolver::iterator endpoint_iterator)
{
	timer_.cancel();
	if (!err)
	{
		addMessage(MWaitingForConnect);

		// Attempt a connection to the first endpoint in the list. Each endpoint
		// will be tried until we successfully establish a connection.
		tcp::endpoint endpoint = *endpoint_iterator;
		cmdsocket_.async_connect(endpoint,
			boost::bind(&FTPClient::handleConnect, this,
			boost::asio::placeholders::error, ++endpoint_iterator));
		startTimer(boost::bind(&FTPClient::close, this));
	}
	else
	{
		errorHandler(err);
	}
}

void FTPClient::handleConnect(const error_code& err, tcp::resolver::iterator endpoint_iterator)
{
	timer_.cancel();
	if(!err)
	{
		addMessage(MWaitingForResponse);
		read();
	}
	else if (endpoint_iterator != tcp::resolver::iterator())
	{
		// The connection failed. Try the next endpoint in the list.
		cmdsocket_.close();
		tcp::endpoint endpoint = *endpoint_iterator;
		cmdsocket_.async_connect(endpoint,
			boost::bind(&FTPClient::handleConnect, this,
			boost::asio::placeholders::error, ++endpoint_iterator));
		startTimer(boost::bind(&FTPClient::close, this));
	}
	else
	{
		errorHandler(err);
	}
}

void FTPClient::handleTimer(const error_code& e)
{
	if(e == boost::asio::error::operation_aborted)
		return;

	++timerCount_;

	// if ESC is pressed
	if(connection_->displayWaitingMessage(timerCount_) == false)
	{
		connection_->AddCmdLineOem("The task was canceled by user.");
		setLastError(replyCode(FTPReplyCodeCategory::Canceled));
		cancelOnTimerExpired_();
//		sendAbort();
	}

	if(timerCount_ < timeForTimeout/timerInterval_)
	{
		startTimer(cancelOnTimerExpired_, false);
	} else
	{
		connection_->AddCmdLineOem("timeout is occurred.");		
		setLastError(replyCode(FTPReplyCodeCategory::Timeout));
		cancelOnTimerExpired_();
	}
}

void FTPClient::handleWriteRequest(const error_code& err)
{
	timer_.cancel();		
	if(!err)
	{
		read();
	} else
	{
		startTimer(boost::bind(&FTPClient::close, this));
		errorHandler(err);
	}
}

void FTPClient::read()
{
	replyString_.clear();
	startTimer(boost::bind(&tcp::socket::cancel, &cmdsocket_));
	boost::asio::async_read_until(cmdsocket_, reply_, "\r\n",
		boost::bind(&FTPClient::handleRead, this, 
		boost::asio::placeholders::error, 
		boost::asio::placeholders::bytes_transferred));
}

error_code FTPClient::readOutput()
{
	lastErrorCode_.clear();
	io_service_.reset();
	read();
	io_service_.run();
	return lastErrorCode_;
}


void FTPClient::handleRead(const error_code& err, size_t /*bytes_transferred*/)
{
	timer_.cancel();
	if (!err)
	{
		reply_.data().begin();
		std::istream replayStream(&reply_);

		std::string line;
		std::getline(replayStream, line);
		BOOST_LOG(ERR, L"handleRead line: " << Unicode::AsciiToUtf16(line));
		if(!line.empty() && *line.rbegin() == '\r')
			line.resize(line.size()-1);

		connection_->AddCmdLineOem(line, ldIn);

		if(line.size() < 4)
		{
			setLastError(replyCode(FTPReplyCodeCategory::InvalidReply));
			return;
		}

		int code = 0;
		bool isLastLine = false;
		int i;
		for(i = 0; i < 3; ++i)
		{
			if(isdigit(line[i]))
				code = code * 10 + line[i] - '0';
			else
			{
				setLastError(replyCode(FTPReplyCodeCategory::InvalidReply));
				return;
			}
		}
		switch(line[3])
		{
		case ' ':
			isLastLine = true;
			break;
		case '-':
			break;
		default:
			setLastError(replyCode(FTPReplyCodeCategory::InvalidReply));
			return;
		}

		if(!replyString_.empty())
			replyString_ += '\n';
		replyString_.append(line, 4, line.size()-4);

		if(!isLastLine)
		{
			startTimer(boost::bind(&tcp::socket::cancel, &cmdsocket_));
			boost::asio::async_read_until(cmdsocket_, reply_, "\r\n",
				boost::bind(&FTPClient::handleRead, this, 
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));
		} else
			timer_.cancel();
		setLastError(replyCode(code));
	} else
	{
		BOOST_LOG(ERR, L"handleRead: " << err);
		errorHandler(err);
	}
}

void FTPClient::startTimer(const boost::function<void()> &cancelOnTimerExpired, bool reStart)
{
	if(reStart)
		timerCount_ = 0;
	timer_.expires_from_now(boost::posix_time::millisec(timerInterval_));
	timer_.async_wait(boost::bind(&FTPClient::handleTimer, this,
		boost::asio::placeholders::error));

	cancelOnTimerExpired_ = cancelOnTimerExpired;
}

Connection* FTPClient::getConnection() const
{
	return connection_;
}

void FTPClient::setLastError(const error_code& err)
{
	lastErrorCode_ = err;
}


void FTPClient::errorHandler(const error_code& err)
{
// 	if(!(err.category() == FTPReplyCodeCategory::getCategory() &&
// 		err.value() == FTPReplyCodeCategory::Canceled ||
// 		err.value() == FTPReplyCodeCategory::Timeout))
	{
		connection_->AddCmdLineOem(err.message(), ldRaw);
		setLastError(replyCode(FTPReplyCodeCategory::SocketError));
//		close();
	}
}

error_code FTPClient::login(const std::wstring &user, const std::wstring &password, const std::wstring &account)
{
	error_code code = sendCommand(g_manager.opt.cmdUser_ + L' ' + user);
	if(isContinue(code))
	{
		if(!g_manager.opt.showPassword_)
			connection_->AddCmdLine(g_manager.opt.cmdPass_ + L" *hidden*", ldOut);
		code = sendCommand(g_manager.opt.cmdPass_ + L' ' + password, true, !g_manager.opt.showPassword_);
	} else
	{
		return replyCode(FTPReplyCodeCategory::InvalidReply); 
	}
	if(isContinue(code))
		code = sendCommand(g_manager.opt.cmdAcct_ + L' ' + account);

	return code;
}

const std::string& FTPClient::getReply() const
{
	return replyString_;
}

bool FTPClient::prepareDataConnection(bool passive)
{
	datasocket_.close();
	acceptor_.close();

	if(passive)
	{
		if(sendPasv() == false)
			return false;
		return true;
	} else
	{
		tcp::endpoint endpoint(cmdsocket_.local_endpoint().address(), 0);
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen(1);
		sendPort(acceptor_.local_endpoint());
		return true;
	}
}

bool FTPClient::sendPort(tcp::endpoint endpoint)
{
	ip::address_v4::bytes_type ipaddr = endpoint.address().to_v4().to_bytes();
	int port = endpoint.port();
	std::wstring cmd = g_manager.opt.cmdPort + L' ' + 
		boost::lexical_cast<std::wstring>(ipaddr[0]) + L',' +
		boost::lexical_cast<std::wstring>(ipaddr[1]) + L',' +
		boost::lexical_cast<std::wstring>(ipaddr[2]) + L',' +
		boost::lexical_cast<std::wstring>(ipaddr[3]) + L',' +
		boost::lexical_cast<std::wstring>((port >> 8)& 0xFF) + L',' +
		boost::lexical_cast<std::wstring>(port & 0xFF);
	return isComplete(sendCommand(cmd));
}

bool FTPClient::sendPasv()
{
	error_code code = sendCommand(g_manager.opt.cmdPasv_);
	if(code.value() != EnteringPassiveMode)
	{
		connection_->AddCmdLine(L"Incorrect PASV reply", ldRaw);
		return false;
	}

	const std::string& reply = getReply();
	size_t ofs = reply.find('(');

	unsigned int a1, a2, a3, a4, p1, p2;

	if(ofs == std::string::npos ||
		sscanf(reply.c_str()+ofs, "(%u,%u,%u,%u,%u,%u)", &a1, &a2, &a3, &a4, &p1, &p2) !=6 ||
		a1 > 255 || a2 > 255 || a3 > 255 || a4 > 255 || p1 > 255 || p2 > 255 )
	{
		connection_->AddCmdLine(L"Incorrect PASV reply", ldRaw);
		return false;
	}

	pasvEndpoint_.address(ip::address_v4(a1 << 24 | a2 << 16 | a3 << 8 | a4));
	pasvEndpoint_.port(static_cast<unsigned short>(p1 << 8 | p2));
	return true;
}

bool FTPClient::establishActiveDataConnection()
{
	lastErrorCode_.clear();
	io_service_.reset();

	acceptor_.async_accept(datasocket_, 
		boost::bind(&FTPClient::handleAccept, this, boost::asio::placeholders::error));
	startTimer(boost::bind(&tcp::acceptor::close, &acceptor_));

	io_service_.run();
	return isComplete(lastErrorCode_);
}

void FTPClient::handleAccept(const error_code& err)
{
	timer_.cancel();
	if(!err)
	{
		lastErrorCode_ = replyCode(CommandOkey);
	} else
	{
		errorHandler(err);
	}
}

bool FTPClient::establishPassiveDataConnection()
{
	lastErrorCode_.clear();
	io_service_.reset();

 	datasocket_.async_connect(pasvEndpoint_,
 		boost::bind(&FTPClient::handlePassiveConnect, this, boost::asio::placeholders::error));

	startTimer(boost::bind(&tcp::socket::close, &datasocket_));

	io_service_.run();
	return isComplete(lastErrorCode_);
}

void FTPClient::handlePassiveConnect(const error_code& err)
{
	timer_.cancel();
	if(!err)
	{
		lastErrorCode_ = replyCode(CommandOkey);
	} else
	{
		errorHandler(err);
	}
}


error_code FTPClient::downloadFile(boost::function<error_code (const std::vector<char>&, size_t)> handler)
{
	lastErrorCode_.clear();
	io_service_.reset();
	
	startTimer(boost::bind(&tcp::socket::close, &datasocket_));

	datasocket_.async_read_some(asio::buffer(iobuffer_),
		boost::bind(&FTPClient::handleDownload, this, 
		boost::asio::placeholders::error, 
		boost::asio::placeholders::bytes_transferred,
		handler));

	io_service_.run();

	if(lastErrorCode_ == replyCode(FTPReplyCodeCategory::Canceled))
	{
		sendAbort();
		lastErrorCode_ = replyCode(FTPReplyCodeCategory::Canceled);
	}
	return lastErrorCode_;
}

void FTPClient::handleDownload(const error_code& err, size_t bytes_transferred, 
							   boost::function<error_code (const std::vector<char>&, size_t)> handler)
{
	timer_.cancel();
	if(!err)
	{
		error_code code = handler(iobuffer_, bytes_transferred);
		if(code)
		{
			connection_->AddCmdLineOem("Cannot write a file", ldRaw);
			setLastError(replyCode(FTPReplyCodeCategory::WriteError));
			return;
		}
		if(CheckForEsc(false))
		{
			connection_->AddCmdLineOem("The downloading was canceled by user", ldRaw);
//			sendAbort();
			setLastError(replyCode(FTPReplyCodeCategory::Canceled));
			return;
		}
		startTimer(boost::bind(&tcp::socket::close, &datasocket_));
		datasocket_.async_read_some(asio::buffer(iobuffer_),
			boost::bind(&FTPClient::handleDownload, this, 
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred,
			handler));
	} else
	if(err == boost::asio::error::eof)
	{
		// downloading finished
		datasocket_.close();
		setLastError(replyCode(CommandOkey));
	}
	else
	{
		datasocket_.close();
		errorHandler(err);
	}
}


void FTPClient::setBufferSize(size_t size)
{
	iobuffer_.resize(size);
}

void FTPClient::close()
{
	datasocket_.close();
//	cmdsocket_.cancel();
	cmdsocket_.close();
	acceptor_.close();
	timer_.cancel();
	resolver_.cancel();
	io_service_.stop();
	connected_ = false;
	reply_.consume(reply_.size());
}


error_code FTPClient::uploadFile(UploadHandler handler)
{
	lastErrorCode_.clear();
	io_service_.reset();


	size_t readbytes;
	error_code code = handler(iobuffer_, &readbytes);
	if(code)
	{
		errorHandler(code);
	} else
	{
		if(readbytes != 0)
		{
			startTimer(boost::bind(&tcp::socket::close, &datasocket_));
			asio::async_write(datasocket_, asio::buffer(iobuffer_, readbytes), 
				boost::bind(&FTPClient::handleUpload, this, 
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred,
				handler));
			io_service_.run();
		}
	}
	if(lastErrorCode_ == replyCode(FTPReplyCodeCategory::Canceled))
	{
		BOOST_LOG(ERR, L"ESC is pressed. datacon: " << datasocket_.is_open());
		sendAbort();
		lastErrorCode_ = replyCode(FTPReplyCodeCategory::Canceled);
	}

	return lastErrorCode_;
}


void FTPClient::handleUpload(const error_code& err, size_t /*bytes_transferred*/, UploadHandler handler)
{
	timer_.cancel();
	if(!err)
	{
		size_t readbytes;
		error_code code = handler(iobuffer_, &readbytes);
		if(code)
		{
			connection_->AddCmdLineOem("Cannot read from file", ldRaw);
			setLastError(replyCode(FTPReplyCodeCategory::ReadError));
			return;
		}
		if(readbytes == 0)
		{
			// uploading finished
			datasocket_.close();
			setLastError(replyCode(CommandOkey));
			return;
		}
		if(CheckForEsc(false))
		{
			connection_->AddCmdLineOem("The uploading was canceled by user", ldRaw);
//			sendAbort();
			setLastError(replyCode(FTPReplyCodeCategory::Canceled));
			return;
		}
		startTimer(boost::bind(&tcp::socket::close, &datasocket_));
		asio::async_write(datasocket_, asio::buffer(iobuffer_, readbytes), 
			boost::bind(&FTPClient::handleUpload, this, 
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred,
			handler));
	} else
		{
			datasocket_.close();
			errorHandler(err);
		}
}

void FTPClient::sendAbort()
{
	BOOST_ASSERT(datasocket_.is_open());
/*
	std::string message = "  ABOR";
	message[0] = cIAC; // send
	message[1] = cIP;  // interrupt process
	message[2] = cIAC; // send
	message[3] = cDM;  // data marker for sync 
*/
	BOOST_LOG(ERR, L"send Abort");
	std::string message = "ABOR";
	datasocket_.close();
	error_code ec = sendCommandOem(message);
	BOOST_LOG(ERR, L"end of \"send Abort\" " << ec);

	BOOST_ASSERT(ec.category() == FTPReplyCodeCategory::getCategory());
	
	boost::asio::socket_base::bytes_readable command(true);
	cmdsocket_.io_control(command);
	std::size_t bytes_readable = command.get();
	while(bytes_readable)
	{
		readOutput();
		cmdsocket_.io_control(command);
		bytes_readable = command.get();
	}
}