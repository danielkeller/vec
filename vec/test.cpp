#include "CompUnit.h"
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

    ast::CompUnit cu;
    lex::Lexer l(fileName, &cu); //we need lexer's buffer for errors

    {
        par::Parser p(&l);
    } //parser goes out of scope & is destroyed

    {
        std::ofstream dot(fileName + std::string(".1.dot"));
        dot << "digraph G {\n";
        cu.treeHead->emitDot(dot);
        dot << '}';
        dot.close();

        sa::Sema s(&cu);

        s.Phase1();

        std::ofstream dot2(fileName + std::string(".2.dot"));
        dot2 << "digraph G {\n";
        cu.treeHead->emitDot(dot2);
        dot2 << '}';
        dot2.close();

        s.Phase2();

        std::ofstream dot3(fileName + std::string(".3.dot"));
        dot3 << "digraph G {\n";
        cu.treeHead->emitDot(dot3);
        dot3 << '}';
        dot3.close();

        s.Phase3();
    } //sema goes out of scope & is destroyed

    getchar();

    delete cu.treeHead;
}
