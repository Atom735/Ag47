static UINT rParse_Docx_Log_v ( const LPCWSTR fmt, va_list args )
{
  static FILE * pF = NULL;
  if ( !fmt )
  {
    if ( pF ) { fclose ( pF ); pF = NULL; return 0; }
    return 0;
  }
  if ( !pF ) { pF = rOpenFileToWriteWith_UTF16_BOM ( L".ag47/.logs/docx.log" ); }
  return vfwprintf ( pF, fmt, args );
}

static UINT rParse_Docx_Log ( const LPCWSTR fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  UINT i = rParse_Docx_Log_v ( fmt, args );
  va_end ( args );
  return i;
}

enum
{
  kD7_Null = 0,
  kD7_document,
  kD7_body,
  kD7_body_p,
  kD7_body_tbl,
  kD7_body_tbl_tr,
  kD7_body_tbl_tr_tc,
  kD7_body_tbl_tr_tc_p,
};

enum
{
  kD74_Null = 0,
  kD74_ink,
  kD74_well,
  kD74_Di,
  kD74_GBK,
  kD74_angle,
  kD74_alt,
  kD74_zab,
  kD74_tbl,
};

struct ink_data
{
  double fDepth;
  double fAn;
  double fAz;
  UINT iAn; // 0 - отсутсвет значение, 1 - нормальное значение, 2 - со звёздочкой
  UINT iAz; // 0 - отсутсвет значение, 1 - нормальное значение, 2 - со звёздочкой
};

struct docx_state_ink
{
  FILE * pF_xml;
  FILE * pF_log;
  FILE * pF_log2;
  LPCWSTR s4wOrigin;
  LPCWSTR wszFileName;
  UINT d;
  UINT i;
  UINT iS;
  LPWSTR s4w;
  UINT n_tbl;
  UINT n_tr;
  UINT n_tc;
  UINT n_p;
  UINT fWell;           // Скважина N
  double fDi;           // Диаметр скважины
  double fGBK;          // Глубина башмака кондуктора
  double fAngleS;       // Угол склонения
  BOOL bAngleS;         // град`мин
  double fAlt;          // Альтитуда
  double fZab;          // Забой

  UINT nTbl;
  BOOL bAn; // минуты ли?
  BOOL bAz; // минуты ли?
  UINT iAn; // Номер колонки для угла
  UINT iAz; // Номер колонки для азимута

  struct ink_data * pData;
  UINT nData;

  // LPWSTR ***  ppTbl;
  // LPWSTR ***  s4pppwLastTable;
  // LPWSTR **   s4ppwLastRow;
  // LPWSTR *    s4pwLastColumn;
  // LPWSTR      s4wLastParagraph;
};

static UINT rParse_Docx_GetAngleType ( LPWSTR * const w )
{
  LPWSTR s = *w;
  while ( *s )
  {
    if ( _wcsnicmp_l ( s, L"град", 4, g_locale_C ) == 0 )
    {
      s += 4;
      while ( *s )
      {
        if ( _wcsnicmp_l ( s, L"град", 4, g_locale_C ) == 0 )
        {
          *w = s+4; return 1;
        }
        else
        if ( _wcsnicmp_l ( s, L"мин", 3, g_locale_C ) == 0 )
        {
          *w = s+3; return 2;
        }
        else
        {
          ++s;
        }
      }
    }
    else
    {
      ++s;
    }
  }
  return 0;
}

static const LPCSTR gszTabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

void _rDocx_CB_startElement ( struct docx_state_ink * const p, const CHAR * name, const xmlChar **  attrs )
{
  fwprintf ( p->pF_xml, L"%.*hs<%hs>\n", p->d, gszTabs, name );
  ++p->d;
  switch ( p->i )
  {
    case kD7_Null:
      if ( strcmp ( name, "w:document" ) == 0 ) { p->i = kD7_document; }
      break;
    case kD7_document:
      if ( strcmp ( name, "w:body" ) == 0 ) { p->i = kD7_body; }
      break;
    case kD7_body:
      if ( strcmp ( name, "w:p" ) == 0 ) { p->i = kD7_body_p; }
      else
      if ( p-> iS != kD74_Null && strcmp ( name, "w:tbl" ) == 0 )
      {
        p->i = kD7_body_tbl;
        ++(p->n_tbl);
        p->n_tr = 0;
      }
      break;
    case kD7_body_tbl:
      if ( strcmp ( name, "w:tr" ) == 0 )
      {
        p->i = kD7_body_tbl_tr;
        ++(p->n_tr);
        p->n_tc = 0;

        // Добавляем новый столбец (на два праграфа) в строку
        // (p->s4ppwLastRow) = (LPWSTR**)r4_malloc_s4p ( 128 );
        // r4_push_array_s4p ( ((LPVOID*)(p->s4pppwLastTable)), (LPVOID*)(&(p->s4ppwLastRow)), 1 );
      }
      break;
    case kD7_body_tbl_tr:
      if ( strcmp ( name, "w:tc" ) == 0 )
      {
        p->i = kD7_body_tbl_tr_tc;
        ++(p->n_tc);
        p->n_p = 0;

        // Добавляем новый столбец (на два праграфа) в строку
        // (p->s4pwLastColumn) = (LPWSTR*)r4_malloc_s4p ( 128 );
        // r4_add_array_s4p ( ((LPVOID*)(p->s4ppwLastRow)), (LPVOID*)(&(p->s4pwLastColumn)), 1 );
      }
      break;
    case kD7_body_tbl_tr_tc:
      if ( strcmp ( name, "w:p" ) == 0 )
      {
        p->i = kD7_body_tbl_tr_tc_p;
        ++(p->n_p);
      }
      break;
  }
}
void _rDocx_CB_endElement ( struct docx_state_ink * const p, const CHAR * name )
{
  switch ( p->i )
  {
    case kD7_document:
      if ( strcmp ( name, "w:document" ) == 0 ) { p->i = kD7_Null; }
      break;
    case kD7_body:
      if ( strcmp ( name, "w:body" ) == 0 ) { p->i = kD7_document; }
      break;
    case kD7_body_p:
      if ( strcmp ( name, "w:p" ) == 0 )
      {
        p->i = kD7_body;
        if ( p->iS == kD74_Null && ( _wcsicmp_l ( p->s4w, L"Инклинометрия", g_locale_C ) == 0 ||
                _wcsicmp_l ( p->s4w, L"Замер кривизны", g_locale_C ) == 0 ) )
        {
          p->iS = kD74_ink;
        }
        else
        if ( p->iS == kD74_ink && _wcsnicmp_l (  p->s4w, L"Скважина N", 10, g_locale_C ) == 0 )
        {
          p->fWell = wcstoul ( p->s4w+10, NULL, 10 );
          p->iS = kD74_well;
        }
        else
        if ( p->iS == kD74_well && _wcsnicmp_l (  p->s4w, L"Диаметр скважины", 16, g_locale_C ) == 0 )
        {
          LPWSTR w;
          p->fDi = wcstod ( p->s4w+17, &w );
          p->iS = kD74_Di;
          while ( *w && p->iS == kD74_Di )
          {
            if ( _wcsnicmp_l (  w, L"Глубина башмака кондуктора", 26, g_locale_C ) == 0 )
            {
              p->fGBK = wcstod ( w+27, NULL );
              p->iS = kD74_GBK;
            }
            else
            {
              ++w;
            }
          }
        }
        else
        if ( (p->iS == kD74_Di || p->iS == kD74_GBK) && _wcsnicmp_l (  p->s4w, L"Угол склонения", 14, g_locale_C ) == 0 )
        {
          LPWSTR w;
          p->fAngleS = wcstod ( p->s4w+15, &w );
          p->iS = kD74_angle;
          UINT i = rParse_Docx_GetAngleType ( &w );
          if ( i == 0 )
          {
            rLog_Error ( L" => DOCX: Невозможо узнать размерность значения угла\n" );
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

        fwprintf ( p->pF_log, L"body => p {%s}\n", p->s4w );
        rParse_Docx_Log ( L"body => p {%s}\t%s\t%s\r\n", p->s4w, p->wszFileName, p->s4wOrigin );
        r4_cut_end_s4w ( p->s4w, 0 );
      }
      break;
    case kD7_body_tbl:
      if ( strcmp ( name, "w:tbl" ) == 0 ) { p->i = kD7_body; }
      break;
    case kD7_body_tbl_tr:
      if ( strcmp ( name, "w:tr" ) == 0 ) { p->i = kD7_body_tbl; }
      break;
    case kD7_body_tbl_tr_tc:
      if ( strcmp ( name, "w:tc" ) == 0 ) { p->i = kD7_body_tbl_tr; }
    case kD7_body_tbl_tr_tc_p:
      if ( strcmp ( name, "w:p" ) == 0 )
      {
        p->i = kD7_body_tbl_tr_tc;
        fwprintf ( p->pF_log, L"body => tbl(%d) => tr(%d) => tc(%d) => p(%d) {%s}\n",
                p->n_tbl, p->n_tr, p->n_tc, p->n_p, p->s4w );
        rParse_Docx_Log ( L"body => tbl(%d) => tr(%d) => tc(%d) => p(%d) {%s}\t%s\t%s\r\n",
                p->n_tbl, p->n_tr, p->n_tc, p->n_p, p->s4w, p->wszFileName, p->s4wOrigin );
        // Добавляем параграф в столбец
        // (p->s4wLastParagraph) = r4_malloc_s4w ( r4_get_count_s4w ( p->s4w ) + 1 );
        // r4_push_array_s4w_sz ( (p->s4wLastParagraph), p->s4w, r4_get_count_s4w ( p->s4w ) + 1 );
        // r4_add_array_s4p ( (LPVOID*)(p->s4pwLastColumn), (LPVOID*)(&(p->s4wLastParagraph)), 1 );
        // Проверяем первою строку таблицы
        if ( p->iS == kD74_zab && p->n_tr == 1 && p->n_tc == 1 && _wcsnicmp_l ( p->s4w, L"Глубина", 7, g_locale_C ) == 0 )
        {
          p->iS = kD74_tbl;
          p->nTbl = p->n_tbl;
          p->pData = r4_malloc_s4s ( 200, sizeof(struct ink_data) );
        }
        else
        if ( p->iS == kD74_tbl && p->nTbl == p->n_tbl && p->n_tr == 1 )
        {
          if ( _wcsnicmp_l ( p->s4w, L"Угол", 4, g_locale_C ) == 0 )
          {
            LPWSTR w = p->s4w + 4;
            const UINT i = rParse_Docx_GetAngleType ( &w );
            if ( i == 0 )
            {
              rLog_Error ( L" => DOCX: Невозможо узнать размерность значения угла\n" );
            }
            else
            {
              p->iAn = p->n_tc;
              p->bAn = i == 2;
            }
          }
          else
          if ( _wcsnicmp_l ( p->s4w, L"Азимут", 6, g_locale_C ) == 0 )
          {
            LPWSTR w = p->s4w + 6;
            const UINT i = rParse_Docx_GetAngleType ( &w );
            if ( i == 0 )
            {
              rLog_Error ( L" => DOCX: Невозможо узнать размерность значения aзимута\n" );
            }
            else
            {
              p->iAz = p->n_tc;
              p->bAz = i == 2;
            }
          }
        }
        else
        if ( p->iS == kD74_tbl && p->nTbl == p->n_tbl && p->n_tr == 2 )
        {
          if ( p->n_tc == 1 )
          {
            struct ink_data t = { };
            LPWSTR wsz;
            t.fDepth = wcstod ( p->s4w, &wsz );
            if ( wsz == p->s4w )
            {
              p->nData = p->n_p - 1;
            }
            else
            {
              if ( p->nData )
              {
                rLog_Error ( L" => DOCX: Таблица заканчивается не единственным пустым значением\n" );
              }
            }
            r4_add_array_s4s ( p->pData, &t, 1 );
          }
          else
          if ( p->n_tc == p->iAn )
          {
            struct ink_data *t = p->pData+(p->n_p-1);
            if ( p->s4w[0] == '*' )
            {
              t->iAn = 2;
              t->fAn = wcstod ( p->s4w+1, NULL );
            }
            else
            {
              LPWSTR wsz;
              t->fAn = wcstod ( p->s4w, &wsz );
              if ( wsz == p->s4w )
              {
                t->iAn = 0;
              }
              else
              {
                t->iAn = 1;
              }
            }
            if ( t->iAn && p->bAn )
            {
              const double d = modf ( t->fAn , &(t->fAn) );
              t->fAn += d * (100.0/60.0);
            }
          }
          else
          if ( p->n_tc == p->iAz )
          {
            struct ink_data *t = p->pData+(p->n_p-1);
            if ( p->s4w[0] == '*' )
            {
              t->iAz = 2;
              t->fAz = wcstod ( p->s4w+1, NULL );
            }
            else
            {
              LPWSTR wsz;
              t->fAz = wcstod ( p->s4w, &wsz );
              if ( wsz == p->s4w )
              {
                t->iAz = 0;
              }
              else
              {
                t->iAz = 1;
              }
            }
            if ( t->iAz && p->bAz )
            {
              const double d = modf ( t->fAz , &(t->fAz) );
              t->fAz += d * (100.0/60.0);
            }
          }
        }
        r4_cut_end_s4w ( p->s4w, 0 );
      }
      break;
  }

  --p->d;
  fwprintf ( p->pF_xml, L"%.*hs</%hs>\n", p->d, gszTabs, name );
}
void _rDocx_CB_characters ( struct docx_state_ink * const p, const xmlChar * ch, int len )
{
  WCHAR w[len+1];
  UINT i = 0;
  UINT j = 0;
  while ( j < len )
  {
    int k = len-j;
    w[i] = xmlGetUTF8Char ( ch+j, &k );
    j += k;
    ++i;
  }
  w[i] = 0;

  fwprintf ( p->pF_xml, L"%.*hs{%s}(%d)\n", p->d, gszTabs, w, i );

  switch ( p->i )
  {
    case kD7_body_p:
    case kD7_body_tbl_tr_tc_p:
      r4_push_array_s4w_sz ( p->s4w, w, i+1 );
      break;
  }
}

/*
  Разбор файла docx, на вход поступает путь к разархивированной папке
*/ // .temp/xxxxxxxx/ |] path/filename.docx |] filename.docx
static UINT rParse_Docx ( const LPWSTR s4wPath, const LPCWSTR s4wOrigin, const LPCWSTR wszFileName )
{
  rLog ( L"Parse_DOCX: %-256s ==> %-256s\n", s4wOrigin, s4wPath );
  const UINT n = r4_push_array_s4w_sz ( s4wPath, L"\\word\\document.xml", 0 );
  struct file_map fm;
  UINT iErr = 0;
  if ( ( iErr = rFS_FileMapOpen ( &fm, s4wPath ) ) ) goto P_End2;

  static xmlSAXHandler hSAX = {
    .characters = (charactersSAXFunc)_rDocx_CB_characters,
    .startElement = (startElementSAXFunc)_rDocx_CB_startElement,
    .endElement = (endElementSAXFunc)_rDocx_CB_endElement,
  };

  const LPWSTR s4w1 = r4_alloca_s4w(kPathMax);
  r4_push_path_s4w_s4w ( s4w1, s4wPathOutLogsDir );
  UINT i = 0;
  for ( i = 0; i <= 999; ++i )
  {
    swprintf ( s4w1+r4_get_count_s4w(s4w1), kPathMax-r4_get_count_s4w(s4w1),
            L"\\%s.[%03u].xml", wszFileName, i );
    FILE * const pF_xml = _wfopen ( s4w1, L"rb" );
    if ( pF_xml ) { fclose ( pF_xml ); } else { break; }
  }

  const LPWSTR s4w2 = r4_alloca_s4w(kPathMax);
  r4_push_path_s4w_s4w ( s4w2, s4wPathOutLogsDir );
  swprintf ( s4w2+r4_get_count_s4w(s4w2), kPathMax-r4_get_count_s4w(s4w2),
          L"\\%s.[%03u].log", wszFileName, i );


  struct docx_state_ink _ = {
    .pF_xml = rOpenFileToWriteWith_UTF16_BOM ( s4w1 ),
    .pF_log = rOpenFileToWriteWith_UTF16_BOM ( s4w2 ),
    .s4wOrigin = s4wOrigin,
    .wszFileName = wszFileName,
    .d = 0,
    .i = kD7_Null,
    .iS = kD74_Null,
    .s4w = r4_alloca_s4w(kPathMax),
  };

  if ( ( iErr = xmlSAXUserParseMemory ( &hSAX, &_, (LPCSTR)fm.pData, fm.nSize ) ) ) { goto P_End; }

  const LPWSTR s4w3 = r4_alloca_s4w(kPathMax);
  r4_push_path_s4w_s4w ( s4w3, s4wPathOutLogsDir );
  swprintf ( s4w3+r4_get_count_s4w(s4w3), kPathMax-r4_get_count_s4w(s4w3),
          L"\\%s.[%03u].%u.txt", wszFileName, i, _.iS );

  FILE * const pF_log2 = rOpenFileToWriteWith_UTF16_BOM ( s4w3 );
  fwprintf (pF_log2, L"Скважина:                  \t%u\r\n", _.fWell );
  fwprintf (pF_log2, L"Диаметр скважины:          \t%f\r\n", _.fDi );
  fwprintf (pF_log2, L"Глубина башмака кондуктора:\t%f\r\n", _.fGBK );
  fwprintf (pF_log2, L"Угол склонения:            \t%f %s\r\n", _.fAngleS, _.bAngleS ? L"минуты" : L"градусы" );
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

  fclose ( _.pF_xml );
  fclose ( _.pF_log );

  if ( _.iS == kD7_Null )
  {
    if ( !DeleteFile ( s4w1 ) )
    {
      rLog_Error_WinAPI ( DeleteFile, GetLastError(), L"%s\n", s4w1 );
      iErr = __LINE__;
    }
    if ( !DeleteFile ( s4w2 ) )
    {
      rLog_Error_WinAPI ( DeleteFile, GetLastError(), L"%s\n", s4w2 );
      iErr = __LINE__;
    }
    if ( !DeleteFile ( s4w3 ) )
    {
      rLog_Error_WinAPI ( DeleteFile, GetLastError(), L"%s\n", s4w2 );
      iErr = __LINE__;
    }
  }

  P_End:
  rFS_FileMapClose ( &fm );
  P_End2:
  r4_cut_end_s4w ( s4wPath, n );
  return iErr;
}
