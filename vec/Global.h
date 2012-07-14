#ifndef GLOBAL_H
#define GLOBAL_H

#include <vector>
#include "Type.h"
#include "SemaNodes.h"

typedef std::vector<std::string> TblType;

class GlobalData
{
    GlobalData();
public:
    ~GlobalData();

    TblType stringTbl;
    TblType identTbl;

    Str addString(const std::string &str);
    Ident addIdent(const std::string &str);

    std::string & getStr(Str idx) {return stringTbl[idx];}
    std::string & getIdent(Ident idx) {return identTbl[idx];}

    ast::Scope universal;

    //reserved identifiers. like keywords, but handled as identifiers
    //for ease of parsing. the struct is syntactic sugar
    //also, reserved things which aren't identifiers
    struct
    {
        Ident null;
        Ident arg;
        Ident main;
        Ident init; //really __init but __names are reserved
        Ident string;
        Ident undeclared;
        Ident intrin; //__intrin
        ast::DeclExpr* undeclared_v;
        ast::DeclExpr* intrin_v;
        typ::Type string_t;

        std::map<tok::TokenType, Ident> opIdents; //for operator+ etc
        //use some sort of hungarian notation here for clarity
    } reserved;

    friend GlobalData& Global();
};

GlobalData& Global();

#endif