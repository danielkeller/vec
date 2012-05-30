#ifndef SEMANODES_H
#define SEMANODES_H

#include "Stmt.h"

namespace ast
{
    struct SemaStmt : public Stmt
    {
        const char *myColor() {return "8";};
    };

    struct ImpliedLoopStmt : public SemaStmt, public AstNode<ExprStmt>
    {
        std::vector<IterExpr*> targets;
        ImpliedLoopStmt(ExprStmt* arg)
            : AstNode<ExprStmt>(arg)
        {};
        std::string myLbl() {return "for (`)";}
    };
}

#endif
