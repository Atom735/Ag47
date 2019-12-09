static UINT rFS_Run ( const LPWSTR cmd, LPPROCESS_INFORMATION pi )
{
  STARTUPINFO s =
  {
    .cb = sizeof(STARTUPINFO),
    .dwFlags = STARTF_USESTDHANDLES,
    .hStdInput = GetStdHandle ( STD_INPUT_HANDLE ),
    .hStdOutput = GetStdHandle ( STD_OUTPUT_HANDLE ),
    .hStdError = GetStdHandle ( STD_ERROR_HANDLE ),
  };
  return CreateProcess ( NULL, cmd, NULL , NULL, TRUE,
          CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,
          NULL, NULL, &s, pi );
}

static UINT rFS_Run_Wait ( const LPWSTR cmd )
{
  PROCESS_INFORMATION pi;
  const UINT i = rFS_Run ( cmd, &pi );
  // Wait until child process exits.
  WaitForSingleObject( pi.hProcess, INFINITE );
  // Close process and thread handles.
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );
  return i;
}


static LPWSTR g_s4wPathToWordConv = NULL;
static LPWSTR g_s4wPathTo7Zip = NULL;


static UINT rFS_Run_7Zip ( const LPCWSTR wszIn, const LPCWSTR wszOut )
{
  const LPWSTR cmd = r4_alloca_s4w ( kPathMax*4 );
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  r4_push_path_s4w_s4w ( cmd, g_s4wPathTo7Zip );
  r4_push_array_s4w_sz ( cmd, L"\" x \"-o", 8 );
  r4_push_path_s4w_s4w ( cmd, wszOut );
  r4_push_array_s4w_sz ( cmd, L"\" \"", 4 );
  r4_push_path_s4w_s4w ( cmd, wszIn );
  r4_push_array_s4w_sz ( cmd, L"\" > \"", 6 );
  r4_push_path_s4w_s4w ( cmd, s4wPathOutLogs_7Zip );
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  return rFS_Run_Wait ( cmd );





  r4_push_array_s4w_sz ( cmd, g_s4wPathTo7Zip, r4_get_count_s4w(g_s4wPathTo7Zip)+1 );
  r4_push_array_s4w_sz ( cmd, L"\" x \"-o", 8 );
  if ( wcsncmp ( wszOut, L"\\\\?\\UNC\\", 8 ) == 0 )
  {
    r4_push_array_s4w_sz ( cmd, L"\\", 2 );
    r4_push_array_s4w_sz ( cmd, wszOut+7, 0 );
  }
  else
  {
    r4_push_array_s4w_sz ( cmd, wszOut, 0 );
  }
  r4_push_array_s4w_sz ( cmd, L"\" \"", 4 );
  if ( wcsncmp ( wszIn, L"\\\\?\\UNC\\", 8 ) == 0 )
  {
    r4_push_array_s4w_sz ( cmd, L"\\", 2 );
    r4_push_array_s4w_sz ( cmd, wszIn+7, 0 );
  }
  else
  {
    r4_push_array_s4w_sz ( cmd, wszIn, 0 );
  }
  r4_push_array_s4w_sz ( cmd, L"\" > \"", 6 );
  return rFS_Run_Wait ( cmd );
}

static UINT rFS_Run_WordConv ( const LPCWSTR wszIn, const LPCWSTR wszOut )
{
  const LPWSTR cmd = r4_alloca_s4w ( kPathMax*4 );
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  r4_push_array_s4w_sz ( cmd, g_s4wPathToWordConv, r4_get_count_s4w(g_s4wPathToWordConv)+1 );
  r4_push_array_s4w_sz ( cmd, L"\" -oice -nme \"", 15 );
  if ( wcsncmp ( wszIn, L"\\\\?\\UNC\\", 8 ) == 0 )
  {
    r4_push_array_s4w_sz ( cmd, L"\\", 2 );
    r4_push_array_s4w_sz ( cmd, wszIn+7, 0 );
  }
  else
  {
    r4_push_array_s4w_sz ( cmd, wszIn, 0 );
  }
  r4_push_array_s4w_sz ( cmd, L"\" \"", 4 );

  if ( wcsncmp ( wszOut, L"\\\\?\\UNC\\", 8 ) == 0 )
  {
    r4_push_array_s4w_sz ( cmd, L"\\", 2 );
    r4_push_array_s4w_sz ( cmd, wszOut+7, 0 );
  }
  else
  {
    r4_push_array_s4w_sz ( cmd, wszOut, 0 );
  }
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  return rFS_Run_Wait ( cmd );
}


/*
  Устанавливает вектор s4w в текущую дерикторию
*/
static UINT rFS_GetCurrentDirectory_s4w ( const LPWSTR s4w )
{
    const UINT u = GetCurrentDirectory ( r4_get_memsz_s4w ( s4w ) - r4_get_count_s4w ( s4w ), s4w + r4_get_count_s4w ( s4w ) );
    if ( u == 0 )
    {
      rLog_Error_WinAPI ( GetCurrentDirectory, GetLastError(), s4w );
      return 0;
    }
    r4_get_count_s4w ( s4w ) += u;
    return u;
}

/*
  Создать папку и добавить её к пути
  Возвращает предыдущее значение длины пути
*/
static UINT rFS_AddDir ( const LPWSTR s4w, const LPCWSTR wsz, const UINT k )
{
  const UINT n = r4_push_array_s4w_sz ( s4w, wsz, k );
  if ( ! CreateDirectory ( s4w, NULL ) )
  {
    const UINT i = GetLastError();
    if ( i != ERROR_ALREADY_EXISTS )
    {
      rLog_Error_WinAPI ( CreateDirectory, GetLastError(), L"%s\n", s4w );
    }
  }
  return n;
}

/*
  Поиск файлов в папке
  s4wPath               => путь к дериктории где производится поиск,
                        в конце будет приписано окончание \* для поиска всех файлов
  bArchives             => искать ли в архивах
  prFileProc            => Функция обработки найденных файлов
  prFolderProc          => Функция обработки найденных папок
*/
static UINT rFS_Tree ( const LPWSTR s4wPath,
        UINT (*prFileProc)( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize ), UINT (*prFolderProc)( const LPWSTR s4wPath,
        const LPCWSTR wszFolderName ) )
{
  WIN32_FIND_DATA ffd;
  UINT iErr = 0;
  const UINT n1 = r4_push_array_s4w_sz ( s4wPath, L"\\*", 3 );
  const HANDLE hFind  = FindFirstFile ( s4wPath, &ffd );
  if ( hFind == INVALID_HANDLE_VALUE )
  {
    rLog_Error_WinAPI ( FindFirstFile, GetLastError(), s4wPath );
    r4_cut_end_s4w ( s4wPath, n1 );
    return __LINE__;
  }
  r4_cut_end_s4w ( s4wPath, n1 );
  do
  {
    if ( wcscmp ( ffd.cFileName, L"." ) == 0 ) continue;
    if ( wcscmp ( ffd.cFileName, L".." ) == 0 ) continue;
    const UINT n2 = r4_push_array_s4w_sz ( s4wPath, L"\\", 2 );
    r4_push_array_s4w_sz ( s4wPath, ffd.cFileName, 0 );
    if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    { if ( prFolderProc ) { if ( ( iErr = prFolderProc ( s4wPath, ffd.cFileName ) ) ) goto P_Err; } }
    else
    { if ( prFileProc ) { if ( ( iErr = prFileProc ( s4wPath, ffd.cFileName, ffd.nFileSizeLow ) ) ) goto P_Err; } }
    r4_cut_end_s4w ( s4wPath, n2 );
  } while ( FindNextFile ( hFind, &ffd ) );
  P_Err:
  FindClose ( hFind );
  return iErr;
}



static UINT rFS_SearchExe ( )
{
  WIN32_FIND_DATA ffd;
  {
    const LPCWSTR wsz = L"C:/Program Files (x86)/Microsoft Office/Office*";
    const HANDLE hFind = FindFirstFile ( wsz, &ffd );
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog_Error_WinAPI ( FindFirstFile, GetLastError(), L"%s\n", wsz );
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
      rLog_Error_WinAPI ( FindFirstFile, GetLastError(), L"%s\n", g_s4wPathTo7Zip );
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
/*
  Создаёт уникальную временную папку
*/
static UINT rFS_NewRandDir_s4w ( const LPWSTR s4w )
{
  static UINT32 x = 0xbe8476f2;
  WCHAR wsz[12];
  while ( TRUE )
  {
    x = (x * 1103515245U) + 12345U;
    const UINT i = r4_push_array_s4w_sz ( s4w, wsz, swprintf ( wsz, 11, L"\\%08" TEXT(PRIx32), x ) + 1 );
    if ( !CreateDirectory ( s4w, NULL ) )
    {
      const UINT i = GetLastError();
      if ( i != ERROR_ALREADY_EXISTS ) { rLog_Error_WinAPI ( CreateDirectory, GetLastError(), L"%s\n", s4w ); return 0; }
      else { continue; }
    }
    else { return i; }
  }
}

UINT rFS_DeleteTree_FileProc ( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize );
UINT rFS_DeleteTree_FolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName );

UINT rFS_DeleteTree_FileProc ( const LPWSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize )
{
  if ( !DeleteFile ( s4wPath ) )
  {
    rLog_Error_WinAPI ( DeleteFile, GetLastError(), L"%s\n", s4wPath );
    return __LINE__;
  }
  return 0;
}

UINT rFS_DeleteTree_FolderProc ( const LPWSTR s4wPath, const LPCWSTR wszFolderName )
{
  const UINT iErr = rFS_Tree ( s4wPath, rFS_DeleteTree_FileProc, rFS_DeleteTree_FolderProc );
  if ( iErr ) return iErr;
  if ( !RemoveDirectory ( s4wPath ) )
  {
    rLog_Error_WinAPI ( RemoveDirectory, GetLastError(), L"%s\n", s4wPath );
    return __LINE__;
  }
  return 0;
}

static UINT rFS_DeleteTree ( const LPWSTR s4w )
{
  const UINT iErr = rFS_Tree ( s4w, rFS_DeleteTree_FileProc, rFS_DeleteTree_FolderProc );
  if ( iErr ) return iErr;
  if ( !RemoveDirectory ( s4w ) )
  {
    rLog_Error_WinAPI ( RemoveDirectory, GetLastError(), L"%s\n", s4w );
    return __LINE__;
  }
  return 0;
}
