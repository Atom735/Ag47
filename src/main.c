#include <windows.h>

#include <inttypes.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#define kPathMax PATH_MAX
static LPWSTR s4wPathOut = NULL;
static LPWSTR s4wPathOutTempDir = NULL;
static LPWSTR s4wPathOutLogsDir = NULL;
static _locale_t g_locale_C = NULL;


#include "ag47_misc.c"
#include "ag47_log.c"
#include "ag47_map.c"
#include "ag47_arrays.c"
#include "ag47_fs.c"
#include "ag47_parse_las.c"
#include "ag47_parse.c"

INT APIENTRY wWinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd )
{



  g_locale_C = _create_locale ( LC_ALL, "C" );
  AllocConsole ( );
  g_s4wPathToWordConv = r4_alloca_s4w ( PATH_MAX );
  g_s4wPathTo7Zip = r4_alloca_s4w ( PATH_MAX );
  rFS_SearchExe ( );
  s4wPathOut = r4_alloca_init_ex_s4w ( L"\\\\?\\", kPathMax );
  rFS_GetCurrentDirectory_s4w ( s4wPathOut );
  rFS_AddDir ( s4wPathOut, L"\\.ag47", 0 );
  s4wPathOutTempDir = r4_alloca_s4w ( kPathMax );
  r4_init_s4w_s4w ( s4wPathOutTempDir, s4wPathOut );
  rFS_AddDir ( s4wPathOutTempDir, L"\\.temp", 0 );
  s4wPathOutLogsDir = r4_alloca_s4w ( kPathMax );
  r4_init_s4w_s4w ( s4wPathOutLogsDir, s4wPathOut );
  rFS_AddDir ( s4wPathOutLogsDir, L"\\.logs", 0 );


  // const LPWSTR s4wPathIn = r4_alloca_init_ex_s4w ( L"\\\\?\\UNC\\NAS\\Public", kPathMax );
  const LPWSTR s4wPathIn = r4_alloca_init_ex_s4w ( L"\\\\?\\UNC\\NAS\\Public\\common\\Gilyazeev\\ГИС", kPathMax );
  const LPWSTR  s4wPathOrigin = r4_alloca_s4w ( kPathMax );
  r4_init_s4w_s4w ( s4wPathOrigin, s4wPathIn );
  const UINT iErr = rParse_Tree ( s4wPathIn, s4wPathOrigin );

  rLog ( NULL );
  _free_locale ( g_locale_C );
  return iErr;
}
