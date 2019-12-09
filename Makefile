.PHONY : all


CC := gcc


CPPFLAGS := \
  -DWIN32_LEAN_AND_MEAN \
  -D_WIN32_WINNT=_WIN32_WINNT_WIN7 \
  -DUNICODE \

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

DEPENDS := \
  src/ag47_misc.c \
  src/ag47_log.c \
  src/ag47_tbl_rus_a.c \
  src/ag47_tbl_rus_b.c \
  src/ag47_arrays.c \
  src/ag47_fs.c \

OUT_EXE := main.exe

all : $(OUT_EXE)
	$(OUT_EXE)

$(OUT_EXE) : src/main.c $(DEPENDS)
	$(CC) -v -o $@ $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -O3
