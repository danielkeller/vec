#ifndef LEXER_H
#define LEXER_H

#include "Token.h"
#include "Util.h"

#include <vector>

namespace ast
{
    class CompUnit;
}

namespace lex
{
    //a class is used for reentrancy, in case this becomes threaded at some point
    class Lexer
    {
    public:
        Lexer(std::string fname, ast::CompUnit *cu);
        ~Lexer() {delete[] buffer;}

        tok::Token & Next();
        tok::Token & Peek() {return nextTok;}
        tok::Token & Last() {return curTok;}
        void Advance();

        bool Expect(tok::TokenType t);
        bool Expect(tok::TokenType t, tok::Token &to);
        //if we don't see t, "insert" it and return false
        //if we do, eat it (1st case) or save it (2nd case)

        void ErrUntil(tok::TokenType t);
        //throw away token until we pass t (ie a ';')

        ast::CompUnit *getCompUnit() {return compUnit;}

    private:
        char * buffer;
        std::string fileName;

        const char * curChr;

        tok::Token nextTok;
        tok::Token curTok;

        ast::CompUnit *compUnit;

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

#endif
