#ifndef __FAR_WRAPPERS_UTILS__
#define __FAR_WRAPPERS_UTILS__

namespace FARWrappers
{
	inline int getForegroundColor(int color)
	{
		return color & 0x0F;
	}

	inline int getBackgroundColor(int color)
	{
		return (color >> 4) & 0x0F;
	}

	inline int makeColor(int foreground, int background)
	{
		return ((background & 0x0F) << 4) | (foreground & 0x0F);
	}

	bool patternCmp(const std::wstring &str, const std::wstring &pattern, wchar_t *delimiters = L",;", bool skipPath = true);


	class Screen
	{
	public:
		Screen()	{ save(); }
		~Screen()	{ restore(); }

		static void save();                 //Save console screen, inc save counter
		static void restore();              //Dec save counter, Restore console screen on zero
		static void restoreWithoutNotes();  //Restore screen without counter changes
		static void fullRestore();          //Dec counter to zero and restore screen
		static bool isSaved();              //Save counter value
	private:
		static int			saveCount_;
		static std::wstring saveTitle_;
		static HANDLE		hScreen_;
	};

	enum FarConsoleColors 
	{
		fccBLACK,          /* dark colors */
		fccBLUE,
		fccGREEN,
		fccCYAN,
		fccRED,
		fccMAGENTA,
		fccBROWN,
		fccLIGHTGRAY,
		fccDARKGRAY,       /* light colors */
		fccLIGHTBLUE,
		fccLIGHTGREEN,
		fccLIGHTCYAN,
		fccLIGHTRED,
		fccLIGHTMAGENTA,
		fccYELLOW,
		fccWHITE
	};

	void convertFindData(WIN32_FIND_DATAW& w, const FAR_FIND_DATA &f);
}

template<typename T, typename F>
static void set_flag(T &flags, F flag)
{
	flags |= (flag);
}

template<typename T, typename F>
static void clr_flag(T &flags, F flag)
{
	flags &= ~(flag);
}

template<typename T, typename F>
static void set_flag(T &flags, F flag, bool value)
{
	if(value)
		set_flag(flags, flag);
	else
		clr_flag(flags, flag);
}

template<typename T, typename F>
static bool is_flag(const T &flags, F flag)
{
	return (flags & flag) != 0;
}



#endif