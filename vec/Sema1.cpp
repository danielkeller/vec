#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>

using namespace ast;
using namespace sa;

//Phase one is for insertion / modification of nodes in a way which
//generally preserves the structure of the AST, and/or which need that
//structure to not be compacted into basic blocks
void Sema::Phase1()
{
    //if there's a lot of dynamic casting going on, try adding new ast walker
    //types that select child and descendent nodes for example.
    //if only a few steps need that, let them implement it

    //eliminate () -> ExprStmt -> Expr form
    //this needs to happen before loop point addition so regular ()s don't interfere
    AstWalk<Block>([] (Block *b)
    {
        ExprStmt* es = dynamic_cast<ExprStmt*>(b->getChildA());
        if (es)
        {
            b->parent->replaceChild(b, es->getChildA());
            es->nullChildA(); //null so we don't delete everything
            delete b;
        }
    });

    //FIXME: memory leak when functions are declared & not defined
    CachedAstWalk<AssignExpr>([this] (AssignExpr* ae)
    {
        FuncDeclExpr* fde = dynamic_cast<FuncDeclExpr*>(ae->getChildA());
        if (fde == 0)
            return;

        //a function definition is really a statement masquerading as an expression
        //doing it this way is a. easier (less ambiguity), and b. gives better errors
        ExprStmt* es = dynamic_cast<ExprStmt*>(ae->parent);
        if (!es)
        {
            err::Error(ae->parent->loc) << "a function definition may not be part of an expression" << err::underline;
            return; //can't deal with it
        }

        Stmt* conts = new ExprStmt(ae->getChildB());

        //add decl exprs generated earlier to the AST
        for (auto it : fde->funcScope->varDefs)
        {
            conts = new StmtPair(new ExprStmt(it.second), conts);
        }

        //create and insert function definition
        es->parent->replaceChild(es, new FunctionDef(fde, conts));

        ae->nullChildA();
        ae->nullChildB();
        delete es; //delete expr stmt and assign expr

        validateTree();
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
        AstNodeB* n;
        for (n = ie; dynamic_cast<ExprStmt*>(n) == 0; n = n->parent)
            assert(n != 0 && "ExprStmt not found");

        ExprStmt* es = dynamic_cast<ExprStmt*>(n);
        ImpliedLoopStmt* il = dynamic_cast<ImpliedLoopStmt*>(es->parent);
        
        if (!il)
        {
            AstNodeB* esParent = es->parent; //save because the c'tor clobbers it
            il = new ImpliedLoopStmt(es);
            esParent->replaceChild(es, il);
        }
        
        il->targets.push_back(ie);
    });

    //remove null stmts under StmtPairs
    AstWalk<StmtPair>([] (StmtPair* sp)
    {
        if (!sp->parent)
            return; //we're screwed

        Stmt* realStmt;
        if (dynamic_cast<NullStmt*>(sp->getChildA()))
        {
            realStmt = sp->getChildB();
            sp->nullChildB(); //unlink so as not to delete it
        }
        else if (dynamic_cast<NullStmt*>(sp->getChildB()))
        {
            realStmt = sp->getChildA();
            sp->nullChildA();
        }
        else
            return; //nope

        sp->parent->replaceChild(sp, realStmt);
        delete sp;
    });
    
    //create expr stmts for other things that contain expressions
    AstWalk<CondStmt>([] (CondStmt* cs)
    {
        Expr* e = cs->getExpr();
        TmpExpr* te = new TmpExpr(e);
        cs->replaceChild(e, te);
        
        AstNodeB* parent = cs->parent;
        StmtPair* sp = new StmtPair(new ExprStmt(e), cs);
        parent->replaceChild(cs, sp);
    });
}
