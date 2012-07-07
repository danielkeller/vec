#ifndef SEMANODES_H
#define SEMANODES_H

#include "Stmt.h"

namespace ast
{
    //This file is for AST nodes that are added during semantic analysis, rather than parsing
    struct SemaStmt : public Stmt
    {
        const char *myColor() {return "8";};
    };

    struct TmpExpr : public Expr, public AstNode0
    {
        const char *myColor() {return "8";};
        std::string myLbl () {return "tmp";};
        Expr* setBy;

        TmpExpr(Expr* sb) : Expr(sb->loc), setBy(sb){};

        void emitDot(std::ostream& os)
        {
            AstNode0::emitDot(os);
            //shows a "sets" relationship
            os << 'n' << static_cast<AstNodeB*>(setBy) << " -> n" << static_cast<AstNodeB*>(this)
                << " [style=dotted];\n";
        };

        typ::Type& Type() {return setBy->Type();}
    };

    struct ImpliedLoopStmt : public SemaStmt, public AstNode1<Stmt>
    {
        std::vector<IterExpr*> targets;
        ImpliedLoopStmt(ExprStmt* arg)
            : AstNode1<Stmt>(arg)
        {};
        std::string myLbl() {return "for (`)";};

        void emitDot(std::ostream& os)
        {
            AstNode1<Stmt>::emitDot(os);
            for (auto n : targets)
                os << 'n' << static_cast<AstNodeB*>(this) << " -> n" << static_cast<AstNodeB*>(n)
                    << " [style=dotted];\n";
        };
    };

    struct IntrinDeclExpr : public DeclExpr
    {
        int intrin_id;
        IntrinDeclExpr(FuncDeclExpr* func, int id)
            : DeclExpr(func->name, func->Type(), func->owner, func->loc), intrin_id(id) {}
        const char *myColor() {return "8";};
    };

    struct FunctionDef : public Expr, public AstNode1<Stmt>
    {
        FunctionDef(typ::Type t, Stmt* s) : Expr(s->loc), AstNode1<Stmt>(s) {Type() = t;}
        std::string myLbl() {return Type().to_str();}
    };

    //basic block class that holds any number of instructions
    struct BasicBlock : public AstNodeN<Expr>, public Stmt
    {
        std::string myLbl() {return "Block";}
    };

    struct Package : public AstNodeN<FunctionDef>, public Stmt
    {
        std::string myLbl() {return "Package";}
    };
}

#endif
