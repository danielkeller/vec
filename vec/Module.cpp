#include "Module.h"

#include <algorithm>
#include <utility>

using namespace ast;

Module::Module()
{

}

Module::~Module()
{
    delete treeHead;
}
