#include "Lexer.h"

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
    std::cin.getline(buf, MAX_PATH - 1);
    return;
}
#endif

int main ()
{
    char fileName[MAX_PATH] = "";
    openDlg(fileName);

    lex::Lexer l(fileName);

    while (l.Peek() != tok::end)
    {
        tok::Token t = l.Next();
/*        std::cout << t.loc << t.Name() << std::endl
            << t.loc.lineStr << std::endl;

        l.Peek().loc.printUnderline(std::cout, t.loc.printCaret(std::cout));

        std::cout << std::endl;*/
    }

    std::string wait;
    std::cin >> wait;
}
