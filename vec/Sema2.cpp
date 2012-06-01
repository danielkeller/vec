#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <iostream>
#include <cassert>

using namespace ast;
using namespace sa;

void Sema::Phase2()
{
    //insert temporaries above all expressions
    //because of the way AstWalker works we don't have to worry about
    //it finding the TmpExprs we create

    //populate the basic blocks with each expresion after creating a temporary for its result
    //when we reach the end of an expression end the basic block
    //and start a new one. closure is used to save state (current BB) between calls
    BasicBlock* curBB = new BasicBlock();
    std::map<ExprStmt*, BasicBlock*> repl;
    AstWalk<Expr>([&curBB, &repl] (Expr* ex)
    {
        if (dynamic_cast<TmpExpr*>(ex))
            return; //don't make a temp for a temp

        //create temporary "set by" current expression
        TmpExpr* te = new TmpExpr(ex);

        //attach temporary to expression's parent in lieu of it
        ex->parent->replaceChild(ex, te);

        //attach expression to the basic block
        curBB->appendChild(ex);

        //attach basic block in place of expression statement
        ExprStmt* es = dynamic_cast<ExprStmt*>(te->parent);
        if (es)
        {
            repl[es] = curBB; //attach the old one
            curBB = new BasicBlock(); //start a new one
        }
    });
    delete curBB; //an extra one was created

    //replace exprstmts with corresponding basic blocks
    AstWalk<ExprStmt>([&repl] (ExprStmt* es)
    {
        if (!repl.count(es)) //doesn't exist?
            return;

        es->parent->replaceChild(es, repl[es]);
        //the child is an unneeded temp, don't bother to unlink it
        delete es;
    });
}
