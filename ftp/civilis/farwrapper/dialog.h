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

	struct ItemValue 
	{
		ItemValue(const std::wstring& text, int id)
			: text_(text), pValueString_(0), type_(ValLess), id_(id)
		{};
		ItemValue(const std::wstring& text, std::wstring* value, int id)
			: pValueString_(value), type_(StringVal), id_(id), text_(text)
		{};
		ItemValue(const std::wstring& text, int* value, int id)
			: pValueInt_(value), type_(IntVal), id_(id), text_(text)
		{};

		ItemValue(const std::wstring& text, bool* value, int id)
			: pValueBool_(value), type_(BoolVal), id_(id), text_(text)
		{};

		wchar_t* getText() const
		{
			return const_cast<wchar_t*>(text_.c_str());
		}

		wchar_t* getValueStr() const
		{
			BOOST_ASSERT(type_ == StringVal);
			return const_cast<wchar_t*>(pValueString_->c_str());
		}

		wchar_t* getValue()
		{
			switch(type_)
			{
				case StringVal:
					return const_cast<wchar_t*>(pValueString_->c_str());
				case IntVal:
					text_ = boost::lexical_cast<std::wstring>(*pValueInt_);
					break;
				case BoolVal:
					text_ = boost::lexical_cast<std::wstring>(*pValueBool_);
					break;
				default:
					BOOST_ASSERT(0 && "unsupported type");
			}
			return const_cast<wchar_t*>(text_.c_str());
		}

		void set(const wchar_t* value)
		{
			try
			{
				switch(type_)
				{
				case StringVal:
					pValueString_->assign(value);
					return;
				case IntVal:
					*pValueInt_	= boost::lexical_cast<int>(boost::trim_copy(std::wstring(value)));
					break;
				case BoolVal:
					*pValueBool_= boost::lexical_cast<int>(boost::trim_copy(std::wstring(value))) != 0;
					break;
				default:
					BOOST_ASSERT(0 && "unsupported type");
				}
			}
			catch (boost::bad_lexical_cast &)
			{
				BOOST_LOG(WRN, std::wstring(L"bad value: ") + value);
			}
		}

		void set(const wchar_t* value, int selected)
		{
			BOOST_ASSERT(0);
			BOOST_ASSERT(pValueInt_ && pValueString_);
			pValueString_->assign(value);
			*pValueInt_	= selected;
		}

		void set(int selected)
		{
			switch(type_)
			{
			case IntVal:
				*pValueInt_ = selected;
				break;
			case BoolVal:
				*pValueBool_ = selected != 0;
			    break;
			default:
				BOOST_ASSERT(0 && "Incorrect type");
			    break;
			}
		}


		enum Type
		{
			StringVal, IntVal, BoolVal, ValLess
		};
		std::wstring			text_;
		int						id_;
		Type					type_;

		union
		{
			std::wstring*		pValueString_;
			int*				pValueInt_;
			bool*				pValueBool_;
		};
	};

	DWORD						flags_;
	std::wstring				helpTopic_;
	std::vector<FarDialogItem>	items_;
	std::vector<ItemValue>		info_;

	FarDialogItem& addItem(DialogItemTypes type, int x1, int y1, int x2, int y2, 
							DWORD flag, const ItemValue& info)
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
		item.MaxLen			= 0;

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

		FarDialogItem& item = addItem(type, x, y, x2, y, flag, ItemValue(L"", value, id));
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
	DialogItem	addCheckbox(int x, int y, bool *value, const std::wstring& label, int id = 0, DWORD flag = 0);
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

