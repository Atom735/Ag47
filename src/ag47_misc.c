
static FILE * rOpenFileToWriteWith_UTF16_BOM ( const LPCWSTR wszFname )
{
  FILE * const fd = _wfopen ( wszFname, L"wb" );
  if ( fd ) fwprintf ( fd, L"%c", 0xFEFF );
  return fd;
}


static UINT rGetMaxNums ( UINT const * const pData, const UINT nSize )
{
  UINT k = 0;
  for ( UINT i = 1; i < nSize; ++i ) { if ( pData[i] > pData[k] ) { k = i; } }
  return k;
}
