#!/bin/sh
ROOT=$(cd "$(dirname "$0")" && pwd)

rm -rf "$ROOT/build/vendor/web"
mkdir -p "$ROOT/build/vendor/web"

(
    cd "$ROOT/vendor/umka/src" || exit 1
    rm -f *.o
    emcc -c -O3 -fno-strict-aliasing -fvisibility=hidden -DUMKA_STATIC -DUMKA_BUILD -Wall -Wno-format-security umka_api.c umka_common.c umka_compiler.c umka_const.c umka_decl.c umka_expr.c umka_gen.c umka_ident.c umka_lexer.c umka_runtime.c umka_stmt.c umka_types.c umka_vm.c
    emar rcs "$ROOT/build/vendor/web/libumka.a" *.o
    rm -f *.o
)

(
    cd "$ROOT/vendor/raylib-6.0/src" || exit 1
    rm -f *.o
    emcc -c rcore.c rshapes.c rtextures.c rtext.c rmodels.c raudio.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES3
    emar rcs "$ROOT/build/vendor/web/librayliba.a" *.o
    rm -f *.o
)