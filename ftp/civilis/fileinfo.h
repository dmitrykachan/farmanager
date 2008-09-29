#ifndef FILE_INFO_HEADER
#define FILE_INFO_HEADER

#include "utils/filetime.h"
#include "utils/parserprimitive.h"
#include "utils/strutils.h"

class FTPFileInfo
{
public:
	enum Type
	{
		undefined, file, directory, symlink, specialdevice, socket, pipe
	};
	typedef unsigned __int64 FileSize;
private:
	typedef std::wstring::const_iterator	const_iterator;
	typedef std::wstring::iterator			iterator;

	Type				type_;
	WinAPI::FileTime	creationTime_;
	WinAPI::FileTime	lastAccessTime_;
	WinAPI::FileTime	lastWriteTime_;
	FileSize			fileSize_;
	std::wstring		fileName_;
	std::wstring		link_;
	std::wstring		owner_;
	std::wstring		group_;
	DWORD				windowsAttribute_;
	std::wstring		secondAttribute_;

public:
	class exception: public std::exception
	{
	public:
		exception(const char * const& str)
			: std::exception(str)
		{};
	};

	FTPFileInfo()
	{
		clear();
	}

	void clear()
	{
		fileSize_	= 0;
		type_		= undefined;
		setLastWriteTime(WinAPI::FileTime());
		setLastAccessTime(WinAPI::FileTime());
		setCreationTime(WinAPI::FileTime());
		fileName_ = link_ = owner_ = group_ = secondAttribute_ = L"";
		windowsAttribute_ = 0;
	}

	inline void check(bool value, const char* const what)
	{
		if(!value)
		{
			throw exception(what);
		}
	}

	void setOwner(const const_iterator &itr, const const_iterator &itr_end)
	{
		owner_.assign(itr, itr_end);
	}

	void setGroup(const const_iterator &itr, const const_iterator &itr_end)
	{
		group_.assign(itr, itr_end);
	}

	void setFileName(const std::wstring &name)
	{
		fileName_ = name;
	}

	void setFileName(const const_iterator &itr, const_iterator itr_end, bool symlink = false)
	{
		static const wchar_t link_delimiter[] = L" -> ";
		const int link_delimiter_size = sizeof(link_delimiter)/sizeof(*link_delimiter)-1;
		if(symlink)
		{
			const_iterator itrlink = 
				std::search(itr, itr_end, 
						link_delimiter, link_delimiter+link_delimiter_size);
			if(itrlink != itr_end)
			{
				link_.assign(itrlink+link_delimiter_size, itr_end);
				itr_end = itrlink;
			}
		} else
			link_.clear();
		fileName_.assign(itr, itr_end);
	}

	void setSecondAttribute(const std::wstring &attribute)
	{
		secondAttribute_ = attribute;
	}

	void setSecondAttribute(const const_iterator &itr, const const_iterator &itr_end)
	{
		secondAttribute_.assign(itr, itr_end);
	}


	void setSymlink(const const_iterator &itr, const_iterator itr_end)
	{
		if(itr == itr_end)
		{
			link_.clear();
			return;
		}
		setType(symlink);
		link_.assign(itr, itr_end);
	}

	void setWindowsAttribute(DWORD attribute)
	{
		windowsAttribute_ = attribute;
	}

	void setFileSize(FileSize size)
	{
		fileSize_ = size;
	}

	void setCreationTime(const WinAPI::FileTime &ft)
	{
		creationTime_ = ft;
	}

	void setLastAccessTime(const WinAPI::FileTime &ft)
	{
		lastAccessTime_ = ft;
	}

	void setLastWriteTime(const WinAPI::FileTime &ft)
	{
		lastWriteTime_ = ft;
	}

	void setType(Type type)
	{
		type_ = type;
		if(type_ == directory)
			windowsAttribute_ |= FILE_ATTRIBUTE_DIRECTORY;
		else
			if(type_ == symlink)
				windowsAttribute_ |= FILE_ATTRIBUTE_REPARSE_POINT;
	}

	const std::wstring& getFileName() const
	{
		return fileName_;
	}

	FileSize getFileSize() const
	{
		return fileSize_;
	}

	WinAPI::FileTime getCreationTime() const
	{
		return creationTime_;
	}

	WinAPI::FileTime getLastAccessTime() const
	{
		return lastAccessTime_;
	}

	WinAPI::FileTime getLastWriteTime() const
	{
		return lastWriteTime_;
	}

	const std::wstring& getOwner() const
	{
		return owner_;
	}

	const std::wstring& getGroup() const
	{
		return group_;
	}

	const std::wstring& getLink() const
	{
		return link_;
	}

	const std::wstring& getSecondAttribute() const
	{
		return secondAttribute_;
	}

	Type getType() const
	{
		return type_;
	}

	static wchar_t toText(Type type)
	{
		const static wchar_t t[7] = {L'u', L'f', L'd', L'l', L'c', L's', L'p'};
		return t[type];
	}

	static Type toType(wchar_t wc)
	{
		const static wchar_t t[7] = {L'u', L'f', L'd', L'l', L'c', L's', L'p'};
		const wchar_t* p_end = t + 7; 
		const wchar_t* p = std::find(t, p_end, wc);
		if(p == p_end)
			return undefined;
		else 
			return static_cast<Type>(p-t);
	}

	DWORD getWindowsFileAttribute() const
	{
		return windowsAttribute_;
	}

};
typedef boost::shared_ptr<FTPFileInfo> FTPFileInfoPtr;
typedef std::vector<FTPFileInfoPtr> FileList;

inline std::wostream& operator << (std::wostream& os, const FTPFileInfo& fi)
{
	os << L'[' << FTPFileInfo::toText(fi.getType()) << "] ";
	if(!fi.getSecondAttribute().empty())
	{
		os << L'[' << fi.getSecondAttribute() << L"]  ";
	}
	else
		os << L"[]             ";
	os << fi.getLastWriteTime() << L"   ";
	os << fi.getLastAccessTime() << L"   ";
	os << fi.getCreationTime() << L"   ";
	os << std::setw(10) << std::setfill(L' ') << fi.getFileSize() << L"   ";
	os << L"[" << std::setw(10) << fi.getOwner() << L"]   ";
	os << L"[" << std::setw(10) << fi.getGroup() << L"]   ";
	os << L"[" << std::setw(13) << fi.getFileName() << L"]   ";
	os << L"[" << fi.getLink() << L"]";
	return os;
}

inline std::wistream& operator >> (std::wistream& is, FTPFileInfo& fi)
{
	using namespace Parser;
	std::wstring line;
	std::getline(is, line);
	
	std::wstring::const_iterator itr = line.begin(), itr_end = line.end(), itr2;
	if(itr == itr_end)
		return is;

	try
	{
		skipString(itr, itr_end, L"[");
		fi.setType(FTPFileInfo::toType(*itr));
		++itr;
		skipString(itr, itr_end, L"]");

		skipString(itr, itr_end, L"[");
		itr2 = itr;
		itr = std::find(itr, itr_end, L']');
		fi.setSecondAttribute(itr2, itr);
		skipString(itr, itr_end, L"]");

		WinAPI::FileTime ft;
		parseUnixDateTime(itr, itr_end, ft);
		fi.setLastWriteTime(ft);
		parseUnixDateTime(itr, itr_end, ft);
		fi.setLastAccessTime(ft);
		parseUnixDateTime(itr, itr_end, ft);
		fi.setCreationTime(ft);
		fi.setFileSize(parseUnsignedNumber<FTPFileInfo::FileSize>(itr, itr_end));
		
		skipString(itr, itr_end, L"[");
		itr2 = std::find(itr, itr_end, L']');
		fi.setOwner(itr, itr2);
		itr = itr2;
		skipString(itr, itr_end, L"]");
		
		skipString(itr, itr_end, L"[");
		itr2 = std::find(itr, itr_end, L']');
		fi.setGroup(itr, itr2);
		itr = itr2;
		skipString(itr, itr_end, L"]");

		skipString(itr, itr_end, L"[");
		itr2 = std::find(itr, itr_end, L']');
		fi.setFileName(itr, itr2);
		itr = itr2;
		skipString(itr, itr_end, L"]");

		skipString(itr, itr_end, L"[");
		itr2 = std::find(itr, itr_end, L']');
		fi.setSymlink(itr, itr2);
		itr = itr2;
		skipString(itr, itr_end, L"]");
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	return is;
}

inline std::wstring getTextDiff(const FTPFileInfo &l, const FTPFileInfo &r)
{
	std::wstringstream result;
	std::wstring delimiter = L" <-> ";
	if(l.getType() != r.getType())
		result	<< std::wstring(L"Type: ") << FTPFileInfo::toText(l.getType()) 
				<< delimiter << FTPFileInfo::toText(r.getType()) << L"\n";

	if(l.getSecondAttribute() != r.getSecondAttribute())
	{
		result	<< L"SecondAttribute: " << l.getSecondAttribute() 
				<< delimiter << r.getSecondAttribute() << L"\n";
	}

	if(l.getLastWriteTime() != r.getLastWriteTime())
	{
		result << L"Last write time: "<< l.getLastAccessTime() << delimiter << r.getLastAccessTime() << L"\n";
	}

	if(l.getLastAccessTime() != r.getLastAccessTime())
	{
		result << L"Last access time: " << l.getLastAccessTime() << delimiter << r.getLastAccessTime() << L"\n";
	}

	if(l.getCreationTime() != r.getCreationTime())
	{
		result << L"Creation time: " << l.getCreationTime() << delimiter << r.getCreationTime() << L"\n";
	}

	if(l.getFileSize() != r.getFileSize())
		result << L"File size: " << l.getFileSize() << delimiter << r.getFileSize() << L"\n";

	if(l.getGroup() != r.getGroup())
		result << L"Group: " << l.getGroup() << delimiter << r.getGroup() << L"\n";

	if(l.getOwner() != r.getOwner())
		result << L"Owner: " << l.getOwner() << delimiter << r.getOwner() << L"\n";

	if(l.getFileName() != r.getFileName())
		result << L"Filename: " << l.getFileName() << delimiter << r.getFileName() << L"\n";

	if(l.getLink() != r.getLink())
		result << L"Link: " << l.getLink() << delimiter << r.getLink() << L"\n";



	return result.str();
}

#endif