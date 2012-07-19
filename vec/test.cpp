#include "Module.h"
#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"

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

int main ()
{
    char fileName[MAX_PATH] = "";
    openDlg(fileName);

    ast::Module mod;

    mod.name = fileName;
    auto ext = mod.name.find_first_of(".vc");
    if (ext != std::string::npos)
        mod.name.resize(ext);

    lex::Lexer l(fileName, &mod); //we need lexer's buffer for errors

    {
        par::Parser p(&l);
    } //parser goes out of scope & is destroyed

    {
        std::ofstream dot(fileName + std::string(".1.dot"));
        dot << "digraph G {\n";
        mod.emitDot(dot);
        dot << '}';
        dot.close();

        sa::Sema s(&mod);

        s.Phase1();

        std::ofstream dot2(fileName + std::string(".2.dot"));
        dot2 << "digraph G {\n";
        mod.emitDot(dot2);
        dot2 << '}';
        dot2.close();

        s.Phase2();

        std::ofstream dot3(fileName + std::string(".3.dot"));
        dot3 << "digraph G {\n";
        mod.emitDot(dot3);
        dot3 << '}';
        dot3.close();

        s.Phase3();
    } //sema goes out of scope & is destroyed

    getchar();
}
