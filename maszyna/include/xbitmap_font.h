#include "../commons.h"
#include "../commons_usr.h"
#include "globals.h"

GLvoid QBuildFontX(GLvoid);
GLvoid QKillFont(GLvoid);
GLvoid QglPrint(GLint, GLint, char *, int set, TColor rgba); //x,y,*string,set
GLvoid QglPrint_(GLint, GLint, char *, int set, TColor rgba); //x-stlpec,y-riadok,*string,set
GLvoid QglPrintFast(GLint, GLint, char *, int set, TColor rgba);
GLvoid QglPrint3D(float x, float y, float z, float s, char *, int set, TColor rgba);