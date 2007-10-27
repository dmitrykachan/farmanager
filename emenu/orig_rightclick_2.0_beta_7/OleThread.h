#ifndef _OLETHREAD_H_
#define _OLETHREAD_H_

#include "Plugin.h"

namespace OleThread
{
	CPlugin::EDoMenu OpenPlugin(int nOpenFrom, int nItem);
	void Stop();
};

#endif