#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"
#include "progress.h"

int Connection::recvrequest(const std::wstring& cmd, const std::wstring &local, const std::wstring &remote, wchar_t *mode )
{  
	//??SaveConsoleTitle _title;

	int res = recvrequestINT(cmd, local, remote, mode);
	IdleMessage(NULL, 0);
	return res;
}

int Connection::recvrequestINT(const std::wstring& cmd, const std::wstring &local, const std::wstring &remote, wchar_t *mode )
{
	int              oldtype = 0,
		is_retr;
	FHandle          fout;
	int              oecode;
	BOOL             oldBrk = getBreakable();
	FTPCurrentStates oState = CurrentState;

	if(type == TYPE_A)
		restart_point = 0;

	if(local != L"-")
	{
		fout.Handle = Fopen(local.c_str(), mode, g_manager.opt.SetHiddenOnAbort ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL );
		BOOST_LOG(INF, L"recv file [" << getHost().IOBuffSize << L"] \"" << local << L"\"=" << fout.Handle);
		if(!fout.Handle)
		{
			BOOST_LOG(ERR, L"!Fopen [" << mode << L"] " << WinAPI::getStringError());
			if (!ConnectMessage( MErrorOpenFile, local, true, MRetry))
				return RPL_CANCEL;
			//goto abort;
			return RPL_ERROR;
		}

		if(restart_point != -1)
		{
			if(!Fmove(fout.Handle, restart_point))
			{
				ErrorCode = GetLastError();
				if ( !ConnectMessage( MErrorPosition, local, true, MRetry) )
					return RPL_CANCEL;
				return RPL_ERROR;
			}
		}
		TrafficInfo->resume(restart_point == -1 ? 0 : restart_point);
	}

	is_retr = g_manager.opt.cmdRetr == cmd;

	in_addr data_addr;
	int		port;

	if(!initDataConnection(data_addr, port))
	{
		BOOST_LOG(INF, L"!initconn");
		return RPL_ERROR;
	}

	if (!is_retr)
	{
		if ( type != TYPE_A ) {
			oldtype = type;
			setascii();
		}
	} else
		if(restart_point)
		{
			if(!resumeSupport_)
			{
				AddCmdLine( getMsg(MResumeRestart) );
				restart_point = 0;
			} else
				if ( restart_point != -1 )
				{
					int result = command(g_manager.opt.cmdRest_ + L" " + boost::lexical_cast<std::wstring>(restart_point));
					if(!isComplete(result))
					{
						BOOST_LOG(INF, L"!restart SIZE");
						return result;
					}
				}
		}

		if (getHost().PassiveMode )
		{
			if(!dataconn(data_addr, port))
			{
				BOOST_LOG(ERR, "!dataconn: PASV ent");
				goto abort;
			}
		}

		if(!remote.empty())
		{
			int result = command(cmd + L" " + remote);
			if(!isPrelim(result))
			{
				if (oldtype)
					SetType(oldtype);
				BOOST_LOG(ERR, L"!command: " << cmd);
				fout.Close();
				if(Fsize(local))
					DeleteFileW(local.c_str());
				return result;
			}
		} else
			if(!isPrelim(command(cmd)))
			{
				if (oldtype)
					SetType(oldtype);
				return RPL_ERROR;
			}

			if(!getHost().PassiveMode )
			{
				if (!dataconn(data_addr, port))
				{
					BOOST_LOG(ERR, ("!dataconn: PASV ret"));
					goto abort;
				}
			}

		switch (type)
		{
		case TYPE_A:
		case TYPE_I:
		case TYPE_L:
			{
				__int64 totalValue = 0;
				WinAPI::Stopwatch stopwatch(500);
				setBreakable(false);

				// TODO		if ( fout.Handle )
				//			FTPNotify().Notify( &ni );
				WinAPI::Stopwatch timer;
				BOOST_LOG(ERR, L"start downloading");

				while( 1 )
				{
					int len = getHost().IOBuffSize;
					len = nb_recv(data_peer_, IOBuff.get(), len, 0);
					if(len == 0)
						break;
					totalValue += len;

					if ( !fout.Handle )
					{
						//Add readed to buffer
						BOOST_LOG(INF, L"AddOutput: +" << len << L" bytes");
						AddOutput( (BYTE*)IOBuff.get(), len);
					} else
						if ( Fwrite(fout.Handle,IOBuff.get(), len) != len)
						{
							//Write to file
							BOOST_LOG(ERR, L"!write local");
							ErrorCode = GetLastError();
							goto abort;
						}

						//Call user CB
						if(IOCallback)
						{
							if(!TrafficInfo->refresh(len))
							{
								BOOST_LOG(INF, L"gf: canceled by CB");
								ErrorCode = ERROR_CANCELLED;
								goto abort;
							}
						} else
							//Show Quite progressing
							if ( g_manager.opt.ShowIdle && remote.empty())
							{
								std::wstring str;
								if (stopwatch.isTimeout())
								{
									str = getMsg(MReaded) + Size2Str(totalValue);
									SetLastError( ERROR_SUCCESS );
									IdleMessage(str.c_str(),g_manager.opt.ProcessColor );
									stopwatch.reset();
									if ( CheckForEsc(FALSE) ) {
										ErrorCode = ERROR_CANCELLED;
										goto abort;
									}
								}
							}
				}

				BOOST_LOG(ERR, L"end downloading. Downloading time is " << timer.getPeriod() << L". Data size is " << output_.size() << L"reserved size is " << output_.capacity());

				if(IOCallback)
					TrafficInfo->refresh(0);

				break;
			}
		}

			setBreakable(oldBrk);
//			ocode              = code;
			oecode             = ErrorCode;
			CurrentState       = oState;

			data_peer_.close();

			if ( getreply(false) == RPL_ERROR ||
				oldtype && !SetType(oldtype) ) {
					lostpeer();
			} else {
//				code      = ocode;
				ErrorCode = oecode;
			}

			if(fout.Handle)
			{
//				ni.Starting = FALSE;
//				ni.Success  = TRUE;
				//TODO	  FTPNotify().Notify( &ni );
			}
			return RPL_OK;

abort:
			setBreakable(oldBrk);
			if ( !cpend )
			{
				BOOST_LOG(INF, L"!!!cpend");
			}

//			ocode        = code;
			oecode       = ErrorCode;
			CurrentState = oState;

//TODO 			if ( !SendAbort(din) ||
// 				(oldtype && !SetType(oldtype)) )
// 				lostpeer();
// 			else {
// //				code      = ocode;
// 				ErrorCode = oecode;
// 			}

			data_peer_.close();

			if(fout.Handle)
			{
//				ni.Starting = FALSE;
//				ni.Success  = FALSE;
				//TODO	  FTPNotify().Notify( &ni );
			}

			return RPL_ERROR;
}

/* abort using RFC959 recommended ffIP,SYNC sequence  */
/*
  WarFTP
  |->‚ABOR
  |<-226 Transfer complete. 39600000 bytes in 18.84 sec. (2052.974 Kb/s)
  |<-225 ABOR command successful.

  FreeBSD, SunOS
  |->ÚABOR
  |<-426 Transfer aborted. Data connection closed.
  |<-226 Abort successful

  QNX
  |->‚ABOR
  |<-226 Transfer complete.
  |<-225 ABOR command successful.

  IIS
  |->‚ABOR
  |<-225 ABOR command successful.

  QNX
  |->ÚABOR
  |<-452 Error writing file: No child processes.
  |<-225 ABOR command successful.

  GuildFTP (have no ABOR notify at all)
  -> <ABORT>
  <- 226 Transfer complete. 11200000 bytes in 2 sec. (5600.00 Kb/s).

  ??
  ->ÚABOR
  <-450 Transfer aborted. Link to file server lost.
  <-500 ˇÙˇÚABOR not understood
*/
bool Connection::SendAbort(TCPSocket &din)
{
/* TODO
	do
	{
		std::string s;
		s = cffIAC;
		s += cffIP;
		fputsSocket(cmd_socket_, s);

		unsigned char msg = cffIAC;
		/ * send ffIAC in urgent mode instead of DM because UNIX places oob mark * /
		/ * after urgent byte rather than before as now is protocol            * /
		nb_send(cmd_socket_, reinterpret_cast<char*>(&msg), 1, MSG_OOB); // TODO OOB

		s = cffDM;
		s += "ABOR\r\n";

		fputsSocket(cmd_socket_, s);

		data_peer_.close();
		int res;

		BOOST_LOG(INF, L"Wait ABOT reply");
		res = getreply(false);
		if ( res == ReplyTimeOut)
		{
			BOOST_LOG(ERR, L"Error waiting first ABOR reply");
			return FALSE;
		}

		//Single line
		if(true / *code == DataConnectionOpen* /)
		{
			BOOST_LOG(INF, L"Single line ABOR reply");
			return TRUE;
		} else
			//Wait OPTIONAL second line
			if(ClosingDataConnection == 226)
			{
				BOOST_LOG(INF, L"Wait OPT second reply");
				do{
					res = getreply(500);
					if ( res == ReplyTimeOut)
					{
//						BOOST_LOG(INF, L"Timeout: res: " << res << ", code: " << code);
						return TRUE;
					} else
						if ( res == RPL_ERROR )
						{
//							BOOST_LOG(ERR, L"Error: res: " << res << ", code: " << code);
							return FALSE;
						}
//						BOOST_LOG(INF, "Result: res: " << res << ", code: " << code);
				}while(1);
			} else {
				//Wait second line
				BOOST_LOG(INF, L"Wait second reply");

				res = getreply(false);
				if ( res == ReplyTimeOut)
				{
					BOOST_LOG(ERR, L"Error waiting second ABOR reply");
					return FALSE;
				}
//				BOOST_LOG(INF, L"Second reply: res: " << res << ", code: " << code);
				return TRUE;
			}

	}while(0); / *Error sending ABOR* /
*/
	return FALSE;
}
