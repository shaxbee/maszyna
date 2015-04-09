cmake_minimum_required(VERSION 2.8.12)
project(maszyna)


find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

set(MASZYNA_SOURCES
    maszyna.cpp
)

add_executable(maszyna ${MASZYNA_SOURCES})
target_include_directories(maszyna PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(maszyna glfw glm glew)
target_compile_definitions(maszyna PRIVATE -DGLBINDING_STATIC)