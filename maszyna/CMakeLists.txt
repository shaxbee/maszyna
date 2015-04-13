cmake_minimum_required(VERSION 2.8.12)
project(maszyna)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

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

add_executable(maszyna ${MASZYNA_SOURCES})
target_include_directories(maszyna PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(maszyna glfw glm)
target_compile_definitions(maszyna PRIVATE -DGLBINDING_STATIC)