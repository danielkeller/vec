#include "CodeGen.h"
#include "AstWalker.h"
#include "SemaNodes.h"
#include "Value.h"

#include <set>

using namespace cg;
using namespace llvm;

CodeGen::CodeGen(std::string outfile)
    : curBB(nullptr)
{
    Module* output = new Module(outfile + ".bc", getGlobalContext());

    //gather up all functions
    std::set<ast::FunctionDef*> funcs;
    for (auto m : Global().allModules)
        for (auto fd : sa::Subtree<ast::FuncDeclExpr>(m))
            if (fd->Value())
                funcs.insert(fd->Value().getFunc().get());

    for (auto func : funcs)
        func->generate(*this);
}

Value* ast::FunctionDef::generate(CodeGen& gen)
{
    //Function* thisFunc = Function::Create();
    return nullptr;
}