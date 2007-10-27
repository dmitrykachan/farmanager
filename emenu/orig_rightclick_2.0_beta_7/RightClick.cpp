// RightClick by SA
//

#include "Plugin.h"
#include <cassert>

CPlugin thePlug;

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD nReason, LPVOID)
{
	if (nReason==DLL_PROCESS_ATTACH)
	{
		thePlug.m_hModule=hModule;
		if (!DisableThreadLibraryCalls(hModule))
		{
			assert(0);
		}
	}
	return TRUE;
}

int WINAPI _export GetMinFarVersion()
{
	return thePlug.GetMinFarVersion();
}

void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info)
{
	thePlug.SetStartupInfo(Info);
}

void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
	thePlug.GetPluginInfo(Info);
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom, int Item)
{
	return thePlug.OpenPlugin(OpenFrom, Item);
}

int WINAPI _export Configure(int)
{
	return thePlug.Configure();
}

void WINAPI _export ExitFAR()
{
	thePlug.ExitFAR();
}
