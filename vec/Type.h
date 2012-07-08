#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <list>
#include <map>
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
    class TupleContsParser;
}

namespace typ
{
    struct TypeNodeB;

    class FuncType;
    class ListType;
    class TupleType;
    class RefType;
    class NamedType;
    class ParamType;
    class PrimitiveType;

    class TypeCompareResult;

    class Type
    {
        //these are for visual studio's debugger
#ifdef _DEBUG
        std::string name;
#endif

    protected:
        TypeNodeB* node;
        operator TypeNodeB*() {return node;}

    public:
        Type();
        Type(TypeNodeB& n) : node(&n)
        {
#ifdef _DEBUG
            name = to_str();
#endif
        }
        Type(TypeNodeB* n) : node(n)
        {
#ifdef _DEBUG
            name = to_str();
#endif
        }

        TypeCompareResult compare(const Type& other);
        bool operator==(const Type& other) const {return node == other.node;}

        std::string to_str();

        //factory functions for derived types
        FuncType getFunc();
        ListType getList();
        TupleType getTuple();
        RefType getRef();
        NamedType getNamed();
        ParamType getParam();
        PrimitiveType getPrimitive();

        friend class TypeManager;
    };

    struct FuncNode;
    struct ListNode;
    struct TupleNode;
    struct RefNode;
    struct NamedNode;
    struct ParamNode;
    struct PrimitiveNode;

    //horrible copy-paste coding, but with templates we can't preserve encapsulation
    class FuncType : public Type
    {
        FuncNode* und_node;
    public:
        Type arg();
        Type ret();
        bool isValid() {return und_node != 0;}
        friend class Type;
    };

    class ListType : public Type
    {
        ListNode* und_node;
    public:
        bool isValid() {return und_node != 0;}
        friend class Type;
    };

    class TupleType : public Type
    {
        TupleNode* und_node;
    public:
        bool isValid() {return und_node != 0;}
        friend class Type;
    };

    class RefType : public Type
    {
        RefNode* und_node;
    public:
        bool isValid() {return und_node != 0;}
        friend class Type;
    };

    class NamedType : public Type
    {
        NamedNode* und_node;
    public:
        bool isValid() {return und_node != 0;}
        friend class Type;
    };

    class ParamType : public Type
    {
        ParamNode* und_node;
    public:
        bool isValid() {return und_node != 0;}
        friend class Type;
    };

    class PrimitiveType : public Type
    {
        PrimitiveNode* und_node;
    public:
        bool isValid() {return und_node != 0;}
        friend class Type;
    };

    extern Type int8;
    extern Type int16;
    extern Type int32;
    extern Type int64;

    extern Type float16;
    extern Type float32;
    extern Type float64;
    extern Type float80;

    extern Type any;
    extern Type null;
    extern Type overload;
    extern Type error;

    class TypeManager
    {
        std::list<TypeNodeB*> nodes;
        std::list<std::pair<TypeNodeB*, ast::Ident>> tupleConts;
        std::map<ast::Ident, TypeNodeB*> namedConts;

        //either add n to the list of known nodes
        //or delete n and return an identical node from the list
        TypeNodeB* unique(TypeNodeB* n);

    public:
        ~TypeManager();

        Type makeList(Type conts, int length = 0);
        Type makeRef(Type conts);
        Type makeParam(ast::Ident name);
        Type makeFunc(Type ret, Type arg);
        
        void addNamedArg(ast::Ident name, Type t) {namedConts[name] = t;}
        void clearNamedArgs() {namedConts.clear();} //if there was an error
        Type finishNamed(Type conts, ast::Ident name);

        void addToTuple(Type elem, ast::Ident name) {tupleConts.emplace_back(elem.node, name);}
        void clearTuple() {tupleConts.clear();} //if there was an error
        Type finishTuple();

        //these functions aren't really part of the public interface but its simpler to make them public

        //creates a new type with params repaced by the specified types
        Type substitute(Type old, std::map<ast::Ident, TypeNodeB*>& subs);
        //performs a deep copy, keeping everything unique and subbing in params
        TypeNodeB* clone(TypeNodeB* n);
    };

    //result of comparison saying how "close" two types are
    //if they are entirely different the score is -1
    class TypeCompareResult
    {
        unsigned int score; //unsigned so invalid result is always greater than valid
        TypeCompareResult(int s) : score(s) {}

    public:
        TypeCompareResult(bool valid) {score = valid ? 0 : -1;}
        operator bool() {return score != -1;}

        TypeCompareResult& operator+=(int diff)
        {
            if (*this)
                score += diff;
            return *this;
        }

        TypeCompareResult operator+(int diff)
        {
            return TypeCompareResult(*this) += diff;
        }

        TypeCompareResult& operator+= (TypeCompareResult& other)
        {
            if (other)
                operator+=(other.score);
            else
                *this = invalid;
            return *this;
        }

        TypeCompareResult operator+ (TypeCompareResult other)
        {
            return TypeCompareResult(*this) += other;
        }

        //invalid if either are -1
        bool operator< (TypeCompareResult const & other) const {return score < other.score;}

        bool operator== (const TypeCompareResult& other) const {return score == other.score;}
        bool operator!= (const TypeCompareResult& other) const {return !(*this == other);}

        static TypeCompareResult valid;
        static TypeCompareResult invalid;
    };
}

#endif
