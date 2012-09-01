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

    //things to be fixed up are var exprs that are undeclared, and decl exprs that point 
    //to external types. we look in private because that's the highest global scope, and
    //the highest one that it's reasonable to go looking for undeclared stuff. this lets us
    //put things in private or higher scope in any order which is nice.

    for (auto evar : Subtree<VarExpr>(mod))
    {
        if (evar->ename == 0)
            continue; //not external

        DeclExpr* realDecl = mod->priv.getVarDef(evar->ename);
        if (realDecl) //found it!
            evar->var = realDecl;
        else //undeclared!
        {
            err::Error(evar->loc) << "undeclared variable '" << evar->ename
                << "'" << err::underline;
            evar->Annotate(typ::error);
        }
        //fortunately we can sort of recover by leaving it the way it is, changing the type
        //to <error type>
    }

    //now we have to go in and replace all uses of external types with their actual type
    std::map<typ::Type, typ::Type> typeReplace;

    for (auto& externType : mod->externTypes)
    {
        auto type = externType.first.getNamed();
        assert(type.isValid() && "extern type is not a named type");

        TypeDef* td = mod->priv.getTypeDef(type.name());

        if (!td) //don't complain just yet, it could be extraneous
            continue;

        //if there are arguments, it's not extraneous
        if (type.numArgs() && type.numArgs() != td->params.size())
            err::Error(externType.second.argsLoc)
                << "incorrect number of type arguments, expected 0 or "
                << td->params.size() << ", got " << type.numArgs() << err::underline;

        typeReplace[type] = typ::mgr.fixExternNamed(type, td->mapped, td->params);
    }

    for (auto n : Subtree<>(mod))
    {
        typ::NamedType type = n->Type().getNamed();
        if (type.isValid() && type.isExternal())
        {
            if (typeReplace.count(type))
                n->Annotate(typeReplace[type]);
            //remove it from externTypes so as not to emit multiple errors
            else if (mod->externTypes.count(type))
            {
                err::Error(mod->externTypes[type].nameLoc) << "type '"
                    << type.name() << "' is undefined" << err::underline;
                mod->externTypes.erase(type);
            }
        }
    }
}