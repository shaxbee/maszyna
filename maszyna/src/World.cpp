//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others
*/
// 2015.04.13 has included other soundsystem files
// 2015.04.13 starts changing char pointer type into CString (more flexible)
// 2015.04.14 removed many compiler warnings
// 2015.04.16 unsuccessful attempt to connect ground.cpp module
// 2015.04.16 changing directories for logs and configuration files
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
C:\FreeImage\Dist\x64\freeimage.lib
*/

// ADDITIONAL INCLUDE DIR
//C:\DEPENDIENCES\freeimage\FreeImageLib\Release

#include "commons.h"
#include "commons_usr.h"

//#include "shaders.h"
//#include "glmath.h"
#pragma hdrstop

TCamera Camera;
vector3 campos;
GLuint logo;
char fps[60];
char str[60];
std::vector<std::string> glexts;
CString ModuleDirectory, ErrorLog;
GLfloat  lightPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

TSky::~TSky()
{

};

TSky::TSky()
{

};

void  TSky::Init()
{
	WriteLog(Global::asSky.c_str());
	WriteLog("init");
	std::string asModel;
	asModel = Global::asSky = "cgskj_blueclear008.t3d";

	//anCloud = new TAnimModel();
	//anCloud->Load()
	if ((asModel != "1") && (asModel != "0")) mdCloud = TModelsManager::GetModel("cgskj_blueclear008.t3d", false);
	
};

void TSky::Render()
{
	if (mdCloud)
	{//jeśli jest model nieba
		glPushMatrix();
		//glDisable(GL_DEPTH_TEST);
		glTranslatef(Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z);
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
		if (Global::bUseVBO)
		{//renderowanie z VBO
		//-	mdCloud->RaRender(100, 0);
		//-	mdCloud->RaRenderAlpha(100, 0);
		}
		else
		{//renderowanie z Display List
		//-	mdCloud->Render(100, 0);
		//-	mdCloud->RenderAlpha(100, 0);
		}
		//glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		//glEnable(GL_LIGHTING);
		glPopMatrix();
		glLightfv(GL_LIGHT0, GL_POSITION, Global::lightPos);
	}
};



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
	hDC = GetDC(0);
	bool Error = false;
	std::string subpath = Global::asCurrentSceneryPath.c_str(); //   "scenery/";

	CString ax = stdstrtocstr("opopoa13123");
	//WriteLog("string to CString test: " + ax);
	
	WriteLog("Starting MaSzyna rail vehicle simulator.");
	WriteLog("Online documentation and additional files on http://eu07.pl");
	WriteLog("Authors: Marcin_EU, McZapkie, ABu, Winger, Tolaris, nbmx_EU, OLO_EU, Bart, Quark-t, ShaXbee, Oli_EU, youBy, KURS90, hunter, Ra i Ja");

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

	GLfloat FogColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
	glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT); // Clear screen and depth buffer
	glLoadIdentity();
	glClearColor(0.2f, 0.4f, 0.33f, 1.0f); // Background Color
	WriteLog("glClearDepth(1.0f);  ");
	glClearDepth(1.0f); // ZBuffer Value

	WriteLog("glEnable(GL_TEXTURE_2D);");
	glEnable(GL_TEXTURE_2D); // Enable Texture Mapping
	WriteLog("glShadeModel(GL_SMOOTH);");
	glShadeModel(GL_SMOOTH); // Enable Smooth Shading
	WriteLog("glEnable(GL_DEPTH_TEST);");
	glEnable(GL_DEPTH_TEST);

	AlphaBlendMode(Global::bRenderAlpha); //McZapkie:261102-uruchomienie polprzezroczystosci (na razie linie) pod kierunkiem Marcina
	
	WriteLog("glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);");
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	WriteLog("glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);");
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
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

	WriteLog("glEnable(GL_NORMALIZE);");
	glEnable(GL_NORMALIZE);
	WriteLog("glEnable(GL_RESCALE_NORMAL);");
	glEnable(GL_RESCALE_NORMAL);
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

// FOG SETUP ----------------------------------------------------------------------------------------------------------
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



	EN_ALPHATEST(true);
	EN_DITHER(false);
	EN_MULTISAMPLE(false);
	EN_POLYGONOFFSETLINE(true);
	EN_POLYGONOFFSETPOINT(true);
	EN_POLYGONSMOOTH(false);
	EN_SAMPLEALPHATOCOVERAGE(true);
	EN_SAMPLEALPHATOONE(true);
	EN_FRAMEBUFFERSRGB(false);

	/*--------------------Render Initialization End---------------------*/

	WriteLog("Font init"); // początek inicjacji fontów 2D
	
	Global::bGlutFont = false;


	HFONT font; // Windows Font ID
	Global::fbase = glGenLists(96); // storage for 96 characters
	font = CreateFont(-15, 0, 0, 0, 
	FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Courier New"); 
	SelectObject(hDC, font);
	wglUseFontBitmapsA(hDC, 32, 96, Global::fbase); // builds 96 characters starting at character 32
	WriteLog("Display Lists font used."); //+AnsiString(glGetError())


  //Global::BuildFont();							
	QBuildFontX();					              
	BuildFont3D(hDC);
	WriteLog("Font init OK"); 

	WriteLog("Sound Init");
	TSoundsManager::Init(hWIN);
	WriteLog("Sound Init OK");

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	Global::CAMERA.Position_Camera(5, 2.5, 13, 0, 2.5, 3, 0, 1, 0);  // Position      View(target)  Up

    RenderLoader(hDC, 77, "SOUND INITIALIZATION...");
	
	//Global::glPrintxy(110, 10, "MaSZyna 2", 0);
	glfwSwapBuffers(Global::window);
	Sleep(2400);

	//cParser parser("scenery.scn", cParser::buffer_FILE, subpath, false);


	//Camera.Init(Global::pFreeCameraInit[0], Global::pFreeCameraInitAngle[0]);
	//Global::bFreeFlyModeFlag = true; //Ra: automatycznie włączone latanie
	//Camera.Type = tp_Free;
	//Camera.Reset();
	WriteLog("CREATE TObject3d instance...");
	TObject3d *O3D;
	O3D = new TObject3d();
	WriteLog("CREATE TObject3d instance OK.");

	WriteLog("CREATE TSubObject instance...");
	TSubObject *SO3D;
	SO3D = new TSubObject();
	WriteLog("CREATE TSubObject instance OK.");

	char *Name;
	char *newName ="zlksajdkjskdjs";
	Name = new char[strlen(newName) + 1];

	Clouds.Init();


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

	EN_TEX(0);
	RenderX();

	EN_TEX(0);
	//glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(0.9f, 0.6f, 0.1f, 1.0f);
	glPushMatrix();
	glPrint3D(0, 2, 6, "Nowa MaSZyna 2");
	glPopMatrix();


	//switch2dRender();
	EN_TEX(0);
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

	EN_TEX(1);
	sprintf_s(szBuffer, "Symulator Pojazdow Trakcyjnych MaSZyna 2");
	TColor rgba = Global::SetColor(0.9f, 0.7f, 0.0f, 0.9f);
	QglPrint_(2, 1, szBuffer, 1, rgba);

	sprintf_s(szBuffer, "Symulator Pojazdow Trakcyjnych MaSZyna 4");
    rgba = Global::SetColor(0.2f, 0.2f, 0.2f, 0.9f);
	QglPrint(10, 1015, szBuffer, 0, rgba);
	
 return true;
};

