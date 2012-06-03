#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>
#include "Token.h"

namespace utl
{
    class weak_string;
}
namespace ast
{
    class Scope;
}
namespace par
{
    class Parser;
    class TypeListParser;
}

namespace typ
{
    class Type
    {
    public:
        Type();
        utl::weak_string w_str();
        utl::weak_string ex_w_str();
        bool isFunc();
        bool isTempl();

        void expand(ast::Scope *s);

        void clear() {code.clear(); expanded.clear();};

        TypeIter begin();
        TypeIter end();
        //iterators to expanded type
        TypeIter exbegin();
        TypeIter exend();

    private:
        std::string code; //string representation of type - "nominative" type
        std::string expanded; //with all named types inserted - "actual" type

        friend class par::TypeListParser;
        friend class par::Parser;
    };

    class TypeIter
    {
        std::string::iterator pos;

    public:
        TypeIter& operator++();
        const char operator*() {return *pos};
        bool operator==(TypeIter& other) {return pos == other.pos;};
        bool operator!=(TypeIter& other) {return pos != other.pos;};
        void descend() {++pos;};
        utl::weak_string w_str();
        Type type();
    };

    namespace cod
    {
        static const char list = 'L';
        static const char tuple = 'T';
        static const char tupname = 'v'; //name of variable for struct-style access
        static const char dim = 'd'; //dimension of list
        static const char arg = 'A';
        static const char integer = 'I';
        static const char floating = 'F';
        static const char ref = 'R';
        static const char named = 'N';
        static const char any = 'Y';
        static const char param = 'P';
        static const char function = 'U';

        inline char endOf(const char begin)
        {
            return begin - 'A' + 'a'; //ie lowercase version
        }
    }
}

#endif
