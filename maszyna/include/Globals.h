#ifndef _Globals_H_
#define _Globals_H_


#include "ground.h"
#include "camerax.h"   
//#include "shaders.h"  
#define qPI 3.14159

#define SafeDelete(a) { delete (a); a=NULL; }
#define SafeDeleteArray(a) { delete[] (a); a=NULL; }

#define sign(x) ((x)<0?-1:((x)>0?1:0))

#define DegToRad(a) ((M_PI/180.0)*(a)) //(a) w nawiasie, bo mo¿e byæ dodawaniem
#define RadToDeg(r) ((180.0/M_PI)*(r))

#define Fix(a,b,c) {if (a<b) a=b; if (a>c) a=c;}

#define asModelsPath std::string("models\\")
#define asSceneryPath std::string("scenery\\")
//#define asTexturePath std::string("textures\\")
//#define asTextureExt std::string(".bmp")
#define szSceneryPath "scenery\\"
#define szDefaultTexturePath "textures\\"


#define PI 3.1415926535897f
#define DTOR (PI/180.0f)
#define SQR(x) (x*x)

typedef struct {
	float x, y, z;
	unsigned int color;
	float u, v;
} VERTEX;

typedef struct {
	float r, g, b, a;
} TColor;

typedef struct {
	GLboolean blendEnabled;
	GLint blendSrc;
	GLint blendDst;
} GLblendstate;

#define KEYBINDINGS 200
typedef struct {
	std::string keycommand;
	std::string keyaction;
	std::string key;
	std::string keymod;
} keyboardsets;

extern VERTEX *Vertices;
extern int NumVertices;

using namespace Math3D;

//definicje klawiszy
const int k_IncMainCtrl = 0; //[Num+]
const int k_IncMainCtrlFAST = 1; //[Num+] [Shift]
const int k_DecMainCtrl = 2; //[Num-]
const int k_DecMainCtrlFAST = 3; //[Num-] [Shift]
const int k_IncScndCtrl = 4; //[Num/]
const int k_IncScndCtrlFAST = 5;
const int k_DecScndCtrl = 6;
const int k_DecScndCtrlFAST = 7;
const int k_IncLocalBrakeLevel = 8;
const int k_IncLocalBrakeLevelFAST = 9;
const int k_DecLocalBrakeLevel = 10;
const int k_DecLocalBrakeLevelFAST = 11;
const int k_IncBrakeLevel = 12;
const int k_DecBrakeLevel = 13;
const int k_Releaser = 14;
const int k_EmergencyBrake = 15;
const int k_Brake3 = 16;
const int k_Brake2 = 17;
const int k_Brake1 = 18;
const int k_Brake0 = 19;
const int k_WaveBrake = 20;
const int k_AntiSlipping = 21;
const int k_Sand = 22;

const int k_Main = 23;
const int k_DirectionForward = 24;
const int k_DirectionBackward = 25;

const int k_Fuse = 26;
const int k_Compressor = 27;
const int k_Converter = 28;
const int k_MaxCurrent = 29;
const int k_CurrentAutoRelay = 30;
const int k_BrakeProfile = 31;

const int k_Czuwak = 32;
const int k_Horn = 33;
const int k_Horn2 = 34;

const int k_FailedEngineCutOff = 35;

const int k_MechUp = 36;
const int k_MechDown = 37;
const int k_MechLeft = 38;
const int k_MechRight = 39;
const int k_MechForward = 40;
const int k_MechBackward = 41;

const int k_CabForward = 42;
const int k_CabBackward = 43;

const int k_Couple = 44;
const int k_DeCouple = 45;

const int k_ProgramQuit = 46;
//const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
//NBMX
const int k_OpenLeft = 49;
const int k_OpenRight = 50;
const int k_CloseLeft = 51;
const int k_CloseRight = 52;
const int k_DepartureSignal = 53;
//NBMX
const int k_PantFrontUp = 54;
const int k_PantRearUp = 55;
const int k_PantFrontDown = 56;
const int k_PantRearDown = 57;

const int k_Heating = 58;

//const int k_FreeFlyMode= 59;

const int k_LeftSign = 60;
const int k_UpperSign = 61;
const int k_RightSign = 62;

const int k_SmallCompressor = 63;

const int k_StLinOff = 64;

const int k_CurrentNext = 65;

const int k_Univ1 = 66;
const int k_Univ2 = 67;
const int k_Univ3 = 68;
const int k_Univ4 = 69;
const int k_EndSign = 70;

const int k_Active = 71;
//Winger 020304
const int k_Battery = 72;
const int k_WalkMode = 73;
const int k_MainDistributor = 74;
const int k_LZS = 75;
const int k_Cupboard1 = 76;
const int k_Cupboard2 = 77;
const int MaxKeys = 78;

static bool FileExists(const std::string & fileName)
{
	struct stat info;
	int ret = -1;

	ret = stat(fileName.c_str(), &info);
	return 0 == ret;
}

double __fastcall Min0R(double x1, double x2);
double __fastcall Max0R(double x1, double x2);
bool __fastcall TestFlag(int Flag, int Value);
bool __fastcall SetFlag(int Flag, int Value);
int Sign(int v);

class Global
{
public:
	static HWND hWnd; //uchwyt okna 
	static GLFWwindow* window;
	static char **argv;
	static TGround *pGround;

	// Camera
	static glm::vec3 cameraPos;
	static glm::vec3 cameraFront;
	static glm::vec3 cameraUp;
	static GLfloat yaw;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
	static GLfloat pitch;
	static GLfloat lastX;
	static GLfloat lastY;
	static bool keys[1024];
	static CCamera CAMERA;
	static float frameTime;
	static float previousFrameTime; // Holds the amount of milliseconds since the last frame
	static float timeAccumulator; // Holds a sum of the time from all passed frame times
	static float fpsMeasureInterval; // The interval where we would like to take an FPS sample. Currently simply each second.
	static int frameCount; // The current amount of frames which have passed

	static vector3 Camerapos;
	static int Keys[MaxKeys];
	static vector3 pCameraPosition; //pozycja kamery w œwiecie
	static double pCameraRotation;  //kierunek bezwzglêdny kamery w œwiecie: 0=pó³noc, 90°=zachód (-azymut)
	static double pCameraRotationDeg;  //w stopniach, dla animacji billboard
	static vector3 pFreeCameraInit[10]; //pozycje kamery
	static vector3 pFreeCameraInitAngle[10];
	static int iWindowWidth;
	static int iWindowHeight;
	static GLuint fonttexturex;
	static GLfloat  AtmoColor[];
	static GLfloat  FogColor[];
	static bool bTimeChange;
	static GLfloat  ambientDayLight[];
	static GLfloat  diffuseDayLight[];
	static GLfloat  specularDayLight[];
	static GLfloat  whiteLight[];
	static GLfloat  noLight[];
	static GLfloat  lightPos[4];
	static GLfloat darkLight[];
	static float Global::fMouseXScale;
	static float Global::fMouseYScale;
	static float fTimeSpeed;
	static double FPS;
	static float fdt;
	static float fms;
	static float FOV;
	static float tracksegmentlen;

	static bool detonatoryOK;
	static bool bFreeFly;
	static bool bFreeFlyModeFlag;
	static bool bWriteLogEnabled;
	static bool bActive;
	static bool bInactivePause;
	static bool bGlutFont;
	static bool bSCNLOADED;
	static bool bfirstloadingscn;
	static bool bSoundEnabled;
	static bool bManageNodes;
	static bool bWireFrame;
	static bool bOpenGL_1_5;
	static bool bRenderAlpha;
	static int iReCompile; //zwiêkszany, gdy trzeba odœwie¿yæ siatki
	static bool bUseVBO; //czy jest VBO w karcie graficznej
	static int iPARSERBYTESPASSED;
	static int iNODES;
	static int postep;
	static int iPause;
	static int iTextMode;
	static int aspectratio;
	static float fOpenGL;
	static int keyid;
	static int keyaction;
	static int keyscancode;
	static int keymods;
	static int keybindsnum;
	static int iRailProFiltering;
	static int iBallastFiltering;
	static int iWriteLogEnabled;
	static int iMultiplayer;
	static std::string KEYCODE;
	static std::string KEYCOMMAND;
	static std::string asCWD;
	static std::string logfilenm1;
	static std::string szDefaultExt;
	static std::string asCurrentDynamicPath;
	static std::string asCurrentSceneryPath;
	static std::string asLang;

	static GLuint boxtex;
	static GLuint balltex;
	static GLuint dirttex;
	static GLuint logotex;
	static GLuint bfonttex;
	static GLuint loaderbackg;
	static GLuint fbase;
	static TColor color;

	static keyboardsets KBD[KEYBINDINGS];
	//metody

	static void LoadIniFile(std::string asFileName);
	static void InitKeys(std::string asFileName);
	inline static vector3 GetCameraPosition() {	return pCameraPosition;};
	static void SetCameraPosition(vector3 pNewCameraPosition);
	static void SetCameraRotation(double Yaw);
	static TColor SetColor(float r, float g, float b, float a);
	static std::string GETCWD();
	static GLvoid glPrintxy(GLint x, GLint y, char *string, int set);
	static GLvoid BuildFont(GLvoid);
	static GLvoid KillFont(GLvoid);
	static double CALCULATEFPS();
};

//---------------------------------------------------------------------------
#endif
