#include "Token.h"

using namespace tok;

bool tok::operator==(Token &lhs, TokenType rhs)
{
    return lhs.Type() == rhs;
}

bool tok::operator==(TokenType lhs, Token &rhs)
{
    return lhs == rhs.Type();
}

bool tok::operator==(Token &lhs, Token &rhs)
{
    return lhs.Type() == rhs.Type();
}

std::string Token::Name()
{
    switch (type)
    {
        case tilde:
            return "'~'";
        case tick:
            return "'`'";
        case bang:
            return "'!'";
        case at:
            return "'@'";
        case pound:
            return "'#'";
        case dollar:
            return "'$'";
        case percent:
            return "'%'";
        case caret:
            return "'^'";
        case amp:
            return "'&'";
        case ampamp:
            return "'&&'";
        case star:
            return "'*'";
        case lparen:
            return "'('";
        case rparen:
            return "')'";
        case minus:
            return "'-'";
        case minusminus:
            return "'--'";
        case plus:
            return "'+'";
        case plusplus:
            return "'++'";
        case equals:
            return "'='";
        case equalsequals:
            return "'=='";
        case notequals:
            return "'!='";
        case opequals:
            return "'" + Token(value.op).Name() + "='";
        case lbrace:
            return "'{'";
        case rbrace:
            return "'}'";
        case lsquare:
            return "'['";
        case rsquare:
            return "']'";
        case bar:
            return "'|'";
        case barbar:
            return "'||'";
        case backslash:
            return "'\\'";
        case colon:
            return "':'";
        case semicolon:
            return "';'";
        case quote:
            return "'''";
        case dquote:
            return "'\"'";
        case less:
            return "'<'";
        case lshift:
            return "'<<'";
        case notgreater:
            return "'<='";
        case greater:
            return "'>'";
        case rshift:
            return "'>>'";
        case notless:
            return "'>='";
        case comma:
            return "','";
        case dot:
            return "'.'";
        case slash:
            return "'/'";
        case question:
            return "'?'";

        case integer:
            return "integer literal";
        case floating:
            return "floating point literal";
        case stringlit:
            return "string literal";
        case identifier:
            return "identifier";
    
        case k_break:
            return "'break'";
        case k_case:
            return "'case'";
        case k_continue:
            return "'continue'";
        case k_default:
            return "'default'";
        case k_do:
            return "'do'";
        case k_else:
            return "'else'";
        case k_float:
            return "'float'";
        case k_for:
            return "'for'";
        case k_goto:
            return "'goto'";
        case k_while:
            return "'while'";
        case k_if:
            return "'if'";
        case k_int:
            return "'int'";
        case k_return:
            return "'return'";
        case k_switch:
            return "'switch'";
        case k_type:
            return "'type'";
        case k_inline:
            return "'inline'";
        case k_c_call:
            return "'c_call'";
        case k_true:
            return "'true'";
        case k_false:
            return "'false'";
        default:
            return "unknown token";
    }
}

prec::precidence Token::Precidence()
{
    switch (type)
    {
        case colon:
            return prec::call;
        case star:
        case slash:
        case percent:
            return prec::multiply;
        case minus:
        case plus:
            return prec::add;
        case dollar:
            return prec::concat;
        case lshift:
        case rshift:
            return prec::shift;
        case less:
        case notgreater:
        case greater:
        case notless:
            return prec::compare;
        case equalsequals:
        case notequals:
            return prec::equality;
        case amp:
            return prec::bitwand;
        case caret:
            return prec::bitxor;
        case bar:
            return prec::bitwor;
        case ampamp:
            return prec::logand;
        case barbar:
            return prec::logor;
        case question:
        case tilde:
            return prec::ternary;
        case equals:
        case opequals:
            return prec::assignment;
        case semicolon: //give ; lowest prec to make parsing easier
        case comma:
            return prec::comma;
        case dot: //does the do anything?
        default:
            return prec::none;
    }
}

Associativity Token::Asso_ty()
{
    switch (type)
    {
        case star:
        case slash:
        case percent:
        case minus:
        case plus:
        case dollar:
        case lshift:
        case rshift:
        case amp:
        case caret:
        case bar:
        case ampamp:
        case barbar:
            return LeftAssoc;
        case less:
        case notgreater:
        case greater:
        case notless:
        case equalsequals:
        case notequals:
        case question:
        case tilde:
            return NonAssoc;
        case equals:
        case opequals:
        case colon:
            return RightAssoc;
        case semicolon:
        case comma:
            return LeftAssoc; //maybe
        case dot: //does the do anything?
        default:
            return NonAssoc;
    }
}
