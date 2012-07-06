#include "CompUnit.h"

#include <algorithm>
#include <utility>

using namespace ast;

Str CompUnit::addString(const std::string &str)
{
    TblType::iterator it = std::find(stringTbl.begin(), stringTbl.end(), str);
    Str ret = it - stringTbl.begin();
    if (it == stringTbl.end())
        stringTbl.push_back(std::move(str));
    return ret;
}

Ident CompUnit::addIdent(const std::string &str)
{
    TblType::iterator it = std::find(identTbl.begin(), identTbl.end(), str);
    Ident ret = it - identTbl.begin();
    if (it == identTbl.end())
        identTbl.push_back(std::move(str));
    return ret;
}

CompUnit::CompUnit()
    : entryPt(0)
{
    //add reserved identifiers
    reserved.null = addIdent("");
    reserved.arg = addIdent("arg");
    reserved.main = addIdent("main");
    reserved.init = addIdent("__init");
    reserved.string = addIdent("String");
    reserved.undeclared = addIdent("undeclared");
    reserved.intrin = addIdent("__intrin");

    //add pre defined stuff
    //once we add the operators, this should really be in a source file because
    //it'll get huge and cumbersome here
    TypeDef td;
    td.mapped = tm.makeList(typ::int8);
    global.addTypeDef(reserved.string, td);
    reserved.string_t = tm.finishNamed(td.mapped, reserved.string);

    reserved.undeclared_v = new DeclExpr(reserved.undeclared, typ::error, &global, tok::Location());
    global.addVarDef(reserved.undeclared, reserved.undeclared_v);

    reserved.intrin_v = new DeclExpr(reserved.intrin, typ::error, &global, tok::Location());
    global.addVarDef(reserved.intrin, reserved.intrin_v);
}

CompUnit::~CompUnit()
{
    delete reserved.undeclared_v;
}
