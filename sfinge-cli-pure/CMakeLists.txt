cmake_minimum_required(VERSION 3.16)
project(sfinge-cli-pure VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags (use -march=x86-64 for compatibility)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=x86-64 -DNDEBUG")

# Static linking for portability
# Use: CC=musl-gcc CXX=musl-g++ cmake .. para compilação totalmente estática
if(STATIC_BUILD)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
else()
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif()

# Find threads
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/core/image.cpp
    src/core/orientation_generator.cpp
    src/core/gabor_filter.cpp
    src/core/ridge_generator.cpp
    src/core/minutiae_generator.cpp
    src/core/fingerprint_generator.cpp
    src/core/batch_generator.cpp
    src/models/singular_points.cpp
    src/models/fingerprint_parameters.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Executable
add_executable(sfinge-cli ${SOURCES})

# Link libraries
target_link_libraries(sfinge-cli
    Threads::Threads
)

# Install
install(TARGETS sfinge-cli RUNTIME DESTINATION bin)
