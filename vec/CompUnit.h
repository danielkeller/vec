#ifndef COMPUNIT_H

#include <vector>
#include <string>

namespace ast
{
    class CompUnit
    {
        typedef std::vector<std::string> TblType;
        TblType stringTbl;
        TblType identTbl;

    public:
        int addString(std::string & str);
        int addIdent(std::string &str);

        std::string & getStr(int idx) {return stringTbl[idx];}
        std::string & getIdent(int idx) {return identTbl[idx];}
    };
}


#define COMPUNIT_H
#endif
