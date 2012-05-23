#ifndef STMT_H
#define STMT_H

#include "Expr.h"

#include <algorithm>

namespace ast
{
    struct SemiExpr : public BinExpr
    {
        SemiExpr(Expr *lhs, Expr *rhs) : BinExpr(lhs, rhs, tok::semicolon) {};
    };

    struct Block : public Expr, public AstNode<Expr>
    {
        Scope scope;
        Block(Expr* conts, Scope &&s, tok::Location &&l)
            : Expr(std::move(l)),
            AstNode<Expr>(conts),
            scope(std::move(s))
        {};
    };

    struct IfExpr : public Expr, public AstNode<Expr, Expr>
    {
        IfExpr(Expr* pred, Expr* act, tok::Token &o)
            : Expr(o.loc + act->loc),
            AstNode<Expr, Expr>(pred, act)
        {};
    };

    struct IfElseExpr : public Expr, public AstNode<Expr, Expr, Expr>
    {
        IfElseExpr(Expr* pred, Expr* act1, Expr* act2, tok::Token &o)
            : Expr(o.loc + act2->loc),
            AstNode<Expr, Expr, Expr>(pred, act1, act2)
        {};
    };

}

#endif
