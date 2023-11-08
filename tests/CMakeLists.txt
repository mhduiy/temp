
set(TEST_NAME "deepin-passkey-tests")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(GTest REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)
pkg_check_modules(JSONC REQUIRED IMPORTED_TARGET json-c)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gobject-2.0 gio-unix-2.0)
pkg_check_modules(Fido REQUIRED IMPORTED_TARGET libfido2)


file(GLOB_RECURSE TEST_SRCS
    "${PROJECT_SOURCE_DIR}/src/common/*.c"
    "${PROJECT_SOURCE_DIR}/src/decode/*.c"
    "${PROJECT_SOURCE_DIR}/src/service/err.c"
    "${PROJECT_SOURCE_DIR}/src/service/dev/*.c"
    "${PROJECT_SOURCE_DIR}/src/service/serviceframe/*.c"
    "${PROJECT_SOURCE_DIR}/src/service/servicesignal.c"
    "*.cpp"
)

add_executable(${TEST_NAME}
    ${TEST_SRCS}
)

target_include_directories(${TEST_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/src/
    ${PROJECT_SOURCE_DIR}/src/service
)
target_link_libraries(${TEST_NAME} PRIVATE
    ${GTEST_LIBRARIES}
    PkgConfig::GLIB
    PkgConfig::OPENSSL
    PkgConfig::JSONC
    PkgConfig::Fido
    -lpthread
    -ldl
)

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} "--gtest_output=xml:${PROJECT_BINARY_DIR}/ut-test-report.xml")