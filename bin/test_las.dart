import 'dart:convert';
import 'dart:io';

import 'package:ag47/freq_2letters.dart';

const String pathIn = '\\\\NAS\\Public\\common\\Gilyazeev';

List<List<String>> mappings = [];

Map<String, List<String>> dbf = {};

void end() {
  dbf.forEach((key, value) {
    value.sort();
  });
  final f = File('__dbf.json');
  f.createSync(recursive: true);
  f.writeAsStringSync(json.encode(dbf));
}

Future<void> main(List<String> args) async {
  for (final e
      in Directory('mappings').listSync(recursive: true, followLinks: false)) {
    if (e is File) {
      final map = List<String>(0x80);
      for (final line in e.readAsLinesSync(encoding: ascii)) {
        if (line.startsWith('#')) {
          continue;
        }
        final lineCeils = line.split('\t');
        if (lineCeils.length >= 2) {
          final i = int.parse(lineCeils[0]);
          if (i >= 0x80) {
            if (lineCeils[1].startsWith('0x')) {
              map[i - 0x80] = String.fromCharCode(int.parse(lineCeils[1]));
            } else {
              map[i - 0x80] = '?';
            }
          }
        }
      }
      print('mappings[${mappings.length}]: ${e.path}');
      mappings.add(map);
    }
  }

  var lll = 0;
  var lle = false;

  await Directory(pathIn).list(recursive: true, followLinks: false).listen((e) {
    if (e is File && e.path.toLowerCase().endsWith('.las')) {
      ++lll;
      try {
        e.readAsBytes().then((bytes) {
          final r = List<int>.filled(mappings.length, 0, growable: false);
          var byteLast = 0;
          for (final byte in bytes) {
            if (byte >= 0x80 && byteLast >= 0x80) {
              for (var i = 0; i < mappings.length; i++) {
                r[i] += freq_2letters(
                    mappings[i][byteLast - 0x80] + mappings[i][byte - 0x80]);
              }
            }
            byteLast = byte;
          }
          print(
              '-----------------------------------------------------------------');
          print(e);
          var k = 0;
          for (var i = 0; i < mappings.length; i++) {
            print('Mapping[$i]: ${r[i]}');
            if (r[i] > r[k]) {
              k = i;
            }
          }
          final buffer = String.fromCharCodes(bytes
              .map((i) => i >= 0x80 ? mappings[k][i - 0x80].codeUnitAt(0) : i));
          var lines = LineSplitter.split(buffer);
          var section = '';
          for (var line in lines) {
            if (line.startsWith('#')) {
              continue;
            }
            if (line.startsWith('~')) {
              section = line[1];
              if (dbf['~' + line[1]] == null) {
                dbf['~' + line[1]] = [];
              }
              if (!dbf['~' + line[1]].contains(line.trim())) {
                dbf['~' + line[1]].add(line.trim());
              }
              if (section == 'A') {
                break;
              }
              continue;
            }
            if (line.trim().isEmpty) {
              continue;
            }
            try {
              final i0 = line.indexOf('.');
              final i1 = line.indexOf(' ', i0);
              final i2 = line.lastIndexOf(':');

              if (i0 < 0 || i1 < 0 || i2 < 0) {
                continue;
              }

              final name = line.substring(0, i0).trim();
              final val0 = line.substring(i0, i1).trim();
              final val1 = line.substring(i1, i2).trim();
              final val2 = line.substring(i2 + 1).trim();

              final key = section + '~' + name + '~';

              if (dbf[key + 'm'] == null) {
                dbf[key + 'm'] = [];
              }
              if (dbf[key + '1'] == null) {
                dbf[key + '1'] = [];
              }
              if (dbf[key + '2'] == null) {
                dbf[key + '2'] = [];
              }
              if (!dbf[key + 'm'].contains(val0)) {
                dbf[key + 'm'].add(val0);
              }
              if (!dbf[key + '1'].contains(val1)) {
                dbf[key + '1'].add(val1);
              }
              if (!dbf[key + '2'].contains(val2)) {
                dbf[key + '2'].add(val2);
              }
            } catch (e) {
              print(e);
            }
          }
          --lll;
          if (lll == 0 && lle) {
            end();
          }
        });
      } catch (err) {
        print(err);
      }
    }
  }, onDone: () {
    lle = true;
    if (lll == 0) {
      end();
    }
  });
}
