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

#ifdef _WIN32
#ifdef _DEBUG
void __cdecl pause()
{
    printf("Press enter...");
    getchar();
}
#endif
#endif

int main ()
{
    //pause in windoze for debuggin'
#ifdef _WIN32
#ifdef _DEBUG
    atexit(pause);
#endif
#endif

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

    return 0;
}
