//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others
*/

#pragma hdrstop

#include "commons.h"
#include "commons_usr.h"

TGround *Global::pGround = NULL;
char **Global::argv = NULL;
HWND Global::hWnd = NULL; // 
GLFWwindow* Global::window;
int Global::Keys[MaxKeys];
vector3 Global::pCameraPosition;
double Global::pCameraRotation;
double Global::pCameraRotationDeg;
vector3 Global::pFreeCameraInit[10];
vector3 Global::pFreeCameraInitAngle[10];
int Global::iWindowWidth = 800;
int Global::iWindowHeight = 600;

GLuint Global::fonttexturex;
GLfloat  Global::AtmoColor[] = { 0.6f, 0.7f, 0.8f };
GLfloat  Global::FogColor[] = { 0.6f, 0.7f, 0.8f };
GLfloat  Global::ambientDayLight[] = { 0.40f, 0.40f, 0.45f, 1.0f };
GLfloat  Global::diffuseDayLight[] = { 0.55f, 0.54f, 0.50f, 1.0f };
GLfloat  Global::specularDayLight[] = { 0.95f, 0.94f, 0.90f, 1.0f };
GLfloat  Global::whiteLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat  Global::noLight[] = { 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat  Global::lightPos[4];
GLfloat Global::darkLight[] = { 0.03f, 0.03f, 0.03f, 1.0f }; //œladowe
float Global::fMouseXScale = 3.5f;
float Global::fMouseYScale = 2.4f;
float Global::fTimeSpeed = 1.0f;
double Global::FPS = 0.0f;
float Global::fdt = 0.0f;
float Global::fms = 0.0f;
float Global::FOV = 45.0f;

bool Global::detonatoryOK = false;
bool Global::bFreeFly = false;
bool Global::bFreeFlyModeFlag = false;
bool Global::bWriteLogEnabled = false;
bool Global::bActive = false;
bool Global::bInactivePause = false;
bool Global::bGlutFont = false;
bool Global::bSCNLOADED = false;
bool Global::bfirstloadingscn = true;
bool Global::bSoundEnabled = false;
bool Global::bManageNodes = false;
bool Global::bUseVBO = false;
bool Global::bWireFrame = false;
bool Global::bOpenGL_1_5 = false;
bool Global::bRenderAlpha = true;
int Global::iReCompile = 0;
int Global::iPARSERBYTESPASSED = 0;
int Global::iNODES = 0;
int Global::postep = 0;
int Global::iPause = 0;
int Global::iTextMode = 0;
int Global::aspectratio = 43;
int Global::keyid = 0;
int Global::keyaction = 0;
int Global::keyscancode = 0;
int Global::keymods = 0;
int Global::keybindsnum = 0;
int Global::iRailProFiltering = 0;
int Global::iBallastFiltering = 0;
int Global::iWriteLogEnabled = 1;
int Global::iMultiplayer = 0;
float Global::fOpenGL = 0.0;
std::string Global::KEYCODE = "0";
std::string Global::KEYCOMMAND = "NONE";
std::string Global::asCWD = "";
std::string Global::logfilenm1 = "log.txt";
std::string Global::szDefaultExt = ".bmp";
std::string Global::asCurrentDynamicPath = "dynamic\\eu07\\";
std::string Global::asCurrentSceneryPath = "scenery\\";
std::string Global::asLang = "pl";

GLuint Global::boxtex = NULL;
GLuint Global::balltex = NULL;
GLuint Global::dirttex = NULL;
GLuint Global::logotex = NULL;
GLuint Global::bfonttex = NULL;
GLuint Global::loaderbackg = NULL;
GLuint Global::fbase;
TColor Global::color;
keyboardsets Global::KBD[KEYBINDINGS];

vector3 Global::Camerapos;
// Camera
glm::vec3 Global::cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 Global::cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 Global::cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
GLfloat Global::yaw = -90.0f;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
GLfloat Global::pitch = 0.0f;
GLfloat Global::lastX = 1280 / 2.0;
GLfloat Global::lastY = 1024 / 2.0;
bool Global::keys[1024];
CCamera Global::CAMERA;

float Global::tracksegmentlen = 2.0;
float Global::frameTime = 0;
float Global::previousFrameTime = 0; // Holds the amount of milliseconds since the last frame
float Global::timeAccumulator = 0; // Holds a sum of the time from all passed frame times
float Global::fpsMeasureInterval = 1.0f; // The interval where we would like to take an FPS sample. Currently simply each second.
int Global::frameCount = 0; // The current amount of frames which have passed


void Global::SetCameraPosition(vector3 pNewCameraPosition)
{
	pCameraPosition = pNewCameraPosition;
}

void Global::SetCameraRotation(double Yaw)
{//ustawienie bezwzglêdnego kierunku kamery z korekcj¹ do przedzia³u <-M_PI,M_PI>
	pCameraRotation = Yaw;
	while (pCameraRotation<-M_PI) pCameraRotation += 2 * M_PI;
	while (pCameraRotation> M_PI) pCameraRotation -= 2 * M_PI;
	pCameraRotationDeg = pCameraRotation*180.0 / M_PI;
}

TColor Global::SetColor(float r, float g, float b, float a)
{
	Global::color.r = r;
	Global::color.g = g;
	Global::color.b = b;
	Global::color.a = a;

	return Global::color;
}

GLvoid Global::glPrintxy(GLint x, GLint y, char *string, int set)	// Where The Printing Happens
{
	if (set>1)
	{
		set = 1;
	}
	//glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);			// Select Our Font Texture
	//glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
	//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	//glPushMatrix();										// Store The Projection Matrix
	//glLoadIdentity();									// Reset The Projection Matrix
	//glOrtho(0, 1280, 0, 1024, -1, 1);							// Set Up An Ortho Screen
	//glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPushMatrix();										// Store The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslated(x, y, 0);								// Position The Text (0,0 - Bottom Left)
	glListBase(Global::fbase - 32 + (128 * set));						// Choose The Font Set (0 or 1)
	glCallLists((int)strlen(string), GL_UNSIGNED_BYTE, string);// Write The Text To The Screen
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	//glPopMatrix();										// Restore The Old Projection Matrix
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
}



GLvoid Global::BuildFont(GLvoid)								// Build Our Font Display List
{
	float	cx;											// Holds Our X Character Coord
	float	cy;											// Holds Our Y Character Coord

	fbase = glGenLists(256);								// Creating 256 Display Lists
	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);			// Select Our Font Texture
	glBindTexture(GL_TEXTURE_2D, Global::loaderbackg);
	for (int loop = 0; loop<256; loop++)						// Loop Through All 256 Lists
	{
		cx = float(loop % 16) / 16.0f;						// X Position Of Current Character
		cy = float(loop / 16) / 16.0f;						// Y Position Of Current Character

		glNewList(fbase + loop, GL_COMPILE);				// Start Building A List
		
		glBegin(GL_QUADS);							// Use A Quad For Each Character
		glTexCoord2f(cx, 1 - cy - 0.0625f);			// Texture Coord (Bottom Left)
		glVertex2i(0, 0);						// Vertex Coord (Bottom Left)
		glTexCoord2f(cx + 0.0625f, 1 - cy - 0.0625f);	// Texture Coord (Bottom Right)
		glVertex2i(16, 0);						// Vertex Coord (Bottom Right)
		glTexCoord2f(cx + 0.0625f, 1 - cy);			// Texture Coord (Top Right)
		glVertex2i(16, 16);						// Vertex Coord (Top Right)
		glTexCoord2f(cx, 1 - cy);					// Texture Coord (Top Left)
		glVertex2i(0, 16);						// Vertex Coord (Top Left)
		glEnd();									// Done Building Our Quad (Character)
		glTranslated(10, 0, 0);						// Move To The Right Of The Character
		glEndList();									// Done Building The Display List
	}													// Loop Until All 256 Are Built
}

GLvoid Global::KillFont(GLvoid)									// Delete The Font From Memory
{
	glDeleteLists(fbase, 256);							// Delete All 256 Display Lists
}


double Global::CALCULATEFPS()
{
	float newTime = float(glfwGetTime());

	Global::frameTime = newTime - Global::previousFrameTime;

	Global::fdt = Global::frameTime;

	Global::previousFrameTime = newTime;

	Global::timeAccumulator += Global::frameTime;

	Global::frameCount++;

	// If our timeAccumulator passes 1 second, it means we should take a sample of our FPS
	if (Global::timeAccumulator > Global::fpsMeasureInterval)
	{
		Global::FPS = (double)Global::frameCount;
		Global::frameCount = 0;
		Global::timeAccumulator = 0;
	}
	return Global::FPS;
}


// **************************************************************************************
// GETCWD()
// **************************************************************************************

std::string Global::GETCWD()
{
	char* cwdbuffer;
	char szBuffer[200];

	// Get the current working directory:
	if ((cwdbuffer = _getcwd(NULL, 0)) == NULL)  perror("_getcwd error");
	else
	{
		// stdstrtochar
		sprintf_s(szBuffer, "workdir: [%s]", cwdbuffer);
		WriteLog(szBuffer);

		Global::asCWD = chartostdstr(szBuffer);
		return cwdbuffer;
		//free(cwdbuffer);
	}
	
}


// ********************************************************************************************************************************
// MIN0R
// ********************************************************************************************************************************

double __fastcall Min0R(double x1, double x2)
{
	double Min0R;

	if (x1 < x2)  Min0R = x1; else Min0R = x2;

	return Min0R;
}


// ********************************************************************************************************************************
// MAX0R
// ********************************************************************************************************************************

double __fastcall Max0R(double x1, double x2)
{
	double Max0R;

	if (x1 > x2)  Max0R = x1; else Max0R = x2;

	return Max0R;
}


// ********************************************************************************************************************************
// TestFlag()
// ********************************************************************************************************************************

bool __fastcall TestFlag(int Flag, int Value)
{
	if ((Flag && Value) == Value)
		return true;
	else return false;
}

bool __fastcall SetFlag(int Flag, int Value)
{
	bool SF = false;
	if (Value > 0)
		if ((Flag && Value) == 0)
		{
			Flag = Flag + Value;
			SF = true;
		}
	if (Value < 0)
		if ((Flag && abs(Value)) == abs(Value))
		{
			Flag = Flag + Value;
			SF = true;
		}
	return SF;
}

int Sign(int v)
{
	return v > 0 ? 1 : (v < 0 ? -1 : 0);
}
