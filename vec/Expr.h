#ifndef EXPR_H
#define EXPR_H

#include "AstNode.h"
#include "Scope.h"
#include "Location.h"
#include "Type.h"

namespace ast
{
    typedef int Str;

    //abstract expression type
    struct Expr : public virtual AstNodeB
    {
        typ::Type type;
        virtual bool isLval() {return false;};
        //Expr() = default;
        Expr(tok::Location &&l) : AstNodeB(l) {};
        Expr(tok::Location &l) : AstNodeB(l) {};
        const char *myColor() {return "4";};
    };

    //leaf expression type
    struct NullExpr : public Expr, public AstNode0
    {
        NullExpr(tok::Location &&l) : Expr(l) {};
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";};
    };

    struct DeclExpr;

    struct VarExpr : public Expr, public AstNode0
    {
        DeclExpr* var; //the variable we belong to

        VarExpr(DeclExpr* v, tok::Location &l) : Expr(l), var(v) {};
        bool isLval() {return true;};
        inline std::string myLbl();
        const char *myColor() {return "5";};
    };

    //could possible have a weak_string of its name?
    struct DeclExpr : public VarExpr
    {
        Scope* owner; //to look up typedefs
        Ident name; //for errors
        DeclExpr(Ident n, typ::Type& t, Scope* o, tok::Location &l) : VarExpr(this, l), owner(o), name(n) {type = t;}
        std::string myLbl() {return std::string(type.w_str()) + " " + utl::to_str(name);}
    };

    //put this here so it knows what a DeclExpr is
    std::string VarExpr::myLbl()
    {
        return var != 0 ? "var " + utl::to_str(var->name) : "undefined var";
    }

    struct FuncDeclExpr : public DeclExpr
    {
        Scope* funcScope;
        FuncDeclExpr(Ident n, typ::Type& t, Scope* s, tok::Location &l) : DeclExpr(n, t, s->getParent(), l), funcScope(s) {}
    };

    struct ConstExpr : public Expr, public AstNode0
    {
        ConstExpr(tok::Location &l) : Expr(l) {};
        const char *myColor() {return "7";};
    };

    struct IntConstExpr : public ConstExpr
    {
        long long value;
        IntConstExpr(long long v, tok::Location &l) : ConstExpr(l), value(v) {};
        std::string myLbl() {return utl::to_str(value);}
    };

    struct FloatConstExpr : public ConstExpr
    {
        long double value;
        FloatConstExpr(long double v, tok::Location &l) : ConstExpr(l), value(v) {};
        std::string myLbl() {return utl::to_str(value);}
    };

    struct StringConstExpr : public ConstExpr
    {
        Str value;
        StringConstExpr(Str v, tok::Location &l) : ConstExpr(l), value(v) {};
        std::string myLbl() {return "str: " + utl::to_str(value);}
    };

    //general binary expressions
    struct BinExpr : public Expr, public AstNode2<Expr, Expr>
    {
        tok::TokenType op;
        tok::Location opLoc;
        BinExpr(Expr* lhs, Expr* rhs, tok::Token &o)
            : Expr(lhs->loc + rhs->loc),
            AstNode2<Expr, Expr>(lhs, rhs), op(o.type), opLoc(o.loc)
        {};
        BinExpr(Expr* lhs, Expr* rhs, tok::TokenType o)
            : Expr(lhs->loc + rhs->loc),
            AstNode2<Expr, Expr>(lhs, rhs), op(o)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct AssignExpr : public BinExpr
    {
        AssignExpr(Expr* lhs, Expr* rhs, tok::Token &o) : BinExpr(lhs, rhs, o) {};
        std::string myLbl() {return "'='";}
    };

    struct OpAssignExpr : public AssignExpr
    {
        tok::TokenType assignOp;
        OpAssignExpr(Expr* lhs, Expr* rhs, tok::Token &o) : AssignExpr(lhs, rhs, o), assignOp(o.value.op) {};
        std::string myLbl() {return std::string(tok::Name(assignOp)) + '=';}
    };

    inline BinExpr* makeBinExpr(Expr* lhs, Expr* rhs, tok::Token &op)
    {
        if (op == tok::equals)
            return new AssignExpr(lhs, rhs, op);
        else if (op == tok::opequals)
            return new OpAssignExpr(lhs, rhs, op);
        else
            return new BinExpr(lhs, rhs, op);
    }

    //unary expressions
    struct UnExpr : public Expr, public AstNode1<Expr>
    {
        tok::TokenType op;
        UnExpr(Expr* arg, tok::Token &o)
            : Expr(o.loc + arg->loc),
            AstNode1<Expr>(arg), op(o.type)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct IterExpr : public Expr, public AstNode1<Expr>
    {
        IterExpr(Expr* arg, tok::Token &o)
            : Expr(o.loc + arg->loc),
            AstNode1<Expr>(arg)
        {};

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "`";}
    };

    struct AggExpr : public Expr, public AstNode1<Expr>
    {
        tok::TokenType op;
        AggExpr(Expr* arg, tok::Token &o)
            : Expr(o.loc + arg->loc),
            AstNode1<Expr>(arg), op(o.value.op)
        {};
        std::string myLbl() {return tok::Name(op) + std::string("=");}
    };

    struct ListifyExpr : public UnExpr
    {
        ListifyExpr(Expr* arg, tok::Token &o)
            : UnExpr(arg, o)
        {};
        std::string myLbl() {return "{...}";}
    };

    struct TuplifyExpr : public UnExpr
    {
        TuplifyExpr(Expr* arg, tok::Token &o)
            : UnExpr(arg, o)
        {};
        std::string myLbl() {return "[...]";}
    };

    //postfix expressions
    //for ++ and --
    struct PostExpr : public Expr, public AstNode1<Expr>
    {
        tok::TokenType op;
        PostExpr(Expr* arg, tok::Token &o)
            : Expr(arg->loc + o.loc),
            AstNode1<Expr>(arg), op(o.type)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct TupAccExpr : public BinExpr
    {
        TupAccExpr(Expr* lhs, Expr* rhs, tok::Token &o)
            : BinExpr(lhs, rhs, o)
        {}

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "a{b}";}
    };

    struct ListAccExpr : public BinExpr
    {
        ListAccExpr(Expr* lhs, Expr* rhs, tok::Token &o)
            : BinExpr(lhs, rhs, o)
        {}

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "a[b]";}
    };
}

#endif
