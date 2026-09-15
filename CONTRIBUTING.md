# Contributing to HyperTube Music

This document lists the rules every change is reviewed against. CI enforces formatting and static analysis, everything else is checked in review.

## Getting started

Build the application as described in [BUILDING.md](BUILDING.md). Use a `RelWithDebInfo` or `Debug` build for development.

On Linux, Qt logs to the systemd journal instead of stderr, so the application looks silent unless you force it:

```bash
QT_FORCE_STDERR_LOGGING=1 ./build/app/ht-music
```

Turn on the application's own log categories with `QT_LOGGING_RULES`, either all of them or one at a time:

```bash
QT_FORCE_STDERR_LOGGING=1 QT_LOGGING_RULES="htmusic.*=true" ./build/app/ht-music
```

The categories are `htmusic.theme`, `htmusic.playback`, `htmusic.artwork`, `htmusic.net`, `htmusic.innertube`, `htmusic.stream`, `htmusic.transition`, `htmusic.platform`, `htmusic.plugins` and `htmusic.diagnostics`. They are declared in `shared/core/Logging.cpp`.

Every run also writes a rotating log file:

| Platform | Log                                                        |
| -------- | ---------------------------------------------------------- |
| Linux    | `$XDG_STATE_HOME/ht-music/ht-music/logs/ht-music.log`      |
| Windows  | `%LOCALAPPDATA%/ht-music/ht-music/State/logs/ht-music.log` |
| macOS    | `~/Library/Logs/ht-music/ht-music.log`                     |

### Use a separate profile

Only one instance of the application can run at a time. Launching it again just brings the running window to the front. The instance is tied to the configuration directory, so on Linux you can run a development build next to your normal copy by giving it its own XDG directories:

```bash
P=/tmp/ht-music-dev
mkdir -p $P/{config,data,cache,state}
XDG_CONFIG_HOME=$P/config XDG_DATA_HOME=$P/data XDG_CACHE_HOME=$P/cache XDG_STATE_HOME=$P/state \
    QT_FORCE_STDERR_LOGGING=1 ./build/app/ht-music
```

- **It's not recommended to run a copy of your real profile with a network connection.** Google rotates session cookies on the requests the client makes and invalidates the previous values, so a copied profile that goes online can sign out the original.
- **Note that playing tracks while signed in reports listening activity to that account.** It affects the account's history and recommendations. You should test playback signed out or on an account you do not mind affecting.

## Project layout

| Path         | Contents                                                                            |
| ------------ | ----------------------------------------------------------------------------------- |
| `shared/`    | `htmusic-shared`, QML module `HtMusic`: core utilities, generic components etc.     |
| `app/`       | `ht-music`, QML module `HtMusic.App`: the application                               |
| `installer/` | `ht-music-setup`, QML module `HtMusic.Setup`: the Windows installer                 |
| `packaging/` | Font subsetting, packaging scripts for each platform, and the installer's pack tool |
| `cmake/`     | CMake modules                                                                       |
| `assets/`    | Brand icons and bundled fonts                                                       |
| `.github/`   | CI workflows, actions and scripts                                                   |

Directories should match C++ namespaces. Inside `app/`:

| Directory      | Contents                                                                |
| -------------- | ----------------------------------------------------------------------- |
| `net/`         | HTTP client, cookie jar, rate limiting, connectivity                    |
| `innertube/`   | InnerTube client registry, context builder, endpoints, renderer parsers |
| `model/`       | Items, shelves and browse models                                        |
| `media/`       | Tracks, `PlaybackController`, browsing, radio, lyrics, artwork          |
| `player/`      | Audio engine, mpv controller, stream resolution, playback settings      |
| `analysis/`    | Track analysis for smart transitions: loudness, beats, structure, key   |
| `auth/`        | Sign-in webview, credentials, account                                   |
| `library/`     | Image cache, library actions, playlists, downloads                      |
| `control/`     | The control center and its notifications                                |
| `platform/`    | Media controls, tray, single instance, session and window state         |
| `plugin/`      | The plugin system and one directory per plugin                          |
| `diagnostics/` | Opt-in crash reporting                                                  |
| `update/`      | Release checks and updates                                              |
| `qml/`         | Application components and pages                                        |

## Ground rules

- **Polluting source code with comments is generally discouraged.** You may add comments to clarify complex pieces of code, but most of the time, the code should be self-explanatory. Use descriptive names for variables, functions, and classes to convey their purpose.
- **ASCII only.** For the sake of consistency, source, documentation, UI strings and commit messages must be in ASCII. No em dashes, typographic quotes or ellipsis characters; write `-`, `->` and `...`. The translation catalogues under `app/i18n` and `installer/i18n` are the only files that contain anything else.
- **Every user-facing string must be added to the translation catalogues.** See [Translations](#translations).
- **Degrade instead of throwing.** Parsers should skip and log any errors or warnings, and a single unknown field shouldn't fail a whole response. A UI binding must never throw.
- **No business logic in QML.** Computation must always happen in C++. QML is for layout, bindings and motion.
- **Platform-specific code must live in C++.** Put it behind a single interface and let CMake compile the implementation for the current platform, like `platform::MediaControls` and `platform::WindowChrome` do. QML files must not check which platform they are running on. Anything the application saves to disk must get its location from `core::paths` instead of a hardcoded path.

## C++

The target is C++20 and Qt 6.

- Use one class per header, with `PascalCase.h` and `PascalCase.cpp` named after the class.
- Namespaces must match directories. A namespace's closing brace must not have any comment.
- Use `#pragma once` instead of include guards.
- Name members `m_camelCase`. Name free constants `kCamelCase` and put them in an anonymous namespace in the `.cpp` file, not in a header.
- If something is only used in one `.cpp` file, put it in an anonymous namespace there. Keep headers as small as possible.
- Order includes as follows: own header, project headers, Qt, standard library, with a blank line between each group.
- Make things `const` by default. Pass arguments by `const &` unless the type is trivially copyable, and return container members by `const &`.
- Use `QStringLiteral` for literals that become a `QString`, and `QLatin1String` for comparisons against one.
- Declare signals under `Q_SIGNALS:` and emit them with `Q_EMIT`. If a slot would only be used by a single connection, use a lambda instead. When the receiver is not `this`, always pass a context object as the third argument to `connect`.
- Setters must return early if the value hasn't changed, so the signal isn't emitted for nothing:

  ```cpp
  void Foo::setBar(int bar)
  {
      if (m_bar == bar)
          return;
      m_bar = bar;
      Q_EMIT barChanged();
  }
  ```

- Value types should be `Q_GADGET` with public members and `QML_VALUE_TYPE`. If you keep one in a `QList` behind a `MEMBER` property, it needs an `operator==`, otherwise the setter moc generates won't compile.
- Singletons exposed to QML use `QML_ELEMENT` together with `QML_SINGLETON`.

### CMake

- List source and QML files explicitly. Don't use globs.
- Add every source subdirectory to `target_include_directories`. Qt's generated QML type registration includes headers by their bare file name, and if it can't find one it silently skips it, which leads to a confusing build error later.

## QML

- Put one component in each file, name it `PascalCase.qml` and add it to its target's `qt_add_qml_module`.
- Order the contents of an object like this: `id`, declared properties, `readonly` derived properties, signals, bindings on base type properties, child objects, handlers, animations.
- Leave a blank line after `id:`. Keep ids short and lowerCamelCase. The root object's id must always be `root`.
- Mark properties computed from other properties as `readonly`. Properties that a parent is supposed to set should not be `readonly`.
- Prefix color role properties with `col`. QML treats a property named `onSurface` as a signal handler and the file won't load, while `colOnSurface` works fine.
- Declare every delegate property that comes from a model as a `required property`. Don't rely on implicit `modelData`.
- Prefer pointer handlers (`HoverHandler`, `TapHandler`, `DragHandler`) over `MouseArea`.
- Keep in mind that `parent` on a pointer handler is evaluated only once, when the handler is created, and is not a binding. If the target doesn't exist yet, set `parent` from code once the item is in a window.

## Design system

The UI was inspired by end-4's [illogical-impulse](https://github.com/end-4/dots-hyprland) Quickshell shell, which uses Material 3 Expressive. If you're not sure how something should look or behave, check how the material design guidelines suggest it should be done.

### Colors

- Don't hardcode colors. Use the `Theme.col*` roles instead. All 49 Material roles are available through `Palette`.
- Surfaces should use the following layers:

  | Role        | Material surface          | Use                   |
  | ----------- | ------------------------- | --------------------- |
  | `colLayer0` | `background`              | The window            |
  | `colLayer1` | `surfaceContainerLow`     | Raised panels         |
  | `colLayer2` | `surfaceContainer`        | The player bar, cards |
  | `colLayer3` | `surfaceContainerHigh`    | Controls on layer 2   |
  | `colLayer4` | `surfaceContainerHighest` | Controls on layer 3   |

### Motion

Use the curves and durations from `Theme`:

| Token                      | Duration (ms) | Use for                                                 |
| -------------------------- | ------------- | ------------------------------------------------------- |
| `expressiveEffects`        | 200           | Color, opacity, state layers                           |
| `expressiveFastSpatial`    | 350           | Small movement, press bounce                            |
| `expressiveDefaultSpatial` | 500           | Normal movement                                         |
| `expressiveSlowSpatial`    | 650           | Large, slow movement                                    |
| `emphasized`               | 500           | Sheets, panels, anything that travels across the screen |
| `emphasizedDecel`          | 400           | Small elements entering in place                        |
| `emphasizedAccel`          | 200           | Small elements leaving                                  |

- **Each transition must be driven by a single animated value.** Don't animate a property whose binding depends on another animated property: the two animations race each other and the parts drift apart mid-transition. Instead, animate one value from 0 to 1 and compute everything else from it with plain bindings:

  ```qml
  property real expansion: expanded ? 1 : 0

  Behavior on expansion {
      NumberAnimation { ... }
  }

  implicitWidth: Theme.size.railCollapsed + (Theme.size.railExpanded - Theme.size.railCollapsed) * expansion
  ```

- If something needs different curves for entering and exiting, use `states` and `transitions`. Don't put ternaries in a `Behavior`'s `duration` or `easing`, because the animation can start with the wrong curve.
- Properties that animate together should use the same duration and curve.
- Don't call `start()`, `stop()` or `restart()` on an animation whose `running` property is bound. Doing so removes the binding permanently.
- Popups should open from the point where they were triggered. When you add a new `Popover`, set both `anchorX` and `anchorY`.

### Components

- Use `StyledText` for all text, `Sym` for all icons and `RippleSurface` for anything clickable. Pass content to `RippleSurface` as children so the hover layer never covers it. Don't build your own `MouseArea` with a hover rectangle.
- For a filled button, set `colBackground` and `colState` together.
- Controls that only appear on hover should keep their space when hidden. Animate `opacity` and disable input with `interactive` instead of toggling `visible`, which shifts the layout around them.
- Don't bind a `Sym`'s `width` to its own `implicitWidth`, it causes a binding loop. Use `iconSize` instead.
- Check `shared/qml/components` and `app/qml/components` before writing a new component. If the installer could use it too, put it in `shared/`.
- To round anything that isn't a `Rectangle`, wrap the layers in a single `Item` and set `layer.effect: RoundedMask { ... }` on it. If the effect is only needed some of the time, bind `layer.enabled` to visibility.
- Put effects on the item's own `layer.effect`. Don't point an effect at a sibling through `source:`, because it keeps sampling an outdated texture after the item is resized.
- Don't import `Qt5Compat.GraphicalEffects`. Use `QtQuick.Effects` or the project's shaders in `shared/qml/shaders` instead.
- When blurring, render the content small and scale it up instead of blurring a large texture.
- Context menu entries come from `Actions`. Each page should have a single `ItemMenu`, and delegates should emit `menuRequested` instead of creating their own menu.
- Background tasks should report progress and errors in the control center, not with toasts.
- Write Material Symbols icon names as double-quoted string literals in `.qml`, `.cpp`, `.h`, `.mm` or `.js` files. The icon font is trimmed at build time to the names found this way, so an icon name built at runtime will be missing.

## Translations

The UI is translated into English, Russian and Ukrainian using Qt Linguist catalogues. The application's catalogues are in `app/i18n` and the installer's are in `installer/i18n`. Strings from `shared/` end up in both.

- Use `qsTr()` in QML and `tr()` in C++. For a literal in a table that gets translated later through `tr()`, use `QT_TRANSLATE_NOOP("<context>", ...)`.
- Don't build sentences by concatenating strings, because word order and plural rules differ between languages. Use placeholders instead:

  ```cpp
  tr("Added %n songs to %1", nullptr, count).arg(title)
  ```

- Strings that are only written to the log aren't user-facing and don't need to be translated.
- After adding or changing strings, update the catalogues and commit them together with your change.

  ```bash
  cmake --build build --target update_translations
  ```

  The installer is only built on Windows, so its catalogues can only be updated from a Windows build.

- If you add a string with `%n`, fill in its English plural forms in `app/i18n/ht-music_en.ts` (or `installer/i18n/ht-music-setup_en.ts`). The English catalogue only contains plurals, and without them the UI shows text like "1 songs".
- Translating into other languages is optional. Fill in the ones you know and leave the rest marked `unfinished`. Untranslated strings are shown in English until someone translates them. Please mention in the pull request description if you left any translations unfinished.

## Plugins

Plugins are built into the application.

- Each plugin lives in its own `app/plugin/<id>/` directory and derives from `plugin::Plugin`. It must implement `info()`, `start()` and `stop()`, and can optionally override `schema()`, `valueChanged()` and `restate()`.
- Register the plugin as a QML singleton and add it to the `PluginRegistry` constructor.
- List its source and QML files in `app/CMakeLists.txt` and add its directory to `target_include_directories`.
- Prefix the plugin's QML file names with its name, since QML type names are shared across the whole module: `DiscordCard.qml`, not `Card.qml`.
- Declare settings in `schema()`. Their values are saved in `QSettings` under `plugins/<id>/<key>`. Plugins must be disabled by default.
- Use `tr()` in `info()` and `schema()`. If the plugin caches a status string, recompute it in `restate()` so it updates when the language changes.
- Report the plugin's state with `setState(health, status)`. If something fails and the user needs to act on it, post a notification to the control center. Plugins must not throw.
- For the plugin's icon, either set `icon` to a Material Symbols name or ship an SVG and reference it with `Plugin::assetSource`.
- Plugins can currently add a card to the control center (`cardSource`) and a custom settings page (`settingsSource`). If your plugin needs to draw somewhere else, add the new slot in the same change as the plugin.

## Crash reporting and privacy

Crash reporting is opt-in. Reports must never contain anything that identifies the user or the content they listen to.

- Only `app/diagnostics/SentryBackend.cpp` may include `sentry.h`. All other code must go through `diagnostics/Diagnostics.h`.
- Log message text is never sent, only the category and source location of a warning. You can log whatever helps with debugging, but don't expect the message text to show up in a report.
- Never send titles, artists, video or playlist ids, search queries or other typed text, URLs, headers, cookies, file paths or account details.
- Only add a breadcrumb at the boundary of a subsystem, and only if the logs and automatic breadcrumbs don't already cover the event:

  ```cpp
  #include "diagnostics/Diagnostics.h"

  diagnostics::breadcrumb("player.decks_swapped",
                          {{"crossfade", crossfade},
                           {"client", diagnostics::Value::symbol(stream.clientKey)}},
                          diagnostics::Level::Warning);
  ```

  - The category must be a string literal in the form `subsystem.past_tense_event`, and keys must be `snake_case` string literals.
  - Values can be a `bool`, a number, a string literal or `diagnostics::Value::symbol(...)`. Passing a runtime string intentionally fails to compile.
  - Only use `symbol()` for identifiers from a fixed set defined in the code, like a client key, a codec or an enum name. Don't use it for anything a server or a user can influence. Video ids look like symbols but must never be sent.
  - Use breadcrumbs rather than `captureMessage` or `captureError` where possible, since every capture creates an issue that someone has to triage.
- Clicks in the UI are recorded using the control's `objectName`, QML id or component name. If two controls in the same place can't be told apart that way, give the component instance a fixed `objectName`. Never bind `objectName` to content.

## Python

Scripts in `.github/scripts` and `packaging/` target Python 3.12.

- Add type hints to every parameter and return value. Give every function, method and class a one-line PEP 257 docstring, written in the imperative and ending with a period.
- Import modules, not names from them: `import pathlib` and `pathlib.Path` instead of `from pathlib import Path`. This applies to third-party packages too.
- Scripts must end with `if __name__ == "__main__": sys.exit(main())`, and `main()` must return the exit code.

## Formatting and static analysis

CI runs clang-format, clang-tidy and clazy on every push and pull request, and any warning fails the job.

Format your code with the same clang-format version CI uses:

```bash
pip install clang-format==23.1.1
git ls-files '*.cpp' '*.h' '*.mm' | xargs clang-format -i
```

Run clang-tidy and clazy in a separate build directory configured with Clang, since a GCC build produces compiler flags that Clang doesn't understand:

```bash
cmake -S . -B build-tidy -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build-tidy

python3 .github/scripts/analyze.py --depends-on .clang-tidy build-tidy build-tidy/results/clang-tidy -- \
    clang-tidy -p build-tidy --quiet --warnings-as-errors='*'

python3 .github/scripts/analyze.py build-tidy build-tidy/results/clazy -- \
    clazy-standalone -p build-tidy \
        --checks="level1,no-non-pod-global-static,no-range-loop-detach,no-qproperty-without-notify,no-container-anti-pattern" \
        --header-filter='.*/(app|shared)/.*' \
        --extra-arg="-resource-dir=$(clang -print-resource-dir)"
```

`analyze.py` only re-checks files that changed since the last successful run, so running it again on an unchanged tree finishes immediately.

- CI uses LLVM 23. Older clang-tidy versions miss some warnings that 23 reports, so code that passes locally can still fail in CI. Avoid clang-tidy 21, which is much slower.
- If you apply `clang-tidy --fix`, rebuild and run the application afterwards. Some fixes compile fine but change behavior.
- Note that some checks are disabled on purpose because they conflict with Qt or with this codebase.

## Verifying a change

- Your change must not add any new build warnings.
- The application must start with an empty log. Any QML warning is considered a defect. The following lines are expected and can be ignored:
  - `qt.qpa.services` failing to register with the desktop portal when running from the build tree
  - `htmusic.*` informational lines when you enabled them
  - `js: Failed to create WebGPU Context Provider`, printed by the sign-in webview
- Check any visual change on screen yourself, in both light and dark mode, and in a few different languages if applicable.

## Commits and pull requests

Commits must follow [Conventional Commits](https://www.conventionalcommits.org) with a few additional rules (see below). This is strictly enforced because release notes are generated from them with git-cliff, and a commit that does not follow the format is left out.

Examples of valid commit messages:

```
feat(player): add a sleep timer
fix(search): keep the scope when a query is cleared
imp(downloads): resume transfers after the network returns
```

| Type       | Use for                                              |
| ---------- | ---------------------------------------------------- |
| `feat`     | A new feature                                        |
| `fix`      | A bug fix                                            |
| `imp`      | An improvement to an existing feature                |
| `perf`     | A performance improvement                            |
| `refactor` | A change that neither fixes a bug nor adds a feature |
| `docs`     | Documentation                                        |
| `build`    | The build system, packaging, dependencies            |
| `ci`       | CI workflows, actions and scripts                    |
| `style`    | Formatting only                                      |
| `chore`    | Anything else                                        |
| `revert`   | Reverting an earlier commit                          |

- The scope is optional. It names the part of the project the commit touches, usually the directory: `player`, `plugins`, `installer`.
- Use `chore(l10n): ...` for commits that only change translations. They are left out of release notes.
- Mark a breaking change with `!` after the type or a `BREAKING CHANGE:` footer.
- Commit messages must be ASCII only.

Both CI jobs, Format and Compile and analyze, must pass before a pull request can be merged. Explicitly state in the description what you verified and on which platform.
