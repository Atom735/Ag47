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

UINT rFileProc ( const LPCWSTR s4wFilePath, const LPCWSTR wszFileName,
        const UINT nFileSize )
{
  rLog ( L"\t\t==>% 14u ==> %s\n", nFileSize, s4wFilePath );
  return 0;
}


INT APIENTRY wWinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd )
{
  const LPCWSTR p = L"Hello world!"; LPCWSTR p2;
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"*l*", TRUE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"*l*", FALSE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"*L*", TRUE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"*L*", FALSE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"*HELLO*", TRUE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"*HELLO*", FALSE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"*world?", TRUE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"*world?", FALSE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"H??LO", TRUE ) );
  rLog ( L"%s(%s)==>%s\n", p, p2, r4_search_template_wsz ( p, p2 = L"H??LO", FALSE ) );

  AllocConsole ( );
  const LPWSTR s4w = r4_alloca_init_ex_s4w ( L"\\\\?\\UNC\\NAS\\Public", 2048 );
  rFS_Tree ( s4w, TRUE, rFileProc );

  rLog ( NULL );
  return 0;
}
