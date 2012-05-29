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
    //check for assign to non-lval
    //check for reffing non-lval
    AstWalk<OpAssignExpr>([] (OpAssignExpr* ex)
    {
        if (ex->assignOp == tok::at && !ex->getChild<1>()->isLval())
            err::Error(ex->opLoc) << "cannot reference non-lval" << err::caret
                << ex->getChild<1>()->loc << err::underline << err::endl;
    });

    //add loop points for ` expr
}
