qt_add_executable(ht-music
    main.cpp
    analysis/TrackAnalysis.h
    analysis/TrackAnalysis.cpp
    analysis/AnalysisStore.h
    analysis/AnalysisStore.cpp
    analysis/PcmReader.h
    analysis/PcmReader.cpp
    analysis/Loudness.h
    analysis/Loudness.cpp
    analysis/Tensor.h
    analysis/Tensor.cpp
    analysis/Gemm.h
    analysis/Gemm.cpp
    analysis/Activation.h
    analysis/Activation.cpp
    analysis/Fft.h
    analysis/Fft.cpp
    analysis/Spectrogram.h
    analysis/Spectrogram.cpp
    analysis/ModelWeights.h
    analysis/ModelWeights.cpp
    analysis/BatchNorm.h
    analysis/BatchNorm.cpp
    analysis/Convolution.h
    analysis/Convolution.cpp
    analysis/Linear.h
    analysis/Linear.cpp
    analysis/RmsNorm.h
    analysis/RmsNorm.cpp
    analysis/Rotary.h
    analysis/Rotary.cpp
    analysis/Attention.h
    analysis/Attention.cpp
    analysis/FeedForward.h
    analysis/FeedForward.cpp
    analysis/FrontendBlock.h
    analysis/FrontendBlock.cpp
    analysis/TransformerBlock.h
    analysis/TransformerBlock.cpp
    analysis/BeatNetwork.h
    analysis/BeatNetwork.cpp
    analysis/BeatRegion.h
    analysis/BeatRegion.cpp
    analysis/BeatTracker.h
    analysis/BeatTracker.cpp
    analysis/Structure.h
    analysis/Structure.cpp
    analysis/KeyEstimator.h
    analysis/KeyEstimator.cpp
    analysis/Analyzer.h
    analysis/Analyzer.cpp
    media/ArtworkProvider.h
    media/ArtworkProvider.cpp
    media/Browser.h
    media/Browser.cpp
    media/PlaybackController.h
    media/PlaybackController.cpp
    media/QueueSync.h
    media/QueueSync.cpp
    media/Radio.h
    media/Radio.cpp
    media/Lyrics.h
    media/Lyrics.cpp
    media/EpisodeDetails.h
    media/EpisodeDetails.cpp
    media/Track.h
    media/SearchSuggestions.h
    media/SearchSuggestions.cpp
    media/SearchHistory.h
    media/SearchHistory.cpp
    model/BrowseContentModel.h
    model/BrowseContentModel.cpp
    model/Item.h
    model/ItemActions.h
    model/SortOption.h
    model/Chip.h
    model/Shelf.h
    model/ItemModel.h
    model/ItemModel.cpp
    model/DownloadsModel.h
    model/DownloadsModel.cpp
    model/OfflineSearchModel.h
    model/OfflineSearchModel.cpp
    model/BrowseModel.h
    model/BrowseModel.cpp
    library/ImageCache.h
    library/ImageCache.cpp
    library/LibraryActions.h
    library/LibraryActions.cpp
    library/PlaylistTargets.h
    library/PlaylistTargets.cpp
    library/TrackStorage.h
    library/TrackStorage.cpp
    library/Uploads.h
    library/Uploads.cpp
    library/PlayLog.h
    library/PlayLog.cpp
    library/DownloadPlanner.h
    library/DownloadPlanner.cpp
    library/Downloads.h
    library/Downloads.cpp
    library/OfflineSearch.h
    library/OfflineSearch.cpp
    control/Notification.h
    control/ControlCenter.h
    control/ControlCenter.cpp
    control/DownloadActivity.h
    control/DownloadActivity.cpp
    update/SemanticVersion.h
    update/SemanticVersion.cpp
    update/ReleaseNote.h
    update/ReleaseSection.h
    update/ReleaseNotes.h
    update/ReleaseNotes.cpp
    update/Release.h
    update/Release.cpp
    update/PackageDownload.h
    update/PackageDownload.cpp
    update/Updater.h
    update/Updater.cpp
    diagnostics/Value.h
    diagnostics/Value.cpp
    diagnostics/Backend.h
    diagnostics/Diagnostics.h
    diagnostics/Diagnostics.cpp
    diagnostics/LogBridge.h
    diagnostics/LogBridge.cpp
    diagnostics/StateObserver.h
    diagnostics/StateObserver.cpp
    diagnostics/InteractionTracker.h
    diagnostics/InteractionTracker.cpp
    diagnostics/DiagnosticsSettings.h
    diagnostics/DiagnosticsSettings.cpp
    plugin/PluginInfo.h
    plugin/PluginSetting.h
    plugin/PluginSetting.cpp
    plugin/PluginStore.h
    plugin/PluginStore.cpp
    plugin/Plugin.h
    plugin/Plugin.cpp
    plugin/PluginRegistry.h
    plugin/PluginRegistry.cpp
    plugin/discord/DiscordIpc.h
    plugin/discord/DiscordIpc.cpp
    plugin/discord/DiscordPresence.h
    plugin/discord/DiscordPresence.cpp
    auth/Credentials.h
    auth/Credentials.cpp
    auth/FirefoxAgent.h
    auth/FirefoxAgent.cpp
    auth/WebLogin.h
    auth/WebLogin.cpp
    auth/Account.h
    auth/Account.cpp
    innertube/parsers/RendererReader.h
    innertube/parsers/RendererReader.cpp
    innertube/parsers/RendererParser.h
    innertube/parsers/RendererParser.cpp
    net/DownloadTransfer.h
    net/DownloadTransfer.cpp
    net/HttpClient.h
    net/HttpClient.cpp
    net/CookieJar.h
    net/CookieJar.cpp
    net/Connectivity.h
    net/Connectivity.cpp
    net/RateLimiter.h
    net/RateLimiter.cpp
    innertube/ClientRegistry.h
    innertube/ClientRegistry.cpp
    innertube/ContextBuilder.h
    innertube/ContextBuilder.cpp
    innertube/Endpoints.h
    innertube/Endpoints.cpp
    innertube/Session.h
    innertube/Session.cpp
    player/FormatPicker.h
    player/FormatPicker.cpp
    player/LiveManifest.h
    player/LiveManifest.cpp
    player/Fade.h
    player/MpvController.h
    player/MpvController.cpp
    player/PlaybackSettings.h
    player/PlaybackSettings.cpp
    player/AudioEngine.h
    player/AudioEngine.cpp
    player/TransitionPlan.h
    player/TransitionPlan.cpp
    player/TransitionPlanner.h
    player/TransitionPlanner.cpp
    player/TransitionRunner.h
    player/TransitionRunner.cpp
    player/PlayerScript.h
    player/PlayerScript.cpp
    player/StreamResolver.h
    player/StreamResolver.cpp
    player/VideoOutput.h
    player/VideoOutput.cpp
    player/VideoSurface.h
    player/VideoSurface.cpp
    player/PlaybackTrackingSeed.h
    player/PlaybackTrackingSeed.cpp
    player/PlaybackTracker.h
    player/PlaybackTracker.cpp
    platform/SingleInstance.h
    platform/SingleInstance.cpp
    platform/WindowState.h
    platform/WindowState.cpp
    platform/SessionStore.h
    platform/SessionStore.cpp
    platform/MediaControls.h
    platform/MediaControls.cpp
    platform/TrayIcon.h
    platform/TrayIcon.cpp
    platform/Desktop.h
    platform/Desktop.cpp
    platform/Clipboard.h
    platform/Clipboard.cpp
    platform/SystemSettings.h
    platform/SystemSettings.cpp
)

if(LINUX OR (UNIX AND NOT APPLE))
    target_sources(ht-music PRIVATE
        platform/MprisRootAdaptor.h
        platform/MprisRootAdaptor.cpp
        platform/MprisPlayerAdaptor.h
        platform/MprisPlayerAdaptor.cpp
        platform/MprisService.h
        platform/MprisService.cpp)
    target_compile_definitions(ht-music PRIVATE HT_MUSIC_MPRIS)
    target_link_libraries(ht-music PRIVATE Qt6::DBus)
elseif(APPLE)
    set(bundle_icon ${PROJECT_SOURCE_DIR}/assets/ht-music.icns)
    target_sources(ht-music PRIVATE
        platform/NowPlayingControls.h
        platform/NowPlayingControls.mm
        ${bundle_icon})
    set_source_files_properties(${bundle_icon} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(platform/NowPlayingControls.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
    target_compile_definitions(ht-music PRIVATE HT_MUSIC_NOW_PLAYING)
    target_link_libraries(ht-music PRIVATE
        "-framework AppKit"
        "-framework Foundation"
        "-framework MediaPlayer")
elseif(MSVC)
    target_sources(ht-music PRIVATE
        platform/SystemMediaControls.h
        platform/SystemMediaControls.cpp)
    target_compile_definitions(ht-music PRIVATE HT_MUSIC_SMTC)
    target_link_libraries(ht-music PRIVATE windowsapp)
endif()

if(HT_MUSIC_CRASH_REPORTING)
    target_sources(ht-music PRIVATE diagnostics/SentryBackend.cpp)
    target_link_libraries(ht-music PRIVATE sentry::sentry)
    add_dependencies(ht-music ${HT_MUSIC_CRASH_HANDLER} ${HT_MUSIC_CRASH_HANDLER_MODULE})
    set(crash_handler_files $<TARGET_FILE:${HT_MUSIC_CRASH_HANDLER}>)
    if(HT_MUSIC_CRASH_HANDLER_MODULE)
        list(APPEND crash_handler_files $<TARGET_FILE:${HT_MUSIC_CRASH_HANDLER_MODULE}>)
    endif()
    add_custom_command(TARGET ht-music POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${crash_handler_files} $<TARGET_FILE_DIR:ht-music>
        COMMAND_EXPAND_LISTS
        VERBATIM)
else()
    target_sources(ht-music PRIVATE diagnostics/NullBackend.cpp)
endif()

if(MSVC AND HT_MUSIC_SEPARATE_DEBUG_INFO)
    set(symbol_dir ${CMAKE_BINARY_DIR}/symbols)
    set(symbol_files $<TARGET_FILE:ht-music> $<TARGET_PDB_FILE:ht-music>)
    if(HT_MUSIC_CRASH_REPORTING)
        list(APPEND symbol_files
            $<TARGET_FILE:${HT_MUSIC_CRASH_HANDLER}> $<TARGET_PDB_FILE:${HT_MUSIC_CRASH_HANDLER}>)
    endif()
    add_custom_target(ht-music-symbols
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${symbol_dir}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${symbol_dir}
        COMMAND ${CMAKE_COMMAND} -E copy ${symbol_files} ${symbol_dir}
        COMMAND_EXPAND_LISTS
        VERBATIM)
    add_dependencies(ht-music-symbols ht-music ${HT_MUSIC_CRASH_HANDLER})
endif()

set_source_files_properties(
    analysis/Activation.cpp
    analysis/Attention.cpp
    analysis/BatchNorm.cpp
    analysis/BeatNetwork.cpp
    analysis/Convolution.cpp
    analysis/FeedForward.cpp
    analysis/Fft.cpp
    analysis/FrontendBlock.cpp
    analysis/Gemm.cpp
    analysis/Linear.cpp
    analysis/RmsNorm.cpp
    analysis/Rotary.cpp
    analysis/Spectrogram.cpp
    analysis/Tensor.cpp
    analysis/TransformerBlock.cpp
    PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O3>")

qt_add_resources(ht-music "innertube"
    PREFIX "/innertube"
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/innertube
    FILES innertube/clients.json
)

qt_add_resources(ht-music "models"
    PREFIX "/models"
    BASE ${PROJECT_SOURCE_DIR}/assets/models
    FILES ${PROJECT_SOURCE_DIR}/assets/models/beat-this-small0.bin
)

target_include_directories(ht-music PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/media
    ${CMAKE_CURRENT_SOURCE_DIR}/net
    ${CMAKE_CURRENT_SOURCE_DIR}/innertube
    ${CMAKE_CURRENT_SOURCE_DIR}/player
    ${CMAKE_CURRENT_SOURCE_DIR}/model
    ${CMAKE_CURRENT_SOURCE_DIR}/library
    ${CMAKE_CURRENT_SOURCE_DIR}/control
    ${CMAKE_CURRENT_SOURCE_DIR}/update
    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/discord
    ${CMAKE_CURRENT_SOURCE_DIR}/auth
    ${CMAKE_CURRENT_SOURCE_DIR}/innertube/parsers
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

set_source_files_properties(qml/components/Toasts.qml qml/components/Actions.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(ht-music
    URI HtMusic.App
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/HtMusic/App
    QML_FILES
        qml/Main.qml
        qml/components/PlayPauseButton.qml
        qml/components/BrowseFilter.qml
        qml/components/RevealingButton.qml
        qml/components/RevealingSelect.qml
        qml/components/RankedShelf.qml
        qml/components/RankedRow.qml
        qml/components/Artwork.qml
        qml/components/Marquee.qml
        qml/components/TrackSwitch.qml
        qml/components/AmbientArt.qml
        qml/components/SeekBar.qml
        qml/components/SearchBar.qml
        qml/components/NavRail.qml
        qml/components/NavItem.qml
        qml/components/Card.qml
        qml/components/Shelf.qml
        qml/components/SongRow.qml
        qml/components/PlayingIndicator.qml
        qml/components/PageView.qml
        qml/components/SectionHeader.qml
        qml/components/PlayerBar.qml
        qml/components/FullPlayer.qml
        qml/components/VideoStage.qml
        qml/components/VideoModeSwitch.qml
        qml/components/QueuePane.qml
        qml/components/LyricsPane.qml
        qml/pages/BrowsePage.qml
        qml/components/BrowseHeader.qml
        qml/components/BrowseRow.qml
        qml/components/ChipRow.qml
        qml/components/ChipSelect.qml
        qml/components/TopResultCard.qml
        qml/components/EpisodeRow.qml
        qml/components/PodcastHeader.qml
        qml/components/SpeedButton.qml
        qml/components/SkipButton.qml
        qml/components/EpisodeDetailsPane.qml
        qml/components/SkeletonBlock.qml
        qml/components/LoadingSkeleton.qml
        qml/components/Toasts.qml
        qml/components/ToastHost.qml
        qml/components/TransferNotice.qml
        qml/components/NotificationRow.qml
        qml/components/CookieShape.qml
        qml/components/ReleaseNoteRow.qml
        qml/components/ReleaseSheet.qml
        qml/components/ControlCenterPanel.qml
        qml/components/ControlCenterButton.qml
        qml/components/AccountButton.qml
        qml/components/ConnectionChip.qml
        qml/components/LoginSheet.qml
        qml/components/TextPrompt.qml
        qml/components/LikeButton.qml
        qml/components/LiveBadge.qml
        qml/components/ColorSourceRow.qml
        qml/components/BandSlider.qml
        qml/components/TuningPopup.qml
        qml/components/VolumeControl.qml
        qml/components/ColorPicker.qml
        qml/components/DownloadSettings.qml
        qml/components/SettingsSheet.qml
        qml/components/Actions.qml
        qml/components/ItemMenu.qml
        qml/components/PlaylistEditor.qml
        qml/components/PlaylistDrag.qml
        qml/components/PlaylistPicker.qml
        qml/components/SelectionBar.qml
        qml/components/MoodCard.qml
        qml/components/MoodShelf.qml
        qml/components/DestinationCard.qml
        qml/components/PluginsSheet.qml
        qml/components/PluginGlyph.qml
        qml/components/PluginRow.qml
        qml/components/PluginPage.qml
        qml/components/PluginSettingRow.qml
        qml/components/TrayMenu.qml
)

qt_target_qml_sources(ht-music
    QML_FILES
        plugin/discord/qml/DiscordCard.qml
    RESOURCES
        plugin/discord/assets/discord.svg
)

target_link_libraries(ht-music PRIVATE
    Qt6::Network
    Qt6::OpenGL
    Qt6::Widgets
    Qt6::WebEngineQuick
    Mpv::Mpv
)

ht_music_design_system(ht-music)

ht_music_windows_resources(ht-music
    DESCRIPTION "${HT_MUSIC_DISPLAY_NAME}"
    FILENAME "ht-music.exe")

set_target_properties(ht-music PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "${HT_MUSIC_DISPLAY_NAME}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${HT_MUSIC_ORGANIZATION_DOMAIN}.${PROJECT_NAME}"
    MACOSX_BUNDLE_ICON_FILE ht-music.icns)

qt_add_translations(ht-music
    SOURCE_TARGETS ht-music htmusic-shared
    TS_FILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/i18n
    PLURALS_TS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/i18n/ht-music_en.ts
    RESOURCE_PREFIX "/i18n"
    MERGE_QT_TRANSLATIONS
    LUPDATE_OPTIONS -locations relative -no-obsolete)

ht_music_stage(ht-music
    NAME app
    QMLDIRS ${PROJECT_SOURCE_DIR}/app/qml ${PROJECT_SOURCE_DIR}/shared/qml
    EXTRA_FILES ${MPV_RUNTIME_LIBRARY} ${PROJECT_SOURCE_DIR}/LICENSE ${crash_handler_files}
    PRUNE_STYLES Fusion Imagine Material Universal FluentWinUI3 Windows
    PRUNE qmltooling dxcompiler.dll dxil.dll resources/qtwebengine_devtools_resources.pak)

install(TARGETS ht-music
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .)

if(HT_MUSIC_CRASH_REPORTING AND (LINUX OR (UNIX AND NOT APPLE)))
    set_target_properties(${HT_MUSIC_CRASH_HANDLER} PROPERTIES INSTALL_RPATH "$ORIGIN/../../lib")
    install(TARGETS ${HT_MUSIC_CRASH_HANDLER}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/${PROJECT_NAME})
endif()

if(LINUX OR (UNIX AND NOT APPLE))
    install(FILES ht-music.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES ${PROJECT_SOURCE_DIR}/assets/ht-music.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
endif()
