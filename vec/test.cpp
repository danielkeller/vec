#include "Module.h"
#include "Global.h"
#include "Error.h"
#include "LLVM.h"

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

int main (int argc, char* argv[])
{
    std::vector<std::string> params(argv, argv + argc);

    //pause in windoze for debuggin'
#ifdef _WIN32
#ifdef _DEBUG
    atexit(pause);
#endif
#endif

    GlobalData::create();
    llvm::llvm_shutdown_obj shutdown/*(multithreaded = false)*/; //clean up llvm upon exit
    
    try
    {
#ifdef _WIN32
        char fileName[MAX_PATH] = "";
        openDlg(fileName);
        Global().ParseMainFile(fileName);
#else
        Global().ParseMainFile(params[1].c_str());
#endif
    }
    catch (err::FatalError)
    {
        err::Error(err::fatal, tok::Location()) << "could not recover from previous errors, aborting";
    }

    return 0;
}
