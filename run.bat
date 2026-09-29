rmdir /s /q "%ROOT%build\bin\windows" 2>nul 
mkdir "%ROOT%build\bin\windows"
gcc -std=c23 -DUMKA_STATIC -o build/bin/windows/main.exe ./src/main.c -Iinclude -Ivendor/umka/src -Lbuild/vendor/windows -Llib -lraylib -lgdi32 -lwinmm -lumka
.\build\bin\windows\main.exe