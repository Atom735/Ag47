#include <windows.h>

#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

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

LPWSTR g_s4wPathToWordConv = NULL;
LPWSTR g_s4wPathTo7Zip = NULL;

UINT rFolderProc ( const LPCSTR s4wPath, const LPCWSTR wszFolderName );

UINT rFileProc ( const LPCWSTR s4wPath, const LPCWSTR wszFileName,
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
       r4_search_template_wsz ( s4wPath, L"*.zip", FALSE ) )
  {
    rLog ( L"\t\t==>% 14u ==> %s\n", nFileSize, s4wPath );
    nSizes += nFileSize;
    ++nFiles;
    if ( r4_search_template_wsz ( s4wPath, L"*.rar", FALSE ) || r4_search_template_wsz ( s4wPath, L"*.zip", FALSE ) )
    {

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

UINT rFolderProc ( const LPCSTR s4wPath, const LPCWSTR wszFolderName )
{
  const UINT n1 = r4_push_array_s4w_sz ( s4wPFolders, L"\\", 2 );
  r4_push_array_s4w_sz ( s4wPFolders, wszFolderName, 0 );
  if ( ! CreateDirectory ( s4wPFolders, NULL ) )
  {
    rLog_Error_WinAPI ( CreateDirectory, GetLastError(), s4wPFolders );
  }
  rFS_Tree ( s4wPath, rFileProc, rFolderProc );
  r4_cut_end_s4w ( s4wPFolders, n1 );
}


UINT rSearchExes_rFolderProc ( const LPCSTR s4wPath, const LPCWSTR wszFolderName )
{
  const UINT n1 = r4_push_array_s4w_sz ( s4wPFolders, L"\\", 2 );
  r4_push_array_s4w_sz ( s4wPFolders, wszFolderName, 0 );
  if ( ! CreateDirectory ( s4wPFolders, NULL ) )
  {
    rLog_Error_WinAPI ( CreateDirectory, GetLastError(), s4wPFolders );
  }
  rFS_Tree ( s4wPath, rFileProc, rFolderProc );
  r4_cut_end_s4w ( s4wPFolders, n1 );
}

static UINT rSearchExe ( )
{
  WIN32_FIND_DATA ffd;
  {
    const HANDLE hFind = FindFirstFile ( L"C:/Program Files (x86)/Microsoft Office/Office*", &ffd );
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog_Error ( L"Невозможно найти папку с установленным [MS Office]\n" );
      FindClose ( hFind );
      return __LINE__;
    }
    do
    {
      r4_cut_end_s4w ( g_s4wPathToWordConv, 0 );
      r4_push_array_s4w_sz ( g_s4wPathToWordConv, L"C:/Program Files (x86)/Microsoft Office/", 0 );
      r4_push_array_s4w_sz ( g_s4wPathToWordConv, ffd.cFileName, 0 );
      r4_push_array_s4w_sz ( g_s4wPathToWordConv, L"/wordconv.exe", 0 );
      WIN32_FIND_DATA _ffd;
      const HANDLE _hFind = FindFirstFile ( g_s4wPathToWordConv, &_ffd );
      if ( _hFind != INVALID_HANDLE_VALUE )
      {
        rLog ( L"!INFO: [wordconv.exe] найден по пути: %s\n", g_s4wPathToWordConv );
        FindClose ( _hFind );
        FindClose ( hFind );
        goto P_7Zip;
      }
    } while ( FindNextFile ( hFind, &ffd ) );
    FindClose ( hFind );
    rLog_Error ( L"Невозможно найти [wordconv.exe]\n" );
    r4_cut_end_s4w ( g_s4wPathToWordConv, 0 );
    return __LINE__;
  }
  P_7Zip:
  {
    r4_cut_end_s4w ( g_s4wPathTo7Zip, 0 );
    r4_push_array_s4w_sz ( g_s4wPathTo7Zip, L"C:/Program Files/7-Zip/7z.exe", 0 );
    const HANDLE hFind = FindFirstFile ( g_s4wPathTo7Zip, &ffd );
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog_Error ( L"Невозможно найти [7z.exe], возможно не установлен [7zip]\n" );
      FindClose ( hFind );
      r4_cut_end_s4w ( g_s4wPathTo7Zip, 0 );
      return __LINE__;
    }
    rLog ( L"!INFO: [7z.exe] найден по пути: %s\n", g_s4wPathTo7Zip );
    FindClose ( hFind );
  }
  return 0;
}

INT APIENTRY wWinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd )
{
  AllocConsole ( );

  g_s4wPathToWordConv = r4_alloca_s4w ( PATH_MAX );
  g_s4wPathTo7Zip = r4_alloca_s4w ( PATH_MAX );

  rSearchExe ( );

  s4wPFolders = r4_alloca_init_ex_s4w ( L"\\\\?\\", 2048 );
  {
    const UINT u = GetCurrentDirectory ( 2048-r4_get_count_s4w(s4wPFolders), s4wPFolders+r4_get_count_s4w(s4wPFolders) );
    if ( u == 0 )
    {
      rLog_Error_WinAPI ( GetCurrentDirectory, GetLastError(), s4wPFolders );
    }
    r4_get_count_s4w(s4wPFolders) += u;
  }

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

  const LPWSTR s4w = r4_alloca_init_ex_s4w ( L"\\\\?\\UNC\\NAS\\Public", 2048 );
  rFS_Tree ( s4w, rFileProc, rFolderProc );
  rLog ( L"\t\t==>% 14u ==> All files (%u)\n", nSizes, nFiles );
  rLog ( NULL );
  return 0;
}
