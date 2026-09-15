function(ht_music_design_system target)
    target_link_libraries(${target} PRIVATE htmusic-shared htmusic-sharedplugin)

    qt_add_resources(${target} "ht_music_brand"
        PREFIX "/icons"
        BASE ${PROJECT_SOURCE_DIR}/assets
        FILES ${PROJECT_SOURCE_DIR}/assets/ht-music.svg
    )

    ht_music_fonts(${target})
endfunction()
