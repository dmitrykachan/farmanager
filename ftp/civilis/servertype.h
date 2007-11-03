#ifndef SERVER_TYPE_HEADER_GUARD
#define SERVER_TYPE_HEADER_GUARD

#include "fileinfo.h"

class ServerType: boost::noncopyable
{
public:
	typedef std::wstring::const_iterator const_iterator;
	typedef std::wstring::iterator iterator;
	typedef std::pair<const_iterator, const_iterator> entry;

	virtual std::wstring	getName() const = 0;
	virtual std::wstring	getDescription() const = 0;
	virtual void			parseListingEntry(const_iterator itr, 
											  const const_iterator &itr_end,
											  FTPFileInfo& fi) const = 0;
	virtual bool			acceptServerInfo(const std::wstring &str) const = 0;
	virtual entry			getEntry(const_iterator &itr, const const_iterator &itr_end) const;
	virtual	std::wstring	parsePWD(const_iterator itr, const const_iterator itr_end) const
	{
		itr = std::find(itr, itr_end, L'\"');
		if(itr == itr_end)
			return std::wstring(itr, itr_end);

		++itr;
		const_iterator i2 = itr;
		do
		{
			i2 = std::find(itr, itr_end, L'\"');
			if(i2 == itr_end)
				return std::wstring(itr, itr_end);
			if(i2+1 == itr_end || *(i2+1) != L'\"')
				return std::wstring(itr, i2); // TODO unquote
			i2 += 2;
		} while(true);
	};
	virtual bool			isValidEntry(const const_iterator &itr, const const_iterator &itr_end) const;
	virtual boost::shared_ptr<ServerType> clone() const = 0;
};


typedef boost::shared_ptr<ServerType> ServerTypePtr;

class ServerList: public boost::noncopyable
{
public:

	typedef std::vector<ServerTypePtr> ListType;
	typedef ListType::iterator iterator;
	typedef ListType::const_iterator const_iterator;

	const_iterator	begin() const {return list_.begin(); }
	iterator		begin() {return list_.begin(); }
	const_iterator	end() const {return list_.end(); }
	iterator		end() {return list_.end(); }
	static ServerList& instance()
	{
		static ServerList list;
		return list;
	}
	const_iterator find(const std::wstring &name) const
	{
		BOOST_ASSERT(begin() != end() && "There are no servers in the list");

		return std::find_if(begin(), end(), boost::bind(&ServerType::getName, _1) == name);
	}
	size_t size() { return list_.size(); };
	ServerTypePtr operator[](size_t n)
	{
		return list_[n];
	}

	ServerTypePtr create(const std::wstring &name);
	static bool isAutodetect(const ServerTypePtr& server);
	static ServerTypePtr autodetect(const std::wstring &serverInfo);

private:
	ServerList();
	void addServer(ServerType* type);
	ListType list_;
};

#endif