cmake_minimum_required(VERSION 3.16)
project(nerd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/network/flow.cpp
    src/network/packet.cpp
    src/network/flow_manager.cpp
    src/editor/flow_editor.cpp
    src/core/flow_file.cpp
)

# Create executable
add_executable(nerd ${SOURCES})

# Link libraries
target_link_libraries(nerd 
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Compiler flags for network programming
target_compile_options(nerd PRIVATE
    -Wall
    -Wextra
    -O2
    -D_GNU_SOURCE
)

# Install target
install(TARGETS nerd DESTINATION bin)