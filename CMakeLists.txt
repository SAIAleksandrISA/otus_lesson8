cmake_minimum_required(VERSION 3.15)

project(bayan VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

find_package(Boost 1.70 REQUIRED COMPONENTS filesystem program_options)

add_executable(bayan 
    src/filescanner.cpp 
    src/utils.cpp
    src/hasher.cpp
    src/filecomparer.cpp
    src/main.cpp
)

target_include_directories(bayan PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}"
)

# Линковка
target_link_libraries(bayan PRIVATE 
    Threads::Threads
    Boost::filesystem
    Boost::program_options
)

# Опции компиляции
if (MSVC)
    target_compile_options(bayan PRIVATE /W4)
else ()
    target_compile_options(bayan PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

# Генерация версии
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" 
    "${CMAKE_BINARY_DIR}/version.h"          
)