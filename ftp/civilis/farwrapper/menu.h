#ifndef __FAR_WRAPPER_MENU_H__
#define __FAR_WRAPPER_MENU_H

namespace FARWrappers
{

class Menu: boost::noncopyable
{
public:
	Menu(DWORD flags = 0)
		: conststrings_(0), flags_(flags), selectedItem_(no_selected_items), maxHeight_(0)
	{
		reserve(reserveSize);
		keys_ = 0;
		breakIndex_ = -2;
	}
	template<size_t size>
	Menu(const wchar_t*(&items)[size], bool copy = true, DWORD flags = 0)
		: flags_(flags), selectedItem_(no_selected_items), maxHeight_(0)
	{
		keys_ = 0;
		breakIndex_ = -2;
		init(items, size, copy);
	}

	~Menu();

	void	select	(size_t n);
	void	setCheck(size_t n, bool val = true);
	bool	isChecked(size_t n) const;
	int		show	(int x = -1, int y = -1);
	void	setTitle(const std::wstring& title);
	void	setBottom(const std::wstring& bottom);
	void	setHelpTopic(const std::wstring& helpTopic);
	void	setFlag	(DWORD flags);
	void	setMaxHeight(int maxHeight = 0);
	void	setText	(size_t item, const std::wstring &text);
	void	setGray(size_t item, bool value = true);

	void	addItem(const std::wstring &text);
	void	reserve(size_t n);
	size_t  size() const;
	void	setBreakKeys(const int keys[]);
	int     getBreakIndex() const;

private:
	enum { no_selected_items = INT_MAX, reserveSize = 10 };
	std::vector<FarMenuItemEx>	items_;
	const wchar_t**				conststrings_;
	std::vector<std::wstring>	strings_;
	DWORD						flags_;
	std::wstring				title_;
	std::wstring				bottom_;
	std::wstring				helpTopic_;
	int							maxHeight_;
	size_t						selectedItem_;
	const int*					keys_;
	int							breakIndex_;
	
	void	init(const wchar_t** items, size_t size, bool copy);
	void	preshow();
	bool	isConstMode() const
	{
		return conststrings_ != 0;
	}
};

}

#endif