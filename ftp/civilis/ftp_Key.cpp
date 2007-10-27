#include "stdafx.h"

#pragma hdrstop

#include "ftp_Int.h"

#include "farwrapper/menu.h"
#include <io.h>
#include "utils/uniconverts.h"


bool FTP::processArrowKeys(int key)
{
	PanelRedrawInfo	ri;
	PanelInfo	pi, otherPi;
	
	switch(key)
	{
		case  VK_END:
		case  VK_HOME:
		case  VK_UP:
		case  VK_DOWN:
			break;
		default:
			return false;
	}
			
	//Get info
	if(!getPanelInfo(pi))
		return false;

	//Skip self processing for QView work correctly
	if (!FARWrappers::getShortPanelInfo(otherPi, false) ||
		(otherPi.PanelType == PTYPE_QVIEWPANEL || otherPi.PanelType == PTYPE_TREEPANEL))
		return false;

	//Get current
	ri.TopPanelItem = pi.TopPanelItem;
	switch(key)
	{
		case   VK_END: ri.CurrentItem = pi.ItemsNumber-1;   break;
		case  VK_HOME: ri.CurrentItem = 0;                  break;
		case    VK_UP: ri.CurrentItem = pi.CurrentItem - 1; break;
		case  VK_DOWN: ri.CurrentItem = pi.CurrentItem + 1; break;
	}

	//Move cursor
	if (!(ri.CurrentItem == pi.CurrentItem || ri.CurrentItem < 0 || ri.CurrentItem >= pi.ItemsNumber))
		FARWrappers::getInfo().Control(this,FCTL_REDRAWPANEL,&ri);
	return true;
}

int FTP::DisplayUtilsMenu()
{
	static const wchar_t* strings[] = 
	{
		/*00*/ getMsg(MHostParams),
		/*01*/ getMsg(MCloseConnection),
		/*02*/ getMsg(MUtilsDir),
		/*03*/ getMsg(MUtilsCmd),
		/*04*/ L"",
		/*05*/ getMsg(MUtilsLog),
		/*06*/ L"", //Sites list
		/*07*/ getMsg(MShowQueue),
	};

	FARWrappers::Menu menu(strings, true, FMENU_WRAPMODE);

#define TOUNICODE_(x) L##x
#define TOUNICODE(x) TOUNICODE_(x)

	std::wstring bottom = std::wstring(getMsg(MVersionTitle)) + 
		L" " + TOUNICODE(__DATE__) + L" " + TOUNICODE(__TIME__);

#if defined(__DEBUG__)
	bottom += L" (DEBUG)";
#endif

	menu.setText(6, backups_.find(this)? getMsg(MRemoveFromites) : getMsg(MAddToSites));

	int     sel = (ShowHosts) ? 5 : 0;

	menu.setTitle(getMsg(MUtilsCaption));
	menu.setBottom(bottom);
	menu.setHelpTopic(L"FTPUtils");

	do
	{
		if(ShowHosts || !getConnection().isConnected())
		{
			menu.setGray(0);
			menu.setGray(1);
			menu.setGray(2);
			menu.setGray(3);
		}

		if(!g_manager.opt.UseBackups)
			menu.setGray(6);

		menu.select(sel);
		sel = menu.show();

		switch(sel)
		{
			case -1: return true;

			//Host parameters
			case  0:
				{
					if(ShowHosts || !getConnection().isConnected()) 
						break;
					FTPHost tmp = chost_;
					if ( !GetHost( MEditFtpTitle, &tmp, FALSE ) ) 
						return true;

					//Reconnect
					if(!chost_.CmpConnected( &tmp ) )
					{
						chost_ = tmp;
						if ( !FullConnect() )
						{
							BackToHosts();
							Invalidate();
						}
						return true;
					}

					chost_ = tmp;

					//Change connection paras
					getConnection().InitData( &chost_,-1 );
					getConnection().InitIOBuff();
					Invalidate();
					return true;
				}

			//Switch to hosts
			case  1: 
				if(ShowHosts) 
					break;
				BackToHosts();
				Invalidate();
					return true;

			//Dir listing
			case  2: 
				if(ShowHosts)
					break;
				{
					int file;
					wchar_t str[_MAX_PATH+1];  //Must be static buff because of MkTemp
					FARWrappers::getFSF()->MkTemp(str, sizeof(str)/sizeof(*str), L"FTP");
					boost::filesystem::wpath path = str;
					boost::filesystem::create_directories(path);
					path /= L"FTPDir.txt";
					file  = _wcreat(path.string().c_str(), 0);
					if(file == -1)
					{
						getConnection().ConnectMessage(MErrorTempFile, path.string(), true, MOk);
						return true;
					}
					_write(file, Unicode::utf16ToUtf8(getConnection().output_).c_str(), static_cast<int>(getConnection().output_.size()));
					_close(file);
					FARWrappers::getInfo().Viewer(path.string().c_str(), 
						(std::wstring(getMsg(MDirTitle)) + L": " + panelTitle_ + L" {" + path.string() + L'}').c_str(),
						0,0,-1,-1,VF_NONMODAL|VF_DELETEONCLOSE );
				}
				return true;

			//Show CMD
			case  3: 
				{
					int saveCmvView;
					if(ShowHosts)
						break;
					saveCmvView = getConnection().getHost().ExtCmdView;
					getConnection().getHost().ExtCmdView = TRUE;
					SetLastError(ERROR_SUCCESS);
					getConnection().ConnectMessage( MNone__,L"", false, MOk);
					getConnection().getHost().ExtCmdView = saveCmvView;
					return true;
				}

			// Show LOG
			case  5: 
				if(IsCmdLogFile())
					FARWrappers::getInfo().Viewer(GetCmdLogFile().c_str(), getMsg(MLogTitle),0,0,-1,-1,VF_NONMODAL|VF_ENABLE_F6 );
				return true;

			// Add\Remove sites list
			case  6: 
				if(backups_.find(this))
					backups_.remove(this);
				else
					backups_.add(this);
				return true;

			//FTP queque
			case  7:
				QuequeMenu();
				return true;
		}
		menu.select(sel);
	} while(1);
	return false;
}

int FTP::ProcessKey(int Key, unsigned int ControlState)
{
	PROCP(Key << L", state: " << ControlState);
	FTPHost		h;

	Key &= 0xFF;

	//process arrows
	if(ControlState == 0 && processArrowKeys(Key))
		return true;

	//Utils menu
	if(ControlState == PKF_SHIFT && Key == VK_F1)
		return DisplayUtilsMenu();

	if(panel_->ProcessKey(Key, ControlState))
		return true;

	return false;
}
