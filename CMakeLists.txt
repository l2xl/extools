# Scratcher project
# Copyright (c) 2025 l2xl (l2xl/at/proton.me)
# Distributed under the Intellectual Property Reserve License (IPRL)
# -----BEGIN PGP PUBLIC KEY BLOCK-----
#
# mDMEYdxcVRYJKwYBBAHaRw8BAQdAfacBVThCP5QDPEgSbSIudtpJS4Y4Imm5dzaN
# lM1HTem0IkwyIFhsIChsMnhsKSA8bDJ4bEBwcm90b25tYWlsLmNvbT6IkAQTFggA
# OBYhBKRCfUyWnduCkisNl+WRcOaCK79JBQJh3FxVAhsDBQsJCAcCBhUKCQgLAgQW
# AgMBAh4BAheAAAoJEOWRcOaCK79JDl8A/0/AjYVbAURZJXP3tHRgZyYyN9txT6mW
# 0bYCcOf0rZ4NAQDoFX4dytPDvcjV7ovSQJ6dzvIoaRbKWGbHRCufrm5QBA==
# =KKu7
# -----END PGP PUBLIC KEY BLOCK-----

cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

project(exscratcher VERSION 0.1 LANGUAGES CXX)

include(cmake/CPM.cmake)

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

find_package(Qt6 6 CONFIG REQUIRED COMPONENTS Core Widgets Svg)

find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(Boost 1.83 REQUIRED COMPONENTS context system)

# Add Qlementine style library
#CPMAddPackage(
#    NAME Qlementine
#    GITHUB_REPOSITORY oclero/qlementine
#    GIT_TAG v1.2.2
#    OPTIONS
#        "QLEMENTINE_BUILD_EXAMPLES OFF"
#        "QLEMENTINE_BUILD_TESTS OFF"
#)


set(PROJECT_SOURCES
        src/main.cpp
        src/config.hpp
        src/config.cpp
        src/app/trade_cockpit.cpp
        src/app/trade_cockpit.h
        src/app/trade_cockpit.ui
        src/app/view_controller.hpp
        src/widget/scratch_widget.cpp
        src/widget/scratch_widget.h
        src/widget/content_frame.h
        src/widget/content_frame.cpp
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
        src/widget
)

target_link_libraries(exscratcher PRIVATE Qt6::Widgets Qt6::Svg)
target_link_libraries(exscratcher PRIVATE ${Boost_LIBRARIES})
target_link_libraries(exscratcher PRIVATE OpenSSL::SSL)
#target_link_libraries(exscratcher PRIVATE qlementine)

include(GNUInstallDirs)
install(TARGETS exscratcher
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(exscratcher)

if(NOT ANDROID)

    CPMAddPackage("gh:catchorg/Catch2@3.5.4")
    #find_package(Catch2 3 REQUIRED)

    # Function to create test executable for each test file
    function(add_test TEST_NAME TEST_FILE)
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} PRIVATE Catch2::Catch2WithMain)
    endfunction()

    # Add each test file as a separate executable
    add_test(test_currency test/common/test_currency.cpp)

endif()

