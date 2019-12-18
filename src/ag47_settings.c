


static UINT rLogScript_ ( LPCSTR const szFile, UINT const nLine,
        struct ag47_script * const script, UINT const iErr )
{
  return rLog_Error_ ( szFile,  nLine, L"script on line (%u): %-*.*hs\r\n  error 0x%x => %s\r\n",
          script->p_txt->nLine, script->p_txt->n < 16 ? script->p_txt->n : 16,
          script->p_txt->n < 16 ? script->p_txt->n : 16,
          script->p_txt->p, iErr, g7ErrStrScript[iErr] );
}
#define rLogScript(_s_,_i_) rLogScript_(__FILE__,__LINE__,_s_,_i_)


static BOOL rScriptRun ( struct ag47_script * const script )
{
  rLog ( L" === === === Выполнение сценария === === === \r\n" );
  return rFS_Tree ( script->s4wRun,
          (BOOL (*)(LPWSTR const,  LPCWSTR const, UINT const, LPVOID const))rParse_FileProc,
          (BOOL (*)(LPWSTR const,  LPCWSTR const, LPVOID const))rParse_FolderProc,
          script  );
}

static VOID rScriptFree ( struct ag47_script * const script )
{
  if ( script->s4wRun ) { r4_free_s4w ( script->s4wRun ); script->s4wRun = NULL; }
  if ( script->s4wOutPath ) { r4_free_s4w ( script->s4wOutPath ); script->s4wOutPath = NULL; }
  if ( script->s4wPathToWordconv ) { r4_free_s4w ( script->s4wPathToWordconv ); script->s4wPathToWordconv = NULL; }
  if ( script->s4wPathTo7Zip ) { r4_free_s4w ( script->s4wPathTo7Zip ); script->s4wPathTo7Zip = NULL; }
  if ( script->ss4wExcludeFF ) { r4_free_ss4w_withsub ( script->ss4wExcludeFF ); script->ss4wExcludeFF = NULL; }
  if ( script->s4uExcludeSizes ) { r4_free_s4u ( script->s4uExcludeSizes ); script->s4uExcludeSizes = NULL; }
  if ( script->ss4wLasFF ) { r4_free_ss4w_withsub ( script->ss4wLasFF ); script->ss4wLasFF = NULL; }
  if ( script->ss4wInkFF ) { r4_free_ss4w_withsub ( script->ss4wInkFF ); script->ss4wInkFF = NULL; }
}

static BOOL rScript_CaseName ( struct mem_ptr_txt * const p, LPCSTR const sz )
{ return rMemPtrTxt_CmpCaseWordA ( p, sz ) && rMemPtrTxt_Skip_NoValid ( p, strlen ( sz ) ); }

static BOOL rScript_ParseVal_String ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, LPWSTR * const ps4w )
{
  if ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) && ( *(p->p) == '=' ) &&
       rMemPtrTxt_Skip_NoValid ( p, 1 ) && rMemPtrTxt_Skip_ToFirstNonSpace ( p ) )
  {
    if ( *ps4w ) { r4_free_s4w ( *ps4w ); *ps4w = NULL; }
    if ( rScript_CaseName ( p, "NULL" ) || *(p->p) == ';' ) { }
    else
    if ( *(p->p) == '\'' )
    {
      rMemPtrTxt_Skip_NoValid ( p, 1 );
      UINT i = 0;
      for ( ; i < p->n; ++i ) { if ( p->p[i] == '\'' ) { break; } }
      UINT iW = MultiByteToWideChar ( script->iCP, 0, p->p, i, NULL, 0 );
      *ps4w = r4_malloc_s4w ( iW+1 );
      r4_get_count_s4w ( (*ps4w) ) = MultiByteToWideChar ( script->iCP, 0, p->p, i, *ps4w, iW+1 );
      (*ps4w)[iW] = 0;
      rMemPtrTxt_Skip_NoValid ( p, i+1 );
    }
    else
    if ( *(p->p) == '\"' )
    {
      rMemPtrTxt_Skip_NoValid ( p, 1 );
      UINT i = 0;
      for ( ; i < p->n; ++i ) { if ( p->p[i] == '\"' ) { break; } }
      UINT iW = MultiByteToWideChar ( script->iCP, 0, p->p, i, NULL, 0 );
      *ps4w = r4_malloc_s4w ( iW+1 );
      r4_get_count_s4w ( (*ps4w) ) = MultiByteToWideChar ( script->iCP, 0, p->p, i, *ps4w, iW+1 );
      (*ps4w)[iW] = 0;
      rMemPtrTxt_Skip_NoValid ( p, i+1 );
    }
    else
    { rLogScript ( script, kErr_Script_InvalidValue ); return FALSE; }
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( *(p->p) == ';' ) { return rMemPtrTxt_Skip_NoValid ( p, 1 ); }
    else
    { rLogScript ( script, kErr_Script_EndOfValue ); return FALSE; }
  }
  else
  { rLogScript ( script, kErr_Script_EqValue ); return FALSE; }
}


static BOOL rScript_ParseVal_Bool ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, BOOL * const pFlag )
{
  if ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) && ( *(p->p) == '=' ) &&
       rMemPtrTxt_Skip_NoValid ( p, 1 ) && rMemPtrTxt_Skip_ToFirstNonSpace ( p ) )
  {
    if ( rScript_CaseName ( p, "NULL" ) || rScript_CaseName ( p, "0" ) ||
            rScript_CaseName ( p, "FALSE" ) || rScript_CaseName ( p, "NO" ) ||
            *(p->p) == ';' ) { *pFlag = FALSE; }
    else
    if ( rScript_CaseName ( p, "1" ) || rScript_CaseName ( p, "TRUE" ) ||
            rScript_CaseName ( p, "YES" ) ) { *pFlag = TRUE; }
    else
    { rLogScript ( script, kErr_Script_InvalidValue ); return FALSE; }
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( *(p->p) == ';' ) { return rMemPtrTxt_Skip_NoValid ( p, 1 ); }
    else
    { rLogScript ( script, kErr_Script_EndOfValue ); return FALSE; }
  }
  else
  { rLogScript ( script, kErr_Script_EqValue ); return FALSE; }
}

static BOOL rScript_ParseVal_Uint ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, UINT * const pVal )
{
  if ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) && ( *(p->p) == '=' ) &&
       rMemPtrTxt_Skip_NoValid ( p, 1 ) && rMemPtrTxt_Skip_ToFirstNonSpace ( p ) )
  {
    if ( rScript_CaseName ( p, "NULL" ) || rScript_CaseName ( p, "FALSE" ) ||
            rScript_CaseName ( p, "NO" ) || *(p->p) == ';' ) { *pVal = 0; }
    else
    if ( rScript_CaseName ( p, "TRUE" ) || rScript_CaseName ( p, "YES" ) ) { *pVal = 1; }
    else
    if ( rMemPtrTxt_GetUint ( p, pVal ) ) { }
    else
    { rLogScript ( script, kErr_Script_InvalidValue ); return FALSE; }
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( *(p->p) == ';' ) { return rMemPtrTxt_Skip_NoValid ( p, 1 ); }
    else
    { rLogScript ( script, kErr_Script_EndOfValue ); return FALSE; }
  }
  else
  { rLogScript ( script, kErr_Script_EqValue ); return FALSE; }
}

static BOOL rScript_ParseVal_VectorOfStrings ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, LPWSTR ** const pss4w )
{
  if ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) && ( *(p->p) == '=' ) &&
       rMemPtrTxt_Skip_NoValid ( p, 1 ) && rMemPtrTxt_Skip_ToFirstNonSpace ( p ) )
  {
    if ( *pss4w ) { r4_free_ss4w_withsub ( *pss4w ); *pss4w = NULL; }
    if ( rScript_CaseName ( p, "NULL" ) || *(p->p) == ';' ) { }
    else
    if ( *(p->p) == '[' && rMemPtrTxt_Skip_NoValid ( p, 1) )
    {
      *pss4w = r4_malloc_ss4w ( 4 );
      while ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) )
      {
        if ( *(p->p) == ']' && rMemPtrTxt_Skip_NoValid ( p, 1) ) { break; }
        else
        if ( rScript_CaseName ( p, "NULL" )  )
        {
          LPWSTR q = NULL;
          *pss4w = r4_add_array_ss4w ( *pss4w, &q, 1 );
        }
        else
        if ( *(p->p) == '\'' && rMemPtrTxt_Skip_NoValid ( p, 1) )
        {
          UINT i = 0;
          for ( ; i < p->n; ++i ) { if ( p->p[i] == '\'' ) { break; } }
          UINT iW = MultiByteToWideChar ( script->iCP, 0, p->p, i, NULL, 0 );
          LPWSTR s4w = r4_malloc_s4w ( iW+1 );
          r4_get_count_s4w ( s4w ) = MultiByteToWideChar ( script->iCP, 0, p->p, i, s4w, iW+1 );
          s4w[iW] = 0;
          *pss4w = r4_add_array_ss4w ( *pss4w, &s4w, 1 );
          rMemPtrTxt_Skip_NoValid ( p, i+1 );
        }
        else
        if ( *(p->p) == '\"' && rMemPtrTxt_Skip_NoValid ( p, 1) )
        {
          UINT i = 0;
          for ( ; i < p->n; ++i ) { if ( p->p[i] == '\"' ) { break; } }
          UINT iW = MultiByteToWideChar ( script->iCP, 0, p->p, i, NULL, 0 );
          LPWSTR s4w = r4_malloc_s4w ( iW+1 );
          r4_get_count_s4w ( s4w ) = MultiByteToWideChar ( script->iCP, 0, p->p, i, s4w, iW+1 );
          s4w[iW] = 0;
          *pss4w = r4_add_array_ss4w ( *pss4w, &s4w, 1 );
          rMemPtrTxt_Skip_NoValid ( p, i+1 );
        }
        else
        { rLogScript ( script, kErr_Script_InvalidValue ); return FALSE; }
        // ccc
        rMemPtrTxt_Skip_ToFirstNonSpace ( p );
        if ( *(p->p) == ',' && rMemPtrTxt_Skip_NoValid ( p, 1 ) ) { continue; }
        else
        if ( *(p->p) == ']' && rMemPtrTxt_Skip_NoValid ( p, 1 ) ) { break; }
        else
        { rLogScript ( script, kErr_Script_ArraySeparator ); return FALSE; }
      }
    }
    else
    { rLogScript ( script, kErr_Script_InvalidValue ); return FALSE; }
    ////cccc
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( *(p->p) == ';' ) { return rMemPtrTxt_Skip_NoValid ( p, 1 ); }
    else
    { rLogScript ( script, kErr_Script_EndOfValue ); return FALSE; }
  }
  else
  { rLogScript ( script, kErr_Script_EqValue ); return FALSE; }
}


static BOOL rScript_ParseVal_VectorOfUints ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, UINT ** const ps4u )
{
  if ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) && ( *(p->p) == '=' ) &&
       rMemPtrTxt_Skip_NoValid ( p, 1 ) && rMemPtrTxt_Skip_ToFirstNonSpace ( p ) )
  {
    if ( *ps4u ) { r4_free_s4u ( *ps4u ); *ps4u = NULL; }
    if ( rScript_CaseName ( p, "NULL" ) || *(p->p) == ';' ) { }
    else
    if ( *(p->p) == '[' && rMemPtrTxt_Skip_NoValid ( p, 1) )
    {
      *ps4u = r4_malloc_s4u ( 4 );
      while ( rMemPtrTxt_Skip_ToFirstNonSpace ( p ) )
      {
        UINT u = 0;
        if ( *(p->p) == ']' && rMemPtrTxt_Skip_NoValid ( p, 1) ) { break; }
        else
        if ( rScript_CaseName ( p, "NULL" ) || rScript_CaseName ( p, "FALSE" ) ||
                rScript_CaseName ( p, "NO" ) ) { u = 0; }
        else
        if ( rScript_CaseName ( p, "TRUE" ) || rScript_CaseName ( p, "YES" ) ) { u = 1; }
        else
        if ( rMemPtrTxt_GetUint ( p, &u ) ) { }
        else
        { rLogScript ( script, kErr_Script_InvalidValue ); return FALSE; }
        // ccc
        *ps4u = r4_add_array_s4u ( *ps4u, &u, 1 );
        rMemPtrTxt_Skip_ToFirstNonSpace ( p );
        if ( *(p->p) == ',' && rMemPtrTxt_Skip_NoValid ( p, 1 ) ) { continue; }
        else
        if ( *(p->p) == ']' && rMemPtrTxt_Skip_NoValid ( p, 1 ) ) { break; }
        else
        { rLogScript ( script, kErr_Script_ArraySeparator ); return FALSE; }
      }
    }
    else
    { rLogScript ( script, kErr_Script_InvalidValue ); return FALSE; }
    ////cccc
    rMemPtrTxt_Skip_ToFirstNonSpace ( p );
    if ( *(p->p) == ';' ) { return rMemPtrTxt_Skip_NoValid ( p, 1 ); }
    else
    { rLogScript ( script, kErr_Script_EndOfValue ); return FALSE; }
  }
  else
  { rLogScript ( script, kErr_Script_EqValue ); return FALSE; }
}

static BOOL rScript_ParseValName_String ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, LPWSTR * const ps4w, LPCSTR const sz )
{
  if ( rScript_CaseName ( p, sz ) )
  {
    const BOOL b = rScript_ParseVal_String ( script, p, ps4w );
    if ( b ) { rLog ( L"script %-16hs %-12hs => \'%s\'\r\n", sz, "STRING", *ps4w ); }
    return b;
  }
  return FALSE;
}

static BOOL rScript_ParseValName_Bool ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, BOOL * const pFlag, LPCSTR const sz )
{
  if ( rScript_CaseName ( p, sz ) )
  {
    const BOOL b = rScript_ParseVal_Bool ( script, p, pFlag );
    if ( b ) { rLog ( L"script %-16hs %-12hs => %hs\r\n", sz, "BOOL", *pFlag ? "TRUE" : "FALSE" ); }
    return b;
  }
  return FALSE;
}

static BOOL rScript_ParseValName_Uint ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, UINT * const pVal, LPCSTR const sz )
{
  if ( rScript_CaseName ( p, sz ) )
  {
    const BOOL b = rScript_ParseVal_Uint ( script, p, pVal );
    if ( b ) { rLog ( L"script %-16hs %-12hs => %u (0x%x)\r\n", sz, "UINT", *pVal, *pVal ); }
    return b;
  }
  return FALSE;
}


static BOOL rScript_ParseValName_VectorOfStrings ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, LPWSTR ** const pss4w, LPCSTR const sz )
{
  if ( rScript_CaseName ( p, sz ) )
  {
    const BOOL b = rScript_ParseVal_VectorOfStrings ( script, p, pss4w );
    if ( b )
    {
      if ( *pss4w )
      {
        rLog ( L"script %-16hs %-12hs == %u\r\n", sz, "STRING[]", r4_get_count_ss4w (*pss4w) );
        D4ForAll_ss4w ( *pss4w, i, n ) { rLog ( L"script %-16hs % 12u == %s\r\n", sz, i, (*pss4w)[i] ); }
      }
      else
      { rLog ( L"script %-16hs %-12hs => NULL\r\n", sz, "STRING[]" ); }
    }
    return b;
  }
  return FALSE;
}

static BOOL rScript_ParseValName_VectorOfUints ( struct ag47_script * const script,
        struct mem_ptr_txt * const p, UINT ** const ps4u, LPCSTR const sz )
{
  if ( rScript_CaseName ( p, sz ) )
  {
    const BOOL b = rScript_ParseVal_VectorOfUints ( script, p, ps4u );
    if ( b )
    {
      if ( *ps4u )
      {
        rLog ( L"script %-16hs %-12hs == %u\r\n", sz, "UINT[]", r4_get_count_s4u (*ps4u) );
        D4ForAll_s4u ( *ps4u, i, n ) { rLog ( L"script %-16hs % 12u == %u (0x%x)\r\n", sz, i, (*ps4u)[i], (*ps4u)[i] ); }
      }
      else
      { rLog ( L"script %-16hs %-12hs => NULL\r\n", sz, "UINT[]" ); }
    }
    return b;
  }
  return FALSE;
}

static BOOL rScript_ParseName ( struct ag47_script * const script, struct mem_ptr_txt * const p )
{
  return
  ( rScript_ParseValName_String ( script, p, &(script->s4wRun), "RUN" ) && rScriptRun ( script ) ) ||
  rScript_ParseValName_String ( script, p, &(script->s4wOutPath), "OUT_PATH" ) ||
  rScript_ParseValName_String ( script, p, &(script->s4wPathToWordconv), "PATH_TO_WORDCONV" ) ||
  rScript_ParseValName_String ( script, p, &(script->s4wPathTo7Zip), "PATH_TO_7ZIP" ) ||
  rScript_ParseValName_Bool ( script, p, &(script->bOutRecreate), "OUT_RECREATE" ) ||
  rScript_ParseValName_Bool ( script, p, &(script->bArchivesIgnore), "ARCHIVES_IGNORE" ) ||
  rScript_ParseValName_Uint ( script, p, &(script->nRecursive), "RECURSIVE" ) ||
  rScript_ParseValName_Uint ( script, p, &(script->iLasMod), "LAS_MOD" ) ||
  rScript_ParseValName_VectorOfStrings ( script, p, &(script->ss4wExcludeFF), "EXCLUDE_FF" ) ||
  rScript_ParseValName_VectorOfStrings ( script, p, &(script->ss4wLasFF), "LAS_FF" ) ||
  rScript_ParseValName_VectorOfStrings ( script, p, &(script->ss4wInkFF), "INK_FF" ) ||
  rScript_ParseValName_VectorOfUints ( script, p, &(script->s4uExcludeSizes), "EXCLUDE_SIZE" ) ||
  ( rLogScript ( script, kErr_Script_ValueName ), FALSE );
}

static BOOL rScript_ParseOne ( struct ag47_script * const script, struct mem_ptr_txt * const p )
{
  rMemPtrTxt_Skip_ToFirstNonSpace ( p );
  if ( p->n )
  {
    switch ( *(p->p) )
    {
      case '#': return rMemPtrTxt_Skip_ToBeginNewLine ( p );
      case '/':
        if ( p->n >= 2 )
        {
          if ( (p->p)[1] == '/' ) { return rMemPtrTxt_Skip_ToBeginNewLine ( p ); }
          else
          if ( (p->p)[1] == '*' && rMemPtrTxt_Skip_NoValid ( p, 2) ) {  return rMemPtrTxt_Skip_ToFirstCmpArrayA ( p, "*/" ) >= 2 ?
                  rMemPtrTxt_Skip_NoValid ( p, 2) : FALSE; }
          else { rLogScript ( script, kErr_Script_ErrSymbol ); return FALSE; }
        }
      case 'a' ... 'z':
      case 'A' ... 'Z':
        return rScript_ParseName ( script, p );
      default: rLogScript ( script, kErr_Script_ErrSymbol ); return FALSE;
    }
  }
  else
  { return FALSE; }
}

static VOID rScript_GetCodePage ( struct ag47_script * const script, struct mem_ptr_bin * const p )
{
  if ( rMemPtrBin_Skip_ToFirstNonSpace ( p ) && *(p->p) == '#' )
  {
    rMemPtrBin_Skip ( p, 1 );
    rMemPtrBin_Skip_ToFirstNonSpace ( p ) ;
    const UINT iCP = rGetCodePageIdByAsciiName ( (LPCSTR)p->p );
    if ( script->iCP && script->iCP != iCP )
    {
      rLog_Error ( L"script не свопадают BOM скрипта (%hs) и указаная кодировка (%hs)\r\n",
              rGetCodePageNameById ( script->iCP ), rGetCodePageNameById ( iCP ) );
    }
    else
    {
      script->iCP = iCP;
    }
    rMemPtrBin_Skip_ToBeginNewLine ( p );
  }
}

/*
  Загружает и выполняет скрипт находящийся в памяти
  !pBuf                 - указатель на память с данными
  !nSize                - размер памяти с данными
*/
static UINT rScriptRunMem ( BYTE const * const pBuf, UINT const nSize )
{
  struct mem_ptr_bin _ptr = { .p = pBuf, .n = nSize };
  struct ag47_script _script = { .p_bin = &_ptr };
  _script.iCP = rGetBOM ( &_ptr );
  if ( ( _script.iCP == 0 ) || ( _script.iCP == kCP_Utf8 ) )
  {
    struct mem_ptr_txt _txt = { ._ = _ptr, .nLine = 1, .iNL = rGetBuf_NL ( pBuf, nSize ) };
    _script.p_txt = &_txt;
    rScript_GetCodePage ( &_script, &_ptr );
    while ( rScript_ParseOne ( &_script, &_txt ) );
  }
  else
  {
    rLog_Error ( L"Code Page\r\n" );
  }
  rScriptFree ( &_script );
  return _script.iErr;
}

/*
  Загружает и выполняет файл скрипа
  !wszPath              - путь к файлу скрипта
*/
static UINT rScriptRunFile ( LPCWSTR const wszPath )
{
  struct file_map fm;
  if ( !rFS_FileMapOpen ( &fm, wszPath ) ) { return -1; }
  UINT const iErr = rScriptRunMem ( fm.pData, fm.nSize );
  rFS_FileMapClose ( &fm );
  return iErr;
}
