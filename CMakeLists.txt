cmake_minimum_required(VERSION 3.18)
project(rl_example)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(UMKA_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/vendor/umka/src)

add_library(umka
	${UMKA_SRC_ROOT}/umka_api.c
	${UMKA_SRC_ROOT}/umka_common.c
	${UMKA_SRC_ROOT}/umka_compiler.c
	${UMKA_SRC_ROOT}/umka_const.c
	${UMKA_SRC_ROOT}/umka_decl.c
	${UMKA_SRC_ROOT}/umka_expr.c
	${UMKA_SRC_ROOT}/umka_gen.c
	${UMKA_SRC_ROOT}/umka_ident.c
	${UMKA_SRC_ROOT}/umka_lexer.c
	${UMKA_SRC_ROOT}/umka_runtime.c
	${UMKA_SRC_ROOT}/umka_stmt.c
	${UMKA_SRC_ROOT}/umka_types.c
	${UMKA_SRC_ROOT}/umka_vm.c
)
target_include_directories(umka PUBLIC ${UMKA_SRC_ROOT})
target_compile_definitions(umka PRIVATE UMKA_BUILD PUBLIC UMKA_STATIC)

add_library(yyjson ${CMAKE_CURRENT_SOURCE_DIR}/vendor/yyjson/src/yyjson.c)
target_include_directories(yyjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor/yyjson/src)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/raylib)

add_executable(main WIN32 ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
target_link_libraries(main PRIVATE umka yyjson raylib)
if(WIN32)
    target_link_libraries(main PRIVATE gdi32)
endif()

if(EMSCRIPTEN)
    set_target_properties(main PROPERTIES OUTPUT_NAME index)
    set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
    target_compile_definitions(raylib PRIVATE PLATFORM_WEB GRAPHICS_API_OPENGL_ES3)
    target_compile_definitions(main PRIVATE PLATFORM_WEB)
    target_link_options(main PRIVATE
        -sUSE_GLFW=3
        -sMIN_WEBGL_VERSION=2
        -sGL_ENABLE_GET_PROC_ADDRESS
        -sALLOW_MEMORY_GROWTH
        -sSTACK_SIZE=5MB
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/games@games"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/defaultAssets@defaultAssets"
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/vendor/raylib/src/shell.html"
    )
endif()
    