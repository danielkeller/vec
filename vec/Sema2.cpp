#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>

using namespace ast;
using namespace sa;

//Phase two is where the AST is compacted into basic blocks. at some point in the future
//it may also turn it into a CFG (control flow graph), but for now it maintains the
//superstructure of the AST
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
        AnyAstWalker(es, [&curBB] (Node0* ex)
        {
            if (!ex->isExpr() || exact_cast<TmpExpr*>(ex))
                return; //only expressions, don't make a temp for a temp

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
                    NullExpr* ne = new NullExpr(std::move(ex->loc));
                    te = new TmpExpr(ne);
                    curBB->appendChild(ne);
                }
            }
            else //create temporary "set by" current expression
                te = new TmpExpr(ex);

            //attach temporary to expression's parent in lieu of it
            ex->parent->replaceChild(ex, te);

            //attach expression to the basic block
            curBB->appendChild(ex);
        });
    });

    //replace exprstmts with corresponding basic blocks
    for (auto p : repl)
    {
        //attach basic block in place of expression statement
        p.first->parent->replaceChild(p.first, p.second);

        //the child is an unneeded temp, don't bother to unlink it
        delete p.first;
    }

    //now split basic blocks where blocks occur inside them
    AstWalk<BasicBlock>([this] (BasicBlock* bb)
    {
        while (true)
        {
            auto blkit = std::find_if(bb->Children().begin(),
                                      bb->Children().end(),
                                      [](Node0 *const a){return exact_cast<Block*>(a) != 0;});

            if (blkit == bb->Children().end())
                return;
            
            Node0* blkParent = bb->parent;
            blkParent->replaceChild(bb, *blkit);

            BasicBlock* left;
            Node0* blk_expr;
            BasicBlock* right;

            std::tie(left, blk_expr, right) = bb->split<BasicBlock>(blkit);

            Block* blk = exact_cast<Block*>(blk_expr);
            assert(blk && "block is missing");

            if (left->Children().size())
                blkParent->replaceChild(blk, new StmtPair(left, blk));
            else
                delete left;

            blkParent = blk->parent;

            if (right->Children().size())
                blkParent->replaceChild(blk, new StmtPair(blk, right));
            else
            {
                delete right;
                return;
            }

            bb = right; //keep going!
        }
    });
    
    //now we can just delete all the rest of the blocks
    AstWalk<Block>([] (Block* b)
    {
        b->parent->replaceChild(b, b->getChildA());
        b->nullChildA();
        delete b;
    });

    //the tree of stmt pairs is pretty messed up by now, lets fix it
    ReverseAstWalk<StmtPair>([] (StmtPair* upper)
    {
        //is the left child also a stmt pair?
        StmtPair* lower;

        //this has to be repeated in case lower->getChildA() is also a stmt pair
        while ((lower = dynamic_cast<StmtPair*>(upper->getChildA())) != 0)
        {
            /*
                 ===                ===
                  / \                | \
                 === c      ===\     a  ===
                  /|        ===/         | \
                 a b                     b  c
            */

            upper->setChildA(lower->getChildA());
            lower->setChildA(lower->getChildB());
            lower->setChildB(upper->getChildB());
            upper->setChildB(lower);
            //whew!
        }
    });

    //combine adjacent BasicBlocks
    AstWalk<StmtPair>([this] (StmtPair* sp)
    {
        BasicBlock* lhs = dynamic_cast<BasicBlock*>(sp->getChildA());
        if (lhs == 0)
            return;

        StmtPair* sp2 = dynamic_cast<StmtPair*>(sp->getChildB());
        BasicBlock* rhs = dynamic_cast<BasicBlock*>(sp->getChildB());

        if (sp2 != 0) //rhs must be 0, block could be down the right subtree
            rhs = dynamic_cast<BasicBlock*>(sp2->getChildA());

        if (rhs == 0)
            return; //no other block

        lhs->consume(rhs);
        
        if (sp2)
        {
            //reattach what's down the tree
            Node0* other = sp2->getChildB();
            sp2->nullChildB();
            sp2->nullChildA();
            delete sp2;
            sp->setChildB(other); //reattach it
        }
        else
        {
            //only one stmt, don't need stmt pair
            sp->parent->replaceChild(sp, lhs);
            sp->nullChildA();
            sp->nullChildB();
            delete sp;
        }
    });
}
