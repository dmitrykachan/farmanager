#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

/*  1 -       title
    2 -       --- betweeen commands and status
    3 - [OPT] status
    4 - [OPT] --- between status and buttons
              commands
    5 - [OPT] --- betweeen commands and status
    6 - [OPT] button
    7 - [OPT] button1
    8 - [OPT] button2
    9 - [OPT] button Cancel
*/
const int NBUTTONSADDON = 10;

const wchar_t* horizontalLine = L"\x1";

std::wstring WINAPI FixFileNameChars(std::wstring s, bool  slashes)
{  
	if(g_manager.opt.InvalidSymbols.empty()) 
		return s;
	std::wstring::const_iterator old_itr = g_manager.opt.InvalidSymbols.begin();
	std::wstring::const_iterator new_itr = g_manager.opt.CorrectedSymbols.begin();

	while(old_itr != g_manager.opt.InvalidSymbols.end())
	{
		std::replace(s.begin(), s.end(), *old_itr, *new_itr);
		++old_itr;
	}
	if(slashes)
	{
		std::replace(s.begin(), s.end(), L'\\', L'_');
		std::replace(s.begin(), s.end(), L':', L'!');
	}
	return s;
}

/*
    Show `Message` with attention caption and query user to select YES or NO
    Returns nonzero if user select YES
*/
bool WINAPI IsCmdLogFile(void)
{
	return !g_manager.opt.CmdLogFile.empty();
}

std::wstring WINAPI GetCmdLogFile()
{  
	static std::wstring str;
	HMODULE m;

	if(!str.empty())
		return str;

	if(g_manager.opt.CmdLogFile.empty())
		return str;

	wchar_t buffer[_MAX_PATH+1] = {0};
	Utils::safe_wcscpy(buffer, g_manager.opt.CmdLogFile);

	if(PathStripToRootW(buffer))
	{
		str = g_manager.opt.CmdLogFile;
		return str;
	}

	m = GetModuleHandleW(L"ftp.dll");
	buffer[GetModuleFileNameW(m, buffer, sizeof(buffer)/sizeof(*buffer))] = 0;
	str = buffer;
	size_t n = str.rfind('\\');
	str.resize(n+1);
	str += g_manager.opt.CmdLogFile;
	return str;
}
/*
   Writes one string to log file

   Start from end if file at open bigger then limit size
   `out` is a direction of string (server, plugin, internal)
*/
void WINAPI LogCmd(const std::wstring &src, CMDOutputDir out)
{
	static HANDLE LogFile = NULL;
	//File opened and fail
	if(LogFile == INVALID_HANDLE_VALUE)
		return;

	//Params
	if(!IsCmdLogFile() || src.empty())
		return;

	//Open file
	std::wstring m = GetCmdLogFile();
	if(m.empty())
		return;

	//Open
	LogFile = Fopen(m.c_str(), !LogFile && g_manager.opt.TruncateLogFile ? L"w" : L"w+" );
	if(!LogFile)
	{
		LogFile = INVALID_HANDLE_VALUE;
		return;
	}

	//Check limitations
	if(g_manager.opt.CmdLogLimit &&
		Fsize(LogFile) >= (__int64)g_manager.opt.CmdLogLimit*1000)
		Ftrunc(LogFile,FILE_BEGIN);

	//-- RAW
	if(out == ldRaw)
	{
		//TODO Write PluginNumber
		if (src.find_first_of(L"\n\r") != std::wstring::npos)
		{
			Fwrite(LogFile, L"--- RAW ---\r\n", 13*sizeof(wchar_t));
			Fwrite(LogFile, src.c_str(), src.size()*sizeof(wchar_t));
			Fwrite(LogFile, "\r\n--- RAW ---\r\n",15*sizeof(wchar_t));
		}
		Fclose(LogFile);
		return;
	}

	SYSTEMTIME st;
	GetLocalTime(&st);

	//Write multiline to log
	std::wstring ws = src;
	while(!ws.empty())
	{
		//TODO Write PluginNumber
		//Time
		std::wstringstream stream;
		stream << std::setfill(L'0') 
			<< std::setw(4) << st.wYear << L'.'
			<< std::setw(2) << st.wMonth << L'.'
			<< std::setw(2) << st.wDay << L' '
			<< std::setw(2) << st.wHour << L':'
			<< std::setw(2) << st.wMinute << L':'
			<< std::setw(2) << st.wSecond;
		Fwrite(LogFile, stream.str().c_str(), stream.str().size()*sizeof(wchar_t));

		//Direction
		if ( out == ldInt )
			Fwrite(LogFile, L"| ", 2*sizeof(wchar_t));
		else
			Fwrite(LogFile, (out == ldOut) ? L"|->" : L"|<-", 3*sizeof(wchar_t));

		//Message
		size_t pos = ws.find_first_of(L"\n\r");
		if(pos != std::wstring::npos)
			Fwrite(LogFile, ws.c_str(), pos*sizeof(wchar_t));
		else
			Fwrite(LogFile, ws.c_str(), ws.size()*sizeof(wchar_t));

		Fwrite(LogFile, L"\n", sizeof(wchar_t));

		pos = ws.find_first_not_of(L"\n\r", pos);
		if(pos == std::wstring::npos)
			break;
		ws.assign(ws.begin() + pos, ws.end());
	};

	Fclose(LogFile);
}

/*
   Initialize CMD buffer data
*/
void Connection::InitCmdBuff()
{
	cmdBuff_.resize(0);
	cmdMsg_.resize(0);

	CmdVisible		= true;
	retryCount_		= 0;
	
	//Command lines + status command
	cmdMsg_.reserve(g_manager.opt.CmdCount + NBUTTONSADDON);
}

void Connection::pushCmdLine(const std::wstring &src, CMDOutputDir direction)
{
	std::wstring result = src;

	switch(direction)
	{
	case ldIn:
		result = L"<- " + result;
		break;
	case ldOut:
		result = L"-> " + result;
	}

	if(direction == ldRaw)
	{
		size_t n, start = 0;
		std::wstring s;
		do 
		{
 			n = result.find_first_of(L"\n\r", start);
			s = std::wstring(result, start, (n == std::wstring::npos)? n : n-start);
			size_t ofs = 0;
			do 
			{
				cmdBuff_.push_back(std::wstring(s, ofs, g_manager.opt.CmdLine-4));
				ofs += g_manager.opt.CmdLine-4;
			} while(ofs < s.size());
			if(n == std::wstring::npos)
				break;
			start = result.find_first_not_of(L"\n\r", n+1);
		} while(start != std::wstring::npos);
	} else
	{
		size_t n = std::min(result.find_first_of(L"\n\r"),
							static_cast<size_t>(g_manager.opt.CmdLine-4));
		n = std::min(result.size(), n);
		result.resize(n);
		cmdBuff_.push_back(result);
	}
}


void Connection::AddCmdLine(const std::wstring &str, CMDOutputDir direction)
{
	std::wstring buff;

	std::wstring::const_iterator itr = str.begin();
	if(str.find_first_not_of(L"\r\n") == std::wstring::npos)
	{
		BOOST_ASSERT(0); // TODO why it is possible
		return;
	}

	buff = str;

	const size_t cmdSize = g_manager.opt.CmdCount;
	//Remove overflow lines
	if(cmdBuff_.size() >= cmdSize)
		cmdBuff_.erase(cmdBuff_.begin(), cmdBuff_.begin() + (cmdBuff_.size()-cmdSize+1));

	//Add new line
	LogCmd(buff, direction);

	pushCmdLine(buff, direction);
	//Remove overflow lines
	if(cmdBuff_.size() > cmdSize)
		cmdBuff_.erase(cmdBuff_.begin(), cmdBuff_.begin() + (cmdBuff_.size()-cmdSize));

	//Start
	if(startReply_.empty() && direction == ldIn)
	{
		size_t start = str.find_first_not_of(L"0123456789 -");
		if(start == std::wstring::npos)
			start = 0;
		size_t end = str.find_first_of(L"\n\r", start);
		if(end == std::wstring::npos)
			end = str.size();
		startReply_.assign(str, start, end-start);
	}

	ConnectMessage();
}

void Connection::AddCmdLineOem(const std::string& src, CMDOutputDir direction)
{
	std::wstring str;

	char ffIacIp[] = {cffIAC, cffIP, 0};
	if(boost::starts_with(src, ffIacIp))
		str = L"<IP>";
	else
		if(!src.empty() && src[0] == cffDM && src.find("ABOR", 1, 4) != std::string::npos)
			str = L"<ABORT>";
		else
			str = FromOEM(src);

	AddCmdLine(str, direction);
}

int Connection::ConnectMessage(std::wstring message, bool messageTime,
								const std::wstring &title, const std::wstring &bottom, 
								bool showcmds, bool error,
								int button1, int button2, int button3)
{
	cmdMsg_.resize(0);
	cmdMsg_.push_back(title);

	if(messageTime)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		std::wstringstream stream;
		stream << std::setfill(L'0') << std::setw(2) << st.wHour << L':' << 
			std::setw(2) << st.wMinute << L':' << std::setw(2) << st.wSecond <<
			L" " << message;
		message = stream.str();
	};
	message.resize(g_manager.opt.CmdLine-4, L' ');
	cmdMsg_.push_back(message);

	if(showcmds)
	{
		cmdMsg_.push_back(horizontalLine);

		std::vector<std::wstring>::const_iterator itr = cmdBuff_.begin();
		while(itr != cmdBuff_.end())
		{
			cmdMsg_.push_back(*itr);
			++itr;
		}
		for(size_t n = cmdBuff_.size(); n < static_cast<size_t>(g_manager.opt.CmdCount); ++n)
			cmdMsg_.push_back(L"");
	}

	if(!bottom.empty())
	{
		cmdMsg_.push_back(horizontalLine);
		cmdMsg_.push_back(bottom);
	}

	int buttonCount = 0;
	if(button1 != MNone__)
	{
		cmdMsg_.push_back(horizontalLine);
		cmdMsg_.push_back(getMsg(button1));
		++buttonCount;
		if(button2 != MNone__)
		{
			cmdMsg_.push_back(getMsg(button2));
			++buttonCount;
			if(button3 != MNone__)
			{
				cmdMsg_.push_back(getMsg(button3));
				++buttonCount;
			}
		}
	}

	FARWrappers::Screen scr;
	return FARWrappers::message(cmdMsg_, buttonCount, (error? FMSG_WARNING : 0) | FMSG_LEFTALIGN);	
}

bool Connection::ConnectMessageTimeout(int Msg, const std::wstring& hostname, int BtnMsg)
{  
	bool	first = true;
	int		secNum;

	if (is_flag(FP_LastOpMode, OPM_FIND) || !g_manager.opt.RetryTimeout)
		return ConnectMessage(Msg, hostname, BtnMsg);

	secNum = 0;
	WinAPI::Stopwatch stopwatch(1000);
	do
	{
		static const WORD Keys[] = { VK_ESCAPE, 'C', 'R', VK_RETURN };

		switch(WinAPI::checkForKeyPressed(Keys))
		{
		 case 0: case 1:
			 return false;
		 case 2:
		 case 3:
			 return true;
		}

		if(first || stopwatch.isTimeout())
		{
			first = false;
			std::wstring s = std::wstring(L" ") + getMsg(MAutoRetryText) + L" " +
				boost::lexical_cast<std::wstring>(g_manager.opt.RetryTimeout-secNum) +
				getMsg(MSeconds);

			ConnectMessage(std::wstring(getMsg(Msg)) + s, false, hostname, 
						getMsg(MRetryText), true, true);

			WinAPI::setConsoleTitle(std::wstring(getMsg(Msg)) + s);

			secNum++;
			stopwatch.reset();
		}

		if(secNum > g_manager.opt.RetryTimeout)
			return false;

		Sleep(100);
	}while( 1 );

	return false;
}

int Connection::ConnectMessage(std::wstring msg, std::wstring subtitle, bool error,
							   int btn, int btn1, int btn2)
{
	PROCP(msg << L", " << subtitle << L", " << btn << L", " << btn1 << L", " << btn2); 
	int			res;
	bool		exCmd;

	if(error && g_manager.opt.DoNotExpandErrors)
		exCmd = false;
	else
		exCmd = getHost()->ExtCmdView;

	//Called for update CMD window but it disabled
	if(msg.empty() && subtitle.empty() && !exCmd)
		return true;

	if(subtitle.empty())
	{
		if(!userName_.empty())
		{
			subtitle = userName_ + L':';
			if (!userPassword_.empty())
				subtitle += L"*@";
		}
		subtitle += getHost()->url_.Host_;
	}

	if(btn == MNone__)
		if ( is_flag(FP_LastOpMode,OPM_FIND) ||  //called from find
			!CmdVisible ||  //Window disabled
			(IS_SILENT(FP_LastOpMode) && !g_manager.opt.ShowSilentProgress) )
		{ //show silent processing disabled
			if(!msg.empty())
				IdleMessage(msg.c_str(), g_manager.opt.ProcessColor );
			return false;
		}

	//Title
	std::wstring title = getMsg(MFtpTitle) + std::wstring(L" \"") + subtitle + L'\"';

	if(!msg.empty())
		lastMsg_ = msg;

	//Display error in title
	if(error && FARWrappers::Screen::isSaved())
	{
		WinAPI::setConsoleTitle(lastMsg_ + L" \"" + title + L"\"");
	}

	res = ConnectMessage(lastMsg_, true, title, L"", exCmd, error, btn, btn1, btn2);

//SLEEP!!	Sleep(1000);

	//If has user buttons: return number of pressed button
	if(btn1 != MNone__)
		return res;

	//If single button: return TRUE if button was selected
	return btn != MNone__ && res == 0;
}


int Connection::ConnectMessage(int Msg, std::wstring subtitle, bool error,
							   int btn, int btn1, int btn2)
{
	return ConnectMessage(Msg == MNone__? L"" : getMsg(Msg), subtitle, error, btn, btn1, btn2);
}

void WINAPI OperateHidden(const std::wstring& fnm, bool set)
{
	if(!g_manager.opt.SetHiddenOnAbort)
		return;
	BOOST_LOG(INF, (set ? L"Set" : L"Clr") << L" hidden");

	DWORD dw = GetFileAttributesW(fnm.c_str());
	if(dw == INVALID_FILE_ATTRIBUTES)
		return;

	set_flag(dw, FILE_ATTRIBUTE_HIDDEN, set);

	SetFileAttributesW(fnm.c_str(), dw);
}
