#ifndef STMT_H
#define STMT_H

#include "Expr.h"

#include <algorithm>

namespace ast
{
    struct NullStmt : public Node0
    {
        NullStmt(tok::Location const &l) : Node0(l) {};
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";}
    };

    struct ExprStmt : public Node1
    {
        ExprStmt(Ptr conts)
            : Node1(move(conts))
        {};
        std::string myLbl() {return "Expr";}
        const char *myColor() {return "2";}
    };

    struct StmtPair : public Node2
    {
        StmtPair(Ptr lhs, Ptr rhs) : Node2(move(lhs), move(rhs)) {};
        std::string myLbl() {return ";";}
        const char *myColor() {return "3";}
        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << ":p0" <<  " -> n" << static_cast<Node0*>(getChildA()) << ";\n"
               << 'n' << static_cast<Node0*>(this) << ":p1" <<  " -> n" << static_cast<Node0*>(getChildB()) << ";\n";
            nodeDot(getChildA(), os);
            nodeDot(getChildB(), os);
            os << 'n' << static_cast<Node0*>(this)
               << " [label=\";|<p0>|<p1>\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen& gen);
        annot_t& Annot() {return getChildB()->Annot();}
    };

    struct Block : public Node1
    {
        //needed for scope entry&exit points
        NormalScope *scope;
        Block(Ptr conts, NormalScope *s, tok::Location const &l)
            : Node1(move(conts), l),
            scope(s)
        {};
        std::string myLbl() {return "(...)";}
        const char *myColor() {return "3";}
    };

    struct IfStmt : public Node2
    {
        IfStmt(Ptr pred, Ptr act, tok::Token &o)
            : Node2(move(pred), move(act))
        {
            loc = o.loc + getChildB()->loc;
        }
        std::string myLbl() {return "if";}
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen& gen);
    };

    struct IfElseStmt : public Node3
    {
        IfElseStmt(Ptr pred, Ptr act1, Ptr act2, tok::Token &o)
            : Node3(move(pred), move(act1), move(act2))
        {
            loc = o.loc + getChildC()->loc;
        }
        std::string myLbl() {return "if-else";}
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen& gen);
    };

    struct SwitchStmt : public Node2
    {
        SwitchStmt(Ptr pred, Ptr act, tok::Token &o)
            : Node2(move(pred), move(act))
        {
            loc = o.loc + getChildB()->loc;
        }
        std::string myLbl() {return "switch";}
    };

    struct WhileStmt : public Node2
    {
        WhileStmt(Ptr pred, Ptr act, tok::Token &o)
            : Node2(move(pred), move(act))
        {
            loc = o.loc + getChildB()->loc;
        }
        std::string myLbl() {return "while";}
    };

    struct ReturnStmt : public Node1
    {
        ReturnStmt(Ptr arg, tok::Token &o)
            : Node1(move(arg))
        {
            loc = o.loc + getChildA()->loc;
        }
        ReturnStmt(Ptr arg)
            : Node1(move(arg))
        {}
        std::string myLbl() {return "return";}
        annot_t& Annot() {return getChildA()->Annot();}
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen& gen);
    };
}

#endif
