#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "Type.h"

using namespace par;

Parser::Parser(lex::Lexer *l)
    : lexer(l)
{
    while (lexer->Peek() != tok::end)
        parseGlobalDecl();
}

void Parser::parseGlobalDecl()
{
    tok::Token t = lexer->Peek();

    if (t == tok::k_type)
        parseTypeDecl();
    else if (typ::couldBeType(t))
        parseDecl();
    else
    {
        err::Error(t.loc) << "unexpected " << t.Name() <<
        " at global level. did you forget a { ?" << err::caret << err::endl;

        //recover:
        //eat tokens until we find one more } than {
        int braceLvl = 1;
        while (braceLvl != 0 && lexer->Peek() != tok::end)
        {
            switch (lexer->Next().type)
            {
            case tok::lbrace:
                braceLvl++;
                break;
            case tok::rbrace:
                braceLvl--;
                break;
            default:
                break;
            }
        }
    }

}

void Parser::parseTypeDecl() {}
void Parser::parseDecl()
{
    typ::Type t(lexer);
    std::cerr << t.w_str() << std::endl;
}
