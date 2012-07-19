#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>
#include <queue>

using namespace ast;
using namespace sa;

//convenience for overload resolution
typedef std::pair<typ::TypeCompareResult, FuncDeclExpr*> ovr_result;
struct pairComp
{
    bool operator()(const ovr_result& lhs, const ovr_result& rhs) {return rhs.first < lhs.first;}
};
typedef std::priority_queue<ovr_result, std::vector<ovr_result>, pairComp> ovr_queue;

template<class T>
void Sema::resolveOverload(OverloadGroupDeclExpr* oGroup, T* call, typ::Type argType)
{
    ovr_queue result;
    for (auto func : oGroup->functions)
    {
        //funcScope is the function's owning scope if its a decl, funcScope->parent is
        //if its a definition
        if (!call->owner->canSee(func->funcScope) && !call->owner->canSee(func->funcScope->parent))
            continue; //not visible in this scope

        result.push(
            std::make_pair(
                argType.compare(func->Type().getFunc().arg()),
                func)
            );
    }

    //TODO: now try template functions
    if (result.size() == 0)
    {
        err::Error(call->loc) << "function '" << Global().getIdent(oGroup->name) << "' is not defined in this scope"
            << err::underline << call->loc << err::caret;
        call->Type() = typ::error;
        return;
    }

    ovr_result firstChoice = result.top();

    if (!firstChoice.first.isValid())
    {
        err::Error(call->loc) << "no accessible instance of overloaded function '" << Global().getIdent(oGroup->name)
            << "' matches arguments of type " << argType.to_str() << err::underline << call->loc << err::caret;
        call->Type() = typ::error;
    }
    else if (result.size() > 1 && (result.pop(), firstChoice.first == result.top().first))
    {
        err::Error ambigErr(call->loc);
        ambigErr << "overloaded call to '" << Global().getIdent(oGroup->name) << "' is ambiguous" << err::underline
            << call->loc << err::caret << firstChoice.second->loc << err::note << "could be" << err::underline;

        while (result.size() && firstChoice.first == result.top().first)
        {
            ambigErr << result.top().second->loc << err::note << "or" << err::underline;
            result.pop();
        }
        call->ovrResult = firstChoice.second; //recover
        call->Type() = call->ovrResult->Type().getFunc().ret();
    }
    else //success
    {
        call->ovrResult = firstChoice.second;
        call->Type() = call->ovrResult->Type().getFunc().ret();
    }
}

//Phase 3 is type inference, overload resolution, and template instantiation
//OR IS IT??
void Sema::Phase3()
{
    AstWalk<BasicBlock>([this] (BasicBlock* bb)
    {
        //we should be able to just go left to right here
        for (auto& it : bb->Children())
        {
            Node0* node = it.get();
            //we could speed this up by doing it in order of decreasing frequency

            //constants. these should do impicit casts
            if (IntConstExpr* e = exact_cast<IntConstExpr*>(node))
                e->Type() = typ::int32; //FIXME
            else if (FloatConstExpr* e = exact_cast<FloatConstExpr*>(node))
                e->Type() = typ::float32; //FIXME
            else if (StringConstExpr* e = exact_cast<StringConstExpr*>(node))
                e->Type() = Global().reserved.string_t;
            else if (AssignExpr* ae = exact_cast<AssignExpr*>(node))
            {
                //try to merge types if its a decl
                if (ae->Type().compare(ae->getChildB()->Type()) == typ::TypeCompareResult::invalid)
                {
                    err::Error(ae->getChildA()->loc) << "cannot convert from "
                        << ae->getChildB()->Type().to_str() << " to "
                        << ae->getChildA()->Type().to_str() << " in assignment"
                        << err::underline << ae->opLoc << err::caret
                        << ae->getChildB()->loc << err::underline;
                    //whew!
                }
            }
            else if (OverloadCallExpr* call = exact_cast<OverloadCallExpr*>(node))
            {
                OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(call->func->var);
                if (oGroup == 0)
                {
                    err::Error(call->func->loc) << "cannot call object of non-function type '"
                        << call->func->Type().to_str() << '\'' << err::underline;
                    call->Type() = call->func->Type(); //sorta recover
                    continue; //break out
                }

                typ::TupleBuilder builder;
                for (auto& arg : call->Children())
                    builder.push_back(arg->Type(), Global().reserved.null);
                typ::Type argType = typ::mgr.makeTuple(builder);

                resolveOverload(oGroup, call, argType);

            }
            else if (BinExpr* be = exact_cast<BinExpr*>(node))
            {
                if (be->op == tok::colon) //"normal" function call
                {
                    Node0* lhs = be->getChildA();
                    typ::FuncType ft = lhs->Type().getFunc();
                    if (!ft.isValid())
                    {
                        err::Error(be->getChildA()->loc) << "cannot call object of non-function type '"
                            << lhs->Type().to_str() << '\'' << err::underline
                            << be->opLoc << err::caret;
                        be->Type() = lhs->Type(); //sorta recover
                    }
                    else
                    {
                        be->Type() = ft.ret();
                        if (!ft.arg().compare(be->getChildB()->Type()).isValid())
                            err::Error(be->getChildA()->loc)
                                << "function arguments are inappropriate for function"
                                << ft.arg().to_str() << " != " << be->getChildB()->Type().to_str()
                                << err::underline << be->opLoc << err::caret
                                << be->getChildB()->loc << err::underline;
                    }
                }
                else if (tok::CanBeOverloaded(be->op))
                {
                    typ::TupleBuilder builder;
                    builder.push_back(be->getChildA()->Type(), Global().reserved.null);
                    builder.push_back(be->getChildB()->Type(), Global().reserved.null);
                    typ::Type argType = typ::mgr.makeTuple(builder);
                    DeclExpr* def = mod->global.getVarDef(Global().reserved.opIdents[be->op]);
                    OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(def);
                    assert(oGroup && "operator not overloaded properly");

                    resolveOverload(oGroup, be, argType);
                }
            }
            else if (ListifyExpr* le = exact_cast<ListifyExpr*>(node))
            {
                typ::Type conts_t = le->getChild(0)->Type();
                le->Type() = typ::mgr.makeList(conts_t, le->Children().size());
                for (auto& c : le->Children())
                    if (conts_t.compare(c->Type()) == typ::TypeCompareResult::invalid)
                        err::Error(le->getChild(0)->loc) << "list contents must be all the same type, " << conts_t.to_str()
                            << " != " << c->Type().to_str() << err::underline << c->loc << err::underline;
            }
            else if (TuplifyExpr* te = exact_cast<TuplifyExpr*>(node))
            {
                typ::TupleBuilder builder;
                for (auto& c : te->Children())
                    builder.push_back(c->Type(), Global().reserved.null);
                te->Type() = typ::mgr.makeTuple(builder);
            }

            //err::Error(err::warning, node->loc) << node->Type().to_str() << err::underline;
        }
        //types of exprs, for reference
        //VarExpr -> DeclExpr -> FuncDeclExpr
        //ConstExpr -> [IntConstExpr, FloatConstExpr, StringConstExpr] *
        //BinExpr -> AssignExpr * -> OpAssignExpr
        //UnExpr
        //IterExpr, AggExpr
        //TuplifyExpr, ListifyExpr
        //PostExpr (just ++ and --)
        //TupAccExpr, ListAccExpr
    });

    //now replace intrinsic calls with a special node type

    //now check stmts that want a specific type, ie return
}
