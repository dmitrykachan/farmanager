#include <all_far.h>
#pragma hdrstop

#include "fstdlib.h"

char *DECLSPEC StrCat( char *dest,CONSTSTR src,int dest_sz )
  {
     if ( !dest ) return NULL;
     if ( dest_sz == 0 ) return dest;

     if (!src) {
       *dest = 0;
       return dest;
     }

     if ( dest_sz != -1 ) {
       dest_sz--;
       strncat( dest,src,dest_sz );
       dest[dest_sz] = 0;
     } else
       strcat( dest,src );

 return dest;
}
