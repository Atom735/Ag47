struct ag47_script
{
  /*
    {0} == {FALSE} == {NO} == {NULL}
    {1+} == {TRUE} == {YES}
  */
  union
  {
    struct mem_ptr_bin *p_bin;
    struct mem_ptr_txt *p_txt;
  };

  /* Код ошибки скрипта */
  UINT                  iErr;
  /* CodePage самого скрипта */
  UINT                  iCP;
  /*  RUN
    Путь к файлу или папке для поиска
    По умолчанию {"."}  - рабочая папка
  */
  LPWSTR                s4wRun;
  /*  OUT_PATH
    Путь к папке для отладочной информации и сборки проекта
    По умолчанию {".ag47"} - создаёт папку в рабочей папке
  */
  LPWSTR                s4wOutPath;
  /*  OUT_RECREATE
    Флаг пересоздания папки для выхода
    По умолчанию {FALSE} - не отчищает папку если она существует
  */
  BOOL                  bOutRecreate;
  /*  EXCLUDE_FF
    Шаблоны файлов, которые будут исключены из поиска
    По умолчанию {[".ag47"]}
    Возможные значения:
      {[]}              - Отключить исключение файлов ( Ноль элементов )
    Папки . и .. будут всегда исключены
  */
  LPWSTR              * ss4wExcludeFF;
  /*  EXCLUDE_SIZE
    Размеры файлов, которые будут исключены из поиска
    По умолчанию {[0,0]}
    Если первое значение меньше второго, то исключены будут файлы в диапозоне
    Иначе исключены будут файлы вне диапозона
  */
  UINT                  nExcludeSizes[2];
  /*  RECURSIVE
    Максимальная глубина поиска по подпапкам и архивам
    По умолчанию {0}
    Возможные значения:
      {0}               - Бесконечный поиск вложенности
      {1}               - Отключить поиск в подпапках
  */
  UINT                  nRecursive;
  // === LAS === === === === === === === === === === === === === === === === === === === === === ===
  /*  LAS_FF
    Шаблоны поиска LAS файлов
    По умолчанию {["*.las","*.las[?]"]}
    Возможные значения:
      {[]}              - Отключить парсинг ( Ноль элементов )
  */
  LPWSTR              * ss4wLasFF;
  /*  LAS_MOD
    Тип модификации [LAS] файлов
    По умолчанию {0} или {NULL} или {FALSE}
    Возможные значения:
      {0}               - Отключить модификацию
      {1}               - Включить модифицаию без дополнительной информации
      {2}               - Добавить информацию в заголовке файла
      {3}               - Добавить к файлу отладочную информацию
  */
  UINT                  iLasMod;
  /*  LAS_NL
    Символ новой строки для конечного [LAS] файла (если включена модификация)
    По умолчанию {NULL}
    Возможные значения:
      {0},{NULL}        - Значение как в исходном файле
      {1},{CRLF}        - Значение как в системах Windows
      {2},{LF}          - Значение как в системах UNIX
      {3},{CR}          - Значение как в системах Macintosh
  */
  UINT                  iLasNL;
  /*  LAS_CP
    ID кодовой страницы для конечного [LAS] файла (если включена модификация)
    По умолчанию {NULL}
    Возможные значения:
      {NULL}            - Значение как в исходном файле
      {866},  {".866"},  {"cp866"}            - OEM Russian; Cyrillic (DOS)
      {1251}, {".1251"}, {"windows-1251"}     - ANSI Cyrillic; Cyrillic (Windows)
      Остальные по ссылке [https://docs.microsoft.com/en-us/windows/win32/Intl/code-page-identifiers]
  */
  UINT                  iLasCP;

  // === INK === === === === === === === === === === === === === === === === === === === === === ===
  /*  INK_FF
    Шаблоны поиска файлов с инклинометрией
    По умолчанию {["*.txt","*.doc","*.docx","*.dbf"]}
    Возможные значения:
      {[]}              - Отключить парсинг ( Ноль элементов )
  */
  LPWSTR              * ss4wInkFF;
  /*  INK_NL
    Символ новой строки для конечного [INK] файла
    По умолчанию {CRLF}
    Возможные значения:
      {NULL}            - Значение по умолчанию
      {CRLF}            - Значение как в системах Windows
      {LF}              - Значение как в системах UNIX
      {CR}              - Значение как в системах Macintosh
  */
  UINT                  iInkNL;
  /*  INK_CP
    ID кодовой страницы для конечного [INK] файла (если включена модификация)
    По умолчанию {1251}
    Возможные значения:
      {NULL}            - Значение по умолчанию
      {866},  {".866"},  {"cp866"}            - OEM Russian; Cyrillic (DOS)
      {1251}, {".1251"}, {"windows-1251"}     - ANSI Cyrillic; Cyrillic (Windows)
      Остальные по ссылке [https://docs.microsoft.com/en-us/windows/win32/Intl/code-page-identifiers]
  */
  UINT                  iInkCP;
};

enum
{
  kErr_Ok = 0,
  kErr_Script_InvalidValue,
  kErr_Script_EndOfValue,
  kErr_Script_EqValue,
  kErr_Script_ErrSymbol,
};

LPCWSTR const g7ErrStrScript[] =
{
  [kErr_Ok]                             = L"Всё в порядке",
  [kErr_Script_InvalidValue]            = L"Ошибка синтаксиса, некорректное значение",
  [kErr_Script_EndOfValue]              = L"Ошибка синтаксиса, отсуствует символ \';\'",
  [kErr_Script_EqValue]                 = L"Ошибка синтаксиса, отсуствует символ \'=\'",
  [kErr_Script_ErrSymbol]               = L"Ошибка синтаксиса, непредвиденный символ",
};

static UINT rLogScript_ ( LPCSTR const szFile, UINT const nLine,
        struct ag47_script * const script, UINT const iErr )
{
  return rLog_Error_ ( szFile,  nLine, L"script on line (%u): %16.*hs\r\n  error 0x%x => %s\r\n",
          script->p_txt->nLine, script->p_txt->n < 16 ? script->p_txt->n : 16,
          script->p_txt->p, iErr, g7ErrStrScript[iErr] );
}
#define rLogScript(_s_,_i_) rLogScript_(__FILE__,__LINE__,_s_,_i_)


static BOOL rScript_CaseName ( struct mem_ptr_txt * const p, LPCSTR const sz )
{ return rMemPtrTxt_CmpCaseWordA ( p, sz ) && rMemPtrTxt_Skip_NoValid ( p, strlen ( sz ) ); }

static BOOL rScript_ParseVal_String ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, LPWSTR * const ps4w )
{
  // rLog ( L" === (%x)%.8hs\r\n", *(p->p), p->p );
  if ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) && ( *(p->p) == '=' ) &&
       rMemPtrTxt_Skip_NoValid ( p, 1 ) && rMemPtrTxt_Skip_ToFirstNonSpace ( p ) )
  {
    // rLog ( L" === (%x)%.8hs\r\n", *(p->p), p->p );
    if ( rScript_CaseName ( p, "NULL" ) || *(p->p) == ';' ) { if ( *ps4w ) { r4_free_s4w ( *ps4w ); *ps4w = NULL; } }
    else
    if ( *(p->p) == '\'' )
    {
      rMemPtrTxt_Skip_NoValid ( p, 1 );
      UINT i = 0;
      for ( ; i < p->n; ++i ) { if ( p->p[i] == '\'' ) { break; } }
      UINT iW = MultiByteToWideChar ( script->iCP, 0, p->p, i, NULL, 0 );
      *ps4w = r4_malloc_s4w ( iW+1 );
      r4_get_count_s4w ( (*ps4w) ) = MultiByteToWideChar ( script->iCP, 0, p->p, i, *ps4w, iW+1 );
      (*ps4w)[iW] = 0;
      rMemPtrTxt_Skip_NoValid ( p, i+1 );
    }
    else
    { rLogScript ( script, kErr_Script_InvalidValue ); return FALSE; }
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( *(p->p) == ';' ) { return rMemPtrTxt_Skip_NoValid ( p, 1 ); }
    else
    { rLogScript ( script, kErr_Script_EndOfValue ); return FALSE; }
  }
  else
  { rLogScript ( script, kErr_Script_EqValue ); return FALSE; }
}

static BOOL rScript_ParseValName_String ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, LPWSTR * const ps4w, LPCSTR const sz )
{
  if ( rScript_CaseName ( p, sz ) )
  {
    const BOOL b = rScript_ParseVal_String ( script, p, ps4w );
    if ( b ) { rLog ( L"script {%hs}\t=> \'%s\'\r\n", sz, *ps4w ); }
    return b;
  }
  return FALSE;
}

static BOOL rScript_ParseName ( struct ag47_script * const script, struct mem_ptr_txt * const p )
{
  return rScript_ParseValName_String ( script, p, &(script->s4wRun), "RUN" );
}

static BOOL rScript_ParseOne ( struct ag47_script * const script, struct mem_ptr_txt * const p )
{
  rMemPtrTxt_Skip_ToFirstNonSpace ( p );
  if ( p->n )
  {
    switch ( *(p->p) )
    {
      case '#': return rMemPtrTxt_Skip_ToBeginNewLine ( p );
      case '/':
        if ( p->n >= 2 )
        {
          if ( (p->p)[1] == '/' ) { return rMemPtrTxt_Skip_ToBeginNewLine ( p ); }
          else
          if ( (p->p)[1] == '*' ) {  return rMemPtrTxt_Skip_ToFirstCmpArrayA ( p, "*/" ) >= 2 ?
                  rMemPtrTxt_Skip_NoValid ( p, 2) : FALSE; }
          else { rLogScript ( script, kErr_Script_ErrSymbol ); return FALSE; }
        }
      case 'a' ... 'z':
      case 'A' ... 'Z':
        return rScript_ParseName ( script, p );
      default: rLogScript ( script, kErr_Script_ErrSymbol ); return FALSE;
    }
  }
  else
  { return FALSE; }
}

static VOID rScript_GetCodePage ( struct ag47_script * const script, struct mem_ptr_bin * const p )
{
  if ( rMemPtrBin_Skip_ToFirstNonSpace ( p ) && *(p->p) == '#' )
  {
    rMemPtrBin_Skip ( p, 1 );
    rMemPtrBin_Skip_ToFirstNonSpace ( p ) ;
    const UINT iCP = rGetCodePageIdByAsciiName ( (LPCSTR)p->p );
    if ( script->iCP && script->iCP != iCP )
    {
      rLog_Error ( L"script не свопадают BOM скрипта (%hs) и указаная кодировка (%hs)\r\n",
              rGetCodePageNameById ( script->iCP ), rGetCodePageNameById ( iCP ) );
    }
    else
    {
      script->iCP = iCP;
    }
    rMemPtrBin_Skip_ToBeginNewLine ( p );
  }
}

/*
  Загружает и выполняет скрипт находящийся в памяти
  !pBuf                 - указатель на память с данными
  !nSize                - размер памяти с данными
*/
static UINT rScriptRunMem ( BYTE const * const pBuf, UINT const nSize )
{
  struct mem_ptr_bin _ptr = { .p = pBuf, .n = nSize };
  struct ag47_script _script = { .p_bin = &_ptr };
  _script.iCP = rGetBOM ( &_ptr );
  if ( ( _script.iCP == 0 ) || ( _script.iCP == kCP_Utf8 ) )
  {
    struct mem_ptr_txt _txt = { ._ = _ptr, .nLine = 1, .iNL = rGetBufEndOfLine ( pBuf, nSize ) };
    _script.p_txt = &_txt;
    rScript_GetCodePage ( &_script, &_ptr );
    while ( rScript_ParseOne ( &_script, &_txt ) );
  }
  else
  {
    rLog_Error ( L"Code Page\r\n" );
  }

  return _script.iErr;
}

/*
  Загружает и выполняет файл скрипа
  !wszPath              - путь к файлу скрипта
*/
static UINT rScriptRunFile ( LPCWSTR const wszPath )
{
  struct file_map fm;
  UINT iErr = 0;
  if ( ( iErr = rFS_FileMapOpen ( &fm, wszPath ) ) ) { return iErr; }
  iErr = rScriptRunMem ( fm.pData, fm.nSize );
  rFS_FileMapClose ( &fm );
  return iErr;
}
