#ifndef PARSEUTILS_H
#define PARSEUTILS_H

#include "Lexer.h"
#include "Error.h"

namespace par
{
    //calls Action on a comma separated list satisfying Predicate
    //before calling, l->Next() should return the first item
    //at the beginning of Action, it will return the first token 
    //in the list, or the one after a , . When action returns, it
    //should return a , or end of list, where it is expected.
    //contsName is used in error messages ie "list of X"
    template<class Predicate, class Action>
    void parseListOf(lex::Lexer *l, Predicate P, Action &A, tok::TokenType endTok, const char * contsName)
    {
        tok::Location beginLoc = l->Peek().loc;
        while(P(l->Peek()))
        {
            A(l);
            if (!l->Expect(tok::comma))
            {
                if (P(l->Peek()))
                    err::Error(l->Peek().loc) << "missing comma in list of " << contsName << err::caret;
                else
                    break;
            }
        }
        if (!l->Expect(endTok))
            err::Error(beginLoc + l->Last().loc) << "unterminated list of " << contsName
                << "; found " << l->Peek().Name() << err::underline
                << l->Peek().loc << err::caret;// << err::endl;
    }
}


#endif
