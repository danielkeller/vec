#ifndef SEMANODES_H
#define SEMANODES_H

#include "Stmt.h"

namespace ast
{
    //This file is for AST nodes that are added during semantic analysis, rather than parsing
    struct SemaStmt : public Stmt
    {
        const char *myColor() {return "8";};
    };

    struct TmpExpr : public Expr, public AstNode0
    {
        const char *myColor() {return "8";};
        std::string myLbl () {return "tmp";};
        Expr* setBy;

        TmpExpr(Expr* sb) : Expr(sb->loc), setBy(sb){};

        void emitDot(std::ostream& os)
        {
            AstNode0::emitDot(os);
            //shows a "sets" relationship
            os << 'n' << static_cast<AstNodeB*>(setBy) << " -> n" << static_cast<AstNodeB*>(this)
                << " [style=dotted];\n";
        };

        typ::Type& Type() {return setBy->Type();}
    };

    struct OverloadCallExpr : public Expr, public AstNode1<Expr>
    {
        VarExpr* func; //so tree walker won't see it
        Scope* owner;
        FuncDeclExpr* ovrResult;
        OverloadCallExpr(VarExpr* lhs, Expr* rhs, Scope* o, const tok::Location & l)
            : func(lhs), owner(o), Expr(l), AstNode1<Expr>(rhs) {}
        ~OverloadCallExpr() {delete func;} //has to be deleted manually
    };

    struct ImpliedLoopStmt : public SemaStmt, public AstNode1<Stmt>
    {
        std::vector<IterExpr*> targets;
        ImpliedLoopStmt(ExprStmt* arg)
            : AstNode1<Stmt>(arg)
        {};
        std::string myLbl() {return "for (`)";};

        void emitDot(std::ostream& os)
        {
            AstNode1<Stmt>::emitDot(os);
            for (auto n : targets)
                os << 'n' << static_cast<AstNodeB*>(this) << " -> n" << static_cast<AstNodeB*>(n)
                    << " [style=dotted];\n";
        };
    };

    struct FunctionDef : public SemaStmt, public AstNode1<Stmt>
    {
        FuncDeclExpr* decl; //store it here so the walker can't find it
        FunctionDef(FuncDeclExpr* fde, Stmt* s) : AstNode1<Stmt>(s), decl(fde) {}
        std::string myLbl() {return decl->myLbl();}
        ~FunctionDef() {delete decl;} //its not in the tree, we have to delete it manually
    };

    //node with any number of children
    template<class T>
    struct ListNode : public Stmt
    {
        typedef std::vector<T*> conts_t;

        conts_t chld;

        ~ListNode()
        {
            for (auto n : chld)
                delete n;
        };

        void eachChild(sa::AstWalker0 *w)
        {
            for (auto n : chld)
                w->call(n);
        }

        void replaceChild(AstNodeB* old, AstNodeB* nw)
        {
            for (T*& n : chld) //specify ref type to be able to modify it
                if (n == old)
                {
                    n = dynamic_cast<T*>(nw);
                    return;
                }
            assert(false && "didn't find child");
        }

        T*& getChild(int n)
        {
            return chld[n];
        }

        void setChild(int n, T* c)
        {
            c->parent = this;
            chld[n] = c;
        }

        void appendChild(T* c)
        {
            c->parent = this;
            chld.push_back(c);
        }

        void emitDot(std::ostream &os)
        {
            int p = 0;
            for (auto n : chld)
            {
                os << 'n' << static_cast<AstNodeB*>(this) << ":p" << p <<  " -> n" << static_cast<AstNodeB*>(n) << ";\n";
                ++p;
                n->emitDot(os);
            }
            os << 'n' << static_cast<AstNodeB*>(this) << " [label=\"" << myLbl();
            for (unsigned int p = 0; p < chld.size(); ++p)
                os << "|<p" << p << ">     ";
            os << "\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

        void consume(ListNode<T>* bb)
        {
            chld.insert(chld.end(), bb->chld.begin(), bb->chld.end());
            bb->chld.clear();
            delete bb;
        }
    };

    //basic block class that holds any number of instructions
    struct BasicBlock : public ListNode<Expr>
    {
        std::string myLbl() {return "Block";}
    };

    struct Package : public ListNode<FunctionDef>
    {
        std::string myLbl() {return "Package";}
    };
}

#endif
