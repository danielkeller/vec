#ifndef LEXER_H

#include "Token.h"
#include "Util.h"

namespace lex
{
    //a class is used for reentrancy, in case this becomes threaded at some point
    class Lexer
    {
    public:
        Lexer(std::string fname);

        tok::Token Next();
        tok::Token & Peek() {return nextTok;}
        void Advance();

    private:
        std::string buffer;
        std::string fileName;

        const char * curChr;

        tok::Token nextTok;

        inline void lexChar(tok::TokenType type); //consume single character
        inline void lexBinOp(tok::TokenType type); //consume x or x=
        inline void lexDouble(tok::TokenType type); //consume xy
            //consume xx or x= or x
        inline void lexBinDouble(tok::TokenType bintype, tok::TokenType doubletype);
            //consume xx= or xx or x= or x
        inline void lexBinAndDouble(tok::TokenType bintype, tok::TokenType doubletype);

        inline bool lexKw(const char * kw, tok::TokenType kwtype); //may not lex anything
        inline void lexIdent();
        inline void lexKwOrIdent(const char * kw, tok::TokenType kwtype);
        
        inline void lexNumber();
        inline void lexChar();
    };
}

#define LEXER_H
#endif