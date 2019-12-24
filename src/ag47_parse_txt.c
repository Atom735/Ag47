/*

Если строка совпадает с:
[\s]*Инклинометрия[\s]*
[\s]*Замер кривизны[\s]*
То это файл с инклинометрией и далее можем его начать разбирать


[\s]*Скважина[\s]*(?:N[\s]+)?([\w]+)

*/

static UINT rParse_Txt_Begin ( struct docx_state_ink * const p, struct file_map const * const fm )
{
  struct file_data_ptr fdp = { .p = fm->pData, .n = fm->nSize, .nLine = 1 };
  LPWSTR s4w = r4_alloca_s4w ( kPathMax );

  while ( fdp.n )
  {
    rFileData_SkipWhiteSpaces ( &fdp, p->iLineFeed );

    // Считываем строку
    struct file_data_ptr fdp2 = fdp;
    rFileData_SkipToNewLine ( &fdp2, p->iLineFeed );
    r4_get_count_s4w ( s4w ) = swprintf ( s4w, r4_get_memsz_s4w ( s4w ), L"%.*hs", fdp.n-fdp2.n, (LPCSTR)fdp.p );
    // Переходим на новую строку
    fdp = fdp2;


    if ( p->iS == kD74_Null && ( _wcsnicmp_l ( s4w, L"Инклинометрия", 13, g_locale_C ) == 0 ||
            _wcsnicmp_l ( s4w, L"Замер кривизны", 14, g_locale_C ) == 0 ) )
    {
      p->iS = kD74_ink;
    }
    else
    if ( p->iS == kD74_ink && _wcsnicmp_l (  s4w, L"Скважина N", 10, g_locale_C ) == 0 )
    {
      p->fWell = wcstoul ( s4w+10, NULL, 10 );
      p->iS = kD74_well;
    }
    else
    if ( p->iS == kD74_well && _wcsnicmp_l (  s4w, L"Диаметр скважины", 16, g_locale_C ) == 0 )
    {
      LPWSTR w;
      p->fDi = wcstod ( s4w+17, &w );
      p->iS = kD74_Di;
      while ( *w && p->iS == kD74_Di )
      {
        if ( _wcsnicmp_l (  w, L"Глубина башмака кондуктора", 26, g_locale_C ) == 0 )
        {
          p->fGBK = wcstod ( w+27, NULL );
          p->iS = kD74_GBK;
        }
        else
        if ( _wcsnicmp_l (  w, L"Глубина башмака", 15, g_locale_C ) == 0 )
        {
          p->fGBK = wcstod ( w+16, NULL );
          p->iS = kD74_GBK;
        }
        else
        {
          ++w;
        }
      }
    }
    else
    if ( (p->iS == kD74_Di || p->iS == kD74_GBK) && _wcsnicmp_l ( s4w, L"Угол склонения", 14, g_locale_C ) == 0 )
    {
      LPWSTR w;
      p->fAngleS = wcstod ( s4w+15, &w );
      p->iS = kD74_angle;
      UINT i = rParse_Docx_GetAngleType ( &w );
      if ( i == 0 )
      {
        rLog_Error ( L" => TXT: Невозможо узнать размерность значения угла\n" );
      }
      else
      {
        if ( ( p->bAngleS = ( i == 2 ) ) )
        {
          const double d = modf ( p->fAngleS , &(p->fAngleS) );
          p->fAngleS += d * (100.0/60.0);
        }

        while ( *w && p->iS == kD74_angle )
        {
          if ( _wcsnicmp_l (  w, L"Альтитуда", 9, g_locale_C ) == 0 )
          {
            p->fAlt = wcstod ( w+10, &w );
            p->iS = kD74_alt;
          }
          else
          {
            ++w;
          }
        }
        while ( *w && p->iS == kD74_alt )
        {
          if ( _wcsnicmp_l (  w, L"Забой", 5, g_locale_C ) == 0 )
          {
            p->fZab = wcstod ( w+6, &w );
            p->iS = kD74_zab;
          }
          else
          {
            ++w;
          }
        }
      }
    }
    else
    if ( p->iS == kD74_zab )
    {
      fwprintf ( p->pF_log, L" >>> kD74_zab %u >>> %s\r\n", p->nTbl, s4w );
      if ( _wcsnicmp_l ( s4w, L"---", 3, g_locale_C ) == 0 )
      {
        p->iS = kD74_tbl;
        p->nTbl = 0;
      }
    }
    else
    if ( p->iS == kD74_tbl )
    {
      fwprintf ( p->pF_log, L" >>> kD74_tbl %u %u %u >>> %s\r\n", p->nTbl, p->n_tr, p->n_tc, s4w );
      if ( p->nTbl == 0 )
      {
        if ( _wcsnicmp_l ( s4w, L"---", 3, g_locale_C ) == 0 )
        {
          continue;
        }
        else
        if ( _wcsnicmp_l ( s4w, L"Глубина", 7, g_locale_C ) == 0 )
        {
          p->pData = r4_malloc_s4s ( 200, sizeof(struct ink_data) );
          p->iS = kD74_tbl;
          p->nTbl = 1;
          p->n_tr = 1;
          p->n_tc = 1;
          LPWSTR w = s4w+7;
          while ( *w != '|' ) { ++w; }
          ++w;
          while ( iswspace ( *w ) ) { ++w; }
          if ( _wcsnicmp_l ( w, L"Угол", 4, g_locale_C ) == 0 )
          {
            p->iAn = 1;
            p->iAz = 2;
          }
          else
          if ( _wcsnicmp_l ( w, L"Азимут", 4, g_locale_C ) == 0 )
          {
            p->iAn = 2;
            p->iAz = 1;
          }
          fwprintf ( p->pF_log, L" >>> %s >>> %s\r\n", p->iAn==1 ? L"Угол" : L"Азимут", p->iAn!=1 ? L"Угол" : L"Азимут" );
        }
        else
        {
          p->iS = kD74_zab;
          p->nTbl = 0;
        }
      }
      else
      {
        if ( _wcsnicmp_l ( s4w, L"---", 3, g_locale_C ) == 0 )
        {
          ++(p->n_tr);
        }
        else
        if ( p->n_tr == 1 && _wcsnicmp_l ( s4w, L"м", 1, g_locale_C ) == 0 )
        {
          LPWSTR w = s4w+1;

          while ( *w != '|' ) { ++w; }
          ++w;
          while ( iswspace ( *w ) ) { ++w; }

          fwprintf ( p->pF_log, L" >>> GRAD1 %s\r\n", w );
          UINT i = rParse_Docx_GetAngleType ( &w );
          if ( i == 0 ) { rLog_Error ( L" => TXT: Невозможо узнать размерность значения %%1\n" ); }
          else
          if ( p->iAn == 1 ) { p->bAn = ( i == 2 ); }
          else { p->bAz = ( i == 2 ); }


          fwprintf ( p->pF_log, L" >>> GRAD2 %s\r\n", w );
          while ( *w != '|' ) { ++w; }
          ++w;
          while ( iswspace ( *w ) ) { ++w; }
          i = rParse_Docx_GetAngleType ( &w );
          if ( i == 0 ) { rLog_Error ( L" => TXT: Невозможо узнать размерность значения %%2\n" ); }
          else
          if ( p->iAn == 2 ) { p->bAn = ( i == 2 ); }
          else { p->bAz = ( i == 2 ); }


          fwprintf ( p->pF_log, L" >>> %s >>> %s\r\n",
                  p->iAn==1 ? (p->bAn?L"TRUE":L"FALSE") : (p->bAz?L"TRUE":L"FALSE"),
                  p->iAn!=1 ? (p->bAn?L"TRUE":L"FALSE") : (p->bAz?L"TRUE":L"FALSE") );

        }
        else
        if ( p->n_tr == 2 )
        {
          LPWSTR w = s4w;
          struct ink_data t = { };
          t.fDepth = wcstod ( w, &w );

          fwprintf ( p->pF_log, L" >>> DEPTH %f\r\n", t.fDepth );

          while ( iswspace ( *w ) ) { ++w; }
          if ( p->iAn == 1 )
          {
            if ( *w == '*' )  { t.iAn = 2; ++w; } else { t.iAn = 1; }
            t.fAn = wcstod ( w, &w );
            while ( iswspace ( *w ) ) { ++w; }
            if ( *w == '*' )  { t.iAz = 2; ++w; } else { t.iAz = 1; }
            t.fAz = wcstod ( w, &w );
          }
          else
          {
            if ( *w == '*' )  { t.iAz = 2; ++w; } else { t.iAz = 1; }
            t.fAz = wcstod ( w, &w );
            while ( iswspace ( *w ) ) { ++w; }
            if ( *w == '*' )  { t.iAn = 2; ++w; } else { t.iAn = 1; }
            t.fAn = wcstod ( w, &w );
          }
          if ( t.iAn && p->bAn )
          {
            const double d = modf ( t.fAn , &(t.fAn) );
            t.fAn += d * (100.0/60.0);
          }
          if ( t.iAz && p->bAz )
          {
            const double d = modf ( t.fAz , &(t.fAz) );
            t.fAz += d * (100.0/60.0);
          }
          fwprintf ( p->pF_log, L" >>> AN %f\r\n", t.fAn );
          fwprintf ( p->pF_log, L" >>> AZ %f\r\n", t.fAz );
          p->pData = r4_add_array_s4s ( p->pData, &t, 1 );
          p->nData = r4_get_count_s4s ( p->pData );
        }
      }
    }
  }


  return 0;
}

static UINT rParse_Txt ( struct ag47_script * const script, const LPCWSTR s4wPath, const LPCWSTR wszFileName )
{
  rLog ( L"Parse_TXT: %-256s ==> %-256s\n", script->s4wOrigin, s4wPath );
  struct file_map fm;
  UINT iErr = 0;
  if ( ( iErr = rFS_FileMapOpen ( &fm, s4wPath ) ) ) goto P_End2;

  const LPWSTR s4w1 = r4_alloca_s4w(kPathMax);
  r4_push_path_s4w_s4w ( s4w1, script->s4wPathOutLogsDir );
  UINT i = 0;
  for ( i = 0; i <= 999; ++i )
  {
    swprintf ( s4w1+r4_get_count_s4w(s4w1), kPathMax-r4_get_count_s4w(s4w1),
            L"\\%s.[%03u].log", wszFileName, i );
    FILE * const pF_log = _wfopen ( s4w1, L"rb" );
    if ( pF_log ) { fclose ( pF_log ); } else { break; }
  }

  struct docx_state_ink _ = {
    .pF_log = rOpenFileToWriteWith_UTF16_BOM ( s4w1 ),
    .s4wOrigin = script->s4wOrigin,
    .wszFileName = wszFileName,
    .d = 0,
    .iS = kD74_Null,
    .s4w = r4_alloca_s4w(kPathMax),
  };
  UINT a1[g7CharMapCount], a2[g7CharMapCount];
  _.iCodePage           = rGetBufCodePage ( fm.pData, fm.nSize, a1, a2 );
  _.iLineFeed           = rGetBufEndOfLine ( fm.pData, fm.nSize );
  setlocale ( LC_ALL, g7CharMapCP[_.iCodePage] );


  if ( ( iErr = rParse_Txt_Begin ( &_, &fm ) ) ) { goto P_End; }

  fwprintf ( _.pF_log, L"%.*hs", fm.nSize, fm.pData );

  if ( _.iS != kD7_Null )
  {
    const LPWSTR s4w3 = r4_alloca_s4w(kPathMax);
    r4_push_path_s4w_s4w ( s4w3, script->s4wPathOutLogsDir );
    swprintf ( s4w3+r4_get_count_s4w(s4w3), kPathMax-r4_get_count_s4w(s4w3),
            L"\\%s.[%03u].%u.txt", wszFileName, i, _.iS );
    FILE * const pF_log2 = rOpenFileToWriteWith_UTF16_BOM ( s4w3 );
    fwprintf (pF_log2, L"Скважина:                  \t%u\r\n", _.fWell );
    fwprintf (pF_log2, L"Диаметр скважины:          \t%f\r\n", _.fDi );
    fwprintf (pF_log2, L"Глубина башмака кондуктора:\t%f\r\n", _.fGBK );
    fwprintf (pF_log2, L"Угол склонения:            \t%f\r\n", _.fAngleS );
    fwprintf (pF_log2, L"Альтитуда:                 \t%f\r\n", _.fAlt );
    fwprintf (pF_log2, L"Забой:                     \t%f\r\n", _.fZab );
    fwprintf (pF_log2, L"Количество данных:         \t%u\r\n", _.nData );
    fwprintf (pF_log2, L"\r\n" );
    fwprintf (pF_log2, L"%-10s\t%-10s\t%-10s\r\n", L"Глубина", L"Угол", L"Азимут" );
    for ( UINT i = 0; i < _.nData; ++i )
    {
      fwprintf (pF_log2, L"%10.4f\t", _.pData[i].fDepth );
      if ( _.pData[i].iAn == 1 ) { fwprintf (pF_log2, L"%10.4f", _.pData[i].fAn ); }
      else if ( _.pData[i].iAn == 2 ) { fwprintf (pF_log2, L"*%9.4f", _.pData[i].fAn ); }
      else { fwprintf (pF_log2, L"          " ); }
      fwprintf (pF_log2, L"\t" );
      if ( _.pData[i].iAz == 1 ) { fwprintf (pF_log2, L"%10.4f", _.pData[i].fAz ); }
      else if ( _.pData[i].iAz == 2 ) { fwprintf (pF_log2, L"*%9.4f", _.pData[i].fAz ); }
      else { fwprintf (pF_log2, L"          " ); }
      fwprintf (pF_log2, L"\r\n" );
    }
    fclose ( pF_log2 );
  }

  P_End:
  fclose ( _.pF_log );

  if ( _.iS == kD7_Null )
  {
    if ( !DeleteFile ( s4w1 ) )
    {
      rLog_Error_WinAPI ( DeleteFile, GetLastError(), L"%s\n", s4w1 );
      iErr = __LINE__;
    }
  }

  rFS_FileMapClose ( &fm );
  P_End2:
  return iErr;
}
