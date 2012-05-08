#ifndef TOKEN_H

#include "Location.h"

#include <string>

namespace tok
{
    namespace prec
    {
        enum precidence
        {
            none,
            postfix,
            prefix,
            call,
            multiply,
            add,
            concat,
            shift,
            compare,
            equality,
            bitand,
            bitxor,
            bitor,
            and,
            or,
            ternary,
            assignment,
            comma
        };
    };

    enum Associativity
    {
        LeftAssoc,
        NonAssoc,
        RightAssoc
    };

    enum TokenType
    {
        tilde,
        tick,
        bang,
        at,
        pound,
        dollar,
        percent,
        caret,
        and,
        andand,
        star,
        lparen,
        rparen,
        minus,
        minusminus,
        plus,
        plusplus,
        equals,
        equalsequals,
        notequals,
        opequals,
        lbrace,
        rbrace,
        lsquare,
        rsquare,
        or,
        oror,
        backslash,
        colon,
        semicolon,
        quote,
        dquote,
        less,
        lshift,
        notgreater,
        greater,
        rshift,
        notless,
        comma,
        dot,
        slash,
        question,

        integer,
        floating,
        stringlit,
        identifier,
    
        k_break,
        k_case,
        k_continue,
        k_c_call,
        k_default,
        k_do,
        k_else,
        k_false,
        k_for,
        k_goto,
        k_if,
        k_inline,
        k_int,
        k_return,
        k_switch,
        k_type,
        k_true,
        k_while,

        none,
        end
    };

    struct Token
    {
        TokenType type;
        TokenType op; //for binop-equals

        utl::weak_string text;
        union {
            double dbl_v;
            long int_v;
        } value;

        Location loc;

        std::string Name();
        prec::precidence Precidence();
        Associativity Asso_ty();
        TokenType Type() {return type;}
        Token() : type (none) {};
        Token(TokenType t) : type(t) {}
        Token(TokenType t, Location & l) : type(t), loc(l) {}
        Token(TokenType t, TokenType o, Location & l) : type(t), op(o), loc(l) {}
    };
    
    bool operator==(Token &lhs, TokenType rhs);
    bool operator==(TokenType lhs, Token &rhs);
    bool operator==(Token &lhs, Token &rhs);

    inline bool operator!=(Token &lhs, TokenType rhs) {return ! (lhs == rhs);}
    inline bool operator!=(TokenType lhs, Token &rhs) {return ! (lhs == rhs);}
    inline bool operator!=(Token &lhs, Token &rhs) {return ! (lhs == rhs);}
}

#define TOKEN_H
#endif