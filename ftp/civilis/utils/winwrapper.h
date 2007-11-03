#ifndef WINDOWS_WRAPPER_HEADER
#define WINDOWS_WRAPPER_HEADER

#include "unicode.h"

namespace WinAPI
{
	using namespace Unicode;
	typedef boost::shared_ptr<void> module;
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

	int				getConsoleWidth();
	int				getConsoleHeight();

	std::wstring	getStringError();
	std::wstring	getStringError(DWORD error);

	std::wstring	getMonthAbbreviature(int month, LCID loc = LOCALE_USER_DEFAULT);
	std::wstring	getMonth(int month, LCID loc = LOCALE_USER_DEFAULT);

	int checkForKeyPressed(const WORD* codes, int size);

	template<size_t n>
	int checkForKeyPressed(const WORD (&codes)[n])
	{
		return checkForKeyPressed(codes, n);
	}

	class  Stopwatch: boost::noncopyable
	{
	private:
		DWORD	startTime_;
		size_t	timeout_;
	public:
		Stopwatch(size_t timeout = 0);
		size_t	getPeriod() const;
		bool	isTimeout() const;
		size_t	getSecond() const;
		void	reset();
		void	setTimeout(size_t timeout);
		
	};


}

#endif