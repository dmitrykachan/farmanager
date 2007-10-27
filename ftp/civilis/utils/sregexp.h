#ifndef __STATIC_REGEX_H_INCLUDED__
#define    __STATIC_REGEX_H_INCLUDED__

namespace static_regex {

typedef wchar_t rx_char;
typedef unsigned int rx_int;
struct _Sqe {};
enum { _Stc = 0xBAADC0DE };

//-////////////////////////////////////////////////////////////////////////

/**\ingroup grpSRXM
Single match.
\sa mark - capture a %submatch*/
template<class IT>
class submatch {
    rx_int id_; IT begin_; IT end_;
public:
    submatch() {}
    submatch(rx_int id, const IT& begin, const IT& end)
        :id_(id),begin_(begin),end_(end) {}
    template<class IT1> submatch(const submatch<IT1>& rhs)
        :id_(rhs.id()),begin_(rhs.begin()),end_(rhs.end()) {}
    ///get match identifier
    rx_int id() const { return id_; }
    ///get match beginning
    const IT& begin() const { return begin_; }
    ///get match ending
    const IT& end() const { return end_; }
    ///match length
    std::size_t length() const { return end_ - begin_; }
};

template<class IT, std::size_t N> class match_array;

template<class IT, std::size_t N>
void set_match_count(match_array<IT,N>& m, std::size_t cnt)
    { if(cnt > N) cnt = N; m.nmatch_ = cnt; }

template<class IT, std::size_t N>
submatch<IT>& get_match_at(match_array<IT,N>& m, std::size_t pos)
    { return m.match_[pos]; }

/**\ingroup grpSRXM
Submatch container based on static array.
\param IT iterator type
\param N  container capcity*/
template<class IT, std::size_t N=31>
class match_array {
    match_array(const match_array&);
    match_array& operator=(const match_array&);
public:
    typedef submatch<IT> match_type;
    typedef const match_type* iterator;
public:
    match_array()
        :nmatch_(0) {}
    explicit match_array(std::size_t)
        :nmatch_(0) {}
public:
    std::size_t size() const
        { return nmatch_; }
    match_array& reset()
        { nmatch_ = 0; return *this; }
    const match_type& operator[](std::size_t pos) const
        { return match_[pos]; }
    iterator begin() const { return match_; }
    iterator end() const { return match_ + nmatch_; }
private:
    match_type match_[N+1];
    std::size_t nmatch_;

#if defined(_MSC_VER) && _MSC_VER < 1300 //MSVC 6.0
    friend void set_match_count(match_array& m, std::size_t cnt);
    friend submatch<IT>& get_match_at(match_array& m, std::size_t pos);
#else
    friend void set_match_count<>(match_array& m, std::size_t cnt);
    friend submatch<IT>& get_match_at<>(match_array& m, std::size_t pos);
#endif//MSVC 6.0
};

template<class IT, class A> class match_vector;

template<class IT, class A>
void set_match_count(match_vector<IT,A>& m, std::size_t cnt)
    { m.resize(cnt); }

template<class IT, class A>
submatch<IT>& get_match_at(match_vector<IT,A>& m, std::size_t pos)
    { return m.at(pos); }

/**\ingroup grpSRXM
Submatch container based on std::vector.
\param IT iterator type
\param A  allocator type*/
template<class IT, class A=std::allocator< submatch<IT> > >
class match_vector
    :std::vector<submatch<IT>,A>
{
    match_vector(const match_vector&);
    match_vector& operator=(const match_vector&);
    typedef std::vector<submatch<IT>,A> base;
public:
    typedef submatch<IT> match_type;
    typedef typename base::const_iterator iterator;
public:
    match_vector()
        {}
    explicit match_vector(std::size_t size)
        { base::reserve(size); }
public:
    using base::size;

    match_vector& reset()
        { base::resize(0); return *this; }
    const match_type& operator[](std::size_t pos) const
        { return base::at(pos); }
    iterator begin() const { return base::begin(); }
    iterator end() const { return base::end(); }

#if defined(_MSC_VER) && _MSC_VER < 1300 //MSVC 6.0
    friend void set_match_count(match_vector& m, std::size_t cnt);
    friend submatch<IT>& get_match_at(match_vector& m, std::size_t pos);
#else
    friend void set_match_count<>(match_vector& m, std::size_t cnt);
    friend submatch<IT>& get_match_at<>(match_vector& m, std::size_t pos);
#endif//MSVC 6.0
};

//-////////////////////////////////////////////////////////////////////////
#define _SRX_NULLABLE(b) \
    enum{ is_nullable = (b) }

struct _Norx {
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { return false; }
};

/**\ingroup grpSRXL
Literal string - up to 30 symbols */
template<
    rx_int c0,       rx_int c1,       rx_int c2 =_Stc, rx_int c3 =_Stc, rx_int c4 =_Stc, rx_int 
c5 =_Stc, rx_int c6 =_Stc, rx_int c7 =_Stc, rx_int c8 =_Stc, rx_int c9 =_Stc,
    rx_int c10=_Stc, rx_int c11=_Stc, rx_int c12=_Stc, rx_int c13=_Stc, rx_int c14=_Stc, rx_int 
c15=_Stc, rx_int c16=_Stc, rx_int c17=_Stc, rx_int c18=_Stc, rx_int c19=_Stc,
    rx_int c20=_Stc, rx_int c21=_Stc, rx_int c22=_Stc, rx_int c23=_Stc, rx_int c24=_Stc, rx_int 
c25=_Stc, rx_int c26=_Stc, rx_int c27=_Stc, rx_int c28=_Stc, rx_int c29=_Stc>
struct s {
    typedef s<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15,c16,c17,c18,c19,c20,c21,c22,
c23,c24,c25,c26,c27,c28,c29,_Stc> tail;

    _SRX_NULLABLE(false);

    enum { length = 1 + tail::length };

    template<class InIt>
    static bool do_match(InIt& it)
        { return (c0 == static_cast<rx_char>(*it)) && tail::do_match(++it); }

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        InIt tend( it + length );
        if( !(tend < end || tend == end) )
            return false;

        InIt t(it);
        bool res = do_match(it);
        if( !res ) it = t;
        return res;
    }
};
template<> struct s<
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc,
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc,
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc>
{
    enum { length = 0 };

    template<class InIt>
    static bool do_match(InIt&)
        { return true; }
};

/**\ingroup grpSRXL
Single literal charachter */
template<rx_char E>
struct c {
    _SRX_NULLABLE(false);

    static bool char_match(rx_char ch)
        { return (E == ch); }

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        bool res = (it != end) && (E == static_cast<rx_char>(*it));
        if( res ) ++it;
        return res;
    }
};

/**\ingroup grpSRXL
Charachter class - like \c "[abcdEF]" or \c "[^abcdEF]"*/
template<
    bool INCLUSIVE,
    rx_int c0,       rx_int c1,       rx_int c2 =_Stc, rx_int c3 =_Stc, rx_int c4 =_Stc, rx_int 
c5 =_Stc, rx_int c6 =_Stc, rx_int c7 =_Stc, rx_int c8 =_Stc, rx_int c9 =_Stc,
    rx_int c10=_Stc, rx_int c11=_Stc, rx_int c12=_Stc, rx_int c13=_Stc, rx_int c14=_Stc, rx_int 
c15=_Stc, rx_int c16=_Stc, rx_int c17=_Stc, rx_int c18=_Stc, rx_int c19=_Stc,
    rx_int c20=_Stc, rx_int c21=_Stc, rx_int c22=_Stc, rx_int c23=_Stc, rx_int c24=_Stc, rx_int 
c25=_Stc, rx_int c26=_Stc, rx_int c27=_Stc, rx_int c28=_Stc, rx_int c29=_Stc>
struct cc_ {
    typedef cc_<INCLUSIVE,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15,c16,c17,c18,c19,
c20,c21,c22,c23,c24,c25,c26,c27,c28,c29,_Stc> tail;

    _SRX_NULLABLE(false);

    static bool char_match_(rx_char ch)
        { return (c0 == ch) || tail::char_match_(ch); }
    static bool char_match(rx_char ch)
        { return INCLUSIVE ? char_match_(ch) : !char_match_(ch); }

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES&)
    {
        bool res = (it != end) && 
            char_match(static_cast<rx_char>(*it));
        if( res ) ++it;
        return res;
    }
};
template<> struct cc_<true,
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc,
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc,
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc>
{ 
    static bool char_match_(rx_char ch)
        { return false; }
};
template<> struct cc_<false,
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc,
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc,
    _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc, _Stc>
{
    static bool char_match_(rx_char ch)
        { return false; }
};

/**\ingroup grpSRXL
Charachter class - like \c "[abcdEF]"*/
template<
rx_int c0,       rx_int c1,       rx_int c2 =_Stc, rx_int c3 =_Stc, rx_int c4 =_Stc, rx_int 
c5 =_Stc, rx_int c6 =_Stc, rx_int c7 =_Stc, rx_int c8 =_Stc, rx_int c9 =_Stc,
rx_int c10=_Stc, rx_int c11=_Stc, rx_int c12=_Stc, rx_int c13=_Stc, rx_int c14=_Stc, rx_int 
c15=_Stc, rx_int c16=_Stc, rx_int c17=_Stc, rx_int c18=_Stc, rx_int c19=_Stc,
rx_int c20=_Stc, rx_int c21=_Stc, rx_int c22=_Stc, rx_int c23=_Stc, rx_int c24=_Stc, rx_int 
c25=_Stc, rx_int c26=_Stc, rx_int c27=_Stc, rx_int c28=_Stc, rx_int c29=_Stc>
struct cc:cc_<true,c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15,c16,c17,c18,c19,c20,
c21,c22,c23,c24,c25,c26,c27,c28,c29> {};

/**\ingroup grpSRXL
Charachter class - like \c "[^abcdEF]"*/
template<
rx_int c0,       rx_int c1,       rx_int c2 =_Stc, rx_int c3 =_Stc, rx_int c4 =_Stc, rx_int 
c5 =_Stc, rx_int c6 =_Stc, rx_int c7 =_Stc, rx_int c8 =_Stc, rx_int c9 =_Stc,
rx_int c10=_Stc, rx_int c11=_Stc, rx_int c12=_Stc, rx_int c13=_Stc, rx_int c14=_Stc, rx_int 
c15=_Stc, rx_int c16=_Stc, rx_int c17=_Stc, rx_int c18=_Stc, rx_int c19=_Stc,
rx_int c20=_Stc, rx_int c21=_Stc, rx_int c22=_Stc, rx_int c23=_Stc, rx_int c24=_Stc, rx_int 
c25=_Stc, rx_int c26=_Stc, rx_int c27=_Stc, rx_int c28=_Stc, rx_int c29=_Stc>
struct not_cc:cc_<false,c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15,c16,c17,c18,
c19,c20,c21,c22,c23,c24,c25,c26,c27,c28,c29> {};

/**\ingroup grpSRXL
Charachter range - like \c "[a-d]" or \c "[^a-d]" */
template<rx_char F, rx_char B, bool INCLUSIVE=true>
struct cr {
    _SRX_NULLABLE(false);

    static bool char_match_(rx_char ch)
        { return (((F < B) ? F : B) <= ch) && (ch <= ((F < B) ? B : F)); }
    static bool char_match(rx_char ch)
        { return INCLUSIVE ? char_match_(ch) : !char_match_(ch); }

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES&)
    {
        bool res = (it != end) && 
            char_match(static_cast<rx_char>(*it));
        if( res ) ++it;
        return res;
    }
};

/**\ingroup grpSRXL
Charachter set - a combination of two or more charachter set classes.
\param CC0-CC9 charachter set classes
\sa charachter set classes: c, cc, cr, cs */
template<
    class CC0,      class CC1,      class CC2=_Sqe, class CC3=_Sqe, class CC4=_Sqe, 
    class CC5=_Sqe, class CC6=_Sqe, class CC7=_Sqe, class CC8=_Sqe, class CC9=_Sqe>
struct cs {
    typedef cs<CC1,CC2,CC3,CC4,CC5,CC6,CC7,CC8,CC9,_Sqe> tail;

    _SRX_NULLABLE(false);

    static bool char_match(rx_char ch)
        { return CC0::char_match(ch) || tail::char_match(ch); }

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        bool res = (it != end) &&
            char_match( static_cast<rx_char>(*it) );
        if( res ) ++it;
        return res;
    }
};
template<> struct cs<_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe> { 
    static bool char_match(rx_char)
        { return false; }
};

/**\ingroup grpSRXO
Zero or more occurences of \c RX - like \c "(abc)*"
\param RX regular expression class*/
template<class RX>
struct star {
    _SRX_NULLABLE(true);
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { while( RX::match(it,end,m) ); return true; }
};

/**\ingroup grpSRXO
One or more occurences of \c RX - like \c "(abc)+"
\param RX regular expression class*/
template<class RX>
struct plus {
    _SRX_NULLABLE(false);

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { return RX::match(it,end,m) && star<RX>::match(it,end,m); }
};

/**\ingroup grpSRXO
Optional occurence of RX - like \c "(abc)?"
\param RX regular expression class*/
template<class RX>
struct opt {
    _SRX_NULLABLE(true);

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { RX::match(it,end,m); return true; }
};

/**\ingroup grpSRXO
Repetition - like \c "(abc){3,5}"
\param RX regular expression class
\param N1 number of times \c RX \em must be matched 
\param N2 number of times \c RX \em may match - must be greater or equal to \c N1*/
template<std::size_t N1, std::size_t N2, class RX>
struct rep {
    _SRX_NULLABLE(( N1==0 ));
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        std::size_t n, nmbk = m.size();
        InIt itbk(it);

        n = N1;
        while( n && RX::match(it,end,m) ) --n;
        if( n ) { it = itbk; set_match_count(m,nmbk); return false; }

        n = N2 - N1;
        while( n && RX::match(it,end,m) ) --n;
        return true;
    }
};

template<class RX> struct _Sqtw:RX {
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { return RX::do_match(it,end,m); }
};

template<class M,class N,class U>
struct _Sqmw:M {
    typedef N rx_next;
    typedef U rx_unext;
};

/**\ingroup grpSRXO
Sequence of expressions - like \c "(abc)(def)"
\param RX0-RX9 regular expression class*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct seq {
    typedef _Sqtw< seq<RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9,_Sqe> > tail;

    _SRX_NULLABLE(RX0::is_nullable && tail::is_nullable);
    enum { is_end = false };

    template<class InIt, class MATCHES>
    static bool do_match(InIt& it, const InIt& end, MATCHES& m)
    { 
        typedef typename MATCHES::match_container mc;
        typedef typename MATCHES::rx_unext rxun;
        if( tail::is_end ) {
            typedef _Sqmw<mc, rxun, _Norx> mw;
            return RX0::match(it,end,reinterpret_cast<mw&>(m));
        } else {
            typedef _Sqmw<mc, tail, rxun> mw;
            return RX0::match(it, end, reinterpret_cast<mw&>(m)) && tail::do_match(it,end,m);
        }
    }

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        InIt itbk(it);
        std::size_t nmbk = m.size();

        typedef typename MATCHES::match_container mc;
        typedef typename MATCHES::rx_next rxn;
        typedef _Sqmw<mc, _Norx, rxn> mw;    
        bool res = do_match(it, end, reinterpret_cast<mw&>(m));

        if( !res ) { it = itbk;    set_match_count(m,nmbk); }
        return res;
    }
};
template<> struct seq<_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe> {
    _SRX_NULLABLE(true);
    enum { is_end = true };
    template<class InIt, class MATCHES> 
    static bool do_match(InIt& it, const InIt& end, MATCHES& m)
        { return true; }
};

/**\ingroup grpSRXO
Alternative - like \c "(abc|def)"
\param RX0-RX9 regular expression class*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct alt {
    typedef alt<RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9,_Sqe> tail;

    _SRX_NULLABLE(RX0::is_nullable || tail::is_nullable);

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        InIt rxit(it); 
        std::size_t nmbk = m.size(), rxnm = nmbk;
        
        bool res = RX0::match(rxit,end,m);
        if( res ) rxnm = m.size();
        res |= tail::match(it,end,m);

        if( it < rxit || it == rxit ) {
            it = rxit;
            set_match_count(m,rxnm);
        } else
        if( res ) {
            std::size_t delta = rxnm - nmbk;
            if( delta ) {
                std::size_t count = m.size() - rxnm;
                for( ; count; ++nmbk, --count )
                    get_match_at(m,nmbk) = get_match_at(m,nmbk+delta);
                set_match_count(m,m.size()-delta);
            }
        }
        return res;
    }
};
template<> struct alt<_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe,_Sqe> {
    _SRX_NULLABLE(false);
    template<class InIt, class MATCHES> static bool match(InIt& it, const InIt& end, MATCHES& m)
        { return false; }
};

/**\ingroup grpSRXO
Submatch marker
\param ID match identifier
\param RX regular expression to capture into match container
\sa 
    match - class used to capture single %match \n
    match_array - %match container based on static array \n
    match_vector - %match container based on std::vector*/
template<rx_int ID, class RX>
struct mark {
    _SRX_NULLABLE(RX::is_nullable);

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        InIt first(it);
        std::size_t nmbk = m.size();
        set_match_count(m,m.size()+1);
        bool res = RX::match(it,end,m);
        if( res && first != it ) {
            typedef typename MATCHES::match_type mt;
            get_match_at(m,nmbk) = mt(ID,first,it);
        } else
            set_match_count(m,nmbk);
        return res;
    }
};

/**\ingroup grpSRXO
Backreference - like <code>"(sense|response) and \\1bility"</code>
\param ID match identifier to refer*/
template<rx_int ID>
struct bk {
    _SRX_NULLABLE(true);

    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        typename MATCHES::iterator r = m.end();
        if( r == m.begin() ) return false;
        while( true ) {
            if( ID == (--r)->id() ) break;
            if( r == m.begin() ) return false;
        }

        InIt tend( it + (r->end() - r->begin()) );
        if( !(tend < end || tend == end) )
            return false;

        InIt t(it), s(r->begin());
        while( s < r->end() )    {
            if( *s != *t ) return false;
            ++s; ++t;
        }
        it = t;
        return true;
    }
};

/**\ingroup grpSRXO
non-greedy version of operator \c star<> */
template<class RX>
struct xstar:star<RX> {
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        typedef typename MATCHES::rx_next rx_next;
        do {
            InIt t(it); 
            std::size_t nmbk = m.size();
            if( rx_next::match(t,end,m) )
                { set_match_count(m,nmbk); break; }
        } while( RX::match(it,end,m) );
        return true;
    }
};

template<class RX>
struct xxstar:star<RX> {
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        typedef typename MATCHES::rx_next rx_next;
        std::size_t next_nm;
        bool here_matched, next_matched=false;
        InIt next_pos(it);
        do {
            InIt t(it); 
            std::size_t nmbk = m.size();
            if( here_matched = rx_next::match(t,end,m) ) {
                next_matched = true;
                next_pos = it;
                set_match_count(m,next_nm = nmbk);
            }
        } while( RX::match(it,end,m) );
        if( !here_matched && next_matched ) 
            { it = next_pos; set_match_count(m,next_nm); }
        return true;
    }
};

/**\ingroup grpSRXO
non-greedy version of operator \c plus<> */
template<class RX>
struct xplus:plus<RX> {
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { return RX::match(it,end,m) && xstar<RX>::match(it,end,m); }
};

template<class RX>
struct xxplus:plus<RX> {
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { return RX::match(it,end,m) && xxstar<RX>::match(it,end,m); }
};

/**\ingroup grpSRXO
non-greedy version of operator \c rep<>*/
template<std::size_t N1, std::size_t N2, class RX>
struct xrep:rep<N1,N2,RX> {
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        if( !rep<N1,N1,RX>::match(it,end,m) )
            return false;

        typedef typename MATCHES::rx_next rx_next;
        std::size_t n = N2 - N1;
        do {
            if( 0 == n-- ) break;
            InIt t(it); 
            std::size_t nmbk = m.size();
            if( rx_next::match(t,end,m) )
                { set_match_count(m,nmbk); break; }
        } while( RX::match(it,end,m) );
        return true;
    }
};

template<std::size_t N1, std::size_t N2, class RX>
struct xxrep:rep<N1,N2,RX> {
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
    {
        if( !rep<N1,N1,RX>::match(it,end,m) )
            return false;

        typedef typename MATCHES::rx_next rx_next;
        std::size_t next_nm;
        bool here_matched = true, next_matched = false;
        InIt next_pos(it);
        std::size_t n = N2 - N1;
        do {
            if( 0 == n-- ) break;
            InIt t(it); 
            std::size_t nmbk = m.size();
            if( here_matched = rx_next::match(t,end,m) ) {
                next_matched = true;
                next_pos = it;
                set_match_count(m,next_nm = nmbk);
            }
        } while( RX::match(it,end,m) );
        if( !here_matched && next_matched ) 
            { it = next_pos; set_match_count(m,next_nm); }
        return true;
    }
};

//-////////////////////////////////////////////////////////////////////////
/**\ingroup grpSRXO
\link static_regex::star star\endlink operator with multiple arguments*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct star_:star<seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::xstar xstar\endlink operator with multiple arguments*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct xstar_:xstar<seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::xxstar xxstar\endlink operator with multiple arguments*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct xxstar_:xxstar<seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::plus plus\endlink operator with multiple arguments*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct plus_:plus<seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::xplus xplus\endlink operator with multiple arguments*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct xplus_:xplus<seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::xxplus xxplus\endlink operator with multiple arguments*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct xxplus_:xxplus<seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::rep rep\endlink operator with multiple arguments*/
template<
    std::size_t N1, std::size_t N2,
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct rep_:rep<N1,N2,seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::xrep xrep\endlink operator with multiple arguments*/
template<
    std::size_t N1, std::size_t N2,
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct xrep_:xrep<N1,N2,seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::xxrep xxrep\endlink operator with multiple arguments*/
template<
    std::size_t N1, std::size_t N2,
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct xxrep_:xxrep<N1,N2,seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::opt opt\endlink operator with multiple arguments*/
template<
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct opt_:opt<seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

/**\ingroup grpSRXO
\link static_regex::mark mark\endlink operator with multiple arguments*/
template<
    rx_int ID,
    class RX0,      class RX1,      class RX2=_Sqe, class RX3=_Sqe, class RX4=_Sqe, 
    class RX5=_Sqe, class RX6=_Sqe, class RX7=_Sqe, class RX8=_Sqe, class RX9=_Sqe>
struct mark_:mark<ID,seq<RX0,RX1,RX2,RX3,RX4,RX5,RX6,RX7,RX8,RX9> > {};

//-////////////////////////////////////////////////////////////////////////
// Often-used primitives
template<bool INCLUSIVE> struct ws_:cc<INCLUSIVE, ' ', '\t'>{};
struct ws:ws_<true>{};

template<bool INCLUSIVE> struct digit_:cr<'0','9',INCLUSIVE>{};
struct digit:digit_<true>{};

template<bool INCLUSIVE> struct xdigit_:cs< cr<'0','9',INCLUSIVE>, cr<'A','F',INCLUSIVE>, 
cr<'a','f',INCLUSIVE> >{};
struct xdigit:xdigit_<true>{};

template<bool INCLUSIVE> struct alpha_:cs< cr<'A','Z',INCLUSIVE>, cr<'a','z',INCLUSIVE> >{};
struct alpha:alpha_<true>{};

template<bool INCLUSIVE> struct alphanum_:cs< alpha_<INCLUSIVE>, digit_<INCLUSIVE> >{};
struct alphanum:alphanum_<true>{};

struct eol:seq<opt<c<'\r'> >, c<'\n'> >{};

struct eos {
    _SRX_NULLABLE(true);
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { return it == end; }
};

struct dot {
    _SRX_NULLABLE(false);
    template<class InIt, class MATCHES>
    static bool match(InIt& it, const InIt& end, MATCHES& m)
        { return (it != end) ? (++it, true) : false; }
};

//-////////////////////////////////////////////////////////////////////////
template<class IT> class _NoMatches {
    _NoMatches() {}
public:
    typedef submatch<IT> match_type;
    typedef const match_type* iterator;
    typedef _NoMatches match_container;
    typedef _Norx rx_unext;
    typedef _Norx rx_next;
public:
    std::size_t size() const { return 0; }
    iterator begin() const { return 0; }
    iterator end() const { return 0; }
    static _NoMatches<IT>& _Get() { static _NoMatches<IT> m; return m; }
};

template<class IT>
void set_match_count(_NoMatches<IT>& m, std::size_t cnt)
    {}

template<class IT>
submatch<IT> get_match_at(_NoMatches<IT>& m, std::size_t pos)
    { return submatch<IT>(); }

template<class M> struct _Mmw:M {
    typedef _Mmw match_container;
    typedef _Norx rx_unext;
    typedef _Norx rx_next;
};

template<class RX, class InIt>
inline bool match(InIt& it, const InIt& end)
    { return RX::match(it,end, _NoMatches<InIt>::_Get()); }

template<class RX, class InIt, class MATCHES>
inline bool match(InIt& it, const InIt& end, MATCHES& m)
{ 
    typedef _Mmw<MATCHES> mw;
    return RX::match(it,end,reinterpret_cast<mw&>(m.reset())); 
}

}//namespace static_regex

#undef _SRX_NULLABLE
#endif//__STATIC_REGEX_H_INCLUDED__
