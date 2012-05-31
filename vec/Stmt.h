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
            int p = 0;
            os << 'n' << static_cast<AstNode0*>(this) << ":p0" <<  " -> n" << static_cast<AstNode0*>(getChild<0>()) << ";\n"
               << 'n' << static_cast<AstNode0*>(this) << ":p1" <<  " -> n" << static_cast<AstNode0*>(getChild<1>()) << ";\n";
            ++p;
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

    struct IfStmt : public Stmt, public AstNode<Expr, Stmt>
    {
        IfStmt(Expr* pred, Stmt* act, tok::Token &o)
            : Stmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "if";}
    };

    struct IfElseStmt : public Stmt, public AstNode<Expr, Stmt, Stmt>
    {
        IfElseStmt(Expr* pred, Stmt* act1, Stmt* act2, tok::Token &o)
            : Stmt(o.loc + act2->loc),
            AstNode<Expr, Stmt, Stmt>(pred, act1, act2)
        {};
        std::string myLbl() {return "if-else";}
    };

    struct SwitchStmt : public Stmt, public AstNode<Expr, Stmt>
    {
        SwitchStmt(Expr* pred, Stmt* act, tok::Token &o)
            : Stmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "switch";}
    };

    struct WhileStmt : public Stmt, public AstNode<Expr, Stmt>
    {
        WhileStmt(Expr* pred, Stmt* act, tok::Token &o)
            : Stmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
        std::string myLbl() {return "while";}
    };

    struct ReturnStmt : public Stmt, public AstNode<Expr>
    {
        ReturnStmt(Expr* arg, tok::Token &o)
            : Stmt(o.loc + arg->loc),
            AstNode<Expr>(arg)
        {};
        std::string myLbl() {return "return";}
    };
}

#endif
