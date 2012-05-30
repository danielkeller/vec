#ifndef EXPR_H
#define EXPR_H

#include "AstNode.h"
#include "Scope.h"
#include "Location.h"
#include "Type.h"

namespace ast
{
    typedef int Str;

    //abstract expression type
    struct Expr : public virtual AstNode0
    {
        typ::Type type;
        tok::Location loc;
        virtual bool isLval() {return false;};
        //Expr() = default;
        Expr(tok::Location &&l) : loc(l) {};
        const char *myColor() {return "4";};
    };

    //leaf expression type
    struct NullExpr : public Expr, public AstNode<>
    {
        NullExpr(tok::Location &&l) : Expr(std::move(l)) {};
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";};
    };

    struct VarExpr : public Expr, public AstNode<>
    {
        Ident var;

        VarExpr(Ident v, tok::Location &&l) : Expr(std::move(l)), var(v) {};
        bool isLval() {return true;};
        std::string myLbl() {return "var: " + utl::to_str(var);}
        const char *myColor() {return "5";};
    };

    struct ConstExpr : public Expr, public AstNode<>
    {
        ConstExpr(tok::Location &&l) : Expr(std::move(l)) {};
        const char *myColor() {return "7";};
    };

    struct IntConstExpr : public ConstExpr
    {
        long long value;
        IntConstExpr(long long v, tok::Location &l) : ConstExpr(std::move(l)), value(v) {};
        std::string myLbl() {return utl::to_str(value);}
    };

    struct FloatConstExpr : public ConstExpr
    {
        long double value;
        FloatConstExpr(long double v, tok::Location &l) : ConstExpr(std::move(l)), value(v) {};
        std::string myLbl() {return utl::to_str(value);}
    };

    struct StringConstExpr : public ConstExpr
    {
        Str value;
        StringConstExpr(Str v, tok::Location &l) : ConstExpr(std::move(l)), value(v) {};
        std::string myLbl() {return "str: " + utl::to_str(value);}
    };

    //general binary expressions
    struct BinExpr : public Expr, public AstNode<Expr, Expr>
    {
        tok::TokenType op;
        tok::Location opLoc;
        BinExpr(Expr* lhs, Expr* rhs, tok::Token &o)
            : Expr(lhs->loc + rhs->loc),
            AstNode<Expr, Expr>(lhs, rhs), op(o.type), opLoc(o.loc)
        {};
        BinExpr(Expr* lhs, Expr* rhs, tok::TokenType o)
            : Expr(lhs->loc + rhs->loc),
            AstNode<Expr, Expr>(lhs, rhs), op(o)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct AssignExpr : public BinExpr
    {
        AssignExpr(Expr* lhs, Expr* rhs, tok::Token &o) : BinExpr(lhs, rhs, o) {};
        std::string myLbl() {return "'='";}
    };

    struct OpAssignExpr : public AssignExpr
    {
        tok::TokenType assignOp;
        OpAssignExpr(Expr* lhs, Expr* rhs, tok::Token &o) : AssignExpr(lhs, rhs, o), assignOp(o.value.op) {};
        std::string myLbl() {return std::string(tok::Name(assignOp)) + '=';}
    };

    inline BinExpr* makeBinExpr(Expr* lhs, Expr* rhs, tok::Token &op)
    {
        if (op == tok::equals)
            return new AssignExpr(lhs, rhs, op);
        else if (op == tok::opequals)
            return new OpAssignExpr(lhs, rhs, op);
        else
            return new BinExpr(lhs, rhs, op);
    }

    //unary expressions
    struct UnExpr : public Expr, public AstNode<Expr>
    {
        tok::TokenType op;
        UnExpr(Expr* arg, tok::Token &o)
            : Expr(o.loc + arg->loc),
            AstNode<Expr>(arg), op(o.type)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct IterExpr : public Expr, public AstNode<Expr>
    {
        IterExpr(Expr* arg, tok::Token &o)
            : Expr(o.loc + arg->loc),
            AstNode<Expr>(arg)
        {};

        bool isLval() {return getChild<0>()->isLval();}
        std::string myLbl() {return "`";}
    };

    struct AggExpr : public Expr, public AstNode<Expr>
    {
        tok::TokenType op;
        AggExpr(Expr* arg, tok::Token &o)
            : Expr(o.loc + arg->loc),
            AstNode<Expr>(arg), op(o.value.op)
        {};
        std::string myLbl() {return tok::Name(op) + std::string("=");}
    };

    struct ListifyExpr : public UnExpr
    {
        ListifyExpr(Expr* arg, tok::Token &o)
            : UnExpr(arg, o)
        {};
        std::string myLbl() {return "{...}";}
    };

    struct TuplifyExpr : public UnExpr
    {
        TuplifyExpr(Expr* arg, tok::Token &o)
            : UnExpr(arg, o)
        {};
        std::string myLbl() {return "[...]";}
    };

    //postfix expressions
    //for ++ and --
    struct PostExpr : public Expr, public AstNode<Expr>
    {
        tok::TokenType op;
        PostExpr(Expr* arg, tok::Token &o)
            : Expr(arg->loc + o.loc),
            AstNode<Expr>(arg), op(o.type)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct TupAccExpr : public BinExpr
    {
        TupAccExpr(Expr* lhs, Expr* rhs, tok::Token &o)
            : BinExpr(lhs, rhs, o)
        {}

        bool isLval() {return getChild<0>()->isLval();}
        std::string myLbl() {return "a{b}";}
    };

    struct ListAccExpr : public BinExpr
    {
        ListAccExpr(Expr* lhs, Expr* rhs, tok::Token &o)
            : BinExpr(lhs, rhs, o)
        {}

        bool isLval() {return getChild<0>()->isLval();}
        std::string myLbl() {return "a[b]";}
    };
}

#endif
