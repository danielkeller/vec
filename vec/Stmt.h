#ifndef STMT_H
#define STMT_H

#include "Expr.h"

#include <algorithm>

namespace ast
{
    //abstract statement type
    struct Stmt : public virtual AstNode0
    {
        tok::Location loc;
        virtual bool isLval() {return false;};
        Stmt() = default;
        Stmt(tok::Location &&l) : loc(l) {};
        Stmt(tok::Location &l) : loc(l) {};
        const char *myColor() {return "1";};
    };

    struct NullStmt : public Stmt, public AstNode<>
    {
        NullStmt(tok::Location &&l) : Stmt(l) {};
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";};
    };

    struct ExprStmt : public Stmt, public AstNode<Expr>
    {
        ExprStmt(Expr* conts)
            : Stmt(conts->loc),
            AstNode<Expr>(conts)
        {};
        std::string myLbl() {return "Expr";}
        const char *myColor() {return "2";};
    };

    struct StmtPair : public Stmt, public AstNode<Stmt, Stmt>
    {
        StmtPair(Stmt *lhs, Stmt *rhs) : AstNode<Stmt, Stmt>(lhs, rhs) {};
        std::string myLbl() {return ";";}
        const char *myColor() {return "3";};
        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<AstNode0*>(this) << ":p0" <<  " -> n" << static_cast<AstNode0*>(getChild<0>()) << ";\n"
               << 'n' << static_cast<AstNode0*>(this) << ":p1" <<  " -> n" << static_cast<AstNode0*>(getChild<1>()) << ";\n";
            getChild<0>()->emitDot(os);
            getChild<1>()->emitDot(os);
            os << 'n' << static_cast<AstNode0*>(this)
               << " [label=\";|<p0>|<p1>\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }
    };

    struct Block : public Expr, public Stmt, public AstNode<Stmt>
    {
        Scope *scope;
        Block(Stmt* conts, Scope *s, tok::Location &&l)
            : Expr(l),
            Stmt(l),
            AstNode<Stmt>(conts),
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

    struct IfStmt : public CondStmt, public AstNode<Expr, Stmt>
    {
        IfStmt(Expr* pred, Stmt* act, tok::Token &o)
            : CondStmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "if";};
        Expr* getExpr() {return getChild<0>();};
    };

    struct IfElseStmt : public CondStmt, public AstNode<Expr, Stmt, Stmt>
    {
        IfElseStmt(Expr* pred, Stmt* act1, Stmt* act2, tok::Token &o)
            : CondStmt(o.loc + act2->loc),
            AstNode<Expr, Stmt, Stmt>(pred, act1, act2)
        {};
        std::string myLbl() {return "if-else";}
        Expr* getExpr() {return getChild<0>();};
    };

    struct SwitchStmt : public CondStmt, public AstNode<Expr, Stmt>
    {
        SwitchStmt(Expr* pred, Stmt* act, tok::Token &o)
            : CondStmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "switch";}
        Expr* getExpr() {return getChild<0>();};
    };

    struct WhileStmt : public CondStmt, public AstNode<Expr, Stmt>
    {
        WhileStmt(Expr* pred, Stmt* act, tok::Token &o)
            : CondStmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "while";}
        Expr* getExpr() {return getChild<0>();};
    };

    struct ReturnStmt : public CondStmt, public AstNode<Expr>
    {
        ReturnStmt(Expr* arg, tok::Token &o)
            : CondStmt(o.loc + arg->loc),
            AstNode<Expr>(arg)
        {};
        std::string myLbl() {return "return";}
        Expr* getExpr() {return getChild<0>();};
    };
}

#endif
