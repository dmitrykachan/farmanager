#include "stdafx.h"
#include "utils.h"

extern int FP_LastOpMode;

namespace FARWrappers
{

int Screen::saveCount_ = 0;
std::wstring Screen::saveTitle_;
HANDLE	Screen::hScreen_;

bool Screen::isSaved()
{
	return saveCount_ != 0;
}


void Screen::save()
{
	PROCP(L"savecout: " << saveCount_);

	if(FP_LastOpMode & OPM_FIND)
		return;

	if (saveCount_ == 0)
	{
		hScreen_	= FARWrappers::getInfo().SaveScreen(0,0,-1,-1);
		saveTitle_	= WinAPI::getConsoleTitle();
	}

	++saveCount_;
}

void Screen::fullRestore()
{
	while(isSaved())
		restore();
}

void Screen::restoreWithoutNotes()
{
	if(hScreen_)
	{
		FARWrappers::getInfo().RestoreScreen(hScreen_);
		hScreen_ = FARWrappers::getInfo().SaveScreen( 0,0,-1,-1 );
	}
}

void Screen::restore()
{
	if (!hScreen_ || !isSaved())
		return;

	--saveCount_;
	if(saveCount_ == 0)
	{
		BOOST_LOG(DBG, L"Screen: restore");
		WinAPI::setConsoleTitle(saveTitle_);
		FARWrappers::getInfo().RestoreScreen(hScreen_);
		FARWrappers::getInfo().Text(0,0,0,NULL); // refresh far-screen buffer
		hScreen_ = NULL;
	}
}

}