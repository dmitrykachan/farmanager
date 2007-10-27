#include "stdafx.h"
#include "dialog.h"
#include "stddialogs.h"
#include "utils.h"



namespace FARWrappers
{

static int  ColorFore;
static int  ColorBk;
static std::wstring g_Title;

typedef boost::tuple<FARWrappers::Dialog*, std::wstring*, int*> ColorsParameters;

const int startTextId			 = 1;
const int startForegroundColorId = startTextId + 1;
const int startBackgroundColorId = startForegroundColorId + 16;

std::wstring makeTitle(const std::wstring title, int color)
{
	std::wstringstream ws;
	ws	<< L'(' << std::setw(3) << color << L' '
		<< L"0x" << std::hex << std::setw(2) << std::setfill(L'0') << color 
		<< L' ' << std::oct << std::setw(3) << color << L')';
	return title + ws.str();
}

static LONG_PTR WINAPI CDLG_WndProc(HANDLE hDlg,int Msg, int Param1, LONG_PTR Param2)
{
	using namespace FARWrappers;
	static Dialog* dlg;
	static std::wstring* title;
	static int* pColor;

	switch(Msg)
	{
	case DN_INITDIALOG:
		{
			ColorsParameters* p = reinterpret_cast<ColorsParameters*>(Param2);
			dlg		= boost::get<0>(*p);
			title	= boost::get<1>(*p);
			pColor	= boost::get<2>(*p);
			break;
		}
	case DN_BTNCLICK:
		{
			int id = dlg->getID(Param1);
			int oldColor = *pColor;
			if(id >= startForegroundColorId && id < startForegroundColorId + 16)
			{
				*pColor = makeColor(id - startForegroundColorId, getBackgroundColor(*pColor));
			} else
				if(id >= startBackgroundColorId && id < startBackgroundColorId + 16)
				{
					*pColor = makeColor(getForegroundColor(*pColor), id - startBackgroundColorId);
				}
				if(oldColor != *pColor)
				{
					FARWrappers::getInfo().SendDlgMessage(hDlg, DM_SETTEXTPTR, 0, 
						reinterpret_cast<LONG_PTR>(makeTitle(*title, *pColor).c_str()));
					FARWrappers::getInfo().SendDlgMessage( hDlg,DM_REDRAW,0,0 );
				}
		}
		break;
	case DN_CTLCOLORDLGITEM:
		if(dlg->getID(Param1) == startTextId)
			return *pColor;
		break;
	}
	return getInfo().DefDlgProc( hDlg,Msg,Param1,Param2 );
}

static void ColorGroup(FARWrappers::Dialog& dlg, int x, int y, int x2, int y2, const std::wstring title, int firstId)
{
	using namespace FARWrappers;

	static int sel;

	dlg.addSinglebox(x, y, x2, y2,	title, DIF_BOXCOLOR|DIF_LEFTTEXT);
	++x;
	++y;

	DWORD flag = DIF_GROUP | DIF_MOVESELECT|DIF_SETCOLOR |
		Dialog::colorFlag(0x7, 0);
	dlg.addRadioButton(x, y, &sel, L"", firstId, flag);

	for(int i = 1; i < 16; ++i)
	{
		flag = DIF_MOVESELECT|DIF_SETCOLOR |
			Dialog::colorFlag((~i)& 0x7, i);
		dlg.addRadioButton(x+(i/4)*3, y + i%4, &sel, L"", i + firstId, flag);
	}
}

int ShowColorColorDialog(int color,FLngColorDialog* p, const std::wstring &help)
{
	using namespace FARWrappers;

	Dialog dlg(help);

	dlg.addDoublebox( 3, 1,35,13,	makeTitle(p->title_, color), DIF_BOXCOLOR)->
		addLabel	( 5, 8,			p->text_, startTextId,	dlg.colorFlag(fccYELLOW,fccGREEN))->
		addLabel	( 5, 9,			p->text_, startTextId,	dlg.colorFlag(fccYELLOW,fccGREEN))->
		addLabel	( 5,10,			p->text_, startTextId,	dlg.colorFlag(fccYELLOW,fccGREEN))->
		addHLine	( 3,11)->
		addDefaultButton(0,12, 0,	p->set_,				DIF_CENTERGROUP)->
		addButton	( 0,12,   -1,	p->cancel_,				DIF_CENTERGROUP);

	ColorGroup(dlg,  5, 2, 18, 7, p->fore_, startForegroundColorId);
	ColorGroup(dlg, 20, 2, 33, 7, p->bk_, startBackgroundColorId);

	DialogItem item = dlg.find(startForegroundColorId + getForegroundColor(color));
	item.setFocus();
	item.setSelect();

	dlg.find(startBackgroundColorId + getBackgroundColor(color)).setSelect();

	int newcolor = color;
	ColorsParameters param = boost::make_tuple(&dlg, &p->title_, &newcolor);

	int rc = dlg.show(39, 15, &CDLG_WndProc, reinterpret_cast<LONG_PTR>(&param));
	if(rc != -1)
		return newcolor;

	return color;
}

} // namespace FarWrappers
