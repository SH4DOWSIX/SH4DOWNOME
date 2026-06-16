cmake_minimum_required(VERSION 3.16)
project(SH4DOWNOME LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(BUILD_SHARED_LIBS OFF)

# Static linking flags - MinGW only (not on Android)
if(MINGW AND NOT ANDROID)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

if(TARGET Qt6::Core AND NOT Qt6Core_STATIC)
    message(WARNING "You are NOT using a static Qt build! The resulting executable will NOT be fully static.")
endif()

set(APP_VERSION "4.0.1")

find_package(Qt6 COMPONENTS Widgets Multimedia Svg Network Quick QuickControls2 QuickDialogs2 REQUIRED)

if (ANDROID)
    set(ANDROID_OPENSSL_CMAKE "$ENV{ANDROID_SDK_ROOT}/android_openssl/android_openssl.cmake")
    if (NOT EXISTS "${ANDROID_OPENSSL_CMAKE}" AND DEFINED ANDROID_SDK_ROOT)
        set(ANDROID_OPENSSL_CMAKE "${ANDROID_SDK_ROOT}/android_openssl/android_openssl.cmake")
    endif()
    if (NOT EXISTS "${ANDROID_OPENSSL_CMAKE}")
        set(ANDROID_OPENSSL_CMAKE "D:/Android/Sdk/android_openssl/android_openssl.cmake")
    endif()
    if (EXISTS "${ANDROID_OPENSSL_CMAKE}")
        include("${ANDROID_OPENSSL_CMAKE}")
    else()
        message(WARNING "Android OpenSSL helper not found; HTTPS update checks may fail on Android.")
    endif()
endif()

# --- MiniAudio integration (header-only, no external library needed) ---
set(MINIAUDIO_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/miniaudio.h")
if(NOT EXISTS "${MINIAUDIO_HEADER}")
    message(FATAL_ERROR "miniaudio.h not found! Download it from https://github.com/mackron/miniaudio and place it in your source tree.")
endif()

if (WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
endif()

qt_add_executable(SH4DOWNOME
    main.cpp

    # ── QML bridge (new) ───────────────────────────────────────────────────
    MetronomeController.cpp MetronomeController.h
    SectionListModel.cpp    SectionListModel.h
    BeatIndicatorItem.cpp   BeatIndicatorItem.h
    NoteImageProvider.cpp   NoteImageProvider.h
    CustomPatternEditor.cpp CustomPatternEditor.h
    androidinputdialog.cpp  androidinputdialog.h

    # ── Core engine (unchanged) ────────────────────────────────────────────
    metronomeengine.cpp metronomeengine.h
    presetmanager.cpp   presetmanager.h
    audioengine.cpp     audioengine.h
    subdivisionpattern.h subdivisionpattern.cpp
    noteassembler.h     noteassembler.cpp
    svgutils.cpp        svgutils.h
    updatechecker.cpp   updatechecker.h

    # ── Widget dialogs (custom subdivision editor — still used) ───────────
    customsubdivisiondialog.cpp customsubdivisiondialog.h
    patterneditorwidget.cpp     patterneditorwidget.h

    # ── Existing resources ─────────────────────────────────────────────────
    resources/resources.qrc

    # ── QML files ──────────────────────────────────────────────────────────
    qml/qml.qrc

    $<$<BOOL:${WIN32}>:${APP_ICON_RESOURCE}>
    ${MINIAUDIO_HEADER}
)

if (ANDROID AND COMMAND add_android_openssl_libraries)
    add_android_openssl_libraries(SH4DOWNOME)
endif()

if (WIN32)
    set_target_properties(SH4DOWNOME PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if (ANDROID)
    set_target_properties(SH4DOWNOME PROPERTIES
        QT_ANDROID_VERSION_NAME        "${APP_VERSION}"
        QT_ANDROID_VERSION_CODE        1
        QT_ANDROID_MIN_SDK_VERSION     26
        QT_ANDROID_TARGET_SDK_VERSION  34
        QT_ANDROID_COMPILE_SDK_VERSION 36
        QT_ANDROID_PACKAGE_SOURCE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()

target_include_directories(SH4DOWNOME PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SH4DOWNOME PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Svg
    Qt6::Network
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
)

target_compile_definitions(SH4DOWNOME PRIVATE APP_VERSION="${APP_VERSION}")

message(STATUS "MiniAudio header: ${MINIAUDIO_HEADER}")
