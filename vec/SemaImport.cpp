#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"

#include <cassert>
#include <queue>

using namespace ast;
using namespace sa;

void Sema::Import()
{
    //eventually, import all modules mentioned in the current one
    Module* intrinsic = Global().findModule("intrinsic");

    if (mod != intrinsic)
        mod->PrivateImport(intrinsic);

    //things to be fixed up are var exprs that point to undeclared, and decl exprs that point
    //to external types. don't just fix all types because some (many) of them are extraneous.
    //this is an unfortunate side effect of backtrack parsing
}