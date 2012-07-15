#ifndef SEMANODES_H
#define SEMANODES_H

#include "Stmt.h"

namespace ast
{
    //This file is for AST nodes that are added during semantic analysis, rather than parsing
    struct TmpExpr : public Node0
    {
        const char *myColor() {return "8";};
        std::string myLbl () {return "tmp";};
        Node0* setBy;

        TmpExpr(Node0* sb) : Node0(sb->loc), setBy(sb){};

        void emitDot(std::ostream& os)
        {
            Node0::emitDot(os);
            //shows a "sets" relationship
            os << 'n' << static_cast<Node0*>(setBy) << " -> n" << static_cast<Node0*>(this)
                << " [style=dotted];\n";
        };

        typ::Type& Type() {return setBy->Type();}
    };

    struct ImpliedLoopStmt : public Node1
    {
        std::vector<IterExpr*> targets;
        ImpliedLoopStmt(ExprStmt* arg)
            : Node1(arg)
        {};
        std::string myLbl() {return "for (`)";}
        bool isExpr() {return false;}

        void emitDot(std::ostream& os)
        {
            Node1::emitDot(os);
            for (auto n : targets)
                os << 'n' << static_cast<Node0*>(this) << " -> n" << static_cast<Node0*>(n)
                    << " [style=dotted];\n";
        };
    };

    //do it this way, or with inline assembly?
    struct IntrinDeclExpr : public FuncDeclExpr
    {
        int intrin_id;
        IntrinDeclExpr(FuncDeclExpr* func, int id)
            : FuncDeclExpr(*func), intrin_id(id) {}
        const char *myColor() {return "8";};
    };

    struct FunctionDef : public Node1
    {
        FunctionDef(typ::Type t, Node0* s) : Node1(s, s->loc) {Type() = t;}
        std::string myLbl() {return Type().to_str();}
        bool isExpr() {return false;}
    };

    //basic block class that holds any number of instructions
    struct BasicBlock : public NodeN
    {
        std::string myLbl() {return "Block";}
        bool isExpr() {return false;}
    };

    struct Package : public NodeN
    {
        std::string myLbl() {return "Package";}
        bool isExpr() {return false;}
    };
}

#endif
