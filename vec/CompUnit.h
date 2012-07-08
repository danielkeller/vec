#ifndef COMPUNIT_H
#define COMPUNIT_H

#include "Type.h"
#include "SemaNodes.h"

#include <map>
#include <list>
#include <string>

namespace ast
{
    typedef std::vector<std::string> TblType;
    typedef int Ident;
    typedef int Str;

    class CompUnit : public AstNodeB
    {
        TblType stringTbl;
        TblType identTbl;
        Stmt* treeHead;

    public:
        CompUnit();
        ~CompUnit();

        Str addString(const std::string &str);
        Ident addIdent(const std::string &str);

        Scope global;
        std::list<Scope> scopes;

        FunctionDef* entryPt;
        
        typ::TypeManager tm;

        std::string & getStr(Str idx) {return stringTbl[idx];}
        std::string & getIdent(Ident idx) {return identTbl[idx];}

        //the AstNodeB interface
        void eachChild(sa::AstWalker0* w) {w->call(treeHead);}
        void replaceChild(AstNodeB*, AstNodeB* newc) {treeHead = dynamic_cast<Stmt*>(newc);}
        std::string myLbl() {return "Comp Unit";}
        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<AstNodeB*>(this) << " -> n"
                << static_cast<AstNodeB*>(treeHead) << ";\n";
            treeHead->emitDot(os);
            AstNodeB::emitDot(os);
        }
        void TreeHead(Stmt* head) {treeHead = head; head->parent = this;}
        Stmt* TreeHead() {return treeHead;}

        //reserved identifiers. like keywords, but handled as identifiers
        //for ease of parsing. the struct is syntactic sugar
        //also, reserved things which aren't identifiers
        struct
        {
            Ident null;
            Ident arg;
            Ident main;
            Ident init; //really __init but __names are reserved
            Ident string;
            Ident undeclared;
            Ident intrin; //__intrin
            typ::Type string_t;
            DeclExpr* undeclared_v;
            DeclExpr* intrin_v;

            std::map<tok::TokenType, Ident> opIdents; //for operator+ etc
            //use some sort of hungarian notation here for clarity
        } reserved;
    };
}

#endif
