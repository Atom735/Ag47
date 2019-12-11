
static FILE * rOpenFileToWriteWith_UTF16_BOM ( const LPCWSTR wszFname )
{
  FILE * const fd = _wfopen ( wszFname, L"wb" );
  if ( fd ) fwprintf ( fd, L"%c", 0xFEFF );
  return fd;
}


static UINT rGetMaxNums ( UINT const * const pData, const UINT nSize )
{
  UINT k = 0;
  for ( UINT i = 1; i < nSize; ++i ) { if ( pData[i] > pData[k] ) { k = i; } }
  return k;
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
