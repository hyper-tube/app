configure_file(core/BuildInfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.h @ONLY)

qt_add_library(htmusic-shared STATIC
    core/AppInfo.h
    core/AppInfo.cpp
    core/Fonts.h
    core/Fonts.cpp
    core/Json.h
    core/Json.cpp
    core/Localization.h
    core/Localization.cpp
    core/Logging.h
    core/Logging.cpp
    core/Paths.h
    core/Paths.cpp
    theme/Appearance.h
    theme/Appearance.cpp
    theme/ColorFile.h
    theme/ColorFile.cpp
    theme/ColorSource.h
    theme/ColorSource.cpp
    theme/ColorSourceInfo.h
    theme/ColorUtils.h
    theme/ColorUtils.cpp
    theme/Hct.h
    theme/Hct.cpp
    theme/MaterialColors.h
    theme/MaterialColors.cpp
    theme/Palette.h
    theme/Palette.cpp
    theme/PlasmaColors.h
    theme/PlasmaColors.cpp
    theme/PlatformColors.h
    theme/PlatformColors.cpp
    theme/Scheme.h
    theme/Scheme.cpp
    theme/SystemTheme.h
    theme/SystemTheme.cpp
    theme/TerminalColors.h
    theme/TerminalColors.cpp
    platform/WindowChrome.h
    platform/WindowChrome.cpp
    platform/WindowFrame.h
    platform/WindowFrame.cpp
)

if(WIN32)
    target_sources(htmusic-shared PRIVATE
        platform/NonClientFrame.h
        platform/NonClientFrame.cpp)
    target_compile_definitions(htmusic-shared PUBLIC HT_MUSIC_NON_CLIENT_FRAME)
    target_link_libraries(htmusic-shared PRIVATE user32 shell32 dwmapi)
elseif(APPLE)
    target_sources(htmusic-shared PRIVATE
        platform/UnifiedTitleBar.h
        platform/UnifiedTitleBar.mm
        theme/AppleColors.h
        theme/AppleColors.mm)
    target_compile_definitions(htmusic-shared PUBLIC HT_MUSIC_UNIFIED_TITLE_BAR)
    target_link_libraries(htmusic-shared PRIVATE
        "-framework AppKit"
        "-framework Foundation")
elseif(LINUX OR (UNIX AND NOT APPLE))
    target_link_libraries(htmusic-shared PRIVATE Qt6::DBus)
endif()

target_compile_definitions(htmusic-shared PUBLIC QT_MESSAGELOGCONTEXT)

target_include_directories(htmusic-shared PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/theme
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(htmusic-shared
    URI HtMusic
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/HtMusic
    QML_FILES
        qml/Theme.qml
        qml/components/StyledText.qml
        qml/components/ProgressBar.qml
        qml/components/Ripple.qml
        qml/components/RoundedMask.qml
        qml/components/Wordmark.qml
        qml/components/Sym.qml
        qml/components/RippleSurface.qml
        qml/components/MaterialToolTip.qml
        qml/components/CheckMark.qml
        qml/components/RadioMark.qml
        qml/components/IconButton.qml
        qml/components/PillButton.qml
        qml/components/ToggleSwitch.qml
        qml/components/SettingRow.qml
        qml/components/NoticeCard.qml
        qml/components/InputField.qml
        qml/components/SelectField.qml
        qml/components/Popover.qml
        qml/components/PopupMenu.qml
        qml/components/ModalSheet.qml
        qml/components/BusySpinner.qml
        qml/components/EmptyState.qml
        qml/components/ErrorState.qml
        qml/components/GlassSurface.qml
        qml/components/PageScrollBar.qml
        qml/components/ScrollWheel.qml
        qml/components/ScrollGlide.qml
        qml/components/MaterialTabs.qml
        qml/components/SegmentedTabs.qml
        qml/components/WindowTitleBar.qml
        qml/components/WindowButton.qml
        qml/components/WindowResizeGrip.qml
        qml/components/WindowResizeGrips.qml
)

qt_add_shaders(htmusic-shared "htmusic_shaders"
    PREFIX "/qt/qml/HtMusic"
    FILES
        qml/shaders/ripple.frag
        qml/shaders/roundedmask.frag
        qml/shaders/spinner.frag
)

target_link_libraries(htmusic-shared PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
)
