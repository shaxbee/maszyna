#ifndef _commons_H_
#define _commons_H_

#include <windows.h>  // !!
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <fstream>  // std::ifstream
#include <sstream>
#include <assert.h>
#include <direct.h>
#include <wtypes.h>
#include <shellapi.h> // !!
#include <iomanip>
#include <direct.h>
#include <MATH.H>
#include <FLOAT.H>

#define GLFW_INCLUDE_GLU
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//#include "opengl\glew.h"
//#include "opengl\glext.h"
#include "opengl\gl.h"			// Header File For The OpenGL32 Library
#include "opengl\glu.h"			// Header File For The GLu32 Library
#include "opengl\glut.h"	
#include "opengl\glaux.h"		// Header File For The Glaux Library

//#include <glbinding/gl/gl33core.h>
//#include <glbinding/gl/gl.h>
//#include <glbinding/Binding.h>
#include "include\camerax.h"
#include "include\dumb3d.h"
//using namespace gl33core;
//using namespace glbinding;

#pragma comment(lib,"Version.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"dsound.lib")

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#endif 