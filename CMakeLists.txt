cmake_minimum_required(VERSION 3.28)

cmake_policy(SET CMP0091 NEW)

project(ht-music
    VERSION 0.1.0
    DESCRIPTION "Native YouTube Music client"
    HOMEPAGE_URL "https://htm.nekolab.app"
    LANGUAGES C CXX)

set(HT_MUSIC_DISPLAY_NAME "HyperTube Music")
set(HT_MUSIC_REPOSITORY_URL "https://github.com/hyper-tube")
set(HT_MUSIC_ORGANIZATION_DOMAIN "nekolab.app")
set(HT_MUSIC_PUBLISHER "nekolab")
set(HT_MUSIC_COPYRIGHT "Copyright (C) 2026 nekolab. Licensed under the GNU GPL version 3.")
set(HT_MUSIC_APP_USER_MODEL_ID "nekolab.HyperTubeMusic")
set(HT_MUSIC_PRODUCT_FOLDER "HyperTube Music")
set(HT_MUSIC_UNINSTALL_KEY "HyperTubeMusic")
set(HT_MUSIC_URL_SCHEME "htmusic")

set(HT_MUSIC_DIAGNOSTICS_DSN "https://dac7349dc23943e0a424e27726ce1a56@htm-report.nekolab.app/1"
    CACHE STRING "GlitchTip project crash reports and diagnostics are sent to; empty sends nothing")
set(HT_MUSIC_DIAGNOSTICS_ENVIRONMENT "development"
    CACHE STRING "Environment reports are filed under; release builds pass production")
set_property(CACHE HT_MUSIC_DIAGNOSTICS_ENVIRONMENT PROPERTY STRINGS development production)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

set(HT_MUSIC_QT_COMPONENTS Core Gui OpenGL Widgets Quick QuickControls2 Network Svg ShaderTools WebEngineQuick LinguistTools)
if(LINUX OR (UNIX AND NOT APPLE))
    list(APPEND HT_MUSIC_QT_COMPONENTS DBus)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS ${HT_MUSIC_QT_COMPONENTS})

include(GNUInstallDirs)
include(cmake/DebugInfo.cmake)
include(cmake/BuildCommit.cmake)
include(cmake/Sentry.cmake)
include(cmake/Mpv.cmake)
include(cmake/WindowsResources.cmake)
include(cmake/Fonts.cmake)
include(cmake/DesignSystem.cmake)
include(cmake/Deploy.cmake)
if(WIN32)
    include(cmake/Zstd.cmake)
endif()

qt_standard_project_setup(REQUIRES 6.8
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES ru uk)

add_subdirectory(shared)
add_subdirectory(app)
if(WIN32)
    add_subdirectory(installer)
    add_subdirectory(packaging)
endif()
