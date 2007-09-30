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

	bool				getPanelInfo(PanelInfo &pi, bool currentPanel = true);
	bool				getShortPanelInfo(PanelInfo &pi, bool currentPanel = true);

}

using FARWrappers::getMsg;

#endif