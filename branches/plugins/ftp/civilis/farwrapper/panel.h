#ifndef PANEL_ITEMS_WRAPPERS
#define PANEL_ITEMS_WRAPPERS

namespace FARWrappers
{
	inline wchar_t* dup(const wchar_t* str)
	{
		if(str == 0)
			return 0;
		wchar_t* s = new wchar_t [wcslen(str)+1];
		wcscpy(s, str);
		return s;
	}

	inline wchar_t* dup(const std::wstring &str)
	{
		if(str.empty())
			return 0;
		wchar_t* s = new wchar_t [str.size()+1];
		wcscpy(s, str.c_str());
		return s;
	}

	inline void setFileName(PluginPanelItem &item, const std::wstring &str)
	{
		delete [] item.FindData.lpwszFileName;
		item.FindData.lpwszFileName = dup(str);
	}

	inline void copy(PluginPanelItem& dst, const PluginPanelItem& src)
	{
		dst = src;
		dst.FindData.lpwszFileName			= dup(src.FindData.lpwszFileName);
		dst.FindData.lpwszAlternateFileName	= dup(src.FindData.lpwszAlternateFileName);
		dst.Description						= dup(src.Description);
		dst.Owner							= dup(src.Owner);
		if(src.CustomColumnNumber)
		{
			dst.CustomColumnData			= new wchar_t*[src.CustomColumnNumber];
			for(int i = 0; i < src.CustomColumnNumber; ++i)
			{
				dst.CustomColumnData[i] = dup(src.CustomColumnData[i]);
			}		
		} else
			dst.CustomColumnData = 0;
	}

	inline void free(PluginPanelItem& item)
	{
		for(int n = 0; n < item.CustomColumnNumber; ++n)
			delete [] item.CustomColumnData[n];
		delete [] item.CustomColumnData;
		delete [] item.Owner;
		delete [] item.Description;
		delete [] item.FindData.lpwszAlternateFileName;
		delete [] item.FindData.lpwszFileName;
		item.Owner = item.Description = 
			item.FindData.lpwszAlternateFileName = item.FindData.lpwszFileName = 0;
		item.CustomColumnData = 0;
	}

	inline void clear(PluginPanelItem& item)
	{
		std::fill_n(reinterpret_cast<int*>(&item), sizeof(item)/sizeof(int), 0);
	}
	
class ItemList: boost::noncopyable
{
private:
	std::vector<PluginPanelItem>	list_;

public:
	ItemList();
	ItemList(PluginPanelItem* items, size_t count);
	~ItemList()
	{}

	PluginPanelItem&	add(const PluginPanelItem &src);
	void				add(const PluginPanelItem *src, size_t count);
	void				reserve(size_t n);

	PluginPanelItem* items()
	{ 
		return &(*list_.begin());
	}
	size_t	size() const
	{ 
		return list_.size();
	}
	const PluginPanelItem& at(size_t num) const
	{
		return list_.at(num);
	}
	PluginPanelItem& at(size_t num)
	{
		return list_.at(num);
	}

	PluginPanelItem& operator[](size_t num)
	{
		return list_[num];
	}
	const PluginPanelItem& operator[](size_t num) const
	{
		return list_[num];
	}

	void clear()
	{
		list_.clear();
		std::vector<PluginPanelItem>().swap(list_);
	}
};

}

#endif


