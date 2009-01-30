#include "stdafx.h"
#include "message.h"
#include "info.h"

namespace FARWrappers
{
	enum StandartMsg
	{
		MessageYes, MessageNo, MessageOk, MessageAttention, StandartMessageCount
	};

	boost::array<int, StandartMessageCount> g_standartMessages;
	void setStandartMessages(int mYes, int mNo, int mOk, int mAttention)
	{
		g_standartMessages[MessageYes]		= mYes;
		g_standartMessages[MessageNo]		= mNo;
		g_standartMessages[MessageOk]		= mOk;

		g_standartMessages[MessageAttention] = mAttention;
	}

	static const wchar_t* getStandartMessage(StandartMsg id)
	{
		return getMsg(g_standartMessages[id]);
	}

	int messageYesNo(const std::wstring &str, DWORD flag)
	{
		const wchar_t* msgItems[] =
		{
			getStandartMessage(MessageAttention),
			str.c_str(),
			getStandartMessage(MessageYes),
			getStandartMessage(MessageNo)
		};

		return message(msgItems, 2, flag) == 0;
	}

	void message(int titleId, const std::wstring& mess, DWORD flag)
	{
		const wchar_t* items[] =
		{
			getMsg(titleId),
			mess.c_str(),
			getStandartMessage(MessageOk)
		};
		message(items, 1, flag);
	}

	void message(int titleId, int messageId, DWORD flag)
	{
		const wchar_t* items[] =
		{
			getMsg(titleId),
			getMsg(messageId),
			getStandartMessage(MessageOk)
		};
		message(items, 1, flag);
	}

	void message(const std::wstring &str, DWORD flag)
	{
		message(g_standartMessages[MessageAttention], str, flag);
	}

	void message(int messageId, DWORD flag)
	{
		message(g_standartMessages[MessageAttention], messageId, flag);
	}

	int message(const std::vector<std::wstring> &items, int buttonsNumber, DWORD flags, const wchar_t* helpTopic)
	{
		int    rc;

		static size_t lastMsgWidth  = 0;
		static size_t lastMsgHeight = 0;

		bool   buttons = buttonsNumber || (flags & FMSG_MB_MASK);

		//Array of lines - check if lines are message id
 		size_t width = std::max_element(items.begin(), items.end(), 
 			boost::bind(&std::wstring::size, _1) < boost::bind(&std::wstring::size, _2))->size();

		if(!buttons) // No FAR buttons
		{

			//Calc if message need to be redrawn with smaller dimensions
// 			if (!lastMsgWidth || width < lastMsgWidth ||
// 				!lastMsgHeight|| items.size() < lastMsgHeight)
// 				FP_Screen::RestoreWithoutNotes();
		}

		boost::scoped_array<const wchar_t*> v(new const wchar_t* [items.size()]);
		const wchar_t** p = v.get();
		BOOST_FOREACH(const std::wstring& s, items)
		{
			*p = s.c_str();
			++p;
		}
		rc = getInfo().Message(getModuleNumber(), flags, helpTopic,
					v.get(), static_cast<int>(items.size()), buttonsNumber);

		lastMsgWidth	= width;
		lastMsgHeight	= items.size();

		return rc;
	}


} // namaspace FARWrappers