#ifndef GLOBAL_H
#define GLOBAL_H

#include <vector>
#include "Type.h"
#include "SemaNodes.h"
#include "Module.h"

typedef std::vector<std::string> TblType;

//TODO: make members private and use synchronization to better support threads
class GlobalData
{
    void Initialize();
public:
    ~GlobalData();

    //deterministically create singleton to avoid stupid circular dependencies with TypeManager
    static void create();

    TblType stringTbl;
    TblType identTbl;

    Str addString(const std::string &str);
    Ident addIdent(const std::string &str);

    std::string & getStr(Str idx) {return stringTbl[idx];}
    std::string & getIdent(Ident idx) {return identTbl[idx];}

    ast::NormalScope universal;

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

    std::list<ast::Module*> allModules;

    ast::Module* findModule(const std::string& name);

    friend GlobalData& Global();
};

//this is outside of the class for brevity of use ie
// Global().stuff
// instead of something like
// utl::GlobalData::Get().stuff
GlobalData& Global();

#endif