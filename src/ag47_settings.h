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
  /* Путь к оригиналу обрабатываемого файла */
  LPWSTR                s4wOrigin;

  LPWSTR                s4wPathOutTempDir;
  LPWSTR                s4wPathOutLogsDir;
  LPWSTR                s4wPathOutLasDir;
  LPWSTR                s4wPathOutInkDir;
  LPWSTR                s4wPathOutErrorDir;

  /* Код ошибки скрипта */
  UINT                  iErr;
  /* CodePage самого скрипта */
  UINT                  iCP[2];
  /* Количество обработанных LAS файлов */
  UINT                  nLasDetected;
  /* Файл базы данных всех обработаных LAS файлов  */
  FILE                * pFLasDB;
  FILE                * pFAB;
  /* Количество файлов с ошибкой */
  UINT                  nErrorFiles;



  // === SET === === === === === === === === === === === === === === === === === === === === === ===
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
  /*  PATH_TO_WORDCONV
      Путь к программе wordconv из офисного пакета Microsoft
      По умолчанию {NULL} - Поиск в Program Files
  */
  LPWSTR                s4wPathToWordconv;
  /*  PATH_TO_7ZIP
      Путь к программе архиватору 7zip
      По умолчанию {NULL} - Поиск в Program Files
  */
  LPWSTR                s4wPathTo7Zip;

  /*  OUT_RECREATE
    Флаг пересоздания папки для выхода
    По умолчанию {FALSE} - не отчищает папку если она существует
  */
  BOOL                  bOutRecreate;
  /*  EXCLUDE_FF
    Шаблоны файлов, которые будут исключены из поиска
    По умолчанию {NULL} - Исключает папку OUT_PATH
    Возможные значения:
      {NULL}            - Вернуть значение поумолчанию
      {[]}              - Отключить исключение файлов ( Ноль элементов )

    Папки . и .. будут всегда исключены
  */
  LPWSTR              * ss4wExcludeFF;
  /*  EXCLUDE_SIZE
    Размеры файлов, которые будут исключены из поиска
    По умолчанию {NULL} - отключает исключение по размерам
    Задаётся парами чисел, если чисел нечётное количество,
    то также будут исключены все файлы превышающие последнее число
    Если первое значение меньше второго, то исключены будут файлы в диапозоне
    Иначе исключены будут файлы вне диапозона
    Например:
      {[1024]}          - Исключает все файл, размером более одного килобайта
      {[1024,0]}        - Аналогично предудщему
      {[0,1024,1048576]}- Исключает файлы меньше одного килобайта и больше одного мегабайта
      {[1048576,1024]}  - Аналогично предудщему
  */
  UINT                * s4uExcludeSizes;
  /*  RECURSIVE
    Максимальная глубина поиска по подпапкам и архивам
    По умолчанию {0}
    Возможные значения:
      {0}               - Бесконечный поиск вложенности
      {1}               - Отключить поиск в подпапках
  */
  UINT                  nRecursive;

  /*  ARCHIVE_FF
    Шаблоны поиска архивных файлов
    По умолчанию {["*.zip","*.rar","*.7z"]}
    Возможные значения:
      {[]}              - Отключить вхождение в архивы ( Ноль элементов )
  */
  LPWSTR              * ss4wArchiveFF;
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
      {+0x1}            - Включить преобразование кодировок и символа конца линии
      {+0x2}            - Добавить информацию в заголовке файла
      {+0x4}            - Отчистить от комментариев
      {+0x10}           - Добавить к файлу отладочную информацию
      {+0x20}           - Добавить данне файлы к БД
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
  UINT                  iLasCP[2];
  /* LAS_ERR
    Значение ошибки погрешности для расчёта секций ASCII
    По умолчанию {0.0} - переходит {в 1e-6}
  */
  double                fLasErr;

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
  UINT                  iInkCP[2];
};

enum
{
  kErr_Ok = 0,

  kErr_Script_InvalidValue,
  kErr_Script_EndOfValue,
  kErr_Script_EqValue,
  kErr_Script_ErrSymbol,
  kErr_Script_ValueName,
  kErr_Script_ArraySeparator,

  kErr_ParserLas_Section,
  kErr_ParserLas_EOF_DoublePoint,
  kErr_ParserLas_EOL_DoublePoint,
  kErr_ParserLas_EOF_Space,
  kErr_ParserLas_EOL_Space,
  kErr_ParserLas_EOF_Point,
  kErr_ParserLas_EOL_Point,
  kErr_ParserLas_EOF,

  kErr_ParserLas_UndefinedMnem,
  kErr_ParserLas_IncorrectVersion,
  kErr_ParserLas_IncorrectWrap,

  kErr_ParserLas_CantReadAscii_First,
  kErr_ParserLas_CantReadAscii,
  kErr_ParserLas_FistDepthNotEaqual,
  kErr_ParserLas_IncorrectDepthGap,
  kErr_ParserLas_NotEOF,
};

LPCWSTR const g7ErrStrScript[] =
{
  [kErr_Ok]                             = L"Всё в порядке",

  [kErr_Script_InvalidValue]            = L"Ошибка синтаксиса, некорректное значение",
  [kErr_Script_EndOfValue]              = L"Ошибка синтаксиса, отсуствует символ \';\'",
  [kErr_Script_EqValue]                 = L"Ошибка синтаксиса, отсуствует символ \'=\'",
  [kErr_Script_ErrSymbol]               = L"Ошибка синтаксиса, непредвиденный символ",
  [kErr_Script_ValueName]               = L"Неизвестное название переменной",
  [kErr_Script_ArraySeparator]          = L"Ошибка синтаксиса, отсутсвует разделитель \',\'",

  [kErr_ParserLas_Section]              = L"Неопределённая секция",
  [kErr_ParserLas_EOF_DoublePoint]      = L"Непредвиденный конец файла (отсутсвует двоеточие)",
  [kErr_ParserLas_EOL_DoublePoint]      = L"Непредвиденный конец строки (отсутсвует двоеточие)",
  [kErr_ParserLas_EOF_Space]            = L"Непредвиденный конец файла (отсутсвует пробел после точки)",
  [kErr_ParserLas_EOL_Space]            = L"Непредвиденный конец строки (отсутсвует пробел после точки)",
  [kErr_ParserLas_EOF_Point]            = L"Непредвиденный конец файла (отсутсвует точка)",
  [kErr_ParserLas_EOL_Point]            = L"Непредвиденный конец строки (отсутсвует точка)",
  [kErr_ParserLas_EOF]                  = L"Непредвиденный конец файла",

  [kErr_ParserLas_UndefinedMnem]        = L"Неизвестная мнемоника",
  [kErr_ParserLas_IncorrectVersion]     = L"Некооректное значение версии LAS файла",
  [kErr_ParserLas_IncorrectWrap]        = L"Некооректное значение флага многострочности данных",

  [kErr_ParserLas_CantReadAscii_First]  = L"Невозможо прочесть первое значение глубины",
  [kErr_ParserLas_CantReadAscii]        = L"Невозможо прочесть значение",
  [kErr_ParserLas_FistDepthNotEaqual]   = L"Первое значение глубины не совпадает с полем ~W:STRT",
  [kErr_ParserLas_IncorrectDepthGap]    = L"Непредвиденый разрыв значения глубин",
  [kErr_ParserLas_NotEOF]               = L"Излишние данные в конце файла",


};

