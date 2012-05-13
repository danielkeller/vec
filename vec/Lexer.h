#ifndef LEXER_H

#include "Token.h"
#include "Util.h"

#include <vector>

namespace lex
{
    //a class is used for reentrancy, in case this becomes threaded at some point
    class Lexer
    {
    public:
        Lexer(std::string fname);
        ~Lexer() {delete buffer;}

        tok::Token Next();
        tok::Token & Peek() {return nextTok;}
        void Advance();

        std::string & getStr(int idx) {return stringTbl[idx]:}

    private:
        char * buffer;
        std::string fileName;

        const char * curChr;

        tok::Token nextTok;

        std::vector<std::string> stringTbl;

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
        inline void lexString();
        inline char consumeChar();
    };
}

#define LEXER_H
#endif
