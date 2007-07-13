#include <all_far.h>
#pragma hdrstop

#include "fstdlib.h"

PPluginStartupInfo          FP_Info          = NULL;
PFarStandardFunctions       FP_FSF           = NULL;
std::wstring				g_pluginStartPath;
std::wstring				g_PluginRootKey;
OSVERSIONINFO              *FP_WinVer        = NULL;
HMODULE                     FP_HModule       = NULL;
int                         FP_LastOpMode    = 0;
BOOL                        FP_IsOldFar;
DWORD                       FP_WinVerDW;
#if !defined(USE_ALL_LIB)
PHEX_DumpInfo               HEX_Info         = NULL;
#endif

static void RTL_CALLBACK idAtExit( void )
{
     delete FP_Info;                FP_Info            = NULL;
     delete FP_FSF;                 FP_FSF             = NULL;
     delete FP_WinVer;              FP_WinVer          = NULL;

     FP_HModule = NULL;
}

void DECLSPEC FP_SetStartupInfo( const PluginStartupInfo *Info, const std::wstring& plugginName)
{
//Info
    FP_HModule = GetModuleHandle( FP_GetPluginName() );
    FP_Info    = new PluginStartupInfo;
    MemCpy( FP_Info,Info,sizeof(*Info) );

//FSF
    FP_IsOldFar = TRUE;
    if( Info->StructSize >= FAR_SIZE_170 ) {
#if !defined(__USE_165_HEADER__)
      FP_FSF = new FarStandardFunctions;
      MemCpy( FP_FSF,Info->FSF,sizeof(*FP_FSF) );
#endif
      FP_IsOldFar = FALSE;
    }

//Version
    FP_WinVer = new OSVERSIONINFO;
    FP_WinVer->dwOSVersionInfoSize = sizeof(*FP_WinVer);
    GetVersionEx( FP_WinVer );

    FP_WinVerDW = GetVersion();

//Plugin Reg key
	g_PluginRootKey = Unicode::fromOem(Info->RootKey) +	L'\\' + plugginName;


//ExcDUMP
#if !defined(USE_ALL_LIB)
    HMODULE md;
	std::wstring Path;

    //ExcDump
	do{
		md = LoadLibrary( "ExcDump.dll" );
		if ( md ) break;

		//Start path
		wchar_t startPath[MAX_PATH_SIZE]; 
		startPath[ GetModuleFileNameW(FP_HModule, startPath,sizeof(startPath)/sizeof(*startPath))] = 0;
		g_pluginStartPath = startPath;
		g_pluginStartPath.resize(g_pluginStartPath.find(L"\\")-1);
		Path = g_pluginStartPath + L"\\ExcDump.dll";
		md = LoadLibraryW(Path.c_str());
		if (md) break;
		break;

	}while( 0 );

    HEX_Info = NULL;

    if ( md ) {
      HT_QueryInterface_t p = (HT_QueryInterface_t)GetProcAddress( md,"HEX_QueryInterface" );
      if ( !p || (HEX_Info=p()) == NULL )
        FreeLibrary( md );
    }
#endif
}

#if defined(USE_ALL_LIB)
static char LogFileName[ MAX_PATH_SIZE ];

static CONSTSTR RTL_CALLBACK idGetLog( void ) { return LogFileName; }

void SetLogProc( void )
  {  CONSTSTR m = FP_GetPluginLogName();

     if ( !m || !m[0] ) {
       LogFileName[0] = 0;
       return;
     }

     SetLogNameProc( idGetLog );

     if ( IsAbsolutePath(m) ) {
       TStrCpy( LogFileName, m );
       return;	
     }

     LogFileName[ GetModuleFileName( FP_HModule, LogFileName, sizeof(LogFileName) ) ] = 0;
     strcpy( strrchr(LogFileName,'\\')+1, m );
}
#else
#define SetLogProc()
#endif

#if defined(__BORLAND)
BOOL WINAPI DllEntryPoint( HINSTANCE hinst, DWORD reason, LPVOID ptr )
  {
    if ( reason == DLL_PROCESS_ATTACH ) {
      FP_HModule = GetModuleHandle( FP_GetPluginName() );
      SetLogProc();
      AtExit( idAtExit );
    }

    BOOL res = FP_PluginStartup(reason);

    if ( reason == DLL_PROCESS_DETACH ) {
      CallAtExit();
    }

 return res;
}
#else
#if defined(__MSOFT)
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID ptr )
  {
    if ( reason == DLL_PROCESS_ATTACH ) {
      FP_HModule = GetModuleHandle( FP_GetPluginName() );
      SetLogProc();
      AtExit( idAtExit );
    }

    BOOL res = FP_PluginStartup(reason);

    if ( reason == DLL_PROCESS_DETACH ) {
      CallAtExit();
    }

 return res;
}
#else
  #error "Define plugin DLL entry point procedure for your  compiller"
#endif
#endif
