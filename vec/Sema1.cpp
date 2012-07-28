#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"

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
        ExprStmt* es = exact_cast<ExprStmt*>(b->getChildA());
        if (es)
        {
            b->parent->replaceChild(b, es->detachChildA());
        }
    });

    //flatten out listify and tuplify expressions
    auto ifyFlatten = [] (NodeN* ify)
    {
        auto comma = ify->detachChildAs<BinExpr>(ify->Children().size() - 1);
        while (comma && comma->op == tok::comma)
        {
            ify->popChild(); //remove comma
            ify->appendChild(comma->detachChildA()); //add the left child
            ify->appendChild(comma->detachChildB()); //add the (potetially comma) right child
            comma = ify->detachChildAs<BinExpr>(ify->Children().size() - 1);
        }
    };
    AstWalk<ListifyExpr>(ifyFlatten);
    AstWalk<TuplifyExpr>(ifyFlatten);

    //push flattened tuplify exprs into func calls
    AstWalk<OverloadCallExpr>([] (OverloadCallExpr* call)
    {
        auto args = call->detachChildAs<TuplifyExpr>(0);
        if (!args)
            return;
        call->popChild();
        call->swap(args.get());
    });

    //Package* pkg = new Package();

    //FIXME: memory leak when functions are declared & not defined
    CachedAstWalk<AssignExpr>([this] (AssignExpr* ae)
    {
        FuncDeclExpr* fde = exact_cast<FuncDeclExpr*>(ae->getChildA());
        //TODO: or varexpr?
        if (!fde)
            return;

        //insert intrinsic declaration if that's what this is
        if (OverloadCallExpr* call = exact_cast<OverloadCallExpr*>(ae->getChildB()))
        {
            VarExpr* ve = call->func.get();
            if (ve->var == Global().reserved.intrin_v)
            {
                for (auto it : fde->funcScope->varDefs)
                    Ptr(it.second); //this is kind of dumb but it doesn't access the d'tor directly

                IntConstExpr* ice = exact_cast<IntConstExpr*>(call->getChild(0));
                assert(ice && "improper use of __intrin");
                auto ide = MkNPtr(new IntrinDeclExpr(fde, (int)ice->value));

                //now replace the function decl in the overload group
                OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(Global().universal.getVarDef(fde->name));
                assert(oGroup && "function decl not in group");
                oGroup->functions.remove(fde);
                oGroup->functions.push_back(ide.get());

                ae->parent->replaceChild(ae, move(ide));
                return;
            }
        }

        Ptr conts = Ptr(new ExprStmt(ae->detachChildB()));

        //add decl exprs generated earlier to the AST
        for (auto it : fde->funcScope->varDefs)
        {
            conts = Ptr(new StmtPair(Ptr(new ExprStmt(Ptr(it.second))), move(conts)));
        }

        //create and insert function definition
        auto fd = MkNPtr(new FunctionDef(fde->Type(), move(conts)));
        //if (fde->name == Global().reserved.main
        //    && fde->Type().compare(typ::mgr.makeList(Global().reserved.string_t))
        //        == typ::TypeCompareResult::valid)
            //mod->entryPt = fd; //just reassign it if it's defined twice, there will already be an error

        //find expression in tail position
        Node0* end = findEndExpr(fd->getChildA());
        if (end) //if it's really there
        {
            //we know its an expr stmt
            ExprStmt* endParent = exact_cast<ExprStmt*>(end->parent);
            Ptr impliedRet = Ptr(new ReturnStmt(end->detachSelf()));
            endParent->parent->replaceChild(endParent, move(impliedRet)); //put in the implied return
        }

        ae->setChildB(move(fd));
    });

    //after we do this, anything under root is global initialization code
    //so make the __init function for it
    //TODO: does this run at compile time or run time?
    //typ::Type void_void = typ::mgr.makeFunc(typ::null, typ::mgr.makeTuple()); //empty tuple
    //FuncDeclExpr* fde = new FuncDeclExpr(Global().reserved.init, void_void, &(mod->global), mod->treeHead->loc);
    //FunctionDef* init = new FunctionDef(fde, mod->treeHead); //stick everything in there
    //pkg->appendChild(init);
    //mod->treeHead = pkg;

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
        Node0* n;
        for (n = ie; exact_cast<ExprStmt*>(n) == 0; n = n->parent)
            assert(n != 0 && "ExprStmt not found");

        ExprStmt* es = exact_cast<ExprStmt*>(n);
        ImpliedLoopStmt* il = exact_cast<ImpliedLoopStmt*>(es->parent);
        
        if (!il)
        {
            Node0* esParent = es->parent; //save because the c'tor clobbers it
            il = new ImpliedLoopStmt(es->detachSelf());
            esParent->replaceDetachedChild(Ptr(il));
        }
        
        il->targets.push_back(ie);
    });

    //replace variables that represent overloaded functions with the function that they
    //must represent, if there is only one such function
    //this makes function pointers & stuff easier
    AstWalk<VarExpr>([] (VarExpr* ve)
    {
        OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(ve->var);
        if (oGroup == 0)
            return;

        if (oGroup->functions.size() == 1)
            ve->var = oGroup->functions.front();
    });

    //remove null stmts under StmtPairs
    AstWalk<StmtPair>([] (StmtPair* sp)
    {
        if (exact_cast<NullStmt*>(sp->getChildA()) != 0)
            sp->parent->replaceChild(sp, sp->detachChildB());
        else if (exact_cast<NullStmt*>(sp->getChildB()) != 0)
            sp->parent->replaceChild(sp, sp->detachChildA());
    });
    
    //create expr stmts for other things that contain expressions
    DynamicAstWalk<CondStmt>([] (CondStmt* cs)
    {
        Node0* parent = dynamic_cast<Node0*>(cs)->parent;
        Ptr node = dynamic_cast<Node0*>(cs)->detachSelf();

        Ptr expr = cs->getExpr()->detachSelf();
        Ptr temp = Ptr(new TmpExpr(expr.get()));
        node->replaceDetachedChild(move(temp));
        
        Ptr sp = Ptr(new StmtPair(Ptr(new ExprStmt(move(expr))), move(node)));
        parent->replaceDetachedChild(move(sp));
    });
}
