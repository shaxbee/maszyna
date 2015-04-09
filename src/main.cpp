/*
MaSzyna Train Simulator
Copyright (C) 2014 MaSzyna Team

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "commons.h"
#include "commons_usr.h"


MASZYNA_DECLARE_TRACE_SWITCH(AppMain, Debug, Debug);
MASZYNA_DEFINE_TRACE_SWITCH(AppMain);

const int GLFW_TRUE = static_cast<int>(GL_TRUE);
const int GLFW_FALSE = static_cast<int>(GL_FALSE);

std::vector<std::string> ssize;

float g_Scale = 1.0f;
float g_MouseWheelScale = 0.10f;
TWorld World;

// ***************************************************************************************************************************************
// Error callback

void error_callback(int, const char* description)
{
    std::cerr << description << std::endl;
	Maszyna::Diagnostics::Trace::Fail("OpenGL error", description);
}

// ***************************************************************************************************************************************
// Keyboard callback

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "Window: %p, Key: %d, Scancode: %08x, Action: %08x, Modifiers: %08x",window, key, scancode, action, mods);

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// ***************************************************************************************************************************************
// Mouse scroll callback

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset ==  1) g_Scale += g_MouseWheelScale  ;
	if (yoffset == -1) g_Scale -= g_MouseWheelScale  ;
	//std::cout << yoffset << ",  " << g_Scale <<'\n';
	//MASZYNA_TRACE_WRITELINE(AppMain, Debug, "factor: %d.%d.%d", xoffset, yoffset, g_Scale);
}

// ***************************************************************************************************************************************
// Drop callback

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	int i;
	for (i = 0; i < count; i++) std::cout << paths[i] << '\n';
}


// ***************************************************************************************************************************************
// Monitor callback

void monitor_callback(GLFWmonitor* monitor, int event)
{
	//int count;
	//GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
}

void Initialize(void) {
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0.4, 0.3, 0.1, 1.0);
}

void resize_callback(GLFWwindow* window, int w, int h) {
	if (h < 1)
		h = 1;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (float)w / (float)h, 0.1f, 1000.0f);
	gluLookAt(0.0f, 0.0f, 30, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	glMatrixMode(GL_MODELVIEW);
}


// ***************************************************************************************************************************************
// int main(int, char const* []) 

int main(int, char const* [])
{
	bool exists;
	int ssx, ssy;
	int count;
	int major, minor, rev;
	GLFWwindow* window;
	// Initialize tracing with 'maszyna.log' file as a target.
	Maszyna::Diagnostics::Trace::Initialize("debuglog.log");
	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "Reading configuration...");

	std::cerr << "Reading configuration..." << std::endl;
	ConfigFile cfg("config.txt");

	//exists = cfg.keyExists("testbool");
	//std::cerr << "testbool key: " << std::boolalpha << exists << "\n";
	//exists = cfg.keyExists("scenery");
	//std::cerr << "scenery: " << exists << "\n";
	//exists = cfg.keyExists("vechicle");
	//std::cerr << "vechicle: " << exists << "\n";

	std::string sceneryValue = cfg.getValueOfKey<std::string>("scenery", "default.scn");
	std::cerr << "value of key scenery: " << sceneryValue << "\n";
	std::string vechicleValue = cfg.getValueOfKey<std::string>("vechicle", "EU07-424");
	std::cerr << "value of key vechicle: " << vechicleValue << "\n";
	std::string screensizeValue = cfg.getValueOfKey<std::string>("screensize", "1024x768");
	std::cerr << "value of key screensize: " << screensizeValue << "\n";

	std::string testboolValue = cfg.getValueOfKey<std::string>("testbool");
	std::cerr << "value of key testbool: " << testboolValue << "\n";
	std::string desktopresValue = cfg.getValueOfKey<std::string>("desktopres");
	std::cerr << "value of key desktopres: " << desktopresValue << "\n";
	bool fullscreenVal = cfg.getValueOfKey<bool>("fullscreen");
	std::cerr << "value of key fullscreen: " << fullscreenVal << "\n";
	double doubleVal = cfg.getValueOfKey<double>("testdouble");
	std::cerr << "value of key double: " << doubleVal << "\n";


    ssize = split(screensizeValue, ':');                                   // Splits screensize key value into two params
	std::cerr << "screensize: " << ssize[0] << "x" << ssize[1] << "\n";


	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "Initializing OpenGL...");
	std::cerr << "Initializing OpenGL..." << std::endl;

    glfwInit();

	glfwGetVersion(&major, &minor, &rev);

	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "OpenGL ver: %d.%d.%d", major, minor, rev);

	std::cerr << "OpenGL version: " << major << " " << minor << " " << rev << std::endl;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	std::cerr << "monitors count " << count << std::endl;
	const GLFWvidmode* mode = glfwGetVideoMode(monitors[0]);
	std::cerr << "video mode: " << mode->width << "x"  << mode->height << std::endl; // current desktop resplution
	std::cerr << "video refr: " << mode->refreshRate << std::endl;
	const char* monitorname = glfwGetMonitorName(monitors[0]);
	std::cerr << "Monitor name: " << monitorname << std::endl;

    //glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    ssx = std::stoi(ssize[0]);
	ssy = std::stoi(ssize[1]);

	if (desktopresValue == "1") // dimensions of the canvas as desktop resolution
	{
		MASZYNA_TRACE_WRITELINE(AppMain, Debug, "Viewport size as desktop resolution...");
		ssx = mode->width;
		ssy = mode->height;
	}


	
	if (fullscreenVal == 1) window = glfwCreateWindow(ssx, ssy, "MaSzyna 2", monitors[0], nullptr);
	if (fullscreenVal == 0) window = glfwCreateWindow(ssx, ssy, "MaSzyna 2", nullptr, nullptr);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}



	

	
	glfwSetWindowSize(window, ssx, ssy); // Change window size sets in configuration
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	//glewExperimental = GL_TRUE;
	//glewInit();
	Initialize();

	glfwSetWindowSizeCallback(window, resize_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetErrorCallback(error_callback);
	glfwSetMonitorCallback(monitor_callback);
	glfwSetScrollCallback(window, scroll_callback);
	//glfwSetDropCallback(window, drop_callback);

	glfwSetCursorPos(window, ssx / 2, ssy / 2);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	World.Init();

	glfwSetTime(0);

    while(!glfwWindowShouldClose(window))
    {
		float deltat = glfwGetTime();

		float ratio;
		int width = 1280;
		int height = 1024;;

		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;
		glViewport(0, 0, width, height);

		World.Update(deltat);
		
		glfwSetTime(0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
