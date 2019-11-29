.PHONY : all


CC := gcc


CPPFLAGS :=\
	-DWIN32_LEAN_AND_MEAN\
	-D_WIN32_WINNT=_WIN32_WINNT_WIN7\
	-DUNICODE\
#   -DNDEBUG\

CFLAGS :=\
	-Wall\
	-municode\
	-g\
#   -mwindows\

LDFLAGS :=\
	-Wall\
	-municode\
	-g\
	-larchive\


all : main.exe
	main

main.exe : src/main.c
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -O3


