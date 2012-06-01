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
    std::map<ExprStmt*, BasicBlock*> repl;
    AstWalk<ExprStmt>([&repl] (ExprStmt* es)
    {
        BasicBlock* curBB = new BasicBlock();
        repl[es] = curBB; //attach it

        //start looking under current expr stmt, so we don't mix our expressions
        AstWalker<Expr>(es, [&curBB] (Expr* ex)
        {
            if (dynamic_cast<TmpExpr*>(ex))
                return; //don't make a temp for a temp

            //create temporary "set by" current expression
            TmpExpr* te = new TmpExpr(ex);

            //attach temporary to expression's parent in lieu of it
            ex->parent->replaceChild(ex, te);

            //attach expression to the basic block
            curBB->appendChild(ex);
        });
    });

    std::function<void(ExprStmt*)> blockInsert;

    //replace exprstmts with corresponding basic blocks
    blockInsert = [&repl, &blockInsert] (ExprStmt* es)
    {
        assert (repl.count(es) && "basic block doesn't exist");

        //attach basic block in place of expression statement
        es->parent->replaceChild(es, repl[es]);
        //the child is an unneeded temp, don't bother to unlink it
        delete es;
    
        //recurse in case we just inserted more exprstmt that would be missed
        AstWalker<ExprStmt>(repl[es], blockInsert);
    };

    //call it
    AstWalk<ExprStmt>(blockInsert);
}
