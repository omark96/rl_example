set ROOT=%~dp0

rmdir /s /q "%ROOT%build\vendor\windows" 2>nul
mkdir "%ROOT%build\vendor\windows"

pushd "%ROOT%vendor\umka\src"
del *.O 2>nul
gcc -O2 -fno-strict-aliasing -DUMKA_STATIC -DUMKA_BUILD -DUMKA_EXT_LIBS -c umka_api.c umka_common.c umka_compiler.c umka_const.c umka_decl.c umka_expr.c umka_gen.c umka_ident.c umka_lexer.c umka_runtime.c umka_stmt.c umka_types.c umka_vm.c
if errorlevel 1 (popd &exit /b 1)
del "%ROOT%build\vendor\windows\libumka.a" 2>nul
for %%f in (*.o) do ar rcs "%ROOT%build\vendor\windows\libumka.a" %%f
del *.o
popd

pushd "%ROOT%vendor\raylib-6.0\src"
make PLATFORM=PLATFORM_DESKTOP
del "%ROOT%vendor\raylib-6.0\src\*.o"
move /y "%ROOT%vendor\raylib-6.0\src\libraylib.a" "%ROOT%build\vendor\windows\libraylib.a"
popd