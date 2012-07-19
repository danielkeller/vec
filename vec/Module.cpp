#include "Module.h"

#include <algorithm>
#include <utility>

using namespace ast;

Module::Module()
    : Node1(nullptr, tok::Location()),
    pub(&Global().universal), global(&pub) //set scope parents (global -> public -> universal)
{
}

Module::~Module()
{
}
