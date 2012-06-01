#ifndef SEMANODES_H
#define SEMANODES_H

#include "Stmt.h"

namespace ast
{
    struct SemaStmt : public Stmt
    {
        const char *myColor() {return "8";};
    };

    struct TmpExpr : public Expr, public AstNode<>
    {
        const char *myColor() {return "8";};
        std::string myLbl () {return "tmp";};
        Expr* setBy;

        TmpExpr(Expr* sb) : Expr(sb->loc), setBy(sb){};
        
        void emitDot(std::ostream& os)
        {
            AstNode<>::emitDot(os);
            //shows a "sets" relationship
            os << 'n' << static_cast<AstNode0*>(setBy) << " -> n" << static_cast<AstNode0*>(this)
                << " [style=dotted];\n";
        };
    };

    struct ImpliedLoopStmt : public SemaStmt, public AstNode<Stmt>
    {
        std::vector<IterExpr*> targets;
        ImpliedLoopStmt(ExprStmt* arg)
            : AstNode<Stmt>(arg)
        {};
        std::string myLbl() {return "for (`)";};

        void emitDot(std::ostream& os)
        {
            AstNode<Stmt>::emitDot(os);
            for (auto n : targets)
                os << 'n' << static_cast<AstNode0*>(this) << " -> n" << static_cast<AstNode0*>(n)
                    << " [style=dotted];\n";
        };
    };

    //basic block class that holds any number of instructions
    class BasicBlock : public Stmt
    {
        typedef std::vector<AstNode0*> conts_t;

        conts_t chld;

    public:
        ~BasicBlock()
        {
            for (auto n : chld)
                delete n;
        };

        void eachChild(sa::AstWalker0 *w)
        {
            for (auto n : chld)
                w->call(n);
        }

        void replaceChild(AstNode0* old, AstNode0* nw)
        {
            for (AstNode0*& n : chld) //specify ref type to be able to modify it
                if (n == old)
                {
                    n = nw;
                    return;
                }
            assert(false && "didn't find child");
        }

        AstNode0*& getChild(int n)
        {
            return chld[n];
        }

        void setChild(int n, AstNode0* c)
        {
            c->parent = this;
            chld[n] = c;
        }

        void appendChild(AstNode0* c)
        {
            c->parent = this;
            chld.push_back(c);
        }

        void emitDot(std::ostream &os)
        {
            int p = 0;
            for (auto n : chld)
            {
                os << 'n' << static_cast<AstNode0*>(this) << ":p" << p <<  " -> n" << static_cast<AstNode0*>(n) << ";\n";
                ++p;
                n->emitDot(os);
            }
            os << 'n' << static_cast<AstNode0*>(this) << " [label=\"Block";
            for (int p = 0; p < chld.size(); ++p)
                os << "|<p" << p << ">     ";
            os << "\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

        void consume(BasicBlock* bb)
        {
            chld.insert(chld.end(), bb->chld.begin(), bb->chld.end());
            bb->chld.clear();
            delete bb;
        }
    };
}

#endif
