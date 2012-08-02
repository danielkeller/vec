#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>

using namespace ast;
using namespace sa;

//Phase two is where the AST is compacted into basic blocks. at some point in the future
//it may also turn it into a CFG (control flow graph), but for now it maintains the
//superstructure of the AST. This is basically to support arbitrary nested statements.
void Sema::Phase2()
{
    //insert temporaries above all expressions
    //because of the way AstWalker works we don't have to worry about
    //it finding the TmpExprs we create

    //populate the basic blocks with each expresion after creating a temporary for its result
    //when we reach the end of an expression end the basic block
    //and start a new one. closure is used to save state (current BB) between calls
    std::map<ExprStmt*, Ptr> repl;
    AstWalk<ExprStmt>([&repl] (ExprStmt* es)
    {
        auto curBB = MkNPtr(new BasicBlock());

        //start looking under current expr stmt, so we don't mix our expressions
        AnyAstWalker(es, [&curBB] (Node0* ex)
        {
            if (!ex->isExpr() || exact_cast<TmpExpr*>(ex) || exact_cast<NullExpr*>(ex))
                return; //only expressions, don't make a temp for a temp or null

            TmpExpr* te;
            
            if (Block* b = exact_cast<Block*>(ex)) //if it's a block, point to correct expr
            {
                TmpExpr* end = exact_cast<TmpExpr*>(findEndExpr(b->getChildA()));
                if (end)
                {
                    //conveniently, the temp expr is already there
                    te = new TmpExpr(end->setBy); //copy it so it doesn't get deleted
                }
                else //it's a stmt of some sort (i think this isn't really needed)
                {
                    NullExpr* ne = new NullExpr(ex->loc);
                    te = new TmpExpr(ne);
                    curBB->appendChild(Ptr(ne));
                }
            }
            else //create temporary "set by" current expression
                te = new TmpExpr(ex);

            //attach temporary to expression's parent in lieu of it
            Ptr expr = ex->parent->replaceChild(ex, Ptr(te));

            //attach expression to the basic block
            curBB->appendChild(move(expr));
        });

        repl[es] = move(curBB); //attach it
    });

    //replace exprstmts with corresponding basic blocks
    for (auto& p : repl)
    {
        //attach basic block in place of expression statement
        p.first->parent->replaceChild(p.first, move(p.second));
    }

    //now split basic blocks where blocks occur inside them
    AstWalk<BasicBlock>([this] (BasicBlock* bb)
    {
        while (bb)
        {
            auto blkit = bb->Children().cbegin();

            for (; blkit != bb->Children().cend(); ++blkit)
                if (exact_cast<Block*>(blkit->get()) != 0)
                    break;

            if (blkit == bb->Children().end())
                return;
            
            Node0* bbParent = bb->parent;

            NPtr<BasicBlock>::type left;
            Ptr blk;
            NPtr<BasicBlock>::type right;

            std::tie(left, blk, right) = bb->split<BasicBlock>(blkit);
            //after split bb is empty but attached
            bb->detachSelf();

            if (left->Children().size())
                blk = Ptr(new StmtPair(move(left), move(blk)));

            if (right->Children().size())
            {
                bb = right.get(); //keep going!
                blk = Ptr(new StmtPair(move(blk), move(right)));
            }
            else
                bb = nullptr;

            bbParent->replaceDetachedChild(move(blk));
        }
    });
    
    //now we can just delete all the rest of the blocks
    AstWalk<Block>([this] (Block* b)
    {
        b->parent->replaceChild(b, b->detachChildA());
        validateTree();
    });

    validateTree();

    //the tree of stmt pairs is pretty messed up by now, lets fix it
    ReverseAstWalk<StmtPair>([] (StmtPair* upper)
    {
        //is the left child also a stmt pair?
        //this has to be repeated in case lower->getChildA() is also a stmt pair
        while (auto lower = upper->detachChildAAs<StmtPair>())
        {
            /*
                 ===                ===
                  / \                | \
                 === c      ===\     a  ===
                  /|        ===/         | \
                 a b                     b  c
            */

            upper->setChildA(lower->detachChildA());
            lower->setChildA(lower->detachChildB());
            lower->setChildB(upper->detachChildB());
            upper->setChildB(move(lower));
            //whew!
        }
    });

    //combine adjacent BasicBlocks
    AstWalk<StmtPair>([this] (StmtPair* upper)
    {
        auto lhs = upper->detachChildAAs<BasicBlock>();
        if (!lhs)
            return;

        auto lower = upper->detachChildBAs<StmtPair>();

        if (lower) //block could be down the right subtree
        {
            auto rhs = lower->detachChildAAs<BasicBlock>();
            if (rhs)
            {
                lhs->consume(move(rhs));
                upper->setChildB(lower->detachChildB());
                upper->setChildA(move(lhs));
                return;
            }

            upper->setChildB(move(lower));
        }
        else
        {
            auto rhs = upper->detachChildBAs<BasicBlock>();
            if (rhs)
            {
                lhs->consume(move(rhs));
                upper->parent->replaceChild(upper, move(lhs));
                return;
            }
        }
        
        upper->setChildA(move(lhs));
    });
}
