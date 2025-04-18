cmake_minimum_required(VERSION 3.5)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

project(exscratcher VERSION 0.1 LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(Boost 1.83 REQUIRED COMPONENTS context coroutine)


set(PROJECT_SOURCES
        src/main.cpp
        src/config.hpp
        src/config.cpp
        src/app/mainwindow.cpp
        src/app/mainwindow.h
        src/app/mainwindow.ui
        src/app/market_widget.cpp
        src/app/market_widget.h
        src/data/bybit.cpp
        src/data/bybit.hpp
        src/data/scheduler.cpp
        src/data/scheduler.hpp
        src/data/data_provider.cpp
        src/data/data_provider.hpp
        src/common/currency.hpp
        src/data/bybit/stream.cpp
        src/data/bybit/stream.hpp
        src/data/bybit/data_manager.cpp
        src/data/bybit/data_manager.hpp
        src/data/bybit/error.hpp
        src/app/market_controller.cpp
        src/app/market_controller.hpp
        src/data/bybit/subscription.hpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(exscratcher
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET exscratcher APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(exscratcher SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(exscratcher
            ${PROJECT_SOURCES}
        )
    endif()
endif()

include_directories(
        contrib
        src
        src/common
        src/app
        src/data
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(exscratcher PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(exscratcher PRIVATE ${Boost_LIBRARIES})
target_link_libraries(exscratcher PRIVATE OpenSSL::SSL)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.exscratcher)
endif()
set_target_properties(exscratcher PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS exscratcher
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(exscratcher)
endif()
