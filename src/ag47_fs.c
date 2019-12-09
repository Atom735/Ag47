/*
  Поиск файлов в папке
  s4wPath               => путь к дериктории где производится поиск,
                        в конце будет приписано окончание \* для поиска всех файлов
  bArchives             => искать ли в архивах
  prFileProc            => Функция обработки найденных файлов
  prFolderProc          => Функция обработки найденных папок
*/
static UINT rFS_Tree ( const LPWSTR s4wPath,
        UINT (*prFileProc)( const LPCSTR s4wPath, const LPCWSTR wszFileName,
        const UINT nFileSize ), UINT (*prFolderProc)( const LPCSTR s4wPath,
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
