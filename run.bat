gcc -std=c23 -DUMKA_STATIC -o main.exe ./src/main.c -Iinclude -Ivendor/umka/src -Lbuild/vendor -Llib -lraylib -lgdi32 -lwinmm -lumka
main.exe