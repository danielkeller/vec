#include "Module.h"
#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"
#include "Global.h"

#include <cstdio>
#include <iostream>
#include <fstream>

#define MAX_PATH 256
void openDlg(char * buf)
{
    std::cout << "> ";
    std::cin.getline(buf, MAX_PATH - 1);
    return;
}

//don't output diagnostics
//may add code later to suppress warnings
//use unique_ptr to make cleanup easier
void processBuiltin(const char * path)
{
    ast::Module* mod = new ast::Module(path);
    Global().allModules.push_back(mod);
    mod->name = path;

    lex::Lexer l(mod);
    par::Parser p(&l);

    sa::Sema s(mod);
    s.Phase1();
    s.Phase2();
}

void processFile(const char * path)
{
    ast::Module* mod = new ast::Module(path);

    Global().allModules.push_back(mod);

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

    s.Phase2();

    std::ofstream dot3(path + std::string(".3.dot"));
    dot3 << "digraph G {\n";
    mod->emitDot(dot3);
    dot3 << '}';
    dot3.close();
}

int main ()
{
    GlobalData::create();

    char fileName[MAX_PATH] = "";
    openDlg(fileName);

    processBuiltin("intrinsic");
    processFile(fileName);

    //TODO: sema3 will start with entry point and proceed recursively, via imports in the file with
    //the entry point, then called functions, etc.
    for (auto mod : Global().allModules)
    {
        sa::Sema s(mod);
        s.Import();
        s.Phase3();
    }

    //FIXME: this is not a good place for this
    for (auto mod : Global().allModules)
        delete mod;

    getchar();

    return 0;
}
