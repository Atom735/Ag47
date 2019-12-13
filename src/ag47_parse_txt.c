/*

Если строка совпадает с:
[\s]*Инклинометрия[\s]*
[\s]*Замер кривизны[\s]*
То это файл с инклинометрией и далее можем его начать разбирать


[\s]*Скважина[\s]*(?:N[\s]+)?([\w]+)

*/


static UINT rParse_Txt ( const LPCWSTR s4wPath, const LPCWSTR s4wOrigin, const LPCWSTR wszFileName )
{
  rLog ( L"Parse_TXT: %-256s ==> %-256s\n", s4wOrigin, s4wPath );
  return 0;
}
