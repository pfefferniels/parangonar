cmake_minimum_required(VERSION 3.14)
project(parangonar_cpp VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compile options for optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Default to Release build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp/include)

# Find dependencies
find_package(Eigen3 3.3 QUIET)

# If Eigen is not found, provide header-only fallback message
if(NOT Eigen3_FOUND)
    message(STATUS "Eigen3 not found, using internal linear algebra implementations")
endif()

# Add library
add_library(parangonar_cpp
    cpp/src/note.cpp
    cpp/src/dtw.cpp
    cpp/src/matchers.cpp
    cpp/src/preprocessors.cpp
)

# Add WASM bindings library for Emscripten builds
if(EMSCRIPTEN)
    add_executable(parangonar_wasm
        cpp/src/note.cpp
        cpp/src/dtw.cpp
        cpp/src/matchers.cpp
        cpp/src/preprocessors.cpp
        cpp/src/wasm_bindings.cpp
    )
    
    target_include_directories(parangonar_wasm PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
    )
    
    if(Eigen3_FOUND)
        target_link_libraries(parangonar_wasm Eigen3::Eigen)
        target_compile_definitions(parangonar_wasm PUBLIC USE_EIGEN)
    endif()
endif()

# Add include directories to library
target_include_directories(parangonar_cpp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
)

# Link Eigen if found
if(Eigen3_FOUND)
    target_link_libraries(parangonar_cpp Eigen3::Eigen)
    target_compile_definitions(parangonar_cpp PUBLIC USE_EIGEN)
endif()

# Add executable for testing
add_executable(test_parangonar_cpp
    cpp/tests/test_matchers.cpp
)

target_link_libraries(test_parangonar_cpp parangonar_cpp)

# Add WASM API test
add_executable(test_wasm_api
    cpp/tests/test_wasm_api.cpp
)

target_link_libraries(test_wasm_api parangonar_cpp)

# Enable testing
enable_testing()
add_test(NAME parangonar_tests COMMAND test_parangonar_cpp)
add_test(NAME wasm_api_tests COMMAND test_wasm_api)

# Emscripten configuration
if(EMSCRIPTEN)
    # Base Emscripten flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 --bind")
    
    # Set link flags for the WASM library
    set_target_properties(parangonar_wasm PROPERTIES 
        LINK_FLAGS "-s WASM=1 --bind -s MODULARIZE=1 -s EXPORT_NAME='ParangonarModule' -s ENVIRONMENT=web,node -s ALLOW_MEMORY_GROWTH=1"
    )
    
    # Set output name
    set_target_properties(parangonar_wasm PROPERTIES OUTPUT_NAME "parangonar")
    
    # Keep the test executable simple
    set_target_properties(test_parangonar_cpp PROPERTIES LINK_FLAGS "-s WASM=1")
endif()