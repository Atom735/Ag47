/*
  Поиск вспомогательной программы 7zip
*/
static BOOL rFS_SearchExe_7Zip ( LPWSTR const s4w )
{
  WIN32_FIND_DATA ffd;
  r4_cut_end_s4w ( s4w, 0 );
  r4_push_array_s4w_sz ( s4w, L"C:/Program Files/7-Zip/7z.exe", 0 );
  HANDLE const hFind = FindFirstFile ( s4w, &ffd );
  if ( hFind == INVALID_HANDLE_VALUE )
  {
    rLog_Error_WinAPI ( FindFirstFile, GetLastError(), L"%s\r\n", s4w );
    rLog_Error ( L"Невозможно найти [7z.exe], возможно не установлен [7zip]\r\n" );
    FindClose ( hFind );
    r4_cut_end_s4w ( s4w, 0 );
    return FALSE;
  }
  rLog ( L"!INFO: [7z.exe] найден по пути: %s\r\n", s4w );
  FindClose ( hFind );
  return TRUE;
}

/*
  Поиск вспомогательной программы wordconv
*/
static BOOL rFS_SearchExe_Wordconv ( LPWSTR const s4w )
{
  WIN32_FIND_DATA ffd;
  LPCWSTR const wsz = L"C:/Program Files (x86)/Microsoft Office/Office*";
  HANDLE const hFind = FindFirstFile ( wsz, &ffd );
  if ( hFind == INVALID_HANDLE_VALUE )
  {
    rLog_Error_WinAPI ( FindFirstFile, GetLastError(), L"%s\r\n", wsz );
    rLog_Error ( L"Невозможно найти папку с установленным [MS Office]\r\n" );
    FindClose ( hFind );
    return FALSE;
  }
  do
  {
    r4_cut_end_s4w ( s4w, 0 );
    r4_push_array_s4w_sz ( s4w, L"C:/Program Files (x86)/Microsoft Office/", 0 );
    r4_push_array_s4w_sz ( s4w, ffd.cFileName, 0 );
    r4_push_array_s4w_sz ( s4w, L"/wordconv.exe", 0 );
    WIN32_FIND_DATA _ffd;
    HANDLE const _hFind = FindFirstFile ( s4w, &_ffd );
    if ( _hFind != INVALID_HANDLE_VALUE )
    {
      FindClose ( _hFind );
      FindClose ( hFind );
      return TRUE;
    }
  } while ( FindNextFile ( hFind, &ffd ) );
  FindClose ( hFind );
  rLog_Error ( L"Невозможно найти [wordconv.exe]\r\n" );
  r4_cut_end_s4w ( s4w, 0 );
  return FALSE;
}

static BOOL rFS_ProcRun ( LPWSTR const cmd, LPPROCESS_INFORMATION const pi )
{
  STARTUPINFO s =
  {
    .cb = sizeof(STARTUPINFO),
    .dwFlags = STARTF_USESTDHANDLES,
    .hStdInput = GetStdHandle ( STD_INPUT_HANDLE ),
    .hStdOutput = GetStdHandle ( STD_OUTPUT_HANDLE ),
    .hStdError = GetStdHandle ( STD_ERROR_HANDLE ),
  };
  if ( !CreateProcess ( NULL, cmd, NULL , NULL, TRUE,
          CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,
          NULL, NULL, &s, pi ) )
  {
    rLog_Error_WinAPI ( CreateProcess, GetLastError(), L"%s\r\n", cmd );
    return FALSE;
  }
  return TRUE;
}

static UINT rFS_ProcRun_Wait ( LPWSTR const cmd )
{
  PROCESS_INFORMATION pi;
  if ( !rFS_ProcRun ( cmd, &pi ) ) { return UINT_MAX; };
  // Wait until child process exits.
  WaitForSingleObject( pi.hProcess, INFINITE );
  DWORD dw;
  if ( !GetExitCodeProcess ( pi.hProcess, &dw ) )
  {
    rLog_Error_WinAPI ( GetExitCodeProcess, GetLastError(), L"%s\r\n", cmd );
    return UINT_MAX;
  }
  // Close process and thread handles.
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );
  return (UINT)dw;
}

static BOOL rFS_Run_7Zip ( struct ag47_script * const script,
        LPCWSTR const wszIn, LPCWSTR const wszOut )
{
  if ( !script->s4wPathTo7Zip )
  {
    script->s4wPathTo7Zip = r4_malloc_s4w ( kPathMax );
    rFS_SearchExe_7Zip ( script->s4wPathTo7Zip );
  }
  LPWSTR const cmd = r4_alloca_s4w ( kPathMax*4 );
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  r4_push_path_s4w_s4w ( cmd, script->s4wPathTo7Zip );
  r4_push_array_s4w_sz ( cmd, L"\" x \"-o", 8 );
  r4_push_path_s4w_s4w ( cmd, wszOut );
  r4_push_array_s4w_sz ( cmd, L"\" \"", 4 );
  r4_push_path_s4w_s4w ( cmd, wszIn );
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  UINT const i = rFS_ProcRun_Wait ( cmd );
  switch ( i )
  {
    case 0: return TRUE;
    case 1:
      rLog_Error ( L"7zip ( %s => %s )\r\n%u (%x) === "
              L"Warning (Non fatal error(s)). For example, one or more files were locked by some other application, so they were not compressed.\r\n",
              wszIn, wszOut, i, i );
      return FALSE;
    case 2:
      rLog_Error ( L"7zip ( %s => %s )\r\n%u (%x) === "
              L"Fatal error\r\n",
              wszIn, wszOut, i, i );
      return FALSE;
    case 7:
      rLog_Error ( L"7zip ( %s => %s )\r\n%u (%x) === "
              L"Command line error\r\n",
              wszIn, wszOut, i, i );
      return FALSE;
    case 8:
      rLog_Error ( L"7zip ( %s => %s )\r\n%u (%x) === "
              L"Not enough memory for operation\r\n",
              wszIn, wszOut, i, i );
      return FALSE;
    case 255:
      rLog_Error ( L"7zip ( %s => %s )\r\n%u (%x) === "
              L"User stopped the process\r\n",
              wszIn, wszOut, i, i );
      return FALSE;
    default:
      rLog_Error ( L"7zip ( %s => %s )\r\n%u (%x) === "
              L"Unknown error\r\n",
              wszIn, wszOut, i, i );
      return FALSE;
  }
}

static UINT rFS_Run_WordConv ( struct ag47_script * const script,
        LPCWSTR const wszIn, LPCWSTR const wszOut )
{
  if ( !script->s4wPathToWordconv )
  {
    script->s4wPathToWordconv = r4_malloc_s4w ( kPathMax );
    rFS_SearchExe_Wordconv ( script->s4wPathToWordconv );
  }
  LPWSTR const cmd = r4_alloca_s4w ( kPathMax*4 );
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  r4_push_path_s4w_s4w ( cmd, script->s4wPathToWordconv );
  r4_push_array_s4w_sz ( cmd, L"\" -oice -nme \"", 15 );
  r4_push_path_s4w_s4w ( cmd, wszIn );
  r4_push_array_s4w_sz ( cmd, L"\" \"", 4 );
  r4_push_path_s4w_s4w ( cmd, wszOut );
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  UINT const i = rFS_ProcRun_Wait ( cmd );
  switch ( i )
  {
    case 0: return TRUE;
    default:
      rLog_Error ( L"WordConv ( %s => %s )\r\n%u (%x) === "
              L"Unknown error\r\n",
              wszIn, wszOut, i, i );
      return FALSE;
  }
}


/*
  Устанавливает вектор s4w в текущую дерикторию
  Возвращает длину добавленного пути
*/
static UINT rFS_AddCurrentDirectory ( LPWSTR const s4w )
{
    UINT const u = GetCurrentDirectory ( r4_get_memsz_s4w ( s4w ) - r4_get_count_s4w ( s4w ), s4w + r4_get_count_s4w ( s4w ) );
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
*/
static BOOL rFS_AddDir ( LPWSTR const s4w, LPCWSTR const wsz, UINT const k )
{
  r4_push_array_s4w_sz ( s4w, wsz, k );
  if ( ! CreateDirectory ( s4w, NULL ) )
  {
    UINT const i = GetLastError();
    if ( i != ERROR_ALREADY_EXISTS )
    {
      rLog_Error_WinAPI ( CreateDirectory, i, L"%s\n", s4w );
      return FALSE;
    }
  }
  return TRUE;
}

/*
  Поиск файлов в папке
  s4wPath               => путь к дериктории где производится поиск,
                        в конце будет приписано окончание \* для поиска всех файлов
  bArchives             => искать ли в архивах
  prFileProc            => Функция обработки найденных файлов
  prFolderProc          => Функция обработки найденных папок
*/
static BOOL rFS_Tree ( LPWSTR const s4wPath,
        BOOL (*prFileProc)( LPWSTR const s4wPath, LPCWSTR const wszFileName,
        UINT const nFileSize, LPVOID const ptr ),
        BOOL (*prFolderProc)( LPWSTR const s4wPath, LPCWSTR const wszFolderName,
        LPVOID const ptr ), LPVOID const ptr )
{
  WIN32_FIND_DATA ffd;
  UINT const n1 = r4_get_count_s4w ( s4wPath );
  r4_push_array_s4w_sz ( s4wPath, L"\\*", 3 );
  HANDLE const hFind  = FindFirstFile ( s4wPath, &ffd );
  if ( hFind == INVALID_HANDLE_VALUE )
  {
    rLog_Error_WinAPI ( FindFirstFile, GetLastError(), s4wPath );
    r4_cut_end_s4w ( s4wPath, n1 );
    return FALSE;
  }
  r4_cut_end_s4w ( s4wPath, n1 );
  do
  {
    if ( wcscmp ( ffd.cFileName, L"." ) == 0 ) continue;
    if ( wcscmp ( ffd.cFileName, L".." ) == 0 ) continue;
    UINT const n2 = r4_get_count_s4w ( s4wPath );
    r4_push_array_s4w_sz ( s4wPath, L"\\", 2 );
    r4_push_array_s4w_sz ( s4wPath, ffd.cFileName, 0 );
    if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    { if ( prFolderProc ) { if ( ( !prFolderProc ( s4wPath, ffd.cFileName, ptr ) ) ) { FindClose ( hFind ); return FALSE; } } }
    else
    { if ( prFileProc ) { if ( ( !prFileProc ( s4wPath, ffd.cFileName, ffd.nFileSizeLow, ptr ) ) ) { FindClose ( hFind ); return FALSE; } } }
    r4_cut_end_s4w ( s4wPath, n2 );
  } while ( FindNextFile ( hFind, &ffd ) );
  FindClose ( hFind ); return TRUE;
}


/*
  Создаёт уникальную временную папку
*/
static BOOL rFS_NewRandDir_s4w ( LPWSTR const s4w )
{
  static UINT32 x = 0xbe8476f2;
  WCHAR wsz[12];
  while ( TRUE )
  {
    x = (x * 1103515245U) + 12345U;
    const UINT k = r4_get_count_s4w ( s4w );
    r4_push_array_s4w_sz ( s4w, wsz, swprintf ( wsz, 11, L"\\%08" TEXT(PRIx32), x ) + 1 );
    if ( !CreateDirectory ( s4w, NULL ) )
    {
      const UINT i = GetLastError();
      if ( i != ERROR_ALREADY_EXISTS ) { rLog_Error_WinAPI ( CreateDirectory, GetLastError(), L"%s\r\n", s4w ); return FALSE; }
      else
      {
        r4_cut_end_s4w ( s4w, k );
        continue;
      }
    }
    else { return TRUE; }
  }
}

BOOL rFS_DeleteTree_FileProc ( LPWSTR const s4wPath, LPCWSTR const wszFileName,
        UINT const nFileSize, LPVOID const ptr );
BOOL rFS_DeleteTree_FolderProc ( LPWSTR const s4wPath, LPCWSTR const wszFolderName,
        LPVOID const ptr );

BOOL rFS_DeleteTree_FileProc ( LPWSTR const s4wPath, LPCWSTR const wszFileName,
        UINT const nFileSize, LPVOID const ptr )
{
  if ( !DeleteFile ( s4wPath ) )
  {
    rLog_Error_WinAPI ( DeleteFile, GetLastError(), L"%s\r\n", s4wPath );
    return FALSE;
  }
  return TRUE;
}

BOOL rFS_DeleteTree_FolderProc ( LPWSTR const s4wPath, LPCWSTR const wszFolderName,
        LPVOID const ptr )
{
  if ( !rFS_Tree ( s4wPath, rFS_DeleteTree_FileProc, rFS_DeleteTree_FolderProc, ptr ) ) { return FALSE; }
  if ( !RemoveDirectory ( s4wPath ) ) { rLog_Error_WinAPI ( RemoveDirectory, GetLastError(), L"%s\r\n", s4wPath ); return FALSE; }
  return TRUE;
}
/*
  Удаляет папку и его содержимое
  s4w                   -- Путь к папке которую удаляем
*/
static BOOL rFS_DeleteTree ( const LPWSTR s4w )
{
  if ( !rFS_Tree ( s4w, rFS_DeleteTree_FileProc, rFS_DeleteTree_FolderProc, NULL ) ) { return FALSE; }
  if ( !RemoveDirectory ( s4w ) ) { rLog_Error_WinAPI ( RemoveDirectory, GetLastError(), L"%s\r\n", s4w ); return FALSE; }
  return TRUE;
}

struct file_map
{
  HANDLE                hFile;
  HANDLE                hMapping;
  UINT                  nSize;
  BYTE          const * pData;
};

static VOID rFS_FileMapClose ( struct file_map * const pMap )
{
  if ( pMap->pData && !UnmapViewOfFile ( pMap->pData ) ) { rLog_Error_WinAPI ( UnmapViewOfFile, GetLastError(), L"Data\n" ); }
  if ( pMap->hMapping && !CloseHandle ( pMap->hMapping ) ) { rLog_Error_WinAPI ( CloseHandle, GetLastError(), L"Mapping\n" ); }
  if ( pMap->hFile && !CloseHandle ( pMap->hFile ) ) { rLog_Error_WinAPI ( CloseHandle, GetLastError(), L"File\n" ); }
  *pMap = ((struct file_map){ NULL, NULL, 0, NULL });
}

static BOOL rFS_FileMapOpen ( struct file_map * const pMap, const LPCWSTR wszFilePath )
{
  struct file_map fm = { };
  if ( ( fm.hFile = CreateFile ( wszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE )
  {
    rLog_Error_WinAPI ( CreateFile, GetLastError(), L"%s\r\n", wszFilePath );
    fm.hFile = NULL;
    return FALSE;
  }
  if ( ( fm.nSize = GetFileSize ( fm.hFile, NULL ) ) == INVALID_FILE_SIZE )
  {
    rLog_Error_WinAPI ( GetFileSize, GetLastError(), L"%s\r\n", wszFilePath );
    rFS_FileMapClose ( &fm );
    return FALSE;
  }
  if ( ( fm.hMapping = CreateFileMapping ( fm.hFile, NULL, PAGE_READONLY, 0, 0, NULL ) ) == NULL )
  {
    rLog_Error_WinAPI ( CreateFileMapping, GetLastError(), L"%s\r\n", wszFilePath );
    rFS_FileMapClose ( &fm );
    return FALSE;
  }
  if ( ( fm.pData = (BYTE const*) MapViewOfFileEx ( fm.hMapping, FILE_MAP_READ, 0, 0, 0, NULL ) ) == NULL )
  {
    rLog_Error_WinAPI ( MapViewOfFileEx, GetLastError(), L"%s\r\n", wszFilePath );
    rFS_FileMapClose ( &fm );
    return FALSE;
  }
  *pMap = fm;
  return TRUE;
}
