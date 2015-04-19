#ifndef qglutilsH
#define qglutilsH


#include "../commons.h"
#include "../commons_usr.h"


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


// *****************************************************************************
//
// *****************************************************************************
inline void ENTEX(bool state)
{
	if (state == false) glDisable(GL_TEXTURE_2D); else glEnable(GL_TEXTURE_2D);
}

inline void ENLIGHTING(bool state)
{
	if (state == false) glDisable(GL_LIGHTING); else glEnable(GL_LIGHTING);
}

//If enabled, blend the computed fragment color values with the values in the color buffers.See glBlendFunc.
inline void ENBLEND(bool state)
{
	if (state == false) glDisable(GL_BLEND); else glEnable(GL_BLEND);
}

//If enabled, do depth comparisons and update the depth buffer.Note that even if the depth buffer exists and the depth mask is non - zero, the depth buffer is not updated if the depth test is disabled.See glDepthFunc and glDepthRange.
inline void ENDEPTHTEST(bool state)
{
	if (state == false) glDisable(GL_DEPTH_TEST); else glEnable(GL_DEPTH_TEST);
}

//If enabled, draw lines with correct filtering. Otherwise, draw aliased lines. See glLineWidth. 
inline void ENLINESMOOTH(bool state)
{
	if (state == false) glDisable(GL_LINE_SMOOTH); else glEnable(GL_LINE_SMOOTH);
}

inline void ENNORMALIZE(bool state)
{
	if (state == false) glDisable(GL_NORMALIZE); else glEnable(GL_NORMALIZE);
}

inline void ENRESCALENORMAL(bool state)
{
	if (state == false) glDisable(GL_RESCALE_NORMAL); else glEnable(GL_RESCALE_NORMAL);
}

inline void ENALPHATEST(bool state)
{
	if (state == false) glDisable(GL_ALPHA_TEST); else glEnable(GL_ALPHA_TEST);
}

// If enabled, cull polygons based on their winding in window coordinates. See glCullFace. 
inline void ENCULLFACE(bool state)
{
	if (state == false) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
}

inline void ENCOLORMATERIAL(bool state)
{
	if (state == false) glDisable(GL_COLOR_MATERIAL);else glEnable(GL_COLOR_MATERIAL);
}

inline void ENFOG(bool state)
{
	if (state == false) glDisable(GL_FOG); else glEnable(GL_FOG);
}

//If enabled, dither color components or indices before they are written to the color buffer. 
inline void ENDITHER(bool state)
{
	if (state == false) glDisable(GL_DITHER);  else glEnable(GL_DITHER);
}

//If enabled, use multiple fragment samples in computing the final color of a pixel. See glSampleCoverage. 
inline void ENMULTISAMPLE(bool state)
{
	if (state == false) glDisable(GL_MULTISAMPLE); else glEnable(GL_MULTISAMPLE);
}

//If enabled, and if the polygon is rendered in GL_FILL mode, an offset is added to depth values of a polygon's fragments before the depth comparison is performed. See glPolygonOffset. 
inline void ENPOLYGONOFFSETFILL(bool state)
{
	if (state == false) glDisable(GL_POLYGON_OFFSET_FILL); else glEnable(GL_POLYGON_OFFSET_FILL);
}

//If enabled, and if the polygon is rendered in GL_LINE mode, an offset is added to depth values of a polygon's fragments before the depth comparison is performed. See glPolygonOffset. 
inline void ENPOLYGONOFFSETLINE(bool state)
{
	if (state == false) glDisable(GL_POLYGON_OFFSET_LINE); else glEnable(GL_POLYGON_OFFSET_LINE);
}

//If enabled, an offset is added to depth values of a polygon's fragments before the depth comparison is performed, if the polygon is rendered in GL_POINT mode. See glPolygonOffset. 
inline void ENPOLYGONOFFSETPOINT(bool state)
{
	if (state == false) glDisable(GL_POLYGON_OFFSET_POINT); else glEnable(GL_POLYGON_OFFSET_POINT);
}

//If enabled, draw polygons with proper filtering. Otherwise, draw aliased polygons. For correct antialiased polygons, an alpha buffer is needed and the polygons must be sorted front to back. 
inline void ENPOLYGONSMOOTH(bool state)
{
	if (state == false) glDisable(GL_POLYGON_SMOOTH); else glEnable(GL_POLYGON_SMOOTH);
}

//If enabled, compute a temporary coverage value where each bit is determined by the alpha value at the corresponding sample location. The temporary coverage value is then ANDed with the fragment coverage value. 
inline void ENSAMPLEALPHATOCOVERAGE(bool state)
{
	if (state == false) glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE); else glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
}

//If enabled, each sample alpha value is replaced by the maximum representable alpha value.
inline void ENSAMPLEALPHATOONE(bool state)
{
	if (state == false) glDisable(GL_SAMPLE_ALPHA_TO_ONE); else glEnable(GL_SAMPLE_ALPHA_TO_ONE);
}

//If enabled, the fragment's coverage is ANDed with the temporary coverage value. If GL_SAMPLE_COVERAGE_INVERT is set to GL_TRUE, invert the coverage value. See glSampleCoverage. 
inline void ENSAMPLECOVERAGE(bool state)
{
	if (state == false) glDisable(GL_SAMPLE_COVERAGE); else glEnable(GL_SAMPLE_COVERAGE);
}

//If enabled and the value of GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING for the framebuffer attachment corresponding to 
//the destination buffer is GL_SRGB, the R, G, and B destination color values (after conversion from fixed-point to 
//floating-point) are considered  to be encoded for the sRGB color space and hence are linearized prior to their use in blending. 
inline void ENFRAMEBUFFERSRGB(bool state)
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
