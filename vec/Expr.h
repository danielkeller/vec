#ifndef EXPR_H
#define EXPR_H

#include "AstNode.h"
#include "Scope.h"
#include "Location.h"

namespace ast
{
    //leaf expression type
    struct NullExpr : public Node0
    {
        NullExpr(tok::Location const &l) : Node0(l) {};
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";};
        typ::Type& Type() {return typ::null;}
    };

    struct DeclExpr;

    struct VarExpr : public Node0
    {
        DeclExpr* var; //the variable we belong to

        VarExpr(DeclExpr* v, tok::Location const &l) : Node0(l), var(v) {};
        bool isLval() {return true;};
        inline std::string myLbl();
        const char *myColor() {return "5";};
        inline typ::Type& Type();
    };

    //could possibly have a weak_string of its name?
    struct DeclExpr : public VarExpr
    {
        Scope* owner; //to look up typedefs
        Ident name; //for errors

        DeclExpr(Ident n, typ::Type t, Scope* o, tok::Location const &l)
            : VarExpr(this, l), owner(o), name(n) {type = t;}
        DeclExpr(const DeclExpr& old) //this has to be specified because var must point to this
            : VarExpr(this, old.loc), owner(old.owner), name(old.name) {type = old.type;}

        std::string myLbl() {return Type().to_str() + " " + utl::to_str(name);}
        typ::Type& Type() {return type;} //have to re-override it back to the original
    };

    //put this here so it knows what a DeclExpr is
    std::string VarExpr::myLbl()
    {
        return var != 0 ? "var " + utl::to_str(var->name) : "undefined var";
    }

    typ::Type& VarExpr::Type()
    {
        return var->Type();
    }

    struct FuncDeclExpr : public DeclExpr
    {
        Scope* funcScope;
        FuncDeclExpr(Ident n, typ::Type t, Scope* s, tok::Location const &l)
            : DeclExpr(n, t, s->getParent(), l), funcScope(s) {}
    };

    //declaration of an entire function overload group
    struct OverloadGroupDeclExpr : public DeclExpr
    {
        typ::Type& Type() {return typ::overload;}
        std::list<FuncDeclExpr*> functions;
        OverloadGroupDeclExpr(Ident n, Scope* global, tok::Location const &firstLoc)
            : DeclExpr(n, typ::error, global, firstLoc) {}
    };

    struct ConstExpr : public Node0
    {
        ConstExpr(tok::Location &l) : Node0(l) {};
        const char *myColor() {return "7";};
    };

    struct IntConstExpr : public ConstExpr
    {
        long long value;
        IntConstExpr(long long v, tok::Location &l) : ConstExpr(l), value(v) {};
        std::string myLbl() {return utl::to_str(value);}
    };

    struct FloatConstExpr : public ConstExpr
    {
        long double value;
        FloatConstExpr(long double v, tok::Location &l) : ConstExpr(l), value(v) {};
        std::string myLbl() {return utl::to_str(value);}
    };

    struct StringConstExpr : public ConstExpr
    {
        Str value;
        StringConstExpr(Str v, tok::Location &l) : ConstExpr(l), value(v) {};
        std::string myLbl() {return "str: " + utl::to_str(value);}
    };

    struct OverloadableExpr
    {
        Scope* owner;
        FuncDeclExpr* ovrResult;
        OverloadableExpr(Scope* o)
            : owner(o), ovrResult(0) {}
    };

    //general binary expressions
    struct BinExpr : public OverloadableExpr, public Node2
    {
        tok::TokenType op;
        tok::Location opLoc;
        BinExpr(Node0* lhs, Node0* rhs, Scope* sc, tok::Token &o)
            : OverloadableExpr(sc),
            Node2(lhs, rhs), op(o.type), opLoc(o.loc)
        {};
        BinExpr(Node0* lhs, Node0* rhs, Scope* sc, tok::TokenType o)
            : OverloadableExpr(sc),
            Node2(lhs, rhs), op(o)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct AssignExpr : public BinExpr
    {
        AssignExpr(Node0* lhs, Node0* rhs, Scope* sc, tok::Token &o)
            : BinExpr(lhs, rhs, sc, o) {};
        std::string myLbl() {return "'='";}
        typ::Type& Type() {return getChildA()->Type();}
    };

    struct OpAssignExpr : public AssignExpr
    {
        tok::TokenType assignOp;
        OpAssignExpr(Node0* lhs, Node0* rhs, Scope* sc, tok::Token &o)
            : AssignExpr(lhs, rhs, sc, o), assignOp(o.value.op) {};
        std::string myLbl() {return std::string(tok::Name(assignOp)) + '=';}
    };

    struct OverloadCallExpr : public OverloadableExpr, public NodeN
    {
        VarExpr* func; //so tree walker won't see it
        OverloadCallExpr(VarExpr* lhs, Node0* rhs, Scope* o, const tok::Location & l)
            : OverloadableExpr(o), NodeN(rhs, l), func(lhs) {}
        ~OverloadCallExpr() {delete func;} //has to be deleted manually
        std::string myLbl() {return utl::to_str(func->var->name) + " ?:?";};
    };

    inline BinExpr* makeBinExpr(Node0* lhs, Node0* rhs, Scope* sc, tok::Token &op)
    {
        if (op == tok::equals)
            return new AssignExpr(lhs, rhs, sc, op);
        else if (op == tok::opequals)
            return new OpAssignExpr(lhs, rhs, sc, op);
        else
            return new BinExpr(lhs, rhs, sc, op);
    }

    //unary expressions
    struct UnExpr : public Node1
    {
        tok::TokenType op;
        UnExpr(Node0* arg, tok::Token &o)
            : Node1(arg, o.loc + arg->loc), op(o.type)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct IterExpr : public Node1
    {
        IterExpr(Node0* arg, tok::Token &o)
            : Node1(arg, o.loc + arg->loc)
        {};

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "`";}
    };

    struct AggExpr : public Node1
    {
        tok::TokenType op;
        AggExpr(Node0* arg, tok::Token &o)
            : Node1(arg, o.loc + arg->loc), op(o.value.op)
        {};
        std::string myLbl() {return tok::Name(op) + std::string("=");}
    };

    //TODO: more accurate location
    struct ListifyExpr : public NodeN
    {
        ListifyExpr(Node0* arg)
            : NodeN(arg, arg->loc)
        {};
        std::string myLbl() {return "\\{...\\}";}
    };

    struct TuplifyExpr : public NodeN
    {
        TuplifyExpr(Node0* arg)
            : NodeN(arg, arg->loc)
        {};
        std::string myLbl() {return "[...]";}
    };

    //postfix expressions
    //for ++ and --
    struct PostExpr : public Node1
    {
        tok::TokenType op;
        PostExpr(Node0* arg, tok::Token &o)
            : Node1(arg, arg->loc + o.loc), op(o.type)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct TupAccExpr : public BinExpr
    {
        TupAccExpr(Node0* lhs, Node0* rhs, tok::Token &o)
            : BinExpr(lhs, rhs, 0, o)
        {}

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "a[b]";}
    };

    struct ListAccExpr : public BinExpr
    {
        ListAccExpr(Node0* lhs, Node0* rhs, Scope* sc, tok::Token &o)
            : BinExpr(lhs, rhs, sc, o)
        {}

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "a{b}";}
    };
}

#endif
