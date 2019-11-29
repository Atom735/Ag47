﻿#include <windows.h>

#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <archive.h>
#include <archive_entry.h>

#define kPathMaxLen 2048

static FILE * pF = NULL;
static WCHAR w7Path [ kPathMaxLen ];



static FILE * rOpenFileToWriteWith_UTF16_BOM ( const LPCWSTR wszFname )
{
  FILE * const fd = _wfopen ( wszFname, L"wb" );
  if ( fd ) fwprintf ( fd, L"%c", 0xFEFF );
  return fd;
}

/*
  Функция получения номера кодировки для однбайтовых данных совместимых с кодировкой ASCII
  @ p                   указатель на начало данных
  @ n                   оличество данных
  @ return              строка с кодировкой пример ".1252" для функции setlocale
*/
LPCSTR rGetCodePage ( BYTE const * p, UINT n )
{
  static const LPCSTR pOut[] =
  { // https://docs.microsoft.com/ru-ru/windows/win32/intl/code-page-identifiers
    ".1251", ".866", ".855", ".10007", ".28595"
  };
  UINT u[5] = { };
  while ( n )
  {
    // https://ru.wikipedia.org/wiki/CP855
    switch ( *p )
    {
      case 0x9C ... 0x9D:
      case 0xA0 ... 0xAD:
      case 0xB5 ... 0xB8:
      case 0xC6 ... 0xC7:
      case 0xD0 ... 0xD8:
      case 0xDD ... 0xDE:
      case 0xE0 ... 0xEE:
      case 0xF1 ... 0xFC:
        ++u[2]; break;
    }
    // https://ru.wikipedia.org/wiki/CP866
    switch ( *p )
    {
      case 0x80 ... 0xAF:
      case 0xE0 ... 0xF1:
        ++u[1]; break;
    }
    // https://ru.wikipedia.org/wiki/Windows-1251
    switch ( *p )
    {
      case 0xA8: case 0xB8:
      case 0xC0 ... 0xFF:
        ++u[0]; break;
    }
    // https://ru.wikipedia.org/wiki/MacCyrillic
    switch ( *p )
    {
      case 0x80 ... 0x9F:
      case 0xDD ... 0xFE:
        ++u[3]; break;
    }
    // https://ru.wikipedia.org/wiki/ISO_8859-5
    switch ( *p )
    {
      case 0xA1: case 0xF1:
      case 0xB0 ... 0xEF:
        ++u[4]; break;
    }
    ++p; --n;
  }
  UINT k=0;
  for ( UINT i=1; i<5; ++i ) { if ( u[i] > u[k] ) k = i; }
  return pOut[k];
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



#define D7PRNT(...) fwprintf ( pF, __VA_ARGS__ );
#define D7PRNT_E(...) fwprintf ( pF, L"!ERROR: " __VA_ARGS__ );
#define D7PRNT_W(...) fwprintf ( pF, L"!WARN : " __VA_ARGS__ );
#define D7PRNT_I(...) fwprintf ( pF, L"!INFO : " __VA_ARGS__ );
#define D7PATH_PUSH(sz) const UINT __nPathSz = w7Path[0]; rW7_addf ( w7Path, L"\\%s", sz );
#define D7PATH_POP() w7Path[0] = __nPathSz; w7Path [ __nPathSz+1 ] = '\0';
#define D7PATH_PRNT(sz,n) D7PRNT ( L"%-16.16s %10u %s\n", sz, n, w7Path+1 )
#define D7IF_1(i,a) ( w7Path [ w7Path[0]-(i) ] == a )
#define D7IF_2(i,A) ( D7IF_1(i,A) || D7IF_1(i,A+0x20) )
#define D7IF_3(i,A,B,C) ( D7IF_2(i,A) && D7IF_2(i-1,B) && D7IF_2(i-2,C) )
#define D7IF_4_LAS ( D7IF_1(3,'.') && D7IF_3(2,'L','A','S') )
#define D7IF_4_ZIP ( D7IF_1(3,'.') && D7IF_3(2,'Z','I','P') )
#define D7IF_4_RAR ( D7IF_1(3,'.') && D7IF_3(2,'R','A','R') )
#define D7IF_4_ARCHIVE ( D7IF_4_ZIP || D7IF_4_RAR )

static VOID rParseLasData ( const PBYTE pData, const UINT nSize )
{

}

/* Загрузка файла в память */
static PBYTE rLoadFile ( const PBYTE pData, const LPCWSTR wszFileName, const UINT nFileSize )
{
  FILE * const fd = _wfopen ( wszFileName, L"rb" );
  assert ( fd );
  DWORD n = 0;
  while ( n < nFileSize )
  {
    n += fread ( pData+n, 1, nFileSize-n, fd );
  }
  pData[nFileSize] = '\0';
  fclose ( fd );
  return pData;
}

/* Парсим внутринности архива */
static UINT rParseArchive ( struct archive * const ar )
{
  struct archive_entry *are;
  while ( archive_read_next_header ( ar, &are ) == ARCHIVE_OK )
  {
    const UINT nSize = (UINT)archive_entry_size(are);
    D7PATH_PUSH(archive_entry_pathname_w(are))
    // Проверяем путь на путь к файлу
    if ( D7IF_4_LAS )
    {
      D7PATH_PRNT(L"+ar +las",nSize);
      BYTE abData[nSize+1];
      // Парсим Las файл из архива
      archive_read_data ( ar, abData, nSize+1 );
      rParseLasData ( abData, nSize );
    }
    else
    if ( D7IF_4_ARCHIVE )
    {
      D7PATH_PRNT(L"+ar +ar",nSize);
      struct archive *ar2 = archive_read_new();
      archive_read_support_filter_all ( ar2 );
      archive_read_support_format_all ( ar2 );
      // Открываем архив
      BYTE abData[nSize+1];
      if ( archive_read_open_memory ( ar2, abData, archive_read_data ( ar, abData, nSize+1 ) ) != ARCHIVE_OK )
      { D7PRNT_E ( L"archive_read_open_memory (\"%s\")\n", w7Path+1 ) }
      else
      { rParseArchive ( ar2 ); }
      if ( archive_read_free ( ar2 ) != ARCHIVE_OK )
      { D7PRNT_E ( L"archive_read_free (\"%s\")\n", w7Path+1 ) }
    }
    else
    {
      D7PATH_PRNT(L"+ar +",nSize);
      archive_read_data_skip ( ar );
    }
    D7PATH_POP();
  }
  return 0;
}

/*
  Разбирает путь по структуре ffd
*/
static UINT rParseFFD ( WIN32_FIND_DATA const * const pFFD )
{
  if ( pFFD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
  {
    if ( pFFD->cFileName[0] == '.' )
    {
      if ( pFFD->cFileName[1] == '\0' )  { return 0; }
      else
      if ( pFFD->cFileName[1] == '.' )
      { if ( pFFD->cFileName[2] == '\0' ) { return 0; } }
    }
    // Если путь указан к папке, то ищем внтури папки
    D7PATH_PUSH(L"*");
    WIN32_FIND_DATA ffd;
    HANDLE hFind;
    hFind = FindFirstFile ( w7Path+1, &ffd );
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      D7PRNT_E ( L"FindFirstFile (0x%X) (\"%s\")\n", (UINT)GetLastError(), w7Path+1 );
      return __LINE__;
    }
    D7PATH_POP();
    do
    {
      D7PATH_PUSH(ffd.cFileName);
      rParseFFD ( &ffd );
      D7PATH_POP();
    } while ( FindNextFile ( hFind, &ffd ) );
    FindClose ( hFind );
  }
  else
  {
    // Проверяем путь на путь к файлу
    if ( D7IF_4_LAS )
    {
      D7PATH_PRNT(L"+las",pFFD->nFileSizeLow);
      BYTE abData[pFFD->nFileSizeLow+1];
      // Парсим Las файл
      rParseLasData ( rLoadFile ( abData, w7Path+1, pFFD->nFileSizeLow ), pFFD->nFileSizeLow );
    }
    else
    if ( D7IF_4_ARCHIVE )
    {
      D7PATH_PRNT(L"+ar",pFFD->nFileSizeLow);
      struct archive *ar = archive_read_new();
      archive_read_support_filter_all ( ar );
      archive_read_support_format_all ( ar );
      // Открываем архив
      FILE * const pFAr = _wfopen ( w7Path+1, L"rb" );
      assert ( pFAr );
      if ( archive_read_open_FILE ( ar, pFAr ) != ARCHIVE_OK )
      { D7PRNT_E ( L"archive_read_open_FILE (\"%s\")\n", w7Path+1 ) }
      else
      { rParseArchive ( ar ); }
      if ( archive_read_free ( ar ) != ARCHIVE_OK )
      { D7PRNT_E ( L"archive_read_free (\"%s\")\n", w7Path+1 ) }
      fclose ( pFAr );
    }
    else
    {
      D7PATH_PRNT(L"+",pFFD->nFileSizeLow);
    }
  }
  return 0;
}

/* Разбирает указаные в пути */
static UINT rParseFile ( )
{
  WIN32_FIND_DATA ffd;
  HANDLE hFind;
  hFind = FindFirstFile ( w7Path+1, &ffd );
  if ( hFind == INVALID_HANDLE_VALUE )
  {
    D7PRNT_E ( L"FindFirstFile (0x%X) (\"%s\")\n", (UINT)GetLastError(), w7Path+1 );
    return __LINE__;
  }
  UINT const i = rParseFFD ( &ffd );
  FindClose ( hFind );
  return i;
}


INT wmain ( INT argc, WCHAR const *argv[], WCHAR const *envp[] )
{
  printf ( "setlocale: %s\n", setlocale ( LC_ALL, "" ) );
  pF = rOpenFileToWriteWith_UTF16_BOM ( L"out.log" );
  fwprintf ( pF, L"ARGC >> %d\n", argc );
  for ( UINT i = 0; i < argc; ++i )
  { fwprintf ( pF, L"ARGV[%d] >> %s\n", i, argv[i] ); }
  for ( UINT i = 0; envp[i]; ++i )
  { fwprintf ( pF, L"ENVP[%d] >> %s\n", i, envp[i] ); }

  if ( argc == 1 )
  {
    rW7_set ( w7Path, L"\\\\NAS\\Public\\common\\Gilyazeev\\ГИС\\Искринское м-е" );
    rParseFile();
  }
  else
  for ( UINT i = 1; i < argc; ++i )
  {
    rW7_set ( w7Path, argv[i] );
    rParseFile();
  }

  fclose ( pF );
  return 0;
}
