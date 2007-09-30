#ifndef __FAR_WRAPPERS_MESSAGE_HEADER
#define __FAR_WRAPPERS_MESSAGE_HEADER


namespace FARWrappers
{
	void setStandartMessages(int mYes, int mNo, int mOk, int mAttention);
	template<size_t size>
	int message(const wchar_t*(&items)[size], int buttonsNumber = 0, DWORD flags = 0, const std::wstring& helpTopic = L"")
	{
		return getInfo().Message(getModuleNumber(), flags, helpTopic.c_str(), items, size, buttonsNumber);
	}

	const unsigned int FMSG_MB_MASK = 0x000F0000;       //Mask for all FMSG_MB_xxx

	int messageYesNo(const std::wstring &str, DWORD flag = FMSG_WARNING);
	void message	(const std::wstring &str, DWORD flag = FMSG_WARNING);
	void message	(int messageId, DWORD flag = FMSG_WARNING);
	void message	(int titleId, const std::wstring& mess, DWORD flag = FMSG_WARNING);
	void message	(int titleId, int messageId, DWORD flag = FMSG_WARNING);
	int  message	(const std::vector<std::wstring> &items, int buttonsNumber, DWORD flags, const wchar_t* helpTopic = 0);
}

#endif