#include <windows.h>

#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <archive.h>
#include <archive_entry.h>

#include "cyrillic.c"

#define kPathMaxLen 512

static FILE * rOpenFileToWriteWith_UTF16_BOM ( const LPCWSTR wszFname )
{
  FILE * const fd = _wfopen ( wszFname, L"wb" );
  if ( fd ) fwprintf ( fd, L"%c", 0xFEFF );
  return fd;
}

/*
  Функция получения номера кодировки для однбайтовых данных совместимых с кодировкой ASCII
  @ p                   указатель на начало данных
  @ n                   количество данных
  @ return              номер кодировки из базы
*/
UINT rGetCodePage ( BYTE const * p, UINT n )
{
  UINT u[kNCyrillicTables] = { };
  while ( n )
  {
    if ( (*p) & 0x80 )
    {
      for ( UINT i = 0; i < kNCyrillicTables; ++i )
      {
        const WCHAR w = g_ctCyrillicTables[i][ (*p) & 0x7f ];
        if ( w >= 0x410 && w <= 0x44F )
        {
          u[i] += g_nCyrillicPeriodTable[(w<0x430)?(w-0x410):(w-0x430)];
        }
      }
    }
    ++p; --n;
  }
  UINT k=0;
  for ( UINT i = 1; i < kNCyrillicTables; ++i )
  {
    if ( u[i] > u[k] )
    {
      k = i;
    }
  }
  return k;
}

/*
  Функция загрузки файла в память
  @ p                   указатель на блок памяти размера как минимум n+1
  @ wsz                 путь к файлу
  @ n                   размер файла
*/
static PBYTE rLoadFile ( const PBYTE p, const LPCWSTR wsz, const UINT n )
{
  FILE * const fd = _wfopen ( wsz, L"rb" );
  assert ( fd );
  UINT N = 0;
  while ( N < n )
  {
    N += fread ( p+N, 1, n-N, fd );
  }
  p[n]    = '\0';
  fclose ( fd );
  return p;
}

static UINT rUTF8_GetLength ( BYTE const * p, UINT n )
{
  UINT i = 0;
  while ( n )
  {
    if ( *p & 0x80 )
    {
      if ( ((*p) & (0xff<<5)) == ((*p) & (0xff<<6)) )
      { p += 2; n -= 2; ++i; }
      else
      if ( ((*p) & (0xff<<4)) == ((*p) & (0xff<<5)) )
      { p += 3; n -= 3; ++i; }
      else
      if ( ((*p) & (0xff<<3)) == ((*p) & (0xff<<4)) )
      { p += 4; n -= 4; ++i; }
      else
      { abort (); }
    }
    else
    {
      ++p; --n; ++i;
    }
  }
  return i;
}
static UINT rUTF8_ToWide ( BYTE const * p, UINT n, WCHAR * wsz )
{
  UINT i = 0;
  while ( n )
  {
    if ( *p & 0x80 )
    {
      if ( ((*p) & (0xff<<5)) == ((*p) & (0xff<<6)) )
      {
        *wsz = (((p[0])&(0xff>>3))<<6) | (((p[1])&(0xff>>2))<<0); ++wsz;
        p += 2; n -= 2; ++i;
      }
      else
      if ( ((*p) & (0xff<<4)) == ((*p) & (0xff<<5)) )
      {
        *wsz = (((p[0])&(0xff>>4))<<12) | (((p[1])&(0xff>>2))<<6) | (((p[2])&(0xff>>2))<<0); ++wsz;
        p += 3; n -= 3; ++i;
      }
      else
      if ( ((*p) & (0xff<<3)) == ((*p) & (0xff<<4)) )
      {
        *wsz = (((p[0])&(0xff>>5))<<18) | (((p[1])&(0xff>>2))<<12) | (((p[2])&(0xff>>2))<<6) | (((p[3])&(0xff>>2))<<0); ++wsz;
        p += 4; n -= 4; ++i;
      }
      else
      { abort (); }
    }
    else
    {
      *wsz = *p; ++wsz;
      ++p; --n; ++i;
    }
  }
  *wsz = 0;
  return i;
}

enum
{
  kBOM_Null = 0,
  kBOM_UTF_8,
  kBOM_UTF_16BE,
  kBOM_UTF_16LE,
};

static UINT rGetBOM ( const PBYTE p )
{
  if ( p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF ) return kBOM_UTF_8;
  if ( p[0] == 0xFE && p[1] == 0xFF ) return kBOM_UTF_16BE;
  if ( p[0] == 0xFF && p[1] == 0xFE ) return kBOM_UTF_16LE;
  return kBOM_Null;
}


/*
  @ w7...                       строка UTF16 хранящая длину строки первым символом и оканчивающаяся нулеым символом
    w7...[0]                    длина строки в символах
    w7...[0] * sizeof(WCHAR)    размер строки
          +1 * sizeof(WCHAR)    размер строки с конечным нулём
          +2 * sizeof(WCHAR)    размер всех данных
    w7... +1                    указатель на строку с конечным нулём
    w7...[w7...[0]   ]          последний символ (не ноль)
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
static VOID rW7_PushSymbol ( const LPWSTR w7Dst, const WCHAR nSymbol )
{ ++w7Dst[0]; w7Dst[w7Dst[0]] = nSymbol; w7Dst[w7Dst[0]+1] = L'\0'; }
static LPWSTR rW7_Alloc ( const UINT n ) { return (LPWSTR) malloc ( (n+2) * sizeof(WCHAR) ); }
static VOID rW7_Free ( const LPWSTR w7 ) { free ( w7 ); }

/*
  @ v7...                       Вектор, т.е. массив переменной длины
  UINT16[0]                     Количество выделенного места под элементы (в размерах элемента)
  UINT16[1]                     Количество занятого места (в размерах элемента)
  UINT16[2]                     Размер элемента (в байтах)
  UINT16[3]                     неиспользуемое поле
*/
#define rV7_GetHeadPtr(v7) ((UINT16*)((UINT_PTR)(v7)-(sizeof(UINT16)*4)))

static UINT rV7_GetSize ( VOID const * const v7 )
{
  return rV7_GetHeadPtr(v7)[1];
}
// Выделяет место под вектор
static LPVOID rV7_Alloc ( const UINT nElementSize, UINT nCount )
{
  if ( nCount == 0 ) { nCount = 4; }
  UINT16 * const u = malloc ( (nElementSize*nCount) + (sizeof(UINT16)*4) );
  u[0] = nCount;
  u[1] = 0;
  u[2] = nElementSize;
  u[4] = 0xFEF0;
  const LPVOID o = (LPVOID)(u+4);
  memset ( o, 0, u[0]*u[2] );
  return o;
}
static LPWSTR * rV7_Alloc_W7 ( UINT nCount )
{
  return rV7_Alloc ( sizeof(WCHAR), nCount );
}
// Создаёт копию вектора
static LPVOID rV7_Copy ( const LPVOID v7, UINT nCount )
{
  if ( nCount == 0 ) { nCount = rV7_GetHeadPtr ( v7 ) [1]; }
  const LPVOID _v7 = rV7_Alloc ( rV7_GetHeadPtr ( v7 ) [2], nCount );
  memcpy ( _v7, v7, rV7_GetHeadPtr ( v7 ) [2] );
  return _v7;
}
// Освобождает вектор
static VOID rV7_Free ( const LPVOID v7 )
{
  free ( rV7_GetHeadPtr ( v7 ) );
}
static VOID rV7_Free_W7 ( LPWSTR * const v7 )
{
  const UINT n = rV7_GetSize ( v7 );
  for ( UINT i = 0; i < n; ++i )
  {
    if ( v7[i] ) rW7_Free ( v7[i] );
  }
  rV7_Free ( v7 );
}
// Добавляет элемент в конец вектора
static LPVOID rV7_Add ( const LPVOID v7, const LPVOID p )
{
  UINT16 * const u = rV7_GetHeadPtr ( v7 );
  // Если неосталось места
  if ( u[0] <= u[1] )
  {
    const LPVOID _v7 = rV7_Copy ( v7, u[0]*2 );
    free ( v7 );
    return rV7_Add ( _v7, p );
  }
  memcpy ( (LPVOID)( ((UINT_PTR)v7) + (u[1]*u[2]) ), p, u[2] );
  ++u[1];
  return v7;
}



static UINT rLog ( const LPCWSTR fmt, ... )
{
  static FILE * pFLog = NULL;
  if ( (!fmt) && (pFLog) ) { fclose ( pFLog ); pFLog = NULL; return 0; }
  if ( !pFLog ) { pFLog = rOpenFileToWriteWith_UTF16_BOM ( L".ag47.log" ); }
  va_list args;
  va_start ( args, fmt );
  UINT i = vfwprintf ( pFLog, fmt, args );
  va_end ( args );
  return i;
}

static struct
{
  LPWSTR *              vw7PathIn;
  LPWSTR *              vw7PostfixLas;
  LPWSTR *              vw7PostfixIncl;
  LPWSTR *              vw7PostfixAr;
  LPWSTR                w7PathOut;
  BOOL                  bReCreate;
} gScript = {};

static VOID rScriptRelease ( )
{
  if ( gScript.vw7PathIn )
  {
    rV7_Free_W7 ( gScript.vw7PathIn );
    gScript.vw7PathIn = NULL;
  }
  if ( gScript.vw7PostfixLas )
  {
    rV7_Free_W7 ( gScript.vw7PostfixLas );
    gScript.vw7PostfixLas = NULL;
  }
  if ( gScript.vw7PostfixIncl )
  {
    rV7_Free_W7 ( gScript.vw7PostfixIncl );
    gScript.vw7PostfixIncl = NULL;
  }
  if ( gScript.vw7PostfixAr )
  {
    rV7_Free_W7 ( gScript.vw7PostfixAr );
    gScript.vw7PostfixAr = NULL;
  }

  if ( gScript.w7PathOut )
  {
    rW7_Free ( gScript.w7PathOut );
    gScript.w7PathOut = NULL;
  }
}

static VOID rScriptInit ( )
{
  rScriptRelease ( );
  gScript.bReCreate = FALSE;
}

static UINT rScriptParse ( LPWSTR p )
{
  rScriptInit();
  UINT nLine = 1;
  UINT nError;
  VOID _rSkipWs ( )
  {
    while ( isspace ( *p ) )
    {
      if ( *p == '\n' ) { ++nLine; }
      ++p;
    }
  }
  VOID _rSkipToNewLine ( )
  {
    while ( *p != '\n' && *p ) { ++p; }
    if ( *p == '\n' ) { ++nLine; ++p; }
  }
  VOID _rSkipComment ( )
  {
    while ( *p )
    {
      if ( *p == '\n' ) { ++nLine; }
      if ( *p == '*' && p[1] == '/' ) { p += 2; return; }
      ++p;
    }
  }
  UINT _rCmp ( LPCSTR wsz )
  {
    UINT i = 0;
    while ( *wsz )
    {
      if ( p[i] != *wsz ) { return 0; }
      ++i; ++wsz;
    }
    if ( isalnum ( p[i] ) ) { return 0; }
    return i;
  }
  UINT _rGetStringSize ( )
  {
    UINT i = 0;
    while ( p[i] != '\"' && p[i] ) { ++i; }
    return i;
  }

  UINT _rValArrayOfStrings ( LPWSTR ** val )
  {
    _rSkipWs ( );
    if ( *p != '=' ) { return __LINE__; }
    ++p;
    _rSkipWs ( );
    if ( *p != '[' ) { return __LINE__; }
    ++p;

    if ( *val )
    {
      rV7_Free_W7 ( *val );
      *val = NULL;
    }
    *val = rV7_Alloc_W7 ( 0 );

    while ( TRUE )
    {
      LPWSTR w7;
      _rSkipWs ( );
      switch ( *p )
      {
        case 0:
          return __LINE__;
        case '\"':
        {
          ++p;
          const UINT nTemp = _rGetStringSize ( );
          w7 = rW7_Alloc ( nTemp );
          p[nTemp] = 0;
          rW7_set ( w7, p );
          *val = rV7_Add ( *val, &w7 );
          p += nTemp;
          ++p;
          _rSkipWs ( );
          if ( *p == ',' ) { ++p; continue; }
          if ( *p != ']' ) { return __LINE__; }
          continue;
        }
        case ']':
          ++p;
          _rSkipWs ( );
          if ( *p != ';' ) { return __LINE__; }
          ++p;
          return 0;
      }
    }
  }

  UINT _rValString ( LPWSTR * pw7 )
  {
    _rSkipWs ( );
    if ( *p != '=' ) { return __LINE__; }
    ++p;
    _rSkipWs ( );
    if ( *p != '\"' ) { return __LINE__; }
    ++p;
    const UINT nTemp = _rGetStringSize ( );
    if ( *pw7 )
    {
      rW7_Free ( *pw7 );
      *pw7 = NULL;
    }
    *pw7 = rW7_Alloc ( nTemp );
    p[nTemp] = 0;
    rW7_set ( *pw7, p );
    p += nTemp;
    ++p;
    _rSkipWs ( );
    if ( *p != ';' ) { return __LINE__; }
    ++p;
    return 0;
  }

  UINT _rValNull ( )
  {
    _rSkipWs ( );
    if ( *p != ';' ) { return __LINE__; }
    ++p;
    return 0;
  }

  UINT _rValName ( )
  {
    UINT nTemp;

    if ( ( nTemp = _rCmp ( "PATH_IN" ) ) ) { p += nTemp; if ( ( nTemp = _rValArrayOfStrings ( &(gScript.vw7PathIn) ) ) ) { return nTemp; } return 0; }
    if ( ( nTemp = _rCmp ( "FORMAT_LAS" ) ) ) { p += nTemp; if ( ( nTemp = _rValArrayOfStrings ( &(gScript.vw7PostfixLas) ) ) ) { return nTemp; } return 0; }
    if ( ( nTemp = _rCmp ( "FORMAT_INCL" ) ) ) { p += nTemp; if ( ( nTemp = _rValArrayOfStrings ( &(gScript.vw7PostfixIncl) ) ) ) { return nTemp; } return 0; }
    if ( ( nTemp = _rCmp ( "FORMAT_AR" ) ) ) { p += nTemp; if ( ( nTemp = _rValArrayOfStrings ( &(gScript.vw7PostfixAr) ) ) ) { return nTemp; } return 0; }

    if ( ( nTemp = _rCmp ( "PATH_OUT" ) ) ) { p += nTemp; if ( ( nTemp = _rValString ( &(gScript.w7PathOut) ) ) ) { return nTemp; } return 0; }

    if ( ( nTemp = _rCmp ( "RECREATE" ) ) ) { p += nTemp; if ( ( nTemp = _rValNull ( ) ) ) { return nTemp; } gScript.bReCreate = TRUE; return 0; }
    if ( ( nTemp = _rCmp ( "NORECREATE" ) ) ) { p += nTemp; if ( ( nTemp = _rValNull ( ) ) ) { return nTemp; } gScript.bReCreate = FALSE; return 0; }

    return __LINE__;
  }

  {
    UINT nTemp;
    while ( TRUE )
    {
      _rSkipWs();
      switch ( *p )
      {
        case '/':
          if ( p[1] == '/' ) { p += 2; _rSkipToNewLine(); continue; }
          if ( p[1] == '*' ) { p += 2; _rSkipComment(); continue; }
          nError = __LINE__; goto P_Error;
        case 'A' ... 'Z':
        case 'a' ... 'z':
          if ( ( nTemp = _rValName() ) ) { nError = nTemp; goto P_Error; }
          continue;
        case 0:
          goto P_Ok;
        default:
          nError = __LINE__; goto P_Error;
      }
    }
  }
  P_Error:
    rLog ( L"!ERROR: [%u] Ошибка синтаксиса (Line in script %u)\n", nError, nLine );
    return nError;

    void _rLogVW7 ( const LPCWSTR wsz, LPWSTR * v7 )
    {
      if ( !v7 ) {
        rLog ( L"~ %-16s = NULL\n", wsz );
        return;
      }
      const UINT n = rV7_GetSize ( v7 );
      rLog ( L"~ %-16s = [%d]\n", wsz, n );
      for ( UINT i = 0; i < n; ++i )
      { rLog ( L"~ %-16s[%d] = \"%s\"\n", wsz, i, v7[i]+1 ); }
    }
    void _rLogW7 ( const LPCWSTR wsz, LPWSTR w7 )
    {
      rLog ( L"~ %-16s = \"%s\"\n", wsz, w7+1 );
    }
    void _rLogBool ( const LPCWSTR szT, const LPCWSTR szF, const BOOL b )
    {
      if ( b && szT ) { rLog ( L"~ %-16s\n", szT ); }
      if ( !b && szF ) { rLog ( L"~ %-16s\n", szF ); }
    }

  P_Ok:

    // _rLogVW7  ( L"PATH_IN", gScript.vw7PathIn );
    _rLogVW7  ( L"FORMAT_LAS", gScript.vw7PostfixLas );
    _rLogVW7  ( L"FORMAT_INCL", gScript.vw7PostfixIncl );
    _rLogVW7  ( L"FORMAT_AR", gScript.vw7PostfixAr );

    _rLogW7   ( L"PATH_OUT", gScript.w7PathOut );

    _rLogBool ( L"RECREATE", L"NORECREATE", gScript.bReCreate );

    return 0;
}

static UINT rScriptOpen ( const LPCWSTR wszFilePath )
{
  rLog ( L"Скрипт (\"%s\")\n", wszFilePath );
  WIN32_FIND_DATA ffd;
  {
    HANDLE hFind;
    hFind = FindFirstFile ( wszFilePath, &ffd );
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog ( L"!ERROR: FindFirstFile (0x%X) (\"%s\")\n", (UINT)GetLastError(), wszFilePath );
      return __LINE__;
    }
    FindClose ( hFind );
  }
  BYTE buf [ ffd.nFileSizeLow+1 ];
  rLoadFile ( buf, wszFilePath, ffd.nFileSizeLow );
  BYTE * p = buf;
  if ( rGetBOM ( p ) == kBOM_UTF_8 )
  {
    p += 3;
    UINT n = rUTF8_GetLength ( p, ffd.nFileSizeLow - 3 );
    WCHAR wBuf [ n+1 ];
    UINT n2 = rUTF8_ToWide ( p, ffd.nFileSizeLow - 3, wBuf );
    wBuf[n2] = L'\0';
    return rScriptParse ( wBuf );
  }
  rLog ( L"!ERROR: неизвестная кодировка файла скрипта (\"%s\")\n", wszFilePath );
  return __LINE__;
}

static FILE * pFTree            = NULL;
#if 0
static UINT rLogTree ( const LPCWSTR fmt, ... )
{
  if ( (!fmt) && (pFTree) )
  {
    fclose ( pFTree );
    pFTree = NULL;
  }
  if ( !pFTree )
  {
    WCHAR w7[kPathMaxLen];
    rW7_addf ( w7, L"/" )
    pFTree = _wfopen (  );
  }
}
#endif
static FILE * pFSection         = NULL;
static FILE * pFMethods         = NULL;
static FILE * pFTable           = NULL;
static FILE * pFCopy            = NULL;
static FILE * pFErros           = NULL;















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

#define D7SKIP_WHILE(a) while ((a) && *p) { ++p; }
#define D7SKIP_WHILE_SPACE() D7SKIP_WHILE(D7_IF_SPACE(*p))
#define D7_IF_CRLF(a) (((a) == '\n') || ((a) == '\r'))
#define D7_IF_SPACE(a) (((a) == ' ') || ((a) == '\t'))

#define D7PRNTF(...) fwprintf ( pF, L"    " __VA_ARGS__ );
#define D7PRNT_EL(a) D7PRNT_E(L"(Line %u) %s\n",nLine,a)
#define D7_LAS_SET_nD(i) nD[i] = (LONG_PTR)(p)-(LONG_PTR)(pD[i]);
#define D7_TRIM_LAST_SPACE(_pD,_nD) while ( (_nD) && D7_IF_SPACE((_pD)[_nD-1]) ) { --(_nD); }

static FILE * pF = NULL;
static FILE * pF_S = NULL;
static FILE * pF_M = NULL;
static FILE * pF_O = NULL;

// Обрабатываемый путь
static WCHAR w7Path [ kPathMaxLen ];




/* Разбор данных LAS */
static VOID rParseLasData ( BYTE const * const pData, const UINT nSize )
{
  {
    // const LPCSTR lc = rGetCodePage ( pData, nSize );
    // setlocale ( LC_ALL, lc );
    // D7PRNTF ( L"~ Размер файла: %d KiB\n", nSize/1024 );
    // D7PRNTF ( L"~ Кодировка: CP%hs\n", lc );
  }
  BYTE const * p = pData;       // Указатель на обрабатываемый байт
  UINT nLine = 1;               // Номер обрабатываемой линии
  BYTE iSection = '\0';         // Символ названия секции

  // 0,1,2,3 -- MNEM, UNIT, DATA, DESC
  // 4       -- METD
  BYTE const * pD[5] = { };
  UINT nD[5] = { };
  // 0,1,2,3 -- STRT, STOP, STEP, NULL
  double fD[4] = { };
  BOOL bD[4] = { };

  // METHODS
  #define kMethodsMax 16
  BYTE const * pC [kMethodsMax] = { };
  UINT         nC [kMethodsMax] = { };
  double       fCA[kMethodsMax] = { }; // Start of method
  double       fCB[kMethodsMax] = { }; // End of method
  UINT         kC = 0; // Count of methods

  // Изначально находимся на новой строке, поэтому сразу переходим к выбору секции
  goto P_State_NewLine;

  P_SkipLine: // Переход на новую строку
    D7SKIP_WHILE ( !D7_IF_CRLF ( *p ) );
    if ( D7_IF_CRLF ( *p ) ) { ++p; ++nLine; }
    else { goto P_End; }

  P_State_NewLine: // Начало новой строки
    D7SKIP_WHILE ( D7_IF_SPACE ( *p ) || D7_IF_CRLF ( *p ) );
    if ( *p == '~' )
    {
      // Начало секции
      switch ( p[1] )
      {
        case 'V': case 'W': case 'C': case 'P': case 'O': iSection = p[1]; goto P_SkipLine;
        case 'A': goto P_Section_A;
        default: D7PRNT_EL ( L"Некорректное значение начала секци" ); goto P_SkipLine;
      }
    }
    else if ( *p == '#' ) { goto P_SkipLine; }

    ////  MNEM
    // Разбираем строку секции
    nD[3] = nD[2] = nD[1] = nD[0] = 0;
    // Разбираем мнемонику
    if ( !isalnum ( *p ) && *p && *p < 0x80 ) { D7PRNT_EL ( L"Некорректное название мнемоники" ); goto P_SkipLine; }
    pD[0] = p;
    while ( isalnum ( *p ) || *p == '_' || *p > 0x80 || *p == '(' || *p == ')' )
    {
      if ( *p == ')' )  { D7PRNT_EL ( L"Закрывающая скобка без открывабщей в названии мнемоники" ); goto P_SkipLine; }
      else
      // Если нашли открывающую скобку
      if ( *p == '(' )
      {
        // Ищем закрывающую скобку
        while ( *p != ')' && *p ) { ++p; }
        if ( *p == ')' ) { ++p; }
        else { D7PRNT_EL ( L"Отсутсвует закрывающая скобка в названии мнемоники" );  goto P_SkipLine; }
        // Считается что после скобок пустая строка до точки
        break;
      }
      ++p;
    }
    D7_LAS_SET_nD ( 0 );
    D7SKIP_WHILE_SPACE();
    if ( *p != '.' ) { D7PRNT_EL ( L"Отсутсвует разделитель [.] после мнемоники" ); goto P_SkipLine; }
    ++p;
    ////  UNITS
    // размерность данных
    pD[1] = p;
    // доходим до первого пробела
    while ( !isspace ( *p ) && *p ) { ++p; }
    D7_LAS_SET_nD ( 1 );
    D7SKIP_WHILE_SPACE();
    ////  DATA
    // данные
    pD[2] = p;
    // доходим до разделителя
    while ( *p != ':' && !D7_IF_CRLF(*p) && *p ) { ++p; }
    if ( *p != ':' ) { D7PRNT_EL ( L"Отсутсвие разделитель [:] после данных" ); goto P_SkipLine; }
    D7_LAS_SET_nD ( 2 );
    ++p;
    D7_TRIM_LAST_SPACE(pD[2],nD[2]);
    D7SKIP_WHILE_SPACE();
    ////  DESCRIPTION
    // данные описания
    pD[3] = p;
    while ( !D7_IF_CRLF(*p) && *p ) { ++p; }
    D7_LAS_SET_nD ( 3 );
    D7_TRIM_LAST_SPACE(pD[3],nD[3]);

    fwprintf ( pF_S, L"~%c %-8.*hs.%-8.*hs %-32.*hs : %-32.*hs  %s\n",
            iSection,
            nD[0],pD[0],nD[1],pD[1],nD[2],pD[2],nD[3],pD[3],
            w7Path+1 );

    switch ( iSection )
    {
      case 'W':
        #define D7_IF_MNEM(a) ( ( memcmp ( pD[0], a, nD[0] ) == 0 ) && ( nD[0] == sizeof(a) - 1 ) )
        #define D7_IF_DATA(a) ( ( memcmp ( pD[2], a, nD[2] ) == 0 ) && ( nD[2] == sizeof(a) - 1 ) )
        #define D7_IF_DESC(a) ( ( memcmp ( pD[3], a, nD[3] ) == 0 ) && ( nD[3] == sizeof(a) - 1 ) )
        #define D7_SSSN(a) LPSTR pp; fD[a] = strtod ( (LPCSTR)(pD[2]), &pp ); bD[a] = ((LONG_PTR)pp != (LONG_PTR)pD[2]);

        // Разбираем STRT, STOP, STEP и NULL
        if ( D7_IF_MNEM ( "STRT" ) ) { D7_SSSN(0); }
        else
        if ( D7_IF_MNEM ( "STOP" ) ) { D7_SSSN(1); }
        else
        if ( D7_IF_MNEM ( "STEP" ) ) { D7_SSSN(2); }
        else
        if ( D7_IF_MNEM ( "NULL" ) ) { D7_SSSN(3); }
        else
        if ( D7_IF_MNEM ( "METD" ) )
        {
          if ( !D7_IF_DATA ( "METHOD" ) )
          { pD[4] = pD[2]; nD[4] = nD[2]; }
          else
          if ( !D7_IF_DESC ( "METHOD" ) )
          { pD[4] = pD[3]; nD[4] = nD[3]; }
        }
        else
        if ( D7_IF_MNEM ( "WELL" ) )
        {

        }
        break;
      case 'C':
        // Находим методы ГИС
        if ( kC == 0 )
        {
          if ( D7_IF_MNEM ( "DEPT" ) || D7_IF_MNEM ( "DEPTH" ) ) { ++kC; goto P_SkipLine; }
          else { D7PRNT_EL ( L"Первый параметр секции ~C не глубина" ); goto P_SkipLine; }
        }
        else
        {
          pC[kC-1] = pD[0];
          nC[kC-1] = nD[0];
          fwprintf ( pF_M, L"%-16.*hs%-64.*hs%-64.*hs%s\n", nD[0],pD[0],nD[2],pD[2],nD[3],pD[3], w7Path+1 );
          ++kC;
          goto P_SkipLine;
        }
        break;
    }

    goto P_SkipLine;
  P_Section_A:
  {
    if ( kC == 0 ) goto P_End;
    // Секция с числовыми данными
    UINT nK = 0;
    --kC;
    BOOL b = FALSE;
    if ( bD[0] == FALSE ) { D7PRNT_EL ( L"Не удалось получить значение поля [STRT]" ); b = TRUE; }
    if ( bD[1] == FALSE ) { D7PRNT_EL ( L"Не удалось получить значение поля [STOP]" ); b = TRUE; }
    if ( bD[2] == FALSE ) { D7PRNT_EL ( L"Не удалось получить значение поля [STEP]" ); b = TRUE; }
    if ( bD[3] == FALSE ) { D7PRNT_EL ( L"Не удалось получить значение поля [NULL]" ); b = TRUE; }
    if ( b ) goto P_End;
    if ( fD[1] - fD[0] > 0.0 )
    {
      nK = (UINT)(((fD[1] - fD[0]) / fD[2]));
    }
    else
    {
      D7PRNT_EL ( L"[ STOP < STRT ]" ); goto P_End;
    }
    D7SKIP_WHILE ( !D7_IF_CRLF ( *p ) );
    if ( D7_IF_CRLF ( *p ) ) { ++p; ++nLine; }
    else { goto P_End; }

    // Устанавливаем верхнюю границу снизу, а нижнюю сверху
    for ( UINT i = 0; i < kC; ++i )
    { fCA[i] = fD[1]; fCB[i] = fD[0]; }
    // Начинаем считыавать числа
    LPSTR pp;
    // Первое число -- значение глубины
    double fDD = strtod ( (LPCSTR)p, &pp );
    UINT nKK = 0;
    // Пока числа считываются
    while ( (LONG_PTR)p != (LONG_PTR)pp )
    {
      ++nKK;
      p = (BYTE const *)(pp);
      for ( UINT i = 0; i < kC; ++i )
      {
        // считываем значение
        double f = strtod ( (LPCSTR)p, &pp );
        // если он не близок к значению NULL, т.е. значение существует
        #define kAsciiDataErr 0.01
        if ( fabs ( f-fD[3] ) > kAsciiDataErr )
        {
          // если верхняя граница ниже настоящего значения, то записываем и с другой границе также
          if ( fCA[i] >= fDD ) { fCA[i] = fDD; }
          if ( fCB[i] <= fDD ) { fCB[i] = fDD; }
        }
        if ( (LONG_PTR)p == (LONG_PTR)pp )
        {
          D7PRNT_E ( L"Ошибка чтения данных в секции ASCII (%u/%u)\n", nKK, nK );
          goto P_End;
        }
        p = (BYTE const *)(pp);
      }
      fDD = strtod ( (LPCSTR)p, &pp );
    }
    if ( nKK < nK )
    {
      D7PRNT_E ( L"Не все данные секции ASCII были прочитаны %u/%u\n", nKK, nK );
    }
  }


  P_End:
    // Конец обработки файла, анализ и копирование

    fwprintf ( pF_O, L"% 8i\t%.6f\t%.6f\t%.6f\t%.6f\t%8.*hs\t|", 0,
              fD[0], fD[1], fD[2], fD[3],
              nD[4], pD[4] );
    for ( UINT k = 0; k < kC; ++k )
    {
      for(UINT i = 0; i < nC[k]; ++i)
      { if( ispunct( pC[k][i] ) ) nC[k] = i; break; }
      fwprintf ( pF_O, L"%8.*hs\t%.6f\t%.6f\t|", nC[k], pC[k], fCA[k], fCB[k] );
    }
    fwprintf ( pF_O, L"\t\t\t%s\n", w7Path+1 );

  return;
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

/* Разбирает указаный в w7Path путь */
static UINT rParsePath ( )
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
  if ( argc == 1 )
  {
    return rScriptOpen ( L".ag47-script" );
  }
  // printf ( "setlocale: %s\n", setlocale ( LC_ALL, "" ) );
  // pF = rOpenFileToWriteWith_UTF16_BOM ( L"out.log" );
  // pF_S = rOpenFileToWriteWith_UTF16_BOM ( L"sections.log" );
  // pF_M = rOpenFileToWriteWith_UTF16_BOM ( L"methods.log" );
  // pF_O = rOpenFileToWriteWith_UTF16_BOM ( L"table.log" );
  // fwprintf ( pF, L"ARGC >> %d\n", argc );
  // for ( UINT i = 0; i < argc; ++i )
  // { fwprintf ( pF, L"ARGV[%d] >> %s\n", i, argv[i] ); }
  // for ( UINT i = 0; envp[i]; ++i )
  // { fwprintf ( pF, L"ENVP[%d] >> %s\n", i, envp[i] ); }

  // if ( argc == 1 )
  // {
  //   // rW7_set ( w7Path, L"F:\\ARGilyazeev\\github\\Ag47\\t_data" );

  //   rParseFile();
  // }
  // else
  // for ( UINT i = 1; i < argc; ++i )
  // {
  //   rW7_set ( w7Path, argv[i] );
  //   rParseFile();
  // }

  // printf ( "%d\n", atoi ( "   1312093saodakskdo") );

  // fclose ( pF_O );
  // fclose ( pF_M );
  // fclose ( pF_S );
  // fclose ( pF );
  return 0;
}
