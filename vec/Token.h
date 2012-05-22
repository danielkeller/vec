#ifndef TOKEN_H
#define TOKEN_H

#include "Location.h"

#include <string>

namespace tok
{
    namespace prec
    {
        enum precidence
        {
            none,
            semicolon,
            comma,
            assignment,
            ternary,
            logor,
            logand,
            equality,
            compare,
            call,
            bitwor,
            bitxor,
            bitwand,
            shift,
            concat,
            add,
            multiply,
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
        amp,
        ampamp,
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
        bar,
        barbar,
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
    
        k_agg,
        k_break,
        k_case,
        k_continue,
        k_c_call,
        k_default,
        k_do,
        k_else,
        k_false,
        k_float,
        k_for,
        k_goto,
        k_if,
        k_inline,
        k_int,
        k_return,
        k_switch,
        k_tail,
        k_type,
        k_true,
        k_while,

        none,
        end
    };

    const char* Name(TokenType type);

    struct Token
    {
        TokenType type;

        union {
            TokenType op; //for binop-equals
            long double dbl_v;
            long long int_v;
        } value;

        Location loc;

        const char* Name() {return tok::Name(type);};
        prec::precidence Precidence();
        Associativity Asso_ty();
        TokenType Type() {return type;}
        Token() : type (none) {};
        Token(TokenType t) : type(t) {}
        /*
        Token(TokenType t, Location & l) : type(t), loc(l) {}
        Token(TokenType t, TokenType o, Location & l) : type(t), loc(l) {value.op = o;}
        */
    };
    
    bool operator==(Token &lhs, TokenType rhs);
    bool operator==(TokenType lhs, Token &rhs);
    bool operator==(Token &lhs, Token &rhs);

    inline bool operator!=(Token &lhs, TokenType rhs) {return ! (lhs == rhs);}
    inline bool operator!=(TokenType lhs, Token &rhs) {return ! (lhs == rhs);}
    inline bool operator!=(Token &lhs, Token &rhs) {return ! (lhs == rhs);}
}

#endif
