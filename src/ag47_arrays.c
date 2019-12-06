/*
  s4...                 - массив элеаметнов, где
      [-2]              - общий размер памяти ( в количестве элементов )
      [-1]              - длина строки ( в количестве элементов, для строк исключая нулевой символ )
      [0...]            - сами данные
    Строки (вектора с символами)
  s4a     - (CHAR)      ASCII строка        ( max 255 )
  s4w     - (WCHAR)     WIDE строка         ( max ~65k )
  s4z     - (CHAR)      UTF8 строка         ( max ~65k )
          ^ [-3]        - размер памяти
          ^ [-2]        - количество занятых байт
          ^ [-1]        - количество символов
    Вектора с числами
  s4f     - (FLOAT)                         ( max ~4kT )
  s4d     - (DOUBLE)                        ( max ~4kT )
  s4i     - (INT)                           ( max ~4kT )
  s4u     - (UINT)                          ( max ~4kT )
  s4i8    - (INT8)                          ( max ~4kT )
  s4u8    - (UINT8)                         ( max ~4kT )
  s4i16   - (INT16)                         ( max ~4kT )
  s4u16   - (UINT16)                        ( max ~4kT )
  s4i32   - (INT32)                         ( max ~4kT )
  s4u32   - (UINT32)                        ( max ~4kT )
  s4i64   - (INT64)                         ( max ~4kT )
  s4u64   - (UINT64)                        ( max ~4kT )
    Вектор символьных строк
  ss4a    - (LPSTR)     ASCII строка        ( max ~4kT )
  ss4w    - (LPWSTR)    WIDE строка         ( max ~4kT )
  ss4z    - (LPSTR)     UTF8 строка         ( max ~4kT )
    Вектора с другими данными
  s4p     - (LPVOID)    вектор указателей   ( max ~4kT )
  s4s     - (struct)    вектор структур     ( max ~4kT )
          x86
          ^ [-4]        - указатель на таблицу функций
          ^ [-3]        - размер памяти
          ^ [-2]        - размер элемента
          ^ [-1]        - количество элементов
          x64
          ^ [-6]        - указатель на таблицу функций
          ^ [-5]        - указатель на таблицу функций
          ^ [-4]        - размер памяти
          ^ [-3]        - размер элемента
          ^ [-2]        - количество элементов
          ^ [-1]        - 0x78abcdef
*/

/* Структура заголовков векторов */
struct s4a_head
{
  UINT8                 memsz;
  UINT8                 count;
};
struct s4w_head
{
  UINT16                memsz;
  UINT16                count;
};
struct s4z_head
{
  UINT16                memsz;
  UINT16                count;
  UINT16                cch;
};
struct s4p_head
{
  UINT32                memsz;
  UINT32                count;
};
struct s4s_vtbl
{
  /* Функция освобождения данных структуры, где первым аргументом идёт указатель на элемент, а вторым на доп данные
  */
  void ( *func_free )( LPVOID p, LPVOID ptr );
  LPVOID                ptr;
};

struct s4s_head
{
  struct s4s_vtbl      *vtbl;
  UINT32                memsz;
  UINT32                count;
  UINT32                elemsz;
  #if (__SIZEOF_POINTER__==8)
  UINT32                _; // Выравнивание данных
  #endif
};

#define D4TypeOf(_p_,_t_) __builtin_choose_expr(__builtin_types_compatible_p(typeof(_p_), _t_),_p_,(void)0)

/*
  Получает указатель на начало блока памяти вектора
*/
#define D4GetPtrHead__(_p_,_h_)         ((_h_*)(((LPVOID)(_p_))-sizeof(_h_)))
#define D4GetPtrHead_s4a(_p_)           D4GetPtrHead__(_p_,struct s4a_head)
#define D4GetPtrHead_s4w(_p_)           D4GetPtrHead__(_p_,struct s4w_head)
#define D4GetPtrHead_s4z(_p_)           D4GetPtrHead__(_p_,struct s4z_head)
#define D4GetPtrHead_s4f(_p_)           D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4d(_p_)           D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4i(_p_)           D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4u(_p_)           D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4i8(_p_)          D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4u8(_p_)          D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4i16(_p_)         D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4u16(_p_)         D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4i32(_p_)         D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4u32(_p_)         D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4i64(_p_)         D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4u64(_p_)         D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_ss4a(_p_)          D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_ss4w(_p_)          D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_ss4z(_p_)          D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4p(_p_)           D4GetPtrHead__(_p_,struct s4p_head)
#define D4GetPtrHead_s4s(_p_)           D4GetPtrHead__(_p_,struct s4s_head)
/*
  Получает указатель на начало блока данных вектора
*/
#define D4GetPtrBody__(_p_,_h_,_b_)     ((_b_*)(((LPVOID)(_p_))+sizeof(_h_)))
#define D4GetPtrBody_s4a(_p_)           D4GetPtrBody__(_p_,struct s4a_head,CHAR  )
#define D4GetPtrBody_s4w(_p_)           D4GetPtrBody__(_p_,struct s4w_head,WCHAR )
#define D4GetPtrBody_s4z(_p_)           D4GetPtrBody__(_p_,struct s4z_head,CHAR  )
#define D4GetPtrBody_s4f(_p_)           D4GetPtrBody__(_p_,struct s4p_head,FLOAT )
#define D4GetPtrBody_s4d(_p_)           D4GetPtrBody__(_p_,struct s4p_head,DOUBLE)
#define D4GetPtrBody_s4i(_p_)           D4GetPtrBody__(_p_,struct s4p_head,INT   )
#define D4GetPtrBody_s4u(_p_)           D4GetPtrBody__(_p_,struct s4p_head,UINT  )
#define D4GetPtrBody_s4i8(_p_)          D4GetPtrBody__(_p_,struct s4p_head,INT8  )
#define D4GetPtrBody_s4u8(_p_)          D4GetPtrBody__(_p_,struct s4p_head,UINT8 )
#define D4GetPtrBody_s4i16(_p_)         D4GetPtrBody__(_p_,struct s4p_head,INT16 )
#define D4GetPtrBody_s4u16(_p_)         D4GetPtrBody__(_p_,struct s4p_head,UINT16)
#define D4GetPtrBody_s4i32(_p_)         D4GetPtrBody__(_p_,struct s4p_head,INT32 )
#define D4GetPtrBody_s4u32(_p_)         D4GetPtrBody__(_p_,struct s4p_head,UINT32)
#define D4GetPtrBody_s4i64(_p_)         D4GetPtrBody__(_p_,struct s4p_head,INT64 )
#define D4GetPtrBody_s4u64(_p_)         D4GetPtrBody__(_p_,struct s4p_head,UINT64)
#define D4GetPtrBody_ss4a(_p_)          D4GetPtrBody__(_p_,struct s4p_head,LPSTR )
#define D4GetPtrBody_ss4w(_p_)          D4GetPtrBody__(_p_,struct s4p_head,LPWSTR)
#define D4GetPtrBody_ss4z(_p_)          D4GetPtrBody__(_p_,struct s4p_head,LPSTR )
#define D4GetPtrBody_s4p(_p_)           D4GetPtrBody__(_p_,struct s4p_head,LPVOID)
#define D4GetPtrBody_s4s(_p_,_b_)       D4GetPtrBody__(_p_,struct s4s_head,_b_   )
/*
  Выделение места на стеке
*/
#define r4_alloca__(_h_,_b_,_n_,_c_)    D4GetPtrBody__(memcpy(alloca(sizeof(_h_)+(sizeof(_b_)*_n_)),&((_h_){.memsz=_n_,.count=_c_}),sizeof(_h_)),_h_,_b_)
#define r4_alloca_s4a(_n_)              r4_alloca__(struct s4a_head,CHAR  ,_n_,0)
#define r4_alloca_s4w(_n_)              r4_alloca__(struct s4w_head,WCHAR ,_n_,0)
#define r4_alloca_s4z(_n_)              r4_alloca__(struct s4z_head,CHAR  ,_n_,0)
#define r4_alloca_s4f(_n_)              r4_alloca__(struct s4p_head,FLOAT ,_n_,0)
#define r4_alloca_s4d(_n_)              r4_alloca__(struct s4p_head,DOUBLE,_n_,0)
#define r4_alloca_s4i(_n_)              r4_alloca__(struct s4p_head,INT   ,_n_,0)
#define r4_alloca_s4u(_n_)              r4_alloca__(struct s4p_head,UINT  ,_n_,0)
#define r4_alloca_s4i8(_n_)             r4_alloca__(struct s4p_head,INT8  ,_n_,0)
#define r4_alloca_s4u8(_n_)             r4_alloca__(struct s4p_head,UINT8 ,_n_,0)
#define r4_alloca_s4i16(_n_)            r4_alloca__(struct s4p_head,INT16 ,_n_,0)
#define r4_alloca_s4u16(_n_)            r4_alloca__(struct s4p_head,UINT16,_n_,0)
#define r4_alloca_s4i32(_n_)            r4_alloca__(struct s4p_head,INT32 ,_n_,0)
#define r4_alloca_s4u32(_n_)            r4_alloca__(struct s4p_head,UINT32,_n_,0)
#define r4_alloca_s4i64(_n_)            r4_alloca__(struct s4p_head,INT64 ,_n_,0)
#define r4_alloca_s4u64(_n_)            r4_alloca__(struct s4p_head,UINT64,_n_,0)
#define r4_alloca_ss4a(_n_)             r4_alloca__(struct s4p_head,LPSTR ,_n_,0)
#define r4_alloca_ss4w(_n_)             r4_alloca__(struct s4p_head,LPWSTR,_n_,0)
#define r4_alloca_ss4z(_n_)             r4_alloca__(struct s4p_head,LPSTR ,_n_,0)
#define r4_alloca_s4p(_n_)              r4_alloca__(struct s4p_head,LPVOID,_n_,0)
#define r4_alloca_s4s(_n_,_b_,_t_)      D4GetPtrBody_s4s(memcpy(alloca(sizeof(struct s4s_head)+(sizeof(_b_)*_n_)),&((struct s4s_head){.memsz=_n_,.count=0,.elemsz=sizeof(_b_)}),sizeof(struct s4s_head)),_b_)
/*
  Выделение места на стеке под заданную строку
*/
#define r4_alloca_init__(_s_,_h_,_b_,_n_)  memcpy(r4_alloca__(_h_,_b_,sizeof(_s_)/sizeof(*_s_),(sizeof(_s_)/sizeof(*_s_))-_n_),_s_,sizeof(_s_))
#define r4_alloca_init_s4a(_s_)         r4_alloca_init__(_s_,struct s4a_head,CHAR  ,1)
#define r4_alloca_init_s4w(_s_)         r4_alloca_init__(_s_,struct s4w_head,WCHAR ,1)
#define r4_alloca_init_s4z(_s_)         r4_alloca_init__(_s_,struct s4z_head,CHAR  ,1)
#define r4_alloca_init_s4f(_a_)         r4_alloca_init__(_a_,struct s4p_head,FLOAT ,0)
#define r4_alloca_init_s4d(_a_)         r4_alloca_init__(_a_,struct s4p_head,DOUBLE,0)
#define r4_alloca_init_s4i(_a_)         r4_alloca_init__(_a_,struct s4p_head,INT   ,0)
#define r4_alloca_init_s4u(_a_)         r4_alloca_init__(_a_,struct s4p_head,UINT  ,0)
#define r4_alloca_init_s4i8(_a_)        r4_alloca_init__(_a_,struct s4p_head,INT8  ,0)
#define r4_alloca_init_s4u8(_a_)        r4_alloca_init__(_a_,struct s4p_head,UINT8 ,0)
#define r4_alloca_init_s4i16(_a_)       r4_alloca_init__(_a_,struct s4p_head,INT16 ,0)
#define r4_alloca_init_s4u16(_a_)       r4_alloca_init__(_a_,struct s4p_head,UINT16,0)
#define r4_alloca_init_s4i32(_a_)       r4_alloca_init__(_a_,struct s4p_head,INT32 ,0)
#define r4_alloca_init_s4u32(_a_)       r4_alloca_init__(_a_,struct s4p_head,UINT32,0)
#define r4_alloca_init_s4i64(_a_)       r4_alloca_init__(_a_,struct s4p_head,INT64 ,0)
#define r4_alloca_init_s4u64(_a_)       r4_alloca_init__(_a_,struct s4p_head,UINT64,0)
#define r4_alloca_init_ss4a(_a_)        r4_alloca_init__(_a_,struct s4p_head,LPSTR ,0)
#define r4_alloca_init_ss4w(_a_)        r4_alloca_init__(_a_,struct s4p_head,LPWSTR,0)
#define r4_alloca_init_ss4z(_a_)        r4_alloca_init__(_a_,struct s4p_head,LPSTR ,0)
#define r4_alloca_init_s4p(_a_)         r4_alloca_init__(_a_,struct s4p_head,LPVOID,0)





