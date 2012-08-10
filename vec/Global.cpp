#include "Global.h"
#include "SemaNodes.h"

#include <memory>

std::unique_ptr<GlobalData> singleton;

void GlobalData::create()
{
    assert(!singleton && "creating multiple GlobalDatas!");
    singleton.reset(new GlobalData());
    singleton->Initialize();
}

GlobalData& Global()
{
    return *singleton;
}

Ident GlobalData::addIdent(const std::string &str)
{
    TblType::iterator it = std::find(identTbl.begin(), identTbl.end(), str);
    Ident ret = mkIdent(it - identTbl.begin());
    if (it == identTbl.end())
        identTbl.push_back(move(str));
    return ret;
}

ast::Module* GlobalData::findModule(const std::string& name)
{
    for (auto mod : allModules)
        if (mod->name == name)
            return mod;
    return nullptr;
}

//don't put this in the constructor so stuff we call can access Global (ie the singleton
//is set already)
void GlobalData::Initialize()
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
    ast::TypeDef td;
    td.mapped = typ::mgr.makeList(typ::int8);
    universal.addTypeDef(reserved.string, td);
    reserved.string_t = typ::mgr.makeNamed(td.mapped, reserved.string);

    reserved.undeclared_v = new ast::DeclExpr(reserved.undeclared, typ::error, tok::Location());
    universal.addVarDef(reserved.undeclared, reserved.undeclared_v);

    reserved.intrin_v = new ast::DeclExpr(reserved.intrin, typ::error, tok::Location());
    universal.addVarDef(reserved.intrin, reserved.intrin_v);

    //HACK HACK
    for (tok::TokenType tt = tok::tilde; tt < tok::integer; tt = tok::TokenType(tt + 1))
    {
        if (tok::CanBeOverloaded(tt))
        {
            if (tt == tok::lbrace) //special case
                reserved.opIdents[tt] = addIdent("operator{}");
            else
                reserved.opIdents[tt] = addIdent(std::string("operator") + tok::Name(tt));
        }
    }
}

GlobalData::~GlobalData()
{
    //universal declarations are not part of anyone's AST
    for (auto decl : universal.varDefs)
        delete decl.second;
}
