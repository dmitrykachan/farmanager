#include "stdafx.h"
#include "dialog.h"
#include "info.h"

namespace FARWrappers
{

DialogItem Dialog::addButton(int x, int y, int id, const std::wstring& label, int flag)
{
	addItem(DI_BUTTON, x, y, 0, 0, flag, ItemInfo(label, id));
	return DialogItem(this, items_.size()-1);
}

DialogItem Dialog::addDefaultButton(int x, int y, int id, const std::wstring& label, int flag)
{
	FarDialogItem& item = addItem(DI_BUTTON, x, y, 0, 0, flag, ItemInfo(label, id));
	item.DefaultButton	= 1;
	return DialogItem(this, items_.size()-1);
}

DialogItem Dialog::addCheckbox(int x, int y, int *value, const std::wstring& label, int id, DWORD flag)
{
	FarDialogItem& item  = addItem(DI_CHECKBOX, x, y, 0, 0, flag, ItemInfo(label, value, id));
	item.Selected = *value;

	return DialogItem(this, items_.size()-1);
}

DialogItem Dialog::addCombobox()
{
	return DialogItem(this, items_.size()-1);
}

DialogItem Dialog::addDoublebox(int x1, int y1, int x2, int y2, const std::wstring &label, DWORD flag)
{
	FarDialogItem& item  = addItem(DI_DOUBLEBOX, x1, y1, x2, y2, flag, ItemInfo(label, 0));

	return DialogItem(this, items_.size()-1);
}

DialogItem Dialog::addEditor(int x, int y, int x2, std::wstring *value, int id, const wchar_t* history, DWORD flag)
{
	return addEditor(DI_EDIT, x, y, x2, value, id, history, 0, flag);
}

DialogItem Dialog::addFixEditor(int x, int y, int x2, std::wstring *value, int id, const wchar_t* history, DWORD flag)
{
	return addEditor(DI_FIXEDIT, x, y, x2, value, id, history, 0, flag);
}

DialogItem Dialog::addMaskEditor(int x, int y, int x2, std::wstring *value, const wchar_t* mask, int id, DWORD flag)
{
	return addEditor(DI_FIXEDIT, x, y, x2, value, id, 0, mask, flag);
}

DialogItem Dialog::addEditorInt(int x, int y, int x2, int *value, int id, const wchar_t* history, DWORD flag)
{
	return addEditor(DI_EDIT, x, y, x2, value, id, history, 0, flag);
}

DialogItem Dialog::addFixEditorInt(int x, int y, int x2, int *value, int id, const wchar_t* history, DWORD flag)
{
	return addEditor(DI_FIXEDIT, x, y, x2, value, id, history, 0, flag);
}

DialogItem Dialog::addMaskEditorInt(int x, int y, int x2, int *value, const wchar_t* mask, int id, DWORD flag)
{
	return addEditor(DI_FIXEDIT, x, y, x2, value, id, 0, mask, flag);
}

DialogItem Dialog::addPasswordEditor(int x, int y, int x2, std::wstring *value, int id, DWORD flag)
{
	return addEditor(DI_PSWEDIT, x, y, x2, value, id, 0, 0, flag);
}

DialogItem Dialog::addListbox()
{
	return DialogItem(this, items_.size()-1);
}

DialogItem Dialog::addRadioButton(int x, int y, int* value, const std::wstring &label, int id, DWORD flag)
{
	FarDialogItem& item  = addItem(DI_RADIOBUTTON, x, y, 0, 0, flag, ItemInfo(label, value, id));
	item.Selected = *value;

	return DialogItem(this, items_.size()-1);
}

DialogItem Dialog::addRadioButtonStart(int x, int y, int* value, const std::wstring &label, int id, DWORD flag)
{
	return addRadioButton(x, y, value, label, id, flag);
}

DialogItem Dialog::addSinglebox(int x1, int y1, int x2, int y2, const std::wstring &label, DWORD flag)
{
	FarDialogItem& item  = addItem(DI_SINGLEBOX, x1, y1, x2, y2, flag, ItemInfo(label, 0));

	return DialogItem(this, items_.size()-1);
}

DialogItem Dialog::addLabel(int x, int y, const std::wstring& label, int id, DWORD flag)
{
	FarDialogItem& item  = addItem(DI_TEXT, x, y, 0, y, flag, ItemInfo(label, id));

	return DialogItem(this, items_.size()-1);
}

//		DialogItem addUserControl();

DialogItem Dialog::addVText()
{
	return DialogItem(this, items_.size()-1);
}

int Dialog::show(int x1, int y1, int x2, int y2, FARWINDOWPROC dlgProc, LONG_PTR param)
{
	BOOST_ASSERT(!items_.empty());

	std::vector<FarDialogItem>::iterator itr = items_.begin();
	while(itr != items_.end())
	{
		switch(itr->Type)
		{
		case DI_TEXT:
		case DI_BUTTON:
		case DI_CHECKBOX:
		case DI_RADIOBUTTON:
		case DI_DOUBLEBOX:
		case DI_SINGLEBOX:
		case DI_USERCONTROL:
		case DI_VTEXT:
			itr->PtrData = info_[itr-items_.begin()].getText();
			break;
		case DI_COMBOBOX:
		case DI_EDIT:
		case DI_FIXEDIT:
		case DI_PSWEDIT:
			itr->PtrData = info_[itr-items_.begin()].getValue();
		}
		++itr;
	}	

	int res = getInfo().DialogEx(getModuleNumber(), x1, y1, x2, y2, helpTopic_.c_str(),
		&items_[0], static_cast<int>(items_.size()), 0, flags_, 
		dlgProc? dlgProc : getInfo().DefDlgProc, param);

	if(res != -1 && info_[res].id_ != -1)
	{
		itr = items_.begin();
		while(itr != items_.end())
		{
			switch(itr->Type)
			{
			case DI_EDIT:
			case DI_FIXEDIT:
			case DI_PSWEDIT:
				info_[itr - items_.begin()].set(itr->PtrData);
				break;
			case DI_COMBOBOX:
				info_[itr - items_.begin()].set(itr->PtrData, itr->Selected);
			case DI_CHECKBOX:
			case DI_RADIOBUTTON:
			case DI_LISTBOX:
				info_[itr - items_.begin()].set(itr->Selected);
			}
			++itr;
		}
		return info_[res].id_;
	}
	else
		return -1;
}

int Dialog::show(int width, int height, FARWINDOWPROC dlgProc, LONG_PTR param)
{
	return show(-1, -1, width, height, dlgProc, param);
}

DialogItem Dialog::addHLine(int x, int y)
{
	FarDialogItem& item  = addItem(DI_TEXT, x, y, x, y, DIF_BOXCOLOR|DIF_SEPARATOR, ItemInfo(L"", 0));
	return DialogItem(this, items_.size()-1);
}

const FarDialogItem& Dialog::get(size_t n) const
{
	return items_[n];
}

FarDialogItem& Dialog::get(size_t n)
{
	return items_[n];
}

FarDialogItem& DialogItem::get() const
{
	return dlg_->get(item_);
}

DWORD Dialog::colorFlag(int foreground, int background)
{
	return ((background & 0x0F) << 4) | (foreground & 0x0F) | DIF_SETCOLOR;
}

int Dialog::getID(size_t n) const
{
	return info_[n].id_;
}

DialogItem Dialog::find(int id)
{
	std::vector<ItemInfo>::iterator itr;
	itr = std::find_if(info_.begin(), info_.end(), boost::bind(&ItemInfo::id_, _1) == id);
	if(itr != info_.end())
		return DialogItem(this, itr - info_.begin());
	return DialogItem(0, 0);
}

} // namespace FarWrappers

