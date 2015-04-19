//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others
*/
// 2015.04.13 has included other soundsystem files
// 2015.04.13 starts changing char pointer type into CString (more flexible)
// 2015.04.14 removed many compiler warnings
// 2015.04.16 unsuccessful attempt to connect ground.cpp module
// 2015.04.17 changing directories for logs and configuration files
// 2015.04.17 creating a system for check and download updates
// 2015.04.18 solved GLEW functions problem
// 2015.04.18 unsuccessful attempt to add a sky with dynamic lighting and color change based on shaders
// 2015.04.18 second attempt to insert a module ground.cpp

// Additional includes:
// C:\DEPENDIENCES\devil\include
// C:\DEPENDIENCES\freeimage
// Additional linker input:
/*
C:\GLEW\glew.lib
C:\GLEW\glew32.lib
C:\GLEW\glew32s.lib
C:\GLEW\glew32mx.lib
C:\GLEW\glew32mxs.lib
C:\DEPENDIENCES\devil\devil.lib
C:\DEPENDIENCES\devil\ilu.lib
C:\DEPENDIENCES\devil\ilut.lib
*/

#include "commons.h"
#include "commons_usr.h"

//#include "shaders.h"
//#include "glmath.h"
#pragma hdrstop


GLuint	texture;			// Storage For Our Font Texture
GLYPHMETRICSFLOAT gmf[256];	// Storage For Information About Our Outline Font Characters
GLuint baseF3D;
HDC hDC;
TCamera Camera;
vector3 campos;
GLuint logo;
char fps[60];
char str[60];
std::vector<std::string> glexts;
CString ModuleDirectory, ErrorLog;




// *****************************************************************************************
// BuildFont3D() - DO NAPISOW 3D ***********************************************************
// *****************************************************************************************

GLvoid BuildFont3D(HDC hDC)								// Build Our Bitmap Font
{
	HFONT	font;										// Windows Font ID

	baseF3D = glGenLists(256);								// Storage For 256 Characters

	font = CreateFont(-12,							// Height Of Font
		0,								// Width Of Font
		0,								// Angle Of Escapement
		0,								// Orientation Angle
		FW_NORMAL,						// Font Weight
		FALSE,							// Italic
		FALSE,							// Underline
		FALSE,							// Strikeout
		ANSI_CHARSET,					// Character Set Identifier
		OUT_TT_PRECIS,					// Output Precision
		CLIP_DEFAULT_PRECIS,			// Clipping Precision
		ANTIALIASED_QUALITY,			// Output Quality
		FF_DONTCARE | DEFAULT_PITCH,		// Family And Pitch
		"Arial");				// Font Name

	SelectObject(hDC, font);							// Selects The Font We Created

	wglUseFontOutlines(hDC,							// Select The Current DC
		0,								// Starting Character
		255,							// Number Of Display Lists To Build
		baseF3D,							// Starting Display Lists
		0.0f,							// Deviation From The True Outlines
		0.02f,							// Font Thickness In The Z Direction
		WGL_FONT_POLYGONS,				// Use Polygons, Not Lines
		gmf);							// Address Of Buffer To Recieve Data
}

int width = 0;
int height = 0;
short BitsPerPixel = 0;
std::vector<unsigned char> Pixels;


GLvoid TWorld::glPrint( CString fmt)					// Custom GL "Print" Routine
{

	if (fmt == NULL)									// If There's No Text
		return;			                                // Do Nothing
	glColor3ub(255, 205, 0);
	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(Global::fbase - 32);								// Sets The Base Character to 32
	glCallLists((int)strlen(fmt), GL_UNSIGNED_BYTE, fmt);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}

GLvoid glPrint3D(float x, float y, float z, CString fmt, ...)					// Custom GL "Print" Routine
{
	float		length = 0;								// Used To Find The Length Of The Text
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf_s(text, fmt, ap);					    	// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text
	GLsizei siz = (int)strlen(fmt);

	for (unsigned int loop = 0; loop<(strlen(text)); loop++)	// Loop To Find Text Length
	{
		length += gmf[text[loop]].gmfCellIncX;			// Increase Length By Each Characters Width
	}
	glPushMatrix();
	glTranslatef(x, y, z);
	glFrontFace(GL_CW);
	glTranslatef(-length / 2, 0.0f, 0.0f);					// Center Our Text On The Screen

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(baseF3D);									// Sets The Base Character to 0
	glCallLists(siz, GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
	glFrontFace(GL_CCW);
	glPopMatrix();
}


 TWorld::TWorld()
{

}

 TWorld::~TWorld()
{

}


// ********************************************************************************************************************
// TWorld::Init() - INICJALIZACJA SZECHSWIATA ;)
// ******************************************************************************************************************** 
bool  TWorld::Init()
{
//	TGroundNode *tmp, *tmp2;
//	TTrack *TRACK;
//	tmp = new TGroundNode();
//	TRACK = new TTrack(tmp);
	bool Error = false;
	std::string subpath = Global::asCurrentSceneryPath.c_str(); //   "scenery/";
	//cParser parser("scenery.scn", cParser::buffer_FILE, subpath, false);

	CString ax = stdstrtocstr("opopoa13123");
	//WriteLog("string to CString test: " + ax);
	
	WriteLog("Starting MaSzyna rail vehicle simulator.");
	WriteLog("Online documentation and additional files on http://eu07.pl");
	WriteLog("Authors: Marcin_EU, McZapkie, ABu, Winger, Tolaris, nbmx_EU, OLO_EU, Bart, Quark-t, ShaXbee, Oli_EU, youBy, KURS90, Ra, hunter and others");

	// Winger030405: sprawdzanie sterownikow
	CString glrndstr =   (char *)glGetString(GL_RENDERER);
	CString glvenstr =   (char *)glGetString(GL_VENDOR);
	CString glverstr =   (char *)glGetString(GL_VERSION);
	        glexts=split((char *)glGetString(GL_EXTENSIONS), ' ');

	std::string extsnum = itoss(glexts.size());

	WriteLogSS("Graphic GPU   :", std::string(glrndstr));
	WriteLogSS("Graphic Vendor:", std::string(glvenstr));
	WriteLogSS("OpenGL version:", std::string(glverstr));
	WriteLogSS("accepted exts :", extsnum);
  //WriteLog((char *)glGetString(GL_EXTENSIONS));
  
	for (int i = 0; i < glexts.size(); i++) WriteLogSS(">", glexts[i]);

	Global::detonatoryOK = true;

	char glver[100];
	char tolog[100];

	//WriteLogSS(glverstr, glverstr);
	std::vector<std::string> sglver;
	sglver = split(chartostdstr(glverstr), '.');
	sprintf_s(tolog, "%s.%s", sglver[0].c_str(), sglver[1].c_str());
	Global::fOpenGL = float(atof(tolog));
	sprintf_s(glver, "glverdouble: %f", Global::fOpenGL);
	WriteLog("");
  //WriteLog(glver);
	WriteLog("");
	Error = CHECKEXTENSIONS();
	

	Global::bfonttex = TTexturesManager::GetTextureID("font.bmp", 0);  // FOR LOADER
	Global::fonttexturex = TTexturesManager::GetTextureID("font.bmp", 0);
	Global::loaderbackg = TTexturesManager::GetTextureID("logo.bmp", 0);
	Global::logotex = TTexturesManager::GetTextureID("logo.bmp", 0);
	Global::boxtex = TTexturesManager::GetTextureID("boxtex.bmp", 0);
	Global::dirttex = TTexturesManager::GetTextureID("lensdirt_lowc.bmp", 0);

	Resize(1280, 1024);

	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);


	HWND hWIN = GetActiveWindow();
	
	/*-----------------------Render Initialization----------------------*/
	//if (Global::fOpenGL >= 1.2) // poniższe nie działa w 1.1
	//	glTexEnvf(TEXTURE_FILTER_CONTROL_EXT, TEXTURE_LOD_BIAS_EXT, -1);
	
	GLfloat FogColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
	glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT); // Clear screen and depth buffer
	glLoadIdentity();
	glClearColor(0.2f, 0.4f, 0.33f, 1.0f); // Background Color
	WriteLog("glClearDepth(1.0f);  ");
	glClearDepth(1.0f); // ZBuffer Value

    glEnable(GL_NORMALIZE);
	glEnable(GL_RESCALE_NORMAL);

	WriteLog("glEnable(GL_TEXTURE_2D);");
	glEnable(GL_TEXTURE_2D); // Enable Texture Mapping
	WriteLog("glShadeModel(GL_SMOOTH);");
	glShadeModel(GL_SMOOTH); // Enable Smooth Shading
	WriteLog("glEnable(GL_DEPTH_TEST);");
	glEnable(GL_DEPTH_TEST);



	//McZapkie:261102-uruchomienie polprzezroczystosci (na razie linie) pod kierunkiem Marcina

	if (Global::bRenderAlpha)
	{
		glEnable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.04f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthFunc(GL_LEQUAL);
	}
	else
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_BLEND);
	}
	

	WriteLog("glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);");
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST); // Really Nice Perspective Calculations
	WriteLog("glPolygonMode(GL_FRONT, GL_FILL);");
	glPolygonMode(GL_FRONT, GL_FILL);
	WriteLog("glFrontFace(GL_CCW);");
	glFrontFace(GL_CCW); // Counter clock-wise polygons face out
	WriteLog("glEnable(GL_CULL_FACE);	");
	glEnable(GL_CULL_FACE); // Cull back-facing triangles
	WriteLog("glLineWidth(1.0f);");
	glLineWidth(1.0f);
	WriteLog("glPointSize(2.0f);");
	glPointSize(2.0f);
  
// ----------- LIGHTING SETUP ------------------------------------------------------------------------------------------
	glm::vec3 lp = normalize(glm::vec3(-5100, 500, 500));

	Global::lightPos[0] = GLfloat(lp.x);
	Global::lightPos[1] = GLfloat(lp.y);
	Global::lightPos[2] = GLfloat(lp.z);
	Global::lightPos[3] = GLfloat(0.0f);

	// glColor() ma zmieniać kolor wybrany w glColorMaterial()
	WriteLog("glEnable(GL_COLOR_MATERIAL);");
	glEnable(GL_COLOR_MATERIAL);
	WriteLog("glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);");
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	WriteLog("glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, whiteLight );");

	// Ra: szczątkowe światło rozproszone - żeby było cokolwiek widać w ciemności
	WriteLog("glLightModelfv(GL_LIGHT_MODEL_AMBIENT,darkLight);");
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Global::darkLight);
	// Ra: światło 0 - główne światło zewnętrzne (Słońce, Księżyc)
	WriteLog("glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);");
	glLightfv(GL_LIGHT0, GL_AMBIENT, Global::ambientDayLight);
	WriteLog("glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);");
	glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight);
	WriteLog("glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);");
	glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight);
	WriteLog("glLightfv(GL_LIGHT0,GL_POSITION,lightPos);");
	glLightfv(GL_LIGHT0, GL_POSITION, Global::lightPos);
	WriteLog("lMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Global::whiteLight);");
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Global::whiteLight);
	WriteLog("glEnable(GL_LIGHT0);");
	glEnable(GL_LIGHT0);
	WriteLog("glEnable(GL_LIGHTING);");
	glEnable(GL_LIGHTING);

	WriteLog("glFogi(GL_FOG_MODE, GL_LINEAR);");
	glFogi(GL_FOG_MODE, GL_LINEAR); // Fog Mode
	WriteLog("glFogfv(GL_FOG_COLOR, FogColor);");
	glFogfv(GL_FOG_COLOR, FogColor); // Set Fog Color
	//	glFogf(GL_FOG_DENSITY, 0.594f);						// How Dense Will
	glHint(GL_FOG_HINT, GL_NICEST);					  
	WriteLog("glFogf(GL_FOG_START, 1.0f);");
	glFogf(GL_FOG_START, 1.0f); // Fog Start Depth
	WriteLog("glFogf(GL_FOG_END, 200.0f);");
	glFogf(GL_FOG_END, 90.0f); // Fog End Depth
	WriteLog("glEnable(GL_FOG);");
	glEnable(GL_FOG); // Enables GL_FOG

	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	ENALPHATEST(true);
	ENDITHER(false);
	ENMULTISAMPLE(false);
	ENPOLYGONOFFSETLINE(true);
	ENPOLYGONOFFSETPOINT(true);
	ENPOLYGONSMOOTH(false);
	ENSAMPLEALPHATOCOVERAGE(true);
	ENSAMPLEALPHATOONE(true);
	ENFRAMEBUFFERSRGB(false);

	/*--------------------Render Initialization End---------------------*/

	WriteLog("Font init"); // początek inicjacji fontów 2D
	
	Global::bGlutFont = false;

	hDC = GetDC(0);

	if (!Global::bGlutFont) { 
		HFONT font; // Windows Font ID
		Global::fbase = glGenLists(96); // storage for 96 characters
		font = CreateFont(-15, 0, 0, 0, 
		FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Courier New"); 
		SelectObject(hDC, font);
		wglUseFontBitmapsA(hDC, 32, 96, Global::fbase); // builds 96 characters starting at character 32
		WriteLog("Display Lists font used."); //+AnsiString(glGetError())
	}

	//Global::BuildFont();							    // Build The Font
	QBuildFontX();					                //inicailizácia fontu
	BuildFont3D(hDC);
	WriteLog("Font init OK"); 

	WriteLog("Sound Init");
	TSoundsManager::Init(hWIN);
	WriteLog("Sound Init OK");

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	Global::CAMERA.Position_Camera(5, 2.5, 13, 0, 2.5, 3, 0, 1, 0);  // Position      View(target)  Up

    RenderLoader(hDC, 77, "SOUND INITIALIZATION...");
	
	Global::glPrintxy(110, 10, "MaSZyna 2", 0);
	glfwSwapBuffers(Global::window);
	Sleep(2400);

	//Camera.Init(Global::pFreeCameraInit[0], Global::pFreeCameraInitAngle[0]);
	//Global::bFreeFlyModeFlag = true; //Ra: automatycznie włączone latanie
	//Camera.Type = tp_Free;
	//Camera.Reset();


	Global::bActive = true;
 return true;
};

void  TWorld::InOutKey()
{//przełączenie widoku z kabiny na zewnętrzny i odwrotnie

};

void __fastcall TWorld::OnMouseMove(double x, double y)
{//McZapkie:060503-definicja obracania myszy

	Camera.OnCursorMove(x*Global::fMouseXScale, -y*Global::fMouseYScale);
	Global::CAMERA.mWindowWidth = Global::iWindowWidth;
	Global::CAMERA.mWindowHeight = Global::iWindowHeight;
	Global::CAMERA.Mouse_Move();

}

void  TWorld::OnMouseWheel(float zDelta)
{

}

void  TWorld::OnRButtonDown(int X, int Y)
{
	Beep(1500, 100);
}

void  TWorld::OnLButtonDown(int X, int Y)
{
	Beep(1000, 100);
}

// ********************************************************************************************************************
// OnKeyDown()
// ********************************************************************************************************************
void __fastcall TWorld::OnKeyDown(int cKey, int scancode, int action, int mode, std::string command)
{
	float camspeed = 2.0f * Global::fdt;

	if (GetKeyState(VK_CONTROL) & 0x80) camspeed = 16.0f * (float)Global::fdt;
	if (GetKeyState(VK_SHIFT) & 0x80) camspeed = 32.0f * (float)Global::fdt;
	if (GetKeyState(VK_TAB) & 0x80) camspeed = 128.0f * (float)Global::fdt;

	Global::KEYCOMMAND = ReplaceCharInString(Global::KEYCOMMAND, '"', "");

	//WriteLogSS("KEYCOMMAND=[", Global::KEYCOMMAND + "]");
	if ((GetKeyState('W') & 0x80))  Global::CAMERA.Move_Camera(CAMERASPEED*float(camspeed));

	if ((GetKeyState('S') & 0x80)) Global::CAMERA.Move_Camera(-CAMERASPEED*float(camspeed));

	if ((GetKeyState('A') & 0x80)) Global::CAMERA.Rotate_View(0, -ROTATESPEED*camspeed, 0);

	if ((GetKeyState('D') & 0x80)) Global::CAMERA.Rotate_View(0, ROTATESPEED*camspeed, 0);

	if ((GetKeyState('Q') & 0x80)) Global::CAMERA.Move_CameraU(CAMERASPEED*camspeed);

	if ((GetKeyState('E') & 0x80)) Global::CAMERA.Move_CameraD(-CAMERASPEED*camspeed);

	if (Global::KEYCOMMAND == "SCREENSHOT") 
	{ 
		takeScreenshot("screenshot.png"); Beep(1000, 50); Sleep(50); Beep(1000, 50); Sleep(50);
		Global::KEYCOMMAND = ""; 
	}


}

bool  TWorld::Resize(int Width, int Height)
{
	return true;
}

bool  TWorld::Update(double dt)
{
 //Camera.Pos = Global::Camerapos;
// Camera.Update(); //uwzględnienie ruchu wywołanego klawiszami

 Global::pCameraPosition.x = Global::CAMERA.mPos.x;
 Global::pCameraPosition.y = Global::CAMERA.mPos.y;
 Global::pCameraPosition.z = Global::CAMERA.mPos.z;

 campos.x = Global::CAMERA.mPos.x;
 campos.y = Global::CAMERA.mPos.y;
 campos.z = Global::CAMERA.mPos.z;

 if (!Render(dt, 1)) return false;

 return (true);
};

bool switch2dRender()
{
	int GWW = Global::iWindowWidth;
	int GWH = Global::iWindowHeight;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, Global::iWindowWidth, Global::iWindowHeight, 0, -100, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	return true;
}


// *****************************************************************************
// RenderX()
// *****************************************************************************

bool  TWorld::RenderX()
{
 //glEnable(GL_TEXTURE_2D);
 //glBindTexture(GL_TEXTURE_2D, 0);
 glLineWidth(0.4f);
 glColor4f(0.9f, 0.2f, 0.0f, 0.7f);

 glPushMatrix(); 
 glTranslatef(-4.6f, 1.75f, 6.0f);
 glBegin(GL_LINE_LOOP);
 glVertex2f(0.25f, 0.25f);
 glVertex2f(0.90f, 0.25f);
 glVertex2f(0.90f, 0.90f);
 glVertex2f(0.25f, 0.90f);
 glEnd();
 glPopMatrix();

 return true;
}

// *********************************************************************************************************************
// TWorld::Render()
// *********************************************************************************************************************

bool  TWorld::Render(double dt, int id)
{

	char  szBuffer[100];
	glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(Global::FOV, (GLdouble)Global::iWindowWidth / (GLdouble)Global::iWindowHeight, 0.1f, 13234566.0f);  //1999950600.0f
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//Camera.SetMatrix();
	glLightfv(GL_LIGHT0, GL_POSITION, Global::lightPos);

	gluLookAt(campos.x, campos.y, campos.z, Global::CAMERA.mView.x, Global::CAMERA.mView.y, Global::CAMERA.mView.z, Global::CAMERA.mUp.x, Global::CAMERA.mUp.y, Global::CAMERA.mUp.z);

	DRAW_XYGRID();

	Draw_SCENE000(0, 0, 0);
	
	glDisable(GL_FOG);
    //Clouds.Render();
    glEnable(GL_FOG);

	ENTEX(0);
	RenderX();

	ENTEX(0);
	//glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(0.9f, 0.6f, 0.1f, 1.0f);
	glPushMatrix();
	glPrint3D(0, 2, 6, "Nowa MaSZyna 2");
	glPopMatrix();


	//switch2dRender();
	ENTEX(0);
	//glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
	glLoadIdentity();
	glTranslatef(0.0f, 0.3f, -0.80f);

	sprintf_s(fps, "fps: %.3f fdt: %.3g", Global::FPS, Global::fdt);
	glRasterPos2f(-0.40f, -0.01f);
	glPrint(fps);

	sprintf_s(str, "key: id=%i scode=%i act=%i str=%s", Global::keyid, Global::keyscancode, Global::keyaction, Global::KEYCODE.c_str());
	glRasterPos2f(-0.40f, -0.03f);
	glPrint(str);

	sprintf_s(str, "mods: id=%i", Global::keymods);
	glRasterPos2f(-0.40f, -0.04f);
	glPrint(str);

	glColor4f(0.2f, 0.8f, 0.2f, 0.9f);
	sprintf_s(str, "command: %s", Global::KEYCOMMAND.c_str());
	glRasterPos2f(-0.40f, -0.06f);
	glPrint(str);

	glRasterPos2f(-0.40f, -0.02f);
	//glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
	glPrint("----------------------------");

	ENTEX(1);
	sprintf_s(szBuffer, "Symulator Pojazdow Trakcyjnych MaSZyna 2");
	TColor rgba = Global::SetColor(0.9f, 0.7f, 0.0f, 0.9f);
	QglPrint_(2, 1, szBuffer, 1, rgba);

	sprintf_s(szBuffer, "Symulator Pojazdow Trakcyjnych MaSZyna 4");
    rgba = Global::SetColor(0.2f, 0.2f, 0.2f, 0.9f);
	QglPrint(10, 1015, szBuffer, 0, rgba);
	
 return true;
};

