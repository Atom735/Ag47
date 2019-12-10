/*
  Разбор файла docx, на вход поступает путь к разархивированной папке
*/
static UINT rParse_Docx ( const LPCWSTR s4wPath, const LPCWSTR s4wOrigin )
{
  rLog ( L"Parse_DOCX: %-256s ==> %-256s\n", s4wOrigin, s4wPath );
  return 0;
}
static UINT rParse_Txt ( const LPCWSTR s4wPath, const LPCWSTR s4wOrigin )
{
  rLog ( L"Parse_TXT: %-256s ==> %-256s\n", s4wOrigin, s4wPath );
  return 0;
}
static UINT rParse_Las ( const LPWSTR s4wPath, const LPCWSTR s4wOrigin )
{
  rLog ( L"Parse_LAS: %-256s ==> %-256s\n", s4wOrigin, s4wPath );
  return 0;
}

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
  const UINT n = r4_push_array_s4w_sz ( s4wOrigin, L"\\", 2 );
  r4_push_array_s4w_sz ( s4wOrigin, wszFileName, 0 );
  UINT iErr = 0;
  if ( r4_path_ending_s4w_zip ( s4wPath ) )
  {
    const LPWSTR s4wPathTempDir = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDir, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDir );
    rFS_Run_7Zip ( s4wPath, s4wPathTempDir );
    iErr = rParse_Tree ( s4wPathTempDir, s4wOrigin );
    rFS_DeleteTree ( s4wPathTempDir );
  }
  else
  if ( r4_path_ending_s4w_docx ( s4wPath ) )
  {
    const LPWSTR s4wPathTempDir = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDir, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDir );
    rFS_Run_7Zip ( s4wPath, s4wPathTempDir );
    iErr = rParse_Docx ( s4wPathTempDir, s4wOrigin );
    rFS_DeleteTree ( s4wPathTempDir );
  }
  else
  if ( r4_path_ending_s4w_doc ( s4wPath ) )
  {
    const LPWSTR s4wPathTempDoc = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDoc, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDoc );
    r4_push_array_s4w_sz ( s4wPathTempDoc, L"\\", 2 );
    r4_push_array_s4w_sz ( s4wPathTempDoc, wszFileName, 0 );
    r4_push_array_s4w_sz ( s4wPathTempDoc, L".docx", 6 );
    rFS_Run_WordConv ( s4wPath, s4wPathTempDoc );
    rParse_Docx ( s4wPathTempDoc, s4wOrigin );
    const LPWSTR s4wPathTempDir = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDir, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDir );
    rFS_Run_7Zip ( s4wPathTempDoc, s4wPathTempDir );
    iErr = rParse_Docx ( s4wPathTempDir, s4wOrigin );
    rFS_DeleteTree ( s4wPathTempDir );
    rFS_DeleteTree ( s4wPathTempDoc );
  }
  else
  if ( r4_path_ending_s4w_txt ( s4wPath ) )
  {
    iErr = rParse_Txt ( s4wPath, s4wOrigin );
  }
  else
  if ( r4_path_ending_s4w_las ( s4wPath ) )
  {
    iErr = rParse_Las ( s4wPath, s4wOrigin );
  }
  r4_cut_end_s4w ( s4wOrigin, n );
  return iErr;
}

UINT rParse_FolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName,
        const LPWSTR s4wOrigin )
{
  const UINT n = r4_push_array_s4w_sz ( s4wOrigin, L"\\", 2 );
  r4_push_array_s4w_sz ( s4wOrigin, wszFolderName, 0 );
  const UINT iErr = rFS_Tree ( s4wPath, rParse_FileProc, rParse_FolderProc, s4wOrigin );
  r4_cut_end_s4w ( s4wOrigin, n );
  return iErr;
}

