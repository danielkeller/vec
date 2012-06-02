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

            TmpExpr* te;

            if (Block* b = dynamic_cast<Block*>(ex)) //if it's a block, point to correct expr
            {
                AstNode0* srch = b->getChild<0>();
                while (true)
                {
                    if ((b = dynamic_cast<Block*>(srch)))
                        srch = b->getChild<0>();
                    else if (StmtPair* sp = dynamic_cast<StmtPair*>(srch))
                        srch = sp->getChild<1>();
                    else if (ExprStmt* es = dynamic_cast<ExprStmt*>(srch))
                        srch = es->getChild<0>();
                    else if (TmpExpr* e = dynamic_cast<TmpExpr*>(srch)) //other than block
                    {
                        //conveniently, the temp expr is already there
                        te = new TmpExpr(e->setBy); //copy it so it doesn't get deleted
                        break;
                    }
                    else //it's a stmt of some sort
                    {
                        NullExpr* ne = new NullExpr(std::move(ex->loc));
                        te = new TmpExpr(ne);
                        curBB->appendChild(ne);
                        break;
                    }
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

    std::function<void(ExprStmt*)> blockInsert;

    //replace exprstmts with corresponding basic blocks
    blockInsert = [&repl, &blockInsert] (ExprStmt* es)
    {
        assert (repl.count(es) && "basic block doesn't exist");

        //attach basic block in place of expression statement
        es->parent->replaceChild(es, repl[es]);
        //the child is an unneeded temp, don't bother to unlink it
        delete es;
    
        //recurse in case we just inserted more exprstmts that would be missed
        AstWalker<ExprStmt>(repl[es], blockInsert);
    };

    //call it
    AstWalk<ExprStmt>(blockInsert);

    //now split basic blocks where blocks occur inside them
    AstWalk<BasicBlock>([] (BasicBlock* bb)
    {
        while (true)
        {
            auto blkit = std::find_if(bb->chld.begin(),
                                      bb->chld.end(),
                                      [](AstNode0*& a){return (bool)dynamic_cast<Block*>(a);});

            if (blkit == bb->chld.end())
                return;

            BasicBlock* left = new BasicBlock();
            left->chld.insert(left->chld.begin(), bb->chld.begin(), blkit);
            bb->chld.erase(bb->chld.begin(), blkit);

            //begin is now blkit, blkit is now invalid because of erase
            blkit = bb->chld.begin();

            BasicBlock* right = new BasicBlock();
            right->chld.insert(right->chld.begin(), blkit + 1, bb->chld.end());
            
            Block* blk = dynamic_cast<Block*>(*blkit); //save the block
            bb->parent->replaceChild(bb, blk);
            bb->chld.clear();
            delete bb;

            if (left->chld.size())
                blk->parent->replaceChild(blk, new StmtPair(left, blk));
            else
                delete left;

            if (right->chld.size())
                blk->parent->replaceChild(blk, new StmtPair(blk, right));
            else
            {
                delete right;
                return; //there are no more
            }

            bb = right; //keep going!
        }
    });

    //now we can just delete all the rest of the blocks
    AstWalk<Block>([] (Block* b)
    {
        b->parent->replaceChild(b, b->getChild<0>());
        b->getChild<0>() = 0;
        delete b;
    });
}
