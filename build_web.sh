ROOT=$(cd "$(dirname "$0")" && pwd)

rm -rf "$ROOT/build/bin/web" 
mkdir -p "$ROOT/build/bin/web"

emcc -std=gnu23 -O2 -DUMKA_STATIC -DPLATFORM_WEB -sMIN_WEBGL_VERSION=2 src/main.c -Ivendor/raylib-6.0/src -Ivendor/umka/src -Lbuild/vendor/web -lraylib -lumka -sUSE_GLFW=3 -sASYNCIFY -sGL_ENABLE_GET_PROC_ADDRESS -sALLOW_MEMORY_GROWTH -sSTACK_SIZE=5MB --preload-file games --preload-file defaultAssets --shell-file vendor/raylib-6.0/src/shell.html -o build/bin/web/index.html