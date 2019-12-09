﻿static UINT rFS_Run ( const LPWSTR cmd, LPPROCESS_INFORMATION pi )
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



static UINT rFS_Run_7Zip ( const LPCWSTR wszIn, const LPCWSTR wszOut )
{
  const LPWSTR cmd = r4_alloca_s4w ( 2048*4 );
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
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
  r4_push_array_s4w_sz ( cmd, L"\"", 2 );
  return rFS_Run_Wait ( cmd );
}

static UINT rFS_Run_WordConv ( const LPCWSTR wszIn, const LPCWSTR wszOut )
{
  const LPWSTR cmd = r4_alloca_s4w ( 2048*4 );
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

// g_s4wPathToWordConv
// g_s4wPathTo7Zip




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
  {
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
      {
        if ( prFolderProc ) { prFolderProc ( s4wPath, ffd.cFileName ); }
      }
      else
      {
        if ( prFileProc ) { prFileProc ( s4wPath, ffd.cFileName, ffd.nFileSizeLow ); }
      }
      r4_cut_end_s4w ( s4wPath, n2 );
    } while ( FindNextFile ( hFind, &ffd ) );
    FindClose ( hFind );
  }
  return 0;
}
