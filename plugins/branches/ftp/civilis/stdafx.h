#pragma once

#ifdef _DEBUG
#define DEBUG_NEW   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
#define DEBUG_NEW
#endif // _DEBUG


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

#include "config.h"

#include "../common/unicode/plugin.hpp"

#ifdef CONFIG_TEST
#include <boost/test/auto_unit_test.hpp>
#endif


#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

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
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include "utils/log.h"

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
#include <map>
#include <hash_set>

#include <Winsock2.h>

#include "utils/winwrapper.h"
#include "utils/sregexp.h"
#include "utils/regkey.h"

#include "farwrapper/info.h"
#include "farwrapper/panel.h"
#include "farwrapper/utils.h"
#include "farwrapper/info.h"
#include "farwrapper/message.h"


extern std::locale defaultLocale_;

#if !defined( ARRAY_SIZE )
	#define ARRAY_SIZE( v )  (sizeof(v) / sizeof( (v)[0] ))
#endif

