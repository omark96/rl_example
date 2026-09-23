@REM  gcc -std=c23 -g -gdwarf-4 -O0 -DUMKA_STATIC -o raylib_basic_window_debug.exe ./src/main.c -Iinclude -Ivendor/umka/src -Lbuild/vendor -Llib -lraylib -lgdi32 -lwinmm -lumka

gcc -std=c23 -g -O0 -DUMKA_STATIC -o main_debug.exe ./src/main.c -Iinclude -Ivendor/umka/src -Lbuild/vendor/windows -lraylib -lgdi32 -lwinmm -lumka