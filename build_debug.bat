@REM  gcc -std=c23 -g -gdwarf-4 -O0 -DUMKA_STATIC -o raylib_basic_window_debug.exe ./src/main.c -Iinclude -Ivendor/umka/src -Lbuild/vendor -Llib -lraylib -lgdi32 -lwinmm -lumka
rmdir /s /q "%ROOT%build\bin\windows" 2>nul 
mkdir "%ROOT%build\bin\windows"
gcc -std=c23 -g -O0 -DUMKA_STATIC -o build/bin/windows/main_debug.exe ./src/main.c -Iinclude -Ivendor/umka/src -Lbuild/vendor/windows -lraylib -lgdi32 -lwinmm -lumka