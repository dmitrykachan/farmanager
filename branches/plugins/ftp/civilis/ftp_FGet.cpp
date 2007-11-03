#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

/****************************************
   PROCEDURES
     FTP::GetFiles
 ****************************************/
void SetupFileTimeNDescription(int OpMode,Connection *hConnect, const std::wstring &nm,FILETIME *tm)
{  
	HANDLE SrcFile = CreateFileW(nm.c_str(), GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL );
	DWORD  FileSize;

	if ( SrcFile == INVALID_HANDLE_VALUE )
		return;

	if ( is_flag(OpMode,OPM_DESCR) &&
		(FileSize = GetFileSize(SrcFile,NULL)) != INVALID_FILE_SIZE )
	{
		boost::scoped_array<unsigned char> buf(new unsigned char[FileSize]);
		ReadFile(SrcFile, buf.get(),FileSize,&FileSize,NULL);
		BOOST_ASSERT(0 && "TODO");
		// TODO DWORD WriteSize = hConnect->toOEM( Buf,FileSize );
		DWORD WriteSize = FileSize;
		SetFilePointer(SrcFile, 0, NULL, FILE_BEGIN);
		WriteFile(SrcFile, buf.get(),WriteSize,&WriteSize,NULL );
		SetEndOfFile(SrcFile);
	}
	if(tm)
		SetFileTime(SrcFile,NULL,NULL,tm);
	CloseHandle(SrcFile);
}
