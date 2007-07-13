#include "stdafx.h"
//#include "windows.h"

#include "uniconverts.h"

namespace WinAPI
{
	std::wstring getModuleFileName()
	{
		WCHAR buffer[MAX_PATH];
		DWORD len = GetModuleFileNameW(NULL, buffer, sizeof(buffer));
		buffer[len] = 0;
		return buffer;
	}

	module loadLibrary(const std::wstring &filename)
	{
		void *p = LoadLibraryW(filename.c_str());
		module sh(LoadLibraryW(filename.c_str()), FreeLibrary);
		return sh;
	};

	bool freeLibrary(module &sh)
	{
		return FreeLibrary(static_cast<HMODULE>(sh.get())) != 0;
	};

	 FarProc getProcAddress(module h, const std::string &filename)
	 {
		 return GetProcAddress(static_cast<HMODULE>(h.get()), filename.c_str());
	 }


	std::string fromWide(const std::wstring &ws, UINT codePage)
	{
		if(ws.size() == 0)
			return "";
		std::vector<char> result;
		result.resize(ws.size());
		int res = ::WideCharToMultiByte(codePage, 0, ws.c_str(), static_cast<int>(ws.size()), 
			&result[0], static_cast<int>(result.size()), NULL, NULL);
		BOOST_ASSERT(res > 0);
		if(res < 0)
		{
			throw std::exception((std::string("WideCharToMultiByte fails: ") + boost::lexical_cast<std::string>(res)).c_str());
		}
		return std::string(result.begin(), result.end());
	}

	std::wstring toWide(const std::string &str, UINT codePage)
	{
		if(str.size() == 0)
			return L"";
		std::vector<wchar_t> result;
		result.resize(str.size());
		int res = ::MultiByteToWideChar(codePage, 0, str.c_str(), static_cast<int>(str.size()), 
			&result[0], static_cast<int>(result.size()));
		BOOST_ASSERT(res > 0);
		if(res < 0)
		{
			throw std::exception((std::string("MultiByteToWideChar fails: ") + boost::lexical_cast<std::string>(res)).c_str());
		}
		return std::wstring(result.begin(), result.end());
	}

	std::wstring fromOEM(const std::string &str)
	{
		return toWide(str, CP_OEMCP);
	}

	std::string toOEM(const std::wstring &ws)
	{
		return fromWide(ws, CP_OEMCP);
	}

	bool copyToClipboard(const std::wstring &data)
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

	std::wstring getTextFromClipboard()
	{
		HANDLE		hData;
		void		*GData;
		std::wstring res;

		if(!OpenClipboard(0))
			return false;
		do
		{
			hData = GetClipboardData(CF_UNICODETEXT);
			if (!hData)
				break;

			size_t dataSize = GlobalSize(hData);
			if (dataSize == 0)
				break;

			GData = GlobalLock( hData );
			if (!GData)
				break;

			const wchar_t* p = static_cast<const wchar_t*>(GData);
			res.reserve(dataSize);
			std::copy(p, p + dataSize, std::back_insert_iterator<std::wstring>(res));
		} while(0);

		CloseClipboard();
		return res;
	}


	void setConsoleTitle(const std::wstring& title)
	{
		SetConsoleTitleW(title.c_str());
	}

	std::wstring getConsoleTitle()
	{
		DWORD size = 128;
		DWORD n;
		boost::scoped_array<wchar_t> p;
		do 
		{
			size *= 2;
			p.reset(new wchar_t [size]);
			n = GetConsoleTitleW(p.get(), size);
		} while(n == 0);
		return std::wstring(p.get(), n);
	}
}