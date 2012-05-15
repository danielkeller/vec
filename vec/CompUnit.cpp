#include "CompUnit.h"

#include <algorithm>

using namespace ast;

int CompUnit::addString(std::string &str)
{
    TblType::iterator it = std::find(stringTbl.begin(), stringTbl.end(), str);
    int ret = it - stringTbl.begin();
    if (it == stringTbl.end())
        stringTbl.push_back(str);
    return ret;
}

int CompUnit::addIdent(std::string &str)
{
    TblType::iterator it = std::find(identTbl.begin(), identTbl.end(), str);
    int ret = it - identTbl.begin();
    if (it == identTbl.end())
        identTbl.push_back(str);
    return ret;
}
