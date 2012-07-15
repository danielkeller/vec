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
        bool isExpr() {return false;}
    };

    struct ExprStmt : public Node1
    {
        ExprStmt(Node0* conts)
            : Node1(conts)
        {};
        std::string myLbl() {return "Expr";}
        const char *myColor() {return "2";}
        bool isExpr() {return false;}
    };

    struct StmtPair : public Node2
    {
        StmtPair(Node0 *lhs, Node0 *rhs) : Node2(lhs, rhs) {};
        std::string myLbl() {return ";";}
        const char *myColor() {return "3";}
        bool isExpr() {return false;}
        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << ":p0" <<  " -> n" << static_cast<Node0*>(getChildA()) << ";\n"
               << 'n' << static_cast<Node0*>(this) << ":p1" <<  " -> n" << static_cast<Node0*>(getChildB()) << ";\n";
            getChildA()->emitDot(os);
            getChildB()->emitDot(os);
            os << 'n' << static_cast<Node0*>(this)
               << " [label=\";|<p0>|<p1>\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }
    };

    struct Block : public Node1
    {
        Scope *scope;
        Block(Node0* conts, Scope *s, tok::Location const &l)
            : Node1(conts, l),
            scope(s)
        {};
        std::string myLbl() {return "(...)";}
        const char *myColor() {return "3";}
        bool isExpr() {return true;}
    };

    //base class for stmts that contain expressions among other things
    struct CondStmt
    {
        virtual Node0* getExpr() = 0;
        CondStmt() {};
    };

    struct IfStmt : public CondStmt, public Node2
    {
        IfStmt(Node0* pred, Node0* act, tok::Token &o)
            : Node2(pred, act, o.loc + act->loc)
        {}
        std::string myLbl() {return "if";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };

    struct IfElseStmt : public CondStmt, public Node3
    {
        IfElseStmt(Node0* pred, Node0* act1, Node0* act2, tok::Token &o)
            : Node3(pred, act1, act2, o.loc + act2->loc)
        {};
        std::string myLbl() {return "if-else";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };

    struct SwitchStmt : public CondStmt, public Node2
    {
        SwitchStmt(Node0* pred, Node0* act, tok::Token &o)
            : Node2(pred, act, o.loc + act->loc)
        {};
        std::string myLbl() {return "switch";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };

    struct WhileStmt : public CondStmt, public Node2
    {
        WhileStmt(Node0* pred, Node0* act, tok::Token &o)
            : Node2(pred, act, o.loc + act->loc)
        {};
        std::string myLbl() {return "while";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };

    struct ReturnStmt : public CondStmt, public Node1
    {
        ReturnStmt(Node0* arg, tok::Token &o)
            : Node1(arg, o.loc + arg->loc)
        {};
        ReturnStmt(Node0* arg)
            : Node1(arg)
        {};
        std::string myLbl() {return "return";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };
}

#endif
