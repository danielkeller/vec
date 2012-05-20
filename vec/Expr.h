#ifndef EXPR_H
#define EXPR_H

#include "AstNode.h"
#include "Scope.h"
#include "Location.h"
#include "Type.h"

namespace ast
{
    typedef int BlockScope;

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
        using Expr::Expr;
        bool isLval() {return true;};
    };

    struct VarExpr : public LvalExpr
    {
        Ident var;
        BlockScope owner;

        VarExpr(Ident v, BlockScope b, tok::Location &&l) : LvalExpr(std::move(l)), var(b), owner(b) {};
    };

    struct SemiExpr : public Expr, public AstNode<Expr, Expr>
    {
        using AstNode<Expr, Expr>::AstNode;
    };
}

#endif
