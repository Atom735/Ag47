#include <windows.h>

#include <stdio.h>

#define kPathMaxLen 2048

static FILE * pF = NULL;
static FILE * rOpenFileToWriteWith_UTF16_BOM ( const LPCWSTR wszFname )
{
  FILE * const fd = _wfopen ( wszFname, L"wb" );
  if ( fd ) fwprintf ( fd, L"%c", 0xFEFF );
  return fd;
}


/*
  @ w7...                       строка UTF16 хранящая длину строки первым символом и оканчивающаяся нулеым символом
    w7...[0]                    длина строки в символах
    w7...[0] * sizeof(WCHAR)    размер строки
          +1 * sizeof(WCHAR)    размер строки с конечным нулём
          +2 * sizeof(WCHAR)    размер всех данных
    w7... +1                    указатель на строку с конечным нулём
    w7...[w7...[0]]             последний символ (не ноль)
                  +1            ноль
*/
static UINT rW7_vsetf ( const LPWSTR w7Dst, const LPCWSTR wszFormat, va_list args )
{ return (w7Dst[0] = vswprintf ( w7Dst+1, kPathMaxLen, wszFormat, args )); }
static UINT rW7_setf ( const LPWSTR w7Dst, const LPCWSTR wszFormat, ... )
{ va_list args; va_start ( args, wszFormat ); rW7_vsetf ( w7Dst, wszFormat, args ); va_end ( args ); return w7Dst[0]; }
static UINT rW7_set ( const LPWSTR w7Dst, const LPCWSTR wszSrc )
{ return rW7_setf ( w7Dst, L"%s", wszSrc ); }
static UINT rW7_vaddf ( const LPWSTR w7Dst, const LPCWSTR wszFormat, va_list args )
{ const UINT i = vswprintf ( w7Dst+1+w7Dst[0], kPathMaxLen, wszFormat, args ); w7Dst[0] += i; return i; }
static UINT rW7_addf ( const LPWSTR w7Dst, const LPCWSTR wszFormat, ... )
{ va_list args; va_start ( args, wszFormat ); const UINT i = rW7_vaddf ( w7Dst, wszFormat, args ); va_end ( args ); return i; }
static UINT rW7_add ( const LPWSTR w7Dst, const LPCWSTR wszSrc )
{ return rW7_addf ( w7Dst, L"%s", wszSrc ); }


INT wmain ( INT argc, WCHAR const *argv[], WCHAR const *envp[] )
{
  pF = rOpenFileToWriteWith_UTF16_BOM ( L"out.log" );
  fwprintf ( pF, L"ARGC >> %d\n", argc );
  for ( UINT i = 0; i < argc; ++i )
  {
    fwprintf ( pF, L"ARGV[%d] >> %s\n", i, argv[i] );
  }

  for ( UINT i = 0; envp[i]; ++i )
  {
    fwprintf ( pF, L"ENVP[%d] >> %s\n", i, envp[i] );
  }
  fclose ( pF );
  return 0;
}
