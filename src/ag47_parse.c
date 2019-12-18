




BOOL rParse_FileProc ( LPWSTR const s4wPath, LPCWSTR const wszFileName,
        UINT const nFileSize, struct ag47_script * const script );
BOOL rParse_FolderProc ( LPWSTR const s4wPath, LPCWSTR const wszFolderName ,
        struct ag47_script * const script );


BOOL rParse_FileProc ( LPWSTR const s4wPath, LPCWSTR const wszFileName,
        UINT const nFileSize, struct ag47_script * const script )
{
  if ( wszFileName[0] == '~' ) { return TRUE; }
  // Пропускаем файлы подходящие под шаблон
  D4ForAll_ss4w ( script->ss4wExcludeFF, i, n1 )
  { if ( PathMatchSpecW ( s4wPath, script->ss4wExcludeFF[i] ) ||
          PathMatchSpecW ( wszFileName, script->ss4wExcludeFF[i] ) ) { return TRUE; } }

  // Пропускаем файлы подходящие под размеры
  if ( script->s4uExcludeSizes )
  {
    UINT const * const p = script->s4uExcludeSizes;
    UINT const n1 = r4_get_count_s4u ( p );
    UINT const n2 = n1 / 2;
    if ( n1 % 2 && nFileSize >= p[n1-1] ) { return TRUE; }
    for ( UINT i = 0; i < n2; ++i )
    {
      if ( p[i*2+0] <= p[i*2+1] ) // исключены будут файлы в диапозоне
      { if ( nFileSize >= p[i*2+0] && nFileSize <= p[i*2+1] ) { return TRUE; } }
      else
      { if ( nFileSize >= p[i*2+0] || nFileSize <= p[i*2+1] ) { return TRUE; } }
    }
  }

  const UINT n = r4_get_count_s4w ( script->s4wOrigin );
  r4_push_array_s4w_sz ( script->s4wOrigin, L"\\", 2 );
  r4_push_array_s4w_sz ( script->s4wOrigin, wszFileName, 0 );

  // Архивы
  if ( script->ss4wArchiveFF && r4_path_match_s4w_by_ss4w ( s4wPath, script->ss4wArchiveFF ) )
  {
    // Пропускаем из за глубины
    if ( script->nRecursive == 0 ) { return TRUE; }
    const LPWSTR s4wPathTempDir = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDir, script->s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDir ); // .temp/xxxxxxxx/
    rFS_Run_7Zip ( script, s4wPath, s4wPathTempDir ); // path/filename.zip ==> .temp/xxxxxxxx/
    --(script->nRecursive);
    const BOOL b = rFS_Tree ( s4wPath, // .temp/xxxxxxxx/ |] path/filename.zip
      (BOOL (*)(LPWSTR const,  LPCWSTR const, UINT const, LPVOID const))rParse_FileProc,
      (BOOL (*)(LPWSTR const,  LPCWSTR const, LPVOID const))rParse_FolderProc,
      script );
    ++(script->nRecursive);
    rFS_DeleteTree ( s4wPathTempDir );
    r4_cut_end_s4w ( script->s4wOrigin, n );
    return b;
  }
  else // Файлы ГИС
  if ( script->ss4wLasFF && r4_path_match_s4w_by_ss4w ( s4wPath, script->ss4wLasFF ) )
  {
    r4_cut_end_s4w ( script->s4wOrigin, n );
    return TRUE;
  }
  else // Файлы Инклинометрии
  if ( script->ss4wInkFF && r4_path_match_s4w_by_ss4w ( s4wPath, script->ss4wInkFF ) )
  {
    r4_cut_end_s4w ( script->s4wOrigin, n );
    return TRUE;
  }
  else
  {
    r4_cut_end_s4w ( script->s4wOrigin, n );
    return TRUE;
  }
  #if 0
  if ( r4_path_ending_s4w_docx ( s4wPath ) )
  {
    const LPWSTR s4wPathTempDir = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDir, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDir ); // .temp/xxxxxxxx/
    rFS_Run_7Zip ( s4wPath, s4wPathTempDir ); // path/filename.docx ==> .temp/xxxxxxxx/
    iErr = rParse_Docx ( s4wPathTempDir, s4wOrigin, wszFileName ); // .temp/xxxxxxxx/ |] path/filename.docx |] filename.docx
    rFS_DeleteTree ( s4wPathTempDir ); // .temp/xxxxxxxx/
  }
  else
  if ( r4_path_ending_s4w_doc ( s4wPath ) )
  {
    struct file_map fm;
    if ( rFS_FileMapOpen ( &fm, s4wPath ) ) { goto P_End; }
    if ( fm.nSize < 8 ) { rFS_FileMapClose ( &fm ); goto P_End; }
    if ( memcmp ( fm.pData, ((BYTE[]){0xD0,0xCF,0x11,0xE0,0xA1,0xB1,0x1A,0xE1}), 8 ) ) { rFS_FileMapClose ( &fm ); goto P_End; }
    rFS_FileMapClose ( &fm );
    const LPWSTR s4wPathTempDoc = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDoc, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDoc ); // .temp/xxxxxxxx/
    const UINT n = r4_get_count_s4w ( s4wPathTempDoc );
    r4_push_array_s4w_sz ( s4wPathTempDoc, L"\\", 2 );
    r4_push_array_s4w_sz ( s4wPathTempDoc, wszFileName, 0 );
    r4_push_array_s4w_sz ( s4wPathTempDoc, L".docx", 6 ); // .temp/xxxxxxxx/filename.docx
    rFS_Run_WordConv ( s4wPath, s4wPathTempDoc ); //  path/filename.doc ==> .temp/xxxxxxxx/filename.doc.docx
    const LPWSTR s4wPathTempDir = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDir, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDir ); // .temp/yyyyyyyy/
    rFS_Run_7Zip ( s4wPathTempDoc, s4wPathTempDir ); // .temp/xxxxxxxx/filename.doc.docx ==> .temp/yyyyyyyy/
    iErr = rParse_Docx ( s4wPathTempDir, s4wOrigin, wszFileName ); // .temp/yyyyyyyy/ |] path/filename.doc |] filename.doc
    rFS_DeleteTree ( s4wPathTempDir ); // .temp/yyyyyyyy/
    r4_cut_end_s4w ( s4wPathTempDoc, n ); // .temp/xxxxxxxx/
    rFS_DeleteTree ( s4wPathTempDoc );
  }
  else
  if ( r4_path_ending_s4w_txt ( s4wPath ) )
  {
    rParse_Txt ( s4wPath, s4wOrigin, wszFileName );
  }
  else
  if ( r4_path_ending_s4w_las ( s4wPath ) )
  {
    rParse_Las ( s4wPath, s4wOrigin, wszFileName );
  }
  else
  if ( r4_path_ending_s4w_dbf ( s4wPath ) )
  {
    rParse_DBF ( s4wPath, s4wOrigin, wszFileName );
  }
  P_End:
  r4_cut_end_s4w ( s4wOrigin, n );

  return FALSE;
  #endif
}

BOOL rParse_FolderProc ( LPWSTR const s4wPath, LPCWSTR const wszFolderName,
        struct ag47_script * const script )
{
  if ( wszFolderName[0] == '~' ) { return TRUE; }
  // Пропускаем из за глубины
  if ( script->nRecursive == 0 ) { return TRUE; }
  // Пропускаем папки под шаблоном
  D4ForAll_ss4w ( script->ss4wExcludeFF, i, n1 )
  { if ( PathMatchSpecW ( s4wPath, script->ss4wExcludeFF[i] ) ||
          PathMatchSpecW ( wszFolderName, script->ss4wExcludeFF[i] ) ) { return TRUE; } }

  const UINT n = r4_get_count_s4w ( script->s4wOrigin );
  r4_push_array_s4w_sz ( script->s4wOrigin, L"\\", 2 );
  r4_push_array_s4w_sz ( script->s4wOrigin, wszFolderName, 0 );
  --(script->nRecursive);
  const BOOL b = rFS_Tree ( s4wPath,
      (BOOL (*)(LPWSTR const,  LPCWSTR const, UINT const, LPVOID const))rParse_FileProc,
      (BOOL (*)(LPWSTR const,  LPCWSTR const, LPVOID const))rParse_FolderProc,
      script );
  ++(script->nRecursive);
  r4_cut_end_s4w ( script->s4wOrigin, n );
  return b;
}

