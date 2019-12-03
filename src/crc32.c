/* https://www.ietf.org/rfc/rfc1952.txt */

/* Table of CRCs of all 8-bit messages. */
static UINT32 g_CRC32_table[256];
/* Flag: has the table been computed? Initially false. */
static BOOL g_CRC32_table_computed = FALSE;

/* Make the table for a fast CRC. */
static VOID rCRC32_make_table ( )
{
  UINT32 c;
  for ( UINT n = 0; n < 256; ++n ) {
    c = (UINT32) n;
    for ( UINT k = 0; k < 8; ++k ) {
      if (c & 1) {
        c = 0xedb88320L ^ (c >> 1);
      } else {
        c = c >> 1;
      }
    }
    g_CRC32_table[n] = c;
  }
  g_CRC32_table_computed = TRUE;
}

/*
   Update a running crc with the bytes buf[0..len-1] and return
 the updated crc. The crc should be initialized to zero. Pre- and
 post-conditioning (one's complement) is performed within this
 function so it shouldn't be done by the caller. Usage example:

   unsigned long crc = 0L;

   while (read_buffer(buffer, length) != EOF) {
     crc = update_crc(crc, buffer, length);
   }
   if (crc != original_crc) error();
*/
static UINT32 rCRC32_update_crc ( const UINT32 crc, BYTE const * const buf, const UINT len )
{
  UINT32 c = crc ^ 0xffffffffL;
  if (!g_CRC32_table_computed) { rCRC32_make_table(); }
  for ( UINT n = 0; n < len; ++n ) {
    c = g_CRC32_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c ^ 0xffffffffL;
}
/* Return the CRC of the bytes buf[0..len-1]. */
static UINT32 rCRC32 ( BYTE const * const buf, const UINT len )
{
  return rCRC32_update_crc(0L, buf, len);
}

static UINT64 rAg47cs ( BYTE const * buf, UINT len )
{
  UINT64 n = 0;
  UINT k=0;
  while ( len )
  {
    n = ((*buf) + n) ^ ((*buf) << (32 + k*4));
    ++k;
    if ( k >= 8 ) k = 0;
    ++buf;
    --len;
  }
  return n;
}
