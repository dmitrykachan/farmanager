#ifndef REGISTRY_KEY_INCLUDE
#define REGISTRY_KEY_INCLUDE

namespace WinAPI
{

class RegKey
{
private:
	HKEY hKey_;

public:
	static const size_t maxKeySize = 255;

	class exception : public std::exception
	{
	private:
		const LONG	error_;
		char*		msg_;
		static char* default_message_;
	public:
		exception(LONG error)
			: error_(error), msg_(0)
		{}

		exception(const char* str)
			: std::exception(str), msg_(default_message_), error_(0)
		{}


		~exception()
		{
			if(msg_ && msg_ != default_message_)
				LocalFree(msg_);
		}
		virtual const char* what() const
		{
			if(msg_ == default_message_)
			{
				return std::exception::what();
			} else
			{
				FormatMessageA(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					error_,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPSTR) &msg_,
					0, NULL );
				return msg_;
			}
		}

		LONG getErrorCode() const
		{
			return error_;
		}
	};

	RegKey();
	RegKey(const RegKey& key);
	/*explicit */RegKey(HKEY hKey);
	RegKey(HKEY parent, const wchar_t* name, REGSAM samDesired = KEY_READ | KEY_WRITE);

	~RegKey();
	RegKey& operator=(const RegKey& key);
	RegKey& operator=(HKEY hKey);

	operator HKEY() const				{ return hKey_;}

	void		open(HKEY hKeyParent, const wchar_t* name, REGSAM samDesired = KEY_READ | KEY_WRITE);
	void		close();

	void		set(const wchar_t* name, const wchar_t* value, DWORD dwType = REG_SZ) const;
	void		set(const wchar_t* name, const void* pValue, size_t nBytes) const;
	void		set(const wchar_t* name, DWORD dwType, const void* pValue, size_t nBytes) const;
	void		set(const wchar_t* name, const std::wstring &value, DWORD dwType = REG_SZ) const;
	void		set(const wchar_t* name, int dwValue) const;
	void		set(const wchar_t* name, long long qwValue) const;
	void		set(const wchar_t* name, std::vector<unsigned char> &vec) const;

	void		getValueInfo(const wchar_t* name, DWORD &Type, ULONG &size) const;
	long		get(const wchar_t* name, DWORD* pdwType, void* pData, ULONG* pnBytes) const;
	void		get(const wchar_t* name, void* pValue, ULONG* pnBytes) const;

	int			get(const wchar_t* name, int defValue) const;
	long long	get(const wchar_t* name, long long defValue) const;
	std::wstring get(const wchar_t* name, const std::wstring &defValue) const;

	void		get(const wchar_t* name, int *value) const;
	void		get(const wchar_t* name, long long *value) const;
	void		get(const wchar_t* name, std::wstring *value) const;
	void		get(const wchar_t* name, std::vector<unsigned char> *vec) const;

	static HKEY	create(HKEY hKeyParent, const wchar_t* name, 
		wchar_t* cls = REG_NONE, DWORD options = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_READ | KEY_WRITE,
		LPSECURITY_ATTRIBUTES pSecAttr = NULL,
		LPDWORD pDisposition = NULL);

	void flush() const
	{
		BOOST_ASSERT(hKey_);
		LONG res = ::RegFlushKey(hKey_); 
		if(res != ERROR_SUCCESS)
			throw exception(res);
	}

	size_t keyCount() const;
	size_t valueCount() const;
	std::wstring enumKey(DWORD index) const;
	std::vector<std::wstring> getKeys() const;

	void deleteSubKey(const wchar_t* name, bool recursive = false) const;
	void deleteValue(const wchar_t* name) const;

	bool isSubKeyExists(const wchar_t* name) const;

	void swap(RegKey &r)
	{
		HKEY h	= r.hKey_;
		r.hKey_	= hKey_;
		hKey_	= h;
	}
};

}
#endif