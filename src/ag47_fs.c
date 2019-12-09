/*
  Поиск файлов в папке
  s4wFilePath           => путь к дериктории где производится поиск,
                        в конце будет приписано окончание \* для поиска всех файлов
  bRecursive            => использовать ли рекурсию
  bArchives             => искать ли в архивах
  prFileProc            => Функция обработки найденных файлов
*/
static UINT rFS_Tree ( const LPWSTR s4wFilePath, const BOOL bRecursive,
        UINT (*prFileProc)( const LPCWSTR s4wFilePath, const LPCWSTR wszFileName,
        const UINT nFileSize ) )
{
  WIN32_FIND_DATA ffd;
  {
    const UINT n1 = r4_push_array_s4w_sz ( s4wFilePath, L"\\*", 3 );
    const HANDLE hFind  = FindFirstFile ( s4wFilePath, &ffd );
    if ( hFind == INVALID_HANDLE_VALUE )
    {
      rLog_Error_WinAPI ( &FindFirstFile, GetLastError(), s4wFilePath );
      r4_cut_end_s4w ( s4wFilePath, n1 );
      return __LINE__;
    }
    r4_cut_end_s4w ( s4wFilePath, n1 );
    do
    {
      if ( wcscmp ( ffd.cFileName, L"." ) == 0 ) continue;
      if ( wcscmp ( ffd.cFileName, L".." ) == 0 ) continue;
      const UINT n2 = r4_push_array_s4w_sz ( s4wFilePath, L"\\", 2 );
      r4_push_array_s4w_sz ( s4wFilePath, ffd.cFileName, 0 );
      if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
      {
        if ( bRecursive ) { rFS_Tree ( s4wFilePath, bRecursive, prFileProc ); }
      }
      else
      {
        if ( prFileProc ) { prFileProc ( s4wFilePath, ffd.cFileName, ffd.nFileSizeLow ); }
      }
      r4_cut_end_s4w ( s4wFilePath, n2 );
    } while ( FindNextFile ( hFind, &ffd ) );
    FindClose ( hFind );
  }
  return 0;
}
