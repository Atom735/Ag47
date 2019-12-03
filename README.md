# Рабочий проект

## Имспользуемые средства

[msys2](https://www.msys2.org/)

Библиотеки:
* libarchive
* libxml2

```bash
  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-make mingw-w64-x86_64-libarchive mingw-w64-x86_64-libxml2
```

## Файл сценария

Расширение файла __\*.ag47-script__

Файл начинается с заголовка, первый символ либо [BOM](https://ru.wikipedia.org/wiki/Маркер_последовательности_байтов),
либо `@` после которого указывается кодировка файла (прим. `@UTF-8`, `@.1251`).
Номера кодовых страниц можно найти на сайте [msdn](https://docs.microsoft.com/ru-ru/windows/win32/intl/code-page-identifiers)

### Общие сведения

* `//` - комментарий до конца строки
* `/*...*/` - блок комментарий

### Параметры сценария

Общая строка параметра выглядит как `ws name ws = ws value ws ;` где вместо `value` может быть следующие типы данных:
- __ARRAY OF VALUE__: `[ ws value ws , ... ]`
- __CSTRING__: `" cstring "`
- __NUMBER__: `number`
_ __BOOL__: `TRUE` или `FALSE`


* `PATH_IN=["."];` - добавляет путь к поиску файлов
* `PATH_OUT=".ag47";` - указывает папку для сохранения файлов и отладочных данных, если папки не существует, то она будет создана.
* `RECREATE;` - если указан, то папка будет пересоздана, т.е. внутри всё почистится
* `NORECREATE;` - не пересоздаёт папку ( по умолчанию )
* `FORMAT_LAS=[".las"];` - указывает окончание для поиска _LAS_ файлов
* `FORMAT_INCL=[".doc",".txt",".docx"];` - указывает окончания для поиска файлов с инклинометрией
* `FORMAT_AR=[".zip",".rar"];` - формат файлов архивов
* `RUN_TREE;` - начинает обход всех файлов и запись их путей
* `RUN_PARSE;` - разбирает и анализирует все файлы



## Порядок обработки

* [x] - открыть файл сценария ( путь к нему передаётся через аргумент программы )
* [x] - пройтись по всем папкам, подпапкам и архивам из `PATH_IN` и записать все пути в файл _.ag47/\_1.tree.log_
* [x] - подсчитать количество необходимых файлов для проверки
* [x] - открывать каждый LAS файл и пытаться проанализировать его
* [x] - переписать каждое поле секций `~V`, `~W`, `~C`, `~P`, `~O` в файл _.ag47/\_2.sections.log_
* [ ] - определить номер скважины ( получить через поле `WELL` )
* [ ] - попытаться определить номер скважины жругими методами
* [ ] - достать параметры `STRT`, `STOP`, `STEP`, `NULL`
* [ ] - проверить поле `METD`
* [ ] - достать все названия методов из секции `~C`
* [ ] - записать все методы в файл _.ag47/las\_methods.log_
* [ ] - проанализировав секцию `~A`, получить граничные значения (`STRT`, `STOP`) для каждого метода
* [ ] - записать все зачения границ и методов а также номера скважины в файл _.ag47/las\_table.log_
* [x] - если выходной папки не существует, то создать её
* [ ] - скопировать все LAS файлы в выходную папку преписав в начале названия номер скважины
* [ ] - записать в файл _.ag47/las\_copy.log_ все данные о копировании
* [ ] - проверить выходные LAS файлы на дубликаты ( подобные методы с одинаковыми границами и шагом )
* [ ] - открывать каждый файл поподающий под подозрение с инклинометрией
* [ ] - поиск значения глубины начала и конца инклинометрии
* [ ] - копирование проблемных файлов в выходную подпапку _.ag47/errors/..._
* [ ] - запись всех ошибок и предупреждений в файл _.ag47/errors.log_
