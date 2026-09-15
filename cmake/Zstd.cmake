include(FetchContent)

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)
set(ZSTD_MULTITHREAD_SUPPORT ON CACHE BOOL "" FORCE)
set(ZSTD_USE_STATIC_RUNTIME ON CACHE BOOL "" FORCE)

FetchContent_Declare(zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG v1.5.7
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR build/cmake
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(zstd)

if(NOT TARGET Zstd::Zstd)
    add_library(Zstd::Zstd ALIAS libzstd_static)
endif()

function(ht_music_zstd_decoder target)
    set(lib "${zstd_SOURCE_DIR}/lib")
    target_sources(${target} PRIVATE
        ${lib}/common/debug.c
        ${lib}/common/entropy_common.c
        ${lib}/common/error_private.c
        ${lib}/common/fse_decompress.c
        ${lib}/common/xxhash.c
        ${lib}/common/zstd_common.c
        ${lib}/decompress/huf_decompress.c
        ${lib}/decompress/zstd_ddict.c
        ${lib}/decompress/zstd_decompress.c
        ${lib}/decompress/zstd_decompress_block.c
    )
    target_include_directories(${target} PRIVATE ${lib} ${lib}/common)
    target_compile_definitions(${target} PRIVATE ZSTD_DISABLE_ASM ZSTDLIB_VISIBLE=)
endfunction()
