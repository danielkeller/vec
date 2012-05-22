#ifndef COMPUNIT_H
#define COMPUNIT_H

#include "Type.h"
#include "Stmt.h"

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

    public:
        CompUnit();

        Str addString(std::string &str);
        Ident addIdent(std::string &str);

        Scope global;

        std::string & getStr(Str idx) {return stringTbl[idx];}
        std::string & getIdent(Ident idx) {return identTbl[idx];}
    };
}

#endif
