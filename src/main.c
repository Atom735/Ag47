#include <windows.h>

#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#define kPathMax PATH_MAX
LPWSTR s4wPathOut = NULL;

#include "ag47_misc.c"
#include "ag47_log.c"
#include "ag47_tbl_rus_a.c"
#include "ag47_tbl_rus_b.c"
#include "ag47_arrays.c"
#include "ag47_fs.c"


// INT wmain ( INT argc, LPCWSTR argv[] )
// {
//   r4_test_alloca_init ( );
//   WCHAR p[] = L"ПрИвЕт мИр! Hello WoRlD!";
//   FILE * pf = fopen ( "ag.log", "wb" );
//   fwprintf( pf, L"%lc\n", 0xfeff );
//   fwprintf( pf, L"- %ls\n", p );
//   for ( UINT i=0; p[i]; ++i )
//   {
//     p[i] = towupper(p[i]);
//   }
//   fwprintf( pf, L"U %ls\n", p );
//   for ( UINT i=0; p[i]; ++i )
//   {
//     if ( iswalpha (p[i]) )
//     p[i] = towlower(p[i]);
//   }
//   fwprintf( pf, L"D %ls\n", p );
//   fclose ( pf );
//   return 0;
// }
UINT nSizes = 0;
UINT nFiles = 0;
LPWSTR s4wPFolders = 0;




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
        const UINT nFileSize )
{
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

  return 0;
}

UINT rFolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName )
{
  const UINT n1 = r4_push_array_s4w_sz ( s4wPFolders, L"\\", 2 );
  r4_push_array_s4w_sz ( s4wPFolders, wszFolderName, 0 );
  if ( ! CreateDirectory ( s4wPFolders, NULL ) )
  {
    rLog_Error_WinAPI ( CreateDirectory, GetLastError(), s4wPFolders );
  }
  rFS_Tree ( s4wPath, rFileProc, rFolderProc );
  r4_cut_end_s4w ( s4wPFolders, n1 );
  return 0;
}




INT APIENTRY wWinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd )
{
  AllocConsole ( );
  g_s4wPathToWordConv = r4_alloca_s4w ( PATH_MAX );
  g_s4wPathTo7Zip = r4_alloca_s4w ( PATH_MAX );
  rFS_SearchExe ( );
  s4wPathOut = r4_alloca_init_ex_s4w ( L"\\\\?\\", kPathMax );
  rFS_GetCurrentDirectory_s4w ( s4wPathOut );
  r4_push_array_s4w_sz ( s4wPFolders, L"\\.ag47", 0 );


  r4_push_array_s4w_sz ( s4wPFolders, L"\\.ag47", 0 );
  if ( ! CreateDirectory ( s4wPFolders, NULL ) )
  {
    rLog_Error_WinAPI ( CreateDirectory, GetLastError(), s4wPFolders );
  }
  r4_push_array_s4w_sz ( s4wPFolders, L"\\db", 0 );
  if ( ! CreateDirectory ( s4wPFolders, NULL ) )
  {
    rLog_Error_WinAPI ( CreateDirectory, GetLastError(), s4wPFolders );
  }

  const LPWSTR s4w = r4_alloca_init_ex_s4w ( L"\\\\?\\UNC\\NAS\\Public", kPathMax );
  rFS_Tree ( s4w, rFileProc, rFolderProc );
  rLog ( L"\t\t==>% 14u ==> All files (%u)\n", nSizes, nFiles );
  rLog ( NULL );
  return 0;
}
