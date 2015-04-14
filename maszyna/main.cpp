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
std::vector<std::string> skeybind;

TWorld World;
//static int mx = 0, my = 0;
static POINT xmouse;
int ckey, ccode, caction, cmod, nbFrames;

GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

//void mouse_callback(GLFWwindow* window, double xpos, double ypos);


// ***************************************************************************************************************************************
// Reading keyboard settings

void LOADKEYBINDINGS()
{
	
	int cl = 0;
	std::ifstream file("keyboard.txt");
	std::string str;
	Global::keybindsnum = 0;
	while (std::getline(file, str))
	{ 
		int ishash;
		ishash = str.find('#');
		
		if ((str.length() > 5) && (ishash <0))
		{// example line: 2 1 kQ "APPEXIT"
			MASZYNA_TRACE_WRITELINE(AppMain, Debug, "line %i: %s", cl, str.c_str());
			skeybind = split(str, ' ');
			Global::KBD[cl].keymod = skeybind[0];
			Global::KBD[cl].keyaction = skeybind[1];
			Global::KBD[cl].key = skeybind[2];
			Global::KBD[cl].keycommand = skeybind[3];
			Global::keybindsnum++;

			//WriteLogSS(Global::KBD[cl].keymod + ":" + Global::KBD[cl].keyaction + ":" + Global::KBD[cl].key + ":" + Global::KBD[cl].keycommand, "?");
			cl++;
		}
	}
}


// ***************************************************************************************************************************************
// This procedure compare 'key string' previously generated in keyboard callback witch items collected
// in keyboards binding table. If it hits in the same string, that happens download command string

void PROCESSKEYACTION(std::string str)
{
	std::string test, testcam, command;
	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "checking %s", str.c_str());
	for (int i = 0; i < Global::keybindsnum; i++)
	{
		test = Global::KBD[i].keymod + ":" + Global::KBD[i].keyaction + ":" + Global::KBD[i].key;
		testcam = "?:" + Global::KBD[i].keyaction + ":" + Global::KBD[i].key;
		MASZYNA_TRACE_WRITELINE(AppMain, Debug, "comparing %s", test.c_str());
		if (test == str)
		{
			Global::KEYCOMMAND = Global::KBD[i].keycommand;
			break;
		}
		if (test == str)
		{
			Global::KEYCOMMAND = Global::KBD[i].keycommand;
			break;
		}
	}
}

void PROCESSKEYACTIONCAM(std::string str)
{
	std::string test, testcam, command;
	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "checking %s", str.c_str());
	for (int i = 0; i < Global::keybindsnum; i++)
	{

		testcam = "?:" + Global::KBD[i].keyaction + ":" + Global::KBD[i].key;
		MASZYNA_TRACE_WRITELINE(AppMain, Debug, "comparing %s", test.c_str());

		if (testcam == str)
		{
			Global::KEYCOMMAND = Global::KBD[i].keycommand;
			break;
		}
	}
}

// ***************************************************************************************************************************************
// Error callback

void error_callback(int, const char* description)
{
    std::cerr << description << std::endl;
	Maszyna::Diagnostics::Trace::Fail("OpenGL error", description);
}

// ***************************************************************************************************************************************
// Drop callback

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	for (int i = 0; i < count; i++) std::cout << paths[i] << '\n';
}


// ***************************************************************************************************************************************
// Monitor callback

void monitor_callback(GLFWmonitor* monitor, int event)
{
	//int count;
	//GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
}


// ***************************************************************************************************************************************
// resize callback

void resize_callback(GLFWwindow* window, int w, int h) {
	if (h < 1) h = 1;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (float)w / (float)h, 0.1f, 1000.0f);
	gluLookAt(0.0f, 0.0f, 30, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	glMatrixMode(GL_MODELVIEW);
}


// ***************************************************************************************************************************************
// Keyboard callback

//void key_callbackq(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "Window: %p, Key: %d, Scancode: %03x, Action: %08x, Modifiers: %03x", window, key, scancode, action, mods);
//
//	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
//		glfwSetWindowShouldClose(window, true);
//}

// ***************************************************************************************************************************************
// Mouse scroll callback

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{

	if (yoffset ==  1) Global::FOV += 1.0; // g_Scale += g_MouseWheelScale;
	if (yoffset == -1) Global::FOV -= 1.0; // g_Scale -= g_MouseWheelScale  ;
	if (Global::FOV < 10) Global::FOV = 10;
	if (Global::FOV > 90) Global::FOV = 90;

	//MASZYNA_TRACE_WRITELINE(AppMain, Debug, "factor: %d.%d.%d", xoffset, yoffset, g_Scale);
}

// ***************************************************************************************************************************************
// key callback

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "Window: %p, Key: %d, Scancode: %08x, Action: %08x, Modifiers: %08x", window, key, scancode, action, mode);

	//GLfloat cameraSpeed = 5.0f * deltaTime;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);

	Global::KEYCOMMAND = "NONE";
	Global::keyid = key;
	Global::keyscancode = scancode;
	Global::keyaction = action;
	Global::keymods = mode;

	// BLOCK MAIN (60)
	if (key ==  96) Global::KEYCODE = "kTYLDE";
	if (key ==  49) Global::KEYCODE = "k1";
	if (key ==  50) Global::KEYCODE = "k2";
	if (key ==  51) Global::KEYCODE = "k3";
	if (key ==  52) Global::KEYCODE = "k4";
	if (key ==  53) Global::KEYCODE = "k5";
	if (key ==  54) Global::KEYCODE = "k6";
	if (key ==  55) Global::KEYCODE = "k7";
	if (key ==  56) Global::KEYCODE = "k8";
	if (key ==  57) Global::KEYCODE = "k9";
	if (key ==  48) Global::KEYCODE = "k0";
	if (key ==  45) Global::KEYCODE = "kMINUS";
	if (key ==  61) Global::KEYCODE = "kEQUAL";
	if (key ==  92) Global::KEYCODE = "kSLASH";
	if (key == 259) Global::KEYCODE = "kBACKSPACE";
	if (key == 258) Global::KEYCODE = "kTAB";
	if (key ==  81) Global::KEYCODE = "kQ";
	if (key ==  87) Global::KEYCODE = "kW";
	if (key ==  69) Global::KEYCODE = "kE";
	if (key ==  82) Global::KEYCODE = "kR";
	if (key ==  84) Global::KEYCODE = "kT";
	if (key ==  89) Global::KEYCODE = "kY";
	if (key ==  85) Global::KEYCODE = "kU";
	if (key ==  73) Global::KEYCODE = "kI";
	if (key ==  79) Global::KEYCODE = "kO";
	if (key ==  80) Global::KEYCODE = "kP";
	if (key ==  91) Global::KEYCODE = "kSQRLBRACKET";
	if (key ==  93) Global::KEYCODE = "kSQRRBRACKET";
	if (key == 280) Global::KEYCODE = "kCAPSLOOK";
	if (key ==  65) Global::KEYCODE = "kA";
	if (key ==  83) Global::KEYCODE = "kS";
	if (key ==  68) Global::KEYCODE = "kD";
	if (key ==  70) Global::KEYCODE = "kF";
	if (key ==  71) Global::KEYCODE = "kG";
	if (key ==  72) Global::KEYCODE = "kH";
	if (key ==  74) Global::KEYCODE = "kJ";
	if (key ==  75) Global::KEYCODE = "kK";
	if (key ==  76) Global::KEYCODE = "kL";
	if (key ==  59) Global::KEYCODE = "kSEMICOLON";
	if (key ==  39) Global::KEYCODE = "kSINGLEQUOTE";
	if (key == 340) Global::KEYCODE = "kLSHIFT";
	if (key ==  90) Global::KEYCODE = "kZ";
	if (key ==  88) Global::KEYCODE = "kX";
	if (key ==  67) Global::KEYCODE = "kC";
	if (key ==  86) Global::KEYCODE = "kV";
	if (key ==  66) Global::KEYCODE = "kB";
	if (key ==  78) Global::KEYCODE = "kN";
	if (key ==  77) Global::KEYCODE = "kM";
	if (key ==  44) Global::KEYCODE = "kCOMMA";
	if (key ==  46) Global::KEYCODE = "kPERIOD";
	if (key ==  47) Global::KEYCODE = "kFORWARDSLASH";
	if (key == 344) Global::KEYCODE = "kRSHIFT";
	if (key == 341) Global::KEYCODE = "kLCONTROL";
	if (key == 343) Global::KEYCODE = "kLWIN";
	if (key == 342) Global::KEYCODE = "kLALT";
	if (key == 346) Global::KEYCODE = "kRALT";
	if (key == 347) Global::KEYCODE = "kRWIN";
	if (key == 348) Global::KEYCODE = "kRMENU";
	if (key == 345) Global::KEYCODE = "kRCONTROL";
	if (key ==  32) Global::KEYCODE = "kSPACE";

	// BLOCK FUNCTION KEYS (12)
	if (key == 290) Global::KEYCODE = "kF1";
	if (key == 291) Global::KEYCODE = "kF2";
	if (key == 292) Global::KEYCODE = "kF3";
	if (key == 293) Global::KEYCODE = "kF4";
	if (key == 294) Global::KEYCODE = "kF5";
	if (key == 295) Global::KEYCODE = "kF6";
	if (key == 296) Global::KEYCODE = "kF7";
	if (key == 297) Global::KEYCODE = "kF8";
	if (key == 298) Global::KEYCODE = "kF9";
	if (key == 299) Global::KEYCODE = "kF10";
	if (key == 300) Global::KEYCODE = "kF11";
	if (key == 301) Global::KEYCODE = "kF12";

	// BLOCK MIDDLE (13)
	if (key == 283) Global::KEYCODE = "kPRINTSCREEN";
	if (key == 281) Global::KEYCODE = "kSCROLLLOCK";
	if (key == 284) Global::KEYCODE = "kPAUSEBREAK";
	if (key == 260) Global::KEYCODE = "kINSERT";
	if (key == 261) Global::KEYCODE = "kDELETE";
	if (key == 268) Global::KEYCODE = "kHOME";
	if (key == 269) Global::KEYCODE = "kEND";
	if (key == 266) Global::KEYCODE = "kPAGEUP";
	if (key == 267) Global::KEYCODE = "kPAGEDOWN";
	if (key == 265) Global::KEYCODE = "kUP";
	if (key == 264) Global::KEYCODE = "kDOWN";
	if (key == 263) Global::KEYCODE = "kLEFT";
	if (key == 264) Global::KEYCODE = "kRIGHT";

	// BLOCK NUMPAD (17)
	if (key == 282) Global::KEYCODE = "kNUMLOCK";
	if (key == 331) Global::KEYCODE = "kNUMDIVIDE";
	if (key == 332) Global::KEYCODE = "kNUMMULTIPLE";
	if (key == 333) Global::KEYCODE = "kNUMSUBTRACT"; 
	if (key == 334) Global::KEYCODE = "kNUMADD";
	if (key == 335) Global::KEYCODE = "kNUMENTER";
	if (key == 327) Global::KEYCODE = "kNUM7";
	if (key == 328) Global::KEYCODE = "kNUM8";
	if (key == 329) Global::KEYCODE = "kNUM9";
	if (key == 324) Global::KEYCODE = "kNUM4";
	if (key == 325) Global::KEYCODE = "kNUM5";
	if (key == 326) Global::KEYCODE = "kNUM6";
	if (key == 321) Global::KEYCODE = "kNUM1";
	if (key == 322) Global::KEYCODE = "kNUM2";
	if (key == 323) Global::KEYCODE = "kNUM3";
	if (key == 320) Global::KEYCODE = "kNUM0";
	if (key == 330) Global::KEYCODE = "kNUMDEL";
	// TOTAL 102 PHYSICAL KEYS

	std::string kmode = std::to_string(mode);
	std::string kaction = std::to_string(action);

	PROCESSKEYACTION(kmode + ":" + kaction + ":" + Global::KEYCODE);
	PROCESSKEYACTIONCAM(    "?:" + kaction + ":" + Global::KEYCODE);

}

//void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
//{
	//World.OnMouseMove(double(xpos )*0.00001, double(ypos)*0.0001);

	//POINT mouse;
	//GetCursorPos(&mouse);

	//World.LM_RetrieveObjectColor(Global::iMPX, Global::iMPY);

	//if (Global::bActive && ((mouse.x != mx) || (mouse.y != my)))
	//{
	//	World.OnMouseMove(double(mouse.x - mx)*0.005, double(mouse.y - my)*0.01);
	//	SetCursorPos(mx, my);      //
	//}

//}

// ***************************************************************************************************************************************
// mouse move callback

bool firstMouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	//MASZYNA_TRACE_WRITELINE(AppMain, Debug, "x: %f, y: %f", xpos, ypos);

	World.OnMouseMove(double(xpos)*0.0001, double(ypos)*0.001);
}


// ***************************************************************************************************************************************
// int main(int, char const* []) 

int main(int, char const* [])
{
	//bool exists;
	int ssx, ssy;
	int count,  argc;
	int major, minor, rev;
	WORD vmajor, vminor, vbuild, vrev;
	char appvers[100];

	Global::bWriteLogEnabled = true;
	// Initialize tracing with 'maszyna.log' file as a target.
	Maszyna::Diagnostics::Trace::Initialize("debuglog.log");

	Global::asCWD = GETCWD();

	argc = ParseCommandline();
	GetAppVersion(Global::argv[0], &vmajor, &vminor, &vbuild, &vrev);
	sprintf_s(appvers, "appfile: %s", Global::argv[0]);
	WriteLog(appvers);
	sprintf_s(appvers, "appvers: %i %i %i", vmajor, vminor, vbuild);
	WriteLog(appvers);
	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "Reading keyboard file...");
	LOADKEYBINDINGS();

	MASZYNA_TRACE_WRITELINE(AppMain, Debug, "Reading configuration file...");
	WriteLog("Reading configuration...");
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
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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


	
	if (fullscreenVal == 1) Global::window = glfwCreateWindow(ssx, ssy, "MaSzyna 2", monitors[0], nullptr);
	if (fullscreenVal == 0) Global::window = glfwCreateWindow(ssx, ssy, "MaSzyna 2", nullptr, nullptr);

	if (!Global::window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	SetProcessorAffinity();

	Global::iWindowWidth = ssx;
	Global::iWindowHeight = ssy;

	glfwSetWindowSize(Global::window, ssx, ssy); // Change window size sets in configuration
	glfwMakeContextCurrent(Global::window);
	glfwSwapInterval(1);
	//glewExperimental = GL_TRUE;
	//glewInit();


	glfwSetWindowSizeCallback(Global::window, resize_callback);
	glfwSetKeyCallback(Global::window, key_callback);
	glfwSetErrorCallback(error_callback);
	glfwSetMonitorCallback(monitor_callback);
	glfwSetScrollCallback(Global::window, scroll_callback);
	glfwSetDropCallback(Global::window, drop_callback);
	glfwSetCursorPosCallback(Global::window, mouse_callback);

	glfwSetInputMode(Global::window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwSetCursorPos(Global::window, ssx / 2, ssy / 2);

	glViewport(0, 0, ssx, ssy);

	World.Init();

	glfwSetTime(0);

	Global::previousFrameTime = float(glfwGetTime()); // Holds the amount of milliseconds since the last frame
	Global::timeAccumulator = 0; // Holds a sum of the time from all passed frame times
	Global::fpsMeasureInterval = 1.0f; // The interval where we would like to take an FPS sample. Currently simply each second.
	Global::frameCount = 0; // The current amount of frames which have passed

	while (!glfwWindowShouldClose(Global::window))
    {
		CALCULATEFPS();

		glfwGetFramebufferSize(Global::window, &ssx, &ssy);

		glfwPollEvents();

		World.OnKeyDown(ckey, ccode, caction, cmod, Global::KEYCOMMAND);

		World.Update(Global::frameTime);

		glFlush();

		glfwSwapBuffers(Global::window);
    }

	std::ofstream fout("info.txt");
	fout << "You can help in the formation of the simulator sending us information about the detected \n"
		    "errors and incorrect the behavior of the graphics and the program itself.\n\n" 
	   	    "http://forum.eu07.es/"  "\n" 
		    "http://forum.eu07.es/index.php?board=24.0" "\n" 
			"http://forum.eu07.es/index.php/topic,27.new.html#new" "\n" << std::endl;

	fout.close();
	
	ShellExecute(0, "open", "notepad.exe", "info.txt", NULL, SW_SHOW);
	ShellExecute(0, "open", "notepad.exe", "log.txt", NULL, SW_SHOW);
	Sleep(140);
	DeleteFile("info.txt");

	glfwDestroyWindow(Global::window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
