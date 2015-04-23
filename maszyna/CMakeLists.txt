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
	download.cpp
	pdownloader.cpp
	float3d.cpp
	animmodel.cpp
	model3d.cpp
	mdlmngr.cpp
	ground.cpp
        download.cpp
	vbo.cpp
)

set(MASZYNA_HEADERS
	include/usefull.h/
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
	include/pdownloader.h
        include/track.h
	include/classes.h
	include/flags.h
	include/float3d.h
	include/animmodel.h
	include/model3d.h
	include/mdlmngr.h
)

set(Q_LIB_DIRS C:\\DEPENDIENCES\\devil)
message("${Q_LIB_DIRS}")
LINK_DIRECTORIES(${Q_LIB_DIRS}/)



add_executable(maszyna ${MASZYNA_SOURCES} ${MASZYNA_HEADERS})
target_include_directories(maszyna PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(maszyna glfw glm glew)
target_compile_definitions(maszyna PRIVATE -DGLBINDING_STATIC)

ADD_DEPENDENCIES(maszyna copy_bar.ini)
FIND_LIBRARY( WIN32_COMCTRL devil.lib 
		PATHS 
			"C:/DEPENDIENCES/devil" 	
		DOC "Path to psdk"
	) 