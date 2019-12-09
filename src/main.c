#include <windows.h>

#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#define kPathMax PATH_MAX
static LPWSTR s4wPathOut = NULL;
static LPWSTR s4wPathOutTempDir = NULL;

#include "ag47_misc.c"
#include "ag47_log.c"
#include "ag47_tbl_rus_a.c"
#include "ag47_tbl_rus_b.c"
#include "ag47_arrays.c"
#include "ag47_fs.c"
#include "ag47_parse.c"









INT APIENTRY wWinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd )
{
  AllocConsole ( );
  g_s4wPathToWordConv = r4_alloca_s4w ( PATH_MAX );
  g_s4wPathTo7Zip = r4_alloca_s4w ( PATH_MAX );
  rFS_SearchExe ( );
  s4wPathOut = r4_alloca_init_ex_s4w ( L"\\\\?\\", kPathMax );
  rFS_GetCurrentDirectory_s4w ( s4wPathOut );
  rFS_AddDir ( s4wPathOut, L"\\.ag47", 0 );
  s4wPathOutTempDir = r4_alloca_s4w ( kPathMax );
  r4_init_s4w_s4w ( s4wPathOutTempDir, s4wPathOut );
  rFS_AddDir ( s4wPathOut, L"\\.temp", 0 );


  r4_push_array_s4w_sz ( s4wPFolders, L"\\db", 0 );
  if ( ! CreateDirectory ( s4wPFolders, NULL ) )
  {
    rLog_Error_WinAPI ( CreateDirectory, GetLastError(), s4wPFolders );
  }

  const LPWSTR s4w = r4_alloca_init_ex_s4w ( L"\\\\?\\UNC\\NAS\\Public", kPathMax );
  rFS_Tree ( s4w, rFileProc, rFolderProc );
  rLog ( L"\t\t==>% 14u ==> All files (%u)\n", nSizes, nFiles );
  rLog ( NULL );
  return 0;
}
