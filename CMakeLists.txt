cmake_minimum_required(VERSION 3.5)

project(layouteditor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Xml)

if (WIN32)
    # ==========================================
    # WINDOWS GSTREAMER SETUP
    # ==========================================
    # The official GStreamer Windows installer sets an environment variable.
    # Change MSVC to MINGW below if you are using the MinGW compiler instead!
    set(GST_ROOT $ENV{GSTREAMER_1_0_ROOT_MINGW_X86_64})

    if (NOT GST_ROOT)
        message(FATAL_ERROR "GStreamer environment variable not found! Did you install the GStreamer Windows development package?")
    endif()

    # 1. Set the Include Directories
    set(GST_INCLUDE_DIRS
        "${GST_ROOT}/include/gstreamer-1.0"
        "${GST_ROOT}/include/glib-2.0"
        "${GST_ROOT}/lib/glib-2.0/include"
    )

    # 2. Find the specific Library files (.lib or .a)
    find_library(GST_LIB_CORE  NAMES gstreamer-1.0 PATHS "${GST_ROOT}/lib" REQUIRED)
    find_library(GST_LIB_VIDEO NAMES gstvideo-1.0  PATHS "${GST_ROOT}/lib" REQUIRED)
    find_library(GST_LIB_APP   NAMES gstapp-1.0    PATHS "${GST_ROOT}/lib" REQUIRED)
    find_library(GLIB_LIB      NAMES glib-2.0      PATHS "${GST_ROOT}/lib" REQUIRED)
    find_library(GOBJECT_LIB   NAMES gobject-2.0   PATHS "${GST_ROOT}/lib" REQUIRED)

    # Bundle them into the same variable name Linux uses
    set(GST_LIBRARIES
        ${GST_LIB_CORE}
        ${GST_LIB_VIDEO}
        ${GST_LIB_APP}
        ${GLIB_LIB}
        ${GOBJECT_LIB}
    )
elseif (UNIX AND NOT APPLE)
    # ==========================================
    # LINUX GSTREAMER & X11 SETUP
    # ==========================================
    find_package(X11 REQUIRED)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(GST REQUIRED
        gstreamer-1.0
        gstreamer-video-1.0
        gstreamer-app-1.0
    )
endif()


set(PROJECT_SOURCES
        main.cpp
        resources.qrc
        mainwindow.h mainwindow.cpp
        layoutscene.h layoutscene.cpp
        resizableappitem.h resizableappitem.cpp
        zoneitem.h zoneitem.cpp
        rulerbar.h rulerbar.cpp
        snappingitemgroup.h snappingitemgroup.cpp
        guidelineitem.h guidelineitem.cpp
        snappingutils.h snappingutils.cpp
        layoutserializer.h layoutserializer.cpp
        propertiesdialog.h propertiesdialog.cpp
        officetoolbar.h officetoolbar.cpp
        newlayoutdialog.h newlayoutdialog.cpp
        settingsdialog.h settingsdialog.cpp
        constants.h
        laserpointeritem.h laserpointeritem.cpp
        windowselector.h windowselector.cpp
        projectorwindow.h projectorwindow.cpp
        mirroredappitem.h mirroredappitem.cpp
        drawingmanager.h drawingmanager.cpp
        crophandleitem.h crophandleitem.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(layouteditor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        draghandlewidget.h draghandlewidget.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET layouteditor APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(layouteditor SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(layouteditor
            ${PROJECT_SOURCES}
        )
    endif()
endif()

qt_add_resources(resources.qrc)

target_include_directories(layouteditor PRIVATE
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(layouteditor PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::Xml
    ${GST_LIBRARIES}
)

if (UNIX AND NOT APPLE)
    target_link_libraries(layouteditor PRIVATE X11::X11)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.layoutmanager)
endif()
set_target_properties(layouteditor PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS layouteditor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


if(MSVC)
    target_compile_options(layouteditor PRIVATE /Zc:lambda /permissive- /Zc:__cplusplus)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(layouteditor)
endif()
