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
    typedef int Ident;
}
namespace par
{
    class Parser;
    class TypeListParser;
}

namespace typ
{
    class TypeIter;

    class Type
    {
    public:
        Type();
        Type(TypeIter &ti);
        utl::weak_string w_str();
        utl::weak_string ex_w_str();
        bool isFunc();
        bool isTempl();

        void expand(ast::Scope *s);

        void clear() {code.clear(); expanded.clear();};

        TypeIter begin(bool arg = false);
        TypeIter end(bool arg = false);
        //iterators to expanded type
        TypeIter exbegin(bool arg = false);
        TypeIter exend(bool arg = false);

    private:
        std::string code; //string representation of type - "nominative" type
        std::string expanded; //with all named types inserted - "actual" type

        friend class par::TypeListParser;
        friend class par::Parser;
    };

    class TypeIter
    {
        std::string::iterator pos;
        TypeIter(std::string::iterator init) : pos(init) {};

    public:
        //repeatedly calling ++ will depth first search the type
        TypeIter& operator++();
        TypeIter operator+(size_t offset);
        TypeIter& operator+=(size_t offset);
        const char operator*() {return *pos;};
        bool operator==(TypeIter& other) {return pos == other.pos;};
        bool operator!=(TypeIter& other) {return pos != other.pos;};

        void descend();
        bool atBottom(); //the current type has no contents
        void advance();
        bool atEnd(); //the thing after the current type is not a type
        void ascend();
        bool atTop(); //there is not another type somewhere ahead

        ast::Ident getName(); //get name of any type with a name, -1 otherwise
        ast::Ident getTupName(); //get name of tuple element

        utl::weak_string w_str();

        friend class Type;
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
