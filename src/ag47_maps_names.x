#define _D7x(_a_,_w_,_s_)
#define _D7xA(_a_,_w_,_s_)
#define _D7xPost()

#include "ag47_maps.x"

_D7xPre(1200,"utf-16","Unicode UTF-16, little endian byte order (BMP of ISO 10646)")
_D7xPre(1201,"unicodeFFFE","Unicode UTF-16, big endian byte order")
_D7xPre(12000,"utf-32","Unicode UTF-32, little endian byte order")
_D7xPre(12001,"utf-32BE","Unicode UTF-32, big endian byte order")
_D7xPre(65001,"utf-8","Unicode (UTF-8)")

#undef _D7x
#undef _D7xA
#undef _D7xPost
