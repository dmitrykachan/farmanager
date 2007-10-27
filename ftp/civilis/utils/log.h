#ifndef __LOG_INCLUDE_FILE
#define __LOG_INCLUDE_FILE

#include "utils/logging.hpp"

enum
{
	IMPTDATA, FTL, ERR, WRN, INF, DBG, RAW_DATA, FUNCTRACE, FUNCTRACEPARAM
};


#define PROC   FunctionTracer functionTracer__(WIDEN(__FUNCSIG__)); functionTracer__.printHeader();

#define PROCP(params)	FunctionTracer functionTracer__(WIDEN(__FUNCSIG__)); \
	if(boost::logging::logger::get_instance()->get_max_log_level() >= FUNCTRACEPARAM)				\
		functionTracer__.params_ << params;		\
	functionTracer__.printHeader();

class FunctionTracer
{
	static int		counter_;
	const wchar_t*	name_;
	enum { indent = 2 };
public:
	FunctionTracer(const wchar_t* name)
		: name_(name)
	{
		++counter_;
	}

	~FunctionTracer()
	{
		--counter_;
		BOOST_LOG(FUNCTRACE, std::wstring(counter_ * indent, L' ') << L"}<" << name_ << L">");
	}

	void printHeader()
	{
		params_ << std::ends;
		BOOST_LOG(FUNCTRACE, std::wstring((counter_-1) * indent, L' ') 
			<< name_ << L"(" << params_.str() << L")\t{");
		params_.str(L"");
	}

	static std::wstringstream params_;
};

#endif