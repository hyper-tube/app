set(HT_MUSIC_STAGE_ROOT "${CMAKE_BINARY_DIR}/stage")

set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
include(InstallRequiredSystemLibraries)

function(ht_music_stage target)
    if(NOT WIN32)
        return()
    endif()
    cmake_parse_arguments(ARG "" "NAME" "QMLDIRS;EXTRA_FILES;PRUNE;PRUNE_STYLES" ${ARGN})

    set(stage "${HT_MUSIC_STAGE_ROOT}/${ARG_NAME}")
    set(stamp "${HT_MUSIC_STAGE_ROOT}/${ARG_NAME}.stamp")

    set(qmldirs "")
    foreach(dir ${ARG_QMLDIRS})
        list(APPEND qmldirs --qmldir "${dir}")
    endforeach()

    set(copies "")
    foreach(file ${ARG_EXTRA_FILES} ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        list(APPEND copies COMMAND ${CMAKE_COMMAND} -E copy_if_different "${file}" "${stage}")
    endforeach()

    set(prunes "")
    foreach(style ${ARG_PRUNE_STYLES})
        list(APPEND ARG_PRUNE
            "qml/QtQuick/Controls/${style}"
            "Qt6QuickControls2${style}.dll"
            "Qt6QuickControls2${style}StyleImpl.dll")
    endforeach()
    foreach(path ${ARG_PRUNE})
        list(APPEND prunes COMMAND ${CMAKE_COMMAND} -E rm -rf "${stage}/${path}")
    endforeach()

    add_custom_command(
        OUTPUT "${stamp}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${stage}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${stage}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${target}>" "${stage}"
        COMMAND Qt6::windeployqt
            --release
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            --no-compiler-runtime
            ${qmldirs}
            "${stage}/$<TARGET_FILE_NAME:${target}>"
        ${copies}
        ${prunes}
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        DEPENDS ${target} ${ARG_EXTRA_FILES} ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
        COMMENT "Staging ${ARG_NAME} into ${stage}"
        VERBATIM)

    add_custom_target(${target}-stage DEPENDS "${stamp}")
    set_property(TARGET ${target}-stage PROPERTY HT_MUSIC_STAGE_DIR "${stage}")
endfunction()
