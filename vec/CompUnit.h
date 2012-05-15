#ifndef COMPUNIT_H
#define COMPUNIT_H

#include "Type.h"

#include <map>
#include <vector>
#include <string>

namespace ast
{
    typedef std::vector<std::string> TblType;
    typedef int Ident;
    typedef int Str;

    struct TypeDef
    {
        Ident name;
        std::vector<Ident> params;
        typ::Type mapped;
    };

    class CompUnit
    {
        TblType stringTbl;
        TblType identTbl;
        std::map<Ident, TypeDef> typeDefs;

    public:
        Str addString(std::string & str);
        Ident addIdent(std::string &str);
        void addTypeDef(TypeDef & td);
        TypeDef * getTypeDef(Ident name);

        std::string & getStr(Str idx) {return stringTbl[idx];}
        std::string & getIdent(Ident idx) {return identTbl[idx];}
    };
}

#endif
