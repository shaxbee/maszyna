cmake_minimum_required(VERSION 2.8.12)

project(MaSzynaSuperbuild)
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
    glew
    URL https://github.com/shaxbee/glew-cmake/archive/master.zip
    INSTALL_COMMAND ""
)

ExternalProject_Add(
    glm
    URL https://github.com/shaxbee/glm/archive/master.zip
    INSTALL_COMMAND ""
)

ExternalProject_Add(
    googlemock
    URL https://googlemock.googlecode.com/files/gmock-1.7.0.zip
    URL_HASH SHA1=f9d9dd882a25f4069ed9ee48e70aff1b53e3c5a5
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Add(
    maszyna
    DEPENDS glfw glew glm
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/maszyna
	CMAKE_ARGS
		-Dglfw3_DIR:STRING=${CMAKE_BINARY_DIR}/Build/glfw
		-Dglew_DIR:STRING=${CMAKE_BINARY_DIR}/Build/glew
		-Dglm_DIR:STRING=${CMAKE_BINARY_DIR}/Build/glm
    INSTALL_COMMAND ""
)

