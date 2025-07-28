# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

set(PROJECT_NAME dcc-passkey-plugin)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

# Find the library
find_package(PkgConfig REQUIRED)
find_package(DdeControlCenter REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets DBus Network LinguistTools Concurrent REQUIRED)
find_package(Dtk6 COMPONENTS Core Gui REQUIRED)

message(STATUS "dde-control-center version:" ${DdeControlCenter_VERSION})

set(Passkey_Name passkey)

file(GLOB_RECURSE passkey_SRCS
        "operation/*.cpp"
        "operation/*.h"
        "operation/*.qrc"
        "../common/*.h"
        "../common/*.cpp"
        "../decode/*.h"
        "../decode/*.c"
)

add_library(${Passkey_Name} MODULE
        ${passkey_SRCS}
)

target_include_directories(${Passkey_Name} PUBLIC
        Dde::Control-Center
)

set(Passkey_Libraries
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::DBus
        Qt6::Concurrent
        Qt::Network
        Dtk6::Core
        Dtk6::Gui
        Dde::Control-Center
)

target_link_libraries(${Passkey_Name} PRIVATE
        ${Passkey_Libraries}
)

target_include_directories(${Passkey_Name} PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

install(FILES "misc/org.deepin.dde.control-center.passkey.json" DESTINATION share/dsg/configs/org.deepin.dde.control-center)
dcc_install_plugin(NAME ${Passkey_Name} TARGET ${Passkey_Name})
dcc_handle_plugin_translation(NAME ${Passkey_Name} )