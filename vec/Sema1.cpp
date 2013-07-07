#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"

#include <cassert>
#include <set>

using namespace ast;
using namespace sa;

//Phase one is for insertion / modification of nodes in a way which
//generally preserves the structure of the AST

//flatten out listify and tuplify expressions
void ifyFlatten (NodeN* ify)
{
    auto comma = ify->detachChildAs<OverloadCallExpr>(ify->Children().size() - 1);

    while (comma && comma->fun->Op() == tok::comma)
    {
        ify->popChild();
        ify->appendChild(comma->detachChild(0)); //add the left child
        ify->appendChild(comma->detachChild(1)); //add the (potetially comma) right child

        comma = ify->detachChildAs<OverloadCallExpr>(ify->Children().size() - 1);
    }

    //replace the non-comma
    ify->setChild(ify->Children().size() - 1, move(comma));
}

bool isValIgnored(Node0* val)
{
    if (StmtPair* p = exact_cast<StmtPair*>(val->parent))
        if (p->getChildA() == val)
            return true;
    if (BranchStmt* br = exact_cast<BranchStmt*>(val->parent))
    {
        if (br->ifFalse != nullptr && br->ifTrue != nullptr)
            return true;
        else
            return isValIgnored(br->parent);
    }
    return false;
}

void Sema::Phase1()
{
    //TODO: insert ScopeEntryExpr and ScopeExitExpr or somesuch

    for (auto opAs : Subtree<OpAssignExpr>(mod).cached())
    {
        // a += b
        // becomes
        // tmp = leval(a)
        // *tmp = *tmp + eval(b)

        tok::Token op;
        op.type = opAs->assignOp;
        op.loc = opAs->opLoc;

        Ptr tmp = Ptr(new TmpExpr(opAs->getChildA())); //first temp
        Ptr call = Ptr(new OverloadCallExpr(op, opAs->sco, move(tmp), opAs->detachChildB()));

        tmp = Ptr(new TmpExpr(opAs->getChildA())); //second temp
        Ptr asgn = Ptr(new AssignExpr(move(tmp), move(call), op.loc));
        Ptr seq = Ptr(new StmtPair(opAs->detachChildA(), move(asgn)));
        opAs->parent->replaceChild(opAs, move(seq));
    }

    for (auto ify : Subtree<ListifyExpr>(mod))
        ifyFlatten(ify);
    for (auto ify : Subtree<TuplifyExpr>(mod))
        ifyFlatten(ify);

    //now throw away any other commas
    for (auto comma : Subtree<OverloadCallExpr>(mod).cached())
    {
        if (comma->fun->Op() != tok::comma)
            continue;
        Ptr pair = Ptr(new StmtPair(comma->detachChild(0), comma->detachChild(1)));
        comma->parent->replaceChild(comma, move(pair));
    }

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

    for (auto ae : Subtree<AssignExpr>(mod).cached())
    {
        DeclExpr* fde = exact_cast<DeclExpr*>(ae->getChildA());
        //TODO: or varexpr?
        if (!fde)
            continue;

        //insert intrinsic declaration if that's what this is
        if (VarExpr* ve = exact_cast<VarExpr*>(ae->getChildB()))
        {
            if (ve->Name() == Global().reserved.intrin)
            {
                //defining the same intrinsic twice causes a crash on this line
                //not a huge deal, but not ideal
                auto ide = MkNPtr(new IntrinDeclExpr(fde, intrin_num));
                ++intrin_num;

                ae->parent->replaceChild(ae, move(ide));
                continue;
            }
        }
        //TODO: insert lambda when function is defined "pointfree"
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
    //FIXME: do this after import
    /*
    for (auto ve : Subtree<VarExpr>(mod))
    {
        OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(ve->var);
        if (oGroup == 0)
            continue;

        if (oGroup->functions.size() == 1)
            ve->var = oGroup->functions.front();
    }
    */

    //convert control statements into their branch forms

    for (auto If : Subtree<IfStmt>(mod).cached())
    {
        auto rho = MkNPtr(new RhoStmt());
        
        auto controlled = MkNPtr(new BranchStmt(If->detachChildB()));

        Ptr condPtr = If->detachChildA();
        rho->appendChild(Ptr(new BranchStmt(move(condPtr), controlled.get(), nullptr)));
        rho->appendChild(move(controlled));
        If->parent->replaceChild(If, move(rho));
    }

    for (auto IfElse : Subtree<IfElseStmt>(mod).cached())
    {
        auto rho = MkNPtr(new RhoStmt());
        
        auto controlled1 = MkNPtr(new BranchStmt(IfElse->detachChildB()));
        auto controlled2 = MkNPtr(new BranchStmt(IfElse->detachChildC()));

        Ptr condPtr = IfElse->detachChildA();
        rho->appendChild(Ptr(new BranchStmt(move(condPtr), controlled1.get(), controlled2.get())));
        rho->appendChild(move(controlled1));
        rho->appendChild(move(controlled2));
        IfElse->parent->replaceChild(IfElse, move(rho));
    }

    for (auto scOp : Subtree<OverloadCallExpr>(mod).cached())
    {
        if (scOp->fun->Op() != tok::barbar && scOp->fun->Op() != tok::ampamp)
            continue;
        
        auto rho = MkNPtr(new RhoStmt());
        auto controlled2 = MkNPtr(new BranchStmt(scOp->detachChild(1)));
        auto controlled1 = MkNPtr(new BranchStmt(scOp->detachChild(0),
            //for and, we eval the rhs if the lhs is true
        /*ifTrue  = */ scOp->fun->Op() == tok::ampamp ? controlled2.get() : nullptr,
            //for or, we eval the rhs if the lhs is false
        /*ifFalse = */ scOp->fun->Op() == tok::barbar ? controlled2.get() : nullptr
            ));

        auto phi = MkNPtr(new PhiExpr(scOp->loc));
        phi->inputs.push_back(controlled1.get());
        phi->inputs.push_back(controlled2.get());

        rho->appendChild(move(controlled1));
        rho->appendChild(move(controlled2));

        scOp->parent->replaceChild(scOp, Ptr(new StmtPair(move(rho), move(phi))));
    }
    //TODO: the rest of them

    //merge all rho stmts into one, underneath the function def (or compilation unit)
    auto globalRho = MkNPtr(new RhoStmt());
    globalRho->appendChild(Ptr(new BranchStmt(mod->detachChildA())));
    mod->setChildA(move(globalRho));

    for (auto fd : Subtree<Lambda>(mod))
    {
        auto funcRho = MkNPtr(new RhoStmt());
        funcRho->appendChild(Ptr(new BranchStmt(fd->detachChildA())));
        fd->setChildA(move(funcRho));
    }

    for (auto rho : Subtree<RhoStmt>(mod).cached())
    {
        //exclude the ones we just added
        if (exact_cast<Lambda*>(rho->parent) || exact_cast<Module*>(rho->parent))
            continue;

        BranchStmt* entry = exact_cast<BranchStmt*>(rho->getChild(0));
        Ptr before;

        //climb up the tree until we find the basic block we're in
        std::unordered_set<Node0*> parents; //all nodes directly between us and the terminator
        Node0* terminator;
        for (terminator = rho->parent;
            !exact_cast<BranchStmt*>(terminator);
            terminator = terminator->parent)
            parents.insert(terminator);

        BranchStmt* exit = exact_cast<BranchStmt*>(terminator);

        //accumulate all nodes that execute before us into before
        //this is every node that is a descendent of one of the parents nodes, and
        //which is seen first in a postorder traversal
        for (auto node : Subtree<>(exit).cached())
        {
            //the last thing that must execute before this control structure
            if (node == rho->getChild(0))
                break;

            //if its value is used by something that happens before
            if (!parents.count(node->parent))
                continue; //ignore it so we can take its parent

            Ptr tmp;
            //don't do this for nodes that ignore this child's value
            if (isValIgnored(node))
                tmp = Ptr(new NullExpr("Temp Ignored"));
            else
                tmp = Ptr(new TmpExpr(node));

            //it is neccesary to maintain the same value for the code under the
            //branch, becuse it is used as the condition
            if (before)
                before = Ptr(new StmtPair(
                    move(before), //get node and put temp where it was
                    node->parent->replaceChild(node, move(tmp))));
            else
                before = node->parent->replaceChild(node, move(tmp));
        }

        //now put the basic block's original contents at the end
        if (before)
            entry->setChildA(Ptr(new StmtPair(move(before), entry->detachChildA())));

        RhoStmt* oldOwner = exact_cast<RhoStmt*>(exit->parent);

        //point upper branches to our first branch
        for (auto& upperBrPtr : oldOwner->Children())
        {
            BranchStmt* upperBr = exact_cast<BranchStmt*>(upperBrPtr.get());
            if (upperBr->ifTrue == exit)
                upperBr->ifTrue = entry;
            if (upperBr->ifFalse == exit)
                upperBr->ifFalse = entry;
        }

        //point our branches that leave the rhoStmt to the "after" branch.
        //also, collect these branches, as they define rho's value
        std::set<BranchStmt*> ends;
        for (auto& lowerBrPtr : rho->Children())
        {
            BranchStmt* lowerBr = exact_cast<BranchStmt*>(lowerBrPtr.get());

            if (lowerBr->ifTrue == nullptr
                || lowerBr->ifFalse == nullptr)
                ends.insert(lowerBr);

            if (lowerBr->ifTrue == nullptr)
                lowerBr->ifTrue = exit;
            if (lowerBr->ifFalse == nullptr)
                lowerBr->ifFalse = exit;
        }
        
        auto newOwner = MkNPtr(new RhoStmt());
        
        Node0* rhoParent = rho->parent;
        bool rhoIgnored = isValIgnored(rho); //this doesn't work if it's detached
        auto rhoPtr = rho->detachSelfAs<RhoStmt>();
        //now take care of rho's value
        if (rhoIgnored)
            rhoParent->replaceDetachedChild(Ptr(new NullExpr("Rho Ignored")));
        else
        {
            auto phi = MkNPtr(new PhiExpr(rho->loc));
            for (auto br : ends)
                phi->inputs.push_back(br);
            rhoParent->replaceDetachedChild(move(phi));
        }

        for (auto& it : oldOwner->Children())
        {
            if (it.get() == exit) //push our stuff on first
                newOwner->consume(move(rhoPtr));

            newOwner->appendChild(it->detachSelf());
        }

        //this looks stupid, but its actually so we don't delete anything other than rho
        oldOwner->clear();
        oldOwner->consume(move(newOwner));
    }

    //now any Branch stmts that branch to null must leave the function
    for (auto br : Subtree<BranchStmt>(mod))
    {
        if (br->ifTrue)
            continue;

        br->setChildA(Ptr(new ReturnStmt(br->detachChildA())));
    }

    //remove null stmts under StmtPairs
    for (auto sp : Subtree<StmtPair>(mod).cached())
    {
        if (exact_cast<NullStmt*>(sp->getChildA()) != 0
         || exact_cast<NullExpr*>(sp->getChildA()) != 0)
            sp->parent->replaceChild(sp, sp->detachChildB());
        /* //this changes the semantics!
        else if (exact_cast<NullStmt*>(sp->getChildB()) != 0
              || exact_cast<NullExpr*>(sp->getChildB()) != 0)
            sp->parent->replaceChild(sp, sp->detachChildA());*/
    }

    //TODO: it might be worthwhile, after each stage of sema to validate the tree and check for
    //null nodes. if there are any we can say 'could not recover from previous errors' and
    //bail out. or we could check each time the iterator goes through. that might actually be a
    //valid use case for exceptions...
}
