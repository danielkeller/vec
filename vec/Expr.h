#ifndef EXPR_H
#define EXPR_H

#include "Global.h"
#include "AstNode.h"
#include "Scope.h"
#include "Location.h"

namespace ast
{
    //leaf expression type
    struct NullExpr : public Node0
    {
        NullExpr(tok::Location const &l)
            : Node0(l)
        {
            Annotate(typ::null); //could be void or <error type> or something
        };
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";};
    };

    struct DeclExpr;

    struct VarExpr : public Node0
    {
        DeclExpr* var; //the variable we belong to
        Ident ename; //name of external variable

        VarExpr(DeclExpr* v, tok::Location const &l)
            : Node0(l), var(v), ename(Global().reserved.null) {}
        VarExpr(Ident n, tok::Location const &l)
            : Node0(l), var(Global().reserved.undeclared_v), ename(n) {}
        bool isLval() {return true;};
        inline std::string myLbl();
        const char *myColor() {return "5";};
        inline typ::Type Type() const;
    };

    //could possibly have a weak_string of its name?
    struct DeclExpr : public VarExpr
    {
        Ident name; //for errors

        DeclExpr(Ident n, typ::Type t, tok::Location const &l)
            : VarExpr(this, l),  name(n) {Annotate(t);}
        DeclExpr(const DeclExpr& old) //this has to be specified because var must point to this
            : VarExpr(this, old.loc), name(old.name) {Annotate(old.Type());}

        std::string myLbl() {return Type().to_str() + " " + utl::to_str(name);}
        typ::Type Type() const {return DefaultType();} //have to re-override it back to the original
    };

    //put this here so it knows what a DeclExpr is
    std::string VarExpr::myLbl()
    {
        return var != 0 ? "var " + utl::to_str(var->name) : "undefined var";
    }

    typ::Type VarExpr::Type() const
    {
        return var->Type();
    }

    struct FuncDeclExpr : public DeclExpr
    {
        NormalScope* funcScope;
        FuncDeclExpr(Ident n, typ::Type t, NormalScope* s, tok::Location const &l)
            : DeclExpr(n, t, l), funcScope(s) {}
    };

    //declaration of an entire function overload group
    struct OverloadGroupDeclExpr : public DeclExpr
    {
        typ::Type Type() const {return typ::overload;}
        std::list<FuncDeclExpr*> functions;
        OverloadGroupDeclExpr(Ident n, tok::Location const &firstLoc)
            : DeclExpr(n, typ::error, firstLoc) {}
        void Insert(FuncDeclExpr* newDecl);
    };

    struct ConstExpr : public Node0
    {
        ConstExpr(tok::Location &l)
            : Node0(l)
        {}

        const char *myColor() {return "7";};
    };

    struct IntrinCallExpr;

    struct OverloadableExpr
    {
        NormalScope* owner;
        FuncDeclExpr* ovrResult;
        virtual NPtr<IntrinCallExpr>::type makeICall() = 0;
        OverloadableExpr(NormalScope* o)
            : owner(o), ovrResult(0) {}
    };

    //general binary expressions
    struct BinExpr : public OverloadableExpr, public Node2
    {
        tok::TokenType op;
        tok::Location opLoc;
        BinExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::Token &o)
            : OverloadableExpr(sc),
            Node2(move(lhs), move(rhs)), op(o.type), opLoc(o.loc)
        {};
        BinExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::TokenType o)
            : OverloadableExpr(sc),
            Node2(move(lhs), move(rhs)), op(o)
        {};
        std::string myLbl() {return tok::Name(op);}

        NPtr<IntrinCallExpr>::type makeICall();
        void inferType(sa::Sema&);
    };

    struct AssignExpr : public BinExpr
    {
        AssignExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::Token &o)
            : BinExpr(move(lhs), move(rhs), sc, o) {};
        std::string myLbl() {return "'='";}
        typ::Type Type() const {return getChildA()->Type();}
        void inferType(sa::Sema&);
    };

    struct OpAssignExpr : public AssignExpr
    {
        tok::TokenType assignOp;
        OpAssignExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::Token &o)
            : AssignExpr(move(lhs), move(rhs), sc, o), assignOp(o.value.op) {};
        std::string myLbl() {return std::string(tok::Name(assignOp)) + '=';}
    };

    struct OverloadCallExpr : public OverloadableExpr, public NodeN
    {
        NPtr<VarExpr>::type func; //so tree walker won't see it
        OverloadCallExpr(NPtr<VarExpr>::type lhs, Ptr rhs, NormalScope* o, const tok::Location & l)
            : OverloadableExpr(o), NodeN(move(rhs), l), func(move(lhs)) {}
        std::string myLbl() {return utl::to_str(func->var->name) + " ?:?";};

        NPtr<IntrinCallExpr>::type makeICall();
        void inferType(sa::Sema&);
    };

    inline BinExpr* makeBinExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::Token &op)
    {
        if (op == tok::equals)
            return new AssignExpr(move(lhs), move(rhs), sc, op);
        else if (op == tok::opequals)
            return new OpAssignExpr(move(lhs), move(rhs), sc, op);
        else
            return new BinExpr(move(lhs), move(rhs), sc, op);
    }

    //unary expressions
    struct UnExpr : public Node1
    {
        tok::TokenType op;
        UnExpr(Ptr arg, tok::Token &o)
            : Node1(move(arg)), op(o.type)
        {
            loc = o.loc + getChildA()->loc;
        };
        std::string myLbl() {return tok::Name(op);}
    };

    struct IterExpr : public Node1
    {
        IterExpr(Ptr arg, tok::Token &o)
            : Node1(move(arg))
        {
            loc = o.loc + getChildA()->loc;
        };

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "`";}
    };

    struct AggExpr : public Node1
    {
        tok::TokenType op;
        AggExpr(Ptr arg, tok::Token &o)
            : Node1(move(arg)), op(o.value.op)
        {
            loc = o.loc + getChildA()->loc;
        };
        std::string myLbl() {return tok::Name(op) + std::string("=");}
    };

    //TODO: more accurate location
    struct ListifyExpr : public NodeN
    {
        ListifyExpr(Ptr arg)
            : NodeN(move(arg), tok::Location())
        {
            loc = getChild(0)->loc;
        };
        std::string myLbl() {return "[...]";}
        void inferType(sa::Sema&);
    };

    struct TuplifyExpr : public NodeN
    {
        TuplifyExpr(Ptr arg)
            : NodeN(move(arg), tok::Location())
        {
            loc = getChild(0)->loc;
        };
        std::string myLbl() {return "\\{...\\}";}
        void inferType(sa::Sema&);
    };

    //postfix expressions
    //for ++ and --
    struct PostExpr : public Node1
    {
        tok::TokenType op;
        PostExpr(Ptr arg, tok::Token &o)
            : Node1(move(arg)), op(o.type)
        {
            loc = getChildA()->loc + o.loc;
        };
        std::string myLbl() {return tok::Name(op);}
    };

    struct TupAccExpr : public BinExpr
    {
        TupAccExpr(Ptr lhs, Ptr rhs, tok::Token &o)
            : BinExpr(move(lhs), move(rhs), 0, o)
        {}

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "a{b}";}
    };

    struct ListAccExpr : public BinExpr
    {
        ListAccExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::Token &o)
            : BinExpr(move(lhs), move(rhs), sc, o)
        {}

        bool isLval() {return getChildA()->isLval();}
        std::string myLbl() {return "a[b]";}
    };
}

#endif
