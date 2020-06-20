import 'dart:async';
import 'dart:isolate';

import 'mapping.dart';

class IsoData {
  int id;
  SendPort sendPortIn; // Порт через который изолят может отправлять сообщения
  SendPort sendPortOut; // Порт через который главный изолят посылает сообщения
  int jobs;
  Map<String, List<String>> map;
}

void runIsolate(IsoData data) {
  final receivePort = ReceivePort(); // Порт прослушиваемый изолятом
  StreamSubscription<dynamic> subscriber; // Подписчик изолята
  subscriber = receivePort.listen((msg) {
    if (msg is String) {
      if (msg == '--end') {
        print('[${data.id}]: end');
        subscriber.cancel();
        subscriber = null;
      }
    }
  });

  data.sendPortOut = receivePort.sendPort;
  data.jobs = 0;
  print('[${data.id}]: send data');
  data.sendPortIn.send(data);
}

const isoCount = 3;
Future<void> main(List<String> args) async {
  final map = Map.unmodifiable(await loadMappings('mappings'));
  final iso = List<IsoData>.filled(isoCount, IsoData(), growable: false);
  final isolate = List<Future<Isolate>>(isoCount);

  final receivePort = ReceivePort(); // Порт прослушиваемый главным изолятом
  StreamSubscription<dynamic> subscriber;
  subscriber = receivePort.listen((data) {
    if (data is IsoData) {
      print('main: [${data.id}] updated');
      iso[data.id - 1] = data;
      var b = true;
      for (var i in iso) {
        if (i.jobs != 0) {
          b = false;
        }
      }
      if (b) {
        for (var i in iso) {
          print('main: send [${i.id}] end');
          i.sendPortOut.send('--end');
        }
        subscriber.cancel();
        Future.wait(isolate);
        print('main: end');
      }
    }
  });

  for (var i = 0; i < isoCount; i++) {
    iso[i].map = map;
    iso[i].id = i + 1;
    iso[i].jobs = 1;
    iso[i].sendPortIn = receivePort.sendPort;
    print('main: spawn [${i + 1}]');
    isolate[i] =
        Isolate.spawn(runIsolate, iso[i], debugName: 'isolate[${i + 1}]');
  }
}
