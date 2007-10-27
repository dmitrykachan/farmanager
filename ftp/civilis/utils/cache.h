#ifndef CACHE_HEADER_FILE
#define CACHE_HEADER_FILE

namespace Utils
{

template<typename Key, typename Value, size_t size>
class CacheFixedArray
{
private:
	struct CacheItem
	{
		bool	used;
		Key		key;
		Value	value;
	};
	typedef boost::array<CacheItem, size> List;
	List			cacheList_;
	size_t			possition_;

public:
	CacheFixedArray()
	{
		clear();
	}

	void		clear()
	{
		possition_ = 0;
		BOOST_FOREACH(CacheItem&i, cacheList_)
		{
			i.used = false;
		}
	}

	void		clear(const Key &key)
	{
		List::const_iterator itr =
			find_if(cacheList_.begin(), cacheList_.end(), boost::bind(&CacheItem::key, _1) == key);
		if(itr != cacheList_.end())
			itr->used = false;
	}

	void		add(const Key &key, const Value &value)
	{
		cacheList_[possition_].used	= true;
		cacheList_[possition_].key		= key;
		cacheList_[possition_].value	= value;
		++possition_;
	}

	bool		find(const Key& key, Value &value)
	{
		List::const_iterator itr =
			find_if(cacheList_.begin(), cacheList_.end(), boost::bind(&CacheItem::key, _1) == key);
		if(itr == cacheList_.end() || itr->used == false)
			return false;
		value = itr->value;
		return true;
	}

};

}

#endif