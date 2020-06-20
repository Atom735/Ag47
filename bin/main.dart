import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import 'mapping.dart';

// Количество потоков
const isoCount = 3;

class IsoData {
  int id; // Номер изолята (начиная с 1)
  SendPort sendPortIn; // Порт через который изолят может отправлять сообщения
  SendPort sendPortOut; // Порт через который главный изолят посылает сообщения
  int jobs; // Количество задач в очереди, отслеживает главный изолят
  Map<String, List<String>> map; // Таблица кодировок
}

void runIsolate(IsoData data) {
  // Обработка файла
  void workFile(final File file) {
    file.readAsBytes().then((bytes) {
      final cp = getMappingMax(getMappingRaitings(data.map, bytes));
      final buffer = String.fromCharCodes(bytes
          .map((i) => i >= 0x80 ? data.map[cp][i - 0x80].codeUnitAt(0) : i));
      final lines = LineSplitter.split(buffer);
      var section = '';
      for (final line in lines) {
        final clearLine = line.trim();
        // Строку с комментарием пропускаем
        if (clearLine.startsWith('#')) {
          continue;
        }
        // Заголовок секции
        if (clearLine.startsWith('~')) {
          section = clearLine[1];
          continue;
        }
        switch (section) {
          case 'W':
            if (line.isNotEmpty) {
              final i0 = line.indexOf('.');
              if (i0 < 0) {
                assert(true);
              }
              final i1 = line.indexOf(' ', i0);
              if (i1 < 0) {
                assert(true);
              }
              final i2 = line.lastIndexOf(':');
              if (i2 < 0) {
                assert(true);
              }
            }
            break;
        }
      }

      data.sendPortIn.send('--job:${data.id}');
    });
  }

  final receivePort = ReceivePort(); // Порт прослушиваемый изолятом
  StreamSubscription<dynamic> subscriber; // Подписчик изолята
  subscriber = receivePort.listen((final msg) {
    // Прослушивание сообщений полученных от главного изолята
    if (msg is String) {
      if (msg == '--end') {
        print('[${data.id}]: end');
        subscriber.cancel();
        data.sendPortIn.send('--end:${data.id}');
      }
    } else if (msg is File) {
      workFile(msg);
    }
  });

  // Отправка данных для порта входящих сообщений
  data.sendPortOut = receivePort.sendPort;
  data.jobs = 0;
  print('[${data.id}]: sync send data');
  data.sendPortIn.send(data);
}

Future<void> main(List<String> args) async {
  final map = Map.unmodifiable(await loadMappings('mappings'));
  final iso = List<IsoData>.filled(isoCount, IsoData(), growable: false);
  final isolate = List<Future<Isolate>>(isoCount);

  final receivePort = ReceivePort(); // Порт прослушиваемый главным изолятом
  StreamSubscription<dynamic> subscriber;

  Directory('.ag-47').createSync(recursive: true);

  void endAll() {
    for (var i in iso) {
      print('main: send [${i.id}] end');
      i.sendPortOut.send('--end');
    }
  }

  void thisEnd() {
    print('main: end');
    subscriber.cancel();
  }

  void sendFile(final File file) {
    final path = file.path.toLowerCase();
    if (path.endsWith('.las')) {
      var j = 0;
      for (var i = 1; i < isoCount; i++) {
        if (iso[i].jobs < iso[j].jobs) {
          j = i;
        }
      }
      iso[j].jobs += 1;
      iso[j].sendPortOut.send(file);
    }
  }

  subscriber = receivePort.listen((data) {
    // Прослушивание посланий от изолятов
    if (data is IsoData) {
      // Передача данных для синхронизации
      print('main: [${data.id}] sync updated');
      iso[data.id - 1] = data;
      var b = true;
      for (final i in iso) {
        if (i.jobs != 0) {
          b = false;
        }
      }
      if (b) {
        // jobs = 0 для каждого изолята, значит всё готово для работы
        Directory('\\\\NAS\\Public\\common').list(recursive: true).listen((e) {
          if (e is File) {
            sendFile(e);
          }
        }, onDone: () {
          endAll();
        });
      }
    }
    if (data is String) {
      // Уведомление изолята о закрытии
      if (data.startsWith('--end:')) {
        iso[int.parse(data.substring(6)) - 1].id = 0;
        var b = true;
        for (final i in iso) {
          if (i.id != 0) {
            b = false;
          }
        }
        if (b) {
          // Все изоляты уведомили о закрытии и теперь можно закрыться самому
          thisEnd();
        }
      } else
      // Уведомление изолята о завершении обработки файла
      if (data.startsWith('--job:')) {
        iso[int.parse(data.substring(6)) - 1].jobs--;
      }
    }
  });

  // Создание изолятов
  for (var i = 0; i < isoCount; i++) {
    iso[i].map = Map.unmodifiable(map);
    iso[i].id = i + 1;
    iso[i].jobs = 1;
    iso[i].sendPortIn = receivePort.sendPort;
    print('main: spawn [${i + 1}]');
    isolate[i] =
        Isolate.spawn(runIsolate, iso[i], debugName: 'isolate[${i + 1}]');
  }
  // Ожидаем создания
  await Future.wait(isolate);
}
