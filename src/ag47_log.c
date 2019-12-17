static UINT rLog_v ( const LPCWSTR fmt, va_list args )
{
  static FILE * pF = NULL;
  if ( !fmt )
  {
    if ( pF ) { fclose ( pF ); pF = NULL; return 0; }
    return 0;
  }
  if ( !pF ) { pF = rOpenFileToWriteWith_UTF16_BOM ( L".ag47.log" ); }
  return vfwprintf ( pF, fmt, args );
}

static UINT rLog ( const LPCWSTR fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  UINT i = rLog_v ( fmt, args );
  va_end ( args );
  return i;
}

static UINT rLog_Error_v ( const LPCSTR szFile,  const UINT nLine, const LPCWSTR fmt, va_list args )
{
  rLog ( L"!ERROR: %hs (%d)\r\n", szFile, nLine );
  return rLog_v ( fmt, args );
}

static UINT rLog_Error_ ( const LPCSTR szFile,  const UINT nLine, const LPCWSTR fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  UINT i = rLog_Error_v ( szFile, nLine, fmt, args );
  va_end ( args );
  return i;
}
#define rLog_Error(...) rLog_Error_(__FILE__,__LINE__,__VA_ARGS__)

static VOID rLog_Error_WinAPI_ ( const LPCSTR szFile,  const UINT nLine, const LPCSTR prFunc, const DWORD iErr, const LPCWSTR fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  rLog_Error_ ( szFile, nLine, fmt, args );
  va_end ( args );
  WCHAR buf[1024];
  FormatMessage ( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
          iErr, MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT), buf, 1024, NULL );
  rLog ( L"\t==> %hs (%d)\r\n\t==> %s", prFunc, iErr, buf );
}
#define rLog_Error_WinAPI(prFunc,iErr,...) rLog_Error_WinAPI_(__FILE__,__LINE__,#prFunc,iErr,__VA_ARGS__)

