import 'dart:convert';
import 'dart:io';
import 'dart:math';

import 'dart:typed_data';

import 'package:dart_test/freq_2letters.dart';

/// Поиск кодировок
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
      map[name] = List.unmodifiable(map[name]);
    }
  }
  return map;
}

/// Возвращает актуальность той или иной кодировки
Map<String, int> getMappingRaitings(
    final Map<String, List<String>> map, final Uint8List bytes) {
  final r = <String, int>{};
  map.forEach((k, v) {
    r[k] = 0;
  });
  var byteLast = 0;
  for (final byte in bytes) {
    if (byte >= 0x80 && byteLast >= 0x80) {
      // map.forEach(k,v) {
      //   // r[i] += freq_2letters(map[i][byteLast - 0x80] + map[i][byte - 0x80]);
      // }
      map.forEach((final k, final v) {
        r[k] += freq_2letters(v[byteLast - 0x80] + v[byte - 0x80]);
      });
    }
    byteLast = byte;
  }
  return r;
}

/// Возвращает актуальную кодировку
String getMappingMax(final Map<String, int> r) {
  var o = '';
  r.forEach((final k, final v) {
    if (r[o] == null) {
      o = k;
    } else if (r[o] < v) {
      o = k;
    }
  });
  return o;
}
