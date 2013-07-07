#ifndef SEMANODES_H
#define SEMANODES_H

#include "Stmt.h"

#include <map>

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
        llvm::Value* generate(cg::CodeGen&);
    };
    
    //TODO: consider holding branchStmts some other way, so we don't have to cast so much
    //a "routing" node that contains BranchStmts
    struct RhoStmt : public NodeN
    {
        std::string myLbl() {return "rho";}
        const char *myColor() {return "3";}
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen&);
    };

    //a BranchStmt is the terminator of the basic block that is attached below it
    struct BranchStmt : public Node1
    {
        //either of these being null means "leave the current rho block"
        BranchStmt* ifTrue;
        BranchStmt* ifFalse;

        annot_t& Annot() {return getChildA()->Annot();}

        //conditional
        BranchStmt(Ptr basicBlock, BranchStmt* ift, BranchStmt* iff)
            : Node1(move(basicBlock)), ifTrue(ift), ifFalse(iff)
        {};

        //unconditional
        BranchStmt(Ptr basicBlock, BranchStmt* dest)
            : Node1(move(basicBlock)), ifTrue(dest), ifFalse(dest)
        {};
        
        //null (leaving current rho block)
        BranchStmt(Ptr basicBlock)
            : Node1(move(basicBlock)), ifTrue(nullptr), ifFalse(nullptr)
        {};

        std::string myLbl() {return "branch";}

        void emitDot(std::ostream& os)
        {
            Node1::emitDot(os);
            if (ifTrue)
                os << 'n' << static_cast<Node0*>(this) << " -> n" << static_cast<Node0*>(ifTrue)
                    << " [style=dotted];\n";
            if (ifFalse)
                os << 'n' << static_cast<Node0*>(this) << " -> n" << static_cast<Node0*>(ifFalse)
                    << " [style=dotted];\n";
        };

        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen&);
    };

    //Generalization of TmpExpr for any number of predecessor blocks
    struct PhiExpr : public Node0
    {
        std::vector<BranchStmt*> inputs;

        const char *myColor() {return "8";};
        std::string myLbl () {return "phi";};

        PhiExpr(tok::Location& loc) : Node0(loc){};

        void emitDot(std::ostream& os)
        {
            Node0::emitDot(os);
            //shows a "sets" relationship
            for (auto p : inputs)
                os << 'n' << static_cast<Node0*>(p) << " -> n" << static_cast<Node0*>(this)
                    << " [style=dotted];\n";
        };
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen&);
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
    struct IntrinDeclExpr : public DeclExpr
    {
        int intrin_id;
        //it's "based on" func so it's ok not to use Ptr
        IntrinDeclExpr(DeclExpr* func, int id)
            : DeclExpr(*func), intrin_id(id) {}
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

        std::string myLbl() {return utl::to_str(intrin_id) + ":";}
        const char *myColor() {return "5";}
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen& gen);
    };

    struct ArithCast : public Node1
    {
        ArithCast(typ::Type to, Ptr node)
            : Node1(move(node)) {Annotate(to);}
        std::string myLbl() {return "(" + Type().to_str() + ")";}
        void preExec(sa::Exec&);
        llvm::Value* generate(cg::CodeGen& gen);
    };
}

#endif
