#include "Module.h"
#include "Global.h"
#include "Error.h"

#include <cstdio>
#include <iostream>

#define MAX_PATH 256
void openDlg(char * buf)
{
    std::cout << "> ";
    std::cin.getline(buf, MAX_PATH - 1);
    return;
}

int main ()
{
    GlobalData::create();

    char fileName[MAX_PATH] = "";
    openDlg(fileName);

    try
    {
        Global().ParseMainFile(fileName);
    }
    catch (err::FatalError)
    {
        err::Error(err::fatal, tok::Location()) << "could not recover from previous errors, aborting";
    }

    //pause in windoze for debuggin'
#ifdef _WIN32
    getchar();
#endif

    return 0;
}
