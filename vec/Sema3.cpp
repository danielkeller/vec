#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"

#include <cassert>
#include <queue>

using namespace ast;
using namespace sa;

//convenience for overload resolution
typedef std::pair<typ::TypeCompareResult, FuncDeclExpr*> ovr_result;
bool operator<(const ovr_result& lhs, const ovr_result& rhs) {return lhs.first < rhs.first;}
typedef std::priority_queue<ovr_result> ovr_queue;


//Phase 3 is type inference, overload resolution, and template instantiation
//OR IS IT??
void Sema::Phase3()
{
    AstWalk<BasicBlock>([this] (BasicBlock* bb)
    {
        //we should be able to just go left to right here
        for (Expr* node : bb->chld)
        {
            //we could speed this up by doing it in order of decreasing frequency

            //constants. these should do impicit casts
            if (Expr* e = dynamic_cast<IntConstExpr*>(node))
                e->Type() = typ::int32; //FIXME
            else if (Expr* e = dynamic_cast<FloatConstExpr*>(node))
                e->Type() = typ::float32; //FIXME
            else if (Expr* e = dynamic_cast<StringConstExpr*>(node))
                e->Type() = cu->reserved.string_t;
            else if (AssignExpr* ae = dynamic_cast<AssignExpr*>(node))
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
            else if (OverloadCallExpr* call = dynamic_cast<OverloadCallExpr*>(node))
            {
                //may need to be adjusted because of overloading
                
                OverloadGroupDeclExpr* oGroup;
                if ((oGroup = dynamic_cast<OverloadGroupDeclExpr*>(call->func->var)) != 0)
                {
                    ovr_queue result;
                    for (auto func : oGroup->functions)
                    {
                        if (!call->owner->canSee(func->owner))
                            continue; //not visible in this scope

                        result.push(
                            std::make_pair(
                                call->getChildA()->Type().compare(func->Type().getFunc().arg()),
                                func)
                            );
                    }

                    ovr_result firstChoice = result.top();
                    result.pop(); //WRONG

                    //TODO: now try template functions
                    if (result.size() == 0)
                    {
                        err::Error(call->loc) << "function '" << cu->getIdent(oGroup->name)
                            << "' is not defined in this scope" << err::underline;
                        call->Type() = typ::error;
                    }
                    else if (!firstChoice.first)
                    {
                        err::Error(call->loc) << "no accessible instance of overloaded function '"
                            << cu->getIdent(oGroup->name) << "' matches the argument list of type "
                            << call->getChildA()->Type().to_str() << err::underline;
                        call->Type() = typ::error;
                    }
                    else if (result.size() > 1 && firstChoice.first == result.top().first)
                    {
                        err::Error ambigErr(call->loc);
                        ambigErr << "overloaded call to '" << cu->getIdent(oGroup->name)
                            << "' is ambiguous" << err::underline << err::note << "could be"
                            << firstChoice.second->loc << err::underline;

                        while (result.size() && firstChoice.first == result.top().first)
                        {
                            ambigErr << err::note << "or" << result.top().second->loc << err::underline;
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
                else
                    ; //no overload group? just type check then

            } //remember that there are also "regular" call exprs
                    /*
    if (!func->var->Type().getFunc().isValid())
        err::Error(func->loc) << "cannot call object of non-function type '"
            << func->var->Type().to_str() << '\'' << err::underline
            << expr->loc << err::caret;
            */ //this goes here somewhere
            err::Error(node->loc) << node->Type().to_str() << err::underline;
        }
        //types of exprs, for reference
        //VarExpr -> DeclExpr -> FuncDeclExpr
        //ConstExpr -> [IntConstExpr, FloatConstExpr, StringConstExpr]
        //BinExpr -> AssignExpr -> OpAssignExpr
        //UnExpr
        //IterExpr, AggExpr
        //TuplifyExpr, ListifyExpr
        //PostExpr (just ++ and --)
        //TupAccExpr, ListAccExpr
    });

    //now check stmts that want a specific type, ie return
}