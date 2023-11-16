set(PLUGIN_NAME "dcc-passkey-plugin")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE SRCS
    "*.h"
    "*.cpp"
    "module/passkey.qrc"
    "${PROJECT_SOURCE_DIR}/file/translate/dcc-passkey-plugin/dcc-passkey-plugin.qrc"
    "${PROJECT_SOURCE_DIR}/src/common/*.h"
    "${PROJECT_SOURCE_DIR}/src/common/*.c"
    "${PROJECT_SOURCE_DIR}/src/decode/*.h"
    "${PROJECT_SOURCE_DIR}/src/decode/*.c"
)

find_package(Qt5 COMPONENTS Core Widgets DBus Svg LinguistTools REQUIRED)
find_package(DdeControlCenter REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(DtkWidget REQUIRED)
find_package(PolkitQt5-1)
find_package(OpenSSL REQUIRED)

pkg_check_modules(DFrameworkDBus REQUIRED dframeworkdbus)
pkg_check_modules(QGSettings REQUIRED gsettings-qt)
pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)

add_library(${PLUGIN_NAME} SHARED
    ${SRCS}
)

target_include_directories(${PLUGIN_NAME} PUBLIC
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Svg_INCLUDE_DIRS}
    ${DtkWidget_INCLUDE_DIRS}
    ${DdeControlCenter_INCLUDE_DIR}
    ${DFrameworkDBus_INCLUDE_DIRS}
    ${QGSettings_INCLUDE_DIRS}
    ${Qt5DBus_INCLUDE_DIRS}
    module
    window
    ${PROJECT_SOURCE_DIR}/src/
)
target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Svg_LIBRARIES}
    ${DtkWidget_LIBRARIES}
    ${DdeControlCenter_LIBRARIES}
    ${DFrameworkDBus_LIBRARIES}
    ${QGSettings_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    PolkitQt5-1::Agent
    PkgConfig::OPENSSL
)

file(GLOB TS_FILES "${PROJECT_SOURCE_DIR}/file/translate/dcc-passkey-plugin/*.ts")
qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(translations ALL DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION share/dcc-passkey-plugin/translations)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-control-center/modules)
