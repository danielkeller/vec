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
    protected:
        //type is private, and has a virtual "sgetter" so we can override its behavior
        //when a derived Expr has some sort of deterministic type. wasted space isn't
        //a big deal since it's only sizeof(void*) bytes
        typ::Type type;

    public:
        //Expr() = default;
        //Expr(tok::Location &&l) : AstNodeB(l) {};
        Expr(tok::Location const &l) : type(typ::error) {loc = l;}
        virtual bool isLval() {return false;};
        virtual typ::Type& Type() {return type;} //sgetter. sget it?
        const char *myColor() {return "4";};
    };

    //leaf expression type
    struct NullExpr : public Expr, public AstNode0
    {
        NullExpr(tok::Location const &l) : Expr(l) {};
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";};
        typ::Type& Type() {return typ::null;}
    };

    struct DeclExpr;

    struct VarExpr : public Expr, public AstNode0
    {
        DeclExpr* var; //the variable we belong to

        VarExpr(DeclExpr* v, tok::Location const &l) : Expr(l), var(v) {};
        bool isLval() {return true;};
        inline std::string myLbl();
        const char *myColor() {return "5";};
        inline typ::Type& Type();
    };

    //could possibly have a weak_string of its name?
    struct DeclExpr : public VarExpr
    {
        Scope* owner; //to look up typedefs
        Ident name; //for errors

        DeclExpr(Ident n, typ::Type t, Scope* o, tok::Location const &l)
            : VarExpr(this, l), owner(o), name(n) {type = t;}
        DeclExpr(const DeclExpr& old) //this has to be specified because var must point to this
            : VarExpr(this, old.loc), owner(old.owner), name(old.name) {type = old.type;}

        std::string myLbl() {return Type().to_str() + " " + utl::to_str(name);}
        typ::Type& Type() {return type;} //have to re-override it back to the original
    };

    //put this here so it knows what a DeclExpr is
    std::string VarExpr::myLbl()
    {
        return var != 0 ? "var " + utl::to_str(var->name) : "undefined var";
    }

    typ::Type& VarExpr::Type()
    {
        return var->Type();
    }

    struct FuncDeclExpr : public DeclExpr
    {
        Scope* funcScope;
        FuncDeclExpr(Ident n, typ::Type t, Scope* s, tok::Location const &l)
            : DeclExpr(n, t, s->getParent(), l), funcScope(s) {}
    };

    //declaration of an entire function overload group
    struct OverloadGroupDeclExpr : public DeclExpr
    {
        typ::Type& Type() {return typ::overload;}
        std::list<FuncDeclExpr*> functions;
        OverloadGroupDeclExpr(Ident n, Scope* global, tok::Location const &firstLoc)
            : DeclExpr(n, typ::error, global, firstLoc) {}
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

    struct OverloadableExpr : public Expr
    {
        Scope* owner;
        FuncDeclExpr* ovrResult;
        OverloadableExpr(Scope* o, const tok::Location & l)
            : Expr(l), owner(o), ovrResult(0) {}
    };

    //general binary expressions
    struct BinExpr : public OverloadableExpr, public AstNode2<Expr, Expr>
    {
        tok::TokenType op;
        tok::Location opLoc;
        BinExpr(Expr* lhs, Expr* rhs, Scope* sc, tok::Token &o)
            : OverloadableExpr(sc, lhs->loc + rhs->loc),
            AstNode2<Expr, Expr>(lhs, rhs), op(o.type), opLoc(o.loc)
        {};
        BinExpr(Expr* lhs, Expr* rhs, Scope* sc, tok::TokenType o)
            : OverloadableExpr(sc, lhs->loc + rhs->loc),
            AstNode2<Expr, Expr>(lhs, rhs), op(o)
        {};
        std::string myLbl() {return tok::Name(op);}
    };

    struct AssignExpr : public BinExpr
    {
        AssignExpr(Expr* lhs, Expr* rhs, Scope* sc, tok::Token &o)
            : BinExpr(lhs, rhs, sc, o) {};
        std::string myLbl() {return "'='";}
        typ::Type& Type() {return getChildA()->Type();}
    };

    struct OpAssignExpr : public AssignExpr
    {
        tok::TokenType assignOp;
        OpAssignExpr(Expr* lhs, Expr* rhs, Scope* sc, tok::Token &o)
            : AssignExpr(lhs, rhs, sc, o), assignOp(o.value.op) {};
        std::string myLbl() {return std::string(tok::Name(assignOp)) + '=';}
    };

    struct OverloadCallExpr : public OverloadableExpr, public AstNodeN<Expr>
    {
        VarExpr* func; //so tree walker won't see it
        OverloadCallExpr(VarExpr* lhs, Expr* rhs, Scope* o, const tok::Location & l)
            : OverloadableExpr(o, l), AstNodeN<Expr>(rhs), func(lhs) {}
        ~OverloadCallExpr() {delete func;} //has to be deleted manually
        std::string myLbl() {return utl::to_str(func->var->name) + " ?:?";};
    };

    inline BinExpr* makeBinExpr(Expr* lhs, Expr* rhs, Scope* sc, tok::Token &op)
    {
        if (op == tok::equals)
            return new AssignExpr(lhs, rhs, sc, op);
        else if (op == tok::opequals)
            return new OpAssignExpr(lhs, rhs, sc, op);
        else
            return new BinExpr(lhs, rhs, sc, op);
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

    //TODO: more accurate location
    struct ListifyExpr : public Expr, public AstNodeN<Expr>
    {
        ListifyExpr(Expr* arg)
            : Expr(arg->loc), AstNodeN<Expr>(arg)
        {};
        std::string myLbl() {return "\\{...\\}";}
    };

    struct TuplifyExpr : public Expr, public AstNodeN<Expr>
    {
        TuplifyExpr(Expr* arg)
            : Expr(arg->loc), AstNodeN<Expr>(arg)
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
            : BinExpr(lhs, rhs, 0, o)
        {}

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "a[b]";}
    };

    struct ListAccExpr : public BinExpr
    {
        ListAccExpr(Expr* lhs, Expr* rhs, Scope* sc, tok::Token &o)
            : BinExpr(lhs, rhs, sc, o)
        {}

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "a{b}";}
    };
}

#endif
