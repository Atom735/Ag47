// Количество используемых кодировок
#define g7CharMapCount 6

// Таблица отображений из кодировки в WCHAR
static WCHAR const g7CharMap[g7CharMapCount][0x80] =
{
  #define _D7x(_a_,_w_,_s_) [_a_-0x80] = _w_,
  #define _D7xBegin(_i_,_ss_,_sl_) {
  #define _D7xEnd() },
  #include "ag47_maps.x"
  #undef _D7x
  #undef _D7xBegin
  #undef _D7xEnd
};

// Расширенные названия кодировок
static CHAR const * const g7CharMapNames[g7CharMapCount] =
{
  #define _D7xBegin(_i_,_ss_,_sl_) _sl_,
  #include "ag47_maps_names.x"
  #undef _D7xBegin
};

// Номер кодировки по стандарту MSDN
static UINT const g7CharMapId[g7CharMapCount] =
{
  #define _D7xBegin(_i_,_ss_,_sl_) _i_,
  #include "ag47_maps_names.x"
  #undef _D7xBegin
};

// Строчная запись номера кодировки по стандарту MSDN
static CHAR const * const g7CharMapCP[g7CharMapCount] =
{
  #define _D7xBegin(_i_,_ss_,_sl_) "."#_i_,
  #include "ag47_maps_names.x"
  #undef _D7xBegin
};

// Локали кодировок
static _locale_t g7CharMapLocales[g7CharMapCount];

// Загрузка всех локалей
static UINT rLocalsInit ( )
{
  for ( UINT i = 0; i < g7CharMapCount; ++i )
  {
    g7CharMapLocales[i] = _create_locale ( LC_ALL, g7CharMapCP[i] );
    rLog ( L"%-64.64hs%p\n", g7CharMapNames[i], g7CharMapLocales[i] );
  }
  setlocale ( LC_ALL, "C" );
  return 0;
}
// Освобождение всех локалей
static UINT rLocalsFree ( )
{
  for ( UINT i = 0; i < g7CharMapCount; ++i )
  {
    _free_locale ( g7CharMapLocales[i] );
  }
  return 0;
}

// =============================================================================

// Получить сокращённые данные руского символа из WCHAR
// 0xdead - если символ не русский
#define D7GetRusCompact(_a) ((_a>=L'а'&&_a<=L'я')?(_a-L'а'):(_a==L'ё')?32:((_a>=L'А'&&_a<=L'Я')?(_a-L'А'):(_a==L'Ё')?32:0xdead))

// Частота встречаемости русских пар букв в скоращённой записи 33х33
static UINT const g7CodePoint_Rus2F[33*33] =
{
  #define _D7x(_0_,_1_,_n_) [(_0_!=L'ё'?_0_-L'а':32)*33+(_1_!=L'ё'?_1_-L'а':32)] = _n_,
  #include "ag47_tbl_rus_b.x"
  #undef _D7x
};

// Получить частоту встречаемости пары букв WCHAR
static INT rGetPoint_Rus2 ( const WCHAR a, const WCHAR b )
{
  if ( a < 0x7f || b < 0x7f ) { return 0; }
  if ( D7GetRusCompact(a) != 0xdead && D7GetRusCompact(b) != 0xdead )
  { return g7CodePoint_Rus2F[D7GetRusCompact(a)*33+D7GetRusCompact(b)]; }
  return -300;
}

// Возвращает номер локали предподчтительный для буффера
// в pOutNewLine возвращает kNewLine_... - символ перевода строки
// в iOutCP возвращает массив значений рейтенга каждой кодировки
static UINT rGetBufLocale ( BYTE const * pBuf, UINT nSize, UINT * const pOutNewLine, INT * const pOutCP )
{
  UINT iCR = 0;
  UINT iLF = 0;
  INT  iCP[g7CharMapCount];
  if ( *pBuf == '\r' ) { ++iCR; } else
  if ( *pBuf == '\n' ) { ++iLF; }
  --nSize;
  for ( ; nSize; --nSize,++pBuf )
  {
    if ( *pBuf == '\r' ) { ++iCR; } else
    if ( *pBuf == '\n' ) { ++iLF; } else
    if ( ((*pBuf)&0x80) )
    {
      for ( UINT i = 0; i < g7CharMapCount; ++i )
      {
        iCP[i] += rGetPoint_Rus2(g7CharMap[i][((*pBuf)&0x7f)], g7CharMap[i][((pBuf[1])&0x7f)]);
      }
    }
  }
  if ( pOutNewLine )
  {
    if ( iCR == iLF ) { *pOutNewLine = kNewLine_CRLF; } else
    if ( iLF >= iCR ) { *pOutNewLine = kNewLine_LF; } else { *pOutNewLine = kNewLine_CR; }
  }
  if ( pOutCP ) { memcpy ( pOutCP, iCP, sizeof(iCP) ); }
  UINT k = 0;
  for ( UINT i = 1; i < g7CharMapCount; ++i ) { if ( iCP[i] > iCP[k] ) { k = i; } }
  return k;
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
  #define _D7xBegin(_i_,_ss_,_sl_) \
    if ( strncasecmp ( sz, _ss_, sizeof(_ss_)-1 ) == 0 && !isalnum ( sz[sizeof(_ss_)-1] ) ) { return _i_; } \
    if ( strncasecmp ( sz, _sl_, sizeof(_sl_)-1 ) == 0 && !isalnum ( sz[sizeof(_sl_)-1] ) ) { return _i_; }
  #include "ag47_maps_names.x"
  #undef _D7xBegin
  return 0;
}
/*
  Получает название кодовой страницы по ID
*/
static LPCSTR rGetCodePageNameById ( UINT const iCP )
{
  switch ( iCP )
  {
    #define _D7xBegin(_i_,_ss_,_sl_) case _i_: return _ss_;
    #include "ag47_maps_names.x"
    #undef _D7xBegin
    default: return NULL;
  }
}
/*
  Получает ID кодовой страницы по номеру
*/
static UINT rGetCodePageNumById ( UINT const iCP )
{
  // __COUNTER__;__COUNTER__;__COUNTER__;
  // __COUNTER__;__COUNTER__;__COUNTER__;
  // __COUNTER__;__COUNTER__;__COUNTER__;
  // __COUNTER__;__COUNTER__;__COUNTER__;
  enum { k__rGetCodePageNumById = __COUNTER__, };
  switch ( iCP )
  {
    #define _D7xBegin(_i_,_ss_,_sl_) case _i_: return __COUNTER__-k__rGetCodePageNumById;
    #include "ag47_maps_names.x"
    #undef _D7xBegin
    default: return 0;
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
