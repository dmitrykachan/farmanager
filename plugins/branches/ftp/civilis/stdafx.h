#pragma once


// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.       
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _ATL_SECURE_NO_DEPRECATE
#define _AFX_SECURE_NO_DEPRECATE
#pragma warning(disable : 4996)

#define NOMINMAX


#include "../common/plugin.hpp"

#ifdef CONFIG_TEST

//#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

#endif


//#include <windows.h>

#include "config.h"
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>

#pragma warning (push)
#pragma warning(disable : 4180)
#include <boost/bind.hpp>
#pragma warning (pop)

#include <boost/static_assert.hpp>
#include <boost/array.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/foreach.hpp>

BOOST_STATIC_ASSERT(sizeof(PluginPanelItem) == 366);

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>
#include <fstream>
#include <limits>
#include <exception>
#include <numeric>


#include "utils/winwrapper.h"
#include "utils/sregexp.h"
#include "utils/regkey.h"

extern std::locale defaultLocale_;
