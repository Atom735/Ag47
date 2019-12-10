static UINT rParse_Las ( const LPWSTR s4wPath, const LPCWSTR s4wOrigin, const LPCWSTR wszFileName )
{
  rLog ( L"Parse_LAS: %-256s ==> %-256s\n", s4wOrigin, s4wPath );
  struct file_map fm = { };
  rFS_FileMapOpen ( &fm, s4wPath );
  UINT a1[g7CharMapCount], a2[g7CharMapCount];
  const UINT iCodePage = rGetBufCodePage ( fm.pData, fm.nSize, a1, a2 );
  const UINT iLineFeed = rGetBufEndOfLine ( fm.pData, fm.nSize );


  rFS_FileMapClose ( &fm );
  return 0;
}
