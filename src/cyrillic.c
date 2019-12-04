﻿static const UINT g_nCyrillicPeriodTable[32] = {
  [L'О'-L'А'] = 10983,
  [L'Е'-L'А'] =  8483,
  [L'А'-L'А'] =  7998,
  [L'И'-L'А'] =  7367,
  [L'Н'-L'А'] =  6700,
  [L'Т'-L'А'] =  6318,
  [L'С'-L'А'] =  5473,
  [L'Р'-L'А'] =  4746,
  [L'В'-L'А'] =  4533,
  [L'Л'-L'А'] =  4343,
  [L'К'-L'А'] =  3486,
  [L'М'-L'А'] =  3203,
  [L'Д'-L'А'] =  2977,
  [L'П'-L'А'] =  2804,
  [L'У'-L'А'] =  2615,
  [L'Я'-L'А'] =  2001,
  [L'Ы'-L'А'] =  1898,
  [L'Ь'-L'А'] =  1735,
  [L'Г'-L'А'] =  1687,
  [L'З'-L'А'] =  1641,
  [L'Б'-L'А'] =  1592,
  [L'Ч'-L'А'] =  1450,
  [L'Й'-L'А'] =  1208,
  [L'Х'-L'А'] =   966,
  [L'Ж'-L'А'] =   940,
  [L'Ш'-L'А'] =   718,
  [L'Ю'-L'А'] =   639,
  [L'Ц'-L'А'] =   486,
  [L'Щ'-L'А'] =   361,
  [L'Э'-L'А'] =   331,
  [L'Ф'-L'А'] =   267,
  [L'Ъ'-L'А'] =    37,
};

// https://docs.microsoft.com/ru-ru/windows/win32/intl/code-page-identifiers

static const WCHAR g_ctCyrillicTable_855[128] = {
    // https://ru.wikipedia.org/wiki/CP855
  0x452,0x402,0x453,0x403,0x451,0x401,0x454,0x404,0x455,0x405,0x456,0x406,0x457,0x407,0x458,0x408,
  0x459,0x409,0x45A,0x40A,0x45B,0x40B,0x45C,0x40C,0x45E,0x40E,0x45F,0x40F,0x44E,0x42E,0x44A,0x42A,
  0x430,0x410,0x431,0x411,0x446,0x426,0x434,0x414,0x435,0x415,0x444,0x424,0x433,0x413,0xAB,0xBB,
  0x2591,0x2592,0x2593,0x2502,0x2524,0x445,0x425,0x438,0x418,0x2563,0x2551,0x2557,0x255D,0x439,0x419,0x2510,
  0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x43A,0x41A,0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0xA4,
  0x43B,0x41B,0x43C,0x41C,0x43D,0x41D,0x43E,0x41E,0x43F,0x2518,0x250C,0x2588,0x2584,0x41F,0x44F,0x2580,
  0x42F,0x440,0x420,0x441,0x421,0x442,0x422,0x443,0x423,0x436,0x416,0x432,0x412,0x44C,0x42C,0x2116,
  0xAD,0x44B,0x42B,0x437,0x417,0x448,0x428,0x44D,0x42D,0x449,0x429,0x447,0x427,0xA7,0x25A0,0xA0,
};

static const WCHAR g_ctCyrillicTable_866[128] = {
    // https://ru.wikipedia.org/wiki/CP866
  0x410,0x411,0x412,0x413,0x414,0x415,0x416,0x417,0x418,0x419,0x41A,0x41B,0x41C,0x41D,0x41E,0x41F,
  0x420,0x421,0x422,0x423,0x424,0x425,0x426,0x427,0x428,0x429,0x42A,0x42B,0x42C,0x42D,0x42E,0x42F,
  0x430,0x431,0x432,0x433,0x434,0x435,0x436,0x437,0x438,0x439,0x43A,0x43B,0x43C,0x43D,0x43E,0x43F,
  0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
  0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
  0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
  0x440,0x441,0x442,0x443,0x444,0x445,0x446,0x447,0x448,0x449,0x44A,0x44B,0x44C,0x44D,0x44E,0x44F,
  0x401,0x451,0x404,0x454,0x407,0x457,0x40E,0x45E,0xB0,0x2219,0xB7,0x221A,0x2116,0xA4,0x25A0,0xA0,
};

static const WCHAR g_ctCyrillicTable_1251[128] = {
    // https://ru.wikipedia.org/wiki/Windows-1251
  0x402,0x403,0x201A,0x453,0x201E,0x2026,0x2020,0x2021,0x20AC,0x2030,0x409,0x2039,0x40A,0x40C,0x40B,0x40F,
  0x452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x0,0x2122,0x459,0x203A,0x45A,0x45C,0x45B,0x45F,
  0xA0,0x40E,0x45E,0x408,0xA4,0x490,0xA6,0xA7,0x401,0xA9,0x404,0xAB,0xAC,0xAD,0xAE,0x407,
  0xB0,0xB1,0x406,0x456,0x491,0xB5,0xB6,0xB7,0x451,0x2116,0x454,0xBB,0x458,0x405,0x455,0x457,
  0x410,0x411,0x412,0x413,0x414,0x415,0x416,0x417,0x418,0x419,0x41A,0x41B,0x41C,0x41D,0x41E,0x41F,
  0x420,0x421,0x422,0x423,0x424,0x425,0x426,0x427,0x428,0x429,0x42A,0x42B,0x42C,0x42D,0x42E,0x42F,
  0x430,0x431,0x432,0x433,0x434,0x435,0x436,0x437,0x438,0x439,0x43A,0x43B,0x43C,0x43D,0x43E,0x43F,
  0x440,0x441,0x442,0x443,0x444,0x445,0x446,0x447,0x448,0x449,0x44A,0x44B,0x44C,0x44D,0x44E,0x44F,
};

static const WCHAR g_ctCyrillicTable_10007[128] = {
    // https://ru.wikipedia.org/wiki/MacCyrillic
  0x410,0x411,0x412,0x413,0x414,0x415,0x416,0x417,0x418,0x419,0x41A,0x41B,0x41C,0x41D,0x41E,0x41F,
  0x420,0x421,0x422,0x423,0x424,0x425,0x426,0x427,0x428,0x429,0x42A,0x42B,0x42C,0x42D,0x42E,0x42F,
  0x2020,0xB0,0xA2,0xA3,0xA7,0x2022,0xB6,0x406,0xAE,0xA9,0x2122,0x402,0x452,0x2260,0x403,0x453,
  0x221E,0xB1,0x2264,0x2265,0x456,0xB5,0x2202,0x408,0x404,0x454,0x407,0x457,0x409,0x459,0x40A,0x45A,
  0x458,0x405,0xAC,0x221A,0x192,0x2248,0x2206,0xAB,0xBB,0x2026,0xA0,0x40B,0x45B,0x40C,0x45C,0x455,
  0x2013,0x2014,0x201C,0x201D,0x2018,0x2019,0xF7,0x201E,0x40E,0x45E,0x40F,0x45F,0x2116,0x401,0x451,0x44F,
  0x430,0x431,0x432,0x433,0x434,0x435,0x436,0x437,0x438,0x439,0x43A,0x43B,0x43C,0x43D,0x43E,0x43F,
  0x440,0x441,0x442,0x443,0x444,0x445,0x446,0x447,0x448,0x449,0x44A,0x44B,0x44C,0x44D,0x44E,0xA4,
};

static const WCHAR g_ctCyrillicTable_28595[128] = {
    // https://ru.wikipedia.org/wiki/ISO_8859-5
  0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
  0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
  0xA0,0x401,0x402,0x403,0x404,0x405,0x406,0x407,0x408,0x409,0x40A,0x40B,0x40C,0xAD,0x40E,0x40F,
  0x410,0x411,0x412,0x413,0x414,0x415,0x416,0x417,0x418,0x419,0x41A,0x41B,0x41C,0x41D,0x41E,0x41F,
  0x420,0x421,0x422,0x423,0x424,0x425,0x426,0x427,0x428,0x429,0x42A,0x42B,0x42C,0x42D,0x42E,0x42F,
  0x430,0x431,0x432,0x433,0x434,0x435,0x436,0x437,0x438,0x439,0x43A,0x43B,0x43C,0x43D,0x43E,0x43F,
  0x440,0x441,0x442,0x443,0x444,0x445,0x446,0x447,0x448,0x449,0x44A,0x44B,0x44C,0x44D,0x44E,0x44F,
  0x2116,0x451,0x452,0x453,0x454,0x455,0x456,0x457,0x458,0x459,0x45A,0x45B,0x45C,0xA7,0x45E,0x45F,
};

#define kNCyrillicTables 5

static const LPCSTR g_aszCyrillicTableLocaleNames[] = {
  ".866",
  ".855",
  ".1251",
  ".10007",
  ".28595",
};
static const UINT g_aszCyrillicTableId[] = {
  866,
  855,
  1251,
  10007,
  28595,
};

static const LPCWSTR g_ctCyrillicTables[] = {
  g_ctCyrillicTable_866,
  g_ctCyrillicTable_855,
  g_ctCyrillicTable_1251,
  g_ctCyrillicTable_10007,
  g_ctCyrillicTable_28595,
};

/*
  Функция получения номера кодировки для однбайтовых данных совместимых с кодировкой ASCII
  @ p                   указатель на начало данных
  @ n                   количество данных
  @ return              номер кодировки из базы
*/
static UINT rGetCodePage ( BYTE const * p, UINT n )
{
  UINT u[kNCyrillicTables] = { };
  while ( n )
  {
    if ( (*p) & 0x80 )
    {
      for ( UINT i = 0; i < kNCyrillicTables; ++i )
      {
        const WCHAR w = g_ctCyrillicTables[i][ (*p) & 0x7f ];
        if ( w >= 0x410 && w <= 0x44F )
        {
          u[i] += g_nCyrillicPeriodTable[(w<0x430)?(w-0x410):(w-0x430)];
        }
      }
    }
    ++p; --n;
  }
  u[3] = u[3]*10/13;
  UINT k=0;
  for ( UINT i = 1; i < kNCyrillicTables; ++i )
  {
    if ( u[i] > u[k] )
    {
      k = i;
    }
  }
  return k;
}
