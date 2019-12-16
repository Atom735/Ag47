




UINT rParse_FileProc ( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize, const LPWSTR s4wOrigin );
UINT rParse_FolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName,
        const LPWSTR s4wOrigin );

static UINT rParse_Tree ( const LPWSTR s4wPath, const LPWSTR s4wOrigin )
{
  return rFS_Tree ( s4wPath, rParse_FileProc, rParse_FolderProc, s4wOrigin  );
}

UINT rParse_FileProc ( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize, const LPWSTR s4wOrigin )
{
  if ( wszFileName[0] == '~' && wszFileName[1] == '$' ) return 0;
  const UINT n = r4_get_count_s4w ( s4wOrigin );
  r4_push_array_s4w_sz ( s4wOrigin, L"\\", 2 );
  r4_push_array_s4w_sz ( s4wOrigin, wszFileName, 0 );
  UINT iErr = 0;
  #if 0
  if ( r4_path_ending_s4w_zip ( s4wPath ) )
  {
    const LPWSTR s4wPathTempDir = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDir, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDir ); // .temp/xxxxxxxx/
    rFS_Run_7Zip ( s4wPath, s4wPathTempDir ); // path/filename.zip ==> .temp/xxxxxxxx/
    iErr = rParse_Tree ( s4wPathTempDir, s4wOrigin ); // .temp/xxxxxxxx/ |] path/filename.zip
    rFS_DeleteTree ( s4wPathTempDir );
  }
  else
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
  #endif
  if ( r4_path_ending_s4w_dbf ( s4wPath ) )
  {
    rParse_DBF ( s4wPath, s4wOrigin, wszFileName );
  }
  P_End:
  r4_cut_end_s4w ( s4wOrigin, n );
  return iErr;
}

UINT rParse_FolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName,
        const LPWSTR s4wOrigin )
{
  if ( _wcsicmp_l(wszFolderName,L".ag47",g_locale_C) == 0 ) { return 0; }
  const UINT n = r4_get_count_s4w ( s4wOrigin );
  r4_push_array_s4w_sz ( s4wOrigin, L"\\", 2 );
  r4_push_array_s4w_sz ( s4wOrigin, wszFolderName, 0 );
  const UINT iErr = rFS_Tree ( s4wPath, rParse_FileProc, rParse_FolderProc, s4wOrigin );
  r4_cut_end_s4w ( s4wOrigin, n );
  return iErr;
}

