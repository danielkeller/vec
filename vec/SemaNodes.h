#ifndef SEMANODES_H
#define SEMANODES_H

#include "Stmt.h"

namespace ast
{
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

    //basic block class that holds any number of instructions
    struct BasicBlock : public Stmt
    {
        typedef std::vector<AstNodeB*> conts_t;

        conts_t chld;

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

        void replaceChild(AstNodeB* old, AstNodeB* nw)
        {
            for (AstNodeB*& n : chld) //specify ref type to be able to modify it
                if (n == old)
                {
                    n = nw;
                    return;
                }
            assert(false && "didn't find child");
        }

        AstNodeB*& getChild(int n)
        {
            return chld[n];
        }

        void setChild(int n, AstNodeB* c)
        {
            c->parent = this;
            chld[n] = c;
        }

        void appendChild(AstNodeB* c)
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
            os << 'n' << static_cast<AstNodeB*>(this) << " [label=\"Block";
            for (unsigned int p = 0; p < chld.size(); ++p)
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
