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
        ExprStmt(Ptr conts)
            : Node1(move(conts))
        {};
        std::string myLbl() {return "Expr";}
        const char *myColor() {return "2";}
        bool isExpr() {return false;}
    };

    struct StmtPair : public Node2
    {
        StmtPair(Ptr lhs, Ptr rhs) : Node2(move(lhs), move(rhs)) {};
        std::string myLbl() {return ";";}
        const char *myColor() {return "3";}
        bool isExpr() {return false;}
        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << ":p0" <<  " -> n" << static_cast<Node0*>(getChildA()) << ";\n"
               << 'n' << static_cast<Node0*>(this) << ":p1" <<  " -> n" << static_cast<Node0*>(getChildB()) << ";\n";
            nodeDot(getChildA(), os);
            nodeDot(getChildB(), os);
            os << 'n' << static_cast<Node0*>(this)
               << " [label=\";|<p0>|<p1>\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }
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
        IfStmt(Ptr pred, Ptr act, tok::Token &o)
            : Node2(move(pred), move(act))
        {
            loc = o.loc + getChildB()->loc;
        }
        std::string myLbl() {return "if";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };

    struct IfElseStmt : public CondStmt, public Node3
    {
        IfElseStmt(Ptr pred, Ptr act1, Ptr act2, tok::Token &o)
            : Node3(move(pred), move(act1), move(act2))
        {
            loc = o.loc + getChildC()->loc;
        }
        std::string myLbl() {return "if-else";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };

    struct SwitchStmt : public CondStmt, public Node2
    {
        SwitchStmt(Ptr pred, Ptr act, tok::Token &o)
            : Node2(move(pred), move(act))
        {
            loc = o.loc + getChildB()->loc;
        }
        std::string myLbl() {return "switch";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };

    struct WhileStmt : public CondStmt, public Node2
    {
        WhileStmt(Ptr pred, Ptr act, tok::Token &o)
            : Node2(move(pred), move(act))
        {
            loc = o.loc + getChildB()->loc;
        }
        std::string myLbl() {return "while";}
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };

    struct ReturnStmt : public CondStmt, public Node1
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
        Node0* getExpr() {return getChildA();}
        bool isExpr() {return false;}
    };
}

#endif
