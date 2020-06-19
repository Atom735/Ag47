import 'dart:convert';
import 'dart:io';
import 'dart:async';

import 'package:pedantic/pedantic.dart';


/*
  Las file => Stream<Raw>
  Stream<Raw> => get CodePage and LineFeed
  Stream<Raw> => Stream<String>
  Stream<String> => Parsed Las File
  Parsed Las File => Modifeded
  Parsed Las File => Set Data
*/


class Ag47Script {
  final List<String> paths;
  final String pathOut;
  IOSink outLog;
  List<Encoding> encodings;

  Ag47Script(this.paths, this.pathOut, this.outLog);

  Ag47Script.fromJson(Map<String, dynamic> json)
    : paths = List.unmodifiable(json['paths']),
      pathOut = ((json['out']??Directory.current.path)+'/.ag47_out')
    {
      Directory(pathOut).createSync(recursive: true);
      outLog = File(pathOut+'/log.txt').openWrite(mode: FileMode.writeOnly);
      encodings = [];
      encodings.add(Encoding.getByName('IBM855'));
      encodings.add(Encoding.getByName('2046'));
      encodings.add(Encoding.getByName('cp855'));
      encodings.add(Encoding.getByName('855'));
      encodings.add(Encoding.getByName('csIBM855'));

      encodings.add(Encoding.getByName('IBM866'));
      encodings.add(Encoding.getByName('windows-1251'));
      encodings.add(Encoding.getByName('KOI8-R'));
      encodings.add(Encoding.getByName('ISO-8859-5'));
    }

  Map<String, dynamic> toJson() =>
    {
      'paths': paths,
      'out': pathOut,
    };



  void parseFile(final File file)
  {
    outLog.writeln(file);
    if (file.path.toLowerCase().endsWith('.las')) {
      final streamInRaw = file.openRead();

    }
  }

  void done()
  {
    outLog.close();
    print('done!');
  }

  void run() {
    var idirscount = 0;
    for (final path in paths) {
      final dir = Directory(path);
      unawaited(dir.exists().then((ex)
      {
        ++idirscount;
        dir.list(recursive: true, followLinks: false).listen((entity) {
          if (entity is File) {
            parseFile(entity);
          }
        }, onDone: (){ if((--idirscount) == 0) done(); });
      } ));
    }
  }
}



Future<void> main(List<String> args) async {
  Ag47Script.fromJson(json.decode(File('script.json').readAsStringSync())).run();

  // for (var dirPath in d['dirs']) {
  //   Directory(dirPath).exists()
  //     .then((value) {
  //       if(value) {
  //         Directory(dirPath).list(recursive: true, followLinks: false)
  //     .listen((e) {
  //       if(e is File)
  //       {
  //         var file = e;
  //         final path = e.path.toLowerCase();
  //         if(path.endsWith('.las')) { parseLas(file); }
  //         else
  //         if(path.endsWith('.txt')) { parseTxt(file); }
  //         else
  //         if(path.endsWith('.doc')) { parseDoc(file); }
  //         else
  //         if(path.endsWith('.docx')) { parseDocx(file); }
  //         else
  //         if(path.endsWith('.dbf')) { parseDbf(file); }
  //       }
  //     });}});
  // }



  // Directory('\\\\NAS\\Public\\common')
  //   .list(recursive: true, followLinks: false)
  //     .listen((entity) {

  //     });
}
