#ifndef COMPUNIT_H
#define COMPUNIT_H

#include "AstNode.h"
#include "Global.h"

#include <map>
#include <list>
#include <string>

namespace ast
{
    class Module : public Node1
    {
    public:
        ~Module();
        Module();

        Scope global;
        std::list<Scope> scopes;

        std::string name;
        ast::Scope pub;

        std::string myLbl() {return "Comp Unit";}

        friend class GlobalData;
    };
}

#endif
