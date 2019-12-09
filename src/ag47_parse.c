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


UINT rFolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName );
UINT rFileProc ( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize );

UINT rFileProc ( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize )
{
#if 0

  const UINT n1 = r4_push_array_s4w_sz ( s4wPFolders, L"\\", 2 );
  r4_push_array_s4w_sz ( s4wPFolders, wszFileName, 0 );


  if ( r4_search_template_wsz ( s4wPath, L"*.las", FALSE ) ||
       r4_search_template_wsz ( s4wPath, L"*.las[?]", FALSE ) ||
       r4_search_template_wsz ( s4wPath, L"*.txt", FALSE ) ||
       r4_search_template_wsz ( s4wPath, L"*.doc", FALSE ) ||
       r4_search_template_wsz ( s4wPath, L"*.docx", FALSE ) ||
       r4_search_template_wsz ( s4wPath, L"*.rar", FALSE ) ||
       r4_search_template_wsz ( s4wPath, L"*.zip", FALSE ) ||
       r4_search_template_wsz ( s4wPath, L"*.7z", FALSE ) )
  {
    rLog ( L"\t\t==>% 14u ==> %s\n", nFileSize, s4wPath );
    nSizes += nFileSize;
    ++nFiles;
    if ( r4_search_template_wsz ( s4wPath, L"*.rar", FALSE ) ||
         r4_search_template_wsz ( s4wPath, L"*.zip", FALSE ) ||
         r4_search_template_wsz ( s4wPath, L"*.7z", FALSE ) )
    {
      r4_push_array_s4w_sz ( s4wPFolders, L".DIR", 5 );
      rFS_Run_7Zip ( s4wPath, s4wPFolders );
      LPWSTR s4w = r4_alloca_s4w ( kPathMax );
      r4_push_array_s4w_sz ( s4w, s4wPFolders, r4_get_count_s4w(s4wPFolders)+1 );
      rFS_Tree ( s4w, rFileProc, rFolderProc );
    }
    else
    if ( r4_search_template_wsz ( s4wPath, L"*.doc", FALSE ) )
    {
      r4_push_array_s4w_sz ( s4wPFolders, L".DOCX", 6 );
      rFS_Run_WordConv ( s4wPath, s4wPFolders );
      LPWSTR s4w = r4_alloca_s4w ( kPathMax );
      r4_push_array_s4w_sz ( s4w, s4wPFolders, r4_get_count_s4w(s4wPFolders)+1 );
      r4_push_array_s4w_sz ( s4w, L".DIR", 5 );
      rFS_Run_7Zip ( s4wPFolders, s4w );
      // rParse_Docx ( s4w );
    }
    else
    if ( r4_search_template_wsz ( s4wPath, L"*.docx", FALSE ) )
    {
      r4_push_array_s4w_sz ( s4wPFolders, L".DIR", 5 );
      rFS_Run_7Zip ( s4wPath, s4wPFolders );
      // rParse_Docx ( s4wPFolders );
    }
    else
    if ( !CopyFile ( s4wPath, s4wPFolders, TRUE ) )
    {
      rLog_Error_WinAPI ( CopyFile, GetLastError(), L"%s => %s\n", s4wPath, s4wPFolders );
    }
  }
  r4_cut_end_s4w ( s4wPFolders, n1 );

#endif
  return 0;
}

UINT rFolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName )
{
  return rFS_Tree ( s4wPath, rFileProc, rFolderProc );
}
