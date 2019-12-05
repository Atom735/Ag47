#define IN_LIBXML

#include <windows.h>

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include <archive.h>
#include <archive_entry.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/globals.h>

#include "cyrillic.c"
#include "crc32.c"
#include "ag47_strings.c"

// [\x00-\x09\x0B-\x1f]
// F:\ARGilyazeev\github\Ag47\t_data\2012г\2253\2253_1.09\z2253_NC_ок.doc
// F:\ARGilyazeev\github\Ag47\.ag47\.ag47\NC.docx
// wordconv.exe -oice -nme <input file> <output file>
// wordconv.exe -oice -nme "F:\ARGilyazeev\github\Ag47\t_data\2012г\2253\2253_1.09\z2253_NC_ок.doc" "F:\ARGilyazeev\github\Ag47\.ag47\.ag47\NC.docx"



#define kPathMaxLen 512
UINT kFilesParsedPrint = 0xff;
WCHAR g_w7PathToWordConv[kPathMaxLen] = { };

// #define D7_printf(...) wprintf ( L##__VA_ARGS__ )
#define D7_printf(...) printf ( __VA_ARGS__ )

static UINT rWideCmpWords ( LPCWSTR w1, LPCWSTR w2 )
{
  UINT i = 0;
  while ( *w1 && *w2 )
  {
    if ( (((*w1) >= L'a' && (*w1) <= L'z') || ((*w1) >= L'A' && (*w1) <= L'Z') || ((*w1) >= L'а' && (*w1) <= L'я') || ((*w1) >= L'А' && (*w1) <= L'Я')) &&
         (((*w2) >= L'a' && (*w2) <= L'z') || ((*w2) >= L'A' && (*w2) <= L'Z') || ((*w2) >= L'а' && (*w2) <= L'я') || ((*w2) >= L'А' && (*w2) <= L'Я')) )
    {
      if ( ((*w1)&(0xffff-0x20)) != ((*w2)&(0xffff-0x20)) ) return 0;
    }
    else
    if ( (*w1) != (*w2) ) return 0;
    ++i; ++w1; ++w2;
  }
  if ( (((*w1) >= L'a' && (*w1) <= L'z') || ((*w1) >= L'A' && (*w1) <= L'Z') || ((*w1) >= L'а' && (*w1) <= L'я') || ((*w1) >= L'А' && (*w1) <= L'Я')) ||
       (((*w2) >= L'a' && (*w2) <= L'z') || ((*w2) >= L'A' && (*w2) <= L'Z') || ((*w2) >= L'а' && (*w2) <= L'я') || ((*w2) >= L'А' && (*w2) <= L'Я')) ||
        ((*w1) >= L'0' && (*w1) <= L'9') ||
        ((*w2) >= L'0' && (*w2) <= L'9') || ((*w1) == L'_') || ((*w2) == L'_') )
  {
    return 0;
  }
  return i;
}

static FILE * rOpenFileToWriteWith_UTF16_BOM ( const LPCWSTR wszFname )
{
  FILE * const fd = _wfopen ( wszFname, L"wb" );
  if ( fd ) fwprintf ( fd, L"%c", 0xFEFF );
  return fd;
}


static UINT rLog_v ( const LPCWSTR fmt, va_list args )
{
  static FILE * pFLog = NULL;
  if ( !fmt )
  {
    if ( pFLog ) { fclose ( pFLog ); pFLog = NULL; return 0; }
    return 0;
  }
  if ( !pFLog ) { pFLog = rOpenFileToWriteWith_UTF16_BOM ( L".ag47.log" ); }
  return vfwprintf ( pFLog, fmt, args );
}

static UINT rLog ( const LPCWSTR fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  UINT i = rLog_v ( fmt, args );
  va_end ( args );
  return i;
}

static UINT rLog_Error ( const LPCWSTR fmt, ... )
{
  rLog ( L"!ERROR: " );
  va_list args;
  va_start ( args, fmt );
  UINT i = rLog_v ( fmt, args );
  va_end ( args );
  return i;
}

static UINT rLog_Warn ( const LPCWSTR fmt, ... )
{
  rLog ( L"!WARN: " );
  va_list args;
  va_start ( args, fmt );
  UINT i = rLog_v ( fmt, args );
  va_end ( args );
  return i;
}
static UINT rLog_Error_WinAPI ( const LPCWSTR wszFuncName, const DWORD nErrorCode, const LPCWSTR fmt, ... )
{
  rLog_Error ( L"%s (0x%x) ", wszFuncName, nErrorCode );
  va_list args;
  va_start ( args, fmt );
  UINT i = rLog_v ( fmt, args );
  va_end ( args );
  WCHAR buf[kPathMaxLen*2];
  FormatMessage ( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
          nErrorCode, MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT), buf, kPathMaxLen*2, NULL );
  rLog_Error ( L" ==> %s", buf );
  return i;
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
  if ( !fd ) { rLog_Error ( L"Неудалось открыть файл %s\n", wsz ); }
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
  if ( nCount == 0 ) { nCount = 1; }
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
  return rV7_Alloc ( sizeof(LPWSTR), nCount );
}
// Создаёт копию вектора
static LPVOID rV7_Copy ( const LPVOID v7, UINT nCount )
{
  if ( nCount == 0 ) { nCount = rV7_GetHeadPtr ( v7 ) [1]; }
  const LPVOID _v7 = rV7_Alloc ( rV7_GetHeadPtr ( v7 ) [2], nCount );
  UINT16 * const u = rV7_GetHeadPtr ( _v7 );
  u[1] =  rV7_GetHeadPtr ( v7 ) [1];
  memcpy ( _v7, v7, rV7_GetHeadPtr ( v7 ) [2] * u[1]  );
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
    rV7_Free ( v7 );
    return rV7_Add ( _v7, p );
  }
  memcpy ( (LPVOID)( ((UINT_PTR)v7) + (u[1]*u[2]) ), p, u[2] );
  ++u[1];
  return v7;
}
static LPWSTR * rV7_Add_W7 ( LPWSTR * const v7, LPWSTR w7 )
{
  UINT16 * const u = rV7_GetHeadPtr ( v7 );
  // Если неосталось места
  if ( u[0] <= u[1] )
  {
    const LPVOID _v7 = rV7_Copy ( v7, u[0]*2 );
    rV7_Free ( v7 );
    return rV7_Add_W7 ( _v7, w7 );
  }
  v7[u[1]] = w7;
  ++u[1];
  return v7;
}




static struct
{
  LPWSTR *              vw7PathIn;
  LPWSTR *              vw7PostfixLas;
  LPWSTR *              vw7PostfixIncl;
  LPWSTR *              vw7PostfixAr;
  LPWSTR                w7PathOut;
  BOOL                  bReCreate;

  UINT                  nFilesLas;
  UINT                  nFilesIncl;
  UINT                  iState;
  UINT                  nFilesParsed;
  UINT                  nFilesError;

} gScript = {};

enum
{
  kSSR_Null,
  kSSR_Prepared,
  kSSR_Tree,
  kSSR_Parse,
};

static UINT rLogTree ( const LPCWSTR fmt, ... )
{
  static FILE * pF = NULL;
  if ( !fmt )
  {
    if ( pF ) { fclose ( pF ); pF = NULL; return 0; }
    return 0;
  }
  if ( !pF )
  {
    WCHAR w7[kPathMaxLen];
    rW7_setf ( w7, L"%s/.ag47/_1.tree.log", gScript.w7PathOut+1 );
    pF = rOpenFileToWriteWith_UTF16_BOM ( w7+1 );
  }
  va_list args;
  va_start ( args, fmt );
  UINT i = vfwprintf ( pF, fmt, args );
  va_end ( args );
  return i;
}

static UINT rLogSection ( const LPCWSTR fmt, ... )
{
  static FILE * pF = NULL;
  if ( !fmt )
  {
    if ( pF ) { fclose ( pF ); pF = NULL; return 0; }
    return 0;
  }
  if ( !pF )
  {
    WCHAR w7[kPathMaxLen];
    rW7_setf ( w7, L"%s/.ag47/_2.sections.log", gScript.w7PathOut+1 );
    pF = rOpenFileToWriteWith_UTF16_BOM ( w7+1 );
  }
  va_list args;
  va_start ( args, fmt );
  UINT i = vfwprintf ( pF, fmt, args );
  va_end ( args );
  return i;
}
static UINT rLogMethods ( const LPCWSTR fmt, ... )
{
  static FILE * pF = NULL;
  if ( !fmt )
  {
    if ( pF ) { fclose ( pF ); pF = NULL; return 0; }
    return 0;
  }
  if ( !pF )
  {
    WCHAR w7[kPathMaxLen];
    rW7_setf ( w7, L"%s/.ag47/_2.methods.log", gScript.w7PathOut+1 );
    pF = rOpenFileToWriteWith_UTF16_BOM ( w7+1 );
  }
  va_list args;
  va_start ( args, fmt );
  UINT i = vfwprintf ( pF, fmt, args );
  va_end ( args );
  return i;
}
static UINT rLogTable ( const LPCWSTR fmt, ... )
{
  static FILE * pF = NULL;
  if ( !fmt )
  {
    if ( pF ) { fclose ( pF ); pF = NULL; return 0; }
    return 0;
  }
  if ( !pF )
  {
    WCHAR w7[kPathMaxLen];
    rW7_setf ( w7, L"%s/.ag47/_3.table.log", gScript.w7PathOut+1 );
    pF = rOpenFileToWriteWith_UTF16_BOM ( w7+1 );
  }
  va_list args;
  va_start ( args, fmt );
  UINT i = vfwprintf ( pF, fmt, args );
  va_end ( args );
  return i;
}

// static FILE * pFSection         = NULL;
// static FILE * pFMethods         = NULL;
// static FILE * pFTable           = NULL;
// static FILE * pFCopy            = NULL;
// static FILE * pFErros           = NULL;

static VOID rScriptLog_VW7 ( const LPCWSTR wsz, LPWSTR * v7 )
{
  if ( !v7 ) {
    rLog ( L"~ %s = NULL\n", wsz );
    return;
  }
  const UINT n = rV7_GetSize ( v7 );
  rLog ( L"~ %s = [%d]\n", wsz, n );
  for ( UINT i = 0; i < n; ++i )
  { rLog ( L"~ %s[%d] = \"%s\"\n", wsz, i, v7[i]+1 ); }
}
static VOID rScriptLog_W7 ( const LPCWSTR wsz, LPWSTR w7 )
{
  rLog ( L"~ %s = \"%s\"\n", wsz, w7+1 );
}
static VOID rScriptLog_Bool ( const LPCWSTR szT, const LPCWSTR szF, const BOOL b )
{
  if ( b && szT ) { rLog ( L"~ %s\n", szT ); }
  if ( !b && szF ) { rLog ( L"~ %s\n", szF ); }
}

static VOID rScriptLog_All ( )
{
  rScriptLog_VW7  ( L"PATH_IN", gScript.vw7PathIn );
  rScriptLog_VW7  ( L"FORMAT_LAS", gScript.vw7PostfixLas );
  rScriptLog_VW7  ( L"FORMAT_INCL", gScript.vw7PostfixIncl );
  rScriptLog_VW7  ( L"FORMAT_AR", gScript.vw7PostfixAr );

  rScriptLog_W7   ( L"PATH_OUT", gScript.w7PathOut );

  rScriptLog_Bool ( L"RECREATE", L"NORECREATE", gScript.bReCreate );
}

static UINT rSearchWordConv ( )
{
  WIN32_FIND_DATA ffd;
  const HANDLE hFind = FindFirstFile ( L"C:/Program Files (x86)/Microsoft Office/Office*", &ffd );
  if ( hFind == INVALID_HANDLE_VALUE )
  {
    rLog_Error ( L"Невозможно найти папку с установленным MS Office\n" );
    FindClose ( hFind );
    return __LINE__;
  }
  do
  {
    rW7_setf ( g_w7PathToWordConv, L"C:/Program Files (x86)/Microsoft Office/%s/wordconv.exe", ffd.cFileName );
    WIN32_FIND_DATA _ffd;
    const HANDLE _hFind = FindFirstFile ( g_w7PathToWordConv+1, &_ffd );
    if ( _hFind != INVALID_HANDLE_VALUE )
    {
      rLog ( L"wordconv.exe найден по пути: %ls\n", g_w7PathToWordConv+1 );
      FindClose ( _hFind );
      FindClose ( hFind );
      return 0;
    }
  } while ( FindNextFile ( hFind, &ffd ) );
  FindClose ( hFind );
  rLog_Error ( L"Невозможно найти wordconv.exe\n" );
  return __LINE__;
}

static UINT rScriptRun_DocToDocx ( const LPCWSTR wsz )
{
  if ( g_w7PathToWordConv[0] == 0 )
  {
    UINT k;
    if ( ( k = rSearchWordConv ( ) ) ) { return k; }
  }
  WCHAR cmd[kPathMaxLen];
  // LPCWSTR lp = L"F:/ARGilyazeev/github/Ag47/.ag47/.ag47/temp_inkl";
  rW7_setf ( cmd, L"for %%W in (%s/*.doc) do (\"%s\" -oice -nme \"%s/%%~nxW\" \"%s/%%~nxW.docx\")", wsz, g_w7PathToWordConv+1, wsz, wsz );
  _wsystem ( cmd+1 );
  return 0;
}

/*
  Удаляет всё что находится в папаке и подпапках
*/
static UINT rEraseFolderTree ( const LPWSTR w7p )
{
  WIN32_FIND_DATA ffd;
  WCHAR w7[kPathMaxLen];
  rW7_setf ( w7, L"%s/*", w7p+1 );
  const HANDLE hFind = FindFirstFile ( w7+1, &ffd );
  if ( hFind == INVALID_HANDLE_VALUE ) { return 0; }
  do
  {
    rW7_setf ( w7, L"%s/%s", w7p+1, ffd.cFileName );
    if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    {
      if ( ffd.cFileName[0] == '.' )
      {
        if ( ffd.cFileName[1] == '\0' )  { continue; }
        else
        if ( ffd.cFileName[1] == '.' ) { if ( ffd.cFileName[2] == '\0' ) { continue; } }
      }
      const UINT n = rEraseFolderTree ( w7 ); if ( n ) { FindClose ( hFind ); return n; }
      if(!RemoveDirectory ( w7+1 ))
      {
        const DWORD er = GetLastError();
        rLog_Error_WinAPI ( L"RemoveDirectory", er, L" (\"%s\") неизвестная ошибка\n", w7+1 );
        FindClose ( hFind ); return __LINE__;
      }
    }
    else
    {
      if ( !DeleteFile ( w7+1 ) )
      {
        const DWORD er = GetLastError();
        if ( er == ERROR_FILE_NOT_FOUND )
        {
          rLog_Error_WinAPI ( L"DeleteFile", er, L"(\"%s\") файл ненайден\n", w7+1 );
          FindClose ( hFind ); return __LINE__;
        }
        else
        if ( er == ERROR_ACCESS_DENIED )
        {
          rLog_Error_WinAPI ( L"DeleteFile", er, L"(\"%s\") нет доступа к файлу\n", w7+1 );
          FindClose ( hFind ); return __LINE__;
        }
        else
        {
          rLog_Error_WinAPI ( L"DeleteFile", er, L" (\"%s\") неизвестная ошибка\n", w7+1 );
          FindClose ( hFind ); return __LINE__;
        }
      }
    }
  } while ( FindNextFile ( hFind, &ffd ) );
  FindClose ( hFind ); return 0;
}

static VOID rSriptPrepareToRun ( )
{
  if ( !gScript.vw7PathIn ) { gScript.vw7PathIn = rV7_Alloc_W7 ( 0 ); }
  if ( rV7_GetSize ( gScript.vw7PathIn ) == 0 )
  {
    const LPWSTR wsz = rW7_Alloc ( 1 );
    rW7_set ( wsz, L"." );
    gScript.vw7PathIn = rV7_Add_W7 ( gScript.vw7PathIn, wsz );
  }
  if ( !gScript.vw7PostfixLas ) { gScript.vw7PostfixLas = rV7_Alloc_W7 ( 0 ); }
  if ( rV7_GetSize ( gScript.vw7PostfixLas ) == 0 )
  {
    LPWSTR wsz = rW7_Alloc ( 4 ); rW7_set ( wsz, L".las" ); gScript.vw7PostfixLas = rV7_Add_W7 ( gScript.vw7PostfixLas, wsz );
  }
  if ( !gScript.vw7PostfixIncl ) { gScript.vw7PostfixIncl = rV7_Alloc_W7 ( 0 ); }
  if ( rV7_GetSize ( gScript.vw7PostfixIncl ) == 0 )
  {
    LPWSTR wsz = NULL;
    wsz = rW7_Alloc ( 4 ); rW7_set ( wsz, L".doc" ); gScript.vw7PostfixIncl = rV7_Add_W7 ( gScript.vw7PostfixIncl, wsz );
    wsz = rW7_Alloc ( 4 ); rW7_set ( wsz, L".txt" ); gScript.vw7PostfixIncl = rV7_Add_W7 ( gScript.vw7PostfixIncl, wsz );
    wsz = rW7_Alloc ( 5 ); rW7_set ( wsz, L".docx" );gScript.vw7PostfixIncl = rV7_Add_W7 ( gScript.vw7PostfixIncl, wsz );
  }
  if ( !gScript.vw7PostfixAr ) { gScript.vw7PostfixAr = rV7_Alloc_W7 ( 0 ); }
  if ( rV7_GetSize ( gScript.vw7PostfixAr ) == 0 )
  {
    LPWSTR wsz = NULL;
    wsz = rW7_Alloc ( 4 ); rW7_set ( wsz, L".zip" ); gScript.vw7PostfixAr = rV7_Add_W7 ( gScript.vw7PostfixAr, wsz );
    wsz = rW7_Alloc ( 4 ); rW7_set ( wsz, L".rar" ); gScript.vw7PostfixAr = rV7_Add_W7 ( gScript.vw7PostfixAr, wsz );
  }
  if ( !gScript.w7PathOut ) { rW7_set ( gScript.w7PathOut = rW7_Alloc ( 5 ), L".ag47" ); }
  gScript.iState = kSSR_Prepared;
}

static BOOL rW7_PathWithEndOf_W7 ( LPCWSTR w7Path, LPCWSTR w7End )
{
  if ( w7Path[0] < w7End[0] ) return FALSE;
  w7Path += w7Path[0]-w7End[0]+1;
  ++w7End;
  while ( *w7End )
  {
    if ( iswalpha ( *w7Path ) && iswalpha ( *w7End ) )
    {
      if ( ( *w7Path & 0x1f ) != ( *w7End & 0x1f ) ) { return FALSE; }
    }
    else
    {
      if ( *w7Path != *w7End ) { return FALSE; }
    }
    ++w7Path; ++w7End;
  }
  return TRUE;
}
static BOOL rW7_PathWithEndOf_VW7 ( const LPCWSTR w7Path, LPWSTR * const vw7End )
{
  const UINT k = rV7_GetSize ( vw7End );
  for ( UINT i = 0; i < k; ++i )
  {
    if ( rW7_PathWithEndOf_W7 ( w7Path, vw7End[i] ) ) { return TRUE; }
  }
  return FALSE;
}
static UINT rW7_PathLastNameGetSize ( const LPCWSTR w7 )
{
  UINT i = 0;
  while ( (i < w7[0]) && (w7[w7[0]-i] != '/') && (w7[w7[0]-i] != '\\') ) { ++i; }
  return i;
}

static UINT rScriptRun_Tree_AR ( const LPWSTR w7, struct archive * const ar )
{
  struct archive_entry *are;
  while ( archive_read_next_header ( ar, &are ) == ARCHIVE_OK )
  {
    const UINT n = (UINT)archive_entry_size(are);
    const UINT l = w7[0];
    rW7_addf ( w7, L"/%s", archive_entry_pathname_w(are) );

    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixAr ) )
    {
      BYTE buf[n+256];
      la_ssize_t aer = archive_read_data ( ar, buf, n+256 );
      if ( aer < 0 ) { aer = archive_read_data ( ar, buf, n+256 ); }
      if ( aer != n ) { rLog_Error ( L"archive_read_data [%d/%u] (\"%s\")\n", aer, n, w7+1 ); }
      struct archive * const ar2 = archive_read_new();
      archive_read_support_filter_all ( ar2 );
      archive_read_support_format_all ( ar2 );
      UINT er = 0;
      if ( archive_read_open_memory ( ar2, buf, n ) != ARCHIVE_OK )
      {
        rLog_Error ( L"archive_read_open_memory (\"%s\")\n", w7+1 );
        er = __LINE__;
        goto P_Err;
      }
      else
      {
        er = rScriptRun_Tree_AR ( w7, ar2 );
        if ( er ) { goto P_Err; }
      }
      P_Err:
      if ( archive_read_free ( ar2 ) != ARCHIVE_OK )
      {
        rLog_Error ( L"archive_read_free (\"%s\")\n", w7+1 );
      }
      if ( er ) { w7[0] = l; w7[w7[0]+1] = 0; return er; }
    }
    else
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixLas ) )
    {
      BYTE buf[n+256];
      la_ssize_t aer = archive_read_data ( ar, buf, n+256 );
      if ( aer < 0 ) { aer = archive_read_data ( ar, buf, n+256 ); }
      if ( aer != n ) { rLog_Error ( L"archive_read_data [%d/%u] (\"%s\")\n", aer, n, w7+1 ); }
      rLogTree ( L"L%08" TEXT(PRIx32) L"%016" TEXT(PRIx64) L"% 10d %s\n",
            rCRC32 ( buf, n ), rAg47cs ( buf, n ), n, w7+1 );
      ++gScript.nFilesLas;
    }
    else
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixIncl ) )
    {
      BYTE buf[n+256];
      la_ssize_t aer = archive_read_data ( ar, buf, n+256 );
      if ( aer < 0 ) { aer = archive_read_data ( ar, buf, n+256 ); }
      if ( aer != n ) { rLog_Error ( L"archive_read_data [%d/%u] (\"%s\")\n", aer, n, w7+1 ); }
      rLogTree ( L"I%08" TEXT(PRIx32) L"%016" TEXT(PRIx64) L"% 10d %s\n",
            rCRC32 ( buf, n ), rAg47cs ( buf, n ), n, w7+1 );
      ++gScript.nFilesIncl;
    }
    else
    {
      archive_read_data_skip ( ar );
    }

    w7[0] = l; w7[w7[0]+1] = 0;
  }
  return 0;
}

static UINT rScriptRun_Tree_FFD ( const LPWSTR w7, WIN32_FIND_DATA * const _ffd )
{
  const UINT l = w7[0];
  rW7_addf ( w7, L"/%s", _ffd->cFileName );
  if ( _ffd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
  {
    if ( _ffd->cFileName[0] == '.' )
    {
      if ( _ffd->cFileName[1] == '\0' )  { w7[0] = l; w7[w7[0]+1] = 0; return 0; } else
      if ( _ffd->cFileName[1] == '.' )
      { if ( _ffd->cFileName[2] == '\0' ) { w7[0] = l; w7[w7[0]+1] = 0; return 0; } }
    }
    // Продолжаем поиск внутри папки
    WIN32_FIND_DATA ffd;
    const UINT _l = w7[0];
    rW7_addf ( w7, L"/*" );
    const HANDLE hFind = FindFirstFile ( w7+1, &ffd );
    w7[0] = _l; w7[w7[0]+1] = 0;
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog_Error_WinAPI ( L"FindFirstFile", GetLastError(), L"(\"%s\")\n", w7+1 );
      w7[0] = l; w7[w7[0]+1] = 0;
      return __LINE__;
    }

    do
    {
      const UINT k = rScriptRun_Tree_FFD ( w7, &ffd );
      if ( k ) { w7[0] = l; w7[w7[0]+1] = 0; FindClose ( hFind ); return k; }
    } while ( FindNextFile ( hFind, &ffd ) );
    FindClose ( hFind );
  }
  else
  {
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixAr ) )
    {
      struct archive * const ar = archive_read_new();
      archive_read_support_filter_all ( ar );
      archive_read_support_format_all ( ar );
      UINT er = 0;
      FILE * const pf = _wfopen ( w7+1, L"rb" );
      assert ( pf );
      if ( archive_read_open_FILE ( ar, pf ) != ARCHIVE_OK )
      {
        rLog_Error ( L"archive_read_open_FILE (\"%s\")\n", w7+1 );
        er = __LINE__;
        goto P_Err;
      }
      else
      {
        er = rScriptRun_Tree_AR ( w7, ar );
        if ( er ) { goto P_Err; }
      }
      P_Err:
      if ( archive_read_free ( ar ) != ARCHIVE_OK )
      {
        rLog_Error ( L"archive_read_free (\"%s\")\n", w7+1 );
      }
      fclose ( pf );
      if ( er ) { w7[0] = l; w7[w7[0]+1] = 0; return er; }
    }
    else
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixLas ) )
    {
      const UINT n = _ffd->nFileSizeLow;
      BYTE buf[n+1];
      rLoadFile ( buf, w7+1, n );
      rLogTree ( L"L%08" TEXT(PRIx32) L"%016" TEXT(PRIx64) L"% 10d %s\n",
            rCRC32 ( buf, n ), rAg47cs ( buf, n ), n, w7+1 );
      ++gScript.nFilesLas;
    }
    else
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixIncl ) )
    {
      const UINT n = _ffd->nFileSizeLow;
      BYTE buf[n+1];
      rLoadFile ( buf, w7+1, n );
      rLogTree ( L"I%08" TEXT(PRIx32) L"%016" TEXT(PRIx64) L"% 10d %s\n",
            rCRC32 ( buf, n ), rAg47cs ( buf, n ), n, w7+1 );
      ++gScript.nFilesIncl;
    }
  }

  w7[0] = l; w7[w7[0]+1] = 0;
  return 0;
}

static UINT rScriptRun_Tree ( )
{
  rLog ( L"~ script RUN_TREE\n" );
  if ( gScript.iState < kSSR_Tree-1 )
  { rSriptPrepareToRun(); rScriptLog_All(); }
  D7_printf ( "Подсчёт количества файлов\n" );

  // Пытаемся создать папку
  if ( !CreateDirectory ( gScript.w7PathOut+1, NULL ) )
  {
    // Если не получилось создать папку
    const DWORD er = GetLastError();
    if ( er == ERROR_ALREADY_EXISTS )
    {
      // Папка существует, то если флаг RECREATE поднят, то удаляем всё что внутри
      if ( gScript.bReCreate )
      {
        rLogTree ( NULL );
        rLogSection ( NULL );
        rLogMethods ( NULL );
        rLogTable ( NULL );
        const UINT n = rEraseFolderTree ( gScript.w7PathOut ); if ( n ) { return n; }
      }
    }
    else
    if ( er == ERROR_PATH_NOT_FOUND )
    {
      // Не существует пути к подпапке
      // TODO
      rLog_Error_WinAPI ( L"CreateDirectory", er, L"(\"%s\") TODO создать все подпапки\n", gScript.w7PathOut+1 );
      return __LINE__;
    }
    else
    {
      // Другая ошибка
      rLog_Error_WinAPI ( L"CreateDirectory", er, L"(\"%s\") неизвестная ошибка\n", gScript.w7PathOut+1 );
      return __LINE__;
    }
  }
  // Пытаемся создать подпапку для рабочей инфы
  {
    WCHAR w7[kPathMaxLen];
    rW7_setf ( w7, L"%s/.ag47", gScript.w7PathOut+1 );
    if ( !CreateDirectory ( w7+1, NULL ) )
    {
      const DWORD er = GetLastError();
      rLog_Error_WinAPI ( L"CreateDirectory", er, L"(\"%s\") неизвестная ошибка\n", w7+1 );
      return __LINE__;
    }
    rW7_setf ( w7, L"%s/.ag47/error_las", gScript.w7PathOut+1 );
    if ( !CreateDirectory ( w7+1, NULL ) )
    {
      const DWORD er = GetLastError();
      rLog_Error_WinAPI ( L"CreateDirectory", er, L"(\"%s\") неизвестная ошибка\n", w7+1 );
      return __LINE__;
    }
    rW7_setf ( w7, L"%s/.ag47/temp_las", gScript.w7PathOut+1 );
    if ( !CreateDirectory ( w7+1, NULL ) )
    {
      const DWORD er = GetLastError();
      rLog_Error_WinAPI ( L"CreateDirectory", er, L"(\"%s\") неизвестная ошибка\n", w7+1 );
      return __LINE__;
    }
    rW7_setf ( w7, L"%s/.ag47/temp_inkl", gScript.w7PathOut+1 );
    if ( !CreateDirectory ( w7+1, NULL ) )
    {
      const DWORD er = GetLastError();
      rLog_Error_WinAPI ( L"CreateDirectory", er, L"(\"%s\") неизвестная ошибка\n", w7+1 );
      return __LINE__;
    }

  }
  // Просматриваем все данные подпапки
  {
    WIN32_FIND_DATA ffd;
    WCHAR w7[kPathMaxLen];
    const UINT n = rV7_GetSize ( gScript.vw7PathIn );
    for ( UINT i = 0; i < n; ++i )
    {
      rW7_setf ( w7, L"%s/*", gScript.vw7PathIn[i]+1 );
      const HANDLE hFind = FindFirstFile ( w7+1, &ffd );
      w7[0] = gScript.vw7PathIn[i][0];
      w7[w7[0]+1] = 0;
      if ( hFind == INVALID_HANDLE_VALUE )
      {
        rLog_Error_WinAPI ( L"FindFirstFile", GetLastError(), L"(\"%s\")\n", w7+1 );
        return __LINE__;
      }
      do
      {
        const UINT k = rScriptRun_Tree_FFD ( w7, &ffd );
        if ( k ) { FindClose ( hFind ); return k; }
      } while ( FindNextFile ( hFind, &ffd ) );
      FindClose ( hFind );
    }
  }
  rLog ( L"#tree LAS: %u INCL: %u\n", gScript.nFilesLas, gScript.nFilesIncl );
  D7_printf ( "Количество файлов LAS: %u INCL: %u\n", gScript.nFilesLas, gScript.nFilesIncl );
  kFilesParsedPrint = (( gScript.nFilesLas + gScript.nFilesIncl ) / 10 ) + 1;
  gScript.iState = kSSR_Tree;
  return 0;
}


static UINT rParseLas ( const LPCWSTR w7, BYTE const * p, UINT n )
{
  BYTE const * const pBufBegin = p;
  const UINT nBufSize = n;
  UINT nLine = 1;
  BYTE iSection = 0;
  const UINT iCP = rGetCodePage ( p, n );
  setlocale ( LC_ALL, g_aszCyrillicTableLocaleNames[iCP] );
  UINT iState = 0; // 0 - Normal, 1 - Warning, 2 - Error
  UINT iErr = 0;
  LPCWSTR sErr = NULL;

  struct
  {
    BYTE const *        p;
    UINT                n;
  } aMNEM, aUNIT, aDATA, aDESC, aLine,
    aWELL_W1 = { }, aWELL_W2 = { },
    aWELL_O1 = { }, aWELL_O2 = { },
    aWN_W1   = { }, aWN_W2   = { };

  struct
  {
    BYTE const *        p;
    UINT                n;
    union {
      double            v;
      UINT              u;
    };
  } aSTRT = { }, aSTOP = { }, aSTEP = { }, aNULL = { }, aWELL = { }, aMETD = { };

  struct
  {
    BYTE const *        p;
    UINT                n;
    double              fA;
    double              fB;
  } * aMethods, aMethodT;
  aMethods = rV7_Alloc ( sizeof(*aMethods), 0 );

  BOOL _bLog = TRUE;
  BOOL _bLogSecton = TRUE;
  // Изначально находимся на новой строке, поэтому сразу переходим к выбору секции
  goto P_State_NewLine;


  VOID _rLog ( const LPCWSTR wsz )
  {
    if ( _bLog ) { rLog ( L"$ Разбор файла: %s\n", w7+1 ); _bLog = FALSE; }
    UINT k = 0;
    while ( aLine.p[k] && !( aLine.p[k] == '\n' || aLine.p[k] == '\r' ) ) { ++k; }
    rLog_Warn ( L"(Line %u) ~%hc %s      ^ %.*hs\n", nLine, iSection, wsz, k, aLine.p );
  }

  VOID _rLogS ( const LPCWSTR wsz, const BOOL b )
  {
    if ( _bLogSecton )
    {
      rLogSection ( L"==> FILE = %s\n", w7+1 );
      _bLogSecton = FALSE;
    }
    rLogSection ( L"(Line %u) ~%hc %s", nLine, iSection, wsz );
    if ( b )
    {
      UINT k = 0;
      while ( aLine.p[k] && !( aLine.p[k] == '\n' || aLine.p[k] == '\r' ) ) { ++k; }
      rLogSection ( L"#~%hc %.*hs\n", iSection, k, aLine.p );
    }
  }


  UINT _rCmp ( BYTE const * pb, CHAR const * sz )
  {
    UINT i = 0;
    while ( *sz )
    {
      if ( *pb != (BYTE)(*sz) ) { return 0; }
      ++pb; ++sz; ++i;
    }
    if ( isalnum ( *pb ) ) { return 0; }
    return i;
  }

  P_SkipLine: // Переход на новую строку
    while ( n && !( *p == '\n' || *p == '\r' ) ) { ++p; --n; }
  P_State_NewLine: // Начало новой строки
    while ( n && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) )
    {
      if ( *p == '\n' ) { ++nLine; }
       ++p; --n;
    }
    aLine.p = p;
    aLine.n = n;
    if ( n == 0 ) goto P_End;
    // Начало секции
    if ( *p == '~' )
    {
      switch ( p[1] )
      {
        case 'V': case 'W': case 'C': case 'P': case 'O': iSection = p[1]; goto P_SkipLine;
        case 'A': goto P_Section_A;
        default:
          sErr = L"Некорректное значение начала секци, прекращаем разбор файла\n";
          _rLog( sErr );
          _rLogS ( sErr, TRUE );
          iState = 2; iErr = __LINE__;
          goto P_End;
      }
    }
    else if ( *p == '#' ) { goto P_SkipLine; }
    if ( iSection == 0 )
    {
      sErr = L"Некорректное начало LAS файла, прекращаем разбор файла\n";
      _rLog ( sErr );
      _rLogS ( sErr, TRUE );
      iState = 2; iErr = __LINE__;
      goto P_End;
    }
    // Разбираем строку секции
    ////  MNEM
    aMNEM.n = n;
    aMNEM.p = p;
    if ( !isalnum ( *p ) && *p && *p < 0x80 )
    {
      sErr = L"Некорректное начало названия мнемоники, пропуск строки\n";
      _rLog ( sErr );
      _rLogS ( sErr, TRUE );
      iState = 2; iErr = __LINE__;
      goto P_SkipLine;
    }
    while ( isalnum ( *p ) || *p == '_' || *p > 0x80 || *p == '(' || *p == ')' )
    {
      if ( *p == ')' )
      {
        sErr = L"Закрывающая скобка без открывающей в названии мнемоники, пропуск строки\n";
        _rLog ( sErr );
        _rLogS ( sErr, TRUE );
        iState = 1; iErr = __LINE__;
        goto P_SkipLine;
      }
      else
      // Если нашли открывающую скобку
      if ( *p == '(' )
      {
        // Ищем закрывающую скобку
        while ( n && *p != ')' ) { ++p; --n; }
        if ( *p == ')' ) { ++p; --n; }
        else
        {
          sErr = L"Отсутсвует закрывающая скобка в названии мнемоники, пропуск строки\n";
          _rLog ( sErr );
          _rLogS ( sErr, TRUE );
          iState = 1; iErr = __LINE__;
          goto P_SkipLine;
        }
        // Считается что после скобок пустая строка до точки
        break;
      }
      ++p; --n;
    }
    aMNEM.n -= n;
    while ( n && ( *p == ' ' || *p == '\t' ) ) { ++p; --n; }
    if ( *p != '.' )
    {
      sErr = L"Отсутсвует разделитель [.] после мнемоники, продолжаем парсинг считая, что далее идут данные\n";
      _rLog ( sErr );
      _rLogS ( sErr, TRUE );
      aUNIT.p = p;
      aUNIT.n = 0;
    }
    else
    {
      ++p; --n;
      ////  UNITS
      aUNIT.n = n;
      aUNIT.p = p;
      while ( n && !( *p == ' ' || *p == '\t' ) ) { ++p; --n; }
      aUNIT.n -= n;
    }
    while ( n && ( *p == ' ' || *p == '\t' ) ) { ++p; --n; }
    ////  DATA
    aDATA.n = n;
    aDATA.p = p;
    // доходим до разделителя
    while ( n && !( *p == ':' || *p == '\r' || *p == '\n' ) ) { ++p; --n; }
    if ( *p != ':' )
    {
      sErr = L"Отсутсвует разделитель [:] после данных, продолжаем парсинг считая, что строка закончилась\n";
      _rLog ( sErr );
      _rLogS ( sErr, FALSE );
      aDESC.p = p;
      aDESC.n = 0;
      aDATA.n -= n;
    }
    else
    {
      aDATA.n -= n;
      ++p; --n;
      while ( n && ( *p == ' ' || *p == '\t' ) ) { ++p; --n; }
      ////  DESCRIPTION
      aDESC.p = p;
      aDESC.n = n;
      while ( n && !( *p == '\r' || *p == '\n' ) ) {  ++p; --n; }
      aDESC.n -= n;
    }
    while ( aDATA.n && ( aDATA.p[aDATA.n-1] == ' ' || aDATA.p[aDATA.n-1] == '\t' ) ) { --aDATA.n; }
    while ( aDESC.n && ( aDESC.p[aDESC.n-1] == ' ' || aDESC.p[aDESC.n-1] == '\t' ) ) { --aDESC.n; }
    aLine.n -= n;

    _rLogS ( L"", TRUE );
    rLogSection ( L"%-16.*hs~%hc.%-16.*hs %-64.*hs:%-64.*hs %s\n", aMNEM.n, aMNEM.p, iSection, aUNIT.n, aUNIT.p, aDATA.n, aDATA.p, aDESC.n, aDESC.p, w7+1 );

    #define D7_PARSER_VAL_SSSN(sz,val) \
      if ( _rCmp ( aMNEM.p, sz ) )\
      {\
        LPSTR pp;\
        val.v = strtod ( (LPCSTR)(aDATA.p), &pp );\
        if ((LONG_PTR)pp != (LONG_PTR)aDATA.p)\
        {\
          val.p = aDATA.p;\
          val.n = aDATA.n;\
        }\
        else\
        {\
          val.v = strtod ( (LPCSTR)(aDESC.p), &pp );\
          if ((LONG_PTR)pp != (LONG_PTR)aDESC.p)\
          {\
            val.p = aDESC.p;\
            val.n = aDESC.n;\
          }\
          else\
          {\
            val.p = NULL;\
            val.n = 0;\
            sErr = L"Неудалось получить значение " TEXT(sz) L", прекращаем разбор файла\n";\
            _rLog ( sErr );\
            iState = 2; iErr = __LINE__;\
            goto P_End;\
          }\
        }\
      }
    if ( iSection == 'W' )
    {
      D7_PARSER_VAL_SSSN("STRT",aSTRT)
      else
      D7_PARSER_VAL_SSSN("STOP",aSTOP)
      else
      D7_PARSER_VAL_SSSN("STEP",aSTEP)
      else
      D7_PARSER_VAL_SSSN("NULL",aNULL)
      else
      if ( _rCmp ( aMNEM.p, "WELL" ) )
      {
        aWELL_W1 = aDATA;
        aWELL_W2 = aDESC;
        if ( _rCmp ( aDATA.p, "WELL" ) || aDATA.n == 0 )
        {
          aWELL.p = aDESC.p;
          aWELL.n = aDESC.n;
        }
        else
        {
          aWELL.p = aDATA.p;
          aWELL.n = aDATA.n;
        }
      }
      else
      if ( _rCmp ( aMNEM.p, "WN" ) )
      {
        aWN_W1 = aDATA;
        aWN_W2 = aDESC;
      }
      else
      if ( _rCmp ( aMNEM.p, "METD" ) )
      {
        if ( _rCmp ( aDATA.p, "METHOD" ) || aDATA.n == 0 )
        {
          aMETD.p = aDESC.p;
          aMETD.n = aDESC.n;
        }
        else
        {
          aMETD.p = aDATA.p;
          aMETD.n = aDATA.n;
        }
      }
    }
    else
    if ( iSection == 'O' )
    {
      if ( _rCmp ( aMNEM.p, "WELL" ) )
      {
        aWELL_O1 = aDATA;
        aWELL_O2 = aDESC;
      }
    }
    else
    if ( iSection == 'C' )
    {
      aMethodT.p = aMNEM.p;
      aMethodT.n = aMNEM.n;
      aMethodT.fA = __max ( aSTRT.v, aSTOP.v );
      aMethodT.fB = __min ( aSTRT.v, aSTOP.v );
      aMethods = rV7_Add ( aMethods, &aMethodT );
      rLogMethods ( L"%-16.*hs~%hc.%-16.*hs %-64.*hs:%-64.*hs %s\n", aMNEM.n, aMNEM.p, iSection, aUNIT.n, aUNIT.p, aDATA.n, aDATA.p, aDESC.n, aDESC.p, w7+1 );
    }


    goto P_SkipLine;

  P_Section_A:
  {
    while ( n && !( *p == '\n' || *p == '\r' ) ) { ++p; --n; }
    // const UINT kLines = fabs ( ( aSTRT.v - aSTOP.v ) / aSTEP.v );
    UINT nLinesReaded = 0;
    LPSTR pp;
    const UINT kN = rV7_GetSize ( aMethods );
    while ( TRUE )
    {
      while ( n && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) )
      {
        if ( *p == '\n' ) { ++nLine; }
         ++p; --n;
      }
      const double fDepth = strtod ( (LPCSTR)p, &pp );
      if ( (LONG_PTR)p == (LONG_PTR)pp ) { goto P_EndOfA; }
      if ( aMethods[0].fA >= fDepth ) { aMethods[0].fA = fDepth; }
      if ( aMethods[0].fB <= fDepth ) { aMethods[0].fB = fDepth; }
      p = (BYTE const *)(pp);
      while ( n && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) )
      {
        if ( *p == '\n' ) { ++nLine; }
         ++p; --n;
      }
      for ( UINT i = 1; i < kN; ++i )
      {
      // считываем значение
        const double f = strtod ( (LPCSTR)p, &pp );
        if ( (LONG_PTR)p == (LONG_PTR)pp ) { goto P_EndOfA; }
        // если он не близок к значению NULL, т.е. значение существует
        #define kAsciiDataErr 0.1
        if ( fabs ( f-aNULL.v ) > kAsciiDataErr )
        {
          // если верхняя граница ниже настоящего значения, то записываем и с другой границе также
          if ( aMethods[i].fA >= fDepth ) { aMethods[i].fA = fDepth; }
          if ( aMethods[i].fB <= fDepth ) { aMethods[i].fB = fDepth; }
        }
        p = (BYTE const *)(pp);
        while ( n && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) )
        {
          if ( *p == '\n' ) { ++nLine; }
           ++p; --n;
        }
      }
      ++nLinesReaded;
    }
    P_EndOfA:
    if ( fabs ( aMethods[0].fA - __min ( aSTRT.v, aSTOP.v ) ) > kAsciiDataErr )
    {
      sErr = L"Неточное значение DEPTH в начале секции ASCII\n";
      _rLog ( sErr );
      iState = 1; iErr = __LINE__;
    }
    if ( fabs ( aMethods[0].fB - __max ( aSTRT.v, aSTOP.v ) ) > kAsciiDataErr )
    {
      sErr = L"Неточное значение DEPTH в конце секции ASCII\n";
      _rLog ( sErr );
      iState = 1; iErr = __LINE__;
    }
  }
  P_End:
  {
    if ( iState )
    {
      static UINT k = 0;
      _rLog ( L"Ошибка в разборе файла, копирование файла в .../.ag47/error_las/\n" );
      WCHAR _w7[kPathMaxLen];
      // rW7_setf ( _w7, L"%s/.ag47/error_las/%s", gScript.w7PathOut+1, w7+(w7[0]-rW7_PathLastNameGetSize(w7)) );
      rW7_setf ( _w7, L"%s/.ag47/error_las/%05u.las", gScript.w7PathOut+1, k );
      ++k;
      FILE * const pf = _wfopen ( _w7+1, L"wb" );
      fprintf ( pf, "# Ag47_CodePage. %s\r\n", g_aszCyrillicTableLocaleNames[iCP] );
      fprintf ( pf, "# Ag47_Origin.   %ls\r\n", w7+1 );
      fprintf ( pf, "# Ag47_Error.    %u\r\n", iErr );
      fprintf ( pf, "# %ls\r\n", sErr );
      fwrite ( pBufBegin, 1, nBufSize, pf );
      fclose ( pf );
      ++(gScript.nFilesError);

      rV7_Free ( aMethods );
      return iErr;
    }
    else
    {
      aWELL.u = 0;
      // Попытка получить номер скважины из поля скважины
      for ( UINT i = 0; i < aWELL.n; ++i )
      {
        if ( ( aWELL.u = (UINT) atoi ( ((LPCSTR)(aWELL.p))+i ) ) ) { break; }
      }
      // Попытка получить номер скважины из других полей скважины
      #define D7_PARSER_GET_WELL(val) \
        if ( aWELL.u == 0 )\
        {\
          for ( UINT i = 0; i < val.n; ++i )\
          {\
            if ( ( aWELL.u = (UINT) atoi ( ((LPCSTR)(val.p))+i ) ) ) { break; }\
          }\
        }
      D7_PARSER_GET_WELL(aWELL_W1);
      D7_PARSER_GET_WELL(aWELL_W2);
      D7_PARSER_GET_WELL(aWELL_O1);
      D7_PARSER_GET_WELL(aWELL_O2);
      D7_PARSER_GET_WELL(aWN_W1);
      D7_PARSER_GET_WELL(aWN_W2);
      #undef D7_PARSER_GET_WELL
      // Неудалось получить номер скважины
      if ( aWELL.u == 0 )
      {
        sErr = L"Не удалось получить значение номера скважины\n";
        _rLog ( sErr );
        iState = 2; iErr = __LINE__;
        goto P_End;
      }

      {
        const UINT kN = rV7_GetSize ( aMethods );
        rLogTable ( L"% 6u\t%f\t%f\t%f\t%f\t%-8.*hs|\t",
                aWELL.u,
                aSTRT.v, aSTOP.v, aSTEP.v, aNULL.v,
                aMETD.n, aMETD.p );

        for ( UINT i = 0; i < kN; ++i )
        {
          rLogTable ( L"%-8.*hs\t%f\t%f|\t",
                aMethods[i].n, aMethods[i].p, aMethods[i].fA, aMethods[i].fB );
        }
        rLogTable ( L"%s\n", w7+1 );
      }


      WCHAR _w7_[kPathMaxLen];
      for ( UINT i = 0; i <= 999; ++i )
      {
        rW7_setf ( _w7_, L"%s/.ag47/temp_las/%u_%s_[%03u].LAS", gScript.w7PathOut+1, aWELL.u, w7+(w7[0]-rW7_PathLastNameGetSize(w7))+1, i );
        FILE * const pf = _wfopen ( _w7_+1, L"rb" );
        if ( pf ) { fclose ( pf ); } else { break; }
      }
      FILE * const pf = _wfopen ( _w7_+1, L"wb" );
      if ( pf )
      {
        fprintf ( pf, "# Ag47_CodePage. %s\r\n", g_aszCyrillicTableLocaleNames[iCP] );
        fprintf ( pf, "# Ag47_Origin.   %ls\r\n", w7+1 );
        fprintf ( pf, "# Ag47_ID.       L%08" PRIx32 "%016" PRIx64 "%08x\r\n",
              rCRC32 ( pBufBegin, nBufSize ), rAg47cs ( pBufBegin, nBufSize ), nBufSize );
        const UINT kN = rV7_GetSize ( aMethods );
        fprintf ( pf, "# Ag47_MtdCount. %u\r\n", kN );
        for ( UINT i = 0; i < kN; ++i )
        {
          fprintf ( pf, "# Ag47_Method. \"%.*s\" %f %f\r\n", aMethods[i].n, aMethods[i].p, aMethods[i].fA, aMethods[i].fB );
        }
        fwrite ( pBufBegin, 1, nBufSize, pf );
        fclose ( pf );
      }
      else
      {
        _rLog ( L"Невозможно открыть файл для записи\n" );
        _rLog ( _w7_+1 );
        iErr = __LINE__;
      }

      rV7_Free ( aMethods );
      return iErr;
    }
  }
}
static UINT rParseIncl_Prepare ( const LPCWSTR w7, BYTE * p, UINT n )
{
  {
    // Создаём временную копию
    WCHAR _w7_[kPathMaxLen];
    rW7_setf ( _w7_, L"%s/.ag47/temp_inkl/%s", gScript.w7PathOut+1, w7+(w7[0]-rW7_PathLastNameGetSize(w7))+1 );
    {
      FILE * pf = _wfopen ( _w7_+1, L"rb" );
      if ( pf )
      {
        fclose ( pf );
        for ( UINT i = 0; i < 999; ++i )
        {
          rW7_setf ( _w7_, L"%s/.ag47/temp_inkl/%u%s", gScript.w7PathOut+1, i, w7+(w7[0]-rW7_PathLastNameGetSize(w7))+1 );
          pf = _wfopen ( _w7_+1, L"rb" );
          if ( pf ) { fclose ( pf ); } else { break; }
        }
      }
    }
    {
      FILE * const pf = _wfopen ( _w7_+1, L"wb" );
      if ( !pf )
      {
        rLog_Error ( L"INKL: Невозможно открыть файл для записи \"%s\"\n", _w7_+1 );
        return __LINE__;
      }
      fwrite ( p, 1, n, pf );
      fclose ( pf );
    }
  }
  return 0;
}

static UINT rParseInkl_Xml ( xmlDocPtr doc, const LPCWSTR w7 )
{
  WCHAR __w7[kPathMaxLen];
  rW7_setf ( __w7, L"%s.xml", w7+1 );
  FILE * const fp = rOpenFileToWriteWith_UTF16_BOM ( __w7+1 );
  UINT iErr = 0;
  fwprintf ( fp, L"Encode = %hs\n", doc->encoding );
  fwprintf ( fp, L"Origin = %s\n", w7+1 );
  xmlNodePtr root_element = xmlDocGetRootElement ( doc );
  UINT d = 0;

  BOOL bTitled = FALSE;
  BOOL bWell = FALSE;
  UINT iWell = 0;
  BOOL bIn = FALSE;
  double fIn = 0;
  BOOL bAlt = FALSE;
  double fAlt = 0;
  BOOL bDataT = FALSE;
  BOOL bDataTAn = FALSE;
  BOOL bDataTAz = FALSE;
  BOOL bDataTTAn;
  BOOL bDataTTAz;

  VOID _rPrintTab ( )
  {
    fwprintf ( fp, L"%.*s", d, L"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" );
  }
  LPCWSTR _rNodeType ( const xmlElementType i )
  {
    switch ( i )
    {
      case XML_ELEMENT_NODE:
        return L"ELEMENT_NODE";
      case XML_ATTRIBUTE_NODE:
        return L"ATTRIBUTE_NODE";
      case XML_TEXT_NODE:
        return L"TEXT_NODE";
      case XML_CDATA_SECTION_NODE:
        return L"CDATA_SECTION_NODE";
      case XML_ENTITY_REF_NODE:
        return L"ENTITY_REF_NODE";
      case XML_ENTITY_NODE:
        return L"ENTITY_NODE";
      case XML_PI_NODE:
        return L"PI_NODE";
      case XML_COMMENT_NODE:
        return L"COMMENT_NODE";
      case XML_DOCUMENT_NODE:
        return L"DOCUMENT_NODE";
      case XML_DOCUMENT_TYPE_NODE:
        return L"DOCUMENT_TYPE_NODE";
      case XML_DOCUMENT_FRAG_NODE:
        return L"DOCUMENT_FRAG_NODE";
      case XML_NOTATION_NODE:
        return L"NOTATION_NODE";
      case XML_HTML_DOCUMENT_NODE:
        return L"HTML_DOCUMENT_NODE";
      case XML_DTD_NODE:
        return L"DTD_NODE";
      case XML_ELEMENT_DECL:
        return L"ELEMENT_DECL";
      case XML_ATTRIBUTE_DECL:
        return L"ATTRIBUTE_DECL";
      case XML_ENTITY_DECL:
        return L"ENTITY_DECL";
      case XML_NAMESPACE_DECL:
        return L"NAMESPACE_DECL";
      case XML_XINCLUDE_START:
        return L"XINCLUDE_START";
      case XML_XINCLUDE_END:
        return L"XINCLUDE_END";
      case XML_DOCB_DOCUMENT_NODE:
        return L"DOCB_DOCUMENT_NODE";
      default:
        return L"UNKNOWN";
    }
  }

  VOID _rPrint_e ( xmlNodePtr a_node )
  {
    for ( xmlNodePtr cur_node = a_node; cur_node; cur_node = cur_node->next )
    {
      if ( cur_node->type == XML_ELEMENT_NODE )
      {
        _rPrintTab();
        fwprintf ( fp, L"<%hs", cur_node->name );
        for ( xmlAttrPtr attr = cur_node->properties; attr; attr = attr->next )
        {
          fwprintf ( fp, L" %hs", attr->name );
          if ( attr->children )
          {
            WCHAR w[kPathMaxLen];
            rUTF8_ToWide ( XML_GET_CONTENT ( attr->children ), xmlStrlen( XML_GET_CONTENT ( attr->children ) ), w );
            fwprintf ( fp, L"=\"%s\"", w );
          }
        }
        fwprintf ( fp, L">\n" );
        // Нашли параграф
        if ( strcmp ( cur_node->name, "p" ) == 0 )
        {
          xmlChar * p = xmlNodeGetContent  ( cur_node );
          WCHAR w[kPathMaxLen];
          const UINT i = rUTF8_ToWide ( p, xmlStrlen( p ), w );
          xmlFree ( p );
          _rPrintTab();
          fwprintf ( fp, L"<!-- %s -->\n", w );
          // Если это уже файл с инклинометрией, ищем необходимые данные
          if ( bTitled )
          {
            for ( UINT i=0; w[i]; ++i )
            {
              if ( !bWell && rWideCmpWords ( L"Скважина N", w+i ) )
              {
                i+=10;
                iWell = _wtoi ( w+i );
                bWell = TRUE;
                rLog ( L"%-16s [%u] >>>%-64s %s\n", L"Скважина", iWell, w, w7+1 );
              }
              if ( !bAlt && rWideCmpWords ( L"Альтитуда:", w+i ) )
              {
                i+=10;
                fAlt = _wtof ( w+i );
                bAlt = TRUE;
                rLog ( L"%-16s [%f] >>>%-64s %s\n\n", L"Альтитуда", fAlt, w, w7+1 );
              }
              if ( !bIn && rWideCmpWords ( L"Угол склонения:", w+i ) )
              {
                i+=15;
                LPWSTR pp;
                LONG v1 = wcstol ( w+i, &pp, 10 );
                if ( pp != w+i )
                {
                  LPWSTR pp2;
                  ULONG v2 = wcstoul ( pp+1, &pp2, 10 );
                  if ( pp != pp2 )
                  {
                    fIn = v2;
                    UINT sv2 = ((UINT_PTR)(pp2)-(UINT_PTR)(pp+1))/2;
                    for ( UINT k = 0; k < sv2; ++k ) { fIn /= 10.0; }
                    pp = pp2;
                    for ( UINT k=i; w[k]; ++k )
                    {
                      if ( rWideCmpWords ( L"град", w+k ) || rWideCmpWords ( L"градусы", w+k ) )
                      {
                        k += 4;
                        for (; w[k]; ++k )
                        {
                          if ( rWideCmpWords ( L"град", w+k ) || rWideCmpWords ( L"градусы", w+k ) )
                          {
                            break;
                          }
                          else
                          if ( rWideCmpWords ( L"минуты", w+k ) || rWideCmpWords ( L"мин", w+k ) )
                          {
                            fIn *= 100.0/60.0;
                            break;
                          }
                        }
                        break;
                      }
                    }
                    // TODO определить значение после запятой
                    fIn += labs(v1);
                    if ( v1 < 0 ) { fIn = -fIn; }
                    bIn = TRUE;
                    rLog ( L"%-16s [%f] >>>%-64s %s\n", L"Угол склонения", fIn, w, w7+1 );
                  }
                }
              }
            }
            if ( !bDataT && rWideCmpWords ( L"Глубина", w ) )
            {
              bDataT = TRUE;
              rLog ( L"%-16s >>>%-64s %s\n", L"Глубина", w, w7+1 );
            }
            else
            if ( bDataT )
            {
              if ( !bDataTAn && rWideCmpWords ( L"Угол", w ) )
              {
                for ( UINT k=4; w[k]; ++k )
                {
                  if ( rWideCmpWords ( L"град", w+k ) || rWideCmpWords ( L"градусы", w+k ) )
                  {
                    k += 4;
                    for (; w[k]; ++k )
                    {
                      if ( rWideCmpWords ( L"град", w+k ) || rWideCmpWords ( L"градусы", w+k ) )
                      {
                        bDataTTAn = FALSE;
                        break;
                      }
                      else
                      if ( rWideCmpWords ( L"минуты", w+k ) || rWideCmpWords ( L"мин", w+k ) )
                      {
                        bDataTTAn = TRUE;
                        break;
                      }
                    }
                    break;
                  }
                }
                bDataTAn = TRUE;
                rLog ( L"%-16s [%s] >>>%-64s %s\n", L"Угол", bDataTTAn?L"минуты":L"градусы", w, w7+1 );
              }
              else
              if ( !bDataTAz && rWideCmpWords ( L"Азимут", w ) )
              {
                for ( UINT k=4; w[k]; ++k )
                {
                  if ( rWideCmpWords ( L"град", w+k ) || rWideCmpWords ( L"градусы", w+k ) )
                  {
                    k += 4;
                    for (; w[k]; ++k )
                    {
                      if ( rWideCmpWords ( L"град", w+k ) || rWideCmpWords ( L"градусы", w+k ) )
                      {
                        bDataTTAz = FALSE;
                        break;
                      }
                      else
                      if ( rWideCmpWords ( L"минуты", w+k ) || rWideCmpWords ( L"мин", w+k ) )
                      {
                        bDataTTAz = TRUE;
                        break;
                      }
                    }
                    break;
                  }
                }
                bDataTAz = TRUE;
                rLog ( L"%-16s [%s] >>>%-64s %s\n", L"Азимут", bDataTTAn?L"минуты":L"градусы", w, w7+1 );
              }

              if ( bDataTAn && bDataTAz )
              {
                // Ищем табличные значения
              }
            }
          }
          // Проверяем на возможность этого файла как файла с инклинометрией
          else
          if ( rWideCmpWords ( L"Инклинометрия", w ) )
          {
            bTitled = TRUE;
            rLog ( L"\n\n%-80s %s\n", w, w7+1 );
          }
        }

        ++d;
        _rPrint_e ( cur_node->children );
        --d;
        _rPrintTab();
        fwprintf ( fp, L"</%hs>\n", cur_node->name );
      }
      else
      if ( cur_node->type == XML_TEXT_NODE )
      {
        WCHAR w[kPathMaxLen];
        const UINT i = rUTF8_ToWide ( XML_GET_CONTENT ( cur_node ), xmlStrlen( XML_GET_CONTENT ( cur_node ) ), w );
        _rPrintTab();
        fwprintf ( fp, L"#text: (%u)%s\n", i, w );
        // rLog ( L"#text: (%u)%s\n", i, w );
      }
      else
      {
        _rPrintTab();
        fwprintf ( fp, L"<%hs>", cur_node->name );
        fwprintf ( fp, L"%-16s\n", _rNodeType(cur_node->type) );
        ++d;
        _rPrint_e ( cur_node->children );
        --d;
        _rPrintTab();
        fwprintf ( fp, L"</%hs>\n", cur_node->name );
      }
    }
  }
  _rPrint_e ( root_element );
  fclose ( fp );
  return iErr;
}

static UINT rParseInkl_Docx ( const LPCWSTR w7 )
{
  UINT iErr = 0;
  struct archive * const ar = archive_read_new();
  archive_read_support_filter_all ( ar );
  archive_read_support_format_all ( ar );
  FILE * const pf = _wfopen ( w7+1, L"rb" );
  assert ( pf );
  if ( archive_read_open_FILE ( ar, pf ) != ARCHIVE_OK )
  {
    rLog_Error ( L"archive_read_open_FILE (\"%s\")\n", w7+1 );
    iErr = __LINE__;
    goto P_Err;
  }
  else
  {
    struct archive_entry *are;
    while ( archive_read_next_header ( ar, &are ) == ARCHIVE_OK )
    {
      const UINT n = (UINT)archive_entry_size(are);
      LPCWSTR pName = archive_entry_pathname_w(are);
      LPCWSTR pName2 = L"word/document.xml";
      BOOL b = TRUE;
      while ( b )
      {
        if ( iswalpha ( *pName2 ) && iswalpha ( *pName ) )
        {
          b &= ((*pName2)&0x1f) == ((*pName)&0x1f);
        }
        else
        if ( ((*pName2) == '/') || ((*pName2) == '\\') )
        {
          b &= ((*pName) == '/' || (*pName) == '\\');
        }
        else
        {
          b &= (*pName) == (*pName2);
          if ( (*pName2) == 0 )
          {
            BYTE buf[n+256];
            la_ssize_t aer = archive_read_data ( ar, buf, n+256 );
            if ( aer < 0 ) { aer = archive_read_data ( ar, buf, n+256 ); }
            if ( aer != n ) { rLog_Error ( L"archive_read_data [%d/%u] (\"%s\")\n", aer, n, w7+1 ); }
            xmlDocPtr doc = xmlReadMemory ( (LPCSTR)buf, aer, "document.xml", NULL, 0 );
            if ( doc == NULL )
            {
              rLog_Error ( L"Ошибка в парсинге xml файла в документе word\n" );
              iErr = __LINE__;
              goto P_Err;
            }
            else
            {
              iErr = rParseInkl_Xml ( doc, w7 );
            }
            xmlFreeDoc ( doc );
            goto P_Err;
          }
        }
        ++pName; ++pName2;
      }
      archive_read_data_skip ( ar );
    }
  }
  P_Err:
  if ( archive_read_free ( ar ) != ARCHIVE_OK )
  {
    rLog_Error ( L"archive_read_free (\"%s\")\n", w7+1 );
  }
  fclose ( pf );
  return iErr;
}

static UINT rParseIncl_All ( const LPWSTR w7 )
{
  const UINT l = w7[0];
  UINT iErr = 0;
  rW7_add ( w7, L"/*" );
  WIN32_FIND_DATA ffd;
  const HANDLE hFind = FindFirstFile ( w7+1, &ffd );
  if ( hFind == INVALID_HANDLE_VALUE )
  {
    rLog_Error_WinAPI ( L"FindFirstFile", GetLastError(), L"(\"%s\")\n", w7+1 );
    return __LINE__;
  }
  w7[0] = l; w7[w7[0]+1] = 0;

  do
  {
    rW7_addf ( w7, L"/%s", ffd.cFileName );

    if ( rW7_PathWithEndOf_W7 ( w7, L"\x05.docx" ) )
    {
      if ( ffd.cFileName[0] != '~' && ( iErr = rParseInkl_Docx ( w7 ) ) ) { goto P_Err; }
    }

    w7[0] = l; w7[w7[0]+1] = 0;
  } while ( FindNextFile ( hFind, &ffd ) );
  P_Err:
  w7[0] = l; w7[w7[0]+1] = 0;
  FindClose ( hFind );
  return iErr;
}



static UINT rScriptRun_Parse_AR ( const LPWSTR w7, struct archive * const ar )
{
  struct archive_entry *are;
  while ( archive_read_next_header ( ar, &are ) == ARCHIVE_OK )
  {
    const UINT n = (UINT)archive_entry_size(are);
    const UINT l = w7[0];
    rW7_addf ( w7, L"/%s", archive_entry_pathname_w(are) );

    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixAr ) )
    {
      BYTE buf[n+256];
      la_ssize_t aer = archive_read_data ( ar, buf, n+256 );
      if ( aer < 0 ) { aer = archive_read_data ( ar, buf, n+256 ); }
      if ( aer != n ) { rLog_Error ( L"archive_read_data [%d/%u] (\"%s\")\n", aer, n, w7+1 ); }
      struct archive * const ar2 = archive_read_new();
      archive_read_support_filter_all ( ar2 );
      archive_read_support_format_all ( ar2 );
      UINT er = 0;
      if ( archive_read_open_memory ( ar2, buf, n ) != ARCHIVE_OK )
      {
        rLog_Error ( L"archive_read_open_memory (\"%s\")\n", w7+1 );
        er = __LINE__;
        goto P_Err;
      }
      else
      {
        er = rScriptRun_Parse_AR ( w7, ar2 );
        if ( er ) { goto P_Err; }
      }
      P_Err:
      if ( archive_read_free ( ar2 ) != ARCHIVE_OK )
      {
        rLog_Error ( L"archive_read_free (\"%s\")\n", w7+1 );
      }
      if ( er ) { w7[0] = l; w7[w7[0]+1] = 0; return er; }
    }
    else
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixLas ) )
    {
      BYTE buf[n+256];
      la_ssize_t aer = archive_read_data ( ar, buf, n+256 );
      if ( aer < 0 ) { aer = archive_read_data ( ar, buf, n+256 ); }
      if ( aer != n ) { rLog_Error ( L"archive_read_data [%d/%u] (\"%s\")\n", aer, n, w7+1 ); }
      rParseLas ( w7, buf, n );
      --gScript.nFilesLas;
      ++gScript.nFilesParsed;
      if ( gScript.nFilesParsed % kFilesParsedPrint == 0 )
      {
        D7_printf ( "Обработано файлов: %u\n", gScript.nFilesParsed );
        D7_printf ( "Осталось обработать [LAS:%u] [INCL:%u]\n", gScript.nFilesLas, gScript.nFilesIncl );
      }
    }
    else
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixIncl ) )
    {
      BYTE buf[n+256];
      la_ssize_t aer = archive_read_data ( ar, buf, n+256 );
      if ( aer < 0 ) { aer = archive_read_data ( ar, buf, n+256 ); }
      if ( aer != n ) { rLog_Error ( L"archive_read_data [%d/%u] (\"%s\")\n", aer, n, w7+1 ); }
      rParseIncl_Prepare ( w7, buf, n );
      --gScript.nFilesIncl;
      ++gScript.nFilesParsed;
      if ( gScript.nFilesParsed % kFilesParsedPrint == 0 )
      {
        D7_printf ( "Обработано файлов: %u\n", gScript.nFilesParsed );
        D7_printf ( "Осталось обработать [LAS:%u] [INCL:%u]\n", gScript.nFilesLas, gScript.nFilesIncl );
      }
    }
    else
    {
      archive_read_data_skip ( ar );
    }

    w7[0] = l; w7[w7[0]+1] = 0;
  }
  return 0;
}

static UINT rScriptRun_Parse_FFD ( const LPWSTR w7, WIN32_FIND_DATA * const _ffd )
{
  const UINT l = w7[0];
  rW7_addf ( w7, L"/%s", _ffd->cFileName );
  if ( _ffd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
  {
    if ( _ffd->cFileName[0] == '.' )
    {
      if ( _ffd->cFileName[1] == '\0' )  { w7[0] = l; w7[w7[0]+1] = 0; return 0; } else
      if ( _ffd->cFileName[1] == '.' )
      { if ( _ffd->cFileName[2] == '\0' ) { w7[0] = l; w7[w7[0]+1] = 0; return 0; } }
    }
    // Продолжаем поиск внутри папки
    WIN32_FIND_DATA ffd;
    const UINT _l = w7[0];
    rW7_addf ( w7, L"/*" );
    const HANDLE hFind = FindFirstFile ( w7+1, &ffd );
    w7[0] = _l; w7[w7[0]+1] = 0;
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog_Error_WinAPI ( L"FindFirstFile", GetLastError(), L"(\"%s\")\n", w7+1 );
      w7[0] = l; w7[w7[0]+1] = 0;
      return __LINE__;
    }

    do
    {
      const UINT k = rScriptRun_Parse_FFD ( w7, &ffd );
      if ( k ) { w7[0] = l; w7[w7[0]+1] = 0; FindClose ( hFind ); return k; }
    } while ( FindNextFile ( hFind, &ffd ) );
    FindClose ( hFind );
  }
  else
  {
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixAr ) )
    {
      struct archive * const ar = archive_read_new();
      archive_read_support_filter_all ( ar );
      archive_read_support_format_all ( ar );
      UINT er = 0;
      FILE * const pf = _wfopen ( w7+1, L"rb" );
      assert ( pf );
      if ( archive_read_open_FILE ( ar, pf ) != ARCHIVE_OK )
      {
        rLog_Error ( L"archive_read_open_FILE (\"%s\")\n", w7+1 );
        er = __LINE__;
        goto P_Err;
      }
      else
      {
        er = rScriptRun_Parse_AR ( w7, ar );
        if ( er ) { goto P_Err; }
      }
      P_Err:
      if ( archive_read_free ( ar ) != ARCHIVE_OK )
      {
        rLog_Error ( L"archive_read_free (\"%s\")\n", w7+1 );
      }
      fclose ( pf );
      if ( er ) { w7[0] = l; w7[w7[0]+1] = 0; return er; }
    }
    else
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixLas ) )
    {
      const UINT n = _ffd->nFileSizeLow;
      BYTE buf[n+1];
      rLoadFile ( buf, w7+1, n );
      rParseLas ( w7, buf, n );
      --gScript.nFilesLas;
      ++gScript.nFilesParsed;
      if ( gScript.nFilesParsed % kFilesParsedPrint == 0 )
      {
        D7_printf ( "Обработано файлов: %u\n", gScript.nFilesParsed );
        D7_printf ( "Осталось обработать [LAS:%u] [INCL:%u]\n", gScript.nFilesLas, gScript.nFilesIncl );
      }
    }
    else
    if ( rW7_PathWithEndOf_VW7 ( w7, gScript.vw7PostfixIncl ) )
    {
      const UINT n = _ffd->nFileSizeLow;
      BYTE buf[n+1];
      rLoadFile ( buf, w7+1, n );
      rParseIncl_Prepare ( w7, buf, n );
      --gScript.nFilesIncl;
      ++gScript.nFilesParsed;
      if ( gScript.nFilesParsed % kFilesParsedPrint == 0 )
      {
        D7_printf ( "Обработано файлов: %u\n", gScript.nFilesParsed );
        D7_printf ( "Осталось обработать [LAS:%u] [INCL:%u]\n", gScript.nFilesLas, gScript.nFilesIncl );
      }
    }
  }

  w7[0] = l; w7[w7[0]+1] = 0;
  return 0;
}

static UINT rScriptRun_Parse ( )
{
  gScript.nFilesError = 0;
  rLog ( L"~ script RUN_PARSE\n" );
  if ( gScript.iState < kSSR_Parse-1 ) { const UINT n = rScriptRun_Tree(); if ( n ) { return n; } }
  D7_printf ( "Обработка и анализ файлов\n" );
  gScript.nFilesParsed = 0;
  const UINT n = rV7_GetSize ( gScript.vw7PathIn );
  for ( UINT i = 0; i < n; ++i )
  {
    WCHAR w7[kPathMaxLen];
    WIN32_FIND_DATA ffd;
    rW7_setf ( w7, L"%s/*", gScript.vw7PathIn[i]+1 );
    const HANDLE hFind = FindFirstFile ( w7+1, &ffd );
    w7[0] = gScript.vw7PathIn[i][0];
    w7[w7[0]+1] = 0;
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog_Error_WinAPI ( L"FindFirstFile", GetLastError(), L"(\"%s\")\n", w7+1 );
      return __LINE__;
    }
    do
    {
      const UINT k = rScriptRun_Parse_FFD ( w7, &ffd );
      if ( k ) { FindClose ( hFind ); return k; }
    } while ( FindNextFile ( hFind, &ffd ) );
    FindClose ( hFind );
  }
  D7_printf ( "Обработано файлов: %u\n", gScript.nFilesParsed );
  D7_printf ( "Осталось обработать [LAS:%u] [INCL:%u]\n", gScript.nFilesLas, gScript.nFilesIncl );
  D7_printf ( "Файлов с ошибками %u\n", gScript.nFilesError );

  WCHAR _w7_[kPathMaxLen];
  rW7_setf ( _w7_, L"%s/.ag47/temp_inkl/", gScript.w7PathOut+1 );
  rScriptRun_DocToDocx ( _w7_+1 );
  rParseIncl_All ( _w7_ );

  gScript.iState = kSSR_Parse;
  return 0;
}



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
  gScript.iState = kSSR_Null;
}



static UINT rScriptParse ( LPWSTR p )
{
  rScriptInit();
  UINT nLine = 1;
  UINT nError;

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

  VOID _rSkipWs ( )
  {
    while ( TRUE )
    {
      while ( isspace ( *p ) )
      {
        if ( *p == '\n' ) { ++nLine; }
        ++p;
      }
      if ( *p == '/' )
      {
        if ( p[1] == '/' ) { p += 2; _rSkipToNewLine(); continue; }
        if ( p[1] == '*' ) { p += 2; _rSkipComment(); continue; }
      }
      return;
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
    gScript.iState = kSSR_Null;
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
          *val = rV7_Add_W7 ( *val, w7 );
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
    gScript.iState = kSSR_Null;
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

    if ( ( nTemp = _rCmp ( "RECREATE" ) ) ) { p += nTemp; gScript.iState = kSSR_Null; if ( ( nTemp = _rValNull ( ) ) ) { return nTemp; } gScript.bReCreate = TRUE; return 0; }
    if ( ( nTemp = _rCmp ( "NORECREATE" ) ) ) { p += nTemp; gScript.iState = kSSR_Null; if ( ( nTemp = _rValNull ( ) ) ) { return nTemp; } gScript.bReCreate = FALSE; return 0; }

    if ( ( nTemp = _rCmp ( "RUN_TREE" ) ) ) { p += nTemp; if ( ( nTemp = _rValNull ( ) ) ) { return nTemp; } if ( ( nTemp = rScriptRun_Tree ( ) ) ) { return nTemp; } return 0; }
    if ( ( nTemp = _rCmp ( "RUN_PARSE" ) ) ) { p += nTemp; if ( ( nTemp = _rValNull ( ) ) ) { return nTemp; } if ( ( nTemp = rScriptRun_Parse ( ) ) ) { return nTemp; } return 0; }

    return __LINE__;
  }

  {
    while ( TRUE )
    {
      _rSkipWs();
      switch ( *p )
      {
        case 'A' ... 'Z':
        case 'a' ... 'z':
          if ( ( nError = _rValName() ) ) { goto P_Error; }
          continue;
        case 0:
          goto P_Ok;
        default:
          nError = __LINE__; goto P_Error;
      }
    }
  }
  P_Error:
    rLog_Error ( L"[%u] Ошибка (Line in script %u)\n", nError, nLine );
    return nError;


  P_Ok:
    rScriptRelease ( );
    return 0;
}

static UINT rScriptOpen ( const LPCWSTR wszFilePath )
{
  rLog ( L"Скрипт (\"%s\")\n", wszFilePath );
  D7_printf ( "Скрипт (\"%ls\")\n", wszFilePath );
  WIN32_FIND_DATA ffd;
  {
    const HANDLE hFind  = FindFirstFile ( wszFilePath, &ffd );
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog_Error_WinAPI ( L"FindFirstFile", GetLastError(), L"(\"%s\")\n", wszFilePath );
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
  rLog_Error ( L"неизвестная кодировка файла скрипта (\"%s\")\n", wszFilePath );
  return __LINE__;
}









































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
    const HANDLE hFind = FindFirstFile ( w7Path+1, &ffd );
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
  const HANDLE hFind = FindFirstFile ( w7Path+1, &ffd );
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
  UINT iErr = 0;
  LIBXML_TEST_VERSION

  rLog ( L"ptr UINT[-1] = %p\n", ((UINT*)(&iErr))-1 );
  rLog ( L"ptr UINT[+0] = %p\n", ((UINT*)(&iErr)) );
  rLog ( L"ptr UINT[+1] = %p\n", ((UINT*)(&iErr))+1 );
  LPVOID pE = &iErr;
  rLog ( L"ptr VOID[-1] = %p\n", ((VOID*)pE)-1 );
  rLog ( L"ptr VOID[+0] = %p\n", ((VOID*)pE) );
  rLog ( L"ptr VOID[+1] = %p\n", ((VOID*)pE)+1 );

  WCHAR w7[kPathMaxLen];
  rW7_setf ( w7, L"./.ag47/.ag47/temp_inkl/" );
  iErr = rParseIncl_All ( w7 );


  xmlCleanupParser();

  return iErr;

  //for %A in (F:/ARGilyazeev/github/Ag47/.ag47/.ag47/temp_inkl/*.doc) do (echo "%~nA" )
  return 0;
  if ( argc == 1 )
  {
    if ( ( iErr = rScriptOpen ( L".ag47-script" ) ) ) goto P_End;
  }
  P_End:
    rLog ( NULL );
    rLogTree ( NULL );
    rLogSection ( NULL );
    rLogMethods ( NULL );
    rLogTable ( NULL );
    xmlCleanupParser();
    return iErr;




  // D7_printf ( "setlocale: %s\n", setlocale ( LC_ALL, "" ) );
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

  // D7_printf ( "%d\n", atoi ( "   1312093saodakskdo") );

  // fclose ( pF_O );
  // fclose ( pF_M );
  // fclose ( pF_S );
  // fclose ( pF );
  return 0;
}
