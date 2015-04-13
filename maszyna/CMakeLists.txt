cmake_minimum_required(VERSION 2.8.12)
project(maszyna)

find_package(glew 1.12.0 REQUIRED CONFIG)
find_package(glfw3 REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

set(MASZYNA_SOURCES
    main.cpp
	globals.cpp
	logs.cpp
	console.cpp
	camera.cpp
	camerax.cpp
	dumb3d.cpp
	world.cpp
	trace.cpp
	texture.cpp
	texturedds.cpp
	xbitmap_font.cpp
	sound.cpp
	wavread.cpp
)

set(MASZYNA_HEADERS
	include/Globals.h
	include/Logs.h
	include/Console.h
	include/Camera.h
	include/Sound.h)

add_executable(maszyna ${MASZYNA_SOURCES} ${MASZYNA_HEADERS})
target_include_directories(maszyna PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(maszyna glfw glm glew)
target_compile_definitions(maszyna PRIVATE -DGLBINDING_STATIC)
