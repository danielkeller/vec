#ifndef COMPUNIT_H
#define COMPUNIT_H

#include "AstNode.h"
#include "Global.h"

#include <map>
#include <list>
#include <string>

namespace ast
{
    class Module : public Node0
    {
        Node0* treeHead;

    public:
        ~Module();
        Module();

        Scope global;
        std::list<Scope> scopes;

        std::string name;
        ast::Scope pub;

        //the Node0 interface
        void eachChild(sa::AstWalker0* w) {w->call(treeHead);}
        void replaceChild(Node0*, Node0* newc)
        {
            treeHead = newc;
            treeHead->parent = this;
        }
        std::string myLbl() {return "Comp Unit";}
        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << " -> n"
                << treeHead << ";\n";
            treeHead->emitDot(os);
            Node0::emitDot(os);
        }
        void TreeHead(Node0* head) {treeHead = head; head->parent = this;}
        Node0* TreeHead() {return treeHead;}

        friend class GlobalData;
    };
}

#endif
