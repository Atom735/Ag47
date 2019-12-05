/*
  s.7*                  - массив символов, где
      [-2]              - общий размер памяти (включая нулевой символ) ( максимум для ASCII 255, для WIDE ~65k, для INT и PTR ~4g )
      [-1]              - длина строки ( в символах, исключая нулевой символ )
      [0...]            - сама строка
  sa7* - (CHAR)         ASCII строка
  su7* - (CHAR)         UTF8 строка
  sw7* - (WCHAR)        WIDE строка
  si7* - (INT)          INT  строка
  sp7* - (LPVOID)       PTR  строка состоящая из указателей
*/

/*
  Получает указатель на начало блока памяти со строкой
*/
#define rSA7_GetPtrHead(_s_) ((UINT8*)(((LPSTR)(_s_))-2))
#define rSU7_GetPtrHead(_s_) ((UINT8*)(((LPSTR)(_s_))-2))
#define rSW7_GetPtrHead(_s_) ((UINT16*)(((LPWSTR)(_s_))-2))
#define rSI7_GetPtrHead(_s_) ((UINT32*)(((PUINT32)(_s_))-2))
#define rSP7_GetPtrHead(_s_) ((UINT32*)(((LPVOID*)(_s_))-2))
#define rSA7_GetPtrBody(_s_) ((LPSTR)(((UINT8*)(_s_))+2))
#define rSU7_GetPtrBody(_s_) ((LPSTR)(((UINT8*)(_s_))+2))
#define rSW7_GetPtrBody(_s_) ((LPWSTR)(((UINT16*)(_s_))+2))
#define rSI7_GetPtrBody(_s_) ((PUINT32)(((UINT32*)(_s_))+2))
#define rSP7_GetPtrBody(_s_) ((LPVOID*)(((UINT32*)(_s_))+2))
/*
  Выделение места на стеке
*/
#define rSA7_alloca(_n_) rSA7_GetPtrBody((alloca((_n_+2))))
#define rSU7_alloca(_n_) rSU7_GetPtrBody((alloca((_n_+2))))
#define rSW7_alloca(_n_) rSW7_GetPtrBody((alloca((_n_+2)*2)))
#define rSI7_alloca(_n_) rSI7_GetPtrBody((alloca((_n_+2)*4)))
#define rSP7_alloca(_n_) rSP7_GetPtrBody((alloca((_n_+2)*sizeof(LPVOID))))
/*
  Выделение места на стеке под заданную строку
*/
#define rSA7_alloca_str(_a_) rSA7_GetPtrBody((LPSTR)(alloca(sizeof(_a_)+(2))))
#define rSU7_alloca_str(_a_) rSU7_GetPtrBody((LPSTR)(alloca(sizeof(_a_)+(2))))
#define rSW7_alloca_str(_a_) rSW7_GetPtrBody((LPWSTR)(alloca(sizeof(_a_)+(2)*2)))
/*
  Создаёт статическую переменную с заданной строкой
*/
#define DVar_SA7_str(_v_,_a_) LPSTR _v_=rSA7_alloca_str(_a_);memcpy(_v_,_a_,sizeof(_a_));rSA7_GetPtrHead(_v_)[1]=(rSA7_GetPtrHead(_v_)[0]=sizeof(_a_)/sizeof(*_a_))-1;
#define DVar_SU7_str(_v_,_a_) LPSTR _v_=rSU7_alloca_str(_a_);memcpy(_v_,_a_,sizeof(_a_));rSU7_GetPtrHead(_v_)[1]=(rSU7_GetPtrHead(_v_)[0]=sizeof(_a_)/sizeof(*_a_))-1;
#define DVar_SW7_str(_v_,_a_) LPWSTR _v_=rSW7_alloca_str(_a_);memcpy(_v_,_a_,sizeof(_a_));rSW7_GetPtrHead(_v_)[1]=(rSW7_GetPtrHead(_v_)[0]=sizeof(_a_)/sizeof(*_a_))-1;
/*
  Выделение места в куче
*/
#define rSA7_malloc(_n_) rSA7_GetPtrBody((malloc((_n_+2))))
#define rSU7_malloc(_n_) rSU7_GetPtrBody((malloc((_n_+2))))
#define rSW7_malloc(_n_) rSW7_GetPtrBody((malloc((_n_+2)*2)))
#define rSI7_malloc(_n_) rSI7_GetPtrBody((malloc((_n_+2)*4)))
#define rSP7_malloc(_n_) rSP7_GetPtrBody((malloc((_n_+2)*sizeof(LPVOID))))
/*
  Освобождение места из кучи
*/
#define rSA7_free(_s_) free(rSA7_GetPtrHead(_s_))
#define rSU7_free(_s_) free(rSU7_GetPtrHead(_s_))
#define rSW7_free(_s_) free(rSW7_GetPtrHead(_s_))
#define rSI7_free(_s_) free(rSI7_GetPtrHead(_s_))
#define rSP7_free(_s_) free(rSP7_GetPtrHead(_s_))




