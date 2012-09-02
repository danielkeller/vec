#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"

#include <cassert>

using namespace ast;
using namespace sa;

//Phase one is for insertion / modification of nodes in a way which
//generally preserves the structure of the AST
void Sema::Phase1()
{
    //TODO: insert ScopeEntryExpr and ScopeExitExpr or somesuch

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
                OverloadGroupDeclExpr* oGroup = assert_cast<OverloadGroupDeclExpr*>(
                    Global().universal.getVarDef(fde->name),
                    "function decl not in group");

                oGroup->functions.remove(fde);
                oGroup->functions.push_back(ide.get());

                ae->parent->replaceChild(ae, move(ide));
                continue;
            }
        }

        //exprStmt is needed so ` doesn't escape function decls
        Ptr conts = Ptr(new ExprStmt(ae->detachChildB()));

        //add decl exprs generated earlier to the AST
        for (auto it : fde->funcScope->varDefs)
        {
            conts = Ptr(new StmtPair(Ptr(it.second), move(conts)));
        }

        //create and insert function definition
        auto fd = MkNPtr(new FunctionDef(fde->Type(), move(conts)));

        ae->setChildB(move(fd));
    }

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

    //eliminate blocks and ExprStmts
    //this needs to happen after loop point addition so (x;) works
    for (auto es : Subtree<ExprStmt>(mod).cached())
            es->parent->replaceChild(es, es->detachChildA());

    for (auto b : Subtree<Block>(mod).cached())
            b->parent->replaceChild(b, b->detachChildA());

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

    //TODO: it might be worthwhile, after each stage of sema to validate the tree and check for
    //null nodes. if there are any we can say 'could not recover from previous errors' and
    //bail out. or we could check each time the iterator goes through. that might actually be a
    //valid use case for exceptions...
}
