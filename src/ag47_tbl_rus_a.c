
static UINT const g7CodePoint_RusA[] =
{
  #define _D7x(_0_,_n_) _n_,
  #include "ag47_tbl_rus_a.x"
  #undef _D7x
};


#define r7CodePoint_RusA_lower(_i_,_0_)  (((_i_)>=L'а'&&(_i_)<=L'я')?(g7CodePoint_RusA[(_i_)-L'a']):((_i_)==L'ё')?(g7CodePoint_RusA[L'я'-L'a'+1]):(_0_))
#define r7CodePoint_RusA_upper(_i_,_0_)  (((_i_)>=L'А'&&(_i_)<=L'Я')?(g7CodePoint_RusA[(_i_)-L'А']):((_i_)==L'Ё')?(g7CodePoint_RusA[L'я'-L'a'+1]):(_0_))
#define r7CodePoint_RusA(_i_,_0_)        D7CodePoint_TblRusA_lower(_i_,D7CodePoint_TblRusA_upper(_i_,_0_))


