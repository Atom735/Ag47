static UINT rParse_Las_Log_v ( const LPCWSTR fmt, va_list args )
{
  static FILE * pF = NULL;
  if ( !fmt )
  {
    if ( pF ) { fclose ( pF ); pF = NULL; return 0; }
    return 0;
  }
  if ( !pF ) { pF = rOpenFileToWriteWith_UTF16_BOM ( L".ag47/.logs/LAS.log" ); }
  return vfwprintf ( pF, fmt, args );
}

static UINT rParse_Las_Log ( const LPCWSTR fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  UINT i = rParse_Las_Log_v ( fmt, args );
  va_end ( args );
  return i;
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



struct file_state_las
{
  struct file_map       fm;
  UINT                  iVersion;       // Версия LAS файла
  LPCWSTR               s4wOrigin;      // Путь к оригиналу файла
  UINT                  iCodePage;      // Номер кодировки
  UINT                  iLineFeed;      // Символ перехода на новую строку
  BYTE                  iSection;       // В какой секции сейчас находимся
  UINT                  nWarnings;      // Количество предупреждений
  struct file_data_ptr  t,
          aMNEM, aUNIT, aDATA, aDESC, aLine;
};



static UINT rParse_Las_SectionA ( struct file_state_las * const p )
{
  rFileData_SkipToNewLine ( &(p->t), p->iLineFeed );
  return 0;
}

static UINT rParse_Las_S_TrimWhiteSpaces ( struct file_state_las * const pL )
{
  // rFileData_SkipToNewLine ( &(p->t), p->iLineFeed );
  // return 0;
  // while ( p->aMNEM.n && ( *(p->aMNEM.p) == ' ' || *(p->aMNEM.p) == '\t' ) ) { ++(p->aMNEM.p); --(p->aMNEM.n); }
  // while ( p->aMNEM.n && ( p->aMNEM.p[p->aMNEM.n-1] == ' ' || p->aMNEM.p[p->aMNEM.n-1] == '\t' ) ) { --(p->aMNEM.n); }
  // while ( p->aUNIT.n && ( *(p->aUNIT.p) == ' ' || *(p->aUNIT.p) == '\t' ) ) { ++(p->aUNIT.p); --(p->aUNIT.n); }
  // while ( p->aUNIT.n && ( p->aUNIT.p[p->aUNIT.n-1] == ' ' || p->aUNIT.p[p->aUNIT.n-1] == '\t' ) ) { --(p->aUNIT.n); }
  // while ( p->aDATA.n && ( *(p->aDATA.p) == ' ' || *(p->aDATA.p) == '\t' ) ) { ++(p->aDATA.p); --(p->aDATA.n); }
  // while ( p->aDATA.n && ( p->aDATA.p[p->aDATA.n-1] == ' ' || p->aDATA.p[p->aDATA.n-1] == '\t' ) ) { --(p->aDATA.n); }
  // while ( p->aDESC.n && ( *(p->aDESC.p) == ' ' || *(p->aDESC.p) == '\t' ) ) { ++(p->aDESC.p); --(p->aDESC.n); }
  // while ( p->aDESC.n && ( p->aDESC.p[p->aDESC.n-1] == ' ' || p->aDESC.p[p->aDESC.n-1] == '\t' ) ) { --(p->aDESC.n); }
  // while ( p->aLine.n && ( *(p->aLine.p) == ' ' || *(p->aLine.p) == '\t' ) ) { ++(p->aLine.p); --(p->aLine.n); }
  // while ( p->aLine.n && ( p->aLine.p[p->aLine.n-1] == ' ' || p->aLine.p[p->aLine.n-1] == '\t' ) ) { --(p->aLine.n); }
  rParse_Las_Log ( L"%hc\t%.*hs\t%.*hs\t%.*hs\t%.*hs\t%s\t%u\r\n",
          pL->iSection,
          pL->aMNEM.n, pL->aMNEM.p,
          pL->aUNIT.n, pL->aUNIT.p,
          pL->aDATA.n, pL->aDATA.p,
          pL->aDESC.n, pL->aDESC.p,
          pL->s4wOrigin, pL->t.nLine-1 );
  return 0;
}

static UINT rParse_Las_S_DATA_2 ( struct file_state_las * const pL )
{
  rFileData_SkipToNewLine ( &(pL->t), pL->iLineFeed );
  return 0;
}

static UINT rParse_Las_S_DATA_1 ( struct file_state_las * const pL )
{
  rFileData_SkipToNewLine ( &(pL->t), pL->iLineFeed );
  return 0;
}

static UINT rParse_Las_S_DATA ( struct file_state_las * const pL )
{
  rFileData_SkipToNewLine ( &(pL->t), pL->iLineFeed );
  return 0;

#if 0

  if ( p->iVersion == 2 ) { return rParse_Las_S_DATA_2 ( p ); }
  if ( p->iVersion == 1 ) { return rParse_Las_S_DATA_1 ( p ); }
  p->aDATA = p->t;
  p->aDESC.n = 0;
  p->aDESC.p = NULL;
  while ( p->n )
  {
    if ( *(p->p) == p->iLineFeed || ( p->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' ) )
    {
      if ( p->aDESC.p == NULL )
      {
        rLog_Error ( L" => LAS(%u): Непредвиденный конец строки (отсутсвует двоеточие)\n", p->nLine );
        return __LINE__;
      }
      p->aDATA.n -= p->aDESC.n+1;
      p->aDESC.n -= p->n;
      p->aLine.n -= p->n;
      if ( p->iLineFeed == 0x0D0A ) { rFileData_Skip(p,2); } else { rFileData_Skip(p,1); }
      ++p->nLine;
      return rParse_Las_S_TrimWhiteSpaces ( p );
    }
    else
    if ( *(p->p) == ':' )
    {
      rFileData_Skip(p,1);
      p->aDESC.n = p->n;
      p->aDESC.p = p->p;
    }
    else { rFileData_Skip(p,1); }
  }
  rLog_Error ( L" => LAS(%u): Непредвиденный конец файла (отсутсвует двоеточие)\n", p->nLine );
  return __LINE__;
#endif
}

static UINT rParse_Las_S_UNIT ( struct file_state_las * const pL )
{
  pL->aUNIT = pL->t;
  struct file_data_ptr * const p = &(pL->t);
  while ( p->n )
  {
    if ( *(p->p) == pL->iLineFeed || ( pL->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' ) )
    {
      rLog_Error ( L" => LAS(%u): Непредвиденный конец строки (отсутсвует пробел после точки)\n", p->nLine );
      return __LINE__;
    }
    else
    if ( *(p->p) == ' ' || *(p->p) == '\t' ) { pL->aUNIT.n -= p->n; rFileData_Skip(p,1); return rParse_Las_S_DATA ( pL ); }
    else { rFileData_Skip(p,1); }
  }
  rLog_Error ( L" => LAS(%u): Непредвиденный конец файла (отсутсвует пробел после точки)\n", p->nLine );
  return __LINE__;
}

static UINT rParse_Las_S_MNEM ( struct file_state_las * const pL )
{
  pL->aLine = pL->t;
  pL->aMNEM = pL->t;
  struct file_data_ptr * const p = &(pL->t);
  // Идём до точки
  while ( p->n )
  {
    if ( *(p->p) == pL->iLineFeed || ( pL->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' ) )
    {
      rLog_Error ( L" => LAS(%u): Непредвиденный конец строки (отсутсвует точка)\n", p->nLine );
      return __LINE__;
    }
    else
    if ( *(p->p) == '.' ) { pL->aMNEM.n -= p->n; rFileData_Skip(p,1); return rParse_Las_S_UNIT ( pL ); }
    else { rFileData_Skip(p,1); }
  }
  rLog_Error ( L" => LAS(%u): Непредвиденный конец файла (отсутсвует точка)\n", p->nLine );
  return __LINE__;
}


static UINT rParse_Las_BeginOfLine ( struct file_state_las * const pL )
{
  struct file_data_ptr * const p = &(pL->t);
  P_Begin:
  rFileData_SkipWhiteSpaces ( &(pL->t), pL->iLineFeed );

  if ( p->n <= 1 )
  {
    rLog_Error ( L" => LAS(%u): Непредвиденный конец файла\n", p->nLine );
    return __LINE__;
  }

  if ( *(p->p) == '~' )
  {
    switch ( p->p[1] )
    {
      case 'V': case 'W': case 'C': case 'P': case 'O':
        pL->iSection = p->p[1];
        rFileData_SkipToNewLine ( p, pL->iLineFeed );
        goto P_Begin;
      case 'A': return rParse_Las_SectionA ( pL );
      default:
        rLog_Error ( L" => LAS(%u): Неизвестное название секции\n", p->nLine );
        return __LINE__;
    }
  }
  else
  if ( *(p->p) == '#' )
  {
    rFileData_SkipToNewLine ( p, pL->iLineFeed );
    goto P_Begin;
  }
  else
  {
    const UINT iErr = rParse_Las_S_MNEM ( pL );
    if ( iErr ) return iErr;
    goto P_Begin;
  }
}


static UINT rParse_Las ( const LPWSTR s4wPath, const LPCWSTR s4wOrigin, const LPCWSTR wszFileName )
{
  UINT iErr = 0;
  rLog ( L"Parse_LAS: %-256s ==> %-256s\n", s4wOrigin, s4wPath );
  struct file_state_las _ = { };
  if ( ( iErr = rFS_FileMapOpen ( &(_.fm), s4wPath ) ) ) { goto P_End; }
  _.iVersion            = 0;
  _.s4wOrigin           = s4wOrigin;
  UINT a1[g7CharMapCount], a2[g7CharMapCount];
  _.iCodePage           = rGetBufCodePage ( _.fm.pData, _.fm.nSize, a1, a2 );
  _.iLineFeed           = rGetBufEndOfLine ( _.fm.pData, _.fm.nSize );
  _.t.p                 = _.fm.pData;
  _.t.n                 = _.fm.nSize;
  _.t.nLine             = 1;
  _.iSection            = 0;
  _.nWarnings           = 0;
  setlocale ( LC_ALL, g7CharMapCP[_.iCodePage] );
#if 0
  static UINT nFile = 0;
  CHAR str[512];
  sprintf ( str, ".ag47/" );
  CreateDirectoryA ( str, NULL );
  sprintf ( str, ".ag47/.db" );
  CreateDirectoryA ( str, NULL );
  sprintf ( str, ".ag47/.db/%02u", (nFile/1000000) );
  CreateDirectoryA ( str, NULL );
  sprintf ( str, ".ag47/.db/%02u/%02u/", (nFile/1000000), (nFile/10000)%100 );
  CreateDirectoryA ( str, NULL );
  sprintf ( str, ".ag47/.db/%02u/%02u/%04u.las", (nFile/1000000), (nFile/10000)%100, nFile%10000 );
  ++nFile;
  FILE * const pF = fopen ( str, "wb" );
  fprintf ( pF, "# Ag47_CodePage    = %hs (%hs)%hs",
          g7CharMapCP[_.iCodePage], g7CharMapNames[_.iCodePage],
          _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  fprintf ( pF, "# Ag47_LineFeed    = %hs%hs",
          _.iLineFeed == '\r' ? "CR (Mac)" : _.iLineFeed == '\n' ? "LF (Unix)" : _.iLineFeed == 0x0D0A ? "CRLF (Windows)" : "???",
          _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  fprintf ( pF, "# Ag47_Origin      = %ls%hs",
          s4wOrigin,
          _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  fprintf ( pF, "# Ag47_Size        = %u%hs",
          _.fm.nSize,
          _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  for ( UINT k = 0; k < g7CharMapCount; ++k )
  {
    fprintf ( pF, "# Ag47_CodePage[i] = %16.16hs (% 8u % 8u)%hs",
            g7CharMapCP[k],a1[k],a2[k],
            _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  }
  fwrite ( _.p, 1, _.n, pF );
  fclose ( pF );
  goto P_End;
#endif

  rParse_Las_Log ( L"%hc\t%hs\t%hs\t%hs\t%hs\t%s\t%u\r\n",
          '#',
          "FILESETS",
          g7CharMapNames[_.iCodePage],
          g7CharMapCP[_.iCodePage],
          _.iLineFeed == '\r' ? "CR (Mac)" : _.iLineFeed == '\n' ? "LF (Unix)" : _.iLineFeed == 0x0D0A ? "CRLF (Windows)" : "???",
          _.s4wOrigin, 0 );
  for ( UINT k = 0; k < g7CharMapCount; ++k )
  {
      rParse_Las_Log ( L"%hc\t%hs\t%hs\t%u\t%u\t%s\t%u\r\n",
              '#',
              "CHARMAP_VARS",
              g7CharMapNames[k],
              a1[k],
              a2[k],
              _.s4wOrigin, 0 );

  }

  // Начало файла
  if ( ( iErr = rParse_Las_BeginOfLine ( &_ ) ) ) { goto P_End; }



  P_End:
  rFS_FileMapClose ( &_.fm );
  return iErr;
}
