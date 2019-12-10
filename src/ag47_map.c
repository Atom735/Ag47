﻿
static WCHAR const g7CharMap[][0x80] =
{
  #define _D7x(_a_,_w_,_s_) [_a_-0x80] = _w_,
  #define _D7xA(_a_,_w_,_s_)
  // {
  //   #include "ag47_tbl_map_KZ1048.x"
  // },
  {
    #include "ag47_tbl_map_cp866_DOSCyrillicRussian.x"
  },
  {
    #include "ag47_tbl_map_cp1251.x"
  },
  {
    #include "ag47_tbl_map_cp855_DOSCyrillic.x"
  },
  {
    #include "ag47_tbl_map_cp10007_MacCyrillic.x"
  },
  {
    #include "ag47_tbl_map_ISO8859-5.x"
  },
  {
    #include "ag47_tbl_map_KOI8-U.x"
  },
  #undef _D7x
  #undef _D7xA
};

static CHAR const * const g7CharMapNames[] =
{
  // "KZ1048",
  "cp866_DOSCyrillicRussian",
  "cp1251_WindowsCyrillic",
  "cp855_DOSCyrillic",
  "cp10007_MacCyrillic",
  "ISO8859-5",
  "KOI8-U",
};

static UINT const g7CharMapCount = sizeof(g7CharMap)/sizeof(*g7CharMap);

static UINT const g7CodePoint_Rus1[] =
{
  #define _D7x(_0_,_n_) _n_,
  #include "ag47_tbl_rus_a.x"
  #undef _D7x
};


#define r7CodePoint_Rus1_lower(_i_,_0_)  (((_i_)>=L'а'&&(_i_)<=L'я')?(g7CodePoint_Rus1[(_i_)-L'а']):((_i_)==L'ё')?(g7CodePoint_Rus1[L'я'-L'a'+1]):(_0_))
#define r7CodePoint_Rus1_upper(_i_,_0_)  (((_i_)>=L'А'&&(_i_)<=L'Я')?(g7CodePoint_Rus1[(_i_)-L'А']):((_i_)==L'Ё')?(g7CodePoint_Rus1[L'я'-L'a'+1]):(_0_))
#define r7CodePoint_Rus1(_i_,_0_)        r7CodePoint_Rus1_lower(_i_,r7CodePoint_Rus1_upper(_i_,_0_))


struct
{
  WCHAR w[2];
  UINT u;
} static const g7CodePoint_Rus2[] =
{
  #define _D7x(_0_,_1_,_n_) {.w[0]=_0_,.w[1]=_1_,.u=_n_,},
  #include "ag47_tbl_rus_b.x"
  #undef _D7x
};


INT r7CodePoint_Rus2_rCmp ( const LPCWSTR pkey, __typeof__(*g7CodePoint_Rus2) const * const pE )
{
  return ((INT)(pkey[0]) - (INT)(pE->w[0])) ?: ((INT)(pkey[1]) - (INT)(pE->w[1])) ?: 0;
}


/*
  return
  '\r'                  - CR    (Mac)
  '\n'                  - LF    (Unix)
  0x0D0A                - CRLF  (Windows)
*/
static UINT rGetBufEndOfLine ( BYTE const * pBuf, UINT nSize )
{
  UINT iCRLF = 0;
  UINT iCR = 0;
  UINT iLF = 0;
  while ( nSize > 1)
  {
    if ( *pBuf == '\r' ) { ++iCR; if ( iCR == 8 ) { return '\r'; }
    if ( pBuf[1] == '\n' ) { ++iCRLF; if ( iCRLF == 4 ) { return 0x0D0A; } } }
    else
    if ( *pBuf == '\n' ) { ++iLF;  if ( iLF == 8 ) { return '\n'; } }
    ++pBuf; --nSize;
  }
  iCRLF *= 2;
  return (iCRLF>iCR) ? ( (iCRLF>iLF) ? 0x0D0A : '\n' ) : ( (iCR>iLF) ? '\r' : '\n' );
}
#define D7_CharCode(_a_,_i_) (g7CharMap[_i_][_a_])



static UINT rGetBufCodePageNums ( BYTE const * pBuf, UINT nSize, UINT a1[g7CharMapCount], UINT a2[g7CharMapCount] )
{
  return 0;
}
/*
  Птаемся определить кодировку
*/
static UINT rGetBufCodePage ( BYTE const * pBuf, UINT nSize )
{
  UINT a1[g7CharMapCount];
  UINT a2[g7CharMapCount];
  for ( UINT i = 0; i < g7CharMapCount; ++i )
  {
    a1[i] = a2[i] = 0;
  }

  --nSize;
  WCHAR w[2];
  for ( ; nSize; --nSize,++pBuf )
  {
    if ( ((*pBuf)&0x80) )
    {
      for ( UINT i = 0; i < g7CharMapCount; ++i )
      {
        w[0] = towlower(D7_CharCode(((*pBuf)&0x7f),i));
        w[1] = towlower(D7_CharCode(((pBuf[1])&0x7f),i));
        a1[i] += r7CodePoint_Rus1(w[0],0);
        __typeof__(*g7CodePoint_Rus2) const * const p = bsearch (
                w, g7CodePoint_Rus2,
                sizeof(g7CodePoint_Rus2)/sizeof(*g7CodePoint_Rus2),
                sizeof(*g7CodePoint_Rus2),
                (int (*)(const void *, const void *))r7CodePoint_Rus2_rCmp );
        if ( p )
        {
          a2[i] += p -> u;
        }
      }
    }
  }
  for ( UINT i = 0; i < g7CharMapCount; ++i )
  {
    rLog ( L" >>> %-16.16hs = % 10u % 10u\n", g7CharMapNames[i], a1[i], a2[i] );
  }
  UINT k = 0;
  for ( UINT i = 1; i < g7CharMapCount; ++i ) { if ( a2[i] > a2[k] ) { k = i; } }
  return k;
}


