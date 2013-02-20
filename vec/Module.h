#ifndef MODULE_H
#define MODULE_H

#include "AstNode.h"
#include "Scope.h"

#include <map>
#include <list>
#include <string>
#include <unordered_set>

namespace lex
{
    class Lexer;
}

namespace ast
{
    struct Module : public Node1
    {
        Module(std::string fname);
        ~Module();

        void PublicImport(Module* other);
        void PublicUnImport(Module* other);
        void PrivateImport(Module* other);

        std::string name;

        std::unordered_set<Module*> imports;

        bool execStarted;

        ImportScope pub_import;
        NormalScope pub;
        ImportScope priv_import;
        NormalScope priv;
        std::list<NormalScope> scopes;

        struct ExternTypeInfo
        {
            tok::Location nameLoc;
            tok::Location argsLoc;
            ExternTypeInfo(tok::Location& nameLoc, tok::Location& argsLoc)
                : nameLoc(nameLoc), argsLoc(argsLoc) {}
            ExternTypeInfo() {}
        };

        std::map<typ::Type, ExternTypeInfo> externTypes;

        std::string myLbl() {return "Comp Unit";}

        char * buffer;
        std::string fileName;
    };
}

#endif
