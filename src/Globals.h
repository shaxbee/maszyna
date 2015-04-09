//NOWA MASZYNA----------------------------------------------------------------------

#ifndef _Globals_H_
#define _Globals_H_

#include "commons.h"
#include "dumb3d.h"
//#include "mathlib.h"

bool FreeFlyModeFlag = false;
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
#define szTexturePath "textures\\"
//#define szDefaultTextureExt ".dds"

#define PI 3.1415926535897f
#define DTOR (PI/180.0f)
#define SQR(x) (x*x)

typedef struct {
	float x, y, z;
	unsigned int color;
	float u, v;
} VERTEX;

typedef struct {
	GLboolean blendEnabled;
	GLint blendSrc;
	GLint blendDst;
} GLblendstate;

extern VERTEX *Vertices;
extern int NumVertices;

using namespace Math3D;

// DEFINICJE ELEMENTOW KABINY

const int ce_ggMainCtrlAct = 10001;
const int ce_ggMainCtrl = 10002;
const int ce_ggScndCtrl = 10003;
const int ce_ggDirKey = 10004;
const int ce_ggBrakeCtrl = 10005;
const int ce_ggLocalBrake = 10006;
const int ce_ggManualBrake = 10007;
const int ce_ggBrakeProfileCtrl = 10008;
const int ce_ggBrakeProfileG = 10009;
const int ce_ggBrakeProfileR = 10010;
const int ce_ggMaxCurrentCtrl = 10011;
const int ce_ggMainOffButton = 10012;
const int ce_ggMainOnButton = 10013;
const int ce_ggSecurityResetButton = 10014;
const int ce_ggReleaserButton = 10015;
const int ce_ggAntiSlipButton = 10016;
const int ce_ggHornButton = 10017;
const int ce_ggFuseButton = 10018;
const int ce_ggConverterFuseButton = 10019;
const int ce_ggStLinOffButton = 10020;
const int ce_ggDoorLeftButton = 10021;
const int ce_ggDoorRightButton = 10022;
const int ce_ggDepartureSignalButton = 10023;
const int ce_ggUpperLightButton = 10024;
const int ce_ggLeftLightButton = 10025;
const int ce_ggRightLightButton = 10026;
const int ce_ggLeftEndLightButton = 10027;
const int ce_ggRightEndLightButton = 10028;
const int ce_ggRearUpperLightButton = 10029;
const int ce_ggRearLeftLightButton = 10030;
const int ce_ggRearRightLightButton = 10031;
const int ce_ggRearLeftEndLightButton = 10032;
const int ce_ggRearRightEndLightButton = 10033;
const int ce_ggCompressorButton = 10034;
const int ce_ggConverterButton = 10035;
const int ce_ggConverterOffButton = 10036;
const int ce_ggMainButton = 10037;
const int ce_ggRadioButton = 10038;
const int ce_ggPantFrontButton = 10039;
const int ce_ggPantRearButton = 10040;
const int ce_ggPantFrontButtonOff = 10041;
const int ce_ggPantAllDownButton = 10042;
const int ce_ggTrainHeatingButton = 10043;
const int ce_ggSignallingButton = 10044;
const int ce_ggDoorSignallingButton = 10045;
const int ce_ggNextCurrentButton = 10046;
const int ce_ggCabLightButton = 10047;
const int ce_ggCabLightDimButton = 10048;
const int ce_ggMainDistributorButton = 10049;
const int ce_ggBatteryButton = 10050;
const int ce_ggLZSButton = 10051;
const int ce_ggCupboard1Button = 10052;
const int ce_ggCupboard2Button = 10053;
const int ce_ggAntiSunRButton = 10054;
const int ce_ggAntiSunLButton = 10055;
const int ce_ggAntiSunCButton = 10056;
const int ce_ggCookerFlapButton = 10057;
const int ce_ggAckermansFlapButton = 10058;
const int ce_ggDoorPMButton = 10059;
const int ce_ggDoorWNButton = 10060;
const int ce_ggDoorCabLButton = 10061;
const int ce_ggDoorCabRButton = 10062;
const int ce_ggArmChairLButton = 10063;
const int ce_ggArmChairRButton = 10064;
const int ce_ggCabWinLButton = 10065;
const int ce_ggCabWinRButton = 10066;
const int ce_ggAshTray1Button = 10067;
const int ce_ggAshTray2Button = 10068;
const int ce_ggCupboard3LButton = 10069;
const int ce_ggCupboard3RButton = 10070;
const int ce_ggHandBrakeButton = 10071;
const int ce_ggHaslerBoltButton = 10072;
const int ce_ggHaslerHullButton = 10073;
const int ce_ggHandBrakeIndButton = 10074;
const int ce_ggRadioAlarmButton = 10075;
const int ce_ggFootSandButton = 10076;
const int ce_ggPantoAirSupplyButton = 10077;
const int ce_ggDoorWNBoltButton = 10078;
const int ce_ggHangerButton = 10079;
const int ce_ggVechDriveTypeButton = 10083;
const int ce_ggCabHeatingButton = 10084;
const int ce_ggHeadLightRDimButton = 10085;
const int ce_ggHeadLightADimButton = 10086;
const int ce_ggSHPDimButton = 10087;
const int ce_ggResistanceFanButton = 10088;
const int ce_ggCookerButton = 10089;
const int ce_ggWiperLButton = 10090;
const int ce_ggWiperRButton = 10091;
const int ce_ggAdjAxlePressureFButton = 10092;

const int ce_btLampkaPoslizg = 20001;
const int ce_btLampkaStyczn = 20002;
const int ce_btLampkaNadmPrzetw = 20003;
const int ce_btLampkaPrzetw = 20004;
const int ce_btLampkaPrzekRozn = 20005;
const int ce_btLampkaPrzekRoznPom = 20006;
const int ce_btLampkaNadmSil = 20007;
const int ce_btLampkaWylSzybki = 20008;
const int ce_btLampkaNadmWent = 20009;
const int ce_btLampkaNadmSpr = 20010;
const int ce_btLampkaOporyB = 20011;
const int ce_btLampkaStycznB = 20012;
const int ce_btLampkaWylSzybkiB = 20013;
const int ce_btLampkaNadmPrzetwB = 20014;
const int ce_btLampkaBezoporowaB = 20015;
const int ce_btLampkaBezoporowa = 20016;
const int ce_btLampkaUkrotnienie = 20017;
const int ce_btLampkaHamPosp = 20018;
const int ce_btLampkaRadio = 20019;
const int ce_btLampkaHamowanie1zes = 20020;
const int ce_btLampkaHamowanie2zes = 20021;
const int ce_btLampkaOpory = 20022;
const int ce_btLampkaWysRozrs = 20023;
const int ce_btLampkaUniversal3 = 20024;
const int ce_btLampkaWentZaluzje = 20025;
const int ce_btLampkaOgrzewanieSkladu = 20026;
const int ce_btLampkaSHP = 20027;
const int ce_btLampkaCzuwaka = 20028;
const int ce_btLampkaRezerwa = 20029;
const int ce_btLampkaNapNastHam = 20030;
const int ce_btLampkaSprezarka = 20031;
const int ce_btLampkaSprezarkaB = 20032;
const int ce_btLampkaBocznik1 = 20033;
const int ce_btLampkaBocznik2 = 20034;
const int ce_btLampkaBocznik3 = 20035;
const int ce_btLampkaBocznik4 = 20036;
const int ce_btLampkaRadiotelefon = 20037;
const int ce_btLampkaHamienie = 20038;
const int ce_btLampkaJazda = 20039;
const int ce_btLampkaBoczniki = 20040;
const int ce_btLampkaMaxSila = 20041;
const int ce_btLampkaPrzekrMaxSila = 20042;
const int ce_btLampkaDoorLeft = 20043;
const int ce_btLampkaDoorRight = 20044;
const int ce_btLampkaDepartureSignal = 20045;
const int ce_btLampkaBlokadaDrzwi = 20046;
const int ce_btLampkaHamulecReczny = 20047;
const int ce_btLampkaForward = 20048;
const int ce_btLampkaBackward = 20049;
const int ce_btCabLight = 20050;
const int ce_btHaslerBrakes = 20051;
const int ce_btHaslerCurrent = 20052;

const int ce_iiRadioTelefon = 30001;
const int ce_iiPlug = 30002;
const int ce_iiCASHPCase1 = 30003;
const int ce_iiCASHPCase2 = 30004;
const int ce_iiTable = 30005;
const int ce_iiHomologation = 30006;
const int ce_iiCASHPSignal = 30007;
const int ce_iiSpeedometer = 30008;
const int ce_iiMainDistributor = 30009;
const int ce_iiVMETERH = 30010;
const int ce_iiVMETERL = 30011;
const int ce_iiVMETER1 = 30012;
const int ce_iiAMETERL = 30013;
const int ce_iiAMETER1 = 30014;
const int ce_iiAMETER2 = 30015;
const int ce_iiAMETER3 = 30016;


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

class Global
{
public:
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

	static bool bFreeFly;
	static bool bFreeFlyModeFlag;
	//metody

	static void __fastcall LoadIniFile(std::string asFileName);
	static void __fastcall InitKeys(std::string asFileName);
	inline static vector3 __fastcall GetCameraPosition()
	{
		return pCameraPosition;
	};
	static void __fastcall SetCameraPosition(vector3 pNewCameraPosition);
	static void __fastcall SetCameraRotation(double Yaw);
};

//---------------------------------------------------------------------------
#endif
