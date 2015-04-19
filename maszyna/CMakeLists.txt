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
	parser.cpp
	timer.cpp
	realsound.cpp
	fadesound.cpp
	advsound.cpp
	cstring.cpp
	gui.cpp
	classes.cpp
	ResourceManager.cpp
	names.cpp
	segment.cpp
	trackfollower.cpp
	track.cpp
	pdownloader.cpp
)

set(MASZYNA_HEADERS
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
	include/segment.h
	include/trackfollower.h
        include/track.h
)

add_executable(maszyna ${MASZYNA_SOURCES} ${MASZYNA_HEADERS})
target_include_directories(maszyna PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(maszyna glfw glm glew)
target_compile_definitions(maszyna PRIVATE -DGLBINDING_STATIC)
