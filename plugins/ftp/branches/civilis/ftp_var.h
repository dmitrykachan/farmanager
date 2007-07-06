#ifndef __FAR_PLUGIN_FTP_VARS
#define __FAR_PLUGIN_FTP_VARS

/* The following defines are from ftp.h and telnet.h from bsd.h */
/* All relevent copyrights below apply.                         */

#define ffIAC     255
#define ffDONT    254
#define ffDO      253
#define ffWONT    252
#define ffWILL    251
#define ffIP      244
#define ffDM      242

#define ffEOF     0x0236000


enum RPL_RESULT
{
	RPL_OK			= 0, 
	RPL_PRELIM		= 1,
	RPL_COMPLETE	= 2,
	RPL_CONTINUE	= 3,
	RPL_TRANSIENT	= 4,
	RPL_BADPASS		= 5,
	RPL_ERROR		= -1,
	RPL_TRANSFERERROR = -2,

	RPL_350			= 350



};
#define RPL_TIMEOUT        3000

#endif
