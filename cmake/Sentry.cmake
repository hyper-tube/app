option(HT_MUSIC_CRASH_REPORTING "Build crash reporting and diagnostics on sentry-native with Crashpad" ON)

if(NOT HT_MUSIC_CRASH_REPORTING)
    return()
endif()

include(FetchContent)

set(SENTRY_BACKEND crashpad CACHE STRING "" FORCE)
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SENTRY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SENTRY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SENTRY_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(SENTRY_INTEGRATION_QT OFF CACHE BOOL "" FORCE)
set(SENTRY_SCREENSHOT none CACHE STRING "" FORCE)

FetchContent_Declare(sentry
    URL https://github.com/getsentry/sentry-native/releases/download/0.16.6/sentry-native.zip
    URL_HASH SHA256=d35145daaafddc50c0c87ec564acf0ba9968e67b23981e7f57c702b2dd6f2ff1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM)

block(SCOPE_FOR VARIABLES)
    set(CMAKE_AUTOMOC OFF)
    FetchContent_MakeAvailable(sentry)
endblock()

set(HT_MUSIC_CRASH_HANDLER crashpad_handler)
if(WIN32)
    set(HT_MUSIC_CRASH_HANDLER_MODULE crashpad_wer)
endif()
