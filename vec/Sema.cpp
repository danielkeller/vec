#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>

using namespace ast;
using namespace sa;

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
        else if (srch->isExpr())
            return srch;
        else
            return nullptr;
    }
}

void Sema::validateTree()
{
    for (auto n : Subtree<>(mod))
    {
        Node0* parent = n->parent;
        if (parent)
        {
            assert(parent != n && "cyclic parent");
            Ptr node = n->detachSelf(); //will fire assertion if n is not a child
            parent->replaceDetachedChild(move(node));
        }
    }
}

//TODO: is this a good place for this?
inline void deleter::operator()(Node0* x)
{
    //this is not a good thing for general debugging, it's probably very annoying (and causes invalid reads)
#if 0
    std::cerr << "deleting a " << x->myLbl() << std::endl;
#endif
    delete x;
}