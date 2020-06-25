import 'dart:io';

main(List<String> args) {
  final tasks = <Future>[];
  final dir = Directory('xlsx/test');
  if (dir.existsSync()) {
    dir.deleteSync(recursive: true);
  }
  dir.createSync(recursive: true);

  final fileXML_ContentTypes = File(dir.path + '/[Content_Types].xml');
  final fileXML_Rels = File(dir.path + '/_rels/.rels');
  final fileXML_docProps_app = File(dir.path + '/docProps/app.xml');
  final fileXML_docProps_core = File(dir.path + '/docProps/core.xml');

  tasks.add(fileXML_ContentTypes.create(recursive: true));
}
