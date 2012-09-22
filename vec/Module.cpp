#include "Module.h"
#include "Error.h"
#include "Global.h"

#include <utility>
#include <fstream>

using namespace ast;

Module::Module(std::string fname)
    : Node1(nullptr, tok::Location()),
    execStarted(false),

    //set scope parents (private -> private import -> public -> public import -> universal)
    pub_import(&Global().universal),
    pub(&pub_import),
    priv_import(&pub),
    priv(&priv_import),

    fileName(fname)
{
    std::ifstream t(fileName.c_str(), std::ios_base::in | std::ios_base::binary); //to keep CR / CRLF from messing us up

    if (!t.is_open())
    {
        err::Error(tok::Location()) << "Could not open file '" << fileName << '\'';
        throw err::FatalError(); //just give up instead of printing 100000 useless errors
    }

    t.seekg(0, std::ios::end);
    std::streamoff size = t.tellg();
    buffer = new char[(size_t)size + 1];

    t.seekg(0);
    t.read(buffer, size);
    buffer[size] = 0;

    fileName = fileName.substr(fileName.find_last_of('\\') + 1);
    fileName = fileName.substr(fileName.find_last_of('/') + 1); //portability!
}

void Module::PublicImport(Module* other)
{
    imports.insert(other);
    pub_import.Import(&other->pub);
}

void Module::PublicUnImport(Module* other)
{
    imports.erase(other);
    pub_import.UnImport(&other->pub);
}

void Module::PrivateImport(Module* other)
{
    imports.insert(other);
    priv_import.Import(&other->pub);
}