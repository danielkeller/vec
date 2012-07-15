#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>

using namespace ast;
using namespace sa;

void Sema::AnyAstWalk(std::function<void(ast::Node0*)> action)
{
    AnyAstWalker aw(mod, action);
}

Node0* sa::findEndExpr(Node0* srch)
{
    while (true)
    {
        if (Block *b = exact_cast<Block*>(srch))
            srch = b->getChildA();
        else if (StmtPair* sp = exact_cast<StmtPair*>(srch))
            srch = sp->getChildB();
        else if (ExprStmt* es = exact_cast<ExprStmt*>(srch))
            srch = es->getChildA();
        else
            return srch;
    }
}

void Sema::validateTree()
{
    AnyAstWalk([] (Node0 *n)
    {
        if (n->parent)
        {
            assert(n->parent != n && "cyclic parent");
            n->parent->replaceChild(n, n); //will fire assertion if n is not a child
        }
    });
}