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


INT APIENTRY wWinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd )
{
  AllocConsole ( );
  const LPWSTR s4w = r4_alloca_init_ex_s4w ( L"\\\\NAS\\Public", 2048 );
  rFS_Tree ( s4w, TRUE, TRUE, NULL );
  rLog ( NULL );
  return 0;
}
