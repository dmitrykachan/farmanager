#ifndef DIAlOG_FAR_WRAPPERS
#define DIAlOG_FAR_WRAPPERS

namespace FARWrappers
{

class Dialog;

class DialogItem
{
private:
	DialogItem(Dialog* dlg, size_t n)
		: dlg_(dlg), item_(n)
	{}
	FarDialogItem& get() const;

	Dialog* dlg_;
	size_t	item_;
	friend class Dialog;
public:
	Dialog* operator ->() const
	{
		return dlg_;
	}
	DialogItem setFocus(bool focus = true)
	{
		get().Focus = focus;
		return *this;
	}
	DialogItem setSelect(bool selected = true)
	{
		get().Selected = selected;
		return *this;
	}
	DialogItem enable()
	{
		get().Flags &= ~DIF_DISABLE;
		return *this;
	}
	DialogItem disable()
	{
		get().Flags |= DIF_DISABLE;
		return *this;
	}
	DialogItem setFlag(DWORD flag)
	{
		get().Flags = flag;
		return *this;
	}
};

class Dialog
{
private:

	struct ItemInfo 
	{
		ItemInfo(const std::wstring& text, int id)
			: text_(text), pValueString_(0), pValueInt_(0), id_(id)
		{};
		ItemInfo(const std::wstring& text, std::wstring* value, int id)
			: pValueString_(value), pValueInt_(0), id_(id), text_(text)
		{};
		ItemInfo(const std::wstring& text, int* value, int id)
			: pValueString_(0), pValueInt_(value), id_(id), text_(text)
		{};

		wchar_t* getText() const
		{
			return const_cast<wchar_t*>(text_.c_str());
		}

		wchar_t* getValueStr() const
		{
			BOOST_ASSERT(pValueString_);
			return const_cast<wchar_t*>(pValueString_->c_str());
		}

		wchar_t* getValue()
		{
			if(pValueString_ != 0)
				return const_cast<wchar_t*>(pValueString_->c_str());
			if(pValueInt_ != 0)
			{
				text_ = boost::lexical_cast<std::wstring>(*pValueInt_);
				return const_cast<wchar_t*>(text_.c_str());
			}
			BOOST_ASSERT(0);
			return const_cast<wchar_t*>(text_.c_str());
		}

		void set(wchar_t* value)
		{
			if(pValueString_ != 0)
			{
				pValueString_->assign(value);
				return;
			}
			if(pValueInt_ != 0)
			{
				try
				{
					std::wstring v = value;
					*pValueInt_	= boost::lexical_cast<int>(boost::trim_copy(v));
				}
				catch (boost::bad_lexical_cast &)
				{
					BOOST_LOG(WRN, std::wstring(L"bad int: ") + value);
				}
				return;
			}
			BOOST_ASSERT(0);
		}

		void set(wchar_t* value, int selected)
		{
			BOOST_ASSERT(pValueInt_ && pValueString_);
			pValueString_->assign(value);
			*pValueInt_	= selected;
		}

		void set(int selected)
		{
			BOOST_ASSERT(pValueInt_);
			*pValueInt_	= selected;
		}


		std::wstring			text_;
		int						id_;
		std::wstring*			pValueString_;
		int*					pValueInt_;
	};

	DWORD						flags_;
	std::wstring				helpTopic_;
	std::vector<FarDialogItem>	items_;
	std::vector<ItemInfo>		info_;

	FarDialogItem& addItem(DialogItemTypes type, int x1, int y1, int x2, int y2, 
							DWORD flag, ItemInfo& info)
	{
		FarDialogItem item;
		item.Type	= type;
		item.X1		= x1;
		item.Y1		= y1;
		item.X2		= x2;
		item.Y2		= y2;
		item.Flags	= flag;

		item.Focus			= 0;
		item.Reserved		= 0;
		item.DefaultButton	= 0;
		item.PtrLength		= 0;

		info_.push_back(info);
		items_.push_back(item);
		return *(items_.end()-1);

	}

	template<typename Value>
	DialogItem addEditor(DialogItemTypes type, int x, int y, int x2,
		Value *value, int id, const wchar_t* history, const wchar_t* mask, DWORD flag)
	{
		BOOST_ASSERT(!(history && mask));
		if(history)
			flag |= DIF_HISTORY;
		else
			BOOST_ASSERT(!(flag & DIF_HISTORY));

		if(mask)
			flag |= DIF_MASKEDIT;
		else
			BOOST_ASSERT(!(flag & DIF_MASKEDIT));

		FarDialogItem& item = addItem(type, x, y, x2, y, flag, ItemInfo(L"", value, id));
		if(history)
			item.History		= history;
		else
			if(mask)
				item.Mask		= mask;

		return DialogItem(this, items_.size()-1);
	}

public:
	Dialog(const std::wstring& helpTopic = L"", DWORD flags = 0)
		: flags_(flags), helpTopic_(helpTopic)
	{
	}

	int 		show(int x1, int y1, int x2, int y2, FARWINDOWPROC dlgProc = 0, LONG_PTR param = 0);
	int			show(int width, int height, FARWINDOWPROC dlgProc = 0, LONG_PTR param = 0);
	DialogItem	addButton(int x, int y, int id, const std::wstring& label, int flag = 0);
	DialogItem	addDefaultButton(int x, int y, int id, const std::wstring& label, int flag = 0);
	DialogItem	addCheckbox(int x, int y, int *value, const std::wstring& label, int id = 0, DWORD flag = 0);
	DialogItem	addCombobox();
	DialogItem	addDoublebox(int x1, int y1, int x2, int y2, const std::wstring &label = L"", DWORD flag = 0);
	DialogItem	addEditor(int x, int y, int x2, std::wstring *value, int id = 0, const wchar_t* history = 0, DWORD flag = 0);
	DialogItem	addFixEditor(int x, int y, int x2, std::wstring *value, int id = 0, const wchar_t* history = 0, DWORD flag = 0);
	DialogItem	addMaskEditor(int x, int y, int x2, std::wstring *value, const wchar_t* mask, int id = 0, DWORD flag = 0);
	DialogItem	addEditorInt(int x, int y, int x2, int *value, int id = 0, const wchar_t* history = 0, DWORD flag = 0);
	DialogItem	addFixEditorInt(int x, int y, int x2, int *value, int id = 0, const wchar_t* history = 0, DWORD flag = 0);
	DialogItem	addMaskEditorInt(int x, int y, int x2, int *value, const wchar_t* mask, int id = 0, DWORD flag = 0);
	DialogItem	addListbox();
	DialogItem	addPasswordEditor(int x, int y, int x2, std::wstring *value, int id = 0, DWORD flag = 0);
	DialogItem	addRadioButton(int x, int y, int* value, const std::wstring &label, int id = 0, DWORD flag = 0);
	DialogItem	addRadioButtonStart(int x, int y, int* value, const std::wstring &label, int id = 0, DWORD flag = DIF_GROUP);
	DialogItem	addSinglebox(int x1, int y1, int x2, int y2, const std::wstring &label = L"", DWORD flag = 0);
	DialogItem	addLabel(int x, int y, const std::wstring& label, int id = 0, DWORD flag = 0);
//		DialogItem& addUserControl();
	DialogItem	addVText();
	DialogItem	addHLine(int x, int y);

	int			getID(size_t n) const;

	const FarDialogItem& get(size_t n) const;
	FarDialogItem&		get(size_t n);
	DialogItem find(int id);

	static DWORD colorFlag(int foreground, int background);
};

} // namespace FARWrappers 


#endif

