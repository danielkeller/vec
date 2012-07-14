#ifndef COMPUNIT_H
#define COMPUNIT_H

#include "AstNode.h"
#include "Global.h"

#include <map>
#include <list>
#include <string>

namespace ast
{
    class Module : public AstNodeB
    {
        Stmt* treeHead;

    public:
        ~Module();
        Module();

        Scope global;
        std::list<Scope> scopes;

        std::string name;
        ast::Scope pub;

        //the AstNodeB interface
        void eachChild(sa::AstWalker0* w) {w->call(treeHead);}
        void replaceChild(AstNodeB*, AstNodeB* newc)
        {
            treeHead = dynamic_cast<Stmt*>(newc);
            treeHead->parent = this;
        }
        std::string myLbl() {return "Comp Unit";}
        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<AstNodeB*>(this) << " -> n"
                << static_cast<AstNodeB*>(treeHead) << ";\n";
            treeHead->emitDot(os);
            AstNodeB::emitDot(os);
        }
        void TreeHead(Stmt* head) {treeHead = head; head->parent = this;}
        Stmt* TreeHead() {return treeHead;}

        friend class GlobalData;
    };
}

#endif
