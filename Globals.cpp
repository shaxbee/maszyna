//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "system.hpp"
#pragma hdrstop

#include "Globals.h"
#include "QueryParserComp.hpp"
#include "usefull.h"
#include "Mover.h"
#include "Driver.h"
#include "Console.h"
#include <Controls.hpp> //do odczytu daty
#include "World.h"

// namespace Global {

// parametry do użytku wewnętrznego
// double Global::tSinceStart=0;
TGround *Global::pGround = NULL;
// char Global::CreatorName1[30]="2001-2004 Maciej Czapkiewicz <McZapkie>";
// char Global::CreatorName2[30]="2001-2003 Marcin Woźniak <Marcin_EU>";
// char Global::CreatorName3[20]="2004-2005 Adam Bugiel <ABu>";
// char Global::CreatorName4[30]="2004 Arkadiusz Ślusarczyk <Winger>";
// char Global::CreatorName5[30]="2003-2009 Łukasz Kirchner <Nbmx>";
AnsiString Global::asCurrentSceneryPath = "scenery/";
AnsiString Global::asCurrentTexturePath = AnsiString(szTexturePath);
AnsiString Global::asCurrentDynamicPath = "";
int Global::iSlowMotion =
    0; // info o malym FPS: 0-OK, 1-wyłączyć multisampling, 3-promień 1.5km, 7-1km
TDynamicObject *Global::changeDynObj = NULL; // info o zmianie pojazdu
bool Global::detonatoryOK; // info o nowych detonatorach
double Global::ABuDebug = 0;
AnsiString Global::asSky = "1";
double Global::fOpenGL = 0.0; // wersja OpenGL - do sprawdzania obecności rozszerzeń
bool Global::bOpenGL_1_5 = false; // czy są dostępne funkcje OpenGL 1.5
double Global::fLuminance = 1.0; // jasność światła do automatycznego zapalania
int Global::iReCompile = 0; // zwiększany, gdy trzeba odświeżyć siatki
HWND Global::hWnd = NULL; // uchwyt okna
int Global::iCameraLast = -1;
AnsiString Global::asRelease = "15.3.1166.469";
AnsiString Global::asVersion =
    "Compilation 2015-03-25, release " + Global::asRelease + "."; // tutaj, bo wysyłany
int Global::iViewMode = 0; // co aktualnie widać: 0-kabina, 1-latanie, 2-sprzęgi, 3-dokumenty
int Global::iTextMode = 0; // tryb pracy wyświetlacza tekstowego
int Global::iScreenMode[12] = {0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0}; // numer ekranu wyświetlacza tekstowego
double Global::fSunDeclination = 0.0; // deklinacja Słońca
double Global::fTimeAngleDeg = 0.0; // godzina w postaci kąta
float Global::fClockAngleDeg[6]; // kąty obrotu cylindrów dla zegara cyfrowego
char *Global::szTexturesTGA[4] = {"tga", "dds", "tex", "bmp"}; // lista tekstur od TGA
char *Global::szTexturesDDS[4] = {"dds", "tga", "tex", "bmp"}; // lista tekstur od DDS
int Global::iKeyLast = 0; // ostatnio naciśnięty klawisz w celu logowania
GLuint Global::iTextureId = 0; // ostatnio użyta tekstura 2D
int Global::iPause = 0x10; // globalna pauza ruchu
bool Global::bActive = true; // czy jest aktywnym oknem
int Global::iErorrCounter = 0; // licznik sprawdzań do śledzenia błędów OpenGL
int Global::iTextures = 0; // licznik użytych tekstur
TWorld *Global::pWorld = NULL;
Queryparsercomp::TQueryParserComp *Global::qParser = NULL;
cParser *Global::pParser = NULL;
int Global::iSegmentsRendered = 90; // ilość segmentów do regulacji wydajności
TCamera *Global::pCamera = NULL; // parametry kamery
TDynamicObject *Global::pUserDynamic = NULL; // pojazd użytkownika, renderowany bez trzęsienia
bool Global::bSmudge = false; // czy wyświetlać smugę, a pojazd użytkownika na końcu
AnsiString Global::asTranscript[5]; // napisy na ekranie (widoczne)
TTranscripts Global::tranTexts; // obiekt obsługujący stenogramy dźwięków na ekranie

// parametry scenerii
vector3 Global::pCameraPosition;
double Global::pCameraRotation;
double Global::pCameraRotationDeg;
vector3 Global::pFreeCameraInit[10];
vector3 Global::pFreeCameraInitAngle[10];
double Global::fFogStart = 1700;
double Global::fFogEnd = 2000;
GLfloat Global::AtmoColor[] = {0.423f, 0.702f, 1.0f};
GLfloat Global::FogColor[] = {0.6f, 0.7f, 0.8f};
GLfloat Global::ambientDayLight[] = {0.40f, 0.40f, 0.45f, 1.0f}; // robocze
GLfloat Global::diffuseDayLight[] = {0.55f, 0.54f, 0.50f, 1.0f};
GLfloat Global::specularDayLight[] = {0.95f, 0.94f, 0.90f, 1.0f};
GLfloat Global::ambientLight[] = {0.80f, 0.80f, 0.85f, 1.0f}; // stałe
GLfloat Global::diffuseLight[] = {0.85f, 0.85f, 0.80f, 1.0f};
GLfloat Global::specularLight[] = {0.95f, 0.94f, 0.90f, 1.0f};
GLfloat Global::whiteLight[] = {1.00f, 1.00f, 1.00f, 1.0f};
GLfloat Global::noLight[] = {0.00f, 0.00f, 0.00f, 1.0f};
GLfloat Global::darkLight[] = {0.03f, 0.03f, 0.03f, 1.0f}; //śladowe
GLfloat Global::lightPos[4];
bool Global::bRollFix = true; // czy wykonać przeliczanie przechyłki
bool Global::bJoinEvents = false; // czy grupować eventy o tych samych nazwach
int Global::iHiddenEvents = 0; // czy łączyć eventy z torami poprzez nazwę toru

// parametry użytkowe (jak komu pasuje)
int Global::Keys[MaxKeys];
int Global::iWindowWidth = 800;
int Global::iWindowHeight = 600;
float Global::fDistanceFactor = 768.0; // baza do przeliczania odległości dla LoD
int Global::iFeedbackMode = 1; // tryb pracy informacji zwrotnej
int Global::iFeedbackPort = 0; // dodatkowy adres dla informacji zwrotnych
bool Global::bFreeFly = false;
bool Global::bFullScreen = false;
bool Global::bInactivePause = true; // automatyczna pauza, gdy okno nieaktywne
float Global::fMouseXScale = 1.5;
float Global::fMouseYScale = 0.2;
char Global::szSceneryFile[256] = "td.scn";
AnsiString Global::asHumanCtrlVehicle = "EU07-424";
int Global::iMultiplayer = 0; // blokada działania niektórych funkcji na rzecz komunikacji
double Global::fMoveLight = -1; // ruchome światło
double Global::fLatitudeDeg = 52.0; // szerokość geograficzna
float Global::fFriction = 1.0; // mnożnik tarcia - KURS90
double Global::fBrakeStep = 1.0; // krok zmiany hamulca dla klawiszy [Num3] i [Num9]
AnsiString Global::asLang = "pl"; // domyślny język - http://tools.ietf.org/html/bcp47

// parametry wydajnościowe (np. regulacja FPS, szybkość wczytywania)
bool Global::bAdjustScreenFreq = true;
bool Global::bEnableTraction = true;
bool Global::bLoadTraction = true;
bool Global::bLiveTraction = true;
int Global::iDefaultFiltering = 9; // domyślne rozmywanie tekstur TGA bez alfa
int Global::iBallastFiltering = 9; // domyślne rozmywanie tekstur podsypki
int Global::iRailProFiltering = 5; // domyślne rozmywanie tekstur szyn
int Global::iDynamicFiltering = 5; // domyślne rozmywanie tekstur pojazdów
bool Global::bUseVBO = false; // czy jest VBO w karcie graficznej (czy użyć)
GLint Global::iMaxTextureSize = 16384; // maksymalny rozmiar tekstury
bool Global::bSmoothTraction = false; // wygładzanie drutów starym sposobem
char **Global::szDefaultExt = Global::szTexturesDDS; // domyślnie od DDS
int Global::iMultisampling = 2; // tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
bool Global::bGlutFont = false; // czy tekst generowany przez GLUT32.DLL
int Global::iConvertModels = 7; // tworzenie plików binarnych, +2-optymalizacja transformów
int Global::iSlowMotionMask = -1; // maska wyłączanych właściwości dla zwiększenia FPS
int Global::iModifyTGA = 7; // czy korygować pliki TGA dla szybszego wczytywania
// bool Global::bTerrainCompact=true; //czy zapisać teren w pliku
TAnimModel *Global::pTerrainCompact = NULL; // do zapisania terenu w pliku
AnsiString Global::asTerrainModel = ""; // nazwa obiektu terenu do zapisania w pliku
double Global::fFpsAverage = 20.0; // oczekiwana wartosć FPS
double Global::fFpsDeviation = 5.0; // odchylenie standardowe FPS
double Global::fFpsMin = 0.0; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
double Global::fFpsMax = 0.0; // górna granica FPS, przy której promień scenerii będzie zwiększany
double Global::fFpsRadiusMax = 3000.0; // maksymalny promień renderowania
int Global::iFpsRadiusMax = 225; // maksymalny promień renderowania
double Global::fRadiusFactor = 1.1; // współczynnik jednorazowej zmiany promienia scenerii

// parametry testowe (do testowania scenerii i obiektów)
bool Global::bWireFrame = false;
bool Global::bSoundEnabled = true;
int Global::iWriteLogEnabled = 3; // maska bitowa: 1-zapis do pliku, 2-okienko, 4-nazwy torów
bool Global::bManageNodes = true;
bool Global::bDecompressDDS = false; // czy programowa dekompresja DDS

// parametry do kalibracji
// kolejno współczynniki dla potęg 0, 1, 2, 3 wartości odczytanej z urządzenia
double Global::fCalibrateIn[6][4] = {
    {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}};
double Global::fCalibrateOut[7][4] = {{0, 1, 0, 0},
                                      {0, 1, 0, 0},
                                      {0, 1, 0, 0},
                                      {0, 1, 0, 0},
                                      {0, 1, 0, 0},
                                      {0, 1, 0, 0},
                                      {0, 1, 0, 0}};

// parametry przejściowe (do usunięcia)
// bool Global::bTimeChange=false; //Ra: ZiomalCl wyłączył starą wersję nocy
// bool Global::bRenderAlpha=true; //Ra: wywaliłam tę flagę
bool Global::bnewAirCouplers = true;
bool Global::bDoubleAmbient = false; // podwójna jasność ambient
double Global::fTimeSpeed = 1.0; // przyspieszenie czasu, zmienna do testów
bool Global::bHideConsole = false; // hunter-271211: ukrywanie konsoli
int Global::iBpp = 32; // chyba już nie używa się kart, na których 16bpp coś poprawi

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

AnsiString Global::GetNextSymbol()
{ // pobranie tokenu z aktualnego parsera
    if (qParser)
        return qParser->EndOfFile ? AnsiString("endconfig") : qParser->GetNextSymbol();
    if (pParser)
    {
        std::string token;
        pParser->getTokens();
        *pParser >> token;
        return AnsiString(token.c_str());
    };
    return "";
};

void Global::LoadIniFile(AnsiString asFileName)
{
    int i;
    for (i = 0; i < 10; ++i)
    { // zerowanie pozycji kamer
        pFreeCameraInit[i] = vector3(0, 0, 0); // współrzędne w scenerii
        pFreeCameraInitAngle[i] = vector3(0, 0, 0); // kąty obrotu w radianach
    }
    TFileStream *fs;
    fs = new TFileStream(asFileName, fmOpenRead | fmShareCompat);
    if (!fs)
        return;
    AnsiString str = "";
    int size = fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(), size);
    // str+="";
    delete fs;
    TQueryParserComp *Parser;
    Parser = new TQueryParserComp(NULL);
    Parser->TextToParse = str;
    // Parser->LoadStringToParse(asFile);
    Parser->First();
    ConfigParse(Parser);
    delete Parser; // Ra: tego jak zwykle nie było wcześniej :]
};

void Global::ConfigParse(TQueryParserComp *qp, cParser *cp)
{ // Ra: trzeba by przerobić na cParser, żeby to działało w scenerii
    pParser = cp;
    qParser = qp;
    AnsiString str;
    int i;
    do
    {
        str = GetNextSymbol().LowerCase();
        if (str == AnsiString("sceneryfile"))
        {
            str = GetNextSymbol().LowerCase();
            strcpy(szSceneryFile, str.c_str());
        }
        else if (str == AnsiString("humanctrlvehicle"))
        {
            str = GetNextSymbol().LowerCase();
            asHumanCtrlVehicle = str;
        }
        else if (str == AnsiString("width"))
            iWindowWidth = GetNextSymbol().ToInt();
        else if (str == AnsiString("height"))
            iWindowHeight = GetNextSymbol().ToInt();
        else if (str == AnsiString("heightbase"))
            fDistanceFactor = GetNextSymbol().ToInt();
        else if (str == AnsiString("bpp"))
            iBpp = ((GetNextSymbol().LowerCase() == AnsiString("32")) ? 32 : 16);
        else if (str == AnsiString("fullscreen"))
            bFullScreen = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("freefly")) // Mczapkie-130302
        {
            bFreeFly = (GetNextSymbol().LowerCase() == AnsiString("yes"));
            pFreeCameraInit[0].x = GetNextSymbol().ToDouble();
            pFreeCameraInit[0].y = GetNextSymbol().ToDouble();
            pFreeCameraInit[0].z = GetNextSymbol().ToDouble();
        }
        else if (str == AnsiString("wireframe"))
            bWireFrame = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("debugmode")) // McZapkie! - DebugModeFlag uzywana w mover.pas,
                                                 // warto tez blokowac cheaty gdy false
            DebugModeFlag = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("soundenabled")) // McZapkie-040302 - blokada dzwieku - przyda
                                                    // sie do debugowania oraz na komp. bez karty
                                                    // dzw.
            bSoundEnabled = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        // else if (str==AnsiString("renderalpha")) //McZapkie-1312302 - dwuprzebiegowe renderowanie
        // bRenderAlpha=(GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str == AnsiString("physicslog")) // McZapkie-030402 - logowanie parametrow
                                                  // fizycznych dla kazdego pojazdu z maszynista
            WriteLogFlag = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("physicsdeactivation")) // McZapkie-291103 - usypianie fizyki
            PhysicActivationFlag = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("debuglog"))
        { // McZapkie-300402 - wylaczanie log.txt
            str = GetNextSymbol().LowerCase();
            if (str == "yes")
                iWriteLogEnabled = 3;
            else if (str == "no")
                iWriteLogEnabled = 0;
            else
                iWriteLogEnabled = str.ToIntDef(3);
        }
        else if (str == AnsiString("adjustscreenfreq"))
        { // McZapkie-240403 - czestotliwosc odswiezania ekranu
            str = GetNextSymbol();
            bAdjustScreenFreq = (str.LowerCase() == AnsiString("yes"));
        }
        else if (str == AnsiString("mousescale"))
        { // McZapkie-060503 - czulosc ruchu myszy (krecenia glowa)
            str = GetNextSymbol();
            fMouseXScale = str.ToDouble();
            str = GetNextSymbol();
            fMouseYScale = str.ToDouble();
        }
        else if (str == AnsiString("enabletraction"))
        { // Winger 040204 - 'zywe' patyki dostosowujace sie do trakcji; Ra 2014-03: teraz łamanie
            bEnableTraction = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        }
        else if (str == AnsiString("loadtraction"))
        { // Winger 140404 - ladowanie sie trakcji
            bLoadTraction = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        }
        else if (str == AnsiString("friction")) // mnożnik tarcia - KURS90
            fFriction = GetNextSymbol().ToDouble();
        else if (str == AnsiString("livetraction"))
        { // Winger 160404 - zaleznosc napiecia loka od trakcji; Ra 2014-03: teraz prąd przy braku
          // sieci
            bLiveTraction = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        }
        else if (str == AnsiString("skyenabled"))
        { // youBy - niebo
            if (GetNextSymbol().LowerCase() == AnsiString("yes"))
                asSky = "1";
            else
                asSky = "0";
        }
        else if (str == AnsiString("managenodes"))
        {
            bManageNodes = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        }
        else if (str == AnsiString("decompressdds"))
        {
            bDecompressDDS = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        }
        // ShaXbee - domyslne rozszerzenie tekstur
        else if (str == AnsiString("defaultext"))
        {
            str = GetNextSymbol().LowerCase(); // rozszerzenie
            if (str == "tga")
                szDefaultExt = szTexturesTGA; // domyślnie od TGA
            // szDefaultExt=std::string(Parser->GetNextSymbol().LowerCase().c_str());
        }
        else if (str == AnsiString("newaircouplers"))
            bnewAirCouplers = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("defaultfiltering"))
            iDefaultFiltering = GetNextSymbol().ToIntDef(-1);
        else if (str == AnsiString("ballastfiltering"))
            iBallastFiltering = GetNextSymbol().ToIntDef(-1);
        else if (str == AnsiString("railprofiltering"))
            iRailProFiltering = GetNextSymbol().ToIntDef(-1);
        else if (str == AnsiString("dynamicfiltering"))
            iDynamicFiltering = GetNextSymbol().ToIntDef(-1);
        else if (str == AnsiString("usevbo"))
            bUseVBO = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("feedbackmode"))
            iFeedbackMode = GetNextSymbol().ToIntDef(1); // domyślnie 1
        else if (str == AnsiString("feedbackport"))
            iFeedbackPort = GetNextSymbol().ToIntDef(0); // domyślnie 0
        else if (str == AnsiString("multiplayer"))
            iMultiplayer = GetNextSymbol().ToIntDef(0); // domyślnie 0
        else if (str == AnsiString("maxtexturesize"))
        { // wymuszenie przeskalowania tekstur
            i = GetNextSymbol().ToIntDef(16384); // domyślnie duże
            if (i <= 64)
                iMaxTextureSize = 64;
            else if (i <= 128)
                iMaxTextureSize = 128;
            else if (i <= 256)
                iMaxTextureSize = 256;
            else if (i <= 512)
                iMaxTextureSize = 512;
            else if (i <= 1024)
                iMaxTextureSize = 1024;
            else if (i <= 2048)
                iMaxTextureSize = 2048;
            else if (i <= 4096)
                iMaxTextureSize = 4096;
            else if (i <= 8192)
                iMaxTextureSize = 8192;
            else
                iMaxTextureSize = 16384;
        }
        else if (str == AnsiString("doubleambient")) // podwójna jasność ambient
            bDoubleAmbient = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("movelight")) // numer dnia w roku albo -1
        {
            fMoveLight = GetNextSymbol().ToIntDef(-1); // numer dnia 1..365
            if (fMoveLight == 0.0)
            { // pobranie daty z systemu
                unsigned short y, m, d;
                TDate date = Now();
                date.DecodeDate(&y, &m, &d);
                fMoveLight =
                    (double)date - (double)TDate(y, 1, 1) + 1; // numer bieżącego dnia w roku
            }
            if (fMoveLight > 0.0) // tu jest nadal zwiększone o 1
            { // obliczenie deklinacji wg:
                // http://naturalfrequency.com/Tregenza_Sharples/Daylight_Algorithms/algorithm_1_11.htm
                // Spencer J W Fourier series representation of the position of the sun Search 2 (5)
                // 172 (1971)
                fMoveLight = M_PI / 182.5 * (Global::fMoveLight - 1.0); // numer dnia w postaci kąta
                fSunDeclination = 0.006918 - 0.3999120 * cos(fMoveLight) +
                                  0.0702570 * sin(fMoveLight) - 0.0067580 * cos(2 * fMoveLight) +
                                  0.0009070 * sin(2 * fMoveLight) -
                                  0.0026970 * cos(3 * fMoveLight) + 0.0014800 * sin(3 * fMoveLight);
            }
        }
        else if (str == AnsiString("smoothtraction")) // podwójna jasność ambient
            bSmoothTraction = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("timespeed")) // przyspieszenie czasu, zmienna do testów
            fTimeSpeed = GetNextSymbol().ToIntDef(1);
        else if (str == AnsiString("multisampling")) // tryb antyaliasingu: 0=brak,1=2px,2=4px
            iMultisampling = GetNextSymbol().ToIntDef(2); // domyślnie 2
        else if (str == AnsiString("glutfont")) // tekst generowany przez GLUT
            bGlutFont = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("latitude")) // szerokość geograficzna
            fLatitudeDeg = GetNextSymbol().ToDouble();
        else if (str == AnsiString("convertmodels")) // tworzenie plików binarnych
            iConvertModels = GetNextSymbol().ToIntDef(7); // domyślnie 7
        else if (str == AnsiString("inactivepause")) // automatyczna pauza, gdy okno nieaktywne
            bInactivePause = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("slowmotion")) // tworzenie plików binarnych
            iSlowMotionMask = GetNextSymbol().ToIntDef(-1); // domyślnie -1
        else if (str == AnsiString("modifytga")) // czy korygować pliki TGA dla szybszego
                                                 // wczytywania
            iModifyTGA = GetNextSymbol().ToIntDef(0); // domyślnie 0
        else if (str == AnsiString("hideconsole")) // hunter-271211: ukrywanie konsoli
            bHideConsole = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str ==
                 AnsiString(
                     "rollfix")) // Ra: poprawianie przechyłki, aby wewnętrzna szyna była "pozioma"
            bRollFix = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("fpsaverage")) // oczekiwana wartosć FPS
            fFpsAverage = GetNextSymbol().ToDouble();
        else if (str == AnsiString("fpsdeviation")) // odchylenie standardowe FPS
            fFpsDeviation = GetNextSymbol().ToDouble();
        else if (str == AnsiString("fpsradiusmax")) // maksymalny promień renderowania
            fFpsRadiusMax = GetNextSymbol().ToDouble();
        else if (str == AnsiString("calibratein")) // parametry kalibracji wejść
        { //
            i = GetNextSymbol().ToIntDef(-1); // numer wejścia
            if ((i < 0) || (i > 5))
                i = 5; // na ostatni, bo i tak trzeba pominąć wartości
            fCalibrateIn[i][0] = GetNextSymbol().ToDouble(); // wyraz wolny
            fCalibrateIn[i][1] = GetNextSymbol().ToDouble(); // mnożnik
            fCalibrateIn[i][2] = GetNextSymbol().ToDouble(); // mnożnik dla kwadratu
            fCalibrateIn[i][3] = GetNextSymbol().ToDouble(); // mnożnik dla sześcianu
        }
        else if (str == AnsiString("calibrateout")) // parametry kalibracji wyjść
        { //
            i = GetNextSymbol().ToIntDef(-1); // numer wejścia
            if ((i < 0) || (i > 6))
                i = 6; // na ostatni, bo i tak trzeba pominąć wartości
            fCalibrateOut[i][0] = GetNextSymbol().ToDouble(); // wyraz wolny
            fCalibrateOut[i][1] = GetNextSymbol().ToDouble(); // mnożnik liniowy
            fCalibrateOut[i][2] = GetNextSymbol().ToDouble(); // mnożnik dla kwadratu
            fCalibrateOut[i][3] = GetNextSymbol().ToDouble(); // mnożnik dla sześcianu
        }
        else if (str == AnsiString("brakestep")) // krok zmiany hamulca dla klawiszy [Num3] i [Num9]
            fBrakeStep = GetNextSymbol().ToDouble();
        else if (str ==
                 AnsiString("joinduplicatedevents")) // czy grupować eventy o tych samych nazwach
            bJoinEvents = (GetNextSymbol().LowerCase() == AnsiString("yes"));
        else if (str == AnsiString("hiddenevents")) // czy łączyć eventy z torami poprzez nazwę toru
            iHiddenEvents = GetNextSymbol().ToIntDef(0);
        else if (str == AnsiString("pause")) // czy po wczytaniu ma być pauza?
            iPause |= (GetNextSymbol().LowerCase() == AnsiString("yes")) ? 1 : 0;
        else if (str == AnsiString("lang"))
            asLang = GetNextSymbol(); // domyślny język - http://tools.ietf.org/html/bcp47
        else if (str == AnsiString("opengl")) // deklarowana wersja OpenGL, żeby powstrzymać błędy
            fOpenGL = GetNextSymbol().ToDouble(); // wymuszenie wersji OpenGL
    } while (str != "endconfig"); //(!Parser->EndOfFile)
    // na koniec trochę zależności
    if (!bLoadTraction) // wczytywanie drutów i słupów
    { // tutaj wyłączenie, bo mogą nie być zdefiniowane w INI
        bEnableTraction = false; // false = pantograf się nie połamie
        bLiveTraction = false; // false = pantografy zawsze zbierają 95% MaxVoltage
    }
    // if (fMoveLight>0) bDoubleAmbient=false; //wtedy tylko jedno światło ruchome
    // if (fOpenGL<1.3) iMultisampling=0; //można by z góry wyłączyć, ale nie mamy jeszcze fOpenGL
    if (iMultisampling)
    { // antyaliasing całoekranowy wyłącza rozmywanie drutów
        bSmoothTraction = false;
    }
    if (iMultiplayer > 0)
    {
        bInactivePause = false; // okno "w tle" nie może pauzować, jeśli włączona komunikacja
        // pauzowanie jest zablokowane dla (iMultiplayer&2)>0, więc iMultiplayer=1 da się zapauzować
        // (tryb instruktora)
    }
    fFpsMin = fFpsAverage -
              fFpsDeviation; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
    fFpsMax = fFpsAverage +
              fFpsDeviation; // górna granica FPS, przy której promień scenerii będzie zwiększany
    if (iPause)
        iTextMode = VK_F1; // jak pauza, to pokazać zegar
    if (qp)
    { // to poniżej wykonywane tylko raz, jedynie po wczytaniu eu07.ini
        Console::ModeSet(iFeedbackMode, iFeedbackPort); // tryb pracy konsoli sterowniczej
        iFpsRadiusMax = 0.000025 * fFpsRadiusMax *
                        fFpsRadiusMax; // maksymalny promień renderowania 3000.0 -> 225
        if (iFpsRadiusMax > 400)
            iFpsRadiusMax = 400;
        if (fDistanceFactor > 1.0)
        { // dla 1.0 specjalny tryb bez przeliczania
            fDistanceFactor =
                iWindowHeight /
                fDistanceFactor; // fDistanceFactor>1.0 dla rozdzielczości większych niż bazowa
            fDistanceFactor *=
                (iMultisampling + 1.0) *
                fDistanceFactor; // do kwadratu, bo większość odległości to ich kwadraty
        }
    }
}

void Global::InitKeys(AnsiString asFileName)
{
    //    if (FileExists(asFileName))
    //    {
    //       Error("Chwilowo plik keys.ini nie jest obsługiwany. Ładuję standardowe
    //       ustawienia.\nKeys.ini file is temporarily not functional, loading default keymap...");
    /*        TQueryParserComp *Parser;
            Parser=new TQueryParserComp(NULL);
            Parser->LoadStringToParse(asFileName);

            for (int keycount=0; keycount<MaxKeys; keycount++)
             {
              Keys[keycount]=Parser->GetNextSymbol().ToInt();
             }

            delete Parser;
    */
    //    }
    //    else
    {
        Keys[k_IncMainCtrl] = VK_ADD;
        Keys[k_IncMainCtrlFAST] = VK_ADD;
        Keys[k_DecMainCtrl] = VK_SUBTRACT;
        Keys[k_DecMainCtrlFAST] = VK_SUBTRACT;
        Keys[k_IncScndCtrl] = VK_DIVIDE;
        Keys[k_IncScndCtrlFAST] = VK_DIVIDE;
        Keys[k_DecScndCtrl] = VK_MULTIPLY;
        Keys[k_DecScndCtrlFAST] = VK_MULTIPLY;
        ///*NORMALNE
        Keys[k_IncLocalBrakeLevel] = VK_NUMPAD1; // VK_NUMPAD7;
        // Keys[k_IncLocalBrakeLevelFAST]=VK_END;  //VK_HOME;
        Keys[k_DecLocalBrakeLevel] = VK_NUMPAD7; // VK_NUMPAD1;
        // Keys[k_DecLocalBrakeLevelFAST]=VK_HOME; //VK_END;
        Keys[k_IncBrakeLevel] = VK_NUMPAD3; // VK_NUMPAD9;
        Keys[k_DecBrakeLevel] = VK_NUMPAD9; // VK_NUMPAD3;
        Keys[k_Releaser] = VK_NUMPAD6;
        Keys[k_EmergencyBrake] = VK_NUMPAD0;
        Keys[k_Brake3] = VK_NUMPAD8;
        Keys[k_Brake2] = VK_NUMPAD5;
        Keys[k_Brake1] = VK_NUMPAD2;
        Keys[k_Brake0] = VK_NUMPAD4;
        Keys[k_WaveBrake] = VK_DECIMAL;
        //*/
        /*MOJE
                Keys[k_IncLocalBrakeLevel]=VK_NUMPAD3;  //VK_NUMPAD7;
                Keys[k_IncLocalBrakeLevelFAST]=VK_NUMPAD3;  //VK_HOME;
                Keys[k_DecLocalBrakeLevel]=VK_DECIMAL;  //VK_NUMPAD1;
                Keys[k_DecLocalBrakeLevelFAST]=VK_DECIMAL; //VK_END;
                Keys[k_IncBrakeLevel]=VK_NUMPAD6;  //VK_NUMPAD9;
                Keys[k_DecBrakeLevel]=VK_NUMPAD9;   //VK_NUMPAD3;
                Keys[k_Releaser]=VK_NUMPAD5;
                Keys[k_EmergencyBrake]=VK_NUMPAD0;
                Keys[k_Brake3]=VK_NUMPAD2;
                Keys[k_Brake2]=VK_NUMPAD1;
                Keys[k_Brake1]=VK_NUMPAD4;
                Keys[k_Brake0]=VK_NUMPAD7;
                Keys[k_WaveBrake]=VK_NUMPAD8;
        */
        Keys[k_AntiSlipping] = VK_RETURN;
        Keys[k_Sand] = VkKeyScan('s');
        Keys[k_Main] = VkKeyScan('m');
        Keys[k_Active] = VkKeyScan('w');
        Keys[k_Battery] = VkKeyScan('j');
        Keys[k_DirectionForward] = VkKeyScan('d');
        Keys[k_DirectionBackward] = VkKeyScan('r');
        Keys[k_Fuse] = VkKeyScan('n');
        Keys[k_Compressor] = VkKeyScan('c');
        Keys[k_Converter] = VkKeyScan('x');
        Keys[k_MaxCurrent] = VkKeyScan('f');
        Keys[k_CurrentAutoRelay] = VkKeyScan('g');
        Keys[k_BrakeProfile] = VkKeyScan('b');
        Keys[k_CurrentNext] = VkKeyScan('z');

        Keys[k_Czuwak] = VkKeyScan(' ');
        Keys[k_Horn] = VkKeyScan('a');
        Keys[k_Horn2] = VkKeyScan('a');

        Keys[k_FailedEngineCutOff] = VkKeyScan('e');

        Keys[k_MechUp] = VK_PRIOR;
        Keys[k_MechDown] = VK_NEXT;
        Keys[k_MechLeft] = VK_LEFT;
        Keys[k_MechRight] = VK_RIGHT;
        Keys[k_MechForward] = VK_UP;
        Keys[k_MechBackward] = VK_DOWN;

        Keys[k_CabForward] = VK_HOME;
        Keys[k_CabBackward] = VK_END;

        Keys[k_Couple] = VK_INSERT;
        Keys[k_DeCouple] = VK_DELETE;

        Keys[k_ProgramQuit] = VK_F10;
        // Keys[k_ProgramPause]=VK_F3;
        Keys[k_ProgramHelp] = VK_F1;
        // Keys[k_FreeFlyMode]=VK_F4;
        Keys[k_WalkMode] = VK_F5;

        Keys[k_OpenLeft] = VkKeyScan(',');
        Keys[k_OpenRight] = VkKeyScan('.');
        Keys[k_CloseLeft] = VkKeyScan(',');
        Keys[k_CloseRight] = VkKeyScan('.');
        Keys[k_DepartureSignal] = VkKeyScan('/');

        // Winger 160204 - obsluga pantografow
        Keys[k_PantFrontUp] = VkKeyScan('p'); // Ra: zamieniony przedni z tylnym
        Keys[k_PantFrontDown] = VkKeyScan('p');
        Keys[k_PantRearUp] = VkKeyScan('o');
        Keys[k_PantRearDown] = VkKeyScan('o');
        // Winger 020304 - ogrzewanie
        Keys[k_Heating] = VkKeyScan('h');
        Keys[k_LeftSign] = VkKeyScan('y');
        Keys[k_UpperSign] = VkKeyScan('u');
        Keys[k_RightSign] = VkKeyScan('i');
        Keys[k_EndSign] = VkKeyScan('t');

        Keys[k_SmallCompressor] = VkKeyScan('v');
        Keys[k_StLinOff] = VkKeyScan('l');
        // ABu 090305 - przyciski uniwersalne, do roznych bajerow :)
        Keys[k_Univ1] = VkKeyScan('[');
        Keys[k_Univ2] = VkKeyScan(']');
        Keys[k_Univ3] = VkKeyScan(';');
        Keys[k_Univ4] = VkKeyScan('\'');
    }
}
/*
vector3 Global::GetCameraPosition()
{
    return pCameraPosition;
}
*/
void Global::SetCameraPosition(vector3 pNewCameraPosition)
{
    pCameraPosition = pNewCameraPosition;
}

void Global::SetCameraRotation(double Yaw)
{ // ustawienie bezwzględnego kierunku kamery z korekcją do przedziału <-M_PI,M_PI>
    pCameraRotation = Yaw;
    while (pCameraRotation < -M_PI)
        pCameraRotation += 2 * M_PI;
    while (pCameraRotation > M_PI)
        pCameraRotation -= 2 * M_PI;
    pCameraRotationDeg = pCameraRotation * 180.0 / M_PI;
}

void Global::BindTexture(GLuint t)
{ // ustawienie aktualnej tekstury, tylko gdy się zmienia
    if (t != iTextureId)
    {
        iTextureId = t;
    }
};

void Global::TrainDelete(TDynamicObject *d)
{ // usunięcie pojazdu prowadzonego przez użytkownika
    if (pWorld)
        pWorld->TrainDelete(d);
};

TDynamicObject *__fastcall Global::DynamicNearest()
{ // ustalenie pojazdu najbliższego kamerze
    return pGround->DynamicNearest(pCamera->Pos);
};

TDynamicObject *__fastcall Global::CouplerNearest()
{ // ustalenie pojazdu najbliższego kamerze
    return pGround->CouplerNearest(pCamera->Pos);
};

bool Global::AddToQuery(TEvent *event, TDynamicObject *who)
{
    return pGround->AddToQuery(event, who);
};
//---------------------------------------------------------------------------

bool Global::DoEvents()
{ // wywoływać czasem, żeby nie robił wrażenia zawieszonego
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return FALSE;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return TRUE;
}
//---------------------------------------------------------------------------

__fastcall TTranscripts::TTranscripts()
{
    iCount = 0; // brak linijek do wyświetlenia
    iStart = 0; // wypełniać od linijki 0
    for (int i = 0; i < MAX_TRANSCRIPTS; ++i)
    { // to do konstruktora można by dać
        aLines[i].fHide = -1.0; // wolna pozycja (czas symulacji, 360.0 to doba)
        aLines[i].iNext = -1; // nie ma kolejnej
    }
    fRefreshTime = 360.0; // wartośc zaporowa
};
__fastcall TTranscripts::~TTranscripts(){};
void TTranscripts::AddLine(char *txt, float show, float hide, bool it)
{ // dodanie linii do tabeli, (show) i (hide) w [s] od aktualnego czasu
    if (show == hide)
        return; // komentarz jest ignorowany
    show = Global::fTimeAngleDeg + show / 240.0; // jeśli doba to 360, to 1s będzie równe 1/240
    hide = Global::fTimeAngleDeg + hide / 240.0;
    int i = iStart, j, k; // od czegoś trzeba zacząć
    while ((aLines[i].iNext >= 0) ? (aLines[aLines[i].iNext].fShow <= show) :
                                    false) // póki nie koniec i wcześniej puszczane
        i = aLines[i].iNext; // przejście do kolejnej linijki
    //(i) wskazuje na linię, po której należy wstawić dany tekst, chyba że
    while (txt ? *txt : false)
        for (j = 0; j < MAX_TRANSCRIPTS; ++j)
            if (aLines[j].fHide < 0.0)
            { // znaleziony pierwszy wolny
                aLines[j].iNext = aLines[i].iNext; // dotychczasowy następny będzie za nowym
                if (aLines[iStart].fHide < 0.0) // jeśli tablica jest pusta
                    iStart = j; // fHide trzeba sprawdzić przed ewentualnym nadpisaniem, gdy i=j=0
                else
                    aLines[i].iNext = j; // a nowy będzie za tamtym wcześniejszym
                aLines[j].fShow = show; // wyświetlać od
                aLines[j].fHide = hide; // wyświetlać do
                aLines[j].bItalic = it;
                aLines[j].asText = AnsiString(txt); // bez sensu, wystarczyłby wskaźnik
                if ((k = aLines[j].asText.Pos("|")) > 0)
                { // jak jest podział linijki na wiersze
                    aLines[j].asText = aLines[j].asText.SubString(1, k - 1);
                    txt += k;
                    i = j; // kolejna linijka dopisywana będzie na koniec właśnie dodanej
                }
                else
                    txt = NULL; // koniec dodawania
                if (fRefreshTime > show) // jeśli odświeżacz ustawiony jest na później
                    fRefreshTime = show; // to odświeżyć wcześniej
                break; // więcej już nic
            }
};
void TTranscripts::Add(char *txt, float len, bool backgorund)
{ // dodanie tekstów, długość dźwięku, czy istotne
    if (!txt)
        return; // pusty tekst
    int i = 0, j = int(0.5 + 10.0 * len); //[0.1s]
    if (*txt == '[')
    { // powinny być dwa nawiasy
        while (*++txt ? *txt != ']' : false)
            if ((*txt >= '0') && (*txt <= '9'))
                i = 10 * i + int(*txt - '0'); // pierwsza liczba aż do ]
        if (*txt ? *++txt == '[' : false)
        {
            j = 0; // drugi nawias określa czas zakończenia wyświetlania
            while (*++txt ? *txt != ']' : false)
                if ((*txt >= '0') && (*txt <= '9'))
                    j = 10 * j + int(*txt - '0'); // druga liczba aż do ]
            if (*txt)
                ++txt; // pominięcie drugiego ]
        }
    }
    AddLine(txt, 0.1 * i, 0.1 * j, false);
};
void TTranscripts::Update()
{ // usuwanie niepotrzebnych (nie częściej niż 10 razy na sekundę)
    if (fRefreshTime > Global::fTimeAngleDeg)
        return; // nie czas jeszcze na zmiany
    // czas odświeżenia można ustalić wg tabelki, kiedy coś się w niej zmienia
    fRefreshTime = Global::fTimeAngleDeg + 360.0; // wartość zaporowa
    int i = iStart, j = -1; // od czegoś trzeba zacząć
    bool change = false; // czy zmieniać napisy?
    do
    {
        if (aLines[i].fHide >= 0.0) // o ile aktywne
            if (aLines[i].fHide < Global::fTimeAngleDeg)
            { // gdy czas wyświetlania upłynął
                aLines[i].fHide = -1.0; // teraz będzie wolną pozycją
                if (i == iStart)
                    iStart = aLines[i].iNext >= 0 ? aLines[i].iNext : 0; // przestawienie pierwszego
                else if (j >= 0)
                    aLines[j].iNext = aLines[i].iNext; // usunięcie ze środka
                change = true;
            }
            else
            { // gdy ma być pokazane
                if (aLines[i].fShow > Global::fTimeAngleDeg) // będzie pokazane w przyszłości
                    if (fRefreshTime > aLines[i].fShow) // a nie ma nic wcześniej
                        fRefreshTime = aLines[i].fShow;
                if (fRefreshTime > aLines[i].fHide)
                    fRefreshTime = aLines[i].fHide;
            }
        // można by jeszcze wykrywać, które nowe mają być pokazane
        j = i;
        i = aLines[i].iNext; // kolejna linijka
    } while (i >= 0); // póki po tablicy
    change = true; // bo na razie nie ma warunku, że coś się dodało
    if (change)
    { // aktualizacja linijek ekranowych
        i = iStart;
        j = -1;
        do
        {
            if (aLines[i].fHide > 0.0) // jeśli nie ukryte
                if (aLines[i].fShow < Global::fTimeAngleDeg) // to dodanie linijki do wyświetlania
                    if (j < 5 - 1) // ograniczona liczba linijek
                        Global::asTranscript[++j] = aLines[i].asText; // skopiowanie tekstu
            i = aLines[i].iNext; // kolejna linijka
        } while (i >= 0); // póki po tablicy
        for (++j; j < 5; ++j)
            Global::asTranscript[j] = ""; // i czyszczenie nieużywanych linijek
    }
};

// Ra: tymczasowe rozwiązanie kwestii zagranicznych (czeskich) napisów
char bezogonkowo[128] = "E?,?\"_++?%S<STZZ?`'\"\".--??s>stzz"
                        " ^^L$A|S^CS<--RZo±,l'uP.,as>L\"lz"
                        "RAAAALCCCEEEEIIDDNNOOOOxRUUUUYTB"
                        "raaaalccceeeeiiddnnoooo-ruuuuyt?";

AnsiString Global::Bezogonkow(AnsiString str, bool _)
{ // wycięcie liter z ogonkami, bo OpenGL nie umie wyświetlić
    for (int i = 1; i <= str.Length(); ++i)
        if (str[i] & 0x80)
            str[i] = bezogonkowo[str[i] & 0x7F];
        else if (str[i] < ' ') // znaki sterujące nie są obsługiwane
            str[i] = ' ';
        else if (_)
            if (str[i] == '_') // nazwy stacji nie mogą zawierać spacji
                str[i] = ' '; // więc trzeba wyświetlać inaczej
    return str;
}

#pragma package(smart_init)
