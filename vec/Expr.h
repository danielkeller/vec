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
    };

    //leaf expression type
    struct NullExpr : public Expr, public AstNode<>
    {
        NullExpr(tok::Location &&l) : Expr(std::move(l)) {};
    };

    struct VarExpr : public Expr, public AstNode<>
    {
        Ident var;

        VarExpr(Ident v, tok::Location &&l) : Expr(std::move(l)), var(v) {};
        bool isLval() {return true;};
    };

    struct ConstExpr : public Expr, public AstNode<>
    {
        ConstExpr(tok::Location &&l) : Expr(std::move(l)) {};
    };

    struct IntConstExpr : public ConstExpr
    {
        long long value;
        IntConstExpr(long long v, tok::Location &l) : ConstExpr(std::move(l)), value(v) {};
    };

    struct FloatConstExpr : public ConstExpr
    {
        long double value;
        FloatConstExpr(long double v, tok::Location &l) : ConstExpr(std::move(l)), value(v) {};
    };

    struct StringConstExpr : public ConstExpr
    {
        Str value;
        StringConstExpr(Str v, tok::Location &l) : ConstExpr(std::move(l)), value(v) {};
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
    };

    struct AssignExpr : public BinExpr
    {
        tok::TokenType assignOp;
        AssignExpr(Expr* lhs, Expr* rhs, tok::Token &o) : BinExpr(lhs, rhs, o), assignOp(o.value.op) {};
    };

    inline BinExpr* makeBinExpr(Expr* lhs, Expr* rhs, tok::Token &op)
    {
        if (op == tok::equals)
            return new AssignExpr(lhs, rhs, op);
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
    };

    struct IterExpr : public Expr, public AstNode<Expr>
    {
        IterExpr(Expr* arg, tok::Token &o)
            : Expr(o.loc + arg->loc),
            AstNode<Expr>(arg)
        {};

        bool isLval() {return getChild<0>()->isLval();}
    };

    struct AggExpr : public UnExpr
    {
        AggExpr(Expr* arg, tok::Token &o)
            : UnExpr(arg, o)
        {};
    };

    struct ListifyExpr : public UnExpr
    {
        ListifyExpr(Expr* arg, tok::Token &o)
            : UnExpr(arg, o)
        {};
    };

    struct TuplifyExpr : public UnExpr
    {
        TuplifyExpr(Expr* arg, tok::Token &o)
            : UnExpr(arg, o)
        {};
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
    };

    struct TupAccExpr : public BinExpr
    {
        TupAccExpr(Expr* lhs, Expr* rhs, tok::Token &o)
            : BinExpr(lhs, rhs, o)
        {}

        bool isLval() {return getChild<0>()->isLval();}
    };

    struct ListAccExpr : public BinExpr
    {
        ListAccExpr(Expr* lhs, Expr* rhs, tok::Token &o)
            : BinExpr(lhs, rhs, o)
        {}

        bool isLval() {return getChild<0>()->isLval();}
    };
}

#endif
