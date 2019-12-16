struct ag47_script
{
  /*
    {0} == {FALSE} == {NO} == {NULL}
    {1+} == {TRUE} == {YES}
  */

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
      {NULL}            - Значение как в исходном файле
      {CRLF}            - Значение как в системах Windows
      {LF}              - Значение как в системах UNIX
      {CR}              - Значение как в системах Macintosh
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






static BOOL rScript_ParseOne ( struct ag47_script * const script, struct mem_ptr_bin * const p )
{

}

static VOID rScript_GetCodePage ( struct ag47_script * const script, struct mem_ptr_bin * const p )
{
  if ( rMemPtrBin_Skip_ToFirstNonSpace ( p ) && *(p->p) == '#' )
  {
    rMemPtrBin_Skip ( p, 1 );
    rMemPtrBin_Skip_ToFirstNonSpace ( p ) ;
    const UINT iCP = rGetCodePageIdByAsciiName ( p->p );
    if ( script->iCP && script->iCP != iCP )
    {
      rLog_Error ( L"Не свопадают BOM скрипта (%hs) и указаная кодировка (%hs)\r\n",
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
  struct ag47_script _script = { };
  struct mem_ptr_bin _ptr = { .p = pBuf, .n = nSize };
  _script.iCP = rGetBOM ( &_ptr );
  if ( ( _script.iCP == 0 ) || ( _script.iCP == kCP_Utf8 ) )
  {
    rScript_GetCodePage ( &_script, &_ptr );
    struct mem_ptr_txt _txt = { ._ = _ptr, .nLine = 1, .iNL = rGetBufEndOfLine ( pBuf, nSize ) };
    while ( rScript_ParseOne ( &_script, &_ptr ) );
  }
  return _script.iErr;
}

/*
  Загружает и выполняет файл скрипа
  !wszPath              - путь к файлу скрипта
*/
static UINT rScriptRunFile ( LPCWSTR const wszPath )
{
  struct ag47_script _script = { };
  return 0;
}
