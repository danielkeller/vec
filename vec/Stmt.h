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
    };

    struct NullStmt : public Stmt
    {
        NullStmt(tok::Location &&l) : Stmt(std::move(l)) {};
    };

    struct ExprStmt : public Stmt, public AstNode<Expr>
    {
        ExprStmt(Expr* conts)
            : Stmt(std::move(conts->loc)),
            AstNode<Expr>(conts)
        {};
    };

    struct StmtPair : public Stmt, public AstNode<Stmt, Stmt>
    {
        StmtPair(Stmt *lhs, Stmt *rhs) : AstNode<Stmt, Stmt>(lhs, rhs) {};
    };

    struct Block : public Expr, public Stmt, public AstNode<Stmt>
    {
        Scope *scope;
        Block(Stmt* conts, Scope *s, tok::Location &&l)
            : Expr(std::move(l)),
            Stmt(std::move(l)),
            AstNode<Stmt>(conts),
            scope(s)
        {};
        ~Block() {delete scope;};
    };

    struct IfStmt : public Stmt, public AstNode<Expr, Stmt>
    {
        IfStmt(Expr* pred, Stmt* act, tok::Token &o)
            : Stmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
    };

    struct IfElseStmt : public Stmt, public AstNode<Expr, Stmt, Stmt>
    {
        IfElseStmt(Expr* pred, Stmt* act1, Stmt* act2, tok::Token &o)
            : Stmt(o.loc + act2->loc),
            AstNode<Expr, Stmt, Stmt>(pred, act1, act2)
        {};
    };

    struct SwitchStmt : public Stmt, public AstNode<Expr, Stmt>
    {
        SwitchStmt(Expr* pred, Stmt* act, tok::Token &o)
            : Stmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
    };

    struct WhileStmt : public Stmt, public AstNode<Expr, Stmt>
    {
        WhileStmt(Expr* pred, Stmt* act, tok::Token &o)
            : Stmt(o.loc + act->loc),
            AstNode<Expr, Stmt>(pred, act)
        {};
    };

    struct ReturnStmt : public Stmt, public AstNode<Expr>
    {
        ReturnStmt(Expr* arg, tok::Token &o)
            : Stmt(o.loc + arg->loc),
            AstNode<Expr>(arg)
        {};
    };
}

#endif
