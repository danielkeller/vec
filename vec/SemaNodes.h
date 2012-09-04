#ifndef SEMANODES_H
#define SEMANODES_H

#include "Stmt.h"

namespace ast
{
    //This file is for AST nodes that are added during semantic analysis, rather than parsing
    struct TmpExpr : public Node0
    {
        const char *myColor() {return "8";};
        std::string myLbl () {return "tmp";};
        Node0* setBy;

        TmpExpr(Node0* sb) : Node0(sb->loc), setBy(sb){};

        void emitDot(std::ostream& os)
        {
            Node0::emitDot(os);
            //shows a "sets" relationship
            os << 'n' << static_cast<Node0*>(setBy) << " -> n" << static_cast<Node0*>(this)
                << " [style=dotted];\n";
        };

        annot_t& Annot() {return setBy->Annot();}
    };

    struct ImpliedLoopStmt : public Node1
    {
        std::vector<IterExpr*> targets;
        ImpliedLoopStmt(Ptr arg)
            : Node1(move(arg))
        {};
        std::string myLbl() {return "for (`)";}

        void emitDot(std::ostream& os)
        {
            Node1::emitDot(os);
            for (auto n : targets)
                os << 'n' << static_cast<Node0*>(this) << " -> n" << static_cast<Node0*>(n)
                    << " [style=dotted];\n";
        };
    };

    //TODO: do it this way, or with inline assembly?
    struct IntrinDeclExpr : public FuncDeclExpr
    {
        int intrin_id;
        //it's "based on" func so it's ok not to use Ptr
        IntrinDeclExpr(FuncDeclExpr* func, int id)
            : FuncDeclExpr(*func), intrin_id(id) {}
        const char *myColor() {return "8";};
    };

    struct IntrinCallExpr : public NodeN
    {
        int intrin_id;
        IntrinCallExpr(NPtr<OverloadCallExpr>::type orig, int intrin_id)
            : NodeN(orig->loc),
            intrin_id(intrin_id)
        {
            Annotate(orig->Type());
            consume(move(orig));
        }

        IntrinCallExpr(NPtr<BinExpr>::type orig, int intrin_id)
            : NodeN(orig->loc),
            intrin_id(intrin_id)
        {
            Annotate(orig->Type());
            appendChild(orig->detachChildA());
            appendChild(orig->detachChildB());
        }

        std::string myLbl() {return utl::to_str(intrin_id) + ":";}
        const char *myColor() {return "5";}
        void inferType(sa::Sema&);
        llvm::Value* generate(cg::CodeGen& gen);
    };

    struct FunctionDef : public Node1
    {
        FunctionDef(typ::Type t, Ptr s)
            : Node1(move(s))
        {Annotate(t);}
        std::string myLbl() {return Type().to_str();}
        llvm::Value* generate(cg::CodeGen&);
    };

    struct ArithCast : public Node1
    {
        ArithCast(typ::Type to, Ptr node)
            : Node1(move(node)) {Annotate(to);}
        std::string myLbl() {return "(" + Type().to_str() + ")";}
        void inferType(sa::Sema&);
        llvm::Value* generate(cg::CodeGen& gen);
    };
}

#endif
