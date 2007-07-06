#ifndef FILE_INFO_HEADER
#define FILE_INFO_HEADER

#include "utils/filetime.h"
#include "utils/parserprimitive.h"
#include "utils/uniconverts.h" // TODO
#include "utils/strutils.h"

	class FTPFileInfo
	{
	public:
		typedef boost::array<wchar_t, 1+3*3> UnixMode;
		enum Type
		{
			undefined, file, directory, symlink, specialdevice, socket, pipe
		};
		typedef unsigned __int64 FileSize;
	private:
		Type				type_;
		WinAPI::FileTime	creationTime_;
		WinAPI::FileTime	lastAccessTime_;
		WinAPI::FileTime	lastWriteTime_;
		FileSize			fileSize_;
		std::wstring		fileName_;
		std::wstring		link_;
		std::wstring		owner_;
		std::wstring		group_;
		UnixMode			unixMode_;

		static bool oneof(wchar_t test, const wchar_t* valid)
		{
			test = std::tolower(test, defaultLocale_);
			while(*valid)
			{
				if(test == *valid)
					return true;
				++valid;
			}
			return false;
		}


		static bool checkAttribute(wchar_t test, wchar_t valid)
		{
			if(test == '-')
				return true;
			if(std::tolower(test, defaultLocale_) == valid)
				return true;
			return false;
		}

		static bool checkAttribute(wchar_t test, wchar_t valid1, wchar_t valid2)
		{
			if(test == '-')
				return true;
			test = std::tolower(test, defaultLocale_);
			if(test == valid1 || test == valid2)
				return true;
			return false;
		}

	public:
		class exception: public std::exception
		{
		public:
			exception(const char * const& str)
				: std::exception(str)
			{};
		};

		FTPFileInfo()
			: fileSize_(0), type_(undefined)
		{
			clearUnixMode();
		}

		void clear()
		{
			fileSize_	= 0;
			type_		= undefined;
			clearUnixMode();
			setLastWriteTime(WinAPI::FileTime());
			setLastAccessTime(WinAPI::FileTime());
			setCreationTime(WinAPI::FileTime());
			fileName_ = link_ = owner_ = group_ = L"";
		}

		static bool verifyUnixMode(const UnixMode &u)
		{
			return oneof(u[0], L"-dcbpl")
				&& checkAttribute(u[1], 'r')
				&& checkAttribute(u[2], 'w')
				&& checkAttribute(u[3], 'x', 's')
				&& checkAttribute(u[4], 'r')
				&& checkAttribute(u[5], 'w')
				&& checkAttribute(u[6], 'x', 's')
				&& checkAttribute(u[7], 'r')
				&& checkAttribute(u[8], 'w')
				&& checkAttribute(u[9], 'x', 't')
			;
		}

		inline void check(bool value, const char* const what)
		{
			if(!value)
			{
				throw exception(what);
			}
		}

		template<typename I1, typename I2>
		void setUnixMode(I1 &itr, I2 &itr_end)
		{
			check(itr + unixMode_.size() <= itr_end, "string size is not enough");

			std::copy(itr, itr+unixMode_.size(), unixMode_.begin());
			itr += unixMode_.size();
			check(verifyUnixMode(unixMode_), "the unix attribute string is not valid");

			switch(std::tolower(unixMode_[0], defaultLocale_))
			{
			case '-': type_ = file; break;
			case 'd': type_ = directory; break;
			case 'l': type_ = symlink; break;
			case 'b': 
			case 'c': type_ = specialdevice; break;
			default:
				check(false, "unknown type");
			}
		}

		void clearUnixMode()
		{
			std::fill(unixMode_.begin(), unixMode_.end(), L'\0');
		}

		void setOwner(const std::wstring::const_iterator &itr, const std::wstring::const_iterator &itr_end)
		{
			owner_.assign(itr, itr_end);
		}

		void setGroup(const std::wstring::const_iterator &itr, const std::wstring::const_iterator &itr_end)
		{
			group_.assign(itr, itr_end);
		}

		void setFileName(const std::wstring::const_iterator &itr, std::wstring::const_iterator itr_end, bool symlink = false)
		{
			static const wchar_t link_delimiter[] = L" -> ";
			const int link_delimiter_size = sizeof(link_delimiter)/sizeof(*link_delimiter)-1;
			if(symlink)
			{
				std::wstring::const_iterator itrlink = 
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

		void setSymlink(const std::wstring::const_iterator &itr, std::wstring::const_iterator itr_end)
		{
			if(itr == itr_end)
			{
				link_.clear();
				return;
			}
			setType(symlink);
			link_.assign(itr, itr_end);
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
		}

		const std::wstring& getFileName() const
		{
			return fileName_;
		}

		FileSize getFileSize() const
		{
			return fileSize_;
		}

		const UnixMode& getUnixMode() const
		{
			return unixMode_;
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

		const std::wstring getOwner() const
		{
			return owner_;
		}

		const std::wstring getGroup() const
		{
			return group_;
		}

		const std::wstring& getLink() const
		{
			return link_;
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

		static std::wstring toText(const UnixMode &type)
		{
			if(type[0] == L'\0')
				return L"";
			else 
				return std::wstring(type.begin(), type.end());
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

		WIN32_FIND_DATA getFindData() const
		{
			WIN32_FIND_DATA data;

			data.dwFileAttributes = (getType() == directory)? FILE_ATTRIBUTE_DIRECTORY : 0 /*FILE_ATTRIBUTE_NORMAL*/;
			// TODO FILE_ATTRIBUTE_READONLY FILE_ATTRIBUTE_REPARSE_POINT
			data.ftCreationTime		= getCreationTime();
			data.ftLastAccessTime	= getLastAccessTime();
			data.ftLastWriteTime	= getLastWriteTime();
			data.nFileSizeHigh		= static_cast<DWORD>(getFileSize() >> 32);
			data.nFileSizeLow		= static_cast<DWORD>(getFileSize() & 0xFFFFFFFF);
			data.dwReserved0		= 0;
			data.dwReserved1		= 0;
			Utils::safe_strcpy(data.cFileName, Unicode::utf16ToUtf8(getFileName()).c_str());
			data.cAlternateFileName[0] = 0;
			return data;
		}
	};

inline std::wostream& operator << (std::wostream& os, const FTPFileInfo& fi)
{
	os << L'[' << FTPFileInfo::toText(fi.getType()) << "]   ";
	if(!FTPFileInfo::toText(fi.getUnixMode()).empty())
	{
		os << FTPFileInfo::toText(fi.getUnixMode()) << L"   ";
	}
	else
		os << L"             ";
	os << fi.getLastWriteTime() << L"   ";
	os << fi.getLastAccessTime() << L"   ";
	os << fi.getCreationTime() << L"   ";
	os << std::setw(10) << std::setfill(L' ') << fi.getFileSize() << L"   ";
	os << L"[" << std::setw(10) << fi.getOwner() << L"]   ";
	os << L"[" << std::setw(10) << fi.getGroup() << L"]   ";
	os << L"[" << std::setw(13) << Unicode::utf16ToUtf8(fi.getFileName()).c_str() << L"]   "; // TODO
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
		if(!std::isdigit(*itr, defaultLocale_))
		{
			fi.setUnixMode(itr, itr_end);
			skipSpaces(itr, itr_end);
		} else
			fi.clearUnixMode();

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

	if(l.getUnixMode() != r.getUnixMode())
	{
		result	<< L"UnixMode: " << FTPFileInfo::toText(l.getUnixMode()) 
				<< delimiter << FTPFileInfo::toText(r.getUnixMode()) << L"\n";
	}

	if(l.getLastWriteTime() != r.getLastWriteTime())
	{
		result << L"Last write time: "<< l.getLastAccessTime() << L"<->" << r.getLastAccessTime() << L"\n";
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