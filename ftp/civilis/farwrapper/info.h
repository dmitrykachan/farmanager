#ifndef __FAR_WRAPPERS_INFO_INCLUDE
#define __FAR_WRAPPERS_INFO_INCLUDE

namespace FARWrappers
{
	void				setStartupInfo(const PluginStartupInfo& info);
	PluginStartupInfo&	getInfo();
	FARSTANDARDFUNCTIONS* getFSF();
	INT_PTR				getModuleNumber();
	const wchar_t*		getModuleName();
	const wchar_t*		getMsg(int msgId);

	struct PanelInfoAuto: public PanelInfo
	{
		PanelInfoAuto(bool currentPanel, bool shortinfo)
		{
			plugin_ = currentPanel? PANEL_ACTIVE : PANEL_PASSIVE;
			getPanelInfo(shortinfo);
		}

		PanelInfoAuto(HANDLE plugin, bool shortinfo)
		{
			plugin_ = plugin;
			getPanelInfo(shortinfo);
		}

		~PanelInfoAuto()
		{
			freePanelInfo();
		}

	private:
		HANDLE	plugin_;

		void freePanelInfo()
		{
			getInfo().Control(plugin_, FCTL_FREEPANELINFO, this);
		}
		void getPanelInfo(bool shortInfo)
		{
			int command = shortInfo? FCTL_GETPANELSHORTINFO : FCTL_GETPANELINFO;
			if(getInfo().Control(plugin_, command, this) == 0)
				throw std::runtime_error("FCTL_GETPANELINFO failed");
		}
	};
}

using FARWrappers::getMsg;

#endif