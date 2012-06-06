#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>

using namespace ast;
using namespace sa;

void Sema::Phase1()
{
    //if there's a lot of dynamic casting going on, try adding new ast walker
    //types that select child and descendent nodes for example.
    //if only a few steps need that, let them implement it

    //eliminate () -> ExprStmt -> Expr form
    //this needs to happen before loop point addition so regular ()s don't interfere
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
/*
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
*/
    //add loop points for ` expr
    //TODO: detect agg exprs
    AstWalk<IterExpr>([] (IterExpr* ie)
    {
        //find nearest exprStmt up the tree
        AstNode0* n;
        for (n = ie; dynamic_cast<ExprStmt*>(n) == 0; n = n->parent)
            assert(n != 0 && "ExprStmt not found");

        ExprStmt* es = dynamic_cast<ExprStmt*>(n);
        ImpliedLoopStmt* il = dynamic_cast<ImpliedLoopStmt*>(es->parent);
        
        if (!il)
        {
            AstNode0* esParent = es->parent; //save because the c'tor clobbers it
            il = new ImpliedLoopStmt(es);
            esParent->replaceChild(es, il);
        }
        
        il->targets.push_back(ie);
    });
/*
    //any block now contains multiple statements so extract them
    //should split the expression to parts above and below the block to output in the right order
    AstWalk<Block>([] (Block* b)
    {
        if (!dynamic_cast<Expr*>(b->parent)) //regular block not in expression
            return; //ignore

        //find owning ES
        ExprStmt* es;
        for (AstNode0* srch = b->parent; (es = dynamic_cast<ExprStmt*>(srch)) == 0; srch = srch->parent)
            ; //will fail-fast if es not found

        //wrap exprStmt and block contents in pair
        AstNode0* esParent = es->parent; //c'tor clobbers it
        StmtPair* newHead = new StmtPair(es, b->getChild<0>());
        b->getChild<0>() = 0;
        esParent->replaceChild(es, newHead);
        
        //find last stmt in block
        Stmt* last;
        for (last = newHead->getChild<1>(); //start with block's child
             dynamic_cast<StmtPair*>(last); //check if it's still a StmtPair
             last = dynamic_cast<StmtPair*>(last)->getChild<1>()) //iterate to RHS
            ;

        //if it's an exprstmt, link in with a temporary
        if (ExprStmt* lastes = dynamic_cast<ExprStmt*>(last))
        {
            TmpExpr* te = new TmpExpr(lastes->getChild<0>());
            b->parent->replaceChild(b, te);
        }
        else
        {
            b->parent->replaceChild(b, new NullExpr(std::move(static_cast<Stmt*>(b)->loc)));
        }

        delete b; //finally delete the block
    });
*/
    //remove null stmts under StmtPairs
    AstWalk<StmtPair>([] (StmtPair* sp)
    {
        if (!sp->parent)
            return; //we're screwed

        Stmt* realStmt;
        if (dynamic_cast<NullStmt*>(sp->getChild<0>()))
        {
            realStmt = sp->getChild<1>();
            sp->getChild<1>() = 0; //unlink so as not to delete it
        }
        else if (dynamic_cast<NullStmt*>(sp->getChild<1>()))
        {
            realStmt = sp->getChild<0>();
            sp->getChild<0>() = 0;
        }
        else
            return; //nope

        sp->parent->replaceChild(sp, realStmt);
        delete sp;
    });

    //turn exprstmt ; exprstmt into expr , expr. this must happen after loop points.
    AstWalk<StmtPair>([] (StmtPair* sp)
    {
        ExprStmt* lhs = dynamic_cast<ExprStmt*>(sp->getChild<0>());
        ExprStmt* rhs = dynamic_cast<ExprStmt*>(sp->getChild<1>());
        if (lhs && rhs)
        {
            ExprStmt* repl = new ExprStmt(
                              new BinExpr(lhs->getChild<0>(),
                                          rhs->getChild<0>(),
                                          tok::comma));
            lhs->getChild<0>() = 0;
            rhs->getChild<0>() = 0;
            sp->parent->replaceChild(sp, repl);
            delete sp;
        }
    });
    
    //create expr stmts for other things that contain expressions
    AstWalk<CondStmt>([] (CondStmt* cs)
    {
        Expr* e = cs->getExpr();
        TmpExpr* te = new TmpExpr(e);
        cs->replaceChild(e, te);
        
        AstNode0* parent = cs->parent;
        StmtPair* sp = new StmtPair(new ExprStmt(e), cs);
        parent->replaceChild(cs, sp);
    });
}
