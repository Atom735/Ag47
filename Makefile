.PHONY : all


CC := gcc


CPPFLAGS := \
  -DWIN32_LEAN_AND_MEAN \
  -D_WIN32_WINNT=_WIN32_WINNT_WIN7 \
  -DUNICODE \
  -IF:/ARGilyazeev/msys64/mingw64/include/libxml2\

CFLAGS := \
  -Wall \
  -municode \
  -mwindows \
  -g \

LDFLAGS := \
  -Wall \
  -municode \
  -mwindows \
  -g \
  -static \
  -lxml2 \
  -liconv \
  -lz \
  -llzma \
  -lws2_32 \

DEPENDS := \
  src/ag47_misc.c \
  src/ag47_log.c \
  src/ag47_map.c \
  src/ag47_arrays.c \
  src/ag47_settings.c \
  src/ag47_fs.c \
  src/ag47_parse_docx.c \
  src/ag47_parse_las.c \
  src/ag47_parse_txt.c \
  src/ag47_dbf.c \
  src/ag47_parse.c \

OUT_EXE := main.exe

all : $(OUT_EXE)
	$(OUT_EXE)

$(OUT_EXE) : src/main.c $(DEPENDS)
	$(CC) -v -o $@ $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -O3
