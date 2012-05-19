#ifndef COMPUNIT_H
#define COMPUNIT_H

#include "Type.h"
#include "Scope.h"

#include <map>
#include <vector>
#include <string>

namespace ast
{
    typedef std::vector<std::string> TblType;
    typedef int Ident;
    typedef int Str;

    class CompUnit
    {
        TblType stringTbl;
        TblType identTbl;
        std::vector<Scope> scopes;

    public:
        CompUnit();

        Str addString(std::string &str);
        Ident addIdent(std::string &str);

        Scope * globalScope() {return &scopes[0];}
        Scope * makeScope()
        {
            scopes.emplace_back();
            return &scopes.back();
        }

        std::string & getStr(Str idx) {return stringTbl[idx];}
        std::string & getIdent(Ident idx) {return identTbl[idx];}
    };
}

#endif
