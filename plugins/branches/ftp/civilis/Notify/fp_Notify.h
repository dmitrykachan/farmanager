#ifndef __FTP_PLUGIN_NOTIFY
#define __FTP_PLUGIN_NOTIFY

//------------------------------------------------------------------------
/*
  IO notify
*/

#define FTP_NOTIFY_MAGIC  MK_ID( 'F','n','t','f' )

struct FTNNotify
{
  __int64           RestartPoint;
  BOOL            Upload;
  BOOL            Starting;
  BOOL            Success;
  char            HostName[ MAX_PATH_SIZE ];
  std::string		username_;
  char            Password[ MAX_PATH_SIZE ];
  WORD            Port;
  char            LocalFile[ MAX_PATH_SIZE ];
  char            RemoteFile[ MAX_PATH_SIZE ];
};

struct NotifyInterface : public FTPPluginInterface
{
  void     (DECLSPEC *Notify )( const FTNNotify* p );
};

#endif