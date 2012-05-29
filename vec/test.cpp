#include "CompUnit.h"
#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"

#include <cstdio>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
void openDlg(char *buf)
{
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    GetOpenFileName(&ofn);
}
#else //posix
#define MAX_PATH 256
void openDlg(char * buf)
{
    std::cout << "> ";
    std::cin.getline(buf, MAX_PATH - 1);
    return;
}
#endif

int main ()
{
    char fileName[MAX_PATH] = "";
    openDlg(fileName);

    ast::CompUnit cu;
    lex::Lexer l(fileName, &cu); //we need lexer's buffer for errors
    {
        par::Parser p(&l);
    } //parser goes out of scope & are destroyed

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

    delete cu.treeHead;
}
