
set(TEST_NAME "deepin-passkey-tests")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(GTest REQUIRED)

message(${PROJECT_SOURCE_DIR})

file(GLOB_RECURSE TEST_SRCS
    "${PROJECT_SOURCE_DIR}/src/common/*.c"
    "*.cpp"
)

add_executable(${TEST_NAME}
    ${TEST_SRCS}
)

target_include_directories(${TEST_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/src/
)
target_link_libraries(${TEST_NAME} PRIVATE
    ${GTEST_LIBRARIES}
    -lpthread
    -ldl
)

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} "--gtest_output=xml:${PROJECT_BINARY_DIR}/ut-test-report.xml")