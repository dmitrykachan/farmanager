#include "stdafx.h"
#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

#include "utils/utf8.h"

#include "utils/uniconverts.h"
using namespace Unicode;

// ------------------------------------------------------------------------
// FTPNotify
// ------------------------------------------------------------------------
void FTPNotify::Notify( const FTNNotify* p ) { Interface()->Notify( p ); }

//------------------------------------------------------------------------
static FTPInterface Interface;

//------------------------------------------------------------------------
HANDLE DECLSPEC idProcStart( CONSTSTR FunctionName,CONSTSTR Format,... )
  {  String str;
     va_list argptr;

     va_start( argptr,Format );
       str.vprintf( Format,argptr );
     va_end( argptr );

 return new FARINProc( FunctionName,str.c_str() );
}

void           DECLSPEC idProcEnd( HANDLE proc )    { delete ((FARINProc*)proc); }
OptionsPlugin* DECLSPEC idGetOpt()					{ return &g_manager.opt; }

int DECLSPEC idFtpCmdBlock( int block /*TRUE,FALSE,-1*/ )
  {  FTP *ftp = LastUsedPlugin;
     if ( !ftp || !ftp->hConnect ) return -1;
  return FtpCmdBlock( ftp->hConnect,block );
}

int DECLSPEC idFtpGetRetryCount( void )
  {  FTP *ftp = LastUsedPlugin;
     if ( !ftp || !ftp->hConnect ) return 0;
 return FtpGetRetryCount( ftp->hConnect );
}

FTPHostPlugin* DECLSPEC idGetHostOpt( void )
  {  FTP *ftp = LastUsedPlugin;
     if ( !ftp || !ftp->hConnect ) return NULL;
     return &ftp->hConnect->Host;
}

FTPPluginHolder* StdCreator(WinAPI::module m, FTPPluginInterface* Interface )
{  
	FTPPluginHolder* p = new FTPPluginHolder(m, Interface);

	return p;
}

class FTPPluginsInfo
{
private:
	unsigned long		id_;
	FTPPluginHolder*	holder_;
	FTPPluginHolder* (*Creator)(WinAPI::module m, FTPPluginInterface* Interface );
	std::wstring		fileName_;
	std::wstring		description_;
public:
	FTPPluginsInfo(unsigned int id, std::wstring fileName, std::wstring description, FTPPluginHolder *holder)
		: id_(id), fileName_(fileName), description_(description), holder_(holder)
		{};
	unsigned long	getId()	const		{return id_;}
	std::wstring	getFileName() const	{return fileName_;}
	std::wstring	getDesctipion()const{return description_;}
	FTPPluginHolder*getHolder() const	{return holder_;}
};

class PluginList
{
private:
	std::vector<FTPPluginsInfo> list_;
public:
	typedef std::vector<FTPPluginsInfo>::const_iterator const_iterator;

	bool loadStdPlugin(const std::wstring &filename, unsigned long id, const std::wstring &description);
	const_iterator find(unsigned long id);
	const_iterator begin() const {return list_.begin();}
	const_iterator end() const {return list_.end();}
	bool empty() const {return list_.empty();}
	void clear() { list_.clear();}
};

PluginList g_plugins;

PluginList::const_iterator PluginList::find(unsigned long id)
{
	return std::find_if(begin(), end(), boost::bind(&FTPPluginsInfo::getId, _1) == id);
}


void freePlugins()
{
	g_plugins.clear();
}

FTPPluginHolder* getPluginHolder(unsigned long id)
{
	PluginList::const_iterator itr;
	itr = g_plugins.find(id);
	if(itr != g_plugins.end())
		return itr->getHolder();
	else
		return 0;
}

bool isPluginAvailable(unsigned long id)
{
	return g_plugins.find(id) != g_plugins.end();
}


//------------------------------------------------------------------------
#define CH_OBJ if (!Object) Object = Interface()->CreateObject();

void FTPProgress::Resume( CONSTSTR LocalFileName )                                     { CH_OBJ Interface()->ResumeFile(Object,LocalFileName); }
void FTPProgress::Resume( __int64 size )                                                 { CH_OBJ Interface()->Resume(Object,size); }
BOOL FTPProgress::Callback( int Size )                                                 { CH_OBJ return Interface()->Callback(Object,Size); }
void FTPProgress::Init( HANDLE Connection,int tMsg,int OpMode,FP_SizeItemList* il )        { CH_OBJ Interface()->Init(Object,Connection,tMsg,OpMode,il); }
void FTPProgress::Skip( void )                                                         { CH_OBJ Interface()->Skip(Object); }
void FTPProgress::Waiting( time_t paused )                                             { CH_OBJ Interface()->Waiting(Object,paused); }
void FTPProgress::SetConnection( HANDLE Connection )                                   { CH_OBJ Interface()->SetConnection(Object,Connection); }

void FTPProgress::InitFile( PluginPanelItem *pi, CONSTSTR SrcName, CONSTSTR DestName )
  {
    __int64 sz;
    if ( pi )
      sz = ((__int64)pi->FindData.nFileSizeHigh) * ((__int64)MAX_DWORD) + ((__int64)pi->FindData.nFileSizeLow);
     else
      sz = 0;
    InitFile( sz, SrcName, DestName);
}

void FTPProgress::InitFile( WIN32_FIND_DATA* pi, CONSTSTR SrcName, CONSTSTR DestName )
  {
    __int64 sz;
    if ( pi )
      sz = ((__int64)pi->nFileSizeHigh) * ((__int64)MAX_DWORD) + ((__int64)pi->nFileSizeLow);
     else
      sz = 0;
    InitFile( sz, SrcName, DestName);
}

void FTPProgress::InitFile( __int64 sz, CONSTSTR SrcName, CONSTSTR DestName )
  {
    CH_OBJ
    Interface()->InitFile(Object,sz,SrcName,DestName);
}
