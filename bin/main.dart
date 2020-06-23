import 'dart:async';
import 'dart:collection';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import 'mapping.dart';

/// Данные лога
class LasMethodData {
  String mnem;
  String unit;
  String apiCode;
  String description;
  String strt;
  String stop;
  String step;
}

/// Разобранные данные
class LasFile {
  // ~V
  int vVersion;
  bool vWrap;
  // ~W
  String wStrt;
  String wStop;
  String wStep;
  String wNull;
  String wWell;
  String wDate;
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

/// Данные вспомогательных изолятов
class IsoSub {}

// Количество потоков
const isoCount = 3;

class IsoData {
  final int id; // Номер изолята (начиная с 1)
  final SendPort sendPort; // Порт для передачи данных главному изоляту
  final Map<String, List<String>> charMaps; // Таблицы кодировок

  IsoData(final int id, final SendPort sendPort, final charMaps)
      : id = id,
        sendPort = sendPort,
        charMaps = Map.unmodifiable(charMaps);
}

void runIsolate(final IsoData data) {
  final receivePort = ReceivePort(); // Порт прослушиваемый изолятом

  receivePort.listen((final msg) {
    // Прослушивание сообщений полученных от главного изолята
    if (msg is String) {
      // Сообщение о закрытии
      if (msg == '-e') {
        receivePort.close();
        print('sub[${data.id}]: end');
        data.sendPort.send([data.id, '-e']);
      }
    }
  });

  // Отправка данных для порта входящих сообщений
  print('sub[${data.id}]: sync main');
  data.sendPort.send([data.id, receivePort.sendPort]);
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
  // == Функции конец ==========================================================

  receivePort.listen((final data) {
    if (data is List) {
      // Передача порта для связи
      if (data[1] is SendPort) {
        isoTasks[data[0] - 1] = 0;
        isoSends[data[0] - 1] = data[1];
        print('main: sub[${data[0]}] synced');
        if (isoTasksZero()) {
          print('main: all subs synced');

          isoSendToAll('-e');
        }
      } else if (data[1] is String) {
        // Ответ на просьбу закрыться
        if (data[1] == '-e') {
          print('main: sub[${data[0]}] ended');
          isoTasks[data[0] - 1] = 0;
          if (isoTasksZero()) {
            print('main: all subs ended');
            receivePort.close();
          }
        }
      }
    }
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
