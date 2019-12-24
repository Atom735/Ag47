/*

  База данных:
  [^] CODEPAGE    -             [iCP]        -              [ORIGIN] [LINE]
  [^] NEWLINE     -             [iCP]        -              [ORIGIN] [LINE]
  [^] CODEPAGE_x  -             [iCP[0]]     -              [ORIGIN] [LINE]
  [~] [SECTION]   -             [F_LINE]     -              [ORIGIN] [LINE]
  [#] -           -             [F_LINE]     -              [ORIGIN] [LINE] // Коментарий
  [*] -           -             [F_LINE]     -              [ORIGIN] [LINE] // Строка
  [V] [MNEM]      [UNITS]       [DATA]      [DESC]          [ORIGIN] [LINE]
  [W] [MNEM]      [UNITS]       [DATA]      [DESC]          [ORIGIN] [LINE]
  [C] [MNEM]      [UNITS]       [DATA]      [DESC]          [ORIGIN] [LINE]
  [P] [MNEM]      [UNITS]       [DATA]      [DESC]          [ORIGIN] [LINE]
  [O] -           -             [F_LINE]     -              [ORIGIN] [LINE]
  L"SECTION\tMNEM\tUNITS\tDATA\tDESC\tFILE_PATH\tLINE\r\n"
*/

struct las_line_data
{
  BYTE iSection;
  struct file_data_ptr
          aMNEM, aUNIT, aDATA, aDESC, aLine;
};

struct las_data_s
{
  struct file_data_ptr
          aMNEM, aUNIT, aDATA, aDESC, aLine;
  union {
      double d;
      INT i;
      UINT u;
      BOOL b;
  };
};

struct las_data_a
{
  struct mem_ptr_txt s;
  double fSTRT, fSTOP;
};

struct file_state_las
{
  struct file_map       fm;
  struct ag47_script  * script;
  LPCWSTR               wszFileName;    // Название файла
  LPWSTR                s4wTempOut;     // Путь к временному файлу
  UINT                  iCP[2];         // Кодировка
  UINT                  iErr;           // Номер ошибки
  BYTE                  iSection;       // В какой секции сейчас находимся
  struct mem_ptr_txt    s,              // Указатель обработки файла
      aMNEM, aUNIT, aDATA, aDESC, aLine;// Переменные обработки линии

  struct mem_ptr_txt    w_WELL;
  double                w_STRT, w_STOP, w_STEP, w_NULL;

  struct las_data_a   * aCurves;

  UINT                  iVersion;       // Версия LAS файла
  BOOL                  bWrap;          // Перенос данных

#if 0
  LPCWSTR               s4wOrigin;      // Путь к оригиналу файла
  UINT                  iCodePage;      // Номер кодировки
  UINT                  iLineFeed;      // Символ перехода на новую строку
  UINT                  nWarnings;      // Количество предупреждений
  double                fErr;           // Поправка на совпадение
  struct las_line_data *pArray;         // Все линии файла
  struct file_data_ptr  t,              // Обработка файла
          aMNEM, aUNIT, aDATA, aDESC, aLine; // Переменные обработки линии
  struct {
    struct file_data_ptr _;
    union {
      double d;
      INT i;
      UINT u;
      BOOL b;
    };
  }       v_VERS, v_WRAP,
          w_STRT, w_STOP, w_STEP, w_NULL,
          w_WELL;
  struct las_c_data    *pA_C;           // Все линии секции C
#endif
};

static VOID rLas_LogToDB ( struct file_state_las * const las, LPCWSTR const fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  vfwprintf ( las->script->pFLasDB, fmt, args );
  va_end ( args );
}



#if 0

static UINT rParse_Las_Log_v ( const LPCWSTR fmt, va_list args )
{
  static FILE * pF = NULL;
  if ( !fmt )
  {
    if ( pF ) { fclose ( pF ); pF = NULL; return 0; }
    return 0;
  }
  if ( !pF ) { pF = rOpenFileToWriteWith_UTF16_BOM ( L".ag47/log/LAS.log" ); }
  return vfwprintf ( pF, fmt, args );
}

static UINT rParse_Las_Log ( const LPCWSTR fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  UINT i = rParse_Las_Log_v ( fmt, args );
  va_end ( args );
  return i;
}









INT _rParse_Las_ArraySortCMP ( struct las_line_data const * const p1, struct las_line_data const * const p2 )
{
  if ( p1->iSection < p2->iSection) { return -1; }
  if ( p1->iSection > p2->iSection) { return +1; }
  LPCSTR q1 = (LPCSTR)(p1->aMNEM.p);
  LPCSTR q2 = (LPCSTR)(p2->aMNEM.p);
  UINT n1 = p1->aMNEM.n;
  UINT n2 = p2->aMNEM.n;
  while ( n1 && n2 )
  {
    if ( tolower(*q1) == tolower(*q2) ) { ++q1; ++q2; --n1; --n2; }
    else { return tolower(*q1) - tolower(*q2); }
  }
  return n1 - n2;
}

static UINT rParse_Las_ArraySort ( struct las_line_data * const p )
{
  qsort ( p, r4_get_count_s4s(p), sizeof(struct las_line_data), (int (*)(const void *, const void *))_rParse_Las_ArraySortCMP );
  return 0;
}

static UINT rParse_Las_Save ( struct file_state_las * const pL )
{
  const LPWSTR s4w = r4_alloca_s4w(kPathMax);
  r4_push_path_s4w_s4w ( s4w, pL->script->s4wPathOutLasDir );
  for ( UINT i = 0; i <= 999; ++i )
  {
    swprintf ( s4w+r4_get_count_s4w(s4w), kPathMax-r4_get_count_s4w(s4w),
            L"\\%.*hs_%s_[%03u].LAS",
            pL->w_WELL._.n, pL->w_WELL._.p,
            pL->wszFileName, i );
    FILE * const pf = _wfopen ( s4w, L"rb" );
    if ( pf ) { fclose ( pf ); } else { break; }
  }
  FILE * const pf = _wfopen ( s4w, L"wb" );
  if ( pf )
  {
    fwrite ( pL->fm.pData, 1, pL->fm.nSize, pf );
    fclose ( pf );
  }
  else
  {
    rLog_Error ( L"Невозможно открыть файл для записи [%s]\n", s4w );
    return __LINE__;
  }
  return 0;
}

static UINT rParse_Las_End ( struct file_state_las * const pL )
{
  rParse_Las_ArraySort ( pL->pArray );
  for ( UINT i = 0; i < r4_get_count_s4s(pL->pArray); ++i )
  {
    // rLog ( L"  |] %c %.*hs\n", pL->pArray[i].iSection, pL->pArray[i].aMNEM.n, pL->pArray[i].aMNEM.p );
  }
  return rParse_Las_Save ( pL );
}

static UINT rParse_Las_SectionA ( struct file_state_las * const pL )
{
  struct file_data_ptr * const p = &(pL->t);
  rFileData_SkipToNewLine ( p, pL->iLineFeed );
  rFileData_SkipWhiteSpaces( p, pL->iLineFeed );
  const UINT nCC = r4_get_count_s4s ( pL->pA_C ); // Количество данных в строке

  // rLog ( L"  ==> STRT: %f\n", pL->w_STRT.d );
  // rLog ( L"  ==> STOP: %f\n", pL->w_STOP.d );
  // rLog ( L"  ==> STEP: %f\n", pL->w_STEP.d );
  // rLog ( L"  ==> NULL: %f\n", pL->w_NULL.d );
  // rLog ( L"  ==> METHODS: %u\n", nCC );
  for ( UINT i = 0; i < nCC; ++i )
  {
    // rLog ( L"  ==> [%u]: %.*hs\n", i, pL->pA_C[i].aMNEM.n, pL->pA_C[i].aMNEM.p );
  }
  for ( UINT i = 0; i < nCC; ++i )
  {
    pL->pA_C[i].fSTRT = max(pL->w_STRT.d,pL->w_STOP.d)+1000.0;
    pL->pA_C[i].fSTOP = min(pL->w_STRT.d,pL->w_STOP.d)-1000.0;
  }

  LPSTR pp;
  double fDepth = strtod ( ((LPCSTR)p->p), &pp );
  if ( ((LPVOID)pp) == ((LPVOID)p->p) )
  {
    rLog_Error ( L" => LAS(%u): Невозможо прочесть первое значение глубины\n", p->nLine );
    return __LINE__;
  }
  rFileData_Skip(p,((LPVOID)pp)-((LPVOID)p->p));
  // p->p = (BYTE const *)pp;
  rFileData_SkipWhiteSpaces( p, pL->iLineFeed );
  if ( fabs ( fDepth - pL->w_STRT.d ) > pL->fErr )
  {
    rLog_Error ( L" => LAS(%u): Первое значение глубины не совпадает с полем ~W:STRT\n", p->nLine );
    return __LINE__;
  }

  P_A:
  if ( pL->pA_C[0].fSTRT >= fDepth ) { pL->pA_C[0].fSTRT = fDepth; }
  if ( pL->pA_C[0].fSTOP <= fDepth ) { pL->pA_C[0].fSTOP = fDepth; }
  for ( UINT i = 1; i < nCC; ++i )
  {
    if ( p->n == 0 )
    {
      rLog_Error ( L" => LAS(%u): Данные внезапно закончились\n", p->nLine );
      return __LINE__;
    }
    const double f = strtod ( ((LPCSTR)p->p), &pp );
    // rLog ( L"  ==> f: %f\n", f );
    if ( ((LPVOID)pp) == ((LPVOID)p->p) )
    {
      rLog_Error ( L" => LAS(%u): Невозможо прочесть значение [%u]\n", p->nLine, i );
      return __LINE__;
    }
    rFileData_Skip(p,((LPVOID)pp)-((LPVOID)p->p));
    // p->p = (BYTE const *)pp;
    rFileData_SkipWhiteSpaces( p, pL->iLineFeed );
    if ( fabs ( f - pL->w_NULL.d ) > pL->fErr )
    {
      // если верхняя граница ниже настоящего значения, то записываем и с другой границе также
      if ( pL->pA_C[i].fSTRT >= fDepth ) { pL->pA_C[i].fSTRT = fDepth; }
      if ( pL->pA_C[i].fSTOP <= fDepth ) { pL->pA_C[i].fSTOP = fDepth; }
    }
  }

  if ( fabs ( fDepth - pL->w_STOP.d ) > pL->fErr )
  {
    if ( p->n == 0 )
    {
      rLog_Error ( L" => LAS(%u): Данные внезапно закончились\n", p->nLine );
      return __LINE__;
    }
    const double _f = strtod ( ((LPCSTR)p->p), &pp );
    // rLog ( L"  ==> fDepth: %f\n", _f );
    if ( ((LPVOID)pp) == ((LPVOID)p->p) )
    {
      rLog_Error ( L" => LAS(%u): Невозможо прочесть значение новой глубин\n", p->nLine );
      return __LINE__;
    }
    rFileData_Skip(p,((LPVOID)pp)-((LPVOID)p->p));
    // p->p = (BYTE const *)pp;
    rFileData_SkipWhiteSpaces( p, pL->iLineFeed );
    if ( fabs ( fabs ( _f - fDepth ) - fabs ( pL->w_STEP.d ) ) > pL->fErr )
    {
      rLog_Error ( L" => LAS(%u): Непредвиденый разрыв значения глубин\n", p->nLine );
      return __LINE__;
    }
    fDepth = _f;
    goto P_A;
  }
  else
  {
    if ( p->n != 0 )
    {
      // if ( p->n == 1 && *p->p == 0 ) { return 0; }
      rLog_Error ( L" => LAS(%u): Непредвиденые данные (%u)[%.*hs](%u)\n", p->nLine, *p->p, p->n, p->p, p->n );
      return __LINE__;
    }
  }

  for ( UINT i = 0; i < nCC; ++i )
  {
    // rLog ( L"  ==> [%u]: %-16.*hs % 10.4f % 10.4f\n",
    //     i, pL->pA_C[i].aMNEM.n, pL->pA_C[i].aMNEM.p, pL->pA_C[i].fSTRT, pL->pA_C[i].fSTOP );
  }
  return rParse_Las_End ( pL );
}

static UINT rParse_Las_S_DATA_2 ( struct file_state_las * const pL )
{
  pL->aDESC = pL->t;
  pL->aDATA = ((struct file_data_ptr){ });
  struct file_data_ptr * const p = &(pL->t);
  while ( p->n )
  {
    if ( *(p->p) == pL->iLineFeed || ( pL->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' ) )
    {
      pL->aDATA.n -= p->n;
      pL->aLine.n -= p->n;
      rFileData_Trim ( &(pL->aDATA) );
      rFileData_Trim ( &(pL->aLine) );
      rFileData_SkipToNewLine ( p, pL->iLineFeed );
      return 0;
    }
    else
    if ( *(p->p) == ':' && pL->aDATA.p == NULL )
    {
      pL->aDESC.n -= p->n;
      rFileData_Trim ( &(pL->aDESC) );
      rFileData_Skip(p,1);
      pL->aDATA = *p;
    }
    else { rFileData_Skip(p,1); }
  }
  rLog_Error ( L" => LAS(%u): Непредвиденный конец файла (отсутсвует двоеточие)\n", p->nLine );
  return __LINE__;
}

static UINT rParse_Las_S_DATA_1 ( struct file_state_las * const pL )
{
  pL->aDATA = pL->t;
  pL->aDESC = ((struct file_data_ptr){ });
  struct file_data_ptr * const p = &(pL->t);
  while ( p->n )
  {
    if ( *(p->p) == pL->iLineFeed || ( pL->iLineFeed == 0x0D0A && p->n >= 2 && p->p[0] == '\r' && p->p[1] == '\n' ) )
    {
      if ( pL->aDESC.p == NULL )
      {
        rLog_Error ( L" => LAS(%u): Непредвиденный конец строки (отсутсвует двоеточие) [%.*hs]\n[%.*hs]\n[MNEM=%.*hs]\n[UNIT=%.*hs]\n", p->nLine,
                p->n < 16 ? p->n : 16, p->p,
                pL->aMNEM.n, pL->aMNEM.p,
                pL->aUNIT.n, pL->aUNIT.p );
        return __LINE__;
      }
      pL->aDATA.n -= pL->aDESC.n+1;
      pL->aDESC.n -= p->n;
      pL->aLine.n -= p->n;
      rFileData_Trim ( &(pL->aDATA) );
      rFileData_Trim ( &(pL->aDESC) );
      rFileData_Trim ( &(pL->aLine) );
      rFileData_SkipToNewLine ( p, pL->iLineFeed );
      return 0;
    }
    else
    if ( *(p->p) == ':' )
    {
      rFileData_Skip(p,1);
      pL->aDESC = *p;
    }
    else { rFileData_Skip(p,1); }
  }
  rLog_Error ( L" => LAS(%u): Непредвиденный конец файла (отсутсвует двоеточие)\n", p->nLine );
  return __LINE__;
}


static UINT rParse_Las_S_C ( struct file_state_las * const pL )
{
  struct file_data_ptr const * const aMNEM = &(pL->aMNEM);
  struct file_data_ptr const * const aUNIT = &(pL->aUNIT);
  struct file_data_ptr const * const aDATA = &(pL->aDATA);
  struct file_data_ptr const * const aDESC = &(pL->aDESC);
  struct file_data_ptr const * const aLine = &(pL->aLine);
  if ( pL->iVersion == 2 ) { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) { return iErr; } }
  else
  if ( pL->iVersion == 1 ) { const UINT iErr = rParse_Las_S_DATA_2 ( pL ); if ( iErr ) { return iErr; } }
  else
  {
    rLog_Error ( L" => LAS(%u): Некооректное значение версии файла\n", pL->t.nLine );
    return __LINE__;
  }
  pL->pArray = r4_add_array_s4s ( pL->pArray, &((struct las_line_data){ .iSection = 'C',
          .aMNEM = *aMNEM, .aUNIT = *aUNIT, .aDATA = *aDATA, .aDESC = *aDESC, .aLine = *aLine }), 1 );
  pL->pA_C = r4_add_array_s4s ( pL->pA_C, &((struct las_c_data){
          .aMNEM = *aMNEM, .aUNIT = *aUNIT, .aDATA = *aDATA, .aDESC = *aDESC, .aLine = *aLine,
          .fSTRT = pL->w_STOP.d, .fSTOP = pL->w_STRT.d }), 1 );
  return 0;
}
static UINT rParse_Las_S_P ( struct file_state_las * const pL )
{
  struct file_data_ptr const * const aMNEM = &(pL->aMNEM);
  struct file_data_ptr const * const aUNIT = &(pL->aUNIT);
  struct file_data_ptr const * const aDATA = &(pL->aDATA);
  struct file_data_ptr const * const aDESC = &(pL->aDESC);
  struct file_data_ptr const * const aLine = &(pL->aLine);
  if ( pL->iVersion == 2 ) { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) { return iErr; } }
  else
  if ( pL->iVersion == 1 ) { const UINT iErr = rParse_Las_S_DATA_2 ( pL ); if ( iErr ) { return iErr; } }
  else
  {
    rLog_Error ( L" => LAS(%u): Некооректное значение версии файла\n", pL->t.nLine );
    return __LINE__;
  }
  pL->pArray = r4_add_array_s4s ( pL->pArray, &((struct las_line_data){ .iSection = 'P',
          .aMNEM = *aMNEM, .aUNIT = *aUNIT, .aDATA = *aDATA, .aDESC = *aDESC, .aLine = *aLine }), 1 );
  return 0;
}
static UINT rParse_Las_S_O ( struct file_state_las * const pL )
{
  struct file_data_ptr const * const aMNEM = &(pL->aMNEM);
  struct file_data_ptr const * const aUNIT = &(pL->aUNIT);
  struct file_data_ptr const * const aDATA = &(pL->aDATA);
  struct file_data_ptr const * const aDESC = &(pL->aDESC);
  struct file_data_ptr const * const aLine = &(pL->aLine);
  if ( pL->iVersion == 2 ) { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) { return iErr; } }
  else
  if ( pL->iVersion == 1 ) { const UINT iErr = rParse_Las_S_DATA_2 ( pL ); if ( iErr ) { return iErr; } }
  else
  {
    rLog_Error ( L" => LAS(%u): Некооректное значение версии файла\n", pL->t.nLine );
    return __LINE__;
  }
  pL->pArray = r4_add_array_s4s ( pL->pArray, &((struct las_line_data){ .iSection = 'O',
          .aMNEM = *aMNEM, .aUNIT = *aUNIT, .aDATA = *aDATA, .aDESC = *aDESC, .aLine = *aLine }), 1 );
  return 0;
}

static UINT rParse_Las_S_W ( struct file_state_las * const pL )
{
  struct file_data_ptr const * const aMNEM = &(pL->aMNEM);
  struct file_data_ptr const * const aUNIT = &(pL->aUNIT);
  struct file_data_ptr const * const aDATA = &(pL->aDATA);
  struct file_data_ptr const * const aDESC = &(pL->aDESC);
  struct file_data_ptr const * const aLine = &(pL->aLine);
  if ( rFileData_Cmp ( aMNEM, "STRT" ) )
  {
    { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) { return iErr; } }
    pL->w_STRT._ = pL->aLine;
    pL->w_STRT.d = atof ( (const LPCSTR)(aDATA->p) );
  }
  else
  if ( rFileData_Cmp ( aMNEM, "STOP" ) )
  {
    { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) { return iErr; } }
    pL->w_STOP._ = pL->aLine;
    pL->w_STOP.d = atof ( (const LPCSTR)(aDATA->p) );
  }
  else
  if ( rFileData_Cmp ( aMNEM, "STEP" ) )
  {
    { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) { return iErr; } }
    pL->w_STEP._ = pL->aLine;
    pL->w_STEP.d = atof ( (const LPCSTR)(aDATA->p) );
  }
  else
  if ( rFileData_Cmp ( aMNEM, "NULL" ) )
  {
    { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) { return iErr; } }
    pL->w_NULL._ = pL->aLine;
    pL->w_NULL.d = atof ( (const LPCSTR)(aDATA->p) );
  }
  else
  {
    if ( pL->iVersion == 2 ) { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) { return iErr; } }
    else
    if ( pL->iVersion == 1 ) { const UINT iErr = rParse_Las_S_DATA_2 ( pL ); if ( iErr ) { return iErr; } }
    else
    {
      rLog_Error ( L" => LAS(%u): Некооректное значение версии файла\n", pL->t.nLine );
      return __LINE__;
    }
    if ( rFileData_Cmp ( aMNEM, "WELL" ) )
    {
      pL->w_WELL._ = *aDATA;
    }
  }
  pL->pArray = r4_add_array_s4s ( pL->pArray, &((struct las_line_data){ .iSection = 'W',
          .aMNEM = *aMNEM, .aUNIT = *aUNIT, .aDATA = *aDATA, .aDESC = *aDESC, .aLine = *aLine }), 1 );
  return 0;
}

static UINT rParse_Las_S_V ( struct file_state_las * const pL )
{
  { const UINT iErr = rParse_Las_S_DATA_1 ( pL ); if ( iErr ) return iErr; }
  struct file_data_ptr const * const p = &(pL->t);
  struct file_data_ptr const * const aMNEM = &(pL->aMNEM);
  struct file_data_ptr const * const aUNIT = &(pL->aUNIT);
  struct file_data_ptr const * const aDATA = &(pL->aDATA);
  struct file_data_ptr const * const aDESC = &(pL->aDESC);
  struct file_data_ptr const * const aLine = &(pL->aLine);
  if ( rFileData_Cmp ( aMNEM, "VERS" ) )
  {
    pL->v_VERS._ = pL->aLine;
    pL->v_VERS.d = atof ( (const LPCSTR)(aDATA->p) );
    if ( pL->v_VERS.d == 1.2 ) { pL->iVersion = 1; }
    else
    if ( pL->v_VERS.d == 2.0 ) { pL->iVersion = 2; }
    else
    {
      rLog_Error ( L" => LAS(%u): Некооректное значение версии файла\n", p->nLine );
      return __LINE__;
    }
  }
  else
  if ( rFileData_Cmp ( aMNEM, "WRAP" ) )
  {
    pL->v_WRAP._ = pL->aLine;
    if ( rFileData_Cmp ( aDATA, "YES" ) ) { pL->bWrap = TRUE; pL->v_WRAP.b = TRUE; }
    else
    if ( rFileData_Cmp ( aDATA, "NO" ) ) { pL->bWrap = FALSE; pL->v_WRAP.b = FALSE; }
    else
    {
      rLog_Error ( L" => LAS(%u): Некооректное значение многострочности данных\n", p->nLine );
      return __LINE__;
    }
  }
  else
  {
    rLog_Error ( L" => LAS(%u): Неизвестная мнемоника %.*hs\n", p->nLine, aMNEM->n, aMNEM->p );
    return __LINE__;
  }
  pL->pArray = r4_add_array_s4s ( pL->pArray, &((struct las_line_data){ .iSection = 'V',
          .aMNEM = *aMNEM, .aUNIT = *aUNIT, .aDATA = *aDATA, .aDESC = *aDESC, .aLine = *aLine }), 1 );
  return 0;
}
#endif

static UINT rLogLas_ ( LPCSTR const szFile, UINT const nLine,
        struct file_state_las * const las, UINT const iErr )
{
  las->iErr = iErr;
  return rLog_Error_ ( szFile,  nLine, L"las on line (%u): %-*.*hs\r\n  error 0x%x => %s\r\n",
          las->s.nLine, las->s.n < 16 ? las->s.n : 16,
          las->s.n < 16 ? las->s.n : 16,
          las->s.p, iErr, g7ErrStrScript[iErr] );
}
#define rLogLas(_s_,_i_) rLogLas_(__FILE__,__LINE__,_s_,_i_)

static BOOL rLas_ParseSection_End ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  las->aLine.n -= p->n;
  rMemPtrTxt_Skip_ToBeginNewLine ( p );
  rMemPtrTxt_TrimSpaces ( &(las->aDATA) );
  rMemPtrTxt_TrimSpaces ( &(las->aDESC) );
  rMemPtrTxt_TrimSpaces ( &(las->aLine) );
  rMemPtrTxt_TrimSpaces ( &(las->aLine) );
  rLas_LogToDB ( las, L"*\t\t\t%.*hs\t\t%s\t%u\r\n",  las->aLine.n,  las->aLine.p, las->script->s4wOrigin, las->aLine.nLine );
  rLas_LogToDB ( las, L"V\t%.*hs\t%.*hs\t%.*hs\t%.*hs\t%s\t%u\r\n",
          las->aMNEM.n,  las->aMNEM.p,
          las->aUNIT.n,  las->aUNIT.p,
          las->aDATA.n,  las->aDATA.p,
          las->aDESC.n,  las->aDESC.p,
          las->script->s4wOrigin, las->aLine.nLine );
  return TRUE;
}

static BOOL rLas_ParseData2 ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  las->aDESC = *p;
  las->aDATA = ((struct mem_ptr_txt){ });
  rMemPtrTxt_Skip_ToFirstNonSpace ( p );
  do
  {
    if ( rMemPtrTxt_IsNewLine ( p ) )
    {
      if ( las->aDESC.p == NULL )
      {
        rLogLas ( las, kErr_ParserLas_EOL_DoublePoint );
        return FALSE;
      }
      las->aDATA.n -= p->n;
      return rLas_ParseSection_End ( las, p );
    }
    else
    if ( *(p->p) == ':' && las->aDATA.p == NULL )
    {
      las->aDESC.n -= p->n;
      rMemPtrTxt_TrimSpaces ( &(las->aDESC) );
      rMemPtrTxt_Skip_NoValid ( p, 1 );
      las->aDATA = *p;
    }
  } while ( rMemPtrTxt_Skip_NoValid ( p, 1 ) );
  rLogLas ( las, kErr_ParserLas_EOF_DoublePoint );
  return FALSE;
}

static BOOL rLas_ParseData1 ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  rMemPtrTxt_Skip_ToFirstNonSpace ( p );
  las->aDESC = ((struct mem_ptr_txt){ });
  las->aDATA = *p;
  do
  {
    if ( rMemPtrTxt_IsNewLine ( p ) )
    {
      if ( las->aDESC.p == NULL )
      {
        rLogLas ( las, kErr_ParserLas_EOL_DoublePoint );
        return FALSE;
      }
      las->aDATA.n -= las->aDESC.n+1;
      las->aDESC.n -= p->n;
      return rLas_ParseSection_End ( las, p );
    }
    else
    if ( *(p->p) == ':' )
    {
      rMemPtrTxt_Skip_NoValid ( p, 1 );
      las->aDESC = *p;
    }
  } while ( rMemPtrTxt_Skip_NoValid ( p, 1 ) );
  rLogLas ( las, kErr_ParserLas_EOF_DoublePoint );
  return FALSE;
}



static BOOL rLas_ParseSection_Version ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  if ( !rLas_ParseData1 ( las, p ) ) { return FALSE; }
  if ( rMemPtrTxt_CmpArrayA ( &(las->aMNEM), "VERS" ) )
  {
    const double d = _atof_l ( las->aDATA.p, g_locale_C );
    if ( d == 1.2 ) { las->iVersion = 1; }
    else
    if ( d == 2.0 ) { las->iVersion = 2; }
    else
    if ( las->iVersion != 1 && las->iVersion != 2 )
    { rLogLas ( las, kErr_ParserLas_IncorrectVersion ); return FALSE; }
  }
  else
  if ( rMemPtrTxt_CmpArrayA ( &(las->aMNEM), "WRAP" ) )
  {
    if ( rMemPtrTxt_CmpArrayA ( &(las->aDATA), "YES" ) ) { las->bWrap = TRUE; }
    else
    if ( rMemPtrTxt_CmpArrayA ( &(las->aDATA), "NO" ) ) { las->bWrap = FALSE; }
    else
    {
      rLogLas ( las, kErr_ParserLas_IncorrectWrap );
      return FALSE;
    }
  }
  else
  {
    rLogLas ( las, kErr_ParserLas_UndefinedMnem );
    return FALSE;
  }
  return TRUE;
}
static BOOL rLas_ParseSection_Well ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  if ( rMemPtrTxt_CmpArrayA ( &(las->aMNEM), "STRT" ) )
  {
    if ( !rLas_ParseData1 ( las, p ) ||
            !rMemPtrTxt_GetDouble_NoSkip ( &(las->aDATA), &(las->w_STRT) ) ) { return FALSE; }
  }
  else
  if ( rMemPtrTxt_CmpArrayA ( &(las->aMNEM), "STOP" ) )
  {
    if ( !rLas_ParseData1 ( las, p ) ||
            !rMemPtrTxt_GetDouble_NoSkip ( &(las->aDATA), &(las->w_STOP) ) ) { return FALSE; }
    las->w_STOP = atof ( las->aDATA.p );
  }
  else
  if ( rMemPtrTxt_CmpArrayA ( &(las->aMNEM), "STEP" ) )
  {
    if ( !rLas_ParseData1 ( las, p ) ||
            !rMemPtrTxt_GetDouble_NoSkip ( &(las->aDATA), &(las->w_STEP) ) ) { return FALSE; }
    if ( las->w_STEP == 0.0 ) { rLogLas ( las, kErr_ParserLas_CantReadAscii ); return FALSE; }
  }
  else
  if ( rMemPtrTxt_CmpArrayA ( &(las->aMNEM), "NULL" ) )
  {
    if ( !rLas_ParseData1 ( las, p ) ||
            !rMemPtrTxt_GetDouble_NoSkip ( &(las->aDATA), &(las->w_NULL) ) ) { return FALSE; }
  }
  else
  {
    if ( las->iVersion == 2 && !rLas_ParseData1 ( las, p ) ) { return FALSE; }
    else
    if ( las->iVersion == 1 && !rLas_ParseData2 ( las, p ) ) { return FALSE; }
    else
    if ( las->iVersion != 1 && las->iVersion != 2 )
    { rLogLas ( las, kErr_ParserLas_IncorrectVersion ); return FALSE; }
    if ( rMemPtrTxt_CmpArrayA ( &(las->aMNEM), "WELL" ) )
    {
      las->w_WELL = las->aDATA;
    }
  }
  return TRUE;
}
static BOOL rLas_ParseSection_Curve ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  if ( las->iVersion == 2 && !rLas_ParseData1 ( las, p ) ) { return FALSE; }
  else
  if ( las->iVersion == 1 && !rLas_ParseData1 ( las, p ) ) { return FALSE; }
  else
  if ( las->iVersion != 1 && las->iVersion != 2 )
  { rLogLas ( las, kErr_ParserLas_IncorrectVersion ); return FALSE; }

  las->aCurves = r4_add_array_s4s ( las->aCurves, &((struct las_data_a){ .s = las->aMNEM }), 1 );
  return TRUE;
}
static BOOL rLas_ParseSection_Param ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  if ( las->iVersion == 2 && !rLas_ParseData1 ( las, p ) ) { return FALSE; }
  else
  if ( las->iVersion == 1 && !rLas_ParseData1 ( las, p ) ) { return FALSE; }
  else
  if ( las->iVersion != 1 && las->iVersion != 2 )
  { rLogLas ( las, kErr_ParserLas_IncorrectVersion ); return FALSE; }
  return TRUE;
}

static BOOL rLas_ParseLine_DATA ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  switch ( las->iSection )
  {
    case 'V': return rLas_ParseSection_Version ( las, p );
    case 'W': return rLas_ParseSection_Well ( las, p );
    case 'C': return rLas_ParseSection_Curve ( las, p );
    case 'P': return rLas_ParseSection_Param ( las, p );
    default:
      rLogLas ( las, kErr_ParserLas_Section );
      return FALSE;
  }
}

static BOOL rLas_ParseLine_UNIT ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  las->aUNIT = *p;
  do {
    if ( rMemPtrTxt_IsNewLine ( p ) )
    {
      rLogLas ( las, kErr_ParserLas_EOL_Space );
      return rMemPtrTxt_Skip_ToBeginNewLine ( p );
    }
    else
    if ( rMemPtrTxt_Skip_1ByteIfSpace ( p ) )
    {
      las->aUNIT.n -= p->n+1;
      rMemPtrTxt_TrimSpaces ( &(las->aUNIT) );
      return rLas_ParseLine_DATA ( las, p );
    }
  } while ( rMemPtrTxt_Skip_NoValid ( p, 1 ) );
  rLogLas ( las, kErr_ParserLas_EOF_Space );
  return FALSE;
}


static BOOL rLas_ParseSection_Other ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  struct mem_ptr_txt q = *p;
  rMemPtrTxt_Skip_ToBeginNewLine ( p );
  q.n -= p->n;
  rMemPtrTxt_TrimSpaces ( &q );
  rLas_LogToDB ( las, L"O\t\t\t%.*hs\t\t%s\t%u\r\n", q.n, q.p, las->script->s4wOrigin, q.nLine );
  return TRUE;
}


static BOOL rLas_ParseLine_MNEM ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  las->aLine = *p;
  if ( las->iSection == 'O' ) { return rLas_ParseSection_Other ( las, p ); }
  las->aMNEM = *p;
  // struct file_data_ptr * const p = &(pL->t);
  // Идём до точки
  do {
    if ( rMemPtrTxt_IsNewLine ( p ) )
    {
      rLogLas ( las, kErr_ParserLas_EOL_Point );
      return rMemPtrTxt_Skip_ToBeginNewLine ( p );
    }
    else
    if ( *(p->p) == '.' )
    {
      las->aMNEM.n -= p->n;
      rMemPtrTxt_Skip_NoValid ( p, 1 );
      rMemPtrTxt_TrimSpaces ( &(las->aMNEM) );
      return rLas_ParseLine_UNIT ( las, p );
    }
  } while ( rMemPtrTxt_Skip_NoValid ( p, 1 ) );
  rLogLas ( las, kErr_ParserLas_EOF_Point );
  return FALSE;
}

static BOOL rLas_ParseSection_Ascii ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  rMemPtrTxt_Skip_ToFirstNonSpace ( p );
  const UINT nCC = r4_get_count_s4s ( las->aCurves ); // Количество данных в строке
  for ( UINT i = 0; i < nCC; ++i )
  {
    las->aCurves[i].fSTRT = max ( las->w_STRT, las->w_STOP ) + 1000.0;
    las->aCurves[i].fSTOP = min ( las->w_STRT, las->w_STOP ) - 1000.0;
  }
  double fDepth;
  if ( rMemPtrTxt_GetDouble ( p, &fDepth ) == 0 )
  {
    rLogLas ( las, kErr_ParserLas_CantReadAscii_First );
    return FALSE;
  }
  rMemPtrTxt_Skip_ToFirstNonSpace ( p );

  if ( fabs ( fDepth - las->w_STRT ) > las->script->fLasErr )
  {
    rLogLas ( las, kErr_ParserLas_FistDepthNotEaqual );
    return FALSE;
  }

  P_A:
  if ( las->aCurves[0].fSTRT >= fDepth ) { las->aCurves[0].fSTRT = fDepth; }
  if ( las->aCurves[0].fSTOP <= fDepth ) { las->aCurves[0].fSTOP = fDepth; }
  for ( UINT i = 1; i < nCC; ++i )
  {
    if ( p->n == 0 )
    {
      rLogLas ( las, kErr_ParserLas_EOF );
      return FALSE;
    }
    double f;
    if ( rMemPtrTxt_GetDouble ( p, &f ) == 0 )
    {
      rLogLas ( las, kErr_ParserLas_CantReadAscii );
      return FALSE;
    }
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( fabs ( f - las->w_NULL ) > las->script->fLasErr )
    {
      // если верхняя граница ниже настоящего значения, то записываем и с другой границе также
      if ( las->aCurves[i].fSTRT >= fDepth ) { las->aCurves[i].fSTRT = fDepth; }
      if ( las->aCurves[i].fSTOP <= fDepth ) { las->aCurves[i].fSTOP = fDepth; }
    }
  }

  if ( fabs ( fDepth - las->w_STOP ) > las->script->fLasErr )
  {
    if ( p->n == 0 )
    {
      rLogLas ( las, kErr_ParserLas_EOF );
      return FALSE;
    }
    double _f;
    if ( rMemPtrTxt_GetDouble ( p, &_f ) == 0 )
    {
      rLogLas ( las, kErr_ParserLas_CantReadAscii );
      return FALSE;
    }
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( fabs ( fabs ( _f - fDepth ) - fabs ( las->w_STEP ) ) > las->script->fLasErr )
    {
      rLogLas ( las, kErr_ParserLas_IncorrectDepthGap );
      return FALSE;
    }
    fDepth = _f;
    goto P_A;
  }
  else
  {
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( p->n != 0 )
    {
      // rLogLas ( las, kErr_ParserLas_NotEOF );
      // return FALSE;
    }
  }

  for ( UINT i = 0; i < nCC; ++i )
  {
    rLog ( L"  ==> [%u]: %-16.*hs % 10.4f % 10.4f\n",
        i, las->aCurves[i].s.n, las->aCurves[i].s.p, las->aCurves[i].fSTRT, las->aCurves[i].fSTOP );
  }
  return FALSE;
}

static BOOL rLas_SkipLineComment ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  struct mem_ptr_txt q = *p;
  rMemPtrTxt_Skip_ToBeginNewLine ( p );
  q.n -= p->n;
  rMemPtrTxt_TrimSpaces ( &q );
  rLas_LogToDB ( las, L"#\t\t\t%.*hs\t\t%s\t%u\r\n", q.n, q.p, las->script->s4wOrigin, q.nLine );
  return TRUE;
}

static BOOL rLas_SkipLineSecton ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  struct mem_ptr_txt q = *p;
  rMemPtrTxt_Skip_ToBeginNewLine ( p );
  q.n -= p->n;
  rMemPtrTxt_TrimSpaces ( &q );
  rLas_LogToDB ( las, L"~\t%c\t\t%.*hs\t\t%s\t%u\r\n", las->iSection, q.n, q.p, las->script->s4wOrigin, q.nLine );
  return TRUE;
}

static BOOL rLas_ParseLine_Head ( struct file_state_las * const las, struct mem_ptr_txt * const p )
{
  if ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) >= 2 )
  {

    if ( *(p->p) == '~' )
    {
      switch ( p->p[1] )
      {
        case 'V': case 'W': case 'C': case 'P': case 'O':
          las->iSection = p->p[1]; return rLas_SkipLineSecton ( las, p );
        case 'A': las->iSection = p->p[1]; return rLas_SkipLineSecton ( las, p ) && rLas_ParseSection_Ascii ( las, p );
        default:
          rLogLas ( las, kErr_ParserLas_Section );
          return FALSE;
      }
    }
    else
    if ( *(p->p) == '#' )
    {
      return rLas_SkipLineComment ( las, p );
    }
    else
    {
      return rLas_ParseLine_MNEM ( las, p );
    }
  }
  rLogLas ( las, kErr_ParserLas_EOF );
  return FALSE;
}


static BOOL rLas_ParseFile ( struct ag47_script * const script, LPCWSTR const s4wPath, LPCWSTR const wszFileName )
{
  rLog ( L"LAS% 9u: %s\n", ++script->nLasDetected, script->s4wOrigin );
  struct file_state_las _las =
  {
    .script             = script,
    .wszFileName        = wszFileName,
  };
  if ( !rFS_FileMapOpen ( &(_las.fm), s4wPath ) ) { return FALSE; }
  UINT a1[g7CharMapCount], a2[g7CharMapCount];
  _las.iCP[1] = rGetBufCodePage ( _las.fm.pData, _las.fm.nSize, a1, a2 );
  _las.iCP[0] = g7CharMapId[_las.iCP[1]];
  setlocale ( LC_ALL, g7CharMapCP[_las.iCP[1]] );
  // _las.iNL    = rGetBuf_NL ( _las.fm.pData, _las.fm.nSize );
  _las.s      = ((struct mem_ptr_txt){
          .p = (LPCSTR)_las.fm.pData,
          .n = _las.fm.nSize,
          .nLine = 1,
          .iNL = rGetBuf_NL ( _las.fm.pData, _las.fm.nSize ) });
  _las.aCurves = r4_malloc_s4s ( 8, sizeof(struct las_data_a) );


  rLas_LogToDB ( &_las, L"^\tCODEPAGE\t%u\t%hs\t\t%s\t\r\n", _las.iCP[0], g7CharMapNames[_las.iCP[1]], script->s4wOrigin );
  rLas_LogToDB ( &_las, L"^\tNEWLINE\t\t%hs\t\t%s\t\r\n", rGetNlName(_las.s.iNL), script->s4wOrigin );
  for ( UINT i = 0; i < g7CharMapCount; ++i )
  {
    rLas_LogToDB ( &_las, L"^\tCP.%u\t%hs\t%u\t%u\t%s\t\r\n", g7CharMapId[i], g7CharMapNames[i], a1[i], a2[i], script->s4wOrigin );
  }

#if 0
  _las.iLineFeed           = rGetBufEndOfLine ( _las.fm.pData, _las.fm.nSize );
  _las.t.p                 = _las.fm.pData;
  _las.t.n                 = _las.fm.nSize;
  _las.t.nLine             = 1;
  _las.iSection            = 0;
  _las.nWarnings           = 0;
  _las.fErr                = 0.015;
  _las.pArray              = (struct las_line_data *) r4_malloc_s4s ( 64, sizeof(struct las_line_data) );
  _las.pA_C                = (struct las_c_data *) r4_malloc_s4s ( 2, sizeof(struct las_c_data) );
#endif
  setlocale ( LC_ALL, g7CharMapCP[_las.iCP[1]] );

  _las.s4wTempOut = r4_alloca_s4w ( kPathMax );
  r4_init_s4w_s4w ( _las.s4wTempOut, script->s4wPathOutTempDir );
  rFS_NewRandDir_s4w ( _las.s4wTempOut ); // .temp/xxxxxxxx/
  while ( rLas_ParseLine_Head ( &_las, &(_las.s) ) ) { }
  if ( _las.iErr )
  {
    LPWSTR s4wPath = r4_alloca_s4w ( kPathMax );
    r4_init_s4w_s4w ( s4wPath, script->s4wPathOutErrorDir );
    r4_get_count_s4w ( s4wPath ) += swprintf ( s4wPath + r4_get_count_s4w ( s4wPath ),
            kPathMax - r4_get_count_s4w ( s4wPath ), L"\\%04u.las", (++script->nErrorFiles) );
    FILE * const pF = _wfopen ( s4wPath, L"wb" );
    fprintf ( pF, "# Ag47_CodePage: [.%u] %s", _las.iCP[0], g7CharMapNames[_las.iCP[1]] );
    fprintf ( pF, rGetNlStr(_las.s.iNL) );
    fprintf ( pF, "# Ag47_LineFeed: %s", rGetNlName(_las.s.iNL) );
    fprintf ( pF, rGetNlStr(_las.s.iNL) );
    fprintf ( pF, "# Ag47_ErrorLine: %u", _las.s.nLine+5 );
    fprintf ( pF, rGetNlStr(_las.s.iNL) );
    fprintf ( pF, "# Ag47_ErrorCode: 0x%x", _las.iErr );
    fprintf ( pF, rGetNlStr(_las.s.iNL) );
    fprintf ( pF, "# Ag47_ErrorDesc: %ls", g7ErrStrScript[_las.iErr] );
    fprintf ( pF, rGetNlStr(_las.s.iNL) );
    fwrite ( _las.fm.pData, 1, _las.fm.nSize, pF );
    fclose ( pF );
  }
  else
  {
    LPWSTR const s4w = r4_alloca_s4w ( kPathMax );
    r4_push_path_s4w_s4w ( s4w, script->s4wPathOutLasDir );
    for ( UINT i = 0; TRUE; ++i )
    {
      swprintf ( s4w+r4_get_count_s4w(s4w), kPathMax-r4_get_count_s4w(s4w),
              L"\\%.*hs_%s_%u.LAS",
              _las.w_WELL.n, _las.w_WELL.p,
              _las.wszFileName, i );
      FILE * const pf = _wfopen ( s4w, L"rb" );
      if ( pf ) { fclose ( pf ); } else { break; }
    }
    FILE * const pf = _wfopen ( s4w, L"wb" );
    if ( pf )
    {
      fwrite ( _las.fm.pData, 1, _las.fm.nSize, pf );
      fclose ( pf );
    }
    else
    {
      rLog_Error ( L"Невозможно открыть файл для записи [%s]\n", s4w );
    }
    UINT const nCC = r4_get_count_s4s ( _las.aCurves ); // Количество данных в строке

    setlocale ( LC_ALL, "C" );
    for ( UINT i = 1; i < nCC; ++i )
    {
      rLogToAB ( script, "LAS\t%.*s\t%.*s\t%f\t%f\r\n",
              _las.w_WELL.n,
              _las.w_WELL.p,
              _las.aCurves[i].s.n,
              _las.aCurves[i].s.p,
              _las.aCurves[i].fSTRT,
              _las.aCurves[i].fSTOP );
    }
  }
  rFS_DeleteTree ( _las.s4wTempOut );

#if 0
  static UINT nFile = 0;
  CHAR str[512];
  sprintf ( str, ".ag47/" );
  CreateDirectoryA ( str, NULL );
  sprintf ( str, ".ag47/.db" );
  CreateDirectoryA ( str, NULL );
  sprintf ( str, ".ag47/.db/%02u", (nFile/1000000) );
  CreateDirectoryA ( str, NULL );
  sprintf ( str, ".ag47/.db/%02u/%02u/", (nFile/1000000), (nFile/10000)%100 );
  CreateDirectoryA ( str, NULL );
  sprintf ( str, ".ag47/.db/%02u/%02u/%04u.las", (nFile/1000000), (nFile/10000)%100, nFile%10000 );
  ++nFile;
  FILE * const pF = fopen ( str, "wb" );
  fprintf ( pF, "# Ag47_CodePage    = %hs (%hs)%hs",
          g7CharMapCP[_.iCodePage], g7CharMapNames[_.iCodePage],
          _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  fprintf ( pF, "# Ag47_LineFeed    = %hs%hs",
          _.iLineFeed == '\r' ? "CR (Mac)" : _.iLineFeed == '\n' ? "LF (Unix)" : _.iLineFeed == 0x0D0A ? "CRLF (Windows)" : "???",
          _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  fprintf ( pF, "# Ag47_Origin      = %ls%hs",
          s4wOrigin,
          _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  fprintf ( pF, "# Ag47_Size        = %u%hs",
          _.fm.nSize,
          _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  for ( UINT k = 0; k < g7CharMapCount; ++k )
  {
    fprintf ( pF, "# Ag47_CodePage[i] = %16.16hs (% 8u % 8u)%hs",
            g7CharMapCP[k],a1[k],a2[k],
            _.iLineFeed==0x0D0A?"\r\n":_.iLineFeed=='\r'?"\r":"\n" );
  }
  fwrite ( _.p, 1, _.n, pF );
  fclose ( pF );
  goto P_End;
#endif

  // rParse_Las_Log ( L"%hc\t%hs\t%hs\t%hs\t%hs\t%s\t%u\r\n",
  //         '#',
  //         "FILESETS",
  //         g7CharMapNames[_.iCodePage],
  //         g7CharMapCP[_.iCodePage],
  //         _.iLineFeed == '\r' ? "CR (Mac)" : _.iLineFeed == '\n' ? "LF (Unix)" : _.iLineFeed == 0x0D0A ? "CRLF (Windows)" : "???",
  //         _.s4wOrigin, 0 );
  // for ( UINT k = 0; k < g7CharMapCount; ++k )
  // {
  //     rParse_Las_Log ( L"%hc\t%hs\t%hs\t%u\t%u\t%s\t%u\r\n",
  //             '#',
  //             "CHARMAP_VARS",
  //             g7CharMapNames[k],
  //             a1[k],
  //             a2[k],
  //             _.s4wOrigin, 0 );

  // }

  // Начало файла
  // if ( ( rParse_Las_BeginOfLine ( &_ ) ) ) { goto P_End; }



  // P_End:
  // r4_free_s4s ( _las.pA_C );
  // r4_free_s4s ( _las.pArray );
  if ( _las.aCurves ) { r4_free_s4s ( _las.aCurves ); }
  rFS_FileMapClose ( &_las.fm );
  return TRUE;
  // return _las.iErr == 0;
}

