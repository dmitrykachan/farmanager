#include <all_far.h>
#pragma hdrstop

#include "fstdlib.h"

bool DECLSPEC FP_CopyToClipboard(const std::wstring data)
{  
	HGLOBAL  hData;
	void    *GData;
	bool     rc = false;

	if (data.empty() || !OpenClipboard(0))
		return false;

	EmptyClipboard();
	BOOST_ASSERT(0 && "To test");

	if((hData=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, data.size()*2+2))!=NULL)
	{
		if((GData = GlobalLock(hData)) != NULL)
		{
			std::copy(data.begin(), data.end(), static_cast<wchar_t*>(GData));
			GlobalUnlock(hData);
			SetClipboardData(CF_UNICODETEXT, (HANDLE)hData);
			rc = true;
		} else
		{
			GlobalFree(hData);
		}
	}
	CloseClipboard();
	return rc;
}


BOOL DECLSPEC FP_GetFromClipboard( LPVOID& Data, size_t& DataSize )
  {  HANDLE   hData;
     void    *GData;
     BOOL     rc = FALSE;

    Data     = NULL;
    DataSize = 0;
    if ( !OpenClipboard(NULL) ) return FALSE;

    do{
      hData = GetClipboardData(CF_TEXT);
      if ( !hData )
        break;

      DataSize = GlobalSize( hData );
      if ( !DataSize )
        break;

      GData = GlobalLock( hData );
      if ( !GData )
        break;

      Data = _Alloc( DataSize+1 );
      memcpy( Data,GData,DataSize );
      rc = TRUE;
    }while(0);

    CloseClipboard();
  return rc;
}
