#ifndef _TABLES_H_
#define _TABLES_H_

namespace Ops {
enum Sop {ref_assign, plus, minus, times, div, eq,
lt, le, ne, gt, ge, lor, land, lnot, bor, bxor, band, bnot, neg, N_Sops};
//enum Stype {scalar, f32, f64, N_Stypes};
enum Spos {before, between, after, N_Sposs};
};

extern const char * sop_txt [Ops::N_Sops] [Ops::N_Sposs];
extern const char * header;

#endif
