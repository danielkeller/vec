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
        std::string myLbl() {return "for (`)";};

        void emitDot(std::ostream& os)
        {
            AstNode<ExprStmt>::emitDot(os);
            for (auto n : targets)
                os << 'n' << static_cast<AstNode0*>(this) << " -> n" << static_cast<AstNode0*>(n)
                    << " [style=dotted];\n";
        };
    };
}

#endif
