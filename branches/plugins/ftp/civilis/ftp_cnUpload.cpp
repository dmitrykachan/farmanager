#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"

//--------------------------------------------------------------------------------
void Connection::sendrequest(const std::wstring &cmd, const std::wstring &local, const std::wstring &remote)
{  //??SaveConsoleTitle _title;

     sendrequestINT(cmd,local,remote );
     IdleMessage( NULL,0 );
}

void Connection::sendrequestINT(const std::wstring &cmd, const std::wstring &local, const std::wstring &remote)
{
	PROCP(L"cmd: " << cmd << L"local: " << local << L"remote" << remote);

	if ( type == TYPE_A )
		restart_point=0;

	WIN32_FIND_DATAW	ffi;
	FHandle				fin;
	LONG				hi;
	FTPCurrentStates	oState = CurrentState;
	bool				oldBrk = getBreakable();
	__int64				fsz;

	HANDLE ff = FindFirstFileW(local.c_str(), &ffi);
	if(ff == INVALID_HANDLE_VALUE)
	{
		ErrorCode = ERROR_OPEN_FAILED;
		return;
	}
	FindClose(ff);

	if ( is_flag(ffi.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) ) {
		ErrorCode = ERROR_OPEN_FAILED;
		BOOST_LOG(INF, "local is directory: " << local);
		if(!ConnectMessage(MNotPlainFile, local, true, MRetry ))
			ErrorCode = ERROR_CANCELLED;
		return;
	}

	fin.Handle = Fopen(local.c_str(), L"r");
	if ( !fin.Handle )
	{
		BOOST_LOG(INF, L"!open local");
		ErrorCode = ERROR_OPEN_FAILED;
		if(!ConnectMessage( MErrorOpenFile, local, true, MRetry))
			ErrorCode = ERROR_CANCELLED;
		return;
	}
	fsz = Fsize(fin.Handle);

	if ( restart_point && fsz == restart_point) {
		AddCmdLine( getMsg(MFileComplete) );
		ErrorCode = ERROR_SUCCESS;
		return;
	}

	in_addr data_addr;
	int		port;
	if(!initDataConnection(data_addr, port))
	{
		BOOST_LOG(INF, L"!initconn");
		return;
	}

	if(getHost().SendAllo)
	{
		if(cmd[0] != g_manager.opt.cmdAppe[0])
		{
			unsigned __int64 v = ((__int64)ffi.nFileSizeHigh) * MAXDWORD + ffi.nFileSizeLow;
			BOOST_LOG(INF, L"ALLO " << v);
			if(command(g_manager.opt.cmdAllo + boost::lexical_cast<std::wstring>(v)) != RPL_COMPLETE)
			{
				BOOST_LOG(INF, "!allo");
				return;
			}
		}
	}

	if ( restart_point ) {
		if(!resumeSupport_ && cmd[0] != g_manager.opt.cmdAppe[0] )
		{
			AddCmdLine( getMsg(MResumeRestart) );
			restart_point = 0;
		} else {
			BOOST_LOG(INF, L"restart_point " << restart_point);

			if ( !Fmove( fin.Handle, restart_point ) ) {
				ErrorCode = GetLastError();
				BOOST_LOG(INF, L"!setfilepointer: " << restart_point);
				if ( !ConnectMessage( MErrorPosition, local, true, MRetry ) )
					ErrorCode = ERROR_CANCELLED;
				return;
			}

			if ( cmd[0] != g_manager.opt.cmdAppe[0] &&
				command(g_manager.opt.cmdRest_ + L" " + boost::lexical_cast<std::wstring>(restart_point)) != RPL_CONTINUE)
				return;

			TrafficInfo->Resume( restart_point );
		}
	}

	if(getHost().PassiveMode)
	{
		BOOST_LOG(INF, L"pasv");
		if(!dataconn(data_addr, port))
			goto abort;
	}

	if (!remote.empty()) 
	{
		BOOST_LOG(INF, "Upload remote: " << remote);
		if(!isPrelim(command(cmd + L" " + remote)))
			return;
	} else
	{
		BOOST_LOG(INF, L"!remote");
		if(isPrelim(command(cmd)))
		{
			return;
		}
	}

	if( !getHost().PassiveMode )
	{
		if(!dataconn(data_addr, port))
			goto abort;
	}

	switch (type) {
	case TYPE_I:
	case TYPE_L:
	case TYPE_A: if ( fsz != 0 ) {

		// TODO FTPNotify().Notify( &ni );

		BOOST_LOG(INF, L"Uploading " << local << L"->" << remote << L" from " << restart_point);

		FTPConnectionBreakable _brk( this, false);
		//-------- READ
		while( 1 ) {

			hi = 0;
			BOOST_LOG(INF, L"read " << getHost().IOBuffSize);
			if ( !ReadFile(fin.Handle,IOBuff.get(), getHost().IOBuffSize,(LPDWORD)&hi,NULL) )
			{
				BOOST_LOG(INF, L"pf: !read buff");
				ErrorCode = GetLastError();
				goto abort;
			}

			if ( hi == 0 ) {
				ErrorCode = GetLastError();
				break;
			}

			//-------- SEND
			LONG  d = 0;
			char *bufp;

			BOOST_LOG(INF, L"doSend");
			for ( bufp = IOBuff.get(); hi > 0; hi -= d, bufp += d) {

				BOOST_LOG(INF, L"ndsend " << hi);
//TODO 				if ( (d=(LONG)nb_send(&dout, bufp,(int)hi, 0)) <= 0 ) {
// 					BOOST_LOG(INF, L"pf(" << code << L"," << GetSocketErrorSTR() << L"): !send " << d << "!=" << hi);
// 					code = RPL_TRANSFERERROR;
// 					goto abort;
// 				}
				BOOST_LOG(INF, L"sent " << d);

				if (IOCallback && !TrafficInfo->Callback( (int)d ) ) {
//					BOOST_LOG(INF, L"pf(" << code << L"," << GetSocketErrorSTR() << L" ): canceled");
					ErrorCode = ERROR_CANCELLED;
					goto abort;
				}
			}//-- SEND
			BOOST_LOG(INF, L"sended");

			//Sleep(1);

		}//-- READ
		if ( IOCallback ) TrafficInfo->Callback(0);
		BOOST_LOG(INF, L"done");

				 } /*fsz != 0*/
				 break;
	}/*SWITCH*/

	//NormExit
	setBreakable(oldBrk);
	CurrentState = oState;
	if(data_peer_.isCreated())
	{
		data_peer_.close();

		if ( getreply(false) > RPL_COMPLETE ) {
			ErrorCode = ERROR_WRITE_FAULT;
		}
	} else
		getreply(false);

// 	ni.Starting = FALSE;
// 	ni.Success  = TRUE;
	// TODO		FTPNotify().Notify( &ni );
	return;

abort:
	setBreakable(oldBrk);
	CurrentState = oState;

	if ( !cpend ) {
		BOOST_LOG(INF, L"!!!cpend");
	}

	int ocode = 0; //code,
	int	oecode = ErrorCode;

	data_peer_.shutdown(SD_SEND);
	data_peer_.close();

	if ( !SendAbort(data_peer_) ) {
		BOOST_LOG(INF, L"!send abort");
		lostpeer();
	} else {
		setascii();
		pwd();
//		code      = ocode;
		ErrorCode = oecode;
	}

// 	ni.Starting = FALSE;
// 	ni.Success  = FALSE;
	//TODO	FTPNotify().Notify( &ni );
}
