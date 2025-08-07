cmake_minimum_required(VERSION 3.16)
project(SH4DOWNOME LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

if(TARGET Qt6::Core AND NOT Qt6Core_STATIC)
    message(WARNING "You are NOT using a static Qt build! The resulting executable will NOT be fully static.")
endif()

find_package(Qt6 COMPONENTS Widgets Multimedia Svg REQUIRED)

if (WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
endif()

add_executable(SH4DOWNOME
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    metronomeengine.cpp metronomeengine.h
    presetmanager.cpp presetmanager.h
    beatindicatorwidget.cpp beatindicatorwidget.h
    subdivisionselectordialog.h subdivisionselectordialog.cpp
    timesignaturedialog.h timesignaturedialog.cpp
    obsbeatwidget.cpp obsbeatwidget.h
    sectiontablewidget.cpp sectiontablewidget.h
    settingsdialog.h settingsdialog.cpp
    svgutils.cpp svgutils.h
    polyrhythmdialog.cpp polyrhythmdialog.h
    resources/resources.qrc
    $<$<BOOL:${WIN32}>:${APP_ICON_RESOURCE}>
)

if (WIN32)
    set_target_properties(SH4DOWNOME PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(SH4DOWNOME PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(SH4DOWNOME
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Svg
)