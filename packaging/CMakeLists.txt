qt_add_executable(htmusic-pack
    pack/main.cpp
    ${PROJECT_SOURCE_DIR}/installer/engine/Archive.h
    ${PROJECT_SOURCE_DIR}/installer/engine/Archive.cpp
    ${PROJECT_SOURCE_DIR}/installer/engine/ArchiveWriter.cpp
    ${PROJECT_SOURCE_DIR}/installer/engine/Bytes.h
    ${PROJECT_SOURCE_DIR}/installer/engine/Digest.h
    ${PROJECT_SOURCE_DIR}/installer/engine/Digest.cpp
)

target_include_directories(htmusic-pack PRIVATE ${PROJECT_SOURCE_DIR}/installer/engine)

target_link_libraries(htmusic-pack PRIVATE Qt6::Core Zstd::Zstd bcrypt)

set_target_properties(htmusic-pack PROPERTIES WIN32_EXECUTABLE FALSE)

set(setup_file "${CMAKE_BINARY_DIR}/HyperTubeMusic-Setup-${PROJECT_VERSION}-x64.exe")

add_custom_command(
    OUTPUT "${setup_file}"
    COMMAND htmusic-pack
        --stub "$<TARGET_FILE:htmusic-setup-stub>"
        --runtime "${HT_MUSIC_STAGE_ROOT}/runtime"
        --payload "${HT_MUSIC_STAGE_ROOT}/app"
        --out "${setup_file}"
        --version "${PROJECT_VERSION}"
    DEPENDS htmusic-pack htmusic-setup-stub ht-music-stage ht-music-setup-stage
        "${HT_MUSIC_STAGE_ROOT}/app.stamp" "${HT_MUSIC_STAGE_ROOT}/runtime.stamp"
    VERBATIM)

add_custom_target(ht-music-installer DEPENDS "${setup_file}")
