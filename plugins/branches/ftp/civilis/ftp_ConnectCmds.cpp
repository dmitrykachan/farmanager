#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"

/*
 * Set transfer type.
 */
bool Connection::settype(ftTypes Mode)
{  
	PROCP(L"Mode: " << Mode);

	std::wstring cmd = std::wstring(L"TYPE ") + static_cast<wchar_t>(Mode);

	if (isComplete(command(cmd))) 
	{
		type = Mode;
		BOOST_LOG(INF, L"TYPE = " << Mode);
		return true;
	} else
		return false;
}

bool Connection::setascii()  { return settype(TYPE_A); }
bool Connection::setbinary() { return settype(TYPE_I); }
bool Connection::setebcdic() { return settype(TYPE_E); }

bool Connection::appended(const std::wstring &local, const std::wstring &remote)
{
	std::wstring cmd;

	if(local.empty())
		return false;

	sendrequest(g_manager.opt.cmdAppe, local, remote.empty()? local : remote);
	restart_point = 0;
	return true;
}

bool Connection::chmod(const std::wstring& file, const std::wstring &mode)
{
	if(file.empty() | mode.empty())
		return false;

	return command(g_manager.opt.cmdSite + L" " + g_manager.opt.cmdChmod + L" " + file + L" " + mode);
}

bool Connection::socketStartup_;

bool Connection::startupWSA()
{
	if(!socketStartup_)
	{
		WSADATA WSAData;
		if(WSAStartup(MAKEWORD(2,2), &WSAData) == 0)
			socketStartup_ = true;
		else
			return false;
	}
	return true;
}

void Connection::cleanupWSA()
{
	if(socketStartup_)
		WSACleanup();
}

/*
 * Connect to peer server and auto-login, if possible.
 * 1      2       3       4
 * <site> [<port> [<user> [<pwd>]]]
 */
bool Connection::setpeer_(const std::wstring& site, size_t port, const std::wstring &user, const std::wstring &pwd)
{
	FARWrappers::Screen scr;

	if(site.empty())
		return false;

	if(startupWSA() == false)
	{
		// TODO error message
		return false;
	}

	//port
	if(port == 0)
	{
		servent *sp = getservbyname("ftp", "tcp");
		if(!sp)
		{
			BOOST_LOG(INF, L"ftp: ftp/tcp: unknown service. Assume port number " << IPPORT_FTP << L"(default)");
			port = IPPORT_FTP;
		} else
		{
			port = ntohs(sp->s_port);
			BOOST_LOG(INF, L"ftp: port " << std::hex << port);
		}
	}

	//Check if need to close connection
	if(isConnected())
	{
		if(!boost::iequals(site, getHost().url_.Host_) || port != portnum)
			disconnect();
	}

	//Make connection
	if (!isConnected())
	{
		if (!hookup(site, static_cast<int>(port)))
		{
			//Open connection
			return false;
		}
		setConnected();
	}

	if(!login(user, pwd))
	{
		disconnect();
		return false;
	}

	return true;
}


bool Connection::setpeer(const std::wstring& site, const std::wstring& port, const std::wstring &user, const std::wstring &pwd)
{
	int nport;
	if(!port.empty())
	{
		try
		{
			nport = htons(boost::lexical_cast<int>(port));
		}
		catch(boost::bad_lexical_cast &)
		{
			return false;
		}		
	}
	else
		nport = 0;
	return setpeer_(site, nport, user, pwd);
}


/*
 * Send a single file.
 */
bool Connection::put(const std::wstring &local, const std::wstring& remote)
{
	PROC;
	std::wstring cmd;
	std::wstring rem = remote;

	if(local.empty())
		return false;
	if(rem.empty())
		rem = local;

	cmd = ((sunique) ? g_manager.opt.cmdPutUniq : g_manager.opt.cmdStor); //STORE or PUT
	sendrequest(cmd, local, remote);
	restart_point = 0;
	return true;
}

bool Connection::reget(const std::wstring &remote, const std::wstring& local)
{
	return getit(remote, local, 1, "a+" );
}

bool Connection::get(const std::wstring &remote, const std::wstring& local)
{
	return getit(remote, local, 0, restart_point ? "r+" : "w" );
}

/*
 * get file if modtime is more recent than current file
 */
bool Connection::newer(const std::wstring &remote, const std::wstring& local)
{
	return getit(remote, local, -1, "w");
}

/*
 * Receive one file.
 */
int Connection::getit(const std::wstring &remote, const std::wstring& local, int restartit, char *mode)
{
	std::wstring loc = local;
	if(loc.empty())
		loc = remote;

	if(restartit == -1)
		restart_point = -1;
	else
		if (restartit)
		{
			restart_point = Fsize(loc);
			BOOST_LOG(INF, L"Restart from " << restart_point);
		}

	recvrequest(g_manager.opt.cmdRetr, loc, remote, mode);
	restart_point = 0;

	return RPL_OK;
}


/*
 * Toggle PORT cmd use before each data connection.
 */
bool Connection::setport()
{
	sendport_ = !sendport_;
	return true;
}


/*
 * Set current working directory
 * on remote machine.
 */

bool Connection::cd(const std::wstring &dir)
{
	int rc = command(g_manager.opt.cmdCwd + L" " + dir);
	if(rc == SyntaxErrorBadCommand)
		rc = command(g_manager.opt.cmdXCwd + L" " + dir);
	return isComplete(rc);
}

/*
 * Delete a single file.
 */
bool Connection::deleteFile(const std::wstring& filename)
{  
	if(filename.empty())
		return false;
	return command(g_manager.opt.cmdDel + L" " + filename);
}


/*
 * Rename a remote file.
 */
bool Connection::renamefile(const std::wstring& oldfilename, const std::wstring& newfilename)
{
	PROC;
	if(oldfilename.empty() || newfilename.empty())
		return false;

	int rc = command(g_manager.opt.cmdRen + L' ' + oldfilename);
	if(rc == RPL_CONTINUE)
		rc = command(g_manager.opt.cmdRenTo + L' ' + newfilename);
	return rc;
}

/*
 * Get a directory listing
 * of remote files.
 */
bool Connection::ls(const std::wstring &path)
{
	ResetOutput();
	if(!getHost().ExtList)
		recvrequest(g_manager.opt.cmdList, L"-", path, "w");
	else
	{
		recvrequest(getHost().listCMD_, L"-", path, "w");
//TODO		if(code > SyntaxErrorBadCommand && !path.empty())
			recvrequest(g_manager.opt.cmdList, L"-", path, "w");
	}
	return true;
}


bool Connection::nlist(const std::wstring &path)
{
	recvrequest(g_manager.opt.cmdNList, L"-", path, "w");
	return RPL_OK;
}

/*
 * Send new user information (re-login)
 */
bool Connection::user(const std::wstring& usr, const std::wstring& pwd, const std::wstring &accountcmd)
{
	return login(usr, pwd);
}

/*
 * Print working directory.
 */
bool Connection::pwd()
{
	PROC;

	int rc = command(g_manager.opt.cmdPwd);
	if(rc == SyntaxErrorBadCommand)
	{
		BOOST_LOG(INF, L"Try XPWD " << g_manager.opt.cmdXPwd);
		rc = command(g_manager.opt.cmdXPwd);
	}
	return isComplete(rc);
}

/*
 * Make a directory.
 */
bool Connection::makedir(const std::wstring &dir)
{  
	if(dir.empty())
		return false;

	int rc = command(g_manager.opt.cmdMkd + L" " + dir);
	if(rc == SyntaxErrorBadCommand)
		rc = command(g_manager.opt.cmdXMkd + L" " + dir);
	return isComplete(rc);
}

/*
 * Remove a directory.
 */
bool Connection::removedir(const std::wstring &dir)
{
	PROC;
	if(dir.empty())
		return false;

	int rc = command(g_manager.opt.cmdRmd  + L' ' + dir);
	if(rc == SyntaxErrorBadCommand)
		rc = command(g_manager.opt.cmdXRmd + L' ' + dir);
	return isComplete(rc);
}

/*
 * Send a line, verbatim, to the remote machine.
 */
bool Connection::quote(const std::vector<std::wstring> &params)
{
	PROC;

	if(params.size() < 2)
		return false;

	std::vector<std::wstring>::const_iterator itr = params.begin()+1;
	std::wstring cmd = *itr;
	while(++itr != params.end())
	{
		cmd += L' ' + *itr;
	}

	int rc = command(cmd);
	if(isPrelim(rc))
	{
		do 
		{
			rc = getreply();
		} while(isPrelim(rc));
	}
	// TODO WTF
	return rc;
}

/*
 * Send a SITE command to the remote machine.  The line
 * is sent almost verbatim to the remote machine, the
 * first argument is changed to SITE.
 */

bool Connection::site(const std::vector<std::wstring>& params)
{  
	PROC;

	if (params.size() < 2)
		return false;

	std::vector<std::wstring>::const_iterator itr = params.begin()+1;
	std::wstring cmd = g_manager.opt.cmdSite;
	while(itr != params.end())
		cmd +=  + L" " + *itr;
	int rc = command(cmd);
	if(isPrelim(rc))
	{
		do 
		{
			rc = getreply();
		} while(isPrelim(rc));
	}
	return rc;
}

bool Connection::do_umask(const std::wstring &mask)
{
	PROC;

	std::wstring cmd = g_manager.opt.cmdSite + L" " + g_manager.opt.cmdUmask;
	if(!mask.empty())
		cmd += L" " + mask;
	return isComplete(command(cmd));
}

bool Connection::idle(const std::wstring &time)
{
	return isComplete(command(g_manager.opt.cmdSite + L' ' + g_manager.opt.cmdIdle + 
		L' ' + time));
}

/*
 * Terminate session and exit.
 */
/*VARARGS*/
bool Connection::quit()
{
	PROC;

	if(isConnected())
		disconnect();

	return true;
}

/*
 * Terminate session, but don't exit.
 */
bool Connection::disconnect()
{
	if (!isConnected())
		return true;
	int rc = command(g_manager.opt.cmdQuit, true);
	setConnected(false);
	data_peer_.close();
	return rc;
}


bool Connection::account(const std::vector<std::wstring> &params)
{  
	PROC;
	std::wstring  acct;

	if(params.size() < 2)
		return false;

	std::vector<std::wstring>::const_iterator itr = params.begin()+1;
	while(itr != params.end())
	{
		acct += L' ' + *itr;
		++itr;

	}
	return command(g_manager.opt.cmdAcct_ + acct);
}


bool Connection::setsunique()
{
	sunique = !sunique;
	return true;
}


bool Connection::setrunique()
{
	runique = !runique;
	return true;
}

/* change directory to parent directory */
bool Connection::cdup()
{
	PROC;
	int rc = command(g_manager.opt.cmdCDUp);
	if(rc == SyntaxErrorBadCommand)
		rc = command(g_manager.opt.cmdXCDUp);
	return isComplete(rc);
}

/* restart transfer at specific point */
bool Connection::restart(const std::wstring& point)
{
	PROC;
	if(!point.empty())
	{
		try
		{
			restart_point = boost::lexical_cast<int>(point);
		}
		catch (boost::bad_lexical_cast &)
		{
			return false;
		}
		
	}
	return RPL_OK;
}


/* show remote system type */
bool Connection::syst()
{
	PROC;
	return isComplete(command(g_manager.opt.cmdSyst));
}


/*
 * get size of file on remote machine
 */
bool Connection::sizecmd(const std::wstring& filename)
{
	PROC;
	if(filename.empty())
		return false;
	return command(g_manager.opt.cmdSize + L' ' + filename);
}

/*
 * get last modification time of file on remote machine
 */
bool Connection::modtime(const std::wstring& file)
{
	if(file.empty())
		return false;

	if(isComplete(command(g_manager.opt.cmdMDTM + L" " + file)))
	{
//TODO		int yy, mo, day, hour, min, sec;
// 			sscanf( replyString_.c_str(),
// 			g_manager.opt.fmtDateFormat.c_str(),
// 			&yy, &mo, &day, &hour, &min, &sec);
		return true;
	}
	return false;
}

/*
 * show status on remote machine
 */
bool Connection::rmtstatus(const std::wstring &c)
{
	PROC;
	std::wstring cmd = g_manager.opt.cmdStat;
	if(!c.empty())
		cmd += L' ' + c;
	return command(cmd);
}

/*
 * Ask the other side for help.
 */
bool Connection::rmthelp(const std::wstring &c)
{
	PROC;
	std::wstring cmd = g_manager.opt.cmdHelp;
	if(c.empty())
		cmd += L' ' + c;
	return command(cmd);
}

bool Connection::doport(in_addr data_addr, int port)
{
	std::wstring cmd = g_manager.opt.cmdPort + L' ' + 
		boost::lexical_cast<std::wstring>(data_addr.S_un.S_un_b.s_b1) + L',' +
		boost::lexical_cast<std::wstring>(data_addr.S_un.S_un_b.s_b2) + L',' +
		boost::lexical_cast<std::wstring>(data_addr.S_un.S_un_b.s_b3) + L',' +
		boost::lexical_cast<std::wstring>(data_addr.S_un.S_un_b.s_b4) + L',' +
		boost::lexical_cast<std::wstring>((port >> 8)& 0xFF) +          L',' +
		boost::lexical_cast<std::wstring>(port & 0xFF);
	return isComplete(command(cmd));
}