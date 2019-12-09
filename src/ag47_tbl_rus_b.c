
struct
{
  WCHAR w[2];
  UINT u;
} static const g7CodePoint_RusB[] =
{
  #define _D7x(_0_,_1_,_n_) {.w[0]=_0_,.w[1]=_1_,.u=_n_,},
  #include "ag47_tbl_rus_b.x"
  #undef _D7x
};

#define r7WideToLower(_i_) (((_i_)>=L'А'&&(_i_)<=L'Я')?((_i_)+0x20):((_i_)==L'Ё')?(L'ё'):(_i_))

static INT r7CodePoint_RusB__rCmp_eq ( const LPCWSTR pkey, const __typeof__(g7CodePoint_RusB) * const pE )
{
  return ((INT)pkey[0] - (INT)pE->w[0]) ?: ((INT)pkey[1] - (INT)pE->w[1]) ?: 0;
}
static INT r7CodePoint_RusB__rCmp_case ( const LPCWSTR pkey, const __typeof__(g7CodePoint_RusB) * const pE )
{
  return ((INT)pkey[0] - (INT)pE->w[0]) ?: ((INT)pkey[1] - (INT)pE->w[1]) ?: 0;
}
#define r7CodePoint_RusB_lower(_i_,_0_) ((((_i_)>=L'а'&&(_i_)<=L'я')||(_i_=='ё'))?(  ):(0))
#define r7CodePoint_RusB_upper(_i_,_0_)
#define r7CodePoint_RusB(_i_,_0_)


