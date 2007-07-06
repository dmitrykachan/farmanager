#include <all_far.h>
#pragma hdrstop

#include "fstdlib.h"

char DECLSPEC ToLower( char ch )
  {
 return (char)CharLower( (LPTSTR)MK_WORD(0,ch) );
}
