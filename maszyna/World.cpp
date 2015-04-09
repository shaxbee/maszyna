//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others
*/

#include "commons.h"
#include "commons_usr.h"
#include "opengl\gl.h"			// Header File For The OpenGL32 Library
#include "opengl\glu.h"			// Header File For The GLu32 Library
#include "opengl\glut.h"	
#include "opengl\glaux.h"		// Header File For The Glaux Library
#include "xbitmap_font.h"
#include "freetype.h"

#pragma hdrstop


//#pragma comment(lib,"glaux.lib")
//#pragma comment(lib,"glut32.lib")
#pragma comment(lib,"glu32.lib")
//#pragma comment(lib,"glew32.lib")
//#pragma comment(lib,"glew32s.lib")
GLuint	fbase;				// Base Display List For The Font
GLuint	texture;			// Storage For Our Font Texture

GLYPHMETRICSFLOAT gmf[256];	// Storage For Information About Our Outline Font Characters
GLuint baseF3D;


//---------------------------------------------------------------------------
//#pragma package(smart_init)


// ********************************************************************************************************************
// BuildFont3D() - DO NAPISOW 3D 
// ********************************************************************************************************************

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
		FW_BOLD,						// Font Weight
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
		0.05f,							// Font Thickness In The Z Direction
		WGL_FONT_POLYGONS,				// Use Polygons, Not Lines
		gmf);							// Address Of Buffer To Recieve Data
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// BoolToStr() 
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

int width = 0;
int height = 0;
short BitsPerPixel = 0;
std::vector<unsigned char> Pixels;

GLuint LoadTexture(const char * filename)
{

	GLuint texture;

	int width, height;

	unsigned char * data;

	FILE * file;

	file = fopen(filename, "rb");

	if (file == NULL) return 0;
	width = 1024;
	height = 512;
	data = (unsigned char *)malloc(width * height * 3);
	//int size = fseek(file,);
	fread(data, width * height * 3, 1, file);
	fclose(file);

	for (int i = 0; i < width * height; ++i)
	{
		int index = i * 3;
		unsigned char B, R;
		B = data[index];
		R = data[index + 2];

		data[index] = R;
		data[index + 2] = B;

	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);


	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

	free(data);

	return texture;
}

GLvoid BuildFont(GLvoid)								// Build Our Font Display List
{
	float	cx;											// Holds Our X Character Coord
	float	cy;											// Holds Our Y Character Coord

	fbase = glGenLists(256);								// Creating 256 Display Lists
	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);			// Select Our Font Texture
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

GLvoid KillFont(GLvoid)									// Delete The Font From Memory
{
	glDeleteLists(fbase, 256);							// Delete All 256 Display Lists
}

GLvoid glPrint(GLint x, GLint y, char *string, int set)	// Where The Printing Happens
{
	if (set>1)
	{
		set = 1;
	}
	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);			// Select Our Font Texture
	glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPushMatrix();										// Store The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	glOrtho(0, 1280, 0, 1024, -1, 1);							// Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPushMatrix();										// Store The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslated(x, y, 0);								// Position The Text (0,0 - Bottom Left)
	glListBase(fbase - 32 + (128 * set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string), GL_UNSIGNED_BYTE, string);// Write The Text To The Screen
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
}


GLvoid TWorld::glPrint(const char *fmt)					// Custom GL "Print" Routine
{

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing
	glColor3ub(255, 205, 0);
	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(fbase - 32);								// Sets The Base Character to 32
	glCallLists(strlen(fmt), GL_UNSIGNED_BYTE, fmt);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}

GLvoid glPrint3D(float x, float y, float z, const char *fmt, ...)					// Custom GL "Print" Routine
{
	float		length = 0;								// Used To Find The Length Of The Text
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

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
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
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

bool  TWorld::Init()
{


	/*-----------------------Render Initialization----------------------*/


	GLfloat  FogColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	                        // Clear screen and depth buffer
	glLoadIdentity();

	glClearColor(0.2, 0.4, 0.33, 1.0);                                         // Background Color

	glFogfv(GL_FOG_COLOR, FogColor);					        // Set Fog Color

	glClearDepth(1.0f);                                                         // ZBuffer Value


	//  glEnable(GL_NORMALIZE);
	//  glEnable(GL_RESCALE_NORMAL);

	glEnable(GL_CULL_FACE);

	glEnable(GL_TEXTURE_2D);						        // Enable Texture Mapping

	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading

	glEnable(GL_DEPTH_TEST);


	//  our_font.init("data\\consolefont.TTF", 16);					//Build the freetype font



	//McZapkie:261102-uruchomienie polprzezroczystosci (na razie linie) pod kierunkiem Marcina
	//if (Global::bRenderAlpha)
	{

		glEnable(GL_BLEND);

		glEnable(GL_ALPHA_TEST);

		glAlphaFunc(GL_GREATER, 0.04);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDepthFunc(GL_LEQUAL);
	}
	//else
	/*
	{

		glEnable(GL_ALPHA_TEST);

		glAlphaFunc(GL_GREATER, 0.5);

		glDepthFunc(GL_LEQUAL);

		glDisable(GL_BLEND);
	}
*/

	glHint(GL_PERSPECTIVE_CORRECTION_HINT | GL_POINT_SMOOTH_HINT | GL_POLYGON_SMOOTH_HINT, GL_NICEST);	   // Really Nice Perspective Calculations


	glPolygonMode(GL_FRONT, GL_FILL);

	glFrontFace(GL_CCW);		                                        // Counter clock-wise polygons face out

	glEnable(GL_CULL_FACE);		                                        // Cull back-facing triangles

	glLineWidth(0.3f);

	glPointSize(2.0f);

	// LINES ANITIALIASING
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	glLineWidth(0.5);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);


	// ----------- LIGHTING SETUP -----------

	//vector3 lp = Normalize(vector3(-500, 500, 200));

	//Global::lightPos[0] = lp.x;
	//Global::lightPos[1] = lp.y;
	//Global::lightPos[2] = lp.z;
	//Global::lightPos[3] = 0.0f;

	// Setup and enable light 0

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Global::ambientDayLight);
	//  glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);

	glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight);

	glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight);

	glLightfv(GL_LIGHT0, GL_POSITION, Global::lightPos);

	glEnable(GL_LIGHT0);

	// Enable color tracking

	glEnable(GL_COLOR_MATERIAL);

	// Set Material properties to follow glColor values

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Global::whiteLight);

	glEnable(GL_LIGHTING);


	glFogi(GL_FOG_MODE, GL_LINEAR);			                        // Fog Mode

	glFogfv(GL_FOG_COLOR, FogColor);					        // Set Fog Color

	glFogf(GL_FOG_DENSITY, 0.594f);						// How Dense Will The Fog Be

	glFogf(GL_FOG_START, 10.0f);						// Fog Start Depth

	glFogf(GL_FOG_END, 200.0f);						        // Fog End Depth

	glEnable(GL_FOG);								// Enables GL_FOG

	/*--------------------Render Initialization End---------------------*/

	glColor4f(1.0f, 3.0f, 3.0f, 0.0f);


	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glTranslatef(0.0f, 0.0f, -0.50f);
	glDisable(GL_DEPTH_TEST);			                                // Disables depth testing
	glColor3f(3.0f, 3.0f, 3.0f);
	glColor3f(0.0f, 0.0f, 100.0f);

	glEnable(GL_LIGHTING);


	
	HDC hDC = GetDC(NULL);
	
	Global::fonttexturex = LoadTexture("font.bmp");
	texture = LoadTexture("font.bmp");
	glGenTextures(1, &Global::fonttexturex);
	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, Pixels.data());
	BuildFont();										// Build The Font
	QBuildFontX();					                //inicailizácia fontu
	BuildFont3D(hDC);
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Clear The Background Color To Black
	glClearDepth(1.0);									// Enables Clearing Of The Depth Buffer
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Test To Do
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);					// Select The Type Of Blending
	glShadeModel(GL_SMOOTH);							// Enables Smooth Color Shading
	glEnable(GL_TEXTURE_2D);							// Enable 2D Texture Mapping
	
	HFONT	font;										// Windows Font ID
	
	fbase = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(-15,							// Height Of Font
		0,								// Width Of Font
		0,								// Angle Of Escapement
		0,								// Orientation Angle
		FW_BOLD,						// Font Weight
		FALSE,							// Italic
		FALSE,							// Underline
		FALSE,							// Strikeout
		ANSI_CHARSET,					// Character Set Identifier
		OUT_TT_PRECIS,					// Output Precision
		CLIP_DEFAULT_PRECIS,			// Clipping Precision
		ANTIALIASED_QUALITY,			// Output Quality
		FF_DONTCARE | DEFAULT_PITCH,		// Family And Pitch
		"Courier New");					// Font Name

	SelectObject(hDC, font);							// Selects The Font We Want
	
	wglUseFontBitmaps(hDC, 32, 96, fbase);				// Builds 96 Characters Starting At Character 32
	
 return true;
};

void  TWorld::InOutKey()
{//prze³¹czenie widoku z kabiny na zewnêtrzny i odwrotnie

};

bool  TWorld::Update(double dt)
{
 if (!Render(dt)) return false;

 return (true);
};



// *****************************************************************************
// RenderX()
// *****************************************************************************

bool  TWorld::RenderX()
{

 glLineWidth(0.2);
 glClearColor(0.4, 0.5, 0.6, 1.0);
 glColor4f(0.3, 0.3, 1.0, 1.0);

 glBegin(GL_LINE_LOOP);
 glVertex2f(0.25, 0.25);
 glVertex2f(0.75, 0.25);
 glVertex2f(0.75, 0.75);
 glVertex2f(0.25, 0.75);
 glEnd();
 glFlush();
 return true;
}


// *****************************************************************************
// TWorld::Render()
// *****************************************************************************

bool  TWorld::Render(double dt)
{
	char  szBuffer[100];
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.50, 0.55, 0.90, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluPerspective(45, (GLdouble)1280/(GLdouble)1024, 0.1f, 13234566.0f);  //1999950600.0f
    glMatrixMode(GL_MODELVIEW);

	//gluLookAt(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);
	glColor4f(0.2f, 0.4f, 0.1f, 0.90f);
	sprintf(szBuffer, "cam: X%5.3f Y%5.3f Z%5.3f", 1.0, 2.22, 3.3);
	QglPrint_(2, 33, szBuffer, 1);

    glDisable(GL_FOG);
    //Clouds.Render();
    glEnable(GL_FOG);

	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);

	glPrint3D(0, 2, 6, "sfdasdgdfhdghhdfh");

	glTranslatef(0, -2, 0);
	glPushMatrix();
	// White side - BACK
	glBegin(GL_POLYGON);
	glColor3f(1.0, 1.0, 1.0);
	glVertex3f(0.5, -0.5, 0.5);
	glVertex3f(0.5, 0.5, 0.5);
	glVertex3f(-0.5, 0.5, 0.5);
	glVertex3f(-0.5, -0.5, 0.5);
	glEnd();

	// Purple side - RIGHT
	glBegin(GL_POLYGON);
	glColor3f(1.0, 0.0, 1.0);
	glVertex3f(0.5, -0.5, -0.5);
	glVertex3f(0.5, 0.5, -0.5);
	glVertex3f(0.5, 0.5, 0.5);
	glVertex3f(0.5, -0.5, 0.5);
	glEnd();

	// Green side - LEFT
	glBegin(GL_POLYGON);
	glColor3f(0.0, 1.0, 0.0);
	glVertex3f(-0.5, -0.5, 0.5);
	glVertex3f(-0.5, 0.5, 0.5);
	glVertex3f(-0.5, 0.5, -0.5);
	glVertex3f(-0.5, -0.5, -0.5);
	glEnd();

	// Blue side - TOP
	glBegin(GL_POLYGON);
	glColor3f(0.0, 0.0, 1.0);
	glVertex3f(0.5, 0.5, 0.5);
	glVertex3f(0.5, 0.5, -0.5);
	glVertex3f(-0.5, 0.5, -0.5);
	glVertex3f(-0.5, 0.5, 0.5);
	glEnd();

	// Red side - BOTTOM
	glBegin(GL_POLYGON);
	glColor3f(1.0, 0.0, 0.0);
	glVertex3f(0.5, -0.5, -0.5);
	glVertex3f(0.5, -0.5, 0.5);
	glVertex3f(-0.5, -0.5, 0.5);
	glVertex3f(-0.5, -0.5, -0.5);
	glEnd();
	glPopMatrix();

	RenderX();
	glFlush();

 return true;
};

