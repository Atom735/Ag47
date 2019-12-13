/*

Если строка совпадает с:
[\s]*Инклинометрия[\s]*
[\s]*Замер кривизны[\s]*
То это файл с инклинометрией и далее можем его начать разбирать


[\s]*Скважина[\s]*(?:N[\s]+)?([\w]+)

*/

static UINT rParse_Txt_Begin ( struct docx_state_ink * const p, struct file_map const * const fm )
{
  return 0;
}

static UINT rParse_Txt ( const LPCWSTR s4wPath, const LPCWSTR s4wOrigin, const LPCWSTR wszFileName )
{
  rLog ( L"Parse_TXT: %-256s ==> %-256s\n", s4wOrigin, s4wPath );
  struct file_map fm;
  UINT iErr = 0;
  if ( ( iErr = rFS_FileMapOpen ( &fm, s4wPath ) ) ) goto P_End2;

  const LPWSTR s4w1 = r4_alloca_s4w(kPathMax);
  r4_push_path_s4w_s4w ( s4w1, s4wPathOutLogsDir );
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
    .s4wOrigin = s4wOrigin,
    .wszFileName = wszFileName,
    .d = 0,
    .i = kD7_Null,
    .iS = kD74_Null,
    .s4w = r4_alloca_s4w(kPathMax),
  };
  UINT a1[g7CharMapCount], a2[g7CharMapCount];
  _.iCodePage           = rGetBufCodePage ( fm.pData, fm.nSize, a1, a2 );
  _.iLineFeed           = rGetBufEndOfLine ( fm.pData, fm.nSize );
  setlocale ( LC_ALL, g7CharMapCP[_.iCodePage] );


  if ( ( iErr = rParse_Txt_Begin ( &_, &fm ) ) ) { goto P_End; }

  if ( _.iS != kD7_Null )
  {
    const LPWSTR s4w3 = r4_alloca_s4w(kPathMax);
    r4_push_path_s4w_s4w ( s4w3, s4wPathOutLogsDir );
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
