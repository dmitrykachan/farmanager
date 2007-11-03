#include "stdafx.h"
#pragma hdrstop

#include "ftp_Int.h"
#include "farwrapper/dialog.h"

static std::wstring MkFileInfo(const wchar_t* title, const FTPFileInfo* p)
{
	SYSTEMTIME tm;

	std::wstringstream ws;

	ws << std::setw(20) << std::setiosflags(std::ios::left) << title;

	if(p) 
	{
		//Size
		ws << std::setw(14) << std::setiosflags(std::ios::right) << p->getFileSize() << L' ';

		//Time
		FileTimeToSystemTime(&p->getLastWriteTime(), &tm);
		ws	<< std::setfill(L'0') << std::setw(2) << tm.wDay << L'.'
			<< std::setw(2) << tm.wMonth  << L'.'
			<< std::setw(4) << tm.wYear   << L'.'
			<< std::setw(2) << tm.wHour   << L':'
			<< std::setw(2) << tm.wMinute << L':'
			<< std::setw(2) << tm.wSecond;
	}
	return ws.str();	
}


overCode FTP::AskOverwrite(int title, BOOL Download, FTPFileInfo* dest, const FTPFileInfo* src,overCode last)
{
	if ( last == ocOverAll || last == ocSkipAll )
		return last;

	if (getConnection().isResumeSupport() && last == ocResumeAll )
		return last;

	FARWrappers::Dialog dlg(L"", FDLG_WARNING);
	int resumeButtonFlag = (Download && !getConnection().isResumeSupport())? DIF_DISABLE : 0;

	dlg.addDoublebox( 3, 1, 66, 11,			getMsg(title))->
		addLabel	( 5, 2,					getMsg(MAlreadyExist))->
		addLabel	( 6, 3,					dest->getFileName())->
		addLabel	( 5, 4,					getMsg(MAskOverwrite))->
		addHLine	( 3, 5)->
		addLabel	( 5, 6,					MkFileInfo(getMsg(MBtnCopyNew), src))->
		addLabel	( 5, 7,					MkFileInfo(getMsg(MBtnCopyExisting), dest))->
		addHLine	( 3, 8)->
		addDefaultButton( 5, 9,	ocOver,		getMsg(MBtnOverwrite))->
		addButton	(21, 9,	ocSkip,			getMsg(MBtnCopySkip))->
		addButton	(37, 9,	ocResume,		getMsg(MBtnCopyResume), resumeButtonFlag)->
		addButton	(53, 9,	ocCancel,		getMsg(MBtnCopyCancel))->
		addButton	( 5,10,	ocOverAll,		getMsg(MBtnOverwriteAll))->
		addButton	(21,10,	ocSkipAll,		getMsg(MBtnCopySkipAll))->
		addButton	(37,10,	ocResumeAll,	getMsg(MBtnCopyResumeAll), resumeButtonFlag);

	int rc = dlg.show(70, 13);

	longBeep_.reset();

	BOOST_ASSERT(rc != 0);
	return (rc == -1)? ocCancel : static_cast<overCode>(rc);
}
