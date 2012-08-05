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
        //catch a few more non-extraneous types
        if (mod->externTypes.count(evar->Type()))
            mod->externTypes[evar->Type()].couldBeExtraneous = false;

        if (evar->ename == 0)
            return; //not external

        DeclExpr* realDecl = mod->priv.getVarDef(evar->ename);
        if (realDecl) //found it!
            evar->var = realDecl;
        else //undeclared!
        {
            err::Error(evar->loc) << "undeclared variable '" << Global().getIdent(evar->ename)
                << "'" << err::underline;
            evar->Type() = typ::error;
        }
        //fortunately we can sort of recover by leaving it the way it is, changing the type
        //to <error type>
    }

    //unfortunately it is not currently possible to emit errors for all undefined types. this is 
    //because it would involve emitting errors for extraneous types that are actually variables. 
    //however we can emit errors for things that must be types and get errors like 
    //"can't convert from 'foo' aka '<undeclared type>' to 'int'" for the rest

    for (auto& externType : mod->externTypes)
    {
        auto type = externType.first.getNamed();
        assert(type.isValid() && "extern type is not a named type");

        TypeDef* td = mod->priv.getTypeDef(type.name());

        if (!td)
        {
            if (!externType.second.couldBeExtraneous) //whine away!
                err::Error(externType.second.nameLoc) << "type '" << Global().getIdent(type.name())
                    << "' is undefined" << err::underline;
            continue;
        }

        //if there are arguments, it's not extraneous
        if (type.numArgs() && type.numArgs() != td->params.size())
            err::Error(externType.second.argsLoc)
                << "incorrect number of type arguments, expected 0 or "
                << td->params.size() << ", got " << type.numArgs() << err::underline;

        typ::mgr.fixExternNamed(type, td->mapped, td->params);
    }
}