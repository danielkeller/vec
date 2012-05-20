#ifndef STMT_H
#define STMT_H

#include "Expr.h"

#include <algorithm>

namespace ast
{
    struct Block : public Expr, public AstNode<Expr>
    {
        using AstNode<Expr>::AstNode;
    };

    struct FuncBody : public AstNode<Expr>
    {
        std::vector<Scope> scopes;

        BlockScope makeScope(Scope * parent)
        {
            scopes.emplace_back(parent);
            return scopes.size() - 1;
        }

        //using AstNode<Expr>::AstNode; //WTF??
        FuncBody(Expr *e) : AstNode<Expr>(e) {}
    };
}

#endif
