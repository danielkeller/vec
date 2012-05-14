#ifndef PARSER_H

namespace lex
{
    class Lexer;
}

namespace par
{
    class Parser
    {
    public:
        Parser(lex::Lexer *l);

    private:
        lex::Lexer *lexer;



        void parseGlobalDecl();
        void parseTypeDecl();
        void parseDecl();
    };
}

#define PARSER_H
#endif
