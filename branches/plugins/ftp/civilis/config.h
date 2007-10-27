#ifndef FAR_FTP_PLUGIN_CONFIG_H
#define FAR_FTP_PLUGIN_CONFIG_H

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#define _AFX_SECURE_NO_DEPRECATE
#define _ATL_SECURE_NO_DEPRECATE

#pragma warning(disable : 4996)
#pragma warning( disable : 4800 )  //Disable: 'int' : forcing value to bool 'true' or 'false' (performance warning)

#endif

#define __NOMEM__ 1
#define __HEAP_MEMORY__ 1
#define __USEASSERT__

#define NOMINMAX

#ifdef _DEBUG                 //VC debug flag
#undef __DEBUG__
#define __DEBUG__ 1
#endif


#endif