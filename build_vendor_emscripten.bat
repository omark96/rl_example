set ROOT=%~dp0

where emcc >nul 2>&1
if errorlevel 1 call C:\emsdk\emsdk_env.bat

if exist "%ROOT%build\vendor\web" rmdir /s /q "%ROOT%build\vendor\web"
mkdir "%ROOT%build\vendor\web"

pushd "%ROOT%vendor\umka\src"
del *.o 2>nul
emcc -c -O3 -fno-strict-aliasing -fvisibility=hidden -DUMKA_STATIC -DUMKA_BUILD -Wall -Wno-format-security umka_api.c umka_common.c umka_compiler.c umka_const.c umka_decl.c umka_expr.c umka_gen.c umka_ident.c umka_lexer.c umka_runtime.c umka_stmt.c umka_types.c umka_vm.c
emar rcs "%ROOT%build\vendor\web\libumka.a" *.o
del *.o 2>nul
popd

pushd "%ROOT%vendor\raylib-6.0\src\"
del *.o 2>nul
emcc -c rcore.c rshapes.c rtextures.c rtext.c rmodels.c raudio.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES3
emar rcs "%ROOT%build\vendor\web\libraylib.a" *.o
del *.o 2>nul
popd