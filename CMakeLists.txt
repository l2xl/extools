cmake_minimum_required(VERSION 3.5)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

project(exscratcher VERSION 0.1 LANGUAGES CXX)

#include(cmake/CPM.cmake)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#CPMAddPackage(
#        NAME "Qt"
#        VERSION "6.9.0"
#        GIT_TAG "v6.9.0"
#        GITHUB_REPOSITORY "qt/qt5")
#CPMAddPackage("gh:qt/qt5@6.9.0")

find_package(Qt6 6.9.0 CONFIG REQUIRED COMPONENTS Core Widgets Quick PATHS /usr/local/Qt-6.9.0)

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
        src/widget/scratch_widget.cpp
        src/widget/scratch_widget.h
        src/data/bybit.cpp
        src/data/bybit.hpp
        src/data/scheduler.cpp
        src/data/scheduler.hpp
        src/data/data_provider.cpp
        src/data/data_provider.hpp
        src/common/currency.hpp
        src/common/timedef.hpp
        src/data/bybit/stream.cpp
        src/data/bybit/stream.hpp
        src/data/bybit/data_manager.cpp
        src/data/bybit/data_manager.hpp
        src/data/bybit/error.hpp
        src/app/market_controller.cpp
        src/app/market_controller.hpp
        src/data/bybit/subscription.hpp
        src/widget/scratcher.hpp
        src/widget/quote_scratcher.cpp
        src/widget/quote_scratcher.hpp
)

qt_add_executable(exscratcher MANUAL_FINALIZATION ${PROJECT_SOURCES})

include_directories(
        .
        contrib
        src
        src/common
        src/app
        src/data
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(exscratcher PRIVATE Qt6::Widgets Qt6::Quick)
target_link_libraries(exscratcher PRIVATE ${Boost_LIBRARIES})
target_link_libraries(exscratcher PRIVATE OpenSSL::SSL)

include(GNUInstallDirs)
install(TARGETS exscratcher
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(exscratcher)
