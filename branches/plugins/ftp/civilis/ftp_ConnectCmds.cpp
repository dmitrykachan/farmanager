#include "stdafx.h"

#include "ftp_Int.h"
#include "utils/uniconverts.h"

/*
 * Set transfer type.
 */
Connection::Result Connection::settype(ftTypes Mode)
{  
	PROCP(L"Mode: " << Mode);

	BOOST_ASSERT(Mode != TYPE_NONE);
	if(type_ == Mode)
		return Done;
	std::wstring cmd = std::wstring(L"TYPE ") + static_cast<wchar_t>(Mode);

	if(FTPClient::isComplete(ftpclient_.sendCommand(cmd))) 
	{
		type_ = Mode;
		BOOST_LOG(INF, L"TYPE = " << Mode);
		return Done;
	} else
	{
		BOOST_LOG(ERR, L"new type was not set: " << Mode);
		return Error;
	}
}

Connection::Result Connection::setascii()  { return settype(TYPE_A); }
Connection::Result Connection::setbinary() { return settype(TYPE_I); }

Connection::Result Connection::appended(const std::wstring &local, const std::wstring &remote)
{
	std::wstring cmd;

	if(local.empty())
		return Error;

	Result res = Done;//TODOsendrequest(g_manager.opt.cmdAppe, local, remote.empty()? local : remote, getHost()->AsciiMode);

	restart_point = 0;
	return res;
}

Connection::Result Connection::chmod(const std::wstring& file, const std::wstring &mode)
{
	if(file.empty() | mode.empty())
		return Error;

	return command(g_manager.opt.cmdSite + L" " + g_manager.opt.cmdChmod + L" " + file + L" " + mode);
}

/*
  Connect to peer server and auto-login, if possible.
*/
Connection::Result Connection::setpeer_(const std::wstring& site, size_t port, std::wstring &user, std::wstring &pwd, bool reaskPassword)
{
	FARWrappers::Screen scr;

	if(site.empty())
		return Error;

	//Check if need to close connection
	if(disconnect() != Done)
		return Error;

	error_code code = ftpclient_.connect(Unicode::utf16ToAscii(site), port);
	if(FTPClient::isCanceled(code))
		return Cancel;
	if(!FTPClient::isComplete(code))
		return Error;

	Result result = Error;
	while(true)
	{
		error_code code = login(user, pwd);
		if(FTPClient::isComplete(code))
			return Done;
		if(code.value() == NotLoggedIn && reaskPassword)
		{
			BOOST_LOG(INF, L"Reask password");
			getHost()->ExtCmdView = true;
			ConnectMessage(MNone__, site, MNone__);
			if(!GetLoginData(user, pwd, true))
			{
				result = Cancel;
				break;
			}
			getHost()->Write();
		} else
			break;
	}
	disconnect();
	return result;
}


Connection::Result Connection::setpeer(const std::wstring& site, const std::wstring& port, const std::wstring &user, const std::wstring &pwd)
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
			return Error;
		}		
	}
	else
		nport = 0;
	std::wstring usr = user;
	std::wstring pas =  pwd;
	return setpeer_(site, nport, usr, pas, false);
}


/*
 * Send a single file.
 */
Connection::Result Connection::put(const std::wstring &local, const std::wstring& remote)
{
	PROC;
	std::wstring cmd;

	if(local.empty())
		return Error;

	cmd = ((sunique) ? g_manager.opt.cmdPutUniq : g_manager.opt.cmdStor); //STORE or PUT
	Result res = Done;//TODOsendrequest(cmd, local, remote.empty()? local : remote, getHost()->AsciiMode);
	restart_point = 0;
	return res;
}

Connection::Result Connection::reget(const std::wstring &remote, const std::wstring& local)
{
	FTPProgress trafficInfo;
	// TODO
	return getit(remote, local, 1, L"a+", trafficInfo);
}

Connection::Result Connection::get(const std::wstring &remote, const std::wstring& local, FTPProgress& trafficInfo)
{
	return getit(remote, local, 0, restart_point ? L"r+" : L"w", trafficInfo);
}

/*
 * get file if modtime is more recent than current file
 */
Connection::Result Connection::newer(const std::wstring &remote, const std::wstring& local)
{
	FTPProgress trafficInfo;
	//TODO
	return getit(remote, local, -1, L"w", trafficInfo);
}

/*
 * Receive one file.
 */
Connection::Result Connection::getit(const std::wstring &remote, const std::wstring& local, int restartit, wchar_t *mode, FTPProgress& trafficInfo)
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

	return recvrequest(g_manager.opt.cmdRetr, loc, remote, mode, trafficInfo);
}


/*
 * Toggle PORT cmd use before each data connection.
 */
Connection::Result Connection::setport()
{
	sendport_ = !sendport_;
	return Done;
}


/*
 * Set current working directory
 * on remote machine.
 */

Connection::Result Connection::cd(const std::wstring &dir)
{
	Result res = command(g_manager.opt.cmdCwd + L" " + dir);
	if(res != Error)
		return res;

	return command(g_manager.opt.cmdXCwd + L" " + dir);
}

/*
 * Delete a single file.
 */
Connection::Result Connection::deleteFile(const std::wstring& filename)
{  
	if(filename.empty())
		return Error;
	return command(g_manager.opt.cmdDel + L' ' + filename);
}


/*
 * Rename a remote file.
 */
Connection::Result Connection::renamefile(const std::wstring& oldfilename, const std::wstring& newfilename)
{
	PROC;
	if(oldfilename.empty() || newfilename.empty())
		return Error;

	Result res = command(g_manager.opt.cmdRen + L' ' + oldfilename);
	if(res != Error)
		return res;
	else
		return command(g_manager.opt.cmdRenTo + L' ' + newfilename);
}

/*
 * Get a directory listing
 * of remote files.
 */
Connection::Result Connection::ls(const std::wstring &path)
{
	ResetOutput();
	FTPProgress trafficInfo;
	trafficInfo.Init(MStatusDownload, OPM_SILENT, FileList(), FTPProgress::NotDisplay);
	trafficInfo.initFile();
	if(!getHost()->ExtList)
		return recvrequest(g_manager.opt.cmdList, g_MemoryFile, path, L"w", trafficInfo);
	else
	{
		Result res = recvrequest(getHost()->listCMD_, g_MemoryFile, path, L"w", trafficInfo);
		if(res != Error)
			return res;
		return recvrequest(g_manager.opt.cmdList, g_MemoryFile, path, L"w", trafficInfo);
	}
}


Connection::Result Connection::nlist(const std::wstring &path)
{
	ResetOutput();
	FTPProgress trafficInfo;
	trafficInfo.Init(MStatusDownload, OPM_SILENT, FileList(), FTPProgress::NotDisplay);
	return recvrequest(g_manager.opt.cmdNList, g_MemoryFile, path, L"w", trafficInfo);
}

/*
 * Send new user information (re-login)
 */
Connection::Result Connection::user(const std::wstring& usr, const std::wstring& pwd, const std::wstring &accountcmd)
{
	error_code code = login(usr, pwd);
	if(FTPClient::isCanceled(code))
		return Cancel;
	if(FTPClient::isComplete(code))
		return Done;
	return Error;
}

/*
 * Print working directory.
 */
Connection::Result Connection::pwd()
{
	PROC;

	Result res = command(g_manager.opt.cmdPwd);
	if(res != Error)
		return res;
	return command(g_manager.opt.cmdXPwd);
}

/*
 * Make a directory.
 */
Connection::Result Connection::makedir(const std::wstring &dir)
{  
	if(dir.empty())
		return Error;

	Result res = command(g_manager.opt.cmdMkd + L" " + dir);
	if(res != Error)
		return res;
	return command(g_manager.opt.cmdXMkd + L" " + dir);
}

/*
 * Remove a directory.
 */
Connection::Result Connection::removedir(const std::wstring &dir)
{
	PROC;
	if(dir.empty())
		return Error;

	Result res = command(g_manager.opt.cmdRmd + L" " + dir);
	if(res != Error)
		return res;
	return command(g_manager.opt.cmdXRmd + L" " + dir);
}

/*
 * Send a line, verbatim, to the remote machine.
 */
Connection::Result Connection::quote(const std::vector<std::wstring> &params)
{
	PROC;

	if(params.size() < 2)
		return Error;

	std::vector<std::wstring>::const_iterator itr = params.begin()+1;
	std::wstring cmd = *itr;
	while(++itr != params.end())
	{
		cmd += L' ' + *itr;
	}

	return command(cmd);
}

/*
 * Send a SITE command to the remote machine.  The line
 * is sent almost verbatim to the remote machine, the
 * first argument is changed to SITE.
 */

Connection::Result Connection::site(const std::vector<std::wstring>& params)
{  
	PROC;

	if (params.size() < 2)
		return Error;

	std::vector<std::wstring>::const_iterator itr = params.begin()+1;
	std::wstring cmd = g_manager.opt.cmdSite;
	while(itr != params.end())
		cmd +=  + L" " + *itr;
	return command(cmd);
}

Connection::Result Connection::do_umask(const std::wstring &mask)
{
	PROC;

	std::wstring cmd = g_manager.opt.cmdSite + L" " + g_manager.opt.cmdUmask;
	if(!mask.empty())
		cmd += L" " + mask;
	return command(cmd);
}

Connection::Result Connection::idle(const std::wstring &time)
{
	return command(g_manager.opt.cmdSite + L' ' + g_manager.opt.cmdIdle + L' ' + time);
}

/*
 * Terminate session, but don't exit.
 */
Connection::Result Connection::disconnect()
{
	if (!isConnected())
		return Done;
	Result res = command(g_manager.opt.cmdQuit, true);
	ftpclient_.close();
	return res;
}


Connection::Result Connection::account(const std::vector<std::wstring> &params)
{  
	PROC;
	std::wstring  acct;

	if(params.size() < 2)
		return Error;

	std::vector<std::wstring>::const_iterator itr = params.begin()+1;
	while(itr != params.end())
	{
		acct += L' ' + *itr;
		++itr;

	}
	return command(g_manager.opt.cmdAcct_ + acct);
}


Connection::Result Connection::setsunique()
{
	sunique = !sunique;
	return Done;
}


Connection::Result Connection::setrunique()
{
	runique = !runique;
	return Done;
}

/* change directory to parent directory */
Connection::Result Connection::cdup()
{
	Result res = command(g_manager.opt.cmdCDUp);
	if(res != Error)
		return res;
	return command(g_manager.opt.cmdXCDUp);
}

/* restart transfer at specific point */
Connection::Result Connection::restart(const std::wstring& point)
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
			return Error;
		}
		
	}
	return Done;
}


/* show remote system type */
Connection::Result Connection::syst()
{
	return command(g_manager.opt.cmdSyst);
}


/*
 * get size of file on remote machine
 */
Connection::Result Connection::sizecmd(const std::wstring& filename)
{
	PROC;
	if(filename.empty())
		return Error;
	return command(g_manager.opt.cmdSize + L' ' + filename);
}

/*
 * get last modification time of file on remote machine
 */
Connection::Result Connection::modtime(const std::wstring& file)
{
	if(file.empty())
		return Error;

	Result res = command(g_manager.opt.cmdMDTM + L" " + file);
	if(res == Done)
	{
//TODO		int yy, mo, day, hour, min, sec;
// 			sscanf( replyString_.c_str(),
// 			g_manager.opt.fmtDateFormat.c_str(),
// 			&yy, &mo, &day, &hour, &min, &sec);
		return Done;
	}
	return res;
}

/*
 * show status on remote machine
 */
Connection::Result Connection::rmtstatus(const std::wstring &c)
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
Connection::Result Connection::rmthelp(const std::wstring &c)
{
	PROC;
	std::wstring cmd = g_manager.opt.cmdHelp;
	if(c.empty())
		cmd += L' ' + c;
	return command(cmd);
}
