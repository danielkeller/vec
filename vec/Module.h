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

        struct ExternTypeInfo
        {
            tok::Location nameLoc;
            tok::Location argsLoc;
            bool couldBeExtraneous; //could it be a byproduct of backtracking?
            ExternTypeInfo(tok::Location& nameLoc, tok::Location& argsLoc, bool couldBeExtraneous)
                : nameLoc(nameLoc), argsLoc(argsLoc), couldBeExtraneous(couldBeExtraneous) {}
            ExternTypeInfo() {}
        };

        std::map<typ::Type, ExternTypeInfo> externTypes;

        std::string myLbl() {return "Comp Unit";}

        char * buffer;
        std::string fileName;
    };
}

#endif
