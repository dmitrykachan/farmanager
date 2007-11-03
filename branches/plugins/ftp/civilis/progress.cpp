#include "stdafx.h"
#include "progress.h"

#include "ftp_JM.h"
#include "ftp_Connect.h"
#include "ftp_Int.h"


static const std::wstring InfinTime = L"--:--:--";
static const std::wstring InfinDate = L"------";

class StandartCommands
{
public:
	enum Command
	{
		SrcPathname, SrcPath, SrcName, DestPathname, DestPath, DestName, 
		CurSize, CurESize, CurFSize, CurRSize, 
		CurTime, CurFTime, CurRTime, CurETime, CurEDate,
		TotSize, TotESize, TotFSize, TotRSize, 
		TotTime, TotFTime, TotRTime, TotETime, TotEDate,
		CurCPS, CPS, TotCPS,
		Progress, FProgress, Percent, FPercent,
		Cn, CnSkip, CnR, CnF,
		CurPc, SkipPc, CurPg, SkipPg, Pc, FPc, Pg, FPg,
		CurCPS4, CPS4, TotCPS4,
		IPc, IFPc, Unknown, RightFiller
	};

private:
	struct pair
	{
		const wchar_t*	name_;
		Command			command_;
	};
	typedef boost::array<pair,  48> List;
	static const List list_;

	struct less
	{
		bool operator()(const pair &x, const pair &y) const
		{
			return wcsicmp(x.name_, y.name_) < 0;
		}
	};
public:
	StandartCommands()
	{
#ifdef _DEBUG
// check that list_ is sorted
		List::const_iterator itr = list_.begin();
		List::const_iterator next = itr+1;
		while(next != list_.end())
		{
			BOOST_ASSERT(wcsicmp(itr->name_, next->name_) < 0);
			++itr;
			++next;
		}
#endif		
	}

	Command find(const std::wstring& cmd)
	{
		return find(cmd.c_str());
	}

	Command find(const wchar_t* cmd)
	{
		pair p = {cmd, Unknown};
		List::const_iterator itr = std::lower_bound(list_.begin(), list_.end(), p, less());
		if(itr == list_.end() || wcsicmp(itr->name_, cmd) != 0)
			return Unknown;
		return itr->command_;
	}
};

const StandartCommands::List StandartCommands::list_ = {{
	L"Cn",			StandartCommands::Cn,			//Complete count
	L"CnF",			StandartCommands::CnF,			//Count
	L"CnR",			StandartCommands::CnR,			//Remain count
	L"CnSkip",		StandartCommands::CnSkip,		//Skipped count
	L"CPS",			StandartCommands::CPS,			//Summary CPS
	L"CPS4",		StandartCommands::CPS4,			//Summary CPS
	L"CurCPS",		StandartCommands::CurCPS,		//CP of current file
	L"CurCPS4",		StandartCommands::CurCPS4,		//CP of current file
	L"CurEDate",	StandartCommands::CurEDate,		//End date to process current file
	L"CurESize",	StandartCommands::CurESize,		//Full size of current file
	L"CurETime",	StandartCommands::CurETime,		//End time to process current file
	L"CurFSize",	StandartCommands::CurFSize,		
	L"CurFTime",	StandartCommands::CurFTime,		//Full time to process current file
	L"CurPc",		StandartCommands::CurPc,		//Current percent
	L"CurPg",		StandartCommands::CurPg,		//Current percent progress
	L"CurRSize",	StandartCommands::CurRSize,		//Remain size of current file
	L"CurRTime",	StandartCommands::CurRTime,		//Remain time to process current file
	L"CurSize",		StandartCommands::CurSize,		//Current processed size
	L"CurTime",		StandartCommands::CurTime,		//Time from start of current file
	L"DestName",	StandartCommands::DestName,		
	L"DestPath",	StandartCommands::DestPath,		
	L"DestPathname",StandartCommands::DestPathname,	//Target filename
	L"FPc",			StandartCommands::FPc,			//ALIAS `FPercent`
	L"FPercent",	StandartCommands::FPercent,		//Total "99.9%" percent
	L"FPg",			StandartCommands::FPg,			//ALIAS `FProgress`
	L"FProgress",	StandartCommands::FProgress,	//Total progress bar
	L"IFPc",		StandartCommands::IFPc,			//FullPercent without float part
	L"IPc",			StandartCommands::IPc,			//Percent without float part
	L"Pc",			StandartCommands::Pc,			//ALIAS `Percent`
	L"Percent",		StandartCommands::Percent,		//Current "99.9%" percent
	L"Pg",			StandartCommands::Pg,			//ALIAS `Progress`
	L"Progress",	StandartCommands::Progress,		//Current progress bar
	L"SkipPc",		StandartCommands::SkipPc,		//Skipped percent
	L"SkipPg",		StandartCommands::SkipPg,		//Skipped percent progress
	L"SrcName",		StandartCommands::SrcName,		
	L"SrcPath",		StandartCommands::SrcPath,		
	L"SrcPathname",	StandartCommands::SrcPathname,		
	L"TotCPS",		StandartCommands::TotCPS,		//Total CPS
	L"TotCPS4",		StandartCommands::TotCPS4,		//Total CPS
	L"TotEDate",	StandartCommands::TotEDate,		//End date to process current file
	L"TotESize",	StandartCommands::TotESize,		//Full size of current file
	L"TotETime",	StandartCommands::TotETime,		//End time to process current file
	L"TotFSize",	StandartCommands::TotFSize,		
	L"TotFTime",	StandartCommands::TotFTime,		//Full time to process current file
	L"TotRSize",	StandartCommands::TotRSize,		//Remain size of current file
	L"TotRTime",	StandartCommands::TotRTime,		//Remain time to process current file
	L"TotSize",		StandartCommands::TotSize,		//Current processed size
	L"TotTime",		StandartCommands::TotTime		//Time from start of current file
}};

static StandartCommands stdcommands;

class InfoItem
{
public:
	enum Alignment
	{
		Left,
		Right,
		Center,
		RightFill
	};

	StandartCommands::Command	type;   //Type of element (
	size_t		line;   //Line number (Y)
	size_t		pos;    //Starting position in line (X)
	size_t		size;   //Width of element (not for all alignment)
	Alignment	align;  //Element alignment (Alignment)
	wchar_t		fill;   //Filler character
};

static const std::wstring StdDialogLines[] =
{
	L"From: %*\xB7-57SrcPathname%",
	L"To:   %*\xB7-57DestPathname%",

	L"\x2500"L"Current %Cn%(%CurPc%)%>\x2500%",
	L"   %+14CurSize%  %CurTime%      %CurCPS%    %CPS%",
	L"   %+14CurFSize%  %CurRTime%         <%CurETime% %CurEDate%>",
	L"\x2500[%54Progress%]%+5Pc%\x2500",

	L"\x2500Total %CnF%/%Cn%/%CnSkip%%>\x2500%",
	L"   %+14TotSize%  %TotTime%                  %TotCps%",
	L"   %+14TotFSize%  %TotRTime%         <%TotETime% %TotEDate%>",
	L"\x2500[%54FProgress%]%+5FPercent%\x2500",
	};


static __int64 toPercent(__int64 N1, __int64 N2)
{
	if(N2 == 0)
		return 0;
	if(N2 < N1)
		return 100;

	return (N1*10000)/N2;
}

static std::wstring percentBar(size_t width, __int64 percent)
{
	std::wstring s;
	BOOST_ASSERT(width > 1);
	percent = std::min(10000LL, std::max(0LL, percent));
	size_t n = width*(static_cast<size_t>(percent)+50)/10000;
	s.insert(0, n, FAR_FULL_CHAR);
	s.insert(s.size(), width-n, FAR_SHADOW_CHAR);
	return s;
}

static std::wstring percent2Str(__int64 n1, __int64 n2)
{
	if(n2 == 0)
		return L"0.0%";
	__int64 percent10 = n1*1000/n2;
	std::wstringstream s;
	s << percent10/10 << L'.' << percent10 % 10 << L'%';
	return s.str();
}

static std::wstring percent2Str(__int64 n10000)
{
	std::wstringstream s;
	s << n10000/100 << L'.' << (n10000 % 100)/10 << L'%';
	return s.str();
}



static std::wstring percent2StrIntegral(__int64 n1, __int64 n2)
{
	if(n2 == 0)
		return L"0%";
	return boost::lexical_cast<std::wstring>(n1*100/n2) + L'%';
}

static std::wstring percent2StrIntegral(__int64 n)
{
	return boost::lexical_cast<std::wstring>(n/100) + L'%';
}

std::wstring fcps(__int64 v)
{
	wchar_t letter = L'b';
	double val = static_cast<double>(v);
	//1M
	if(val >= 1000000.)
	{
		letter = L'M';
		val   /= 1000000.;
	} else
		//1K
		if(val >= 1000.)
		{
			letter = L'K';
			val   /= 1000.;
		}

	std::wstringstream stm;
	stm << std::setw(7);
	if(letter == L'b')
		 stm << v;
	else
		stm << std::setprecision(3) << val;
	stm << letter;
	return stm.str();
}

std::wstring FCps4(__int64 val)
{
	wchar_t  letter = 0;

	//1M
	if(val >= 10000000)
	{
		letter = L'M';
		val /= 1000000;
	} else
	//1K
	if(val >= 10000)
	{
		letter = L'K';
		val /= 1000;
	}
	std::wstring s = boost::lexical_cast<std::wstring>(val);
	if(letter)
		s += letter;
	return s;
}


FTPProgress::FTPProgress()
	: connection_(0)
{
}


FTPProgress::~FTPProgress()
{}

void FTPProgress::Init(int tMsg, int OpMode, const FileList &filelist)
{
	lastSize_ = totalComplete_ = totalSkipped_ = 0;
	averageCps_[0] = averageCps_[1] = averageCps_[2] = 0;

	titleMsg_		= tMsg;

	showStatus_ = true;

	if(is_flag(OpMode, OPM_FIND))
		showStatus_ = false;
	else
		if(is_flag(OpMode, OPM_VIEW) || is_flag(OpMode, OPM_EDIT))
			showStatus_ = g_manager.opt.ShowSilentProgress;

	__int64 totalFullSize = 0;
	totalFiles_		= 0;
	FileList::const_iterator itr = filelist.begin();
	while(itr != filelist.end())
	{
		if((*itr)->getType() != FTPFileInfo::directory)
		{
			totalFullSize += (*itr)->getFileSize();
			++totalFiles_;
		}
		++itr;
	}

	totalSpeed_.start(totalFullSize);
}

bool FTPProgress::refresh(unsigned __int64 size)
{
	std::wstring	str;

	fileSpeed_.progress(size);

	BOOST_LOG(ERR, L"refresh size: " << size << L" downloaded: " << fileSpeed_.getPossition() << L" of " << fileSpeed_.getSize()
		<< L"\nTotal pos: " << totalSpeed_.getPossition() << L" pos2: " << getPossition() << L"size: " << totalSpeed_.getSize());

	if(size == 0)
	{
		++totalComplete_;
		BOOST_ASSERT(totalFiles_ >= totalSkipped_ + totalComplete_);
		totalSpeed_.progress(fileSpeed_.getSize());
		return true;
	}

	if(lastTime_.getPeriod() < static_cast<size_t>(g_manager.opt.IdleShowPeriod))
		return true;	

	//Avg
	if(lastTime_.getPeriod() > 0)
	{
 		averageCps_[0] = averageCps_[1];
		averageCps_[1] = averageCps_[2];
		averageCps_[2] = 1000*(fileSpeed_.getPossition() - lastSize_) / lastTime_.getPeriod();
	}
	lastTime_.reset();


	lastSize_ = fileSpeed_.getPossition();

	//Total

	__int64 totalPercent = toPercent(getPossition(), totalSpeed_.getSize());

	//Show QUIET progressing
	if(showStatus_ == false)
	{
		std::wstringstream stm;
		stm << L'{' << totalPercent/100 << L'.' << totalPercent % 100 << L"%} " 
			<< getMsg(titleMsg_) << L": " << getName(srcFilename_);
		
		IdleMessage(stm.str().c_str(), g_manager.opt.ProcessColor);
		return true;
	}

	//Window caption
	std::wstringstream stm;
	if (connection_->getRetryCount())
		stm << connection_->getRetryCount() << L": ";
	stm << L'{' << totalPercent/100 << L'.' << totalPercent % 100 << L"%} " 
		<< getMsg(titleMsg_) << L" - Far";

	WinAPI::setConsoleTitle(stm.str());

	//Mark CMD window invisible
	FtpCmdBlock(connection_, true);


	std::vector<std::wstring> lines;
	//Show message

	drawInfos(lines);

	std::vector<std::wstring> msgItems;
	msgItems.resize(lines.size()+1);
	if(connection_->getRetryCount())
		msgItems[0] += boost::lexical_cast<std::wstring>(connection_->getRetryCount()) + L": ";
	msgItems[0] += getMsg(titleMsg_);

	for(size_t n = 0; n < lines.size(); n++ )
		msgItems[n+1] = lines[n];

	FARWrappers::message(msgItems, 0, FMSG_LEFTALIGN, NULL);

	return true;
}

void FTPProgress::drawInfos(std::vector<std::wstring> &lines)
{
	std::wstring str;
	std::vector<InfoItem> items;

	WinAPI::RegKey rkey(g_manager.getRegKey(), (std::wstring(L"CopyDialog\\") + getMsg(MLanguage)).c_str());
	lines.clear();

	int linecount	= std::min(rkey.get(L"Count", 0), MAX_TRAF_LINES);
	if(linecount == 0)
	{
		lines.resize(ARRAY_SIZE(StdDialogLines));
		for(int i = 0; i < ARRAY_SIZE(StdDialogLines); ++i)
			formatLine(i, lines, StdDialogLines[i].begin(), StdDialogLines[i].end(), items);
	} else
	{
		lines.resize(linecount);
		for(int i = 0; i < linecount; ++i)
		{
			std::wstringstream stm;
			stm << L"Line" << std::setw(2) << std::setfill(L'0') << i+1;
			std::wstring s = rkey.get(stm.str().c_str(), L"");
			formatLine(i, lines, str.begin(), str.end(), items);
		}
	}

	if(lines.size() == 0)
		return;

	size_t w = std::max_element(lines.begin(), lines.end(), 
				boost::bind(&std::wstring::size, _1) < boost::bind(&std::wstring::size, _2))->size();


	std::vector<InfoItem>::const_iterator itr = items.begin();
	while(itr != items.end())
	{
		if(itr->type == StandartCommands::RightFiller && lines[itr->line].size() < w)
		{
			lines[itr->line].insert(itr->pos+1, w - lines[itr->line].size(), itr->fill);
			// TODO itr->size = 1+ w - lines[itr->line].size(); OFS для всех остальных
		}
		++itr;
	}

	for (size_t n = 0; n < lines.size(); n++ )
		if(!lines[n].empty() && first(lines[n]) != L'\x1' && first(lines[n]) != L'\x2' )
		{
			lines[n].resize(w, L' ');
		}

}

void FTPProgress::formatLine(int num, std::vector<std::wstring> &lines, std::wstring::const_iterator begin, const std::wstring::const_iterator &end, std::vector<InfoItem>& items)
{
	using namespace static_regex;

	enum
	{
		fRightFiller, fAlign, fWidth, fName, fFiller, fCharacter, fsize
	};

	typedef plus<digit> number;
	typedef seq<alpha, star<alphanum> > name;

	typedef mark<fAlign, cs<c<L'+'>, c<L'-'>, c<L'.'> > > align;
	typedef seq<c<'*'>, mark<fFiller, dot> > fff;

	typedef seq<c<L'>'>, mark <fRightFiller, dot> > filler;
	typedef seq<c<L'\\'>, mark<fCharacter, number> > character;
	typedef seq<opt<fff>, opt<align>, opt<mark<fWidth, number> >, mark<fName, name> > symbol;


	typedef seq<alt<filler, character, symbol>, c<L'%'> > format;


	while(begin != end)
	{
		wchar_t wc = *begin;
		++begin;
		if(wc != L'%')
		{
			lines[num].push_back(wc);
			continue;
		}

		if(*begin == L'%')
		{
			lines[num].push_back(L'%');
			++begin;
			continue;
		}

		typedef match_array<std::wstring::const_iterator, fsize> MC;
		MC mc;
		if(match<format>(begin, end, mc))
		{
			InfoItem it;
			it.type		= StandartCommands::Unknown;
			it.line		= num;
			it.pos		= lines[num].size();
			it.align	= InfoItem::Left;
			it.size		= 0;
			it.fill		= L' ';

			switch(mc[0].id())
			{
			case fCharacter:
				BOOST_ASSERT(mc.size() == 1);
				lines[num].push_back(boost::lexical_cast<int>(std::wstring(mc[0].begin(), mc[0].end())));
				break;
			case fRightFiller:
				BOOST_ASSERT(mc.size() == 1 && mc[0].length() == 1);
				it.fill = *mc[0].begin();
				it.align= InfoItem::RightFill;
				it.type = StandartCommands::RightFiller;
				it.size = 1;
				break;
			default:
				for(MC::iterator i=mc.begin(); i!=mc.end(); ++i)
				{
					switch(i->id())
					{
					case fAlign:
						BOOST_ASSERT(i->length() == 1);
						switch(*i->begin())
						{
						case L'+':
							it.align = InfoItem::Right;
							break;
						case L'.':
							it.align = InfoItem::Center;
							break;
						default:
							BOOST_ASSERT(*i->begin() == L'-');
						}
						break;
					case fFiller:
						BOOST_ASSERT(i->length() == 1);
						it.fill = *i->begin();
						break;
					case fWidth:
						it.size = boost::lexical_cast<int>(std::wstring(i->begin(), i->end()));
						break;
					case fName:
						it.type = stdcommands.find(std::wstring(i->begin(), i->end()));
						if(it.type == StandartCommands::Unknown)
						{
							lines[num] += L"<badCmd>";
							return;
						}
						break;
					default:
						BOOST_ASSERT(0 && "invalid case");
						break;
					}
				}
			}
			//Draw
			drawInfo(&it, lines, false);
			items.push_back(it);
		} else
		{
			lines[num] += L"<badcmd>";
			return;
		}
	}
}


static std::wstring FDigit(__int64 val)
{
	// TODO
	return boost::lexical_cast<std::wstring>(val);
}

static std::wstring FTime(__int64 secs)
{
	std::wstringstream stm;
	stm << std::setw(2) << std::setfill(L'0') << secs/3600 << L':'
		<< std::setw(2) << (secs/60)%60 << L':' << std::setw(2) << secs%60;
	return stm.str();
}

static std::wstring FTime(struct tm *tm)
{
	BOOST_ASSERT(tm != NULL);
	std::wstringstream stm;
	stm << std::setw(2) << std::setfill(L'0') << tm->tm_hour << L':'
		<< std::setw(2) << tm->tm_min << L':' << std::setw(2) << tm->tm_sec;
	return stm.str();
}

static std::wstring ETime(__int64 sec)
{
	time_t t;
	time(&t);
	t += sec;
	return FTime(localtime(&t));
}

static std::wstring FDate(const time_t &tm)
{
	BOOST_ASSERT(tm != NULL);

	struct tm ltm;
	std::wstringstream stm;
	if(localtime_s(&ltm, &tm) == 0)
	{
		stm << std::setw(2) << std::setfill(L'0') << ltm.tm_mday << L'-' << WinAPI::getMonthAbbreviature(ltm.tm_mon);
		return stm.str();
	}
	else
		return InfinTime;
}

static std::wstring EDate(__int64 sec)
{
	time_t t;
	time(&t);
	t += sec;
	return FDate(t);
}


void FTPProgress::drawInfo(InfoItem *it, std::vector<std::wstring>& lines, bool isReplace)
{
	__int64    cn;

	std::wstring str;
	std::wstringstream stm;

	switch(it->type)
	{
		case StandartCommands::SrcPathname:
			str = srcFilename_;
			break;
		case StandartCommands::SrcPath:
			str = getPathBranch(srcFilename_);
			break;
		case StandartCommands::SrcName:
			str = getName(srcFilename_);
			break;
		case StandartCommands::DestPathname:
			str = destFilename_;
			break;
		case StandartCommands::DestPath:
			str = getPathBranch(destFilename_);
			break;
		case StandartCommands::DestName:
			str = getName(destFilename_);
			break;
		case StandartCommands::CurSize:
			str = FDigit(fileSpeed_.getPossition());
			break;
		case  StandartCommands::CurESize:
		case  StandartCommands::CurFSize:
			str = FDigit(fileSpeed_.getSize());
			break;
		case StandartCommands::CurRSize:
			str = FDigit(fileSpeed_.getRemain());
			break;
		case StandartCommands::CurTime:
			str = FTime(fileSpeed_.getFullTime()/1000);
			break;
		case StandartCommands::CurFTime:
			cn = 0;
			if(fileSpeed_.getSpeed() > 0)
				cn = fileSpeed_.getDoRemain() / fileSpeed_.getSpeed();
			str = FTime(cn);
			break;
		case StandartCommands::CurRTime:
			cn = 0;
			if(fileSpeed_.getSpeed() > 0)
				cn = fileSpeed_.getRemain() / fileSpeed_.getSpeed();
			str = FTime(cn);
			break;
		case StandartCommands::CurETime:
			if(fileSpeed_.getSpeed() > 0)
				str = ETime(fileSpeed_.getRemain() / fileSpeed_.getSpeed());
			else
				str = InfinTime;
			break;
		case StandartCommands::CurEDate:
			if(fileSpeed_.getSpeed() > 0)
				str = EDate(fileSpeed_.getRemain() / fileSpeed_.getSpeed());
			else
				str = InfinDate;
			break;
			break;
		case StandartCommands::TotSize:
			str = FDigit(getPossition());
			break;
		case StandartCommands::TotESize:
		case StandartCommands::TotFSize:
			str = FDigit(totalSpeed_.getSize());
			break;
		case StandartCommands::TotRSize:
			str = FDigit(totalSpeed_.getSize() - getPossition());
			break;
		case StandartCommands::TotTime:
			str = FTime(totalSpeed_.getFullTime()/1000);
			break;

		case StandartCommands::TotFTime:
			cn = (averageCps_[0] + averageCps_[1] + averageCps_[2])/3;
			if(cn > 0)
				str = FTime((totalSpeed_.getSize()-getPossition()) / cn);
			else
				str = InfinTime;
			break;

		case StandartCommands::TotRTime:
			cn = (averageCps_[0] + averageCps_[1] + averageCps_[2])/3;
			if(cn > 0)
				str = ETime((totalSpeed_.getSize()-getPossition()) / cn);
			else
				str = InfinTime;
			break;

		case StandartCommands::TotETime:
			cn = getCps();
			if (cn > 0)
				str = ETime((totalSpeed_.getSize()-getPossition()) / cn);
			else
				str = InfinTime;
			break;
		case StandartCommands::TotEDate:
			cn = getCps();
			if(cn > 0)
				str = EDate((totalSpeed_.getSize()-getPossition()) / cn);
			else
				str = InfinDate;
			break;
		case StandartCommands::CurCPS:
			str = fcps(static_cast<int>(fileSpeed_.getSpeed()));
			break;
		case StandartCommands::CPS:
			str = fcps((averageCps_[0] + averageCps_[1] + averageCps_[2])/3);
			break;
		case StandartCommands::TotCPS:
			str = fcps(getCps());
			break;
		case StandartCommands::Progress:
		case StandartCommands::Pg:
			str = percentBar(it->size, fileSpeed_.getPercent());
			break;
		case StandartCommands::FProgress:
		case StandartCommands::FPg:
			str = percentBar(it->size, toPercent(getPossition(), totalSpeed_.getSize()));
			break;
		case StandartCommands::Pc:
		case StandartCommands::Percent: 
			str = percent2Str(fileSpeed_.getPercent());
			break;
		case StandartCommands::FPc:
		case StandartCommands::FPercent:
			str = percent2Str(getPossition(), totalSpeed_.getSize());
			break;
		case StandartCommands::Cn:
			str = FDigit(totalComplete_);
			break;
		case StandartCommands::CnSkip:
			str = FDigit(totalSkipped_);
			break;
		case StandartCommands::CnR:
			str = FDigit(totalFiles_ - totalComplete_ - totalSkipped_);
			break;
		case StandartCommands::CnF:
			str = FDigit(totalFiles_);
			break;
		case StandartCommands::CurPc:
			str = percent2Str(totalComplete_, totalFiles_);
			break;
		case StandartCommands::SkipPc:
			str = percent2Str(totalSkipped_, totalFiles_);
			break;
		case StandartCommands::CurPg:
			str = percentBar(it->size, (int)toPercent(totalComplete_, totalFiles_));
			break;
		case StandartCommands::SkipPg:
			str = percentBar(it->size, (int)toPercent(totalSkipped_, totalFiles_));
			break;
		case StandartCommands::CurCPS4:
			str = FCps4(static_cast<int>(fileSpeed_.getSpeed()));
			break;
		case StandartCommands::CPS4:
			str = FCps4(static_cast<int>(averageCps_[0] + averageCps_[1] + averageCps_[2])/3);
			break;
		case StandartCommands::TotCPS4:
			str = FCps4(getCps());
			break;
		case StandartCommands::IPc:
			str = percent2StrIntegral(fileSpeed_.getPercent());
			break;
		case StandartCommands::IFPc:
			str = percent2StrIntegral(getPossition(), totalSpeed_.getSize());
			break;
		case StandartCommands::RightFiller:
			str = it->fill;
			break;
		default:
			BOOST_ASSERT(0 && "Invalid command");
	}

	if(it->type != StandartCommands::Unknown)
	{
		InfoItem::Alignment align = it->align;

		if(!it->size)
			it->size = str.size();

		if(str.size() > it->size && align == InfoItem::Center)
			align = InfoItem::Left;


		if(str.size() > it->size)
		{
			if(it->align == InfoItem::Left)
				str.resize(it->size);
			else
				str.erase(0, str.size()-it->size);
		} else
			if(str.size() < it->size)
			{
				size_t excess = it->size-str.size();
				switch(it->align)
				{
				case InfoItem::Center:
					str.insert(0, excess/2, it->fill);
					str.resize(it->size, it->fill);
					break;
				case InfoItem::Right:
					str.insert(0, excess, it->fill);
				    break;
				case InfoItem::Left:
					str.resize(it->size, it->fill);
				default:
				    break;
				}
			}
		BOOST_ASSERT(str.size() == it->size);
		
		if(isReplace)
			lines[it->line].replace(it->pos, it->size, str);
		else
			lines[it->line] += str;

	}
}

void FTPProgress::resume(__int64 size)
{
	fileSpeed_.start(fileSpeed_.getSize(), size);
}

void FTPProgress::Waiting(size_t paused)
{
	fileSpeed_.wait(paused);
	totalSpeed_.wait(paused);
}

void FTPProgress::initFile(FTPFileInfo& info, const std::wstring &destName)
{
	BOOST_LOG(ERR, L"file: [" << info.getFileName() << L"], size: " << info.getFileSize());

	fileSpeed_.start(info.getFileSize());
	lastSize_			= 0;

	averageCps_[0] = 0; averageCps_[1] = 0; averageCps_[2] = 0;
	lastTime_.reset();

	srcFilename_		= info.getFileName();
	destFilename_		= destName;
}

void FTPProgress::skip()
{
	totalSpeed_.progress(fileSpeed_.getSize());
	++totalSkipped_;
	BOOST_ASSERT(totalFiles_ < totalSkipped_ + totalComplete_);

	fileSpeed_.clear();
}

void FTPProgress::setConnection(Connection* connection)
{
	connection_ = connection;
}

#ifdef CONFIG_TEST

void progress_test_()
{
	FTPProgress progress;
	Connection con;

	FileList list;

	FTPFileInfoPtr info(new FTPFileInfo);

	info->setFileName(L"file1.txt");
	info->setFileSize(128);
	list.push_back(info);

	progress.setConnection(&con);
	progress.Init(MStatusDownload, 0, list);

	progress.initFile(*info, L"destnnn");
	progress.refresh(100);
}

BOOST_AUTO_TEST_CASE(progress_test)
{
	progress_test_();
}

#endif

