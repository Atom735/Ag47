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




struct file_state_las
{
  struct file_map       fm;
  LPCWSTR               s4wOrigin;      // Путь к оригиналу файла
  UINT                  iCodePage;      // Номер кодировки
  UINT                  iLineFeed;      // Символ перехода на новую строку
  BYTE          const * p;              // Указатель на данные
  UINT                  n;              // Сколько данных осталось
  UINT                  nLine;          // На какой линии происходит обработка
  BYTE                  iSection;       // В какой секции сейчас находимся
  UINT                  nWarnings;      // Количество предупреждений
  struct
  {
    BYTE const *        p;
    UINT                n;
  } aMNEM, aUNIT, aDATA, aDESC, aLine;
};

#define D7LasSkip(_p_,_n_)  (((_p_)->p += (_n_)),((_p_)->n -= (_n_)))

static VOID rParse_Las_SkipWhiteSpaces ( struct file_state_las * const p )
{
  while ( p->n )
  {
    if ( *(p->p) == p->iLineFeed )
    { ++(p->nLine); D7LasSkip(p,1); }
    else
    if ( p->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' )
    { ++(p->nLine); D7LasSkip(p,2); }
    else
    if ( *(p->p) == ' ' || *(p->p) == '\t' )
    { D7LasSkip(p,1); }
    else
    { return; }
  }
}

static VOID rParse_Las_SkipToNewLine ( struct file_state_las * const p )
{
  while ( p->n )
  {
    if ( *(p->p) == p->iLineFeed )
    { ++(p->nLine); D7LasSkip(p,1); return; }
    else
    if ( p->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' )
    { ++(p->nLine); D7LasSkip(p,2); return; }
    else
    { D7LasSkip(p,1); }
  }
}


static UINT rParse_Las_SectionA ( struct file_state_las * const p )
{
  rParse_Las_SkipToNewLine ( p );
  return 0;
}

static UINT rParse_Las_S_TrimWhiteSpaces ( struct file_state_las * const p )
{
  // rParse_Las_SkipToNewLine ( p );
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
          p->iSection,
          p->aMNEM.n, p->aMNEM.p,
          p->aUNIT.n, p->aUNIT.p,
          p->aDATA.n, p->aDATA.p,
          p->aDESC.n, p->aDESC.p,
          p->s4wOrigin, p->nLine-1 );
  return 0;
}

static UINT rParse_Las_S_DATA ( struct file_state_las * const p )
{
  p->aDATA.n = p->n;
  p->aDATA.p = p->p;
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
      if ( p->iLineFeed == 0x0D0A ) { D7LasSkip(p,2); } else { D7LasSkip(p,1); }
      ++p->nLine;
      return rParse_Las_S_TrimWhiteSpaces ( p );
    }
    else
    if ( *(p->p) == ':' )
    {
      D7LasSkip(p,1);
      p->aDESC.n = p->n;
      p->aDESC.p = p->p;
    }
    else { D7LasSkip(p,1); }
  }
  rLog_Error ( L" => LAS(%u): Непредвиденный конец файла (отсутсвует двоеточие)\n", p->nLine );
  return __LINE__;
}

static UINT rParse_Las_S_UNIT ( struct file_state_las * const p )
{
  p->aUNIT.n = p->n;
  p->aUNIT.p = p->p;
  while ( p->n )
  {
    if ( *(p->p) == p->iLineFeed || ( p->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' ) )
    {
      rLog_Error ( L" => LAS(%u): Непредвиденный конец строки (отсутсвует пробел после точки)\n", p->nLine );
      return __LINE__;
    }
    else
    if ( *(p->p) == ' ' || *(p->p) == '\t' ) { p->aUNIT.n -= p->n; D7LasSkip(p,1); return rParse_Las_S_DATA ( p ); }
    else { D7LasSkip(p,1); }
  }
  rLog_Error ( L" => LAS(%u): Непредвиденный конец файла (отсутсвует пробел после точки)\n", p->nLine );
  return __LINE__;
}

static UINT rParse_Las_S_MNEM ( struct file_state_las * const p )
{
  p->aLine.n = p->n;
  p->aLine.p = p->p;
  p->aMNEM.n = p->n;
  p->aMNEM.p = p->p;
  // Идём до точки
  while ( p->n )
  {
    if ( *(p->p) == p->iLineFeed || ( p->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' ) )
    {
      rLog_Error ( L" => LAS(%u): Непредвиденный конец строки (отсутсвует точка)\n", p->nLine );
      return __LINE__;
    }
    else
    if ( *(p->p) == '.' ) { p->aMNEM.n -= p->n; D7LasSkip(p,1); return rParse_Las_S_UNIT ( p ); }
    else { D7LasSkip(p,1); }
  }
  rLog_Error ( L" => LAS(%u): Непредвиденный конец файла (отсутсвует точка)\n", p->nLine );
  return __LINE__;
}


static UINT rParse_Las_BeginOfLine ( struct file_state_las * const p )
{
  P_Begin:
  rParse_Las_SkipWhiteSpaces ( p );

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
        p->iSection = p->p[1];
        rParse_Las_SkipToNewLine ( p );
        goto P_Begin;
      case 'A': return rParse_Las_SectionA ( p );
      default:
        rLog_Error ( L" => LAS(%u): Неизвестное название секции\n", p->nLine );
        return __LINE__;
    }
  }
  else
  if ( *(p->p) == '#' )
  {
    rParse_Las_SkipToNewLine ( p );
    goto P_Begin;
  }
  else
  {
    const UINT iErr = rParse_Las_S_MNEM ( p );
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
  _.s4wOrigin           = s4wOrigin;
  UINT a1[g7CharMapCount], a2[g7CharMapCount];
  _.iCodePage           = rGetBufCodePage ( _.fm.pData, _.fm.nSize, a1, a2 );
  _.iLineFeed           = rGetBufEndOfLine ( _.fm.pData, _.fm.nSize );
  _.p                   = _.fm.pData;
  _.n                   = _.fm.nSize;
  _.nLine               = 1;
  _.iSection            = 0;
  setlocale ( LC_ALL, g7CharMapCP[_.iCodePage] );

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
