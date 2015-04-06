cmake_minimum_required(VERSION 2.8.12)
include(ExternalProject)

set_property(DIRECTORY PROPERTY EP_BASE ${CMAKE_CURRENT_BINARY_DIR})

ExternalProject_Add(
    glfw
    URL https://github.com/shaxbee/glfw/archive/master.zip
    CMAKE_ARGS
        -DGLFW_BUILD_EXAMPLES=OFF
        -DGLFW_BUILD_TESTS=OFF
        -DGLFW_BUILD_DOCS=OFF
    INSTALL_COMMAND ""
)

ExternalProject_Add(
    maszyna
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    DEPENDS glfw
    INSTALL_COMMAND ""
)
