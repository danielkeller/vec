#ifndef EXPR_H
#define EXPR_H

#include "AstNode.h"
#include "Scope.h"
#include "Location.h"
#include "Type.h"

namespace ast
{
    struct Expr : public virtual AstNode0
    {
        typ::Type type;
        tok::Location loc;
        virtual bool isLval() {return false;};
        Expr() = default;
        Expr(tok::Location &&l) : loc(l) {};
    };

    struct NullExpr : public Expr {};

    struct LvalExpr : public Expr
    {
        LvalExpr(tok::Location &&l) : Expr(std::move(l)) {};
        bool isLval() {return true;};
    };

    struct VarExpr : public LvalExpr
    {
        Ident var;

        VarExpr(Ident v, tok::Location &&l) : LvalExpr(std::move(l)), var(v) {};
    };

    //general binary expression
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
}

#endif
