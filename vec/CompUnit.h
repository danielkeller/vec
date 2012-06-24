#ifndef COMPUNIT_H
#define COMPUNIT_H

#include "Type.h"
#include "Stmt.h"

#include <map>
#include <list>
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

        Str addString(const std::string &str);
        Ident addIdent(const std::string &str);

        Scope global;
        std::list<Scope> scopes;
        Stmt* treeHead;

        typ::TypeManager tm;

        std::string & getStr(Str idx) {return stringTbl[idx];}
        std::string & getIdent(Ident idx) {return identTbl[idx];}

        //reserved identifiers. like keywords, but handled as identifiers
        //for ease of parsing. the struct is syntactic sugar
        struct
        {
            Ident null;
            Ident arg;
        } reserved;
    };
}

#endif
