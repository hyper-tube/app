function(ht_music_windows_resources target)
    if(NOT WIN32)
        return()
    endif()

    cmake_parse_arguments(ARG "" "DESCRIPTION;FILENAME" "" ${ARGN})

    get_filename_component(HT_MUSIC_RC_INTERNAL_NAME "${ARG_FILENAME}" NAME_WE)
    set(HT_MUSIC_RC_DESCRIPTION "${ARG_DESCRIPTION}")
    set(HT_MUSIC_RC_ORIGINAL_FILENAME "${ARG_FILENAME}")
    set(HT_MUSIC_RC_ICON "${PROJECT_SOURCE_DIR}/assets/ht-music.ico")

    set(templates "${PROJECT_SOURCE_DIR}/packaging/windows")
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/${target}_windows")
    set(manifest "${generated}/${target}.manifest")

    file(TO_NATIVE_PATH "${HT_MUSIC_RC_ICON}" HT_MUSIC_RC_ICON)
    string(REPLACE "\\" "\\\\" HT_MUSIC_RC_ICON "${HT_MUSIC_RC_ICON}")

    configure_file("${templates}/executable.manifest.in" "${manifest}" @ONLY)
    configure_file("${templates}/executable.rc.in" "${generated}/${target}.rc" @ONLY)

    target_sources(${target} PRIVATE
        "${manifest}"
        "${generated}/${target}.rc")
    set_source_files_properties("${generated}/${target}.rc"
        PROPERTIES OBJECT_DEPENDS "${PROJECT_SOURCE_DIR}/assets/ht-music.ico")

    if(MSVC)
        target_link_options(${target} PRIVATE /MANIFEST:NO)
    endif()
endfunction()
