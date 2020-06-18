
static FILE * rOpenFileToWriteWith_UTF16_BOM ( LPCWSTR const wszFname )
{
  FILE * const fd = _wfopen ( wszFname, L"wb" );
  if ( fd ) fwprintf ( fd, L"%c", 0xFEFF );
  return fd;
}


enum
{
  kNewLine_Null = 0,
  kNewLine_CRLF,
  kNewLine_LF,
  kNewLine_CR,
};

/*
  return
      {1},{CRLF}        - Значение как в системах Windows
      {2},{LF}          - Значение как в системах UNIX
      {3},{CR}          - Значение как в системах Macintosh
*/
static UINT rGetBuf_NL ( BYTE const * pBuf, UINT nSize )
{
  UINT iCRLF = 0;
  UINT iLF = 0;
  UINT iCR = 0;
  while ( nSize > 1)
  {
    if ( *pBuf == '\r' ) { ++iCR; if ( iCR == 8 ) { return kNewLine_CR; }
    if ( pBuf[1] == '\n' ) { ++iCRLF; if ( iCRLF == 4 ) { return kNewLine_CRLF; } } }
    else
    if ( *pBuf == '\n' ) { ++iLF;  if ( iLF == 8 ) { return kNewLine_LF; } }
    ++pBuf; --nSize;
  }
  iCRLF *= 2;
  return (iCRLF>iCR) ? ( (iCRLF>iLF) ? kNewLine_CRLF : kNewLine_LF ) : ( (iCR>iLF) ? kNewLine_CR : kNewLine_LF );
}

static LPCSTR rGetNlName ( UINT const i )
{
  switch ( i )
  {
    case kNewLine_CRLF: return "CRLF";
    case kNewLine_LF: return "LF";
    case kNewLine_CR: return "CR";
    default: return "???";
  }
}
static LPCSTR rGetNlStr ( UINT const i )
{
  switch ( i )
  {
    case kNewLine_CRLF: return "\r\n";
    case kNewLine_LF: return "\n";
    case kNewLine_CR: return "\r";
    default: return "???";
  }
}

/* Сравнивает память и слово, возвращает длину слова при совпадении */
static UINT rStrCmpArrayAA ( const LPCSTR p, const UINT n, const LPCSTR p2 )
{
  UINT i = 0;
  for ( ; i < n && p2[i]; ++i ) { if ( p[i]!= p2[i] ) { return 0; } }
  if ( p2[i] ) { return 0; }
  return i;
}
/* Сравнивает память и слово, проверяя границу, возвращает длину слова при совпадении */
static UINT rStrCmpWordAA ( const LPCSTR p, const UINT n, const LPCSTR p2 )
{
  UINT i = 0;
  for ( ; i < n && p2[i]; ++i ) { if ( p[i]!= p2[i] ) { return 0; } }
  if ( p2[i] || ( isalnum ( p[i] ) || p[i] == '_' || *p == '-' ) ) { return 0; }
  return i;
}
/* Сравнивает память и слово игнорируя регистр, проверяя границу, возвращает длину слова при совпадении */
static UINT rStrCmpCaseWordAA ( const LPCSTR p, const UINT n, const LPCSTR p2 )
{
  UINT i = 0;
  for ( ; i < n && p2[i]; ++i ) { if ( tolower ( p[i] ) != tolower ( p2[i] ) ) { return 0; } }
  if ( p2[i] || ( isalnum ( p[i] ) || p[i] == '_' || *p == '-' ) ) { return 0; }
  return i;
}

/* Указатель на бинарыне данные в памяти */
struct mem_ptr_bin
{
  BYTE          const * p;              // Указатель на данные
  UINT                  n;              // Сколько данных осталось
};

static BOOL rStrIsSpace ( BYTE const i )
{
  return i == '\r' || i == '\n' || i == '\t' || i == ' ' ;
}

/* Смещает указатель на n байт, возвращает TRUE если смог сместить */
static BOOL rMemPtrBin_Skip ( struct mem_ptr_bin * const p, UINT const n )
{ if ( p->n >= n ) { p->p += n; p->n -= n; return TRUE; } return FALSE; }

/* Смещает указатель на один байт если найден байт из массива байт */
static BOOL rMemPtrBin_Skip_1ByteIfArray ( struct mem_ptr_bin * const p, BYTE const * const s, UINT const n )
{ for ( UINT i = 0; i < n; ++i ) { if ( p->n && *(p->p) == s[i] ) { rMemPtrBin_Skip ( p, 1 ); return TRUE; } } return FALSE; }
/* Смещает указатель на один байт если не найден байт из массива */
static BOOL rMemPtrBin_Skip_1ByteIfNotArray ( struct mem_ptr_bin * const p, BYTE const * const s, UINT const n )
{ for ( UINT i = 0; i < n; ++i ) { if ( p->n && *(p->p) == s[i] ) { return FALSE; } } rMemPtrBin_Skip ( p, 1 ); return TRUE; }

/* Смещает указатель на один байт если isspace() */
static BOOL rMemPtrBin_Skip_1ByteIfSpace ( struct mem_ptr_bin * const p )
{ return ( p->n && rStrIsSpace ( *(p->p) ) && rMemPtrBin_Skip ( p, 1 ) ); }
/* Смещает указатель на один байт если не isspace() */
static BOOL rMemPtrBin_Skip_1ByteIfNotSpace ( struct mem_ptr_bin * const p )
{ return ( p->n && !rStrIsSpace ( *(p->p) ) && rMemPtrBin_Skip ( p, 1 ) ); }
/* Пропускает все isspace() байты, возвращает количество оставшихся байт */
static UINT rMemPtrBin_Skip_ToFirstNonSpace ( struct mem_ptr_bin * const p )
{ while ( rMemPtrBin_Skip_1ByteIfSpace ( p ) ); return p->n; }
/* Пропускает все байты кроме isspace(), возвращает количество оставшихся байт */
static UINT rMemPtrBin_Skip_ToFirstSpace ( struct mem_ptr_bin * const p )
{ while ( rMemPtrBin_Skip_1ByteIfNotSpace ( p ) ); return p->n; }

/* Смещает указатель к началу новой строки, возвращает количество оставшихся байт */
static UINT rMemPtrBin_Skip_ToBeginNewLine ( struct mem_ptr_bin * const p )
{ while ( rMemPtrBin_Skip_1ByteIfNotArray ( p, (BYTE const *)("\n\r"), 2 ) ); return rMemPtrBin_Skip_ToFirstNonSpace ( p ); }



/* Указатель на текстовые ASCII данные в памяти */
struct mem_ptr_txt
{
  union
  {
    struct mem_ptr_bin  _;
    struct
    {
      LPCSTR            p;              // Указатель на данные
      UINT              n;              // Сколько данных осталось
    };
  };
  UINT                  nLine;          // Номер строки данных
  UINT                  iNL;            // Символ новой строки
};

/* Возвращает длину символа перехода на новую строку если указатель указывает на символ новой строки */
static UINT rMemPtrTxt_IsNewLine ( struct mem_ptr_txt const * const p )
{
  switch ( p->iNL )
  {
    case kNewLine_CRLF: return ((p->n>=2) && (p->p[0] == '\r') && (p->p[1] == '\n')) ? 2 : 0;
    case kNewLine_CR: return ((p->n) && (p->p[0] == '\r')) ? 1 : 0;
    case kNewLine_LF: return ((p->n) && (p->p[0] == '\n')) ? 1 : 0;
    default: return 0;
  }
}

/* Смещает указатель на n байт, возвращает TRUE если смог сместить, без проверок на переход на новую строку */
static BOOL rMemPtrTxt_Skip_NoValid ( struct mem_ptr_txt * const p, UINT const n )
{ return rMemPtrBin_Skip ( &(p->_), n ); }
/* Смещает указатель на n байт, возвращает TRUE если смог сместить, с проверкой на переход на новую строку */
static BOOL rMemPtrTxt_Skip ( struct mem_ptr_txt * const p, UINT n )
{
  if ( p->n >= n )
  {
    while ( n )
    {
      const UINT i = rMemPtrTxt_IsNewLine ( p );
      if ( i ) { rMemPtrTxt_Skip_NoValid ( p, i ); ++(p->nLine); if ( n < i ) { return TRUE; } else { n -= i; } }
      else { rMemPtrTxt_Skip_NoValid ( p, 1 ); --n; }
    }
    return TRUE;
  }
  return FALSE;
}

/* Смещает указатель на один байт если найден байт из массива байт */
static BOOL rMemPtrTxt_Skip_1ByteIfArray ( struct mem_ptr_txt * const p, LPCSTR const s, const UINT n )
{ for ( UINT i = 0; i < n; ++i ) { if ( p->n && *(p->p) == s[i] ) { rMemPtrTxt_Skip ( p, 1 ); return TRUE; } } return FALSE; }
/* Смещает указатель на один байт если не найден байт из массива */
static BOOL rMemPtrTxt_Skip_1ByteIfNotArray ( struct mem_ptr_txt * const p, LPCSTR const s, const UINT n )
{ for ( UINT i = 0; i < n; ++i ) { if ( p->n && *(p->p) == s[i] ) { return FALSE; } } rMemPtrTxt_Skip ( p, 1 ); return TRUE; }
/* Смещает указатель на один байт если найден байт из массива байт */
static BOOL rMemPtrTxt_Skip_1ByteIfArraySz ( struct mem_ptr_txt * const p, LPCSTR const s )
{ return rMemPtrTxt_Skip_1ByteIfArray ( p, s, strlen ( s ) ); }
/* Смещает указатель на один байт если не найден байт из массива */
static BOOL rMemPtrTxt_Skip_1ByteIfNotArraySz ( struct mem_ptr_txt * const p, LPCSTR const s )
{ return rMemPtrTxt_Skip_1ByteIfNotArray ( p, s, strlen ( s ) ); }

/* Смещает указатель на один байт если isspace() */
static BOOL rMemPtrTxt_Skip_1ByteIfSpace ( struct mem_ptr_txt * const p )
{ return ( p->n && rStrIsSpace ( (BYTE)*(p->p) ) && rMemPtrTxt_Skip ( p, 1 ) ); }
/* Смещает указатель на один байт если не isspace() */
static BOOL rMemPtrTxt_Skip_1ByteIfNotSpace ( struct mem_ptr_txt * const p )
{ return ( p->n && !rStrIsSpace ( (BYTE)*(p->p) ) && rMemPtrTxt_Skip ( p, 1 ) ); }
/* Пропускает все isspace() байты, возвращает количество оставшихся байт */
static UINT rMemPtrTxt_Skip_ToFirstNonSpace ( struct mem_ptr_txt * const p )
{ while ( rMemPtrTxt_Skip_1ByteIfSpace ( p ) ); return p->n; }
/* Пропускает все байты кроме isspace(), возвращает количество оставшихся байт */
static UINT rMemPtrTxt_Skip_ToFirstSpace ( struct mem_ptr_txt * const p )
{ while ( rMemPtrTxt_Skip_1ByteIfNotSpace ( p ) ); return p->n; }

/* Смещает указатель к началу новой строки, возвращает количество оставшихся байт */
static UINT rMemPtrTxt_Skip_ToBeginNewLine ( struct mem_ptr_txt * const p )
{ while ( rMemPtrTxt_Skip_1ByteIfNotArray ( p, "\r\n", 2 ) ); return rMemPtrTxt_Skip_ToFirstNonSpace ( p ); }



/* Сравнивает память и слово, возвращает длину слова при совпадении */
static UINT rMemPtrTxt_CmpArrayA ( struct mem_ptr_txt * const p, LPCSTR const sz )
{ return rStrCmpArrayAA ( p->p, p->n, sz ); }
/* Сравнивает память и слово, проверяя границу, возвращает длину слова при совпадении */
static UINT rMemPtrTxt_CmpWordA ( struct mem_ptr_txt * const p, LPCSTR const sz )
{ return rStrCmpWordAA ( p->p, p->n, sz ); }
/* Сравнивает память и слово игнорируя регистр, проверяя границу, возвращает длину слова при совпадении */
static UINT rMemPtrTxt_CmpCaseWordA ( struct mem_ptr_txt * const p, LPCSTR const sz )
{ return rStrCmpCaseWordAA ( p->p, p->n, sz ); }

/* Пропускает все байты доходя до слова, возвращает количество оставшихся байт */
static UINT rMemPtrTxt_Skip_ToFirstCmpArrayA ( struct mem_ptr_txt * const p, LPCSTR const sz )
{ while ( !rMemPtrTxt_CmpArrayA ( p, sz ) ) { rMemPtrTxt_Skip ( p, 1 ); } return p->n; }
static UINT rMemPtrTxt_Skip_ToFirstCmpWordA ( struct mem_ptr_txt * const p, LPCSTR const sz )
{ while ( !rMemPtrTxt_CmpWordA ( p, sz ) ) { rMemPtrTxt_Skip ( p, 1 ); } return p->n; }
static UINT rMemPtrTxt_Skip_ToFirstCmpCaseWordA ( struct mem_ptr_txt * const p, LPCSTR const sz )
{ while ( !rMemPtrTxt_CmpCaseWordA ( p, sz ) ) { rMemPtrTxt_Skip ( p, 1 ); } return p->n; }

/* Удаляет пробелы спереди и сзади */
static UINT rMemPtrTxt_TrimSpaces ( struct mem_ptr_txt * const p )
{
  rMemPtrTxt_Skip_ToFirstNonSpace ( p );
  while ( p->n ) { if ( rStrIsSpace ( (BYTE)(p->p[p->n-1]) ) ) { --(p->n); } else { return p->n; } }
  return 0;
}

/* Пытается преобразовать строку в число, возвращает количество прочитаных символов */
static UINT rMemPtrTxt_GetUint ( struct mem_ptr_txt * const p, UINT * const pVal )
{
  LPSTR s;
  UINT const u = strtoul ( p->p, &s, 0 );
  UINT const d = s - p->p;
  if ( d )
  {
    if ( pVal ) { *pVal = u; }
    rMemPtrTxt_Skip_NoValid ( p, d );
    return d;
  }
  return 0;
}
/* Пытается преобразовать строку в число, возвращает количество прочитаных символов */
static UINT rMemPtrTxt_GetDouble_NoSkip ( struct mem_ptr_txt * const p, double * const pVal )
{
  LPSTR s;
  double const u = _strtod_l ( p->p, &s, g_locale_C );
  UINT const d = s - p->p;
  if ( d )
  {
    if ( pVal ) { *pVal = u; }
    return d;
  }
  return 0;
}
static UINT rMemPtrTxt_GetDouble ( struct mem_ptr_txt * const p, double * const pVal )
{
  LPSTR s;
  double const u = _strtod_l ( p->p, &s, g_locale_C );
  UINT const d = s - p->p;
  if ( d )
  {
    if ( pVal ) { *pVal = u; }
    rMemPtrTxt_Skip_NoValid ( p, d );
    return d;
  }
  return 0;
}


struct file_data_ptr    // указатель на данные файла
{
  BYTE          const * p;              // Указатель на данные
  UINT                  n;              // Сколько данных осталось
  UINT                  nLine;          // На какой линии указатель
};

static VOID rFileData_Skip ( struct file_data_ptr * const p, const UINT n )
{
  if ( p->n >= n ) { p->p += n; p->n -= n; }
  else { p->p += p->n; p->n = 0; }
}

static VOID rFileData_SkipWhiteSpaces ( struct file_data_ptr * const p, const UINT iLineFeed )
{
  while ( p->n )
  {
    if ( *(p->p) == iLineFeed )
    { ++(p->nLine); rFileData_Skip(p,1); }
    else
    if ( iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' )
    { ++(p->nLine); rFileData_Skip(p,2 ); }
    else
    if ( *(p->p) == ' ' || *(p->p) == '\t' )
    { rFileData_Skip(p,1); }
    else
    { return; }
  }
}
static VOID rFileData_SkipToNewLine ( struct file_data_ptr * const p, const UINT iLineFeed )
{
  while ( p->n )
  {
    if ( *(p->p) == iLineFeed )
    { ++(p->nLine); rFileData_Skip(p,1); return; }
    else
    if ( iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' )
    { ++(p->nLine); rFileData_Skip(p,2); return; }
    else
    { rFileData_Skip(p,1); }
  }
}

static VOID rFileData_Trim ( struct file_data_ptr * const p )
{
  while ( p->n )
  {
    if ( *(p->p) == ' ' || *(p->p) == '\t' ) { rFileData_Skip(p,1); }
    else { break; }
  }
  while ( p->n )
  {
    if ( p->p[p->n-1] == ' ' || p->p[p->n-1] == '\t' ) { --(p->n); }
    else { break; }
  }
}

static BOOL rFileData_Cmp ( struct file_data_ptr const * const p, LPCSTR sz )
{
  LPCSTR q = (LPCSTR)(p->p);
  UINT n = p->n;
  while ( *sz && n )
  {
    if ( tolower(*q) == tolower(*sz) ) { ++sz; ++q; --n; } else { break; }
  }
  return !(n || *sz);
}
