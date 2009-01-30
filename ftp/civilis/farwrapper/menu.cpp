#include "stdafx.h"
#include "menu.h"


namespace FARWrappers
{

	Menu::~Menu()
{
}

void Menu::init(const wchar_t** items, size_t size, bool copy)
{
	items_.resize(size);
	if(copy)
	{
		strings_.assign(items, items + size);
		conststrings_	= 0;
	}
	else
		conststrings_ = items;
}

void Menu::setTitle(const std::wstring& title)
{
	title_	= title;
}

void Menu::setBottom(const std::wstring& bottom)
{
	bottom_ = bottom;
}

void Menu::setHelpTopic(const std::wstring& helpTopic)
{
	helpTopic_ = helpTopic;
}

void Menu::setMaxHeight(int maxHeight)
{
	maxHeight_ = maxHeight;
}

void Menu::setFlag(DWORD flags)
{
	flags_	= flags;
}

void Menu::preshow()
{
	if(conststrings_)
	{
		BOOST_ASSERT(strings_.empty());
		for(size_t i = 0; i < items_.size(); ++i)
			if(conststrings_[i] == 0 || conststrings_[i][0] == L'\0')
			{
				set_flag(items_[i].Flags, MIF_SEPARATOR);
				items_[i].Text = 0;
			}
			else
				items_[i].Text = conststrings_[i];
	} else
	{
		for(size_t i = 0; i < items_.size(); ++i)
			if(strings_[i].empty())
			{
				set_flag(items_[i].Flags, MIF_SEPARATOR);
				items_[i].Text = 0;
			}
			else
				items_[i].Text = strings_[i].c_str();
	}
}


int Menu::show(int x, int y)
{
//	BOOST_ASSERT(!items_.empty());

	preshow();
	return getInfo().Menu(getModuleNumber(), x, y, maxHeight_, flags_ | FMENU_USEEXT, 
				title_.c_str(), bottom_.c_str(), helpTopic_.c_str(),
				keys_, keys_? &breakIndex_ : 0, 
				items_.empty()? 0 : reinterpret_cast<const FarMenuItem*>(&items_[0]),
				static_cast<int>(items_.size()));
}

void Menu::setText(size_t item, const std::wstring &text)
{
	BOOST_ASSERT(isConstMode() == false);
	strings_[item] = text;
}


void Menu::setGray(size_t item, bool value)
{
	set_flag(items_[item].Flags, MIF_DISABLE, value);
}

void Menu::setCheck(size_t item, bool value)
{
	set_flag(items_[item].Flags, MIF_CHECKED, value);
}

bool Menu::isChecked(size_t n) const
{
	return is_flag(items_[n].Flags, MIF_CHECKED);
}

void Menu::select(size_t n)
{
	if(selectedItem_ == n)
		return;
	if(n >= items_.size() || n < 0)
		return;
	if(selectedItem_ != no_selected_items)
		clr_flag(items_[selectedItem_].Flags, MIF_SELECTED);

	set_flag(items_[n].Flags, MIF_SELECTED);
	selectedItem_ = n;
}


void Menu::addItem(const std::wstring &text)
{
	BOOST_ASSERT(isConstMode() == false);
	strings_.push_back(text);
	items_.push_back(FarMenuItemEx());
}

void Menu::reserve(size_t n)
{
	strings_.reserve(n);
	items_.reserve(n);
}

size_t Menu::size() const
{
	return items_.size();
}

void Menu::setBreakKeys(const int keys[])
{
	keys_ = keys;
}

int Menu::getBreakIndex() const
{
	BOOST_ASSERT(breakIndex_ != -2);
	return breakIndex_;
}

}
