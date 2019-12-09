static UINT rParse_Docx ( const LPCWSTR s4wPath, const LPCWSTR s4wOrigin )
{
  return 0;
}
static UINT rParse_Txt ( const LPCWSTR s4wPath, const LPCWSTR s4wOrigin )
{
  return 0;
}
static UINT rParse_Las ( const LPWSTR s4wPath, const LPCWSTR s4wOrigin )
{
  return 0;
}

UINT rParse_FileProc ( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize );
UINT rParse_FolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName );



UINT rParse_FileProc ( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize )
{
  if ( r4_path_ending_s4w_zip ( s4wPath ) )
  {
    const LPWSTR s4wPathTempDir = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPathTempDir, s4wPathOutTempDir );
    rFS_NewRandDir_s4w ( s4wPathTempDir );
    rFS_Run_7Zip ( s4wPath, s4wPathTempDir );
  }
  return 0;
}

UINT rParse_FolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName )
{
  return rFS_Tree ( s4wPath, rParse_FileProc, rParse_FolderProc );
}

static UINT rParse_Tree ( const LPWSTR s4wPath )
{
  return rFS_Tree ( s4wPath, rParse_FileProc, rParse_FolderProc );
}
