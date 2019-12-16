
static WCHAR const g7CharMap[][0x80] =
{
  #define _D7x(_a_,_w_,_s_) [_a_-0x80] = _w_,
  #define _D7xA(_a_,_w_,_s_)
  #define _D7xPre(_i_,_ss_,_sl_) {
  #define _D7xPost() },
  #include "ag47_maps.x"
  #undef _D7x
  #undef _D7xA
  #undef _D7xPre
  #undef _D7xPost
};

#define g7CharMapCount (sizeof(g7CharMap)/sizeof(*g7CharMap))

static CHAR const * const g7CharMapNames[g7CharMapCount] =
{
  #define _D7x(_a_,_w_,_s_)
  #define _D7xA(_a_,_w_,_s_)
  #define _D7xPre(_i_,_ss_,_sl_) _sl_,
  #define _D7xPost()
  #include "ag47_maps.x"
  #undef _D7x
  #undef _D7xA
  #undef _D7xPre
  #undef _D7xPost
};
static CHAR const * const g7CharMapCP[g7CharMapCount] =
{
  #define _D7x(_a_,_w_,_s_)
  #define _D7xA(_a_,_w_,_s_)
  #define _D7xPre(_i_,_ss_,_sl_) "."#_i_,
  #define _D7xPost()
  #include "ag47_maps.x"
  #undef _D7x
  #undef _D7xA
  #undef _D7xPre
  #undef _D7xPost
};

static _locale_t g7CharMapLocales[g7CharMapCount];

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

static UINT rLocalsInit ( )
{
  for ( UINT i = 0; i < g7CharMapCount; ++i )
  {
    g7CharMapLocales[i] = _create_locale ( LC_ALL, g7CharMapCP[i] );
    rLog ( L"%-64.64hs%hs\n", g7CharMapNames[i], setlocale ( LC_ALL, g7CharMapCP[i] ) );
  }
  setlocale ( LC_ALL, "C" );
  return 0;
}
static UINT rLocalsFree ( )
{
  for ( UINT i = 0; i < g7CharMapCount; ++i )
  {
    _free_locale ( g7CharMapLocales[i] );
  }
  return 0;
}

static UINT rGetBufCodePage ( BYTE const * pBuf, UINT nSize, UINT a1[g7CharMapCount], UINT a2[g7CharMapCount] )
{
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
  return rGetMaxNums ( a2, g7CharMapCount );
}



enum
{
  kCP_Utf16LE = 1200,
  kCP_Utf16BE = 1201,
  kCP_Utf32LE = 12000,
  kCP_Utf32BE = 12001,
  kCP_Utf8 = 65001,
};

/*
  Получает ID кодовый странице по названию
*/
static UINT rGetCodePageIdByAsciiName ( const LPCSTR sz )
{
  if ( *sz == '.' ) { return atoi(sz+1); }
  #define _D7xPre(_i_,_ss_,_sl_) \
    if ( strncasecmp ( sz, _ss_, sizeof(_ss_)-1 ) == 0 && !isalnum ( sz[sizeof(_ss_)-1] ) ) { return _i_; } \
    if ( strncasecmp ( sz, _sl_, sizeof(_sl_)-1 ) == 0 && !isalnum ( sz[sizeof(_sl_)-1] ) ) { return _i_; }
  #include "ag47_maps_names.x"
  #undef _D7xPre
  return 0;
}
/*
  Получает название кодовой страницы по ID
*/
static LPCSTR rGetCodePageNameById ( UINT const iCP )
{
  switch ( iCP )
  {
    #define _D7xPre(_i_,_ss_,_sl_) case _i_: return _ss_;
    #include "ag47_maps_names.x"
    #undef _D7xPre
    default: return NULL;
  }
}

static BOOL rIsBOM_Utf16LE ( struct mem_ptr_bin const * const p )
{
  return ( p->n >= 3 ) && ( p->p[0]==0xFF ) && ( p->p[1]==0xFE ) && ( p->p[2]!=0x00 );
}
static BOOL rIsBOM_Utf16BE ( struct mem_ptr_bin const * const p )
{
  return ( p->n >= 3 ) && ( p->p[0]==0xFE ) && ( p->p[1]==0xFF ) && ( p->p[2]!=0x00 );
}
static BOOL rIsBOM_Utf32LE ( struct mem_ptr_bin const * const p )
{
  return ( p->n >= 5 ) && ( p->p[0]==0xFF ) && ( p->p[1]==0xFE ) && ( p->p[2]==0x00 ) && ( p->p[3]==0x00 ) && ( p->p[4]!=0x00 );
}
static BOOL rIsBOM_Utf32BE ( struct mem_ptr_bin const * const p )
{
  return ( p->n >= 5 ) && ( p->p[0]==0x00 ) && ( p->p[1]==0x00 ) && ( p->p[2]==0xFE ) && ( p->p[3]==0xFF ) && ( p->p[4]!=0x00 );
}
static BOOL rIsBOM_Utf8 ( struct mem_ptr_bin const * const p )
{
  return ( p->n >= 4 ) && ( p->p[0]==0xEF ) && ( p->p[1]==0xBB ) && ( p->p[2]==0xBF ) && ( p->p[3]!=0x00 );
}


static UINT rGetBOM ( struct mem_ptr_bin * const p )
{
  if ( rIsBOM_Utf8 ( p ) )    { rMemPtrBin_Skip ( p, 3 ); return kCP_Utf8; }
  if ( rIsBOM_Utf16LE ( p ) ) { rMemPtrBin_Skip ( p, 2 ); return kCP_Utf16LE; }
  if ( rIsBOM_Utf16BE ( p ) ) { rMemPtrBin_Skip ( p, 2 ); return kCP_Utf16BE; }
  if ( rIsBOM_Utf32LE ( p ) ) { rMemPtrBin_Skip ( p, 4 ); return kCP_Utf32LE; }
  if ( rIsBOM_Utf32BE ( p ) ) { rMemPtrBin_Skip ( p, 4 ); return kCP_Utf32BE; }
  return 0;
}

