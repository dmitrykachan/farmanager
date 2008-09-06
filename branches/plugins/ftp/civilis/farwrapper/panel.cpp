#include "stdafx.h"
#include "panel.h"

namespace FARWrappers
{

ItemList::ItemList()
{
	list_.reserve(10);
}

ItemList::ItemList(PluginPanelItem** items, size_t count)
{
	add(items, count);
}

PluginPanelItem& ItemList::add(const PluginPanelItem &pi)
{
	list_.push_back(PluginPanelItem());

	copy(*(list_.end()-1), pi);
	return *(list_.end()-1);
}

void ItemList::add(PluginPanelItem **src, size_t count)
{
	list_.insert(list_.end(), count, PluginPanelItem());
	std::vector<PluginPanelItem>::iterator itr = list_.end() - count;
	for(size_t n = 0; n < count; ++n)
	{
		copy(*itr, **src);
		++src;
		++itr;
	}
}

void ItemList::reserve(size_t n)
{
	list_.reserve(n);
}


}
