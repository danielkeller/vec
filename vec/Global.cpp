#include "Global.h"
#include "SemaNodes.h"
#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"
#include "Exec.h"
#include "CodeGen.h"

#include "LLVM.h"

#include <memory>
#include <fstream>

Ident::operator llvm::StringRef() const
{
    return llvm::StringRef(Global().getIdent(*this));
}

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

ast::Module* GlobalData::ParseFile(const char* path)
{
    allModules.emplace_back(path);
    ast::Module* mod = &allModules.back();

    mod->name = path;
    auto ext = mod->name.find_first_of(".vc");
    if (ext != std::string::npos)
        mod->name.resize(ext);

    lex::Lexer l(mod);
    par::Parser p(&l);

    std::ofstream dot(path + std::string(".1.dot"));
    dot << "digraph G {\n";
    mod->emitDot(dot);
    dot << '}';
    dot.close();

    sa::Sema s(mod);

    s.Phase1();

    std::ofstream dot2(path + std::string(".2.dot"));
    dot2 << "digraph G {\n";
    mod->emitDot(dot2);
    dot2 << '}';
    dot2.close();

    return mod;
}

//don't output diagnostics
//may add code later to suppress warnings
void GlobalData::ParseBuiltin(const char* path)
{
    allModules.emplace_back(path);
    ast::Module* mod = &allModules.back();
    mod->name = path;

    lex::Lexer l(mod);
    par::Parser p(&l);

    sa::Sema s(mod);
    s.Phase1();
}

void GlobalData::ParseMainFile(const char* path)
{
    ParseBuiltin("intrinsic");
    ast::Module* mainMod = ParseFile(path);

    sa::Sema s(mainMod);
    s.Import();
    
    sa::Exec ex(mainMod);

    std::ofstream dot(mainMod->fileName + std::string(".3.dot"));
    dot << "digraph G {\n";
    mainMod->emitDot(dot);
    dot << '}';
    dot.close();

    std::string outfile = mainMod->name + ".ll";

    cg::CodeGen gen(outfile);
}

Ident GlobalData::addIdent(const std::string &str)
{
    TblType::iterator it = std::find(identTbl.begin(), identTbl.end(), str);
    Ident ret = mkIdent(it - identTbl.begin());
    if (it == identTbl.end())
        identTbl.push_back(move(str));
    return ret;
}

std::ostream& operator<<(std::ostream& lhs, Ident& rhs)
{
    return lhs << Global().getIdent(rhs);
}

ast::Module* GlobalData::findModule(const std::string& name)
{
    for (auto& mod : allModules)
        if (mod.name == name)
            return &mod;
    return nullptr;
}

//don't put this in the constructor so stuff we call can access Global (ie the singleton
//is set already)
void GlobalData::Initialize()
{
    //add reserved identifiers
    reserved.null = addIdent("");
    reserved.arg = addIdent("arg");
    reserved.ret = addIdent("__ret");
    reserved.main = addIdent("main");
    reserved.init = addIdent("__init");
    reserved.string = addIdent("String");
    reserved.undeclared = addIdent("__undeclared");
    reserved.intrin = addIdent("__intrin");

    //add pre defined stuff
    ast::TypeDef td;
    td.mapped = typ::mgr.makeList(typ::int8);
    universal.addTypeDef(reserved.string, td);
    reserved.string_t = typ::mgr.makeNamed(td.mapped, reserved.string);

    numErrors = 0;

    //HACK HACK
    for (tok::TokenType tt = tok::tilde; tt < tok::integer; tt = tok::TokenType(tt + 1))
    {
        if (tok::CanBeOverloaded(tt))
        {
            if (tt == tok::lbrace) //special case
                reserved.opIdents[tt] = addIdent("[]");
            else
                reserved.opIdents[tt] = addIdent(tok::Name(tt));
        }
    }
}

GlobalData::~GlobalData()
{
    //TODO: allow things to register code to be called at exit?
    //universal declarations are not part of anyone's AST
    //TODO: currently there are none. Do we need universal?
    for (auto decl : universal.varDefs)
        delete decl;
}
