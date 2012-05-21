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
        Block(Expr* conts) : AstNode<Expr>(conts) {};
        Block(Expr* conts, tok::Location &&l) : Expr(std::move(l)), AstNode<Expr>(conts) {};
    };
}

#endif
