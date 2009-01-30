//  Boost general library logging.hpp header file  ---------------------------//

//  (C) Copyright Jean-Daniel Michaud 2007. Permission to copy, use, modify, 
//  sell and distribute this software is granted provided this copyright notice 
//  appears in all copies. This software is provided "as is" without express or 
//  implied warranty, and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for updates, documentation, and revision history.
//  See http://www.boost.org/libs/logging/ for library home page.

#ifndef BOOST_LOGGING_HPP
#define BOOST_LOGGING_HPP

#include <stdio.h>
#include <string>
#include <ostream>
#include <sstream>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#if defined(BOOST_HAS_THREADS)
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#endif // BOOST_HAS_THREADS
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)

#define BOOST_LOG_INIT(format, max_log_level)                                  \
{                                                                              \
  boost::logging::logger *l = boost::logging::logger::get_instance();          \
  l->set_format(format);                                                       \
  l->set_max_log_level(max_log_level);                                         \
}

#define BOOST_LOG_ADD_OUTPUT_STREAM(stream)                                    \
{                                                                              \
  boost::logging::logger *l = boost::logging::logger::get_instance();          \
  l->add_output_streams(stream);                                               \
}

#define BOOST_LOG(level, _trace)                                               \
{                                                                              \
  boost::logging::logger *l = boost::logging::logger::get_instance();          \
  assert(l);                                                                   \
  if (l->get_max_log_level() >= level)                                         \
  {                                                                            \
    std::iostream::ios_base::fmtflags flags = l->m_string_stream.flags();      \
    l->m_string_stream.str(L"");                                               \
    l->m_string_stream << _trace;                                              \
	l->m_string_stream.flags(flags);                                           \
    l->trace(level, l->m_string_stream.str(), __WFILE__, __LINE__);            \
  }                                                                            \
}

#define BOOST_LOG_UNFORMATTED(level, _trace)                                   \
{                                                                              \
  boost::logging::logger *l = boost::logging::logger::get_instance();          \
  assert(l);                                                                   \
  if (l->get_max_log_level() >= level)                                         \
  {                                                                            \
    std::iostream::ios_base::fmtflags flags = l->m_string_stream.flags();      \
    l->m_string_stream.str(L"");                                               \
    l->m_string_stream << _trace;                                              \
    l->m_string_stream.flags(flags);                                           \
    l->unformatted_trace(level, l->m_string_stream.str(), __WFILE__, __LINE__); \
  }                                                                            \
}

#define BOOST_MAX_LINE_STR_SIZE 20 // log(2^64)
#define BOOST_LEVEL_UP_LIMIT    999

namespace boost {

  namespace logging {

//  Logging forward declarations ---------------------------------------------//
    class log_element;
    class level_element;
    class trace_element;
    class logger;
    
//  Logging typedefs declarations --------------------------------------------//
    typedef enum { LEVEL = 0, TRACE, FILENAME, LINE }   param_e;
    typedef std::list<boost::shared_ptr<log_element> >  element_list_t;
    typedef std::list<boost::shared_ptr<std::wostream> > stream_list_t;
    typedef unsigned short                              level_t;
    typedef tuple<level_t, std::wstring, std::wstring, unsigned int> log_param_t;

//  Used for shared_ptr() on statically allocated log_element ----------------//
    struct null_deleter
    { void operator()(void const *) const {} };

//  Logging classes declaration  ---------------------------------------------//
    class log_element
    {
    public:
      virtual std::wstring to_string() { assert(0); return L""; };

      virtual std::wstring visit(logger &l, const log_param_t &log_param);
    };
    
    class level_element : public log_element
    {
    public:
      std::wstring to_string(level_t l) 
      { 
        wchar_t  buffer[3];
        _snwprintf(buffer, 3, L"%i", l);
        return buffer; 
      };

      std::wstring visit(logger &l, const log_param_t &log_param);
    };
    
    class filename_element : public log_element
    {
    public:
      std::wstring to_string(const std::wstring &f) { return f; }
      std::wstring visit(logger &l, const log_param_t &log_param);
    };
    
    class line_element : public log_element
    {
    public:
      std::wstring to_string(unsigned int l) 
      {
        wchar_t  buffer[BOOST_MAX_LINE_STR_SIZE];
        _snwprintf(buffer, BOOST_MAX_LINE_STR_SIZE, L"%i", l);
        return buffer; 
      }
      std::wstring visit(logger &l, const log_param_t &log_param);
    };
    
    class date_element : public log_element
    {
    public:
      std::wstring to_string()
      {
        boost::gregorian::date d(boost::gregorian::day_clock::local_day());
        return boost::gregorian::to_iso_extended_string_type<wchar_t>(d);
      }
    };
    
    class time_element : public log_element
    {
    public:
      std::wstring to_string() 
      { 
        boost::posix_time::ptime 
          t(boost::posix_time::microsec_clock::local_time());
        return boost::posix_time::to_simple_string_type<wchar_t>(t); 
      };
    };
    
    class trace_element : public log_element
    {
    public:
      std::wstring to_string(const std::wstring& s) { return s; };

      std::wstring visit(logger &l, const log_param_t &log_param);
    };

    class eol_element : public log_element
    {
    public:
      std::wstring to_string() { return L"\n"; };
    };

    class literal_element : public log_element
    {
    public:
      literal_element(const std::wstring &l) : m_literal(l) {}
      std::wstring to_string() { return m_literal; };
    private:
      std::wstring m_literal;
    };

//  Logger class declaration  ------------------------------------------------//
    class logger
    {
      public: 
        logger(element_list_t &e, level_t max_log_level = 1) 
           : m_element_list(e), m_max_log_level(max_log_level) {}

        void set_format(element_list_t &e) 
        { 
          m_element_list.insert(m_element_list.begin(), e.begin(), e.end());
        }

        void set_max_log_level(level_t max_log_level)
        { 
          m_max_log_level = ((BOOST_LEVEL_UP_LIMIT < max_log_level) 
            ? BOOST_LEVEL_UP_LIMIT : max_log_level);
        }

        inline level_t get_max_log_level()
        {
          return m_max_log_level;
        }
        
        void set_output_streams(stream_list_t &s) 
        { 
          m_stream_list.insert(m_stream_list.begin(), s.begin(), s.end()); 
        }

		    void add_output_streams(std::wostream *s) 
        { 
          if (s)
            if (*s == std::wcout || *s == std::wcerr || *s == std::wclog)
              m_stream_list.push_back
              (
              boost::shared_ptr<std::wostream>(s, null_deleter())
              );
            else
              m_stream_list.push_back
              (
              boost::shared_ptr<std::wostream>(s)
              );
        }

        static logger *get_instance()
        {
#if defined(BOOST_HAS_THREADS)
          static boost::mutex m_inst_mutex;
          boost::mutex::scoped_lock scoped_lock(m_inst_mutex);
#endif // BOOST_HAS_THREADS
          static logger             *l = NULL;
          
          if (!l)
          {
            l = new logger();
            static shared_ptr<logger> s_ptr_l(l);
          }
          
          return l;
        }
        
        // Visitors for the log elements
        std::wstring accept(log_element &e)
        {
          return e.to_string();
        }
        std::wstring accept(level_element &e, level_t l)
        {
          return e.to_string(l);
        }
        std::wstring accept(trace_element &e, const std::wstring& s)
        {
          return e.to_string(s);
        }
        std::wstring accept(filename_element &e, const std::wstring& s)
        {
          return e.to_string(s);
        }
        std::wstring accept(line_element &e, unsigned int l)
        {
          return e.to_string(l);
        }
      
        void trace(unsigned short     l, 
                   const std::wstring &t, 
                   const std::wstring &f, 
                   unsigned int      ln)
        {
#if defined(BOOST_HAS_THREADS)
          boost::mutex::scoped_lock scoped_lock(m_mutex);
#endif // BOOST_HAS_THREADS
          if (l > m_max_log_level)
             return;
    
          log_param_t log_param(l, t, f, ln);
          
          element_list_t::iterator e_it = m_element_list.begin();
          std::wstringstream str_stream;
          for (; e_it != m_element_list.end(); ++e_it)
          {
            str_stream << (*e_it)->visit(*this, log_param);
          }
    
          stream_list_t::iterator s_it = m_stream_list.begin();
          for (; s_it != m_stream_list.end(); ++s_it)
          {
			  **s_it << str_stream.str();
			  (*s_it)->flush();
          }
        }

        void unformatted_trace(unsigned short     l, 
                               const std::wstring &t, 
                               const std::wstring &/*f*/, 
                               unsigned int      /*ln*/)
        {
#if defined(BOOST_HAS_THREADS)
          boost::mutex::scoped_lock scoped_lock(m_mutex);
#endif // BOOST_HAS_THREADS
          if (l > m_max_log_level)
             return;
    
          stream_list_t::iterator s_it = m_stream_list.begin();
          for (; s_it != m_stream_list.end(); ++s_it)
          {
            **s_it << t;
          }
        }

      public:
        std::wstringstream m_string_stream;

      private:
        logger() : m_max_log_level(0) {}

      private:
        element_list_t    m_element_list;
        stream_list_t     m_stream_list;
        level_t           m_max_log_level;
#if defined(BOOST_HAS_THREADS)
      	boost::mutex      m_mutex;
#endif // BOOST_HAS_THREADS
    };  // logger

//  Element static instantiations --------------------------------------------//
    static level_element     level     = level_element();
    static filename_element  filename  = filename_element();
    static line_element      line      = line_element();
    static date_element      date      = date_element();
    static time_element      time      = time_element();
    static trace_element     trace     = trace_element();
    static eol_element       eol       = eol_element();
    
//  Element functions definition ---------------------------------------------//
    inline std::wstring log_element::visit(logger &l, 
                                          const log_param_t &/*log_param*/)
    {
      return l.accept(*this);
    }

    inline std::wstring level_element::visit(logger &l, 
                                            const log_param_t &log_param)
    {
      return l.accept(*this, get<LEVEL>(log_param));
    }

    inline std::wstring trace_element::visit(logger &l, 
                                            const log_param_t &log_param)
    {
      return l.accept(*this, get<TRACE>(log_param));
    }

    inline std::wstring filename_element::visit(logger &l, 
                                               const log_param_t &log_param)
    {
      return l.accept(*this, get<FILENAME>(log_param));
    }

    inline std::wstring line_element::visit(logger &l, 
                                           const log_param_t &log_param)
    {
      return l.accept(*this, get<LINE>(log_param));
    }


	template<class E, class T=std::char_traits<E> >
	class basic_debugbuf : public std::basic_streambuf<E,T>
	{
	public:
		basic_debugbuf()
		{
			buff_.reserve(buffer_size);
		}
	private:
		std::wstring buff_;
		const static size_t buffer_size = 512;
		virtual std::streamsize xsputn(char_type const* s, std::streamsize n)
		{
			dprints(s);
			return n;
		}
		virtual int_type overflow(int_type c = traits_type::eof())
		{
			if(c != traits_type::eof())
				dprintc(static_cast<E>(c));
			return c;
		}
		void dprints(const char* s)
		{
			OutputDebugStringA(s);
		}

		void dprints(const wchar_t* s)
		{
			if(!buff_.empty())
			{
				basic_debugbuf::sync();
			}
			OutputDebugStringW(s);
		}

		void dprintc(wchar_t c)
		{
			buff_ += c;
			if(buff_.size() >= buffer_size)
			{
				basic_debugbuf::sync();
			}
		}

		virtual int sync()
		{
			OutputDebugStringW(buff_.c_str());
			buff_.resize(0);
			return 0;
		}
	};

	template<class E, class T=std::char_traits<E> >
	class basic_debugstream : public std::basic_ostream<E,T>
	{
	public:
		basic_debugstream() : std::basic_ostream<E,T>(new basic_debugbuf<E, T>())
		{
		}
		~basic_debugstream()
		{
			delete rdbuf();
		}
	};

	typedef basic_debugstream<char> debugstream;
	typedef basic_debugstream<wchar_t> wdebugstream;
  } // !namespace logging

} // !namespace boost

//  Element global operators -------------------------------------------------//
inline boost::logging::element_list_t operator>>(
  boost::logging::log_element &lhs, 
  boost::logging::log_element &rhs)
{ 
  boost::logging::element_list_t l;
  l.push_back(boost::shared_ptr<boost::logging::log_element> 
                (&lhs, boost::logging::null_deleter()));
  l.push_back(boost::shared_ptr<boost::logging::log_element> 
                (&rhs, boost::logging::null_deleter())); 
  return l;
}

inline boost::logging::element_list_t &operator>>(
  boost::logging::element_list_t &lhs, 
  boost::logging::log_element &rhs)
{ 
  lhs.push_back(boost::shared_ptr<boost::logging::log_element> 
                (&rhs, boost::logging::null_deleter())); 
  return lhs; 
}

inline boost::logging::element_list_t operator>>(
  std::wstring s, 
  boost::logging::log_element &rhs)
{
  boost::logging::element_list_t l;
  boost::shared_ptr<boost::logging::literal_element> 
    p(new boost::logging::literal_element(s));
  l.push_back(p);
  l.push_back(boost::shared_ptr<boost::logging::log_element> 
                (&rhs, boost::logging::null_deleter())); 
  return l;
}

inline boost::logging::element_list_t &operator>>(
  boost::logging::element_list_t &lhs, 
  std::wstring s)
{ 
  boost::shared_ptr<boost::logging::literal_element> 
    p(new boost::logging::literal_element(s));
  lhs.push_back(p);
  return lhs;
}

#endif  // !BOOST_LOGGING_HPP
