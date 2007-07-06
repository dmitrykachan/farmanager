#ifndef __FTP_PLUGINS_HOLDERS
#define __FTP_PLUGINS_HOLDERS

class FTPPluginHolder : boost::noncopyable
{
private:
	WinAPI::module module_;
	FTPPluginInterface* interface_;
public:
	FTPPluginHolder(WinAPI::module h, FTPPluginInterface* interface)
		: module_(h), interface_(interface)
	{};
	FTPPluginInterface* getInterface() const
	{
		return interface_;
	}

};

extern FTPPluginHolder* getPluginHolder(unsigned long id);
extern bool             isPluginAvailable(unsigned long id);

template <class Cl, unsigned long id> 
struct FTPPlugin 
{
	FTPPluginHolder* Holder;

	FTPPlugin( void )    { Holder = getPluginHolder(id); }
	virtual ~FTPPlugin() { Holder = NULL; }

	Cl Interface( void ) { Assert( Holder ); return (Cl)Holder->getInterface(); }
};


struct FTPProgress : public FTPPlugin<ProgressInterface*, FTP_PROGRESS_MAGIC> 
{
   HANDLE Object;

   FTPProgress( void ) { Object = NULL; }
   ~FTPProgress()      { if (Object) { Interface()->DestroyObject(Object); Object = NULL; } }

   void Resume( CONSTSTR LocalFileName );
   void Resume( __int64 size );
   BOOL Callback( int Size );
   void Init( HANDLE Connection,int tMsg,int OpMode,FP_SizeItemList* il );
   void InitFile( __int64 sz, CONSTSTR SrcName, CONSTSTR DestName );
   void InitFile( PluginPanelItem *pi, CONSTSTR SrcName, CONSTSTR DestName );
   void InitFile( WIN32_FIND_DATA* pi, CONSTSTR SrcName, CONSTSTR DestName );
   void Skip( void );
   void Waiting( time_t paused );
   void SetConnection( HANDLE Connection );
};

struct FTPNotify: public FTPPlugin<NotifyInterface*, FTP_NOTIFY_MAGIC>
{
   void     Notify(const FTNNotify* p);
};

#endif