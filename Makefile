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

all : main.exe
	main

main.exe : src/main.c src/ag47_arrays.c
	$(CC) -v -o $@ $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -O3
