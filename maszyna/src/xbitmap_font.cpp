#include "commons.h"
#include "commons_usr.h"


GLuint	texture[2];
GLuint	base;				// Base Display List For The Font
GLuint	loop;				// Generic Loop Variable

GLfloat	cnt1;				// 1st Counter Used To Move Text & For Coloring
GLfloat	cnt2;				// 2nd Counter Used To Move Text & For Coloring

int xscreenw = 0; //Global::iWindowWidth;
int xscreenh = 0; //Global::iWindowHeight;


GLvoid QBuildFontX(GLvoid)								// Build Our Font Display List
{
	//char sscreenw[100];
    //char sscreenh[100];
    char sbuffer[200];
	//std::string sw = ToString(Global::iWindowWidth);

    //_itoa(Global::iWindowWidth, sscreenw, 10);
    //_itoa(Global::iWindowHeight, sscreenh, 10);

	sprintf_s( sbuffer, "screen size: %ix%i", 1280, 1024); 

    xscreenw = 1280;
    xscreenh = 1024;

	//WriteLog("BUILD FONT");
	//WriteLog(sbuffer);

	float	cx;											// Holds Our X Character Coord
	float	cy;											// Holds Our Y Character Coord

	
	base=glGenLists(256);								// Creating 256 Display Lists
	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);			// Select Our Font Texture
	for (loop=0; loop<256; loop++)						// Loop Through All 256 Lists
	{
		cx=float(loop%16)/16.0f;						// X Position Of Current Character
		cy=float(loop/16)/16.0f;						// Y Position Of Current Character

		glNewList(base+loop,GL_COMPILE);				// Start Building A List
			glBegin(GL_QUADS);							// Use A Quad For Each Character
				glTexCoord2f(cx,1.0f-cy-0.0625f);		// Texture Coord (Bottom Left)
				glVertex2i(0,0);						// Vertex Coord (Bottom Left)
				glTexCoord2f(cx+0.0625f,1.0f-cy-0.0625f);	// Texture Coord (Bottom Right)
				glVertex2i(16,0);						// Vertex Coord (Bottom Right)
				glTexCoord2f(cx+0.0625f,1.0f-cy);		// Texture Coord (Top Right)
				glVertex2i(16,16);						// Vertex Coord (Top Right)
				glTexCoord2f(cx,1.0f-cy);				// Texture Coord (Top Left)
				glVertex2i(0,16);						// Vertex Coord (Top Left)
			glEnd();									// Done Building Our Quad (Character)
			glTranslatef(11,0,0);	//10,0,0			// Move To The Right Of The Character
		glEndList();									// Done Building The Display List
	}													// Loop Until All 256 Are Built
}

GLvoid QKillFont(GLvoid)									// Delete The Font From Memory
{
	glDeleteLists(base,256);							// Delete All 256 Display Lists
}

GLvoid QglPrint(GLint x, GLint y, char *string, int set, TColor rgba)	// Where The Printing Happens
{
	x = x * 1;
	y = xscreenh - (y + 1) * 1;
	if (set>1)set=1;
	glBlendFunc(GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_COLOR);	// nastavenie miešania farieb
//farba bodu = (pôvodná farba * ALPHA)	+	(kreslená farba - pôvodná farba)
	glEnable(GL_BLEND);									// povolenie miešania farieb

	glColor4f(rgba.r, rgba.g, rgba.b, rgba.a);
	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);			// Select Our Font Texture
	glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPushMatrix();										// Store The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	glOrtho(0,xscreenw,0,xscreenh,-1,1);					// Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPushMatrix();										// Store The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x, (float)y, 0);								// Position The Text (0,0 - Bottom Left)
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists((int)strlen(string), GL_BYTE, string);			// Write The Text To The Screen
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing

	glDisable(GL_BLEND);								//vypnutie miešania farieb

}

GLvoid QglPrint_(GLint x, GLint y, char *string, int set, TColor rgba)	// Where The Printing Happens
{
	x=x*11;
	y=xscreenh-(y+1)*16;
	
	if (set>1)set=1;
	glBlendFunc(GL_SRC_ALPHA,	GL_ONE_MINUS_SRC_COLOR);	// nastavenie miešania farieb
 // glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//farba bodu = (pôvodná farba * ALPHA)	+	(kreslená farba - pôvodná farba)
	glEnable(GL_BLEND);			
	// povolenie miešania farieb
	glColor4f(rgba.r, rgba.g, rgba.b, rgba.a);
	glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);			// Select Our Font Texture
	glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPushMatrix();										// Store The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	glOrtho(0,xscreenw,0,xscreenh,-1,1);					// Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPushMatrix();										// Store The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x,(float)y,0);					// Position The Text (0,0 - Bottom Left)
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists((int)strlen(string), GL_BYTE, string);			// Write The Text To The Screen
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing

	glDisable(GL_BLEND);								//vypnutie miešania farieb

}








GLvoid QglPrintFast(GLint x, GLint y, char *string, int set, TColor rgba)	// Where The Printing Happens
{
	if (set>1)set=1;
	glColor4f(1.0f,0.8f,0.0f,0.8f);
glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);				// Select Our Font Texture
	glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
//	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
//	glPushMatrix();										// Store The Projection Matrix
//	glLoadIdentity();									// Reset The Projection Matrix
//	glOrtho(0,SCREEN_X,0,SCREEN_Y,-1,1);				// Set Up An Ortho Screen
//	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
//	glPushMatrix();										// Store The Modelview Matrix
//	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x,(float)y,0);					// Position The Text (0,0 - Bottom Left)
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists((int)strlen(string), GL_BYTE, string);			// Write The Text To The Screen
//	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
//	glPopMatrix();										// Restore The Old Projection Matrix
//	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
//	glPopMatrix();										// Restore The Old Projection Matrix
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
}

#define M_PI				3.1415926535f
GLvoid QglPrint3D(float x, float y, float z, float s, char *string, int set, TColor rgba)	// Where The Printing Happens
{
	if (set>1)set=1;

 

 //glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
 glEnable(GL_DEPTH_TEST);
 glDisable(GL_LIGHTING);
 glDisable(GL_FOG);
 glEnable(GL_BLEND);	
 glBlendFunc(GL_SRC_ALPHA,GL_ONE);
 glColor4f(1.0f,0.8f,0.0f,0.99f);
 glPushMatrix();
 glLineWidth(2);
 		glBegin(GL_LINES);
			glVertex3f(x, y, z);
			glVertex3f(x, y-y, z); 
		glEnd();
 glPopMatrix();

 glColor4f(1.0f,0.8f,0.0f,0.8f);
 glBindTexture(GL_TEXTURE_2D, Global::fonttexturex);				// Select Our Font Texture
 //glDisable(GL_DEPTH_TEST);	
 glPushMatrix();
 glTranslatef(x,y,z);					// Position The Text (0,0 - Bottom Left)

//matrix4x4 mat; //potrzebujemy wspó³rzêdne przesuniêcia œrodka uk³adu wspó³rzêdnych submodelu
//    glGetDoublev(GL_MODELVIEW_MATRIX,mat.getArray()); //pobranie aktualnej matrycy
//    vector3 gdzie=vector3(mat[3][0],mat[3][1],mat[3][2]); //pocz¹tek uk³adu wspó³rzêdnych submodelu wzglêdem kamery
//    glRotated(-atan2(gdzie.x,gdzie.z)*180.0/M_PI, 0.0, 1.0, 0.0); //jedynie obracamy w pionie o k¹t
  

 glScalef(s, s, s);
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists((int)strlen(string), GL_BYTE, string);			// Write The Text To The Screen
 glPopMatrix();
 
 glPushMatrix();
 glTranslatef(x,y,z);					// Position The Text (0,0 - Bottom Left)
 glScalef(s, s, s);
 glRotatef(180,0,1,0);
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists((int)strlen(string), GL_BYTE, string);			// Write The Text To The Screen
 glPopMatrix();
 
 glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
 glEnable(GL_LIGHTING);
 glDisable(GL_BLEND);
 glEnable(GL_FOG);
 
}