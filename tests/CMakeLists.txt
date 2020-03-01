cmake_minimum_required(VERSION 3.10)

# Enforce use of C++14
set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CMAKE_CXX_STANDARD 14)

set(TESTS_TARGET_NAME "${CMAKE_PROJECT_NAME}.tests")
set(CMAKE_BUILD_TYPE Debug)

# Check for extensions directory as possible location for libraries or other external code
if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/ext")
  set(USING_EXT_DIR TRUE)
  set(EXT_DIR "${CMAKE_SOURCE_DIR}/ext")
endif()

file(GLOB_RECURSE SOURCES "*.cpp" "*.cc" "*.c" "**/*.cpp" "**/*.cc" "**/*.c")
file(GLOB_RECURSE HEADERS "*.hpp" "*.h" "**/*.hpp" "**/*.h")

file(GLOB_RECURSE TESTED_SOURCES "${PROJECT_SOURCE_DIR}/src/**.cc")
list(REMOVE_ITEM TESTED_SOURCES "${PROJECT_SOURCE_DIR}/src/main.cc")

add_executable(${TESTS_TARGET_NAME} ${SOURCES} ${TESTED_SOURCES} ${HEADERS})

include_directories("${PROJECT_SOURCE_DIR}/src")

setBuildProperties(${TESTS_TARGET_NAME})

if(CMAKE_BUILD_TYPE MATCHES Release)
  set(TESTS_DIR "tests/")
else()
  set(TESTS_DIR "${CMAKE_SOURCE_DIR}/tests/")
endif()

add_definitions("-DTESTS_DIR=${TESTS_DIR}")
