#ifndef STMT_H
#define STMT_H

#include "Expr.h"

#include <algorithm>

namespace ast
{
    //abstract statement type
    struct Stmt : public virtual AstNodeB
    {
        tok::Location loc;
        virtual bool isLval() {return false;};
		Stmt() : loc() {};
        Stmt(tok::Location &&l) : loc(l) {};
        Stmt(tok::Location &l) : loc(l) {};
        const char *myColor() {return "1";};
    };

    struct NullStmt : public Stmt, public AstNode0
    {
        NullStmt(tok::Location &&l) : Stmt(l) {};
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";};
    };

    struct ExprStmt : public Stmt, public AstNode1<Expr>
    {
        ExprStmt(Expr* conts)
            : Stmt(conts->loc),
            AstNode1<Expr>(conts)
        {};
        std::string myLbl() {return "Expr";}
        const char *myColor() {return "2";};
    };

    struct StmtPair : public Stmt, public AstNode2<Stmt, Stmt>
    {
        StmtPair(Stmt *lhs, Stmt *rhs) : AstNode2<Stmt, Stmt>(lhs, rhs) {};
        std::string myLbl() {return ";";}
        const char *myColor() {return "3";};
        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<AstNodeB*>(this) << ":p0" <<  " -> n" << static_cast<AstNodeB*>(getChildA()) << ";\n"
               << 'n' << static_cast<AstNodeB*>(this) << ":p1" <<  " -> n" << static_cast<AstNodeB*>(getChildB()) << ";\n";
            getChildA()->emitDot(os);
            getChildB()->emitDot(os);
            os << 'n' << static_cast<AstNodeB*>(this)
               << " [label=\";|<p0>|<p1>\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }
    };

    struct Block : public Expr, public Stmt, public AstNode1<Stmt>
    {
        Scope *scope;
        Block(Stmt* conts, Scope *s, tok::Location &&l)
            : Expr(l),
            Stmt(l),
            AstNode1<Stmt>(conts),
            scope(s)
        {};
        ~Block() {delete scope;};
        std::string myLbl() {return "(...)";}
        const char *myColor() {return "3";};
    };

    //base class for stmts that contain expressions among other things
    struct CondStmt : public Stmt
    {
        virtual Expr* getExpr() = 0;
        CondStmt(tok::Location &&l) : Stmt(l) {};
    };

    struct IfStmt : public CondStmt, public AstNode2<Expr, Stmt>
    {
        IfStmt(Expr* pred, Stmt* act, tok::Token &o)
            : CondStmt(o.loc + act->loc),
            AstNode2<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "if";};
        Expr* getExpr() {return getChildA();};
    };

    struct IfElseStmt : public CondStmt, public AstNode3<Expr, Stmt, Stmt>
    {
        IfElseStmt(Expr* pred, Stmt* act1, Stmt* act2, tok::Token &o)
            : CondStmt(o.loc + act2->loc),
            AstNode3<Expr, Stmt, Stmt>(pred, act1, act2)
        {};
        std::string myLbl() {return "if-else";}
        Expr* getExpr() {return getChildA();};
    };

    struct SwitchStmt : public CondStmt, public AstNode2<Expr, Stmt>
    {
        SwitchStmt(Expr* pred, Stmt* act, tok::Token &o)
            : CondStmt(o.loc + act->loc),
            AstNode2<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "switch";}
        Expr* getExpr() {return getChildA();};
    };

    struct WhileStmt : public CondStmt, public AstNode2<Expr, Stmt>
    {
        WhileStmt(Expr* pred, Stmt* act, tok::Token &o)
            : CondStmt(o.loc + act->loc),
            AstNode2<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "while";}
        Expr* getExpr() {return getChildA();};
    };

    struct ReturnStmt : public CondStmt, public AstNode1<Expr>
    {
        ReturnStmt(Expr* arg, tok::Token &o)
            : CondStmt(o.loc + arg->loc),
            AstNode1<Expr>(arg)
        {};
        std::string myLbl() {return "return";}
        Expr* getExpr() {return getChildA();};
    };
}

#endif
