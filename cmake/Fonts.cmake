find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(HT_MUSIC_FONT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/assets/fonts")
set(HT_MUSIC_FONT_SCRIPT "${PROJECT_SOURCE_DIR}/packaging/fonts/subset-fonts.py")
set(HT_MUSIC_FONT_ROOTS
    "${PROJECT_SOURCE_DIR}/app"
    "${PROJECT_SOURCE_DIR}/shared"
    "${PROJECT_SOURCE_DIR}/installer")
set(HT_MUSIC_FONT_NAMES RobotoFlex.ttf MaterialSymbolsRounded.ttf)

function(ht_music_fonts target)
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/fonts")

    set(originals "")
    set(subsets "")
    foreach(name ${HT_MUSIC_FONT_NAMES})
        list(APPEND originals "${HT_MUSIC_FONT_SOURCE_DIR}/${name}")
        list(APPEND subsets "${generated}/${name}")
    endforeach()

    get_target_property(sources ${target} SOURCES)
    set(scanned "")
    foreach(source ${sources})
        if(source MATCHES "\\.(qml|cpp|h|mm|js)$")
            cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} NORMALIZE
                OUTPUT_VARIABLE resolved)
            list(APPEND scanned "${resolved}")
        endif()
    endforeach()

    add_custom_command(
        OUTPUT ${subsets}
        COMMAND ${Python3_EXECUTABLE} "${HT_MUSIC_FONT_SCRIPT}"
            --source "${HT_MUSIC_FONT_SOURCE_DIR}"
            --output "${generated}"
            --scan ${HT_MUSIC_FONT_ROOTS}
        DEPENDS "${HT_MUSIC_FONT_SCRIPT}" ${originals} ${scanned}
        COMMENT "Subsetting bundled fonts for ${target}"
        VERBATIM)

    set_source_files_properties(${subsets} PROPERTIES GENERATED TRUE)

    qt_add_resources(${target} "ht_music_fonts"
        PREFIX "/fonts"
        BASE ${generated}
        FILES ${subsets}
    )
endfunction()
