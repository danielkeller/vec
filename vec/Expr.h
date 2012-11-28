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
        NullExpr()
            : Node0(tok::Location()) {}
        std::string myLbl() {return "Null";}
        const char *myColor() {return "9";};
        llvm::Value* generate(cg::CodeGen&);
    };

    struct DeclExpr;

    struct VarExpr : public Node0
    {
        NormalScope* sco; //the scope we're in
        Ident name; //name of the variable

        VarExpr(Ident n, NormalScope* s, tok::Location const &l)
            : Node0(l), sco(s), name(n) {}
        inline std::string myLbl();
        const char *myColor() {return "5";};
        inline annot_t& Annot();
        llvm::Value* generate(cg::CodeGen& gen);
    };

    struct DeclExpr : public VarExpr
    {
        DeclExpr(Ident n, NormalScope* s, typ::Type t, tok::Location const &l)
            : VarExpr(n, s, l)
        {
            s->addVarDef(this);
            Annotate(t);
        }

        DeclExpr(DeclExpr& other)
            : VarExpr(other.name, other.sco, other.loc)
        {
            Annotate(other.Type());
        }

        std::string myLbl() {return Type().to_str() + " " + utl::to_str(name);}

        //have to re-override it back to the original
        annot_t& Annot() {return Node0::Annot();}
        llvm::Value* generate(cg::CodeGen& gen);
    };

    //put this here so it knows what a DeclExpr is
    std::string VarExpr::myLbl()
    {
        return sco->getVarDef(name) != 0 ? "var " + utl::to_str(name) : "undefined var";
    }

    Node0::annot_t& VarExpr::Annot()
    {
        DeclExpr* var = sco->getVarDef(name);
        if (var)
            return var->Annot();
        else //is this right?
            return Node0::Annot();
    }

    struct Lambda : public Node1
    {
        Ident name;
        std::vector<Ident> params;
        NormalScope* sco;
        Lambda(Ident n, std::vector<Ident> &&p, NormalScope* s, Ptr conts)
            : Node1(move(conts)), name(n), params(move(p)), sco(s)
        {}
        std::string myLbl() {return Type().to_str();}
        llvm::Value* generate(cg::CodeGen&);
    };


    struct ConstExpr : public Node0
    {
        ConstExpr(tok::Location &l)
            : Node0(l)
        {}

        const char *myColor() {return "7";}
        std::string myLbl() {return "const " + Type().to_str() + "";}

        void emitDot(std::ostream& os);
        llvm::Value* generate(cg::CodeGen& gen);
    };

    struct IntrinCallExpr;

    struct OverloadableExpr
    {
        NormalScope* sco;
        Ident name;
        DeclExpr* ovrResult;
        virtual IntrinCallExpr* makeICall() = 0;
        OverloadableExpr(NormalScope* o, Ident n)
            : sco(o), name(n), ovrResult(0) {}

        //ex is optional; null if we don't want to exec it now
        void resolveOverload(typ::Type argType, sa::Exec* ex);
    };

    //general binary expressions
    struct BinExpr : public OverloadableExpr, public Node2
    {
        tok::TokenType op;
        tok::Location opLoc;
        BinExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::Token &o)
            : OverloadableExpr(sc, Global().reserved.opIdents[o.Type()]),
            Node2(move(lhs), move(rhs)), op(o.type), opLoc(o.loc)
        {};
        BinExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::TokenType o)
            : OverloadableExpr(sc, Global().reserved.opIdents[o]),
            Node2(move(lhs), move(rhs)), op(o)
        {};
        std::string myLbl() {return tok::Name(op);}

        IntrinCallExpr* makeICall();
        void preExec(sa::Exec&);
    };

    struct AssignExpr : public BinExpr
    {
        AssignExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::Token &o)
            : BinExpr(move(lhs), move(rhs), sc, o) {};
        std::string myLbl() {return "'='";}
        annot_t& Annot() {return getChildA()->Annot();}
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen& gen);
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
        OverloadCallExpr(NPtr<VarExpr>::type lhs, Ptr rhs, const tok::Location & l)
            : OverloadableExpr(lhs->sco, lhs->name), NodeN(move(rhs), l) {}
        std::string myLbl() {return utl::to_str(name) + " ?:?";};

        IntrinCallExpr* makeICall();
        void preExec(sa::Exec&);
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
        void preExec(sa::Exec&);
    };

    struct TuplifyExpr : public NodeN
    {
        TuplifyExpr(Ptr arg)
            : NodeN(move(arg), tok::Location())
        {
            loc = getChild(0)->loc;
        };
        std::string myLbl() {return "\\{...\\}";}
        void preExec(sa::Exec&);
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

        std::string myLbl() {return "a{b}";}
    };

    struct ListAccExpr : public BinExpr
    {
        ListAccExpr(Ptr lhs, Ptr rhs, NormalScope* sc, tok::Token &o)
            : BinExpr(move(lhs), move(rhs), sc, o)
        {}

        std::string myLbl() {return "a[b]";}
    };
}

#endif
