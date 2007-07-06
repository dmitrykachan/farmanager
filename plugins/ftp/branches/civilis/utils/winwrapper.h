#ifndef WINDOWS_WRAPPER_HEADER
#define WINDOWS_WRAPPER_HEADER

#include "unicode.h"

namespace WinAPI
{
	using namespace Unicode;
//	typedef void* handle;
//	class HMODULE;
	typedef boost::shared_ptr<void> module;
//	typedef int (__cdecl *FarProc)();
	typedef FARPROC FarProc;

	std::wstring	getModuleFileName();
	module			loadLibrary(const std::wstring &filename);
	bool			freeLibrary(module& sh);
	FarProc			getProcAddress(module h, const std::string &filename);

	std::string		fromWide(const std::wstring &ws, UINT codePage);
	std::wstring	toWide(const std::string &str, UINT codePage);
	std::wstring	fromOEM(const std::string &str);
	std::string		toOEM(const std::wstring &ws);

	bool			copyToClipboard(const std::wstring &data);
	std::wstring	getTextFromClipboard();

	void			setConsoleTitle(const std::wstring& title);
	std::wstring	getConsoleTitle();
}

#endif