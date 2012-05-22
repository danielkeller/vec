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
}

#endif
