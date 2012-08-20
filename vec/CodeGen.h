#ifndef CODEGEN_H
#define CODEGEN_H

#include "LLVM.h"

namespace cg
{
    struct CodeGen
    {
        //for now, all code goes into one file
        CodeGen(std::string outfile);

        llvm::BasicBlock* curBB;
    };
}

#endif