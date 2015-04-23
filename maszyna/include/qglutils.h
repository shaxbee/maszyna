#ifndef qglutilsH
#define qglutilsH


#include "../commons.h"
#include "../commons_usr.h"

/*
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
*/
static GLYPHMETRICSFLOAT gmf[256];	// Storage For Information About Our Outline Font Characters
static GLuint baseF3D;
static HDC hDC;



// *****************************************************************************************
// BuildFont3D() - DO NAPISOW 3D ***********************************************************
// *****************************************************************************************

inline GLvoid BuildFont3D(HDC hDC)								// Build Our Bitmap Font
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


inline GLvoid glPrint3D(float x, float y, float z, CString fmt, ...)					// Custom GL "Print" Routine
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


inline GLvoid TWorld::glPrint(CString fmt)					// Custom GL "Print" Routine
{

	if (fmt == NULL)									// If There's No Text
		return;			                                // Do Nothing
	glColor3ub(255, 205, 0);
	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(Global::fbase - 32);								// Sets The Base Character to 32
	glCallLists((int)strlen(fmt), GL_UNSIGNED_BYTE, fmt);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}

inline void AlphaBlendMode(bool enabled)
{
	if (enabled)
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

}

// *****************************************************************************
//
// *****************************************************************************
inline void EN_TEX(bool state)
{
	if (state == false) glDisable(GL_TEXTURE_2D); else glEnable(GL_TEXTURE_2D);
}

inline void EN_LIGHTING(bool state)
{
	if (state == false) glDisable(GL_LIGHTING); else glEnable(GL_LIGHTING);
}

//If enabled, blend the computed fragment color values with the values in the color buffers.See glBlendFunc.
inline void EN_BLEND(bool state)
{
	if (state == false) glDisable(GL_BLEND); else glEnable(GL_BLEND);
}

//If enabled, do depth comparisons and update the depth buffer.Note that even if the depth buffer exists and the depth mask is non - zero, the depth buffer is not updated if the depth test is disabled.See glDepthFunc and glDepthRange.
inline void EN_DEPTHTEST(bool state)
{
	if (state == false) glDisable(GL_DEPTH_TEST); else glEnable(GL_DEPTH_TEST);
}

//If enabled, draw lines with correct filtering. Otherwise, draw aliased lines. See glLineWidth. 
inline void EN_LINESMOOTH(bool state)
{
	if (state == false) glDisable(GL_LINE_SMOOTH); else glEnable(GL_LINE_SMOOTH);
}

inline void EN_NORMALIZE(bool state)
{
	if (state == false) glDisable(GL_NORMALIZE); else glEnable(GL_NORMALIZE);
}

inline void EN_RESCALENORMAL(bool state)
{
	if (state == false) glDisable(GL_RESCALE_NORMAL); else glEnable(GL_RESCALE_NORMAL);
}

inline void EN_ALPHATEST(bool state)
{
	if (state == false) glDisable(GL_ALPHA_TEST); else glEnable(GL_ALPHA_TEST);
}

// If enabled, cull polygons based on their winding in window coordinates. See glCullFace. 
inline void EN_CULLFACE(bool state)
{
	if (state == false) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
}

inline void EN_COLORMATERIAL(bool state)
{
	if (state == false) glDisable(GL_COLOR_MATERIAL);else glEnable(GL_COLOR_MATERIAL);
}

inline void EN_FOG(bool state)
{
	if (state == false) glDisable(GL_FOG); else glEnable(GL_FOG);
}

//If enabled, dither color components or indices before they are written to the color buffer. 
inline void EN_DITHER(bool state)
{
	if (state == false) glDisable(GL_DITHER);  else glEnable(GL_DITHER);
}

//If enabled, use multiple fragment samples in computing the final color of a pixel. See glSampleCoverage. 
inline void EN_MULTISAMPLE(bool state)
{
	if (state == false) glDisable(GL_MULTISAMPLE); else glEnable(GL_MULTISAMPLE);
}

//If enabled, and if the polygon is rendered in GL_FILL mode, an offset is added to depth values of a polygon's fragments before the depth comparison is performed. See glPolygonOffset. 
inline void EN_POLYGONOFFSETFILL(bool state)
{
	if (state == false) glDisable(GL_POLYGON_OFFSET_FILL); else glEnable(GL_POLYGON_OFFSET_FILL);
}

//If enabled, and if the polygon is rendered in GL_LINE mode, an offset is added to depth values of a polygon's fragments before the depth comparison is performed. See glPolygonOffset. 
inline void EN_POLYGONOFFSETLINE(bool state)
{
	if (state == false) glDisable(GL_POLYGON_OFFSET_LINE); else glEnable(GL_POLYGON_OFFSET_LINE);
}

//If enabled, an offset is added to depth values of a polygon's fragments before the depth comparison is performed, if the polygon is rendered in GL_POINT mode. See glPolygonOffset. 
inline void EN_POLYGONOFFSETPOINT(bool state)
{
	if (state == false) glDisable(GL_POLYGON_OFFSET_POINT); else glEnable(GL_POLYGON_OFFSET_POINT);
}

//If enabled, draw polygons with proper filtering. Otherwise, draw aliased polygons. For correct antialiased polygons, an alpha buffer is needed and the polygons must be sorted front to back. 
inline void EN_POLYGONSMOOTH(bool state)
{
	if (state == false) glDisable(GL_POLYGON_SMOOTH); else glEnable(GL_POLYGON_SMOOTH);
}

//If enabled, compute a temporary coverage value where each bit is determined by the alpha value at the corresponding sample location. The temporary coverage value is then ANDed with the fragment coverage value. 
inline void EN_SAMPLEALPHATOCOVERAGE(bool state)
{
	if (state == false) glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE); else glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
}

//If enabled, each sample alpha value is replaced by the maximum representable alpha value.
inline void EN_SAMPLEALPHATOONE(bool state)
{
	if (state == false) glDisable(GL_SAMPLE_ALPHA_TO_ONE); else glEnable(GL_SAMPLE_ALPHA_TO_ONE);
}

//If enabled, the fragment's coverage is ANDed with the temporary coverage value. If GL_SAMPLE_COVERAGE_INVERT is set to GL_TRUE, invert the coverage value. See glSampleCoverage. 
inline void EN_SAMPLECOVERAGE(bool state)
{
	if (state == false) glDisable(GL_SAMPLE_COVERAGE); else glEnable(GL_SAMPLE_COVERAGE);
}

//If enabled and the value of GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING for the framebuffer attachment corresponding to 
//the destination buffer is GL_SRGB, the R, G, and B destination color values (after conversion from fixed-point to 
//floating-point) are considered  to be encoded for the sRGB color space and hence are linearized prior to their use in blending. 
inline void EN_FRAMEBUFFERSRGB(bool state)
{
	if (state == false) glDisable(GL_FRAMEBUFFER_SRGB); else glEnable(GL_FRAMEBUFFER_SRGB);
}



inline void Draw_SCENE000(double sx, double sy, double sz)
{
	GLboolean blendEnabled;
	GLint blendSrc;
	GLint blendDst;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);
	glDisable(GL_TEXTURE_2D); // Enable Texture Mapping
	glEnable(GL_BLEND);

	glDisable(GL_LIGHTING);
	glLineWidth(0.8);
	glColor4f(0.9, 0.2, 0.2, 0.1);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 10, 0);
	glVertex3f(0, 0, 0);
	glEnd();
	glColor4f(0.4, 0.9, 0.4, 0.1);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0, 0, 0);
	glVertex3f(1000, 0, 0);
	glVertex3f(0, 0, 0);
	glEnd();

	glColor4f(0.4, 0.4, 0.9, 0.1);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, -1000);
	glVertex3f(0, 0, 0);
	glEnd();

	glPushMatrix();
	glTranslatef(0, 0, 0);
	//glutSolidSphere(0.3, 12, 12);
	glPopMatrix();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glBlendFunc(blendSrc, blendDst);
	glDisable(GL_BLEND);
}


// ****************************************************************************************
// Draw_Grid()
// ****************************************************************************************

inline bool DRAW_XYGRID()
{
	float cxx = Global::pCameraPosition.x;
	float czz = Global::pCameraPosition.z;
	cxx = ceil(Global::pCameraPosition.x / 10) * 10;
	czz = ceil(Global::pCameraPosition.z / 10) * 10;


	glEnable(GL_LINE_SMOOTH);
	// glHint( GL_LINE_SMOOTH_HINT, GL_DONT_CARE );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // PRO BLEND

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D); // Enable Texture Mapping
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);

	// if (Global::bGRIDPROAAA) glEnable(GL_BLEND);
	//glDisable(GL_COLOR_MATERIAL);
	glLineWidth(0.02);
	glPushMatrix();
	glTranslatef(cxx, 0, czz);

	// if (!CBGRIDDT) glDisable(GL_DEPTH_TEST);
	// if (CBGRIDBLEND)
	



	glLineWidth(0.01);
	glColor4f(0.2, 0.2, 0.2, 0.5f);
	int bound = 100;
	for (float i = -bound; i <= bound; i += 1)
	{
		glBegin(GL_LINES);
		glVertex3f(-bound, 0, i);
		glVertex3f(bound, 0, i);
		glVertex3f(i, 0, -bound);
		glVertex3f(i, 0, bound);
		glEnd();
	}
	glColor4f(0.3, 0.3, 0.3, 0.7f);
	glLineWidth(0.5);
	bound = 100;
	for (float i = -bound; i <= bound; i += 10)
	{
		glBegin(GL_LINES);
		glVertex3f(-bound, 0, i);
		glVertex3f(bound, 0, i);
		glVertex3f(i, 0, -bound);
		glVertex3f(i, 0, bound);
		glEnd();
	}

	glPopMatrix();

	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D); // Enable Texture Mapping

	//if (Global::isFOGON) glEnable(GL_FOG);
	return true;
}


inline bool CHECKEXTENSIONS()
{
	WriteLog("Checking extensions...");
	bool Error = false;

	
	if (!GLEW_ARB_texture_non_power_of_two)
	{
		WriteLogSS("GL_ARB_texture_non_power_of_two not supported!", "ERROR");
		//ErrorLog.Append("GL_ARB_texture_non_power_of_two not supported!\r\n");
		Error = true;
	}
	else WriteLog("GLEW_ARB_texture_non_power_of_two OK.");

	if (!GLEW_ARB_depth_texture)
	{
		WriteLogSS("GL_ARB_depth_texture not supported!", "ERROR");
		//ErrorLog.Append("GL_ARB_depth_texture not supported!\r\n");
		Error = true;
	}
	else WriteLog("GLEW_ARB_depth_texture OK.");

	if (!GLEW_EXT_framebuffer_object)
	{
		WriteLogSS("GLEW_EXT_framebuffer_object not supported!", "ERROR");
		//ErrorLog.Append("GL_EXT_framebuffer_object not supported!\r\n");
		Error = true;
	}
	else WriteLog("GLEW_EXT_framebuffer_object OK.");

	WriteLog("");
	WriteLog("");
	return Error;
}

inline void takeScreenshot(const char* screenshotFile)
{
	char SSHOTFILEJPG[256];
	char SSHOTFILEPNG[256];
	char FN[80];
	std::time_t rawtime;
	std::tm* timeinfo;

	std::time(&rawtime);
	timeinfo = std::localtime(&rawtime);

	std::strftime(FN, 80, "[%Y%m%d %H%M%S].png", timeinfo);

	WriteLog("SAVING SCREEN TO PNG");

	strcpy(SSHOTFILEPNG, "SCREENSHOT//");

	strcat(SSHOTFILEPNG, FN); // =  FDATE + ".bmp";

	WriteLog(SSHOTFILEPNG);


	ILuint imageID = ilGenImage();
	ilBindImage(imageID);
	ilutGLScreen();
	ilEnable(IL_FILE_OVERWRITE);
	ilSaveImage(SSHOTFILEPNG);
	ilDeleteImage(imageID);
	//printf("Screenshot saved to: %s\n", screenshotFile);

}

//---------------------------------------------------------------------------
#endif
