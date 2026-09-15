add_executable(htmusic-setup-stub WIN32
    stub/main.cpp
    engine/Archive.h
    engine/Archive.cpp
    engine/ArchiveReader.cpp
    engine/SafePath.h
    engine/SafePath.cpp
    engine/Bytes.h
    engine/Digest.h
    engine/Digest.cpp
)

ht_music_zstd_decoder(htmusic-setup-stub)

target_include_directories(htmusic-setup-stub PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/engine)

target_link_libraries(htmusic-setup-stub PRIVATE bcrypt shell32)

set_target_properties(htmusic-setup-stub PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    OUTPUT_NAME "ht-music-setup-stub")

ht_music_windows_resources(htmusic-setup-stub
    DESCRIPTION "${HT_MUSIC_DISPLAY_NAME} Setup"
    FILENAME "ht-music-setup.exe")

qt_add_executable(ht-music-setup
    main.cpp
    engine/Archive.h
    engine/Archive.cpp
    engine/ArchiveReader.cpp
    engine/SafePath.h
    engine/SafePath.cpp
    engine/Bytes.h
    engine/Digest.h
    engine/Digest.cpp
    engine/CommandLine.h
    engine/CommandLine.cpp
    engine/Job.h
    engine/Job.cpp
    engine/Manifest.h
    engine/Manifest.cpp
    engine/Options.h
    engine/Product.h
    engine/Product.cpp
    engine/Registration.h
    engine/Registration.cpp
    engine/Running.h
    engine/Running.cpp
    engine/Session.h
    engine/Session.cpp
    engine/Transaction.h
    engine/Transaction.cpp
)

target_include_directories(ht-music-setup PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/engine)

set_source_files_properties(${PROJECT_SOURCE_DIR}/LICENSE PROPERTIES QT_SKIP_TRANSLATION TRUE)

qt_add_resources(ht-music-setup "ht_music_setup_license"
    PREFIX "/setup"
    BASE ${PROJECT_SOURCE_DIR}
    FILES ${PROJECT_SOURCE_DIR}/LICENSE
)

qt_add_qml_module(ht-music-setup
    URI HtMusic.Setup
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/HtMusic/Setup
    QML_FILES
        qml/Main.qml
        qml/BrandHeader.qml
        qml/CheckRow.qml
        qml/WelcomePage.qml
        qml/UninstallPage.qml
        qml/LicensePage.qml
        qml/OptionsPage.qml
        qml/ScopeChoice.qml
        qml/ProgressPage.qml
        qml/FinishPage.qml
)

target_link_libraries(ht-music-setup PRIVATE
    Qt6::Gui
    Qt6::Quick
    Zstd::Zstd
    bcrypt
    shell32
    ole32
    propsys
)

ht_music_design_system(ht-music-setup)

ht_music_windows_resources(ht-music-setup
    DESCRIPTION "${HT_MUSIC_DISPLAY_NAME} Setup"
    FILENAME "ht-music-setup.exe")

set_target_properties(ht-music-setup PROPERTIES WIN32_EXECUTABLE TRUE)

qt_add_translations(ht-music-setup
    LUPDATE_TARGET ht-music-setup_lupdate
    LRELEASE_TARGET ht-music-setup_lrelease
    SOURCE_TARGETS ht-music-setup htmusic-shared
    TS_FILES
        i18n/ht-music-setup_en.ts
        i18n/ht-music-setup_ru.ts
        i18n/ht-music-setup_uk.ts
    RESOURCE_PREFIX "/i18n"
    MERGE_QT_TRANSLATIONS
    LUPDATE_OPTIONS -locations relative -no-obsolete)

ht_music_stage(ht-music-setup
    NAME runtime
    QMLDIRS ${PROJECT_SOURCE_DIR}/installer/qml ${PROJECT_SOURCE_DIR}/shared/qml
    PRUNE_STYLES Fusion Imagine Material Universal FluentWinUI3 Windows
    PRUNE qmltooling dxcompiler.dll dxil.dll)
