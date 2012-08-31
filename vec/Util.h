#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <ostream>
#include <sstream>
#include <type_traits>
#include <typeinfo>
#include <cassert>

namespace ast
{
    class Node0;
}

//normally it's better to leave on std:: but this more than doubles the length
using std::move;

//#define exact_cast dynamic_cast

#ifndef exact_cast
template<class To, class From>
To exact_cast(From from)
{
    if (from != nullptr && typeid(typename std::remove_pointer<To>::type) == typeid(*from))
        return static_cast<To>(from);
    else
        return nullptr;
}
#endif

#ifdef _DEBUG

//FIXME: user won't see the message w/o a debugger
template<class To, class From>
To assert_cast(From from, const char *)
{
    To ret = exact_cast<To>(from);
    assert(ret);
    return ret;
}

#else

template<class To, class From>
To assert_cast(From from, const char *)
{
    return static_cast<To>(from);
}

#endif

namespace utl
{
    //TODO: create file path object

    template <class T>
    inline std::string to_str (const T& t)
    {
        std::stringstream ss;
        ss << t;
        return ss.str();
    }

    //constant string class with no managed storage
    class weak_string
    {
        const char *s_begin, *s_end;

    public:

        typedef const char * const_iterator;

        weak_string() : s_begin(NULL), s_end(NULL) {}
        weak_string(std::string & str) {assign(str);}
        weak_string(const char * b, const char * e) {assign(b, e);}
        weak_string(const char * l) {assign(l);}

        void assign(std::string & str);
        void assign(const char * b, const char * e) {s_begin = b; s_end = e;}
        void assign(const char * l); //create from line that starts with l

        const_iterator begin() const {return s_begin;}
        const_iterator end() const {return s_end;}

        size_t length() const {return s_end - s_begin;}

        operator std::string() const {return std::string(begin(), end());}

        std::string operator+(weak_string & rhs) const {return (std::string)*this + (std::string)rhs;}
        std::string operator+(std::string & rhs) const {return (std::string)*this + rhs;}
    };
    
    inline std::string operator+(std::string & lhs, weak_string & rhs)
    {
        return lhs + (std::string)rhs;
    }

    template <class CharT, class Traits>
    inline std::basic_ostream<CharT, Traits>& 
        operator<< (std::basic_ostream<CharT, Traits>& os, const weak_string& str)
    {
        os.write(str.begin(), str.length());
        return os;
    }
}

#endif
