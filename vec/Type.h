#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include "Token.h"

namespace utl
{
    class weak_string;
}
namespace par
{
    class Parser;
    class TupleContsParser;
}

namespace llvm
{
    class Type;
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
        bool operator!=(const Type& other) const {return node != other.node;}

        //this allows use in stl map for example
        bool operator<(const Type& other) const {return node < other.node;}

        std::string to_str();

        //do not call this before calling typ::mgr.makeLLVMTypes()
        llvm::Type* toLLVM();

        //factory functions for derived types
        //the const is kind of a lie but it helps when you have types in maps
        FuncType getFunc() const;
        ListType getList() const;
        TupleType getTuple() const;
        RefType getRef() const;
        NamedType getNamed() const;
        ParamType getParam() const;
        PrimitiveType getPrimitive() const;

        friend class TypeManager;
        friend class TupleBuilder;
        friend class NamedBuilder;
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
        Type conts();
        bool isValid() {return und_node != 0;}
        friend class Type;
    };

    class TupleType : public Type
    {
        TupleNode* und_node;
    public:
        Type elem(size_t pos);
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
        Ident name();
        Type realType();
        unsigned int numArgs();
        bool isExternal();
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
        bool isArith(); //will return false for invalid types
        friend class Type;
    };

    extern Type int8;
    extern Type int16;
    extern Type int32;
    extern Type int64;

    extern Type float32;
    extern Type float64;
    extern Type float80;

    extern Type boolean;

    extern Type any;
    extern Type null;
    extern Type overload;
    extern Type error;
    extern Type undeclared;

    class TupleBuilder
    {
        friend class TypeManager;
        std::vector<std::pair<TypeNodeB*, Ident>> tupleConts;
    public:
        void push_back(Type elem, Ident name) {tupleConts.emplace_back(elem.node, name);}
    };

    class NamedBuilder
    {
        friend class TypeManager;
        std::list<TypeNodeB*> namedConts;
    public:
        void push_back(Type arg) {namedConts.push_back(arg);}
    };

    class TypeManager
    {
        std::list<TypeNodeB*> nodes;

        //set this flag after making llvm types so they get created if we need to add
        //new types
        bool makeLLVMImmediately;

        //either add n to the list of known nodes
        //or delete n and return an identical node from the list
        TypeNodeB* unique(TypeNodeB* n);

        //creates a new type with params repaced by the specified types
        Type substitute(Type old, std::map<Ident, TypeNodeB*>& subs);

        //internal version of this function
        Type makeNamed(Type conts, Ident name, std::vector<Ident>& params, std::list<TypeNodeB*>& args);

    public:
        TypeManager()
            : makeLLVMImmediately(false) {}
        ~TypeManager();

        Type makeList(Type conts, int length = 0);
        Type makeRef(Type conts);
        Type makeParam(Ident name);
        Type makeFunc(Type ret, Type arg);
        Type makeTuple(TupleBuilder&);
        Type makeNamed(Type conts, Ident name, std::vector<Ident>& params, NamedBuilder& args)
        {return makeNamed(conts, name, params, args.namedConts);}
        Type makeExternNamed(Ident name, NamedBuilder& args);
        Type fixExternNamed(NamedType toFix, Type conts, std::vector<Ident>& params);
        Type makeNamed(Type conts, Ident name);

        //these must be made after importing so all types are complete
        void makeLLVMTypes();

        void printAll(std::ostream& os);

        //these functions aren't really part of the public interface but its simpler to make them public

        //performs a deep copy, keeping everything unique and subbing in params
        TypeNodeB* clone(TypeNodeB* n);
    };

    extern TypeManager mgr;

    //result of comparison saying how "close" two types are
    //if they are entirely different the score is -1
    class TypeCompareResult
    {
        unsigned int score; //unsigned so invalid result is always greater than valid
        TypeCompareResult(int s) : score(s) {}

    public:
        TypeCompareResult(bool valid) {score = valid ? 0 : -1;}
        bool isValid() {return score != (unsigned int)-1;}

        TypeCompareResult& operator+=(int diff)
        {
            if (isValid())
                score += diff;
            return *this;
        }

        TypeCompareResult operator+(int diff)
        {
            return TypeCompareResult(*this) += diff;
        }

        TypeCompareResult& operator+= (TypeCompareResult other)
        {
            if (other.isValid())
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
