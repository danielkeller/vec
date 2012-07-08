#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>

using namespace ast;
using namespace sa;

Expr* sa::findEndExpr(AstNodeB* srch)
{
    while (true)
    {
        if (Block *b = dynamic_cast<Block*>(srch))
            srch = b->getChildA();
        else if (StmtPair* sp = dynamic_cast<StmtPair*>(srch))
            srch = sp->getChildB();
        else if (ExprStmt* es = dynamic_cast<ExprStmt*>(srch))
            srch = es->getChildA();
        else if (Expr* e = dynamic_cast<Expr*>(srch))
            return e;
        else //it's a stmt
            return 0;
    }
}

//Phase one is for insertion / modification of nodes in a way which
//generally preserves the structure of the AST, and/or which need that
//structure to not be compacted into basic blocks
void Sema::Phase1()
{
    //if there's a lot of dynamic casting going on, try adding new ast walker
    //types that select child and descendent nodes for example.
    //if only a few steps need that, let them implement it

    //insert overload group declarations into the tree so they get cleaned up
    for(auto it : cu->global.varDefs)
        if (OverloadGroupDeclExpr* oGroup = dynamic_cast<OverloadGroupDeclExpr*>(it.second))
            cu->TreeHead(new StmtPair(new ExprStmt(oGroup), cu->TreeHead()));

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

    //flatten out listify and tuplify expressions
    auto ifyFlatten = [] (AstNodeN<Expr>* ify)
    {
        BinExpr* comma = dynamic_cast<BinExpr*>(ify->chld.back());
        while (comma != 0 && comma->op == tok::comma)
        {
            ify->chld.pop_back(); //remove comma
            ify->appendChild(comma->getChildA()); //add the left child
            ify->appendChild(comma->getChildB()); //add the (potetially comma) right child
            comma->nullChildA(), comma->nullChildB();
            delete comma;
            comma = dynamic_cast<BinExpr*>(ify->chld.back());
        }
    };
    AstWalk<ListifyExpr>(ifyFlatten);
    AstWalk<TuplifyExpr>(ifyFlatten);

    //push flattened tuplify exprs into func calls
    AstWalk<OverloadCallExpr>([] (OverloadCallExpr* call)
    {
        TuplifyExpr* args = dynamic_cast<TuplifyExpr*>(call->getChild(0));
        if (args == 0)
            return;
        call->chld.pop_back();
        call->consume(args);
    });

    //Package* pkg = new Package();

    //FIXME: memory leak when functions are declared & not defined
    CachedAstWalk<AssignExpr>([this] (AssignExpr* ae)
    {
        FuncDeclExpr* fde = dynamic_cast<FuncDeclExpr*>(ae->getChildA());
        //TODO: or varexpr?
        if (fde == 0)
            return;

        //insert intrinsic declaration if that's what this is
        if (OverloadCallExpr* call = dynamic_cast<OverloadCallExpr*>(ae->getChildB()))
        {
            VarExpr* ve = call->func;
            if (ve->var == cu->reserved.intrin_v)
            {
                IntConstExpr* ice = dynamic_cast<IntConstExpr*>(call->getChild(0));
                assert(ice && "improper use of __intrin");
                IntrinDeclExpr* ide = new IntrinDeclExpr(fde, (int)ice->value);
                ae->parent->replaceChild(ae, ide);

                //now replace the function decl in the overload group
                OverloadGroupDeclExpr* oGroup = dynamic_cast<OverloadGroupDeclExpr*>(cu->global.getVarDef(fde->name));
                assert(oGroup && "function decl not in group");
                std::remove(oGroup->functions.begin(), oGroup->functions.end(), fde);
                oGroup->functions.push_back(ide);

                delete ae;
                return;
            }
        }

        Stmt* conts = new ExprStmt(ae->getChildB());

        //add decl exprs generated earlier to the AST
        for (auto it : fde->funcScope->varDefs)
        {
            conts = new StmtPair(new ExprStmt(it.second), conts);
        }

        //create and insert function definition
        FunctionDef* fd = new FunctionDef(fde->Type(), conts);
        if (fde->name == cu->reserved.main
            && fde->Type().compare(cu->tm.makeList(cu->reserved.string_t))
                == typ::TypeCompareResult::valid)
            cu->entryPt = fd; //just reassign it if it's defined twice, there will already be an error

        ae->setChildB(fd);

        //find expression in tail position
        Expr* end = findEndExpr(fd->getChildA());
        if (end) //if it's really there
        {
            //we know its an expr stmt
            ExprStmt* endParent = dynamic_cast<ExprStmt*>(end->parent);
            ReturnStmt* impliedRet = new ReturnStmt(end);
            endParent->parent->replaceChild(endParent, impliedRet); //put in the implied return
            endParent->nullChildA();
            delete endParent;
        }
    });

    //after we do this, anything under root is global initialization code
    //so make the __init function for it
    //TODO: does this run at compile time or run time?
    //typ::Type void_void = cu->tm.makeFunc(typ::null, cu->tm.finishTuple()); //empty tuple
    //FuncDeclExpr* fde = new FuncDeclExpr(cu->reserved.init, void_void, &(cu->global), cu->treeHead->loc);
    //FunctionDef* init = new FunctionDef(fde, cu->treeHead); //stick everything in there
    //pkg->appendChild(init);
    //cu->treeHead = pkg;

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

    //replace variables that represent overloaded functions with the function that they
    //must represent, if there is only one such function
    //this makes function pointers & stuff easier
    AstWalk<VarExpr>([] (VarExpr* ve)
    {
        OverloadGroupDeclExpr* oGroup = dynamic_cast<OverloadGroupDeclExpr*>(ve->var);
        if (oGroup == 0 || dynamic_cast<DeclExpr*>(ve) != 0)
            return;

        if (oGroup->functions.size() == 1)
            ve->var = oGroup->functions.front();
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
