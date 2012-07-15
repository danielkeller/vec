#include "Module.h"

#include <algorithm>
#include <utility>

using namespace ast;

Module::Module()
    : Node0(tok::Location())
{
}

Module::~Module()
{
    delete treeHead;
}
