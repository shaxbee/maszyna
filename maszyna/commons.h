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
#include <tchar.h>
#include <strsafe.h>
#include <shellapi.h> // !!
#include <iomanip>
#include <direct.h>
#include <MATH.H>
#include <FLOAT.H>
#include <sys/types.h>
#include <sys/stat.h>
#include <urlmon.h> //"urlmon.h: No such file or directory found"
#include <winhttp.h>

#define GLEW_STATIC // <- NO ZESZ KURWA JA PIERDOLE ;)
#define GLFW_INCLUDE_GLU
#include <GL/glew.h> // include GLEW and new version of GL on Windows  http://glew.sourceforge.net/
#include <GLFW/glfw3.h>
//#include <GL/wglew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "il/il.h"
#include "il/ilu.h"
#include "il/ilut.h"
#include "freeimage.h"


//#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "WinInet.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dsound.lib")

//#pragma comment(lib, "opengl32.lib")
//#pragma comment(lib, "glu32.lib")
//#pragma comment(lib, "opengl32.lib")
//#pragma comment(lib, "c:/glew/glut32.lib")
//#pragma comment(lib, "glew32.lib")
//#pragma comment(lib, "glew32s.lib")
//#pragma comment(lib, "glew32mx.lib")
//#pragma comment(lib, "glew32mxs.lib")
//#pragma comment(lib, "devil.lib")


#endif 