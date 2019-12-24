/*
  https://www.dbf2002.com/dbf-file-format.html
  http://www.autopark.ru/ASBProgrammerGuide/DBFSTRUC.HTM
  https://www.clicketyclick.dk/databases/xbase/format/dbf.html
  http://web.archive.org/web/20150323061445/http://ulisse.elettra.trieste.it:80/services/doc/dbase/DBFstruct.htm
*/
struct dbf_file_header
{
  /* 0 */
  UINT8                 iVersion;                           // Сигнатура
  UINT8                 nLastUpdateYY;                      // Дата последней модификации в виде ГГММДД
  UINT8                 nLastUpdateMM;
  UINT8                 nLastUpdateDD;
  /* 4 */
  UINT32                nNumberOfRecords;                   // Число записей в базе
  /* 8 */
  UINT16                nLengthOfHeaderStruct;              // Полная длина заголовка (с дескрипторами полей)
  UINT16                nLengthOfEachRecord;                // Длина одной записи
  /* 12 */
  UINT8                 _R0[2];                             // Зарезервировано (всегда 0)
  /* 14 */
  UINT8                 iFlagIncompleteTransac;             // Флаг, указывающий на наличие незавершенной транзакции dBASE IV
  UINT8                 iFlagEncryption;                    // Флаг шифрования таблицы dBASE IV
  /* 16 */
  UINT8                 aFreeRecordThread[4];               // Free record thread (reserved for LAN only )
  UINT8                 aMultiUser[8];                      // Зарезервированная область для многопользовательского использования
  /* 28 */
  UINT8                 iFlagMDX;                           // Флаг наличия индексного MDX-файла
  UINT8                 iCodePage;                          // Идентификатор кодовой страницы файла (dBASE IV, Visual FoxPro, XBase)
  UINT8                 _R1[2];                             // Зарезервировано (всегда 0)
  /* 32 */
};

struct dbf_file_subrecord
{
  /* 0 */
  UINT8                 sName[11];                          // Имя поля ASCII (terminated by 0)
  UINT8                 iType;                              // Тип поля ACII
  /* 12 */
  UINT32                nAddress;                           // Адрес данных поля (ссылка на память, а не на диск) (in memory !!!)
  /* 16 */
  UINT8                 nLength;                            // Полная длина поля
  UINT8                 nDecimalCount;                      // Число десятичных разрядов; для типа C - второй байт длины поля
  /* 18 */
  UINT8                 aMultiUser[2];
  /* 20 */
  UINT8                 iWordAreaID;
  UINT8                 aMultiUser2[2];
  UINT8                 iFlagSetFields;
  /* 24 */
  UINT8                 _R0[7];
  UINT8                 iFlagIndex;
  /* 32 */
};

/*
*/

LPCSTR g7Str_DBF_Sinature[0x100] =
{
  #define _D7x(_n_,_s_) [_n_]=_s_,
  // Value  Description
  _D7x(0x02,"FoxBase")
  _D7x(0x03,"File without DBT")
  _D7x(0x04,"dBASE IV w/o memo file")
  _D7x(0x05,"dBASE V w/o memo file")
  _D7x(0x07,"VISUAL OBJECTS (first 1.0 versions) for the Dbase III files w/o memo file")
  _D7x(0x30,"Visual FoxPro")
  _D7x(0x30,"Visual FoxPro w. DBC")
  _D7x(0x31,"Visual FoxPro w. AutoIncrement field")
  _D7x(0x43,".dbv memo var size (Flagship)")
  _D7x(0x7B,"dBASE IV with memo")
  _D7x(0x83,"dBASE III+ with memo file")
  _D7x(0x87,"VISUAL OBJECTS (first 1.0 versions) for the Dbase III files (NTX clipper driver) with memo file")
  _D7x(0x8B,"dBASE IV w. memo")
  _D7x(0x8E,"dBASE IV w. SQL table")
  _D7x(0xB3,".dbv and .dbt memo (Flagship)")
  _D7x(0xE5,"Clipper SIX driver w. SMT memo file.")
  _D7x(0xF5,"FoxPro w. memo file")
  _D7x(0xFB,"FoxPro ???")
  #undef _D7x
};
LPCSTR g7Str_DBF_FieldType[0x100] =
{
  #define _D7x(_n_,_s_) [_n_]=_s_,
  _D7x('C',"Characte")
  _D7x('N',"Number")
  _D7x('L',"Logical")
  _D7x('D',"Date")
  _D7x('M',"Memo")
  _D7x('F',"Floating point")
  _D7x('B',"Binary")
  _D7x('G',"General")
  _D7x('P',"Picture")
  _D7x('Y',"Currency")
  _D7x('T',"DateTime")
  _D7x('I',"Integer")
  _D7x('V',"VariField")
  _D7x('X',"Variant (X) for compatibility with SQL-s (i.e. varChar).")
  _D7x('@',"Timestamp")
  _D7x('O',"Double")
  _D7x('+',"Autoincrement")
  #undef _D7x
};

/*
  Проверить файл на корректность DBF
*/
static BOOL rSignatureMem_DBF ( BYTE const * p, UINT n )
{
  // Версия файла DBF
  if ( g7Str_DBF_Sinature[*p] == NULL ) return FALSE;
  return TRUE;
}


static UINT rParse_DBF_Begin ( struct docx_state_ink * const p, struct file_map const * const fm )
{
  struct file_data_ptr fdp = { .p = fm->pData, .n = fm->nSize, .nLine = 1 };
  struct dbf_file_header _head;
  memcpy ( &_head, fdp.p, sizeof(_head) );
  rFileData_Skip ( &fdp, sizeof(_head) );
  fwprintf ( p->pF_log, L"DBF Версия:\t%hs\r\n", g7Str_DBF_Sinature[_head.iVersion] );
  fwprintf ( p->pF_log, L"DBF Дата обновления:\t%u/%u/%u\r\n", _head.nLastUpdateYY+1900, _head.nLastUpdateMM, _head.nLastUpdateDD );
  fwprintf ( p->pF_log, L"DBF Количество записей:\t%u\r\n", _head.nNumberOfRecords );;
  fwprintf ( p->pF_log, L"DBF Длина заголовка:\t%u (%u)\r\n", _head.nLengthOfHeaderStruct, sizeof(_head) );
  fwprintf ( p->pF_log, L"DBF Длина одной записи:\t%u\r\n", _head.nLengthOfEachRecord );
  fwprintf ( p->pF_log, L"DBF Резервныйх два байта:\t0x%02x 0x%02x\r\n", _head._R0[0], _head._R0[1] );
  fwprintf ( p->pF_log, L"DBF Наличие незавершенной транзакции:\t0x%02x\r\n", _head.iFlagIncompleteTransac );
  fwprintf ( p->pF_log, L"DBF Шифрование:\t0x%02x\r\n", _head.iFlagEncryption );
  fwprintf ( p->pF_log, L"DBF Free record thread (reserved for LAN only ):\t0x%02x 0x%02x 0x%02x 0x%02x\r\n",
          _head.aFreeRecordThread[0],
          _head.aFreeRecordThread[1],
          _head.aFreeRecordThread[2],
          _head.aFreeRecordThread[3] );
  fwprintf ( p->pF_log, L"DBF Зарезервированная область для многопользовательского использования:\t0x%02x 0x%02x 0x%02x 0x%02x\r\n",
          _head.aMultiUser[0],
          _head.aMultiUser[1],
          _head.aMultiUser[2],
          _head.aMultiUser[3] );
  fwprintf ( p->pF_log, L"DBF Зарезервированная область для многопользовательского использования:\t0x%02x 0x%02x 0x%02x 0x%02x\r\n",
          _head.aMultiUser[0+4],
          _head.aMultiUser[1+4],
          _head.aMultiUser[2+4],
          _head.aMultiUser[3+4] );
  fwprintf ( p->pF_log, L"DBF Наличие индексного MDX-файла:\t0x%02x\r\n", _head.iFlagMDX );
  fwprintf ( p->pF_log, L"DBF Идентификатор кодовой страницы файла:\t0x%02x (%u)\r\n", _head.iCodePage, _head.iCodePage );
  fwprintf ( p->pF_log, L"DBF Резервныйх два байта:\t0x%02x 0x%02x\r\n", _head._R1[0], _head._R1[1] );

  struct dbf_file_subrecord * pFields = r4_malloc_s4s(16,sizeof(struct dbf_file_subrecord));
  while ( *fdp.p != 0x0D )
  {
    struct dbf_file_subrecord sr = { };
    memcpy ( &sr, fdp.p, sizeof(sr) );
    rFileData_Skip ( &fdp, sizeof(sr) );
    pFields = r4_add_array_s4s ( pFields, &sr, 1 );
    fwprintf ( p->pF_log, L"DBF [%u] ==> Поле [%u] <==\r\n", r4_get_count_s4s(pFields), r4_get_count_s4s(pFields) );
    fwprintf ( p->pF_log, L"DBF      Имя поля: %hs\r\n", sr.sName );
    fwprintf ( p->pF_log, L"DBF      Тип поля: \'%hc\'(%u) - %hs\r\n", sr.iType, sr.iType, g7Str_DBF_FieldType[sr.iType] );
    fwprintf ( p->pF_log, L"DBF      Адрес в памяти: %p\r\n", (VOID*)sr.nAddress );
    fwprintf ( p->pF_log, L"DBF      Длина поля: %u\r\n", sr.nLength );
    fwprintf ( p->pF_log, L"DBF      Число десятичных разрядов: %u\r\n", sr.nDecimalCount );
    fwprintf ( p->pF_log, L"DBF      aMultiUser:\t0x%02x 0x%02x\r\n",
          sr.aMultiUser[0],
          sr.aMultiUser[1] );
    fwprintf ( p->pF_log, L"DBF      iWordAreaID: %u\r\n", sr.iWordAreaID );
    fwprintf ( p->pF_log, L"DBF      aMultiUser:\t0x%02x 0x%02x\r\n",
          sr.aMultiUser2[0],
          sr.aMultiUser2[1] );
    fwprintf ( p->pF_log, L"DBF      Flag for SET FIELDS:\t0x%02x\r\n", sr.iFlagSetFields );
    fwprintf ( p->pF_log, L"DBF      Резервныйх 7 байт:\t0x%02x 0x%02x 0x%02x 0x%02x\r\n", _head._R0[0], _head._R0[1], _head._R0[2], _head._R0[3] );
    fwprintf ( p->pF_log, L"DBF                        \t0x%02x 0x%02x 0x%02x\r\n", _head._R0[4], _head._R0[5], _head._R0[6] );
    fwprintf ( p->pF_log, L"DBF      iFlagIndex:\t0x%02x\r\n", sr.iFlagIndex );
  }
  rFileData_Skip ( &fdp, 1 );


  fwprintf ( p->pF_log, L"DBF === Таблица данных ===\r\n" );

  fwprintf ( p->pF_log, L"DEL" );
  D4ForAll_s4s ( pFields, j, N1 )
  {
    fwprintf ( p->pF_log, L" | (%hc) %-*hs", pFields[j].iType, pFields[j].nLength, pFields[j].sName );
  }
  fwprintf ( p->pF_log, L" |\r\n" );
  fwprintf ( p->pF_log, L"---" );
  D4ForAll_s4s ( pFields, j, N2 )
  {
    fwprintf ( p->pF_log, L"-|-----%-.*hs", pFields[j].nLength, "------------------------" );
  }
  fwprintf ( p->pF_log, L"-|\r\n" );


  for ( UINT i = 0; i < _head.nNumberOfRecords; ++i )
  {
    if ( fdp.p[0] == ' ' ) { fwprintf ( p->pF_log, L"   " ); }
    else
    if ( fdp.p[0] == '*' ) { fwprintf ( p->pF_log, L" x " ); }
    else
    { fwprintf ( p->pF_log, L">?<" ); }
    D4ForAll_s4s ( pFields, j, N3 )
    {
      WCHAR wsz[64];
      fwprintf ( p->pF_log, L" | %-*.*hs    ", pFields[j].nLength, pFields[j].nLength, fdp.p+pFields[j].nAddress );
    }
    fwprintf ( p->pF_log, L" |\r\n" );

    rFileData_Skip ( &fdp, _head.nLengthOfEachRecord );
  }

  r4_free_s4s ( pFields );
  return 0;
}

static UINT rParse_DBF ( struct ag47_script * const script, const LPCWSTR s4wPath, const LPCWSTR wszFileName )
{
  rLog ( L"Parse_DBF: %-256s ==> %-256s\n", script->s4wOrigin, s4wPath );
  struct file_map fm;
  UINT iErr = 0;
  if ( ( iErr = rFS_FileMapOpen ( &fm, s4wPath ) ) ) goto P_End2;

  const LPWSTR s4w1 = r4_alloca_s4w(kPathMax);
  r4_push_path_s4w_s4w ( s4w1, script->s4wPathOutLogsDir );
  UINT i = 0;
  for ( i = 0; i <= 999; ++i )
  {
    swprintf ( s4w1+r4_get_count_s4w(s4w1), kPathMax-r4_get_count_s4w(s4w1),
            L"\\%s.[%03u].log", wszFileName, i );
    FILE * const pF_log = _wfopen ( s4w1, L"rb" );
    if ( pF_log ) { fclose ( pF_log ); } else { break; }
  }


    const LPWSTR s4w3 = r4_alloca_s4w(kPathMax);
    r4_push_path_s4w_s4w ( s4w3, script->s4wPathOutLogsDir );
    swprintf ( s4w3+r4_get_count_s4w(s4w3), kPathMax-r4_get_count_s4w(s4w3),
            L"\\%s.[%03u].txt", wszFileName, i );

  struct docx_state_ink _ = {
    .pF_log = rOpenFileToWriteWith_UTF16_BOM ( s4w1 ),
    .pF_log2 = rOpenFileToWriteWith_UTF16_BOM ( s4w3 ),
    .s4wOrigin = script->s4wOrigin,
    .wszFileName = wszFileName,
    .d = 0,
    .iS = kD74_Null,
    .s4w = r4_alloca_s4w(kPathMax),
  };
  UINT a1[g7CharMapCount], a2[g7CharMapCount];
  _.iCodePage           = rGetBufCodePage ( fm.pData, fm.nSize, a1, a2 );
  _.iLineFeed           = rGetBufEndOfLine ( fm.pData, fm.nSize );
  setlocale ( LC_ALL, g7CharMapCP[_.iCodePage] );

  if ( ( iErr = rParse_DBF_Begin ( &_, &fm ) ) ) { goto P_End; }


  P_End:
  fclose ( _.pF_log2 );
  fclose ( _.pF_log );

  rFS_FileMapClose ( &fm );
  P_End2:
  return iErr;
}
