set ROOT=%~dp0

if not exist "%ROOT%build\vendor" mkdir "%ROOT%"build\vendor"

pushd "%ROOT%vendor\umka\src"
del *.O 2>nul
gcc -O2 -fno-strict-aliasing -DUMKA_STATIC -DUMKA_BUILD -DUMKA_EXT_LIBS -c umka_api.c umka_common.c umka_compiler.c umka_const.c umka_decl.c umka_expr.c umka_gen.c umka_ident.c umka_lexer.c umka_runtime.c umka_stmt.c umka_types.c umka_vm.c
if errorlevel 1 (popd &exit /b 1)
del "%ROOT%build\vendor\libumka.a" 2>nul
for %%f in (*.o) do ar rcs "%ROOT%build\vendor\libumka.a" %%f
del *.o
popd
