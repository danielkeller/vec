#ifndef STMT_H
#define STMT_H

#include "AstNode.h"
#include "Scope.h"

namespace ast
{
   struct Block : public AstNode<> //AstNode<StmtExpr>
   {
       Scope * scope;

       Block(Scope * s) : scope(s) {}
   };

   struct FuncBody : public AstNode<Block>
   {
       FuncBody(Block *b) : AstNode<Block>(b) {}
   };
}

#endif
