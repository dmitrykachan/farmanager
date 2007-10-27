#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

//---------------------------------------------------------------------------------
FTPCmdBlock::FTPCmdBlock( FTP *c,int block )
  {
    Handle = c;
    hVis   = -1;
    Block( block );
}
FTPCmdBlock::~FTPCmdBlock()
{
   Reset();
}
void FTPCmdBlock::Block( int block )
{
    if ( Handle && block != -1 )
      hVis = FtpCmdBlock(&Handle->getConnection(), block);
}
void FTPCmdBlock::Reset( void )
{
    Block( hVis );
    hVis = -1;
}
