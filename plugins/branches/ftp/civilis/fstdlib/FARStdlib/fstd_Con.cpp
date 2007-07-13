#include <all_far.h>
#pragma hdrstop

#include "fstdlib.h"

size_t DECLSPEC FP_ConWidth( void )
{  
	CONSOLE_SCREEN_BUFFER_INFO ci;

	return GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE),&ci ) ? ci.dwSize.X : 0;
}

int DECLSPEC FP_ConHeight( void )
  {  CONSOLE_SCREEN_BUFFER_INFO ci;

 return GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE),&ci ) ? ci.dwSize.Y : 0;
}
