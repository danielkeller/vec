#ifndef PARSER_H

namespace ast
{
    class CompUnit;
}

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

        ast::CompUnit *cu;

        void parseGlobalDecl();
        void parseTypeDecl();
        void parseDecl();
    };
}

#define PARSER_H
#endif
