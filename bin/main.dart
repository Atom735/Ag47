import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import 'package:test/test.dart';

import 'mapping.dart';

/// Данные лога
class LasMethodData {
  String mnem;
  String unit;
  String data;
  String desc;
  String strt;
  double strtN;
  String stop;
  double stopN;
  String step;
  double stepN;
}

/// Разобранные данные
class LasFile {
  // ~W
  String wStrt;
  double wStrtNum;
  String wStop;
  double wStopNum;
  String wStep;
  double wStepNum;
  String wNull;
  double wNullNum;
  String wWell;
  // ~C
  List<LasMethodData> methods;
}

/**
 * Главный изолят:
 * * Обходит всё дерево и все файлы
 * * Вскрывает архивы с помощью 7zip и обходит их
 * * Передаёт пути к файлам вспомогательным изолятам для разбора
 * * Создаёт конечную таблицу
 *
 * Вспомогательный изолят:
 * * Обрабатывает файлы (сам пытается понять что за файл)
 * * Las: файлы с методами ГИС
 * * * Подбирает кодировку
 * * * Разбирает и возвращает данные
 * * Inc(txt): файлы инклинометрии (текстовый)
 * * * Подбирает кодировку
 * * * Разбирает и возвращает данные
 * * Inc(doc): файлы инклинометрии (бинарный Word)
 * * * Транслирует в docx и обрабатывает его
 * * * Удаляет временный docx файл
 * * Inc(docx): файлы инклинометрии (современный Word)
 * * * Разархивировать его во временную папку
 * * * Разобрать xml файл с данными
 * * * Вернуть обработанные данные
 * * * Удалить временную папку
 * * Inc(dbf): файлы инклинометрии (База данных)
 * * * Разбирает dbf файл и возвращает данные
 * *
 * Взаимодействие между изолятами
 * Данные отправляемые вспомогательным изолятам:
 *
 * -e
 * просьба закончить работу
 *
 * {{path}}
 * просьба разобрать файл (увеличивает таск)
 *
 * Данные отправляемые главному изоляту:
 * [id, ...]
 *
 * -e
 * вспомогательный изолят закончил свою работу, ответ на просьбу закончить работу
 *
 * */

class IsoData {
  final int id; // Номер изолята (начиная с 1)
  final SendPort sendPort; // Порт для передачи данных главному изоляту
  final Map<String, List<String>> charMaps; // Таблицы кодировок

  int iErrors;
  IOSink fErrors;

  IsoData(final int id, final SendPort sendPort, final charMaps)
      : id = id,
        sendPort = sendPort,
        charMaps = Map.unmodifiable(charMaps);
}

Future<void> parseLas(final IsoData iso, final File file) async {
  // Считываем данные файла (Асинхронно)
  final bytes = await file.readAsBytes();
  // Подбираем кодировку
  final cp = getMappingMax(getMappingRaitings(iso.charMaps, bytes));
  final buffer = String.fromCharCodes(bytes
      .map((i) => i >= 0x80 ? iso.charMaps[cp][i - 0x80].codeUnitAt(0) : i));
  // Нарезаем на линии
  final lines = LineSplitter.split(buffer);
  var lineNum = 0;
  final las = LasFile();
  las.methods = [];
  final reLasLine = RegExp(r'^([^.]+)\.([^\s]*)\s+(.*):(.*)$');
  final reLasLineA = RegExp(r'[+-]?\d+(\.\d+)?');

  var section = '';

  for (final lineFull in lines) {
    lineNum += 1;
    final line = lineFull.trim();
    if (line.isEmpty || line.startsWith('#')) {
      // Пустую строку и строк с комментарием пропускаем
      continue;
    } else if (section == 'A') {
      var i = 0;
      Future<File> fErr;

      reLasLineA.allMatches(line).forEach((e) {
        final numStr = e.group(0);
        final n = double.parse(numStr);

        if (fErr != null) {
          return;
        }
        if (i >= las.methods.length) {
          // throw 'Всё, ужасно не правильно!';
          iso.iErrors += 1;
          iso.fErrors.writeln(file);
          final newPath = '.ag-47/errors/${iso.id}/${iso.iErrors}.las';
          iso.fErrors.writeln('\t$newPath');
          iso.fErrors.writeln('\tОшибка в строке: $lineNum');
          fErr = file.copy(newPath);
          iso.sendPort.send([iso.id, '-las']);
          return;
        }
        if (numStr != las.wNull) {
          if (las.methods[i].strt == null) {
            las.methods[i].strt = numStr;
            las.methods[i].strtN = n;
          }
          las.methods[i].stop = numStr;
          las.methods[i].stopN = n;
        }
        i += 1;
      });
      if (fErr != null) {
        await fErr;
        return;
      }
      continue;
    } else if (line.startsWith('~')) {
      // Заголовок секции
      section = line[1];
      continue;
    } else {
      final f = reLasLine.firstMatch(line);
      if (f == null) {
        // |==<??>==<??>==|
        // throw 'LAS($lineNum): Непредвиденная строка';
        iso.iErrors += 1;
        iso.fErrors.writeln(file);
        final newPath = '.ag-47/errors/${iso.id}/${iso.iErrors}.las';
        iso.fErrors.writeln('\t$newPath');
        iso.fErrors.writeln('\tОшибка в строке: $lineNum');
        await file.copy(newPath);
        iso.sendPort.send([iso.id, '-las']);
        return;
      }
      final mnem = f.group(1).trim();
      final unit = f.group(2).trim();
      final data = f.group(3).trim();
      final desc = f.group(4).trim();
      switch (section) {
        case 'W':
          if (mnem == 'WELL') {
            las.wWell = data;
          }
          if (mnem == 'STRT') {
            las.wStrt = data;
            las.wStrtNum = double.parse(data);
          }
          if (mnem == 'STOP') {
            las.wStop = data;
            las.wStopNum = double.parse(data);
          }
          if (mnem == 'STEP') {
            las.wStep = data;
            las.wStepNum = double.parse(data);
          }
          if (mnem == 'NULL') {
            las.wNull = data;
            las.wNullNum = double.parse(data);
          }
          break;
        case 'C':
          final lmd = LasMethodData();
          lmd.mnem = mnem;
          lmd.unit = unit;
          lmd.data = data;
          lmd.desc = desc;
          lmd.strtN = null;
          // lmd.step = las.wStep;
          las.methods.add(lmd);
          break;
      }
    }
  }

  /// ==========================================================================
  /// TODO:
  // sleep(Duration(milliseconds: 30));
  iso.sendPort.send([iso.id, '+las', las]);
  return;
}

void runIsolate(final IsoData iso) {
  // ===========================================================================
  final receivePort = ReceivePort(); // Порт прослушиваемый изолятом
  var futures = <Future>[];

  Directory('.ag-47/errors/${iso.id}').createSync(recursive: true);
  iso.iErrors = 0;
  iso.fErrors = File('.ag-47/errors/${iso.id}/__.txt')
      .openWrite(mode: FileMode.writeOnly, encoding: utf8);

  receivePort.listen((final msg) {
    // Прослушивание сообщений полученных от главного изолята
    if (msg is String) {
      // Сообщение о закрытии
      if (msg == '-e') {
        Future.wait(futures).then((final v) {
          sleep(Duration(milliseconds: 3000));
          iso.fErrors.flush().then((final v) {
            print('sub[${iso.id}]: $v');
            iso.fErrors.close().then((final v) {
              print('sub[${iso.id}]: $v');
              print('sub[${iso.id}]: end');
              iso.sendPort.send([iso.id, '-e']);
            });
          });
        });
        receivePort.close();
        return;
      }
    } else if (msg is File) {
      if (msg.path.toLowerCase().endsWith('.las')) {
        futures.add(parseLas(iso, msg));
        return;
      }
    }
    print('sub[${iso.id}]: recieved unknown msg {$msg}');
    // sleep(Duration(milliseconds: 300));
  });

  // Отправка данных для порта входящих сообщений
  print('sub[${iso.id}]: sync main');
  iso.sendPort.send([iso.id, receivePort.sendPort]);
  return;
}

Future<void> main(List<String> args) async {
  // Таблицы кодировок
  final charMaps = Map.unmodifiable(await loadMappings('mappings'));
  // Количество потоков
  const isoCount = 3;
  // Порты для передачи данных вспомогательным изолятам
  final isoSends = List<SendPort>(isoCount);
  // Количество задач у каждого вспомогательного изолята в очереди
  final isoTasks = List<int>.filled(isoCount, 0);
  final isolate = List<Future<Isolate>>(isoCount);
  // Порт прослушиваемый главным изолятом
  final receivePort = ReceivePort();
  // Лас файлы
  final tableLas = <LasFile>[];

  Directory('.ag-47').createSync(recursive: true);
  // == Функции начало =========================================================
  bool isoTasksZero() {
    for (final i in isoTasks) {
      if (i != 0) {
        return false;
      }
    }
    return true;
  }

  void isoSendToAll(final msg) {
    for (var i = 0; i < isoCount; i++) {
      print('main: send to sub[${i + 1}] "$msg"');
      isoTasks[i] += 1;
      isoSends[i].send(msg);
    }
  }

  void isoSendToIdle(final msg) {
    var j = 0;
    for (var i = 0; i < isoCount; i++) {
      if (isoTasks[i] < isoTasks[j]) {
        j = i;
      }
    }
    isoTasks[j] += 1;
    isoSends[j].send(msg);
  }

  // == Функции конец ==========================================================

  receivePort.listen((final data) {
    if (data is List) {
      if (data[1] is SendPort) {
        // Передача порта для связи
        isoTasks[data[0] - 1] = 0;
        isoSends[data[0] - 1] = data[1];
        print('main: sub[${data[0]}] synced');
        if (isoTasksZero()) {
          print('main: all subs synced');

          Directory(r'\\NAS\Public\common\Gilyazeev\ГИС\Искринское м-е')
              .list(recursive: true, followLinks: false)
              .listen((final e) {
            if (e is File) {
              if (e.path.toLowerCase().endsWith('.las')) {
                isoSendToIdle(e);
              }
            }
          }, onDone: () {
            isoSendToAll('-e');
          });
        }
        return;
      } else if (data[1] is String) {
        if (data[1] == '-e') {
          // Ответ на просьбу закрыться
          print('main: sub[${data[0]}] ended');
          if (isoTasks[data[0] - 1] != 1) {
            print('ERROR: sub[${data[0]}] have ${isoTasks[data[0] - 1]} tasks');
          }

          isoTasks[data[0] - 1] = 0;
          if (isoTasksZero()) {
            print('main: all subs ended');
            receivePort.close();
          }
          return;
        } else if (data[1] == '+las') {
          tableLas.add(data[2]);
          isoTasks[data[0] - 1] -= 1;
          print('main: sub[${data[0]}] have ${isoTasks[data[0] - 1]} tasks');
          return;
        } else if (data[1] == '-las') {
          isoTasks[data[0] - 1] -= 1;
          print('main: sub[${data[0]}] have ${isoTasks[data[0] - 1]} tasks');
          return;
        }
      }
    }

    print('main: recieved unknown msg {$data}');
  });

  // Создание изолятов
  for (var i = 0; i < isoCount; i++) {
    final data = IsoData(i + 1, receivePort.sendPort, charMaps);
    isoTasks[i] = -1;
    print('main: spawn sub[${i + 1}]');
    isolate[i] = Isolate.spawn(runIsolate, data, debugName: 'sub[${i + 1}]');
  }
  // Ожидаем создания
  await Future.wait(isolate);
  print('main: all subs spawned');
}
