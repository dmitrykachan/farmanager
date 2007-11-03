#include "stdafx.h"
#include "info.h"

namespace FARWrappers
{
	static PluginStartupInfo	g_info;
	bool						g_initialized = false;

	void setStartupInfo(const PluginStartupInfo &info)
	{
		BOOST_ASSERT(g_initialized == false);
		g_info = info;
		g_initialized = true;
	}

	PluginStartupInfo& getInfo()
	{
		BOOST_ASSERT(g_initialized);
		return g_info;
	}

	INT_PTR getModuleNumber()
	{
		return getInfo().ModuleNumber;
	}

	const wchar_t* getModuleName()
	{
		return getInfo().ModuleName;
	}

	FARSTANDARDFUNCTIONS* getFSF()
	{
		return getInfo().FSF;
	}


	const wchar_t* getMsg(int msgId)
	{
		BOOST_ASSERT(msgId > 0 && msgId < 1000);
		return getInfo().GetMsg(getModuleNumber(), msgId);
	}

	bool getPanelInfo(PanelInfo &pi, bool currentPanel)
	{
		return getInfo().Control(currentPanel? CURRENT_PANEL : ANOTHER_PANEL, FCTL_GETPANELINFO, &pi) != 0;
	}

	bool getShortPanelInfo(PanelInfo &pi, bool currentPanel)
	{
		return getInfo().Control(currentPanel? CURRENT_PANEL : ANOTHER_PANEL, FCTL_GETPANELSHORTINFO, &pi) != 0;
	}


}