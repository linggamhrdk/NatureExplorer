cmake_minimum_required(VERSION 3.20)
project(Aroxorn VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags for GCC/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -ffast-math -march=native -Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/O2 /fp:fast /arch:AVX2 /W4)
endif()

# Find dependencies
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Aroxorn Core Library
add_library(AroxornCore
    src/Instance.cpp
    src/Device.cpp
    src/Swapchain.cpp
    src/CommandBuffer.cpp
    src/Pipeline.cpp
    src/AccelerationStructure.cpp
    src/RayTracingPipeline.cpp
)

target_include_directories(AroxornCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(AroxornCore PUBLIC Vulkan::Vulkan glfw)

# Aroxorn Example (Triangle)
add_executable(AroxornExample tests/TriangleExample.cpp)
target_link_libraries(AroxornExample PRIVATE AroxornCore)

# Shaders
# A custom command to compile GLSL shaders to SPIR-V would go here
