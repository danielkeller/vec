#include "Module.h"

#include <algorithm>
#include <utility>

using namespace ast;

Module::Module()
    : Node1(nullptr, tok::Location())
{
}

Module::~Module()
{
}
