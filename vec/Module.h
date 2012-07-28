#ifndef MODULE_H
#define MODULE_H

#include "AstNode.h"
#include "Scope.h"

#include <map>
#include <list>
#include <string>

namespace lex
{
    class Lexer;
}

namespace ast
{
    struct Module : public Node1
    {
        Module(std::string fname);
        ~Module() {delete[] buffer;}

        void PublicImport(Module* other);
        void PrivateImport(Module* other);

        std::string name;

        ImportScope pub_import;
        NormalScope pub;
        ImportScope priv_import;
        NormalScope priv;
        std::list<NormalScope> scopes;

        std::map<typ::Type, tok::Location> externTypes;

        std::string myLbl() {return "Comp Unit";}

        char * buffer;
        std::string fileName;
    };
}

#endif
