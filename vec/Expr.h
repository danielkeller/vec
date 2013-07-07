#ifndef EXPR_H
#define EXPR_H

#include "Global.h"
#include "AstNode.h"
#include "Scope.h"
#include "Location.h"

#include <iostream>

namespace ast
{
    //leaf expression type
    struct NullExpr : public Node0
    {
        //some explanation of what this is doing here
        const char * const detail;

        NullExpr(tok::Location const &l)
            : Node0(l), detail(nullptr)
        {
            Annotate(typ::null); //could be void or <error type> or something
        };
        NullExpr()
            : Node0(tok::Location()), detail(nullptr) {}
        NullExpr(const char * const detail)
            : Node0(tok::Location()), detail(detail) {}

        std::string myLbl() {return detail ? detail : "Null";}
        const char *myColor() {return "9";};
        llvm::Value* generate(cg::CodeGen&);
    };

    struct DeclExpr;

    struct VarExpr : public Node0
    {
        NormalScope* sco; //the scope we're in
        bool isOp; //is this an operator?

    private:
        union { //isOp tags this union
            Ident name; //name of the variable
            tok::TokenType op; //operator
        };
        
    public:
        Ident Name() {return isOp ? Global().findIdent(op) : name;}

        tok::TokenType Op() {return isOp ? op : tok::none;}

        VarExpr(Ident n, NormalScope* s, tok::Location const &l)
            : Node0(l), sco(s), name(n), isOp(false) {}
        VarExpr(const tok::Token opt, NormalScope* s)
            : Node0(opt.loc), sco(s), op(opt.type), isOp(true) {}
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

        ~DeclExpr()
        {
            sco->removeVarDef(this);
        }

        DeclExpr(DeclExpr& other)
            : VarExpr(other.Name(), other.sco, other.loc)
        {
            other.sco->addVarDef(this);
            Annotate(other.Type());
        }

        std::string myLbl() {return Type().to_str() + " " + Global().getIdent(Name());}

        //have to re-override it back to the original
        annot_t& Annot() {return Node0::Annot();}
        llvm::Value* generate(cg::CodeGen& gen);
    };

    //put this here so it knows what a DeclExpr is
    std::string VarExpr::myLbl()
    {
        return sco->getVarDef(name) != 0 ? Global().getIdent(name) : "undefined var";
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

    struct AssignExpr : public Node2
    {
        AssignExpr(Ptr lhs, Ptr rhs, tok::Location &opLoc)
            : Node2(move(lhs), move(rhs), tok::Location()), opLoc(opLoc)
        {
            loc = getChildA()->loc + getChildB()->loc;
        };
        std::string myLbl() {return "'='";}
        annot_t& Annot() {return getChildA()->Annot();}
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen& gen);
        tok::Location opLoc;
    };

    struct OpAssignExpr : public AssignExpr
    {
        tok::TokenType assignOp;
        NormalScope* sco;
        OpAssignExpr(Ptr lhs, Ptr rhs, tok::Token &o, NormalScope* sco)
            : AssignExpr(move(lhs), move(rhs), o.loc), assignOp(o.value.op), sco(sco) {};
        std::string myLbl() {return std::string(tok::Name(assignOp)) + '=';}
    };

    struct OverloadCallExpr : public NodeN
    {
        OverloadCallExpr(NPtr<VarExpr>::type lhs, Ptr rhs, const tok::Location & l)
            : fun(move(lhs)), NodeN(move(rhs), l) {}
        OverloadCallExpr(tok::Token op, ast::NormalScope *sco, Ptr a, Ptr b)
            : fun(new VarExpr(op, sco)), NodeN(move(a), move(b), op.loc) {}
        std::string myLbl() {return utl::to_str(fun->Name()) + " ?:?";};

        void preExec(sa::Exec&);
        NPtr<VarExpr>::type fun;
        DeclExpr* ovrResult;

        //ex is optional; null if we don't want to exec it now
        void resolveOverload(typ::Type argType, sa::Exec* ex);
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

    struct TupAccExpr : public OverloadCallExpr
    {
        TupAccExpr(Ptr lhs, Ptr rhs, tok::Token &o, ast::NormalScope *sco)
            : OverloadCallExpr(o, sco, move(lhs), move(rhs))
        {}

        std::string myLbl() {return "a{b}";}
    };

    struct ListAccExpr : public OverloadCallExpr
    {
        ListAccExpr(Ptr lhs, Ptr rhs, tok::Token &o, ast::NormalScope *sco)
            : OverloadCallExpr(o, sco, move(lhs), move(rhs))
        {}

        std::string myLbl() {return "a[b]";}
    };
}

#endif
