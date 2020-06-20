import 'dart:convert';
import 'dart:io';
import 'dart:math';

/// Поиск
Future<Map<String, List<String>>> loadMappings(final String path) async {
  final map = <String, List<String>>{};
  await for (final e in Directory(path).list(recursive: false)) {
    if (e is File) {
      final name = e.path.substring(
          max(e.path.lastIndexOf('/'), e.path.lastIndexOf('\\')) + 1,
          e.path.lastIndexOf('.'));
      map[name] = List<String>(0x80);
      for (final line in await e.readAsLines(encoding: ascii)) {
        if (line.startsWith('#')) {
          continue;
        }
        final lineCeils = line.split('\t');
        if (lineCeils.length >= 2) {
          final i = int.parse(lineCeils[0]);
          if (i >= 0x80) {
            if (lineCeils[1].startsWith('0x')) {
              map[name][i - 0x80] =
                  String.fromCharCode(int.parse(lineCeils[1]));
            } else {
              map[name][i - 0x80] = '?';
            }
          }
        }
      }
    }
  }
  return map;
}
