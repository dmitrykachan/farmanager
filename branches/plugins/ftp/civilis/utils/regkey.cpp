#include "stdafx.h"
#include "regkey.h"

#include "boost/scoped_array.hpp"

namespace WinAPI
{

char* RegKey::exception::default_message_ = "def";

RegKey::RegKey()
	: hKey_(0)
{}

RegKey::RegKey(HKEY key)
	: hKey_(key)
{}

RegKey::RegKey(const RegKey& key)
	: hKey_(0)
{
	open(key.hKey_, NULL);
}

RegKey::RegKey(HKEY parent, const wchar_t* name, REGSAM samDesired)
	: hKey_(0)
{
	open(parent, name, samDesired);
}

RegKey::~RegKey()
{
	close();
}

void RegKey::close()
{
	if (hKey_ != 0)
	{
		LONG lRes = ::RegCloseKey(hKey_);
		hKey_ = 0;
		if(lRes != ERROR_SUCCESS)
		{
			throw exception(lRes);
		}
	}
}

RegKey& RegKey::operator=(const RegKey& key )
{
	if(hKey_ != key.hKey_)
	{
		RegKey r;
		r.openOnly(key.hKey_, NULL);
		swap(r);
		r.close();
	}
	return( *this );
}

RegKey& RegKey::operator=(HKEY hKey)
{
	if(hKey_ != hKey)
	{
		RegKey r;
		r.openOnly(hKey, NULL);
		swap(r);
		r.close();
	}
	return( *this );
}


void RegKey::openOnly(HKEY hKeyParent, const wchar_t* name, REGSAM samDesired)
{
	BOOST_ASSERT(hKeyParent != 0);
	HKEY hKey = NULL;
	LONG lRes = ::RegOpenKeyExW(hKeyParent, name, 0, samDesired, &hKey);
	if(lRes != ERROR_SUCCESS)
		throw exception(lRes);
	close();
	hKey_ = hKey;
}

void RegKey::open(HKEY hKeyParent, const wchar_t* name, REGSAM samDesired)
{
	BOOST_ASSERT(hKeyParent != NULL);
	HKEY hKey = NULL;
	LONG res = ::RegCreateKeyExW(hKeyParent, name, 0,
		REG_NONE, REG_OPTION_NON_VOLATILE, samDesired, NULL, &hKey, NULL);
	if(res != ERROR_SUCCESS)
		throw exception(res);

	close();
	hKey_ = hKey;
}

void RegKey::set(const wchar_t* name, const wchar_t* value, DWORD dwType) const
{
	BOOST_ASSERT(dwType == REG_SZ || dwType == REG_MULTI_SZ);
	set(name, dwType, value, wcslen(value)*sizeof(wchar_t));
}

void RegKey::set(const wchar_t* name, const std::wstring &value, DWORD dwType) const
{
	BOOST_ASSERT(dwType == REG_SZ || dwType == REG_MULTI_SZ);
	set(name, dwType, value.c_str(), value.size()*sizeof(wchar_t));
}

void RegKey::set(const wchar_t* name, const void* pValue, size_t nBytes) const
{
	set(name, REG_BINARY, pValue, nBytes);
}

void RegKey::set(const wchar_t* name, DWORD dwType, const void* pValue, size_t nBytes) const
{
	BOOST_ASSERT(hKey_);
	LONG res = ::RegSetValueExW(hKey_, name, NULL, dwType, 
			reinterpret_cast<const BYTE*>(pValue), static_cast<DWORD>(nBytes));
	if(res != ERROR_SUCCESS)
		throw exception(res);
}


void RegKey::set(const wchar_t* name, int dwValue) const
{
	set(name, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(dwValue));
}


void RegKey::set(const wchar_t* name, long long qwValue) const
{
	set(name, REG_QWORD, reinterpret_cast<const BYTE*>(&qwValue), sizeof(qwValue));
}


void RegKey::set(const wchar_t* name, std::vector<unsigned char> &vec) const
{
	if(vec.empty())
		set(name, REG_BINARY, 0, 0);
	else
		set(name, REG_BINARY, reinterpret_cast<const BYTE*>(&vec[0]), vec.size());
}

std::wstring RegKey::enumKey(DWORD index) const
{
	FILETIME lastWriteTime;

	BOOST_ASSERT(hKey_);
	wchar_t buff[maxKeySize + 1];
	DWORD size = sizeof(buff)/sizeof(*buff);
	LONG res = ::RegEnumKeyExW(hKey_, index, buff, &size, NULL, NULL, NULL, &lastWriteTime);
	if(res != ERROR_SUCCESS)
		throw exception(res);
	return std::wstring(buff);
}

size_t RegKey::keyCount() const
{
	BOOST_ASSERT(hKey_);
	DWORD count;
	::RegQueryInfoKeyW(hKey_, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	return count;
}

size_t RegKey::valueCount() const
{
	BOOST_ASSERT(hKey_);
	DWORD count;
	::RegQueryInfoKeyW(hKey_, NULL, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL);
	return count;
}

std::vector<std::wstring> RegKey::getKeys() const
{
	FILETIME lastWriteTime;
	std::vector<std::wstring> vec;

	BOOST_ASSERT(hKey_);
	wchar_t buff[maxKeySize + 1];
	DWORD size;
	size_t count = keyCount();
	vec.reserve(count);

	for(DWORD index = 0; index < count; ++index)
	{
		size = sizeof(buff)/sizeof(*buff);
		LONG res = ::RegEnumKeyExW(hKey_, index, buff, &size, NULL, NULL, NULL, &lastWriteTime);
		if(res != ERROR_SUCCESS)
			throw exception(res);
		vec.push_back(std::wstring(buff));
	}
	return vec;
}


void RegKey::deleteSubKey(const wchar_t* name, bool recursive) const
{
	if(recursive == false)
	{
		BOOST_ASSERT(hKey_ != 0);
		LONG res = ::RegDeleteKeyW(hKey_, name);
		if(res != ERROR_SUCCESS)
			throw exception(res);
	} else
	{
		RegKey key;
		key.open(hKey_, name, KEY_READ | KEY_WRITE);
		FILETIME time;
		wchar_t buff[maxKeySize+1];
		DWORD dwSize = sizeof(buff)/sizeof(*buff);
		while(::RegEnumKeyExW(key, 0, buff, &dwSize, NULL, NULL, NULL, &time)==ERROR_SUCCESS)
		{
			key.deleteSubKey(buff, true);
			dwSize = sizeof(buff)/sizeof(*buff);
		}
		key.close();
		return deleteSubKey(name, false);
	}
}

/*
void RegKey::create(HKEY hKeyParent, const wchar_t* name, wchar_t* cls / * = REG_NONE * /, 
					DWORD options / * = REG_OPTION_NON_VOLATILE * /, 
					REGSAM samDesired / * = KEY_READ | KEY_WRITE * /, 
					LPSECURITY_ATTRIBUTES pSecAttr / * = NULL * /, LPDWORD pDisposition / * = NULL * /)
{
	BOOST_ASSERT(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey = NULL;
	LONG res = ::RegCreateKeyExW(hKeyParent, name, 0,
		cls, options, samDesired, pSecAttr, &hKey, &dw);
	if (pDisposition != NULL)
		*pDisposition = dw;
	if (res == ERROR_SUCCESS)
	{
		close();
		hKey_ = hKey;
	} else
		throw exception(res);
}
*/


HKEY RegKey::create(HKEY hKeyParent, const wchar_t* name, wchar_t* cls /* = REG_NONE */, 
					DWORD options /* = REG_OPTION_NON_VOLATILE */, 
					REGSAM samDesired /* = KEY_READ | KEY_WRITE */, 
					LPSECURITY_ATTRIBUTES pSecAttr /* = NULL */, LPDWORD pDisposition /* = NULL */)
{
	BOOST_ASSERT(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey = NULL;
	LONG res = ::RegCreateKeyExW(hKeyParent, name, 0,
		cls, options, samDesired, pSecAttr, &hKey, &dw);
	if (pDisposition != NULL)
		*pDisposition = dw;
	if (res == ERROR_SUCCESS)
	{
		return hKey;
	} else
		throw exception(res);
}


long RegKey::get(const wchar_t* name, DWORD* pdwType, void* pData, ULONG* pnBytes) const
{
	BOOST_ASSERT(hKey_);
	return ::RegQueryValueExW(hKey_, name, NULL, pdwType, static_cast<LPBYTE>(pData), pnBytes);
}

void RegKey::getValueInfo(const wchar_t* name, DWORD &Type, ULONG &size) const
{
	size = 0;
	long res = get(name, &Type, NULL, &size);
	if(res != ERROR_SUCCESS)
		throw exception(res);
}

void RegKey::get(const wchar_t* name, void* pValue, ULONG* pnBytes) const
{
	DWORD type;
	long res = get(name, &type, pValue, pnBytes);
	if(res != ERROR_SUCCESS)
		throw exception(res);
	if(type != REG_BINARY)
		throw exception("invalide data type");
}

int RegKey::get(const wchar_t* name, int defValue) const
{
	int value;
	DWORD type;
	ULONG size = sizeof(value);
	long res = get(name, &type, &value, &size);
	if(res == ERROR_SUCCESS && type == REG_DWORD)
		return value;
	else
		return defValue;
}

void RegKey::get(const wchar_t* name, int* value) const
{
	DWORD type;
	ULONG size = sizeof(*value);
	long res = get(name, &type, value, &size);
	if(res != ERROR_SUCCESS)
		throw exception(res);
	if(type != REG_DWORD)
		throw exception("Invalid reg type. DWORD is expected");
}

long long RegKey::get(const wchar_t* name, long long defValue) const
{
	long long value;
	DWORD type;
	ULONG size = sizeof(value);
	long res = get(name, &type, &value, &size);
	if(res == ERROR_SUCCESS && type == REG_QWORD)
		return value;
	else
		return defValue;
}

void RegKey::get(const wchar_t* name, long long* value) const
{
	DWORD type;
	ULONG size = sizeof(*value);
	long res = get(name, &type, value, &size);
	if(res != ERROR_SUCCESS)
		throw exception(res);
	if(type != REG_QWORD)
		throw exception("Invalid reg type. DWORD is expected");
}

std::wstring RegKey::get(const wchar_t* name, const std::wstring &defValue) const
{
	std::wstring value;
	DWORD type;
	ULONG size = 0;
	long res = get(name, &type, NULL, &size);
	if(res != ERROR_SUCCESS && type != REG_SZ)
		return defValue;
	
	if(size == 0)
		return L"";
	boost::scoped_array<wchar_t> buff(new wchar_t[size/sizeof(wchar_t)]);
	res = get(name, &type, buff.get(), &size);
	return std::wstring(buff.get(), buff.get()+size/sizeof(wchar_t)-1);
}

void RegKey::get(const wchar_t* name, std::wstring *value) const
{
	DWORD type;
	ULONG size = 0;
	long res = get(name, &type, NULL, &size);
	if(res != ERROR_SUCCESS)
		throw exception(res);
	if(type != REG_SZ)
		throw exception("Invalid reg type. DWORD is expected");

	boost::scoped_array<wchar_t> buff(new wchar_t[size]);
	res = get(name, &type, buff.get(), &size);
	value->assign(buff.get(), buff.get()+size/sizeof(wchar_t)-1);
}

void RegKey::get(const wchar_t* name, std::vector<unsigned char> *vec) const
{
	DWORD type;
	ULONG size = 0;
	long res = get(name, &type, NULL, &size);
	if(res != ERROR_SUCCESS && type != REG_BINARY)
		throw exception("Invalid reg type. DWORD is expected");

	vec->resize(size);
	if(size)
		get(name, &type, &(*vec)[0], &size);
}

bool RegKey::isSubKeyExists(const wchar_t* name) const
{
	BOOST_ASSERT(hKey_ != 0);
	HKEY h;
	LONG lRes = ::RegOpenKeyExW(hKey_, name, 0, KEY_READ, &h);
	if(lRes == ERROR_SUCCESS)
	{
		LONG lRes = ::RegCloseKey(h);
		return true;
	} else
		return false;
}

}

#ifdef CONFIG_TEST


template<typename T>
void testRegKeyGetSet(WinAPI::RegKey& r, const wchar_t* name, T defValue, T setValue)
{
	using namespace WinAPI;
//	std::string tname = typeid(T).name();
	std::string tname = "";
	T val = r.get(name, defValue);
	BOOST_CHECK_MESSAGE(val == defValue, tname+": not equal to default value");
	BOOST_CHECK_THROW(r.get(name, &val), RegKey::exception);
	r.set(name, setValue);
	val = r.get(name, defValue);
	BOOST_CHECK_MESSAGE(val == setValue, tname+": not equal to set value");
	BOOST_CHECK_NO_THROW(r.get(name, &val));
	BOOST_CHECK_MESSAGE(val == setValue, tname+": not equal to set value2");
}

BOOST_AUTO_TEST_CASE(testRegKey)
{
	using namespace WinAPI;
	try
	{
		RegKey r;

		r.open(HKEY_CURRENT_USER, L"Software");
		try
		{
			r.deleteSubKey(L"Test", true);
		}
		catch (RegKey::exception&)
		{
		}

		r = RegKey::create(HKEY_CURRENT_USER, L"Software\\Test");

		RegKey rp;
		rp = r;

		RegKey::create(rp, L"subtest1");
		RegKey::create(rp, L"subtest2");
		RegKey::create(rp, L"subtest3");
		RegKey::create(rp, L"subtest4");
		RegKey::create(rp, L"subtest5");


		testRegKeyGetSet(r, L"TestInt", -1, 123);
		testRegKeyGetSet(r, L"TestLong", -1LL, 1234567890LL);
		testRegKeyGetSet<std::wstring>(r, L"TestString", L"defValue", L"setValue");

		std::wstring str;

		r.set(L"teststring", L"string1");
		r.set(L"teststring2", std::wstring(L"string2"));
		r.set(L"binary", "123456789", 10);

		r.get(L"teststring", &str);
		str = r.get(L"teststring2", L"");


		r.open(HKEY_CURRENT_USER, L"Software");
		r.deleteSubKey(L"Test", true);

	}
	catch (RegKey::exception& e)
	{
		std::cout << e.what();		
	}
}

#endif