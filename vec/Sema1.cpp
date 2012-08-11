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

    //TODO: insert ScopeEntryExpr and ScopeExitExpr or somesuch

    //eliminate () -> ExprStmt -> Expr form
    //this needs to happen before loop point addition so regular ()s don't interfere
    for (auto b : Subtree<Block>(mod))
    {
        ExprStmt* es = exact_cast<ExprStmt*>(b->getChildA());
        if (es)
        {
            b->parent->replaceChild(b, es->detachChildA());
        }
    }

    //flatten out listify and tuplify expressions
    auto ifyFlatten = [] (NodeN* ify)
    {
        BinExpr* comma = exact_cast<BinExpr*>(ify->getChild(ify->Children().size() - 1));
        while (comma != nullptr && comma->op == tok::comma)
        {
            auto commaSave = comma->detachSelf(); //don't delete the subtree (yet)
            ify->popChild();
            ify->appendChild(comma->detachChildA()); //add the left child
            ify->appendChild(comma->detachChildB()); //add the (potetially comma) right child
        }
    };

    for (auto ify : Subtree<ListifyExpr>(mod))
        ifyFlatten(ify);
    for (auto ify : Subtree<TuplifyExpr>(mod))
        ifyFlatten(ify);


    //push flattened tuplify exprs into func calls
    for (auto call : Subtree<OverloadCallExpr>(mod))
    {
        auto args = call->detachChildAs<TuplifyExpr>(0);
        if (!args)
            continue;
        call->popChild();
        call->swap(args.get());
    }

    //Package* pkg = new Package();

    int intrin_num = 0; //because of the way this works, all intrins need to be in one file

    //FIXME: memory leak when functions are declared & not defined
    for (auto ae : Subtree<AssignExpr>(mod).cached())
    {
        FuncDeclExpr* fde = exact_cast<FuncDeclExpr*>(ae->getChildA());
        //TODO: or varexpr?
        if (!fde)
            continue;

        //insert intrinsic declaration if that's what this is
        if (VarExpr* ve = exact_cast<VarExpr*>(ae->getChildB()))
        {
            if (ve->var == Global().reserved.intrin_v)
            {
                for (auto it : fde->funcScope->varDefs)
                    Ptr(it.second); //this is kind of dumb but it doesn't access the d'tor directly

                auto ide = MkNPtr(new IntrinDeclExpr(fde, intrin_num));
                ++intrin_num;

                //now replace the function decl in the overload group
                OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(Global().universal.getVarDef(fde->name));
                assert(oGroup && "function decl not in group");
                oGroup->functions.remove(fde);
                oGroup->functions.push_back(ide.get());

                ae->parent->replaceChild(ae, move(ide));
                continue;
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
    }

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
    //FIXME: putting ` in an if condition probably has unexpected results
    for (auto ie : Subtree<IterExpr>(mod))
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
    }

    //replace variables that represent overloaded functions with the function that they
    //must represent, if there is only one such function
    //this makes function pointers & stuff easier
    //TODO: won't this break things?
    for (auto ve : Subtree<VarExpr>(mod))
    {
        OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(ve->var);
        if (oGroup == 0)
            continue;

        if (oGroup->functions.size() == 1)
            ve->var = oGroup->functions.front();
    }

    //remove null stmts under StmtPairs
    for (auto sp : Subtree<StmtPair>(mod).cached())
    {
        if (exact_cast<NullStmt*>(sp->getChildA()) != 0)
            sp->parent->replaceChild(sp, sp->detachChildB());
        else if (exact_cast<NullStmt*>(sp->getChildB()) != 0)
            sp->parent->replaceChild(sp, sp->detachChildA());
    }
    
    //create expr stmts for other things that contain expressions
    for (auto cs : Subtree<CondStmt, Cast::Dynamic>(mod))
    {
        Node0* parent = dynamic_cast<Node0*>(cs)->parent;
        Ptr node = dynamic_cast<Node0*>(cs)->detachSelf();

        Ptr expr = cs->getExpr()->detachSelf();
        Ptr temp = Ptr(new TmpExpr(expr.get()));
        node->replaceDetachedChild(move(temp));
        
        Ptr sp = Ptr(new StmtPair(Ptr(new ExprStmt(move(expr))), move(node)));
        parent->replaceDetachedChild(move(sp));
    }

    //TODO: it might be worthwhile, after each stage of sema to validate the tree and check for
    //null nodes. if there are any we can say 'could not recover from previous errors' and
    //bail out. or we could check each time the iterator goes through. that might actually be a
    //valid use case for exceptions...
}
