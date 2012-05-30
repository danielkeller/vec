#include "Sema.h"
#include "Stmt.h"
#include "Error.h"

#include <iostream>

using namespace ast;
using namespace sa;

template<class T>
void Sema::AstWalk(std::function<void(T*)> action)
{
    AstWalker<T> aw(action, cu->treeHead);
}

void Sema::Phase1()
{
    //if there's a lot of dynamic casting going on, try adding new ast walker
    //types that select child and descendent nodes for example.
    //if only a few steps need that, let them implement it

    //eliminate () -> ExprStmt -> Expr form
    AstWalk<Block>([] (Block *b)
    {
        ExprStmt* es = dynamic_cast<ExprStmt*>(b->getChild<0>());
        if (es)
        {
            b->parent->replaceChild(b, es->getChild<0>());
            es->getChild<0>() = 0; //null so we don't delete everything
            delete b;
        }
    });

    //do this after types in case of ref types

    //check for assign to non-lval
    AstWalk<AssignExpr>([] (AssignExpr* ex)
    {
        if (!ex->getChild<0>()->isLval())
            err::Error(ex->getChild<0>()->loc) << "assignment to non-lval" << err::underline
                << ex->opLoc << err::caret << err::endl;
    });

    //check for ref-assigning non-lval
    AstWalk<OpAssignExpr>([] (OpAssignExpr* ex)
    {
        if (ex->assignOp == tok::at && !ex->getChild<1>()->isLval())
            err::Error(ex->opLoc) << "cannot reference non-lval" << err::caret
                << ex->getChild<1>()->loc << err::underline << err::endl;
    });

    //add loop points for ` expr
}
