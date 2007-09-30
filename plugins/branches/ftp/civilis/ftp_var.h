#ifndef __FAR_PLUGIN_FTP_VARS
#define __FAR_PLUGIN_FTP_VARS

/* The following defines are from ftp.h and telnet.h from bsd.h */
/* All relevent copyrights below apply.                         */

const char cffIAC	= '\xff';	// 255
const char cffDONT	= '\xfe';	// 254
const char cffDO	= '\xfd';	// 253
const char cffWONT  = '\xfc';	// 252
const char cffWILL  = '\xfb';	// 251
const char cffIP	= '\xf4';	// 244
const char cffDM	= '\xf2';	// 242

enum RPL_RESULT
{
	RPL_OK			= 0, 
	RPL_PRELIM		= 1,
	RPL_COMPLETE	= 2,
	RPL_CONTINUE	= 3,
	RPL_TRANSIENT	= 4,
	RPL_BADPASS		= 5,
	RPL_ERROR		= -1,
	RPL_TRANSFERERROR = -2
};

enum ReplyCodes
{
	RestartMarkerReply		= 110,
	ServiceReady			= 120,
	TransferStarting		= 125,
	FileStatusOkay			= 150,
	CommandOkey				= 200,
	CommandNotImplementedSuperfluous = 202,
	SystemStatus			= 211,
	DirectoryStatus			= 212,
	FileStatus				= 213,
	HelpMessage				= 214,
	SystemType				= 215,
	ServiceReadyForNewUser	= 220,
	ServiceClosingControlConnection = 221,
	DataConnectionOpen		= 225,
	ClosingDataConnection	= 226,
	EnteringPassiveMode		= 227,
	UserLoggedIn			= 230,
	RequestFileActionCompleted = 250,
	PathCreated				= 257,

	UserNameOk				= 331,
	NeedAccountForLogin		= 332,
	RequestedFileActionPendingFurtherInformation = 350,
	
	ServiceNotAvailable		= 421,
	CannotOpenDataConnection = 425,
	ConnectionClosed		= 426,
	RequestedFileActionNotTaken_Busy = 450,
	LocalErrorInProcessing	= 451,
	InsufficientStorageSpace = 452,

	SyntaxErrorBadCommand	= 500,
	SyntaxErrorInParameters = 501,
	CommandNotImplemented	= 502,
	BadSequenceOfCommands	= 503,
	CommandNotImplementedForThatParameter = 504,
	NotLoggedIn				= 530,
	NeedAccountForStoringFiles = 532,
	RequestedFileActionNotTaken_NotFound = 550,
	PageTypeUnknown			= 551,
	ExceededStorageAllocation = 552,
	FileNameNotAllowed		= 553,

	ReplyTimeOut			= 10000,
	InvalidState			= 10001
};

#endif
