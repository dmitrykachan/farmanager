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

	static void getCurrentItem(bool currentPanel, PluginPanelItem &item)
	{
		HANDLE plugin = currentPanel? PANEL_ACTIVE : PANEL_PASSIVE;
		if(getInfo().Control(plugin, FCTL_GETCURRENTPANELITEM, 0, reinterpret_cast<LONG_PTR>(&item)) == 0)
			throw std::runtime_error("FCTL_GETCURRENTPANELITEM failed");
	}

	static PanelInfo getPanelInfo(bool currentPanel = true)
	{
		HANDLE plugin = currentPanel? PANEL_ACTIVE : PANEL_PASSIVE;
		PanelInfo info;
		if(getInfo().Control(plugin, FCTL_GETPANELINFO, 0, reinterpret_cast<LONG_PTR>(&info)) == 0)
			throw std::runtime_error("FCTL_GETCURRENTPANELITEM failed");
		return info;
	}

	static std::wstring getCurrentDirectory(bool currentPanel = true)
	{
		HANDLE plugin = currentPanel? PANEL_ACTIVE : PANEL_PASSIVE;
		int size = getInfo().Control(plugin, FCTL_GETCURRENTDIRECTORY, 0, 0);
		if(size == 0)
			return L"";
		boost::scoped_array<wchar_t> buffer(new wchar_t[size]);
		getInfo().Control(plugin, FCTL_GETCURRENTDIRECTORY, 0, reinterpret_cast<LONG_PTR>(buffer.get()));
		return std::wstring(buffer.get());
	}


	class PanelItem
	{
	private:
		PluginPanelItem item_;
		bool			initialazed_;
		HANDLE			plugin_;

		void free()
		{
			if(initialazed_)
			{
				int res = getInfo().Control(plugin_, FCTL_FREEPANELITEM, 0, reinterpret_cast<LONG_PTR>(&item_));
				BOOST_ASSERT(res != 0);
				initialazed_ = false;
			}
		}

	public:
		PanelItem(size_t n, bool currentPanel = true)  // get panel item n
		{
			plugin_ = currentPanel? PANEL_ACTIVE : PANEL_PASSIVE;
			getPanelItem(n);
		}

		PanelItem(bool currentPanel = true)
		{
			plugin_ = currentPanel? PANEL_ACTIVE : PANEL_PASSIVE;
			initialazed_ = false;
		}

		void getPanelItem(size_t n)
		{
			free();
			if(getInfo().Control(plugin_, FCTL_GETPANELITEM, n, reinterpret_cast<LONG_PTR>(&item_)) == 0)
				throw std::runtime_error("FCTL_GETPANELITEM failed");
			initialazed_ = true;
		}

		void getSelectedPanelItem(size_t n)
		{
			free();
			if(getInfo().Control(plugin_, FCTL_GETSELECTEDPANELITEM, n, reinterpret_cast<LONG_PTR>(&item_)) == 0)
				throw std::runtime_error("FCTL_GETSELECTEDPANELITEM failed");
			initialazed_ = true;
		}

		void getCurrentPanelItem(size_t n)
		{
			free();
			if(getInfo().Control(plugin_, FCTL_GETCURRENTPANELITEM , n, reinterpret_cast<LONG_PTR>(&item_)) == 0)
				throw std::runtime_error("FCTL_GETCURRENTPANELITEM failed");
			initialazed_ = true;
		}

		const PluginPanelItem& get() const
		{
			if(!initialazed_)
			{
				BOOST_ASSERT(0 && "PanelItem doesn't initialized");
				throw std::runtime_error("PanelItem doesn't initialized");
			}
			return item_;
		}

		~PanelItem()
		{
			free();
		}
	};

	class PanelItemEnum
	{
	private:
		PluginPanelItem item_;
		int				size_;
		int				current_;
		bool			selected_;
		HANDLE			plugin_;

		void free()
		{
			if(current_ != 0)
			{
				int res = getInfo().Control(plugin_, FCTL_FREEPANELITEM, 0, reinterpret_cast<LONG_PTR>(&item_));
				BOOST_ASSERT(res != 0);
			}
		}

	public:
		PanelItemEnum(bool currentPanel, bool selected)
			: plugin_(currentPanel? PANEL_ACTIVE : PANEL_PASSIVE), 
			  selected_(selected), current_(0)
		{
			size_ = getPanelInfo(plugin_).ItemsNumber;
		}

		bool hasNext()
		{
			return current_ < size_;
		}

		const PluginPanelItem& getNext()
		{
			free();
			if(selected_)
			{
				if(getInfo().Control(plugin_, FCTL_GETSELECTEDPANELITEM, current_, reinterpret_cast<LONG_PTR>(&item_)) == false)
					throw std::runtime_error("FCTL_GETSELECTEDPANELITEM failed");
			}
			else
			{
				if(getInfo().Control(plugin_, FCTL_GETPANELITEM, current_, reinterpret_cast<LONG_PTR>(&item_)) == false)
					throw std::runtime_error("FCTL_GETPANELITEM failed");
			}
			++current_;
			return item_;
		}

	};
}

using FARWrappers::getMsg;

#endif