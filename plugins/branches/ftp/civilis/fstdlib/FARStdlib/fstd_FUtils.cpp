#include <all_far.h>
#pragma hdrstop

#include "fstdlib.h"

//------------------------------------------------------------------------
int DECLSPEC CheckForKeyPressed( WORD *Codes,int NumCodes )
{  
	static HANDLE hConInp = CreateFile("CONIN$", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	DWORD         ReadCount;
	int           rc = 0;
	int           n;
	INPUT_RECORD  rec[1];

	BOOST_STATIC_ASSERT(sizeof(INPUT_RECORD) == 20);

	while (1) 
	{
		PeekConsoleInput(hConInp, rec, 1, &ReadCount);
		if(ReadCount==0) 
			break;

		ReadConsoleInput(hConInp, rec, 1, &ReadCount);

		for(n = 0; n < NumCodes; n++)
			if (rec[0].EventType == KEY_EVENT &&
				rec[0].Event.KeyEvent.bKeyDown &&
				rec[0].Event.KeyEvent.wVirtualKeyCode == Codes[n] )
				rc = n+1;
	}
	return rc;
}
//------------------------------------------------------------------------
int DECLSPEC FP_Color(INT_PTR tp )
{
	return (int)FP_Info->AdvControl( FP_Info->ModuleNumber, ACTL_GETCOLOR, (void*)tp );
}
