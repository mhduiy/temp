set(SECURITY_COMPILE_FLAGS "-W -Wall -fstack-protector-strong -D_FORTITY_SOURCE=1 -z noexecstack -pie -fPIC -z lazy")

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
# add_compile_definitions(DK_DEBUG_MODE)
# endif()
add_compile_definitions(_GNU_SOURCE)

set(SERVICE "deepin-passkey")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gobject-2.0 gio-unix-2.0) # gdbus
pkg_check_modules(Fido REQUIRED IMPORTED_TARGET libfido2)

set(CMAKE_C_FLAGS ${SECURITY_COMPILE_FLAGS})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

execute_process(COMMAND sh -c ${CMAKE_CURRENT_SOURCE_DIR}/xml2str.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE SRCS
    "*.c"
    "common/*.c"
    "decode/*.c"
    "dev/*.c"
    "rp/*.c"
    ${CMAKE_CURRENT_BINARY_DIR}/com.deepin.Passkey.c
)

add_executable(${SERVICE}
    ${SRCS}
)

target_include_directories(${SERVICE} PUBLIC
    PkgConfig::GLIB
    PkgConfig::Fido
)

target_link_libraries(${SERVICE} PRIVATE
    PkgConfig::GLIB
    PkgConfig::Fido
    -lpthread
)

install(TARGETS ${SERVICE} DESTINATION bin)

install(DIRECTORY DESTINATION share/deepin-passkey/cred/)
