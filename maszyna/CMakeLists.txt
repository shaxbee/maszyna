cmake_minimum_required(VERSION 2.8.12)
project(maszyna)

option(GTEST_ROOT:STRING "Location of GTest source package")

find_package(glew 1.12.0 REQUIRED CONFIG)
find_package(glfw3 REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

set(MASZYNA_SOURCES
	src/main.cpp
	src/globals.cpp
	src/logs.cpp
	src/console.cpp
	src/camera.cpp
	src/camerax.cpp
	src/dumb3d.cpp
	src/world.cpp
	src/trace.cpp
	src/texture.cpp
	src/texturedds.cpp
	src/xbitmap_font.cpp
	src/sound.cpp
	src/wavread.cpp
	src/parser.cpp
	src/timer.cpp
	src/realsound.cpp
	src/fadesound.cpp
	src/advsound.cpp
	src/cstring.cpp
	src/gui.cpp
	src/classes.cpp
	src/ResourceManager.cpp
	src/names.cpp
	src/segment.cpp
	src/trackfollower.cpp
	src/track.cpp
    src/model3d.cpp
    src/mdlmngr.cpp
    src/ground.cpp
    src/vbo.cpp
)

set(MASZYNA_HEADERS
	include/usefull.h
	include/trace.h
	include/dumb3d.h
	include/sysinfo.h
	include/Globals.h
	include/qglutils.h
	include/qstrutils.h
	include/Logs.h
	include/world.h
	include/Console.h
	include/Camera.h
	include/Camerax.h
	include/Sound.h
	include/parser.h
	include/wavread.h
	include/realsound.h
	include/advsound.h
	include/realsound.h
	include/timer.h
	include/advsound.h
	include/fadesound.h
	include/cstring.h
	include/ground.h
	include/ResourceManager.h
	include/data.h
	include/names.h
	include/texture.h
	include/segment.h
	include/trackfollower.h
    include/track.h
	include/classes.h
	include/flags.h
	include/float3d.h
	include/animmodel.h
	include/model3d.h
	include/mdlmngr.h
)

add_executable(maszyna ${MASZYNA_SOURCES} ${MASZYNA_HEADERS})
target_include_directories(maszyna PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(maszyna glfw glm glew)
target_compile_definitions(maszyna PRIVATE -DGLEW_STATIC)

enable_testing()

set(MASZYNA_TEST_SOURCES
    test/format_test.cpp
    test/parser_test.cpp
)

set(MASZYNA_TEST_SUBJECTS
    src/parser.cpp
)

add_executable(maszyna_test 
    ${MASZYNA_TEST_SOURCES}
    ${MASZYNA_TEST_SUBJECTS}
    "${GMOCK_ROOT}/fused-src/gmock-gtest-all.cc"
    "${GMOCK_ROOT}/fused-src/gmock_main.cc"
)
target_include_directories(maszyna_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${GMOCK_ROOT}/gtest/include"
    "${GMOCK_ROOT}/include"
)
set_target_properties(maszyna_test PROPERTIES 
    CXX_STANDARD 11 
    CXX_STANDARD_REQUIRED ON)

add_test(NAME maszyna_test COMMAND maszyna_test)
