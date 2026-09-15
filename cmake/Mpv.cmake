find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_check_modules(PC_MPV QUIET IMPORTED_TARGET mpv)
endif()

if(TARGET PkgConfig::PC_MPV)
    add_library(Mpv::Mpv ALIAS PkgConfig::PC_MPV)
    return()
endif()

find_path(MPV_INCLUDE_DIR
    NAMES mpv/client.h
    HINTS ${MPV_ROOT} $ENV{MPV_ROOT}
    PATH_SUFFIXES include)

find_library(MPV_LIBRARY
    NAMES mpv libmpv libmpv.dll
    HINTS ${MPV_ROOT} $ENV{MPV_ROOT}
    PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mpv
    REQUIRED_VARS MPV_LIBRARY MPV_INCLUDE_DIR
    FAIL_MESSAGE "libmpv was not found. Install it, or point MPV_ROOT at a libmpv development package.")

add_library(Mpv::Mpv UNKNOWN IMPORTED)
set_target_properties(Mpv::Mpv PROPERTIES
    IMPORTED_LOCATION "${MPV_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${MPV_INCLUDE_DIR}")

if(WIN32)
    find_file(MPV_RUNTIME_LIBRARY
        NAMES libmpv-2.dll mpv-2.dll libmpv.dll
        HINTS ${MPV_ROOT} $ENV{MPV_ROOT}
        PATH_SUFFIXES bin "")
    if(NOT MPV_RUNTIME_LIBRARY)
        message(FATAL_ERROR "libmpv-2.dll was not found next to the import library.")
    endif()
    mark_as_advanced(MPV_RUNTIME_LIBRARY)
endif()

mark_as_advanced(MPV_INCLUDE_DIR MPV_LIBRARY)
