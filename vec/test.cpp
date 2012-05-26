#include "Lexer.h"
#include "Parser.h"
#include "CompUnit.h"

#include <cstdio>
#include <iostream>
#include <time.h>

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

    lex::Lexer l(fileName, &cu);

    par::Parser p(&l);

    cu.treeHead->emitXml(std::cout);

    getchar();
}
