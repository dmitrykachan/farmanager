#include "stdafx.h"

#include "ftp_Int.h"

bool FTP::ProcessShortcutLine(const wchar_t *line)
{ 
	if(line == 0)
		return false;
	g_manager.readCfg();

	if(boost::istarts_with(line, L"HOST:"))
	{
		hostPanel_.setCurrentDirectory(line + 5);
		return true;
	}
	
	if(!boost::istarts_with(line, L"FTP:"))
		return false;

	line += 4;

	/*
	FTP
	AskLogin+3   AsciiMode+3  PassiveMode+3  ExtCmdView+3  FFDup + 3
	Host   1   User   1   Password   1  ServerName 1 codePage 1
	*/
	const wchar_t delimiter = L'\x1';
	const wchar_t base = L'\x3';
	const wchar_t *line_end = line + wcslen(line);

	FtpHostPtr host = FtpHostPtr(new FTPHost);
	host->Init();
	if(line_end - line < 10)
		return false;
	host->AskLogin   = *line++ - base;
	host->AsciiMode  = *line++ - base;
	host->PassiveMode= *line++ - base;
	host->ExtCmdView = *line++ - base;
	host->FFDup      = *line++ - base;

	const wchar_t *p_user = std::find(line, line_end, delimiter);
	if(p_user == line_end)
		return false;
	const wchar_t *p_pass = std::find(p_user+1, line_end, delimiter);
	if(p_pass == line_end)
		return false;
	const wchar_t *p_servname = std::find(p_pass+1, line_end, delimiter);
	if(p_servname == line_end)
		return false;
	const wchar_t *p_codePage = std::find(p_servname+1, line_end, delimiter);
	if(p_codePage == line_end)
		return false;

	host->SetHostName(std::wstring(line, p_user),
					std::wstring(p_user+1, p_pass),
					std::wstring(p_pass+1, p_servname));
	host->serverType_ = *ServerList::instance().find(std::wstring(p_servname+1, p_codePage));
	host->codePage_   = boost::lexical_cast<int>(std::wstring(p_codePage+1, line_end));

	return FullConnect(host);
}
