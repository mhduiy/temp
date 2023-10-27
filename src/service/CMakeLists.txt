set(SECURITY_COMPILE_FLAGS "-W -Wall -fstack-protector-strong -D_FORTITY_SOURCE=1 -z noexecstack -pie -fPIC -z lazy")

add_compile_definitions(_GNU_SOURCE)

set(SERVICE "deepin-passkey")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gobject-2.0 gio-unix-2.0) # gdbus
pkg_check_modules(Fido REQUIRED IMPORTED_TARGET libfido2)
pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)
pkg_check_modules(JSONC REQUIRED IMPORTED_TARGET json-c)

set(CMAKE_C_FLAGS ${SECURITY_COMPILE_FLAGS})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

execute_process(COMMAND sh -c "${CMAKE_CURRENT_SOURCE_DIR}/xml2str.sh ${PROJECT_SOURCE_DIR}/file/xml/com.deepin.Passkey.xml"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE SRCS
    "*.c"
    "${PROJECT_SOURCE_DIR}/src/common/*.c"
    "${PROJECT_SOURCE_DIR}/src/decode/*.c"
    "serviceframe/*.c"
    "dev/*.c"
    "rp/*.c"
    ${CMAKE_CURRENT_BINARY_DIR}/com.deepin.Passkey.c
)

add_executable(${SERVICE}
    ${SRCS}
)

target_include_directories(${SERVICE} PUBLIC
    ${PROJECT_SOURCE_DIR}/src/
    PkgConfig::GLIB
    PkgConfig::Fido
    PkgConfig::JSONC
    PkgConfig::OPENSSL
)

target_link_libraries(${SERVICE} PRIVATE
    PkgConfig::GLIB
    PkgConfig::Fido
    PkgConfig::JSONC
    PkgConfig::OPENSSL
    -lpthread
)

# 手动命令，make tr-pot，生成pot
add_custom_target("tr-pot"
    COMMAND
        find src/service/ -name "*.c" > ${CMAKE_CURRENT_BINARY_DIR}/deepin-passkey-service-c.file
    COMMAND
        xgettext --from-code utf-8 --files-from=${CMAKE_CURRENT_BINARY_DIR}/deepin-passkey-service-c.file  -o ${PROJECT_SOURCE_DIR}/file/translate/service/deepin-passkey-service.pot
    COMMENT
        "to generate deepin-passkey-service.pot"
    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}
    VERBATIM
)

# 手动命令，make tr-po，生成po
add_custom_target("tr-po")
set(PO_LANGS zh_CN zh_TW)
foreach(PO_LANG ${PO_LANGS})
        add_custom_command(
        TARGET "tr-po"
        COMMAND
            msginit --locale=${PO_LANG} --input=deepin-passkey-service.pot
        COMMENT
            "to generate ${PO_LANG}.po"
        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}/file/translate/service/
        VERBATIM
    )
endforeach()

# 构建时自动生成mo
file(GLOB_RECURSE PO_FILES
    "${PROJECT_SOURCE_DIR}/file/translate/service/*.po"
)
foreach(PO ${PO_FILES})
    get_filename_component(PO_NAME ${PO} NAME_WLE)
    add_custom_command(
        TARGET ${SERVICE}
        COMMAND
            mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/mo/${PO_NAME}/LC_MESSAGES/
        COMMAND
            msgfmt -o ${CMAKE_CURRENT_BINARY_DIR}/mo/${PO_NAME}/LC_MESSAGES/deepin-passkey-service.mo ${PO}
        POST_BUILD
        VERBATIM
    )
endforeach()

install(TARGETS ${SERVICE} DESTINATION bin)

install(DIRECTORY DESTINATION share/deepin-passkey/cred/)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mo/ DESTINATION share/locale)
