import 'dart:io';
import 'dart:async';
import 'dart:isolate';



void runIsolate(SendPort sendPort) {
  int counter = 0;
  Timer.periodic(new Duration(milliseconds: 100), (Timer t) {
    counter++;
    String msg = 'notification ' + counter.toString();
    // stdout.write('SEND: ' + msg + ' - ');
    sendPort.send(msg);
  });
}


Future<void> main() async {

  print('spawning isolate...');
  ReceivePort receivePort = ReceivePort(); //port for this main isolate to receive messages.
  receivePort.listen((data) {
    print('RECEIVE: ' + data + ', ');
  });
  final isolate = Isolate.spawn(runIsolate, receivePort.sendPort);

  print('press enter key to quit...');
  // await stdin.first;

  // if (isolate != null) {
  //     stdout.writeln('killing isolate');
  //     isolate.kill(priority: Isolate.immediate);
  //     isolate = null;
  // }
  print('goodbye!');
}
