cmake_minimum_required(VERSION 3.10)

project(newtest C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -z now -pie -O2 -march=native -Wall -Wextra -Wshadow -Wconversion -fanalyzer")
file(GLOB_RECURSE PROJECT_HEADERS "${CMAKE_SOURCE_DIR}/include/**/*.h")
file(GLOB_RECURSE PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/src/**/*.c")

message(STATUS "Loaded sources: ${PROJECT_SOURCES}")
message(STATUS "Loaded headers: ${PROJECT_HEADERS}")

add_executable(main ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
