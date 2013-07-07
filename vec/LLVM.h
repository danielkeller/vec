#ifndef LLVM_H
#define LLVM_H

//disable all warnings for llvm on windows
#ifdef _WIN32
#pragma warning( push, 0 )
#endif

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Constants.h>
#include <llvm/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Instructions.h>
#include <llvm/CallingConv.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Support/ManagedStatic.h>

#ifdef _WIN32
#pragma warning( pop )
#endif

#endif
