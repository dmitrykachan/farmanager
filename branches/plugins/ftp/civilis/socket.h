#ifndef SOCKET_HEADER_FILE
#define SOCKET_HEADER_FILE

class ConnectionError: public std::exception
{
private:
	int		code_;
	mutable boost::scoped_ptr<std::string> what_;

public:
	static const int success = 0;

	ConnectionError()
		: code_(success)
	{}

	ConnectionError(int code)
		: code_(code)
	{}

	ConnectionError(int code, const char* msg)
		: code_(code)
	{
		what_.reset(new std::string(msg)); 
	}

	ConnectionError(const ConnectionError& err)
		: std::exception(err), code_(err.code_)
	{
	}

	ConnectionError& operator=(const ConnectionError& err)
	{
		code_ = err.code_;
		what_.reset();
	}

	/// Get a string representation of the exception.
	virtual const char* what() const
	{
		if (!what_)
		{
			char* msg = 0;
			DWORD length = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
				| FORMAT_MESSAGE_FROM_SYSTEM
				| FORMAT_MESSAGE_IGNORE_INSERTS, 0, code_,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&msg, 0, 0);
			what_.reset(new std::string(msg));
			::LocalFree(msg);
		}
		return what_->c_str();
	}

};

class TCPSocket: boost::noncopyable
{
private:
	SOCKET		socket_;
	WSAEVENT	event_;
	sockaddr_in addr_;

	static int error_wrapper(int n)
	{
		if(n == SOCKET_ERROR)
		{
			int err = ::WSAGetLastError();
			if(err != WSAEWOULDBLOCK)
				throw ConnectionError(err);
		}
		return n;
	}

	template<typename T, typename E>
	static T error_wrapper(T val, E error_value)
	{
		if(val == error_value)
			throw ConnectionError(::WSAGetLastError());
		return val;
	}

	friend std::wostream& operator <<(std::wostream& s, const TCPSocket &sock);

public:
	TCPSocket()
		: socket_(INVALID_SOCKET), event_(WSA_INVALID_EVENT)
	{
		std::fill_n(addr_.sin_zero, sizeof(addr_.sin_zero), 0);
	}
	~TCPSocket()
	{
		close();
	}

	void create(DWORD flag = 0)
	{
		close();
		socket_ = error_wrapper(::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, flag | WSA_FLAG_OVERLAPPED), INVALID_SOCKET);
	};

	void close()
	{
		if(socket_ != INVALID_SOCKET)
		{
			error_wrapper(::closesocket(socket_));
			socket_ = INVALID_SOCKET;
		}
		if(event_ != WSA_INVALID_EVENT)
		{
			::WSACloseEvent(event_);
			event_ = WSA_INVALID_EVENT;
		}
	}

	WSAEVENT getEvent()
	{
		if(event_ == WSA_INVALID_EVENT)
			event_ = ::WSACreateEvent();
		else
			::ResetEvent(event_);
		return event_;
	}

	SOCKET getSocket() const
	{
		BOOST_ASSERT(socket_ != INVALID_SOCKET);
		return socket_;
	}

	bool connect(in_addr &ipaddr, int port)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port	= htons(port);
		addr.sin_addr	= ipaddr;
		error_wrapper(::WSAConnect(getSocket(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr), 0,0,0,0));
		if(::WSAGetLastError() == WSAEWOULDBLOCK)
		{
			error_wrapper(::WSAEventSelect(getSocket(), getEvent(), FD_CONNECT));
			return false;
		}
		return true;
	}

	static in_addr resolveName(const char* hostname)
	{
		in_addr ipaddr;
		ipaddr.s_addr = inet_addr(hostname);
		if(ipaddr.s_addr != INADDR_NONE)
			return ipaddr;
		
		hostent* host = error_wrapper(::gethostbyname(hostname), static_cast<hostent*>(0));
		if(host->h_addrtype != AF_INET)
			throw ConnectionError(WSAEFAULT);
		return *reinterpret_cast<in_addr*>(host->h_addr_list[0]);
	}
	
	sockaddr_in getlocalname()
	{
		sockaddr_in name;
		int len = sizeof(name);
		::getsockname(getSocket(), reinterpret_cast<sockaddr*>(&name), &len);
		return name;
	}

	void shutdown(int how = SD_BOTH)
	{
		BOOST_ASSERT(how == SD_RECEIVE || how == SD_SEND || how == SD_BOTH);
		error_wrapper(::shutdown(getSocket(), how));
	}

	bool write(const char* buffer, size_t len, size_t timeout = INFINITE)
	{
		WSABUF buf = {static_cast<int>(len), const_cast<char*>(buffer)};
		DWORD flag = 0;
		DWORD sendBytes;

		if(wait(timeout, FD_WRITE) == false)
			return false;

		error_wrapper(::WSASend(getSocket(), &buf, 1, &sendBytes, flag, 0, 0));
		return true;
	};

	bool read(char* buffer, int *len, size_t timeout = INFINITE)
	{
		WSABUF buf = {*len, buffer};
		DWORD flag = 0;
		DWORD recvBytes;

		if(wait(timeout, FD_READ) == false)
			return false;
		error_wrapper(::WSARecv(getSocket(), &buf, 1, &recvBytes, &flag, 0, 0));
		*len = recvBytes;
		return true;
	}

	bool accept(sockaddr_in* addr, TCPSocket& sock, size_t timeout = INFINITE)
	{
		if(wait(timeout, FD_ACCEPT) == false)
			return false;

		int len = sizeof(*addr);
		sock.socket_ = error_wrapper(::accept(getSocket(), reinterpret_cast<sockaddr*>(addr), &len), INVALID_SOCKET);
		
		return true;
	}

	void swap(TCPSocket& sock)
	{
		std::swap(sock.socket_, socket_);
		std::swap(sock.event_,  event_);
	}

	void bind(sockaddr_in* addr)
	{
		error_wrapper(::bind(socket_, reinterpret_cast<sockaddr*>(addr), sizeof(*addr)));
	}

	void listen(int backlog)
	{
		error_wrapper(::listen(getSocket(), backlog));
	}

	bool isCreated()
	{
		return socket_ != INVALID_SOCKET;
	}

	void setSockOpt(int level, int optname, const char* optval, int optlen)
	{
		error_wrapper(setsockopt(getSocket(), level, optname, optval, optlen));
	}
private:
	bool wait(size_t timeout, long events)
	{
		WSAEVENT	event = getEvent();
		error_wrapper(WSAEventSelect(getSocket(), event, events | FD_CLOSE));
		DWORD res = ::WSAWaitForMultipleEvents(1, &event, false, static_cast<DWORD>(timeout), false);
		if(res == WSA_WAIT_TIMEOUT)
			return false;
		BOOST_ASSERT(res == WSA_WAIT_EVENT_0);
		return true;
	}


};

inline std::wostream& operator <<(std::wostream& s, const TCPSocket& sock)
{
	s << sock.socket_;
	return s;
}


#endif