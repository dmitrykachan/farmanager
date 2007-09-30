#ifndef DIALOGS_INCLUDE
#define DIALOGS_INCLUDE


#include "ftp_Int.h"

namespace Dialogs
{

enum overwriteResult
{
	Overwrite, OverwriteAll, Skip, SkipAll, Cancel
};

overwriteResult overwrite(const std::wstring &title, const std::wstring &message, const std::wstring &filename)
{
	const wchar_t* MsgItems[] =
	{
		title.c_str(),
		message.c_str(),
		filename.c_str(),
		getMsg(MAskOverwrite ),
		/*0*/getMsg(MOverwrite),
		/*1*/getMsg(MOverwriteAll),
		/*2*/getMsg(MCopySkip),
		/*3*/getMsg(MCopySkipAll),
		/*4*/getMsg(MCopyCancel)
	};

	int MsgCode = FARWrappers::message(MsgItems, 5, FMSG_WARNING);
	switch(MsgCode)
	{
	case 1:
		return Overwrite;
	case 2:
		return OverwriteAll;
	case 3:
	    return Skip;
	case 4:
	    return SkipAll;
	default:
	    return Cancel;
	}
}

}

#endif