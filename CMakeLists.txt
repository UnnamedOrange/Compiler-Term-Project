cmake_minimum_required(VERSION 3.13)

project(
    compiler
    VERSION 0.0.0
    LANGUAGES C CXX
)

# Find packages.
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://gitee.com/onlynagesha/argparse.git
    # GIT_TAG v2.9
)
FetchContent_MakeAvailable(argparse)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://gitee.com/chooosky/fmt.git
    GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

# Package options.
set(COMPILER_LINK_KOOPA
    ON
    CACHE BOOL "Link koopa"
)

# Directory options. DO NOT CHANGE THE CODES HERE.
set(INC_DIR
    "$ENV{CDE_INCLUDE_PATH}"
    CACHE PATH "Directory of includes"
)
set(LIB_DIR
    "$ENV{CDE_LIBRARY_PATH}/native"
    CACHE PATH "Directory of libraries"
)
message(STATUS "Include directory: ${INC_DIR}")
message(STATUS "Library directory: ${LIB_DIR}")

# Generate lexer/parser.
file(GLOB_RECURSE L_SOURCES "src/*.l")
file(GLOB_RECURSE Y_SOURCES "src/*.y")
if(NOT (L_SOURCES STREQUAL "" AND Y_SOURCES STREQUAL ""))
    string(REGEX REPLACE ".*/(.*)\\.l" "${CMAKE_CURRENT_BINARY_DIR}/\\1.lex.cpp" L_OUTPUTS
                         "${L_SOURCES}"
    )
    string(REGEX REPLACE ".*/(.*)\\.y" "${CMAKE_CURRENT_BINARY_DIR}/\\1.tab.cpp" Y_OUTPUTS
                         "${Y_SOURCES}"
    )
    flex_target(Lexer ${L_SOURCES} ${L_OUTPUTS})
    bison_target(Parser ${Y_SOURCES} ${Y_OUTPUTS})
    add_flex_bison_dependency(Lexer Parser)
endif()

# Create executable target.
add_executable(${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PRIVATE c_std_11 cxx_std_20)
# Set warning level.
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4) # /WX
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic) # -Werror
    # Disable warnings caused by old version of Flex.
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-register)
endif()
# Addtional include directories.
target_include_directories(${PROJECT_NAME} PRIVATE "src")
target_include_directories(${PROJECT_NAME} PRIVATE "${INC_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
# Additional link directories.
target_link_directories(${PROJECT_NAME} PRIVATE "${LIB_DIR}")

# Add sources.
file(GLOB_RECURSE C_CXX_SOURCES "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")
target_sources(${PROJECT_NAME} PRIVATE "${C_CXX_SOURCES}")
target_sources(${PROJECT_NAME} PRIVATE "${FLEX_Lexer_OUTPUTS}")
target_sources(${PROJECT_NAME} PRIVATE "${BISON_Parser_OUTPUT_SOURCE}")
# Link libraries.
if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE argparse)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
if(COMPILER_LINK_KOOPA)
    target_link_libraries(${PROJECT_NAME} PRIVATE koopa)
    target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILER_LINK_KOOPA)
endif()
