#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

FTPCmdBlock::FTPCmdBlock(FTP *c, int block)
{
	handle_ = c;
	hVis   = -1;
	Block(block);
}

FTPCmdBlock::~FTPCmdBlock()
{
	Reset();
}

void FTPCmdBlock::Block(int block)
{
	if(handle_ && block != -1)
		hVis = FtpCmdBlock(&handle_->getConnection(), block);
}

void FTPCmdBlock::Reset()
{
	Block(hVis);
	hVis = -1;
}
