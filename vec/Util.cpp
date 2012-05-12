#include "Util.h"

using namespace utl;

void weak_string::assign(const char * l)
{
    s_begin = s_end = l;
    while (*s_end != '\n' && *s_end != '\0')
        s_end++;
}

void weak_string::assign(std::string & str)
{
    s_begin = &*str.begin();
    s_end = &*str.end();
}
