#include "tables.h"

const char * sop_txt [Ops::N_Sops][Ops::N_Sposs] = {
{"(", " = ", ")"},
{"(", " + ", ")"},
{"(", " - ", ")"},
{"(", " * ", ")"},
{"(", " / ", ")"},
{"(", " == ", ")"},
{"(", " <  ", ")"},
{"(", " <= ", ")"},
{"(", " != ", ")"},
{"(", " >  ", ")"},
{"(", " >= ", ")"},
{"(", " || ", ")"},
{"(", " && ", ")"},
{"(!", "", ")"},
{"(", " | ", ")"},
{"(", " ^ ", ")"},
{"(", " & ", ")"},
{"(~", "", ")"},
{"(-", "", ")"},
};
/*
const char * sop_txt [Ops::N_Sops] [Ops::N_Stypes] = {
{"%s", "%s", "%s"},
{"(%s = %s)", "(%s = %s)", "(%s = %s)"},
{"(%s = %s)", "(%s = %s)", "(%s = %s)"},
{"(%s + %s)", "_mm_add_ps(%s, %s)", "_mm_add_pd(%s, %s)"},
{"(%s - %s)", "_mm_sub_ps(%s, %s)", "_mm_sub_pd(%s, %s)"},
{"(%s * %s)", "_mm_mul_ps(%s, %s)", "_mm_mul_pd(%s, %s)"},
{"(%s / %s)", "_mm_div_ps(%s, %s)", "_mm_div_pd(%s, %s)"},
{"(%s == %s)", "_mm_cmpeq_ps(%s, %s)", "_mm_cmpeq_pd(%s, %s)"},
{"(%s <  %s)", "_mm_cmplt_ps(%s, %s)", "_mm_cmplt_pd(%s, %s)"},
{"(%s <= %s)", "_mm_cmple_ps(%s, %s)", "_mm_cmple_pd(%s, %s)"},
{"(%s != %s)", "_mm_cmpneq_ps(%s, %s)", "_mm_cmpneq_pd(%s, %s)"},
{"(%s > %s)", "_mm_cmpnle_ps(%s, %s)", "_mm_cmpnle_pd(%s, %s)"},
{"(%s >= %s)", "_mm_cmpnlt_ps(%s, %s)", "_mm_cmpnlt_pd(%s, %s)"},
{"(%s || %s)", "_mm_or_ps(%s, %s)", "_mm_or_pd(%s, %s)"},
{"(%s && %s)", "_mm_and_ps(%s, %s)", "_mm_and_pd(%s, %s)"},
{"(! %s)", "(~%s)", "(~%s)"},
{"(%s | %s)", "_mm_or_ps(%s, %s)", "_mm_or_pd(%s, %s)"},
{"(%s ^ %s)", "_mm_xor_ps(%s, %s)", "_mm_xor_pd(%s, %s)"},
{"(%s & %s)", "_mm_and_ps(%s, %s)", "_mm_and_pd(%s, %s)"},
{"( ~%s)", "(~%s)", "(~%s)"},
{"(0 - %s)", "_mm_sub_ps(0, %s)", "_mm_sub_pd(0, %s)"}
};
*/

const char * header =
"#include <stdlib.h>\n"
"#include \"vsl.h\"\n"
;
/*"#include <emmintrin.h>\n"
"#ifdef VEC_MAIN\n"
"int main (int argc, char*argv[])\n"
"{\n"
"    //call vec entry point\n"
"    return 0;\n"
"}\n"
"#endif\n"
"\n";*/
