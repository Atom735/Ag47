﻿struct ag47_script
{
  /*
    {0} == {FALSE} == {NO} == {NULL}
    {1+} == {TRUE} == {YES}
  */

  /*
    Путь к файлу или папке для поиска
    По умолчанию {"."} - рабочая папка
  */
  LPWSTR                s4wRun;
  /*
    Путь к папке для отладочной информации и сборки проекта
    По умолчанию {".ag47"} - создаёт папку в рабочей папке
  */
  LPWSTR                s4wOutPath;
  /*
    Флаг пересоздания папки для выхода
    По умолчанию {FALSE} - не отчищает папку если она существует
  */
  BOOL                  bOutRecreate;
  // === LAS === === === === === === === === === === === === === === === === === === === === === ===
  /*
    Шаблоны поиска LAS файлов
    По умолчанию {["*.las","*.las[?]"]}
    Возможные значения:
      {[]}              - Отключить парсинг ( Ноль элементов )
  */
  LPWSTR              * ss4wLasFF;
  /*
    Тип модификации [LAS] файлов
    По умолчанию {0} или {NULL} или {FALSE}
    Возможные значения:
      {0}               - Отключить модификацию
      {1}               - Включить модифицаию без дополнительной информации
      {2}               - Добавить информацию в заголовке файла
      {3}               - Добавить к файлу отладочную информацию
  */
  UINT                  iLasMod;
  /*
    Символ новой строки для конечного [LAS] файла (если включена модификация)
    По умолчанию {NULL}
    Возможные значения:
      {NULL}            - Значение как в исходном файле
      {CRLF}            - Значение как в системах Windows
      {LF}              - Значение как в системах UNIX
      {CR}              - Значение как в системах Macintosh
  */
  UINT                  iLasNL;
  /*
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
  /*
    Шаблоны поиска файлов с инклинометрией
    По умолчанию {["*.txt","*.doc","*.docx","*.dbf"]}
    Возможные значения:
      {[]}              - Отключить парсинг ( Ноль элементов )
  */
  LPWSTR              * ss4wInkFF;
  /*
    Символ новой строки для конечного [INK] файла
    По умолчанию {CRLF}
    Возможные значения:
      {NULL}            - Значение по умолчанию
      {CRLF}            - Значение как в системах Windows
      {LF}              - Значение как в системах UNIX
      {CR}              - Значение как в системах Macintosh
  */
  UINT                  iInkNL;
  /*
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


/*
  Загружает и выполняет скрипт
*/
static UINT rScriptRun ( LPCWSTR const wszPath )
{
  struct ag47_script _script = { };

}
