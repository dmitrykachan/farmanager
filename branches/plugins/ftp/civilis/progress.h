#pragma once

class Connection;
#include "fileinfo.h"

class InfoItem;

class FTPProgress
{
public:
	FTPProgress();
	~FTPProgress();

	enum Show
	{
		ShowProgress, ShowIdle, NotDisplay
	};

	void resume(__int64 size);
	bool refresh(Connection* connection);
	bool refresh(Connection* connection, unsigned __int64 size);
	void Init(int tMsg,int OpMode, const FileList &filelist, Show show);
	void initFile(Connection* connection, const FTPFileInfo& info, const std::wstring &destName);
	void initFile();

	void skip();
	void Waiting(size_t paused);
	bool isShow() const
	{
		return show_ == ShowProgress;
	}
	
	class Speed
	{
	private:
		__int64		startSize_;
		__int64		fullSize_;
		__int64		possition_;

		WinAPI::Stopwatch	startTime_;
		size_t		waitTime_; // in ms

		static const __int64 IncorrectFileSize = -1L;

	public:

		static const int percentRate = 10000;

		Speed()
		{
			clear();
		}

		Speed(__int64 size, __int64 startSize = 0)
		{
			start(size, startSize);
		}
		void	start(__int64 fullsize, __int64 startSize = 0)
		{
			startSize_	= possition_ = startSize;
			fullSize_	= fullsize;
			waitTime_	= 0;
			startTime_.reset();
		}

		__int64 getSize() const
		{
			BOOST_ASSERT(isInitialized());
			return fullSize_;
		}

		__int64	getPossition() const
		{
			BOOST_ASSERT(isInitialized());
			return possition_;
		}

		__int64 getRemain() const
		{
			BOOST_ASSERT(isInitialized());
			return fullSize_ - getPossition();
		}

		void	progress(__int64 n)
		{
			BOOST_ASSERT(isInitialized());
			possition_ += n;
		}

		void	wait(size_t n)
		{
			BOOST_ASSERT(isInitialized());
			waitTime_ += n;
		}

		size_t	getPercent() const
		{
			BOOST_ASSERT(isInitialized());
			BOOST_ASSERT(getPossition() <= getSize());
			return static_cast<size_t>(getPossition()*percentRate / getSize());
		}

		__int64	getFullTime() const
		{
			BOOST_ASSERT(isInitialized());
			return startTime_.getPeriod();
		}

		__int64	getWorkTime() const
		{
			BOOST_ASSERT(isInitialized());
			return startTime_.getPeriod() - waitTime_;
		}

		// returns speed bytes per sec
		__int64	getSpeed() const
		{
			BOOST_ASSERT(isInitialized());
			if(getWorkTime() == 0)
				return 0;
			return (possition_ - startSize_)*1000 / getWorkTime();
		}

		__int64	getDoRemain() const
		{
			BOOST_ASSERT(isInitialized());
			return fullSize_ - startSize_;
		}

		__int64 getStartPossition() const
		{
			BOOST_ASSERT(isInitialized());
			return startSize_;
		}

		bool isInitialized() const
		{
			return fullSize_ != IncorrectFileSize;
		}

		void clear()
		{
			fullSize_ = IncorrectFileSize;
		}
	};

private:
	static const int MAX_TRAF_LINES = 20;

	Show				show_;

	Speed				fileSpeed_;
	Speed				totalSpeed_;

	WinAPI::Stopwatch	lastTime_;
	__int64				lastSize_;

	__int64				totalFiles_;
	__int64				totalComplete_;
	__int64				totalSkipped_;

	boost::array<__int64, 3> averageCps_;

	int					titleMsg_;

	std::wstring		srcFilename_;
	std::wstring		destFilename_;

	__int64				getPossition() const
	{
		return totalSpeed_.getPossition() + fileSpeed_.getPossition();
	}

	__int64				getCps() const
	{
		if(totalSpeed_.getWorkTime() == 0)
			return 0;
		return (getPossition() - totalSpeed_.getStartPossition())*1000 / totalSpeed_.getWorkTime();
	}
	__int64				currentDoRemain();


	void				drawInfos(std::vector<std::wstring> &lines);
	void				formatLine(int num, std::vector<std::wstring> &lines, std::wstring::const_iterator begin, const std::wstring::const_iterator &end, std::vector<InfoItem>& items);
	void				drawInfo(InfoItem *it, std::vector<std::wstring>& lines, bool isReplace);
};
