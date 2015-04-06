//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Model3d.h"
#include "Usefull.h"
#include "Texture.h"
#include "logs.h"
#include "Globals.h"
#include "Timer.h"
#include "mtable.hpp"
//---------------------------------------------------------------------------
#pragma package(smart_init)

double TSubModel::fSquareDist = 0;
int TSubModel::iInstance; // numer renderowanego egzemplarza obiektu
GLuint *TSubModel::ReplacableSkinId = NULL;
int TSubModel::iAlpha = 0x30300030; // maska do testowania flag tekstur wymiennych
TModel3d *TSubModel::pRoot; // Ra: tymczasowo wskaźnik na model widoczny z submodelu
AnsiString *TSubModel::pasText;
// przykłady dla TSubModel::iAlpha:
// 0x30300030 - wszystkie bez kanału alfa
// 0x31310031 - tekstura -1 używana w danym cyklu, pozostałe nie
// 0x32320032 - tekstura -2 używana w danym cyklu, pozostałe nie
// 0x34340034 - tekstura -3 używana w danym cyklu, pozostałe nie
// 0x38380038 - tekstura -4 używana w danym cyklu, pozostałe nie
// 0x3F3F003F - wszystkie wymienne tekstury używane w danym cyklu
// Ale w TModel3d okerśla przezroczystość tekstur wymiennych!

int TSubModelInfo::iTotalTransforms = 0; // ilość transformów
int TSubModelInfo::iTotalNames = 0; // długość obszaru nazw
int TSubModelInfo::iTotalTextures = 0; // długość obszaru tekstur
int TSubModelInfo::iCurrent = 0; // aktualny obiekt
TSubModelInfo *TSubModelInfo::pTable = NULL; // tabele obiektów pomocniczych

char *TStringPack::String(int n)
{ // zwraca wskaźnik do łańcucha o podanym numerze
    if (index ? n < (index[1] >> 2) - 2 : false)
        return data + 8 + index[n + 2]; // indeks upraszcza kwestię wyszukiwania
    // jak nie ma indeksu, to trzeba szukać
    int max = *((int *)(data + 4)); // długość obszaru łańcuchów
    char *ptr = data + 8; // począek obszaru łańcuchów
    for (int i = 0; i < n; ++i)
    { // wyszukiwanie łańcuchów nie jest zbyt optymalne, ale nie musi być
        while (*ptr)
            ++ptr; // wyszukiwanie zera
        ++ptr; // pominięcie zera
        if (ptr > data + max)
            return NULL; // zbyt wysoki numer
    }
    return ptr;
};

__fastcall TSubModel::TSubModel()
{
    ZeroMemory(this, sizeof(TSubModel)); // istotne przy zapisywaniu wersji binarnej
    FirstInit();
};

void TSubModel::FirstInit()
{
    eType = TP_ROTATOR;
    Vertices = NULL;
    uiDisplayList = 0;
    iNumVerts = -1; // do sprawdzenia
    iVboPtr = -1;
    fLight = -1.0; //świetcenie wyłączone
    v_RotateAxis = float3(0, 0, 0);
    v_TransVector = float3(0, 0, 0);
    f_Angle = 0;
    b_Anim = at_None;
    b_aAnim = at_None;
    fVisible = 0.0; // zawsze widoczne
    iVisible = 1;
    fMatrix = NULL; // to samo co iMatrix=0;
    Next = NULL;
    Child = NULL;
    TextureID = 0;
    // TexAlpha=false;
    iFlags = 0x0200; // bit 9=1: submodel został utworzony a nie ustawiony na wczytany plik
    // TexHash=false;
    // Hits=NULL;
    // CollisionPts=NULL;
    // CollisionPtsCount=0;
    Opacity = 1.0; // przy wczytywaniu modeli było dzielone przez 100...
    bWire = false;
    fWireSize = 0;
    fNearAttenStart = 40;
    fNearAttenEnd = 80;
    bUseNearAtten = false;
    iFarAttenDecay = 0;
    fFarDecayRadius = 100;
    fCosFalloffAngle = 0.5; // 120°?
    fCosHotspotAngle = 0.3; // 145°?
    fCosViewAngle = 0;
    fSquareMaxDist = 10000 * 10000; // 10km
    fSquareMinDist = 0;
    iName = -1; // brak nazwy
    iTexture = 0; // brak tekstury
    // asName="";
    // asTexture="";
    pName = pTexture = NULL;
    f4Ambient[0] = f4Ambient[1] = f4Ambient[2] = f4Ambient[3] = 1.0; //{1,1,1,1};
    f4Diffuse[0] = f4Diffuse[1] = f4Diffuse[2] = f4Diffuse[3] = 1.0; //{1,1,1,1};
    f4Specular[0] = f4Specular[1] = f4Specular[2] = 0.0;
    f4Specular[3] = 1.0; //{0,0,0,1};
    f4Emision[0] = f4Emision[1] = f4Emision[2] = f4Emision[3] = 1.0;
    smLetter = NULL; // używany tylko roboczo dla TP_TEXT, do przyspieszenia wyświetlania
};

__fastcall TSubModel::~TSubModel()
{
    if (uiDisplayList)
        glDeleteLists(uiDisplayList, 1);
    if (iFlags & 0x0200)
    { // wczytany z pliku tekstowego musi sam posprzątać
        // SafeDeleteArray(Indices);
        SafeDelete(Next);
        SafeDelete(Child);
        delete fMatrix; // własny transform trzeba usunąć (zawsze jeden)
        delete[] Vertices;
        delete[] pTexture;
        delete[] pName;
    }
    /*
     else
     {//wczytano z pliku binarnego (nie jest właścicielem tablic)
     }
    */
    delete[] smLetter; // używany tylko roboczo dla TP_TEXT, do przyspieszenia wyświetlania
};

void TSubModel::TextureNameSet(const char *n)
{ // ustawienie nazwy submodelu, o ile nie jest wczytany z E3D
    if (iFlags & 0x0200)
    { // tylko jeżeli submodel zosta utworzony przez new
        delete[] pTexture; // usunięcie poprzedniej
        int i = strlen(n);
        if (i)
        { // utworzenie nowej
            pTexture = new char[i + 1];
            strcpy(pTexture, n);
        }
        else
            pTexture = NULL;
    }
};

void TSubModel::NameSet(const char *n)
{ // ustawienie nazwy submodelu, o ile nie jest wczytany z E3D
    if (iFlags & 0x0200)
    { // tylko jeżeli submodel zosta utworzony przez new
        delete[] pName; // usunięcie poprzedniej
        int i = strlen(n);
        if (i)
        { // utworzenie nowej
            pName = new char[i + 1];
            strcpy(pName, n);
        }
        else
            pName = NULL;
    }
};

// int TSubModel::SeekFaceNormal(DWORD *Masks, int f,DWORD dwMask,vector3 *pt,GLVERTEX
// *Vertices)
int TSubModel::SeekFaceNormal(DWORD *Masks, int f, DWORD dwMask, float3 *pt,
                                         float8 *Vertices)
{ // szukanie punktu stycznego do (pt), zwraca numer wierzchołka, a nie trójkąta
    int iNumFaces = iNumVerts / 3; // bo maska powierzchni jest jedna na trójkąt
    // GLVERTEX *p; //roboczy wskaźnik
    float8 *p; // roboczy wskaźnik
    for (int i = f; i < iNumFaces; ++i) // pętla po trójkątach, od trójkąta (f)
        if (Masks[i] & dwMask) // jeśli wspólna maska powierzchni
        {
            p = Vertices + 3 * i;
            if (p->Point == *pt)
                return 3 * i;
            if ((++p)->Point == *pt)
                return 3 * i + 1;
            if ((++p)->Point == *pt)
                return 3 * i + 2;
        }
    return -1; // nie znaleziono stycznego wierzchołka
}

float emm1[] = {1, 1, 1, 0};
float emm2[] = {0, 0, 0, 1};

inline double readIntAsDouble(cParser &parser, int base = 255)
{
    int value;
    parser.getToken(value);
    return double(value) / base;
};

template <typename ColorT> inline void readColor(cParser &parser, ColorT *color)
{
    parser.ignoreToken();
    color[0] = readIntAsDouble(parser);
    color[1] = readIntAsDouble(parser);
    color[2] = readIntAsDouble(parser);
};

inline void readColor(cParser &parser, int &color)
{
    int r, g, b;
    parser.ignoreToken();
    parser.getToken(r);
    parser.getToken(g);
    parser.getToken(b);
    color = r + (g << 8) + (b << 16);
};
/*
inline void readMatrix(cParser& parser,matrix4x4& matrix)
{//Ra: wczytanie transforma
 for (int x=0;x<=3;x++) //wiersze
  for (int y=0;y<=3;y++) //kolumny
   parser.getToken(matrix(x)[y]);
};
*/
inline void readMatrix(cParser &parser, float4x4 &matrix)
{ // Ra: wczytanie transforma
    for (int x = 0; x <= 3; x++) // wiersze
        for (int y = 0; y <= 3; y++) // kolumny
            parser.getToken(matrix(x)[y]);
};

int TSubModel::Load(cParser &parser, TModel3d *Model, int Pos, bool dynamic)
{ // Ra: VBO tworzone na poziomie modelu, a nie submodeli
    iNumVerts = 0;
    iVboPtr = Pos; // pozycja w VBO
    // TMaterialColorf Ambient,Diffuse,Specular;
    // GLuint TextureID;
    // char *extName;
    if (!parser.expectToken("type:"))
        Error("Model type parse failure!");
    {
        std::string type;
        parser.getToken(type);
        if (type == "mesh")
            eType = GL_TRIANGLES; // submodel - trójkaty
        else if (type == "point")
            eType = GL_POINTS; // co to niby jest?
        else if (type == "freespotlight")
            eType = TP_FREESPOTLIGHT; //światełko
        else if (type == "text")
            eType = TP_TEXT; // wyświetlacz tekstowy (generator napisów)
        else if (type == "stars")
            eType = TP_STARS; // wiele punktów świetlnych
    };
    parser.ignoreToken();
    std::string token;
    // parser.getToken(token1); //ze zmianą na małe!
    parser.getTokens(1, false); // nazwa submodelu bez zmieny na małe
    parser >> token;
    NameSet(token.c_str());
    if (dynamic)
    { // dla pojazdu, blokujemy załączone submodele, które mogą być nieobsługiwane
        if (token.find("_on") + 3 == token.length()) // jeśli nazwa kończy się na "_on"
            iVisible = 0; // to domyślnie wyłączyć, żeby się nie nakładało z obiektem "_off"
    }
    else // dla pozostałych modeli blokujemy zapalone światła, które mogą być nieobsługiwane
        if (token.find("Light_On") == 0) // jeśli nazwa zaczyna się od "Light_On"
        iVisible = 0; // to domyślnie wyłączyć, żeby się nie nakładało z obiektem "Light_Off"

    if (parser.expectToken("anim:")) // Ra: ta informacja by się przydała!
    { // rodzaj animacji
        std::string type;
        parser.getToken(type);
        if (type != "false")
        {
            iFlags |= 0x4000; // jak animacja, to trzeba przechowywać macierz zawsze
            if (type == "seconds_jump")
                b_Anim = b_aAnim = at_SecondsJump; // sekundy z przeskokiem
            else if (type == "minutes_jump")
                b_Anim = b_aAnim = at_MinutesJump; // minuty z przeskokiem
            else if (type == "hours_jump")
                b_Anim = b_aAnim = at_HoursJump; // godziny z przeskokiem
            else if (type == "hours24_jump")
                b_Anim = b_aAnim = at_Hours24Jump; // godziny z przeskokiem
            else if (type == "seconds")
                b_Anim = b_aAnim = at_Seconds; // minuty płynnie
            else if (type == "minutes")
                b_Anim = b_aAnim = at_Minutes; // minuty płynnie
            else if (type == "hours")
                b_Anim = b_aAnim = at_Hours; // godziny płynnie
            else if (type == "hours24")
                b_Anim = b_aAnim = at_Hours24; // godziny płynnie
            else if (type == "billboard")
                b_Anim = b_aAnim = at_Billboard; // obrót w pionie do kamery
            else if (type == "wind")
                b_Anim = b_aAnim = at_Wind; // ruch pod wpływem wiatru
            else if (type == "sky")
                b_Anim = b_aAnim = at_Sky; // aniamacja nieba
            else if (type == "ik")
                b_Anim = b_aAnim = at_IK; // IK: zadający
            else if (type == "ik11")
                b_Anim = b_aAnim = at_IK11; // IK: kierunkowany
            else if (type == "ik21")
                b_Anim = b_aAnim = at_IK21; // IK: kierunkowany
            else if (type == "ik22")
                b_Anim = b_aAnim = at_IK22; // IK: kierunkowany
            else if (type == "digital")
                b_Anim = b_aAnim = at_Digital; // licznik mechaniczny
            else if (type == "digiclk")
                b_Anim = b_aAnim = at_DigiClk; // zegar cyfrowy
            else
                b_Anim = b_aAnim = at_Undefined; // nieznana forma animacji
        }
    }
    if (eType < TP_ROTATOR)
        readColor(parser, f4Ambient); // ignoruje token przed
    readColor(parser, f4Diffuse);
    if (eType < TP_ROTATOR)
        readColor(parser, f4Specular);
    parser.ignoreTokens(1); // zignorowanie nazwy "SelfIllum:"
    {
        std::string light;
        parser.getToken(light);
        if (light == "true")
            fLight = 2.0; // zawsze świeci
        else if (light == "false")
            fLight = -1.0; // zawsze ciemy
        else
            fLight = atof(light.c_str());
    };
    if (eType == TP_FREESPOTLIGHT)
    {
        if (!parser.expectToken("nearattenstart:"))
            Error("Model light parse failure!");
        parser.getToken(fNearAttenStart);
        parser.ignoreToken();
        parser.getToken(fNearAttenEnd);
        parser.ignoreToken();
        bUseNearAtten = parser.expectToken("true");
        parser.ignoreToken();
        parser.getToken(iFarAttenDecay);
        parser.ignoreToken();
        parser.getToken(fFarDecayRadius);
        parser.ignoreToken();
        parser.getToken(fCosFalloffAngle); // kąt liczony dla średnicy, a nie promienia
        fCosFalloffAngle = cos(DegToRad(0.5 * fCosFalloffAngle));
        parser.ignoreToken();
        parser.getToken(fCosHotspotAngle); // kąt liczony dla średnicy, a nie promienia
        fCosHotspotAngle = cos(DegToRad(0.5 * fCosHotspotAngle));
        iNumVerts = 1;
        iFlags |= 0x4010; // rysowane w cyklu nieprzezroczystych, macierz musi zostać bez zmiany
    }
    else if (eType < TP_ROTATOR)
    {
        parser.ignoreToken();
        bWire = parser.expectToken("true");
        parser.ignoreToken();
        parser.getToken(fWireSize);
        parser.ignoreToken();
        Opacity = readIntAsDouble(parser,
                                  100.0f); // wymagane jest 0 dla szyb, 100 idzie w nieprzezroczyste
        if (Opacity > 1.0)
            Opacity *= 0.01; // w 2013 był błąd i aby go obejść, trzeba było wpisać 10000.0
        if ((Global::iConvertModels & 1) == 0) // dla zgodności wstecz
            Opacity = 0.0; // wszystko idzie w przezroczyste albo zależnie od tekstury
        if (!parser.expectToken("map:"))
            Error("Model map parse failure!");
        std::string texture;
        parser.getToken(texture);
        if (texture == "none")
        { // rysowanie podanym kolorem
            TextureID = 0;
            iFlags |= 0x10; // rysowane w cyklu nieprzezroczystych
        }
        else if (texture.find("replacableskin") != texture.npos)
        { // McZapkie-060702: zmienialne skory modelu
            TextureID = -1;
            iFlags |= (Opacity < 1.0) ? 1 : 0x10; // zmienna tekstura 1
        }
        else if (texture == "-1")
        {
            TextureID = -1;
            iFlags |= (Opacity < 1.0) ? 1 : 0x10; // zmienna tekstura 1
        }
        else if (texture == "-2")
        {
            TextureID = -2;
            iFlags |= (Opacity < 1.0) ? 2 : 0x10; // zmienna tekstura 2
        }
        else if (texture == "-3")
        {
            TextureID = -3;
            iFlags |= (Opacity < 1.0) ? 4 : 0x10; // zmienna tekstura 3
        }
        else if (texture == "-4")
        {
            TextureID = -4;
            iFlags |= (Opacity < 1.0) ? 8 : 0x10; // zmienna tekstura 4
        }
        else
        { // jeśli tylko nazwa pliku, to dawać bieżącą ścieżkę do tekstur
            // asTexture=AnsiString(texture.c_str()); //zapamiętanie nazwy tekstury
            TextureNameSet(texture.c_str());
            if (texture.find_first_of("/\\") == texture.npos)
                texture.insert(0, Global::asCurrentTexturePath.c_str());
            TextureID = TTexturesManager::GetTextureID(
                szTexturePath, Global::asCurrentTexturePath.c_str(), texture);
            // TexAlpha=TTexturesManager::GetAlpha(TextureID);
            // iFlags|=TexAlpha?0x20:0x10; //0x10-nieprzezroczysta, 0x20-przezroczysta
            if (Opacity < 1.0) // przezroczystość z tekstury brana tylko dla Opacity 0!
                iFlags |= TTexturesManager::GetAlpha(TextureID) ?
                              0x20 :
                              0x10; // 0x10-nieprzezroczysta, 0x20-przezroczysta
            else
                iFlags |= 0x10; // normalnie nieprzezroczyste
            // renderowanie w cyklu przezroczystych tylko jeśli:
            // 1. Opacity=0 (przejściowo <1, czy tam <100) oraz
            // 2. tekstura ma przezroczystość
        };
    }
    else
        iFlags |= 0x10;
    parser.ignoreToken();
    parser.getToken(fSquareMaxDist);
    if (fSquareMaxDist >= 0.0)
        fSquareMaxDist *= fSquareMaxDist;
    else
        fSquareMaxDist = 15000 * 15000; // 15km to więcej, niż się obecnie wyświetla
    parser.ignoreToken();
    parser.getToken(fSquareMinDist);
    fSquareMinDist *= fSquareMinDist;
    parser.ignoreToken();
    fMatrix = new float4x4();
    readMatrix(parser, *fMatrix); // wczytanie transform
    if (!fMatrix->IdentityIs())
        iFlags |= 0x8000; // transform niejedynkowy - trzeba go przechować
    int iNumFaces; // ilość trójkątów
    DWORD *sg; // maski przynależności trójkątów do powierzchni
    if (eType < TP_ROTATOR)
    { // wczytywanie wierzchołków
        parser.ignoreToken();
        // Ra 15-01: to wczytać jako tekst - jeśli pierwszy znak zawiera "*", to dalej będzie nazwa
        // wcześniejszego submodelu, z którego należy wziąć wierzchołki
        // zapewni to jakąś zgodność wstecz, bo zamiast liczby będzie ciąg, którego wartość powinna
        // być uznana jako zerowa
        // parser.getToken(iNumVerts);
        parser.getToken(token);
        if (token[0] == '*')
        { // jeśli pierwszy znak jest gwiazdką, poszukać submodelu o nazwie bez tej gwiazdki i wziąć
          // z niego wierzchołki
            Error("Verticles reference not yet supported!");
        }
        else
        { // normalna lista wierzchołków
            iNumVerts = atoi(token.c_str());
            if (iNumVerts % 3)
            {
                iNumVerts = 0;
                Error("Mesh error, (iNumVertices=" + AnsiString(iNumVerts) + ")%3<>0");
                return 0;
            }
            // Vertices=new GLVERTEX[iNumVerts];
            if (iNumVerts)
            {
                Vertices = new float8[iNumVerts];
                iNumFaces = iNumVerts / 3;
                sg = new DWORD[iNumFaces]; // maski powierzchni: 0 oznacza brak użredniania wektorów
                                           // normalnych
                int *wsp = new int[iNumVerts]; // z którego wierzchołka kopiować wektor normalny
                int maska = 0;
                for (int i = 0; i < iNumVerts; i++)
                { // Ra: z konwersją na układ scenerii - będzie wydajniejsze wyświetlanie
                    wsp[i] = -1; // wektory normalne nie są policzone dla tego wierzchołka
                    if ((i % 3) == 0)
                    { // jeśli będzie maska -1, to dalej będą wierzchołki z wektorami normalnymi,
                      // podanymi jawnie
                        parser.getToken(maska); // maska powierzchni trójkąta
                        sg[i / 3] = (maska == -1) ? 0 : maska; // dla maski -1 będzie 0, czyli nie
                                                               // ma wspólnych wektorów normalnych
                    }
                    parser.getToken(Vertices[i].Point.x);
                    parser.getToken(Vertices[i].Point.y);
                    parser.getToken(Vertices[i].Point.z);
                    if (maska == -1)
                    { // jeśli wektory normalne podane jawnie
                        parser.getToken(Vertices[i].Normal.x);
                        parser.getToken(Vertices[i].Normal.y);
                        parser.getToken(Vertices[i].Normal.z);
                        wsp[i] = i; // wektory normalne "są już policzone"
                    }
                    parser.getToken(Vertices[i].tu);
                    parser.getToken(Vertices[i].tv);
                    if (i % 3 == 2) // jeżeli wczytano 3 punkty
                    {
                        if (Vertices[i].Point == Vertices[i - 1].Point ||
                            Vertices[i - 1].Point == Vertices[i - 2].Point ||
                            Vertices[i - 2].Point == Vertices[i].Point)
                        { // jeżeli punkty się nakładają na siebie
                            --iNumFaces; // o jeden trójkąt mniej
                            iNumVerts -= 3; // czyli o 3 wierzchołki
                            i -= 3; // wczytanie kolejnego w to miejsce
                            WriteLog(AnsiString("Degenerated triangle ignored in: \"") +
                                     AnsiString(pName) + "\", verticle " + AnsiString(i));
                        }
                        if (i > 0) // jeśli pierwszy trójkąt będzie zdegenerowany, to zostanie
                                   // usunięty i nie ma co sprawdzać
                            if (((Vertices[i].Point - Vertices[i - 1].Point).Length() > 1000.0) ||
                                ((Vertices[i - 1].Point - Vertices[i - 2].Point).Length() >
                                 1000.0) ||
                                ((Vertices[i - 2].Point - Vertices[i].Point).Length() > 1000.0))
                            { // jeżeli są dalej niż 2km od siebie //Ra 15-01: obiekt wstawiany nie
                              // powinien być większy niż 300m (trójkąty terenu w E3D mogą mieć
                              // 1.5km)
                                --iNumFaces; // o jeden trójkąt mniej
                                iNumVerts -= 3; // czyli o 3 wierzchołki
                                i -= 3; // wczytanie kolejnego w to miejsce
                                WriteLog(AnsiString("Too large triangle ignored in: \"") +
                                         AnsiString(pName) + "\"");
                            }
                    }
                }
                int i; // indeks dla trójkątów
                float3 *n = new float3[iNumFaces]; // tablica wektorów normalnych dla trójkątów
                for (i = 0; i < iNumFaces; i++) // pętla po trójkątach - będzie szybciej, jak
                                                // wstępnie przeliczymy normalne trójkątów
                    n[i] = SafeNormalize(
                        CrossProduct(Vertices[i * 3].Point - Vertices[i * 3 + 1].Point,
                                     Vertices[i * 3].Point - Vertices[i * 3 + 2].Point));
                int v; // indeks dla wierzchołków
                int f; // numer trójkąta stycznego
                float3 norm; // roboczy wektor normalny
                for (v = 0; v < iNumVerts; v++)
                { // pętla po wierzchołkach trójkątów
                    if (wsp[v] >=
                        0) // jeśli już był liczony wektor normalny z użyciem tego wierzchołka
                        Vertices[v].Normal =
                            Vertices[wsp[v]].Normal; // to wystarczy skopiować policzony wcześniej
                    else
                    { // inaczej musimy dopiero policzyć
                        i = v / 3; // numer trójkąta
                        norm = float3(0, 0, 0); // liczenie zaczynamy od zera
                        f = v; // zaczynamy dodawanie wektorów normalnych od własnego
                        while (f >= 0)
                        { // sumowanie z wektorem normalnym sąsiada (włącznie ze sobą)
                            wsp[f] = v; // informacja, że w tym wierzchołku jest już policzony
                                        // wektor normalny
                            norm += n[f / 3];
                            f = SeekFaceNormal(sg, f / 3 + 1, sg[i], &Vertices[v].Point,
                                               Vertices); // i szukanie od kolejnego trójkąta
                        }
                        // Ra 15-01: należało by jeszcze uwzględnić skalowanie wprowadzane przez
                        // transformy, aby normalne po przeskalowaniu były jednostkowe
                        Vertices[v].Normal =
                            SafeNormalize(norm); // przepisanie do wierzchołka trójkąta
                    }
                }
                delete[] wsp;
                delete[] n;
                delete[] sg;
            }
            else // gdy brak wierzchołków
            {
                eType = TP_ROTATOR; // submodel pomocniczy, ma tylko macierz przekształcenia
                iVboPtr = iNumVerts = 0; // dla formalności
            }
        } // obsługa submodelu z własną listą wierzchołków
    }
    else if (eType == TP_STARS)
    { // punkty świecące dookólnie - składnia jak dla smt_Mesh
        parser.ignoreToken();
        parser.getToken(iNumVerts);
        // Vertices=new GLVERTEX[iNumVerts];
        Vertices = new float8[iNumVerts];
        int i, j;
        for (i = 0; i < iNumVerts; i++)
        {
            if (i % 3 == 0)
                parser.ignoreToken(); // maska powierzchni trójkąta
            parser.getToken(Vertices[i].Point.x);
            parser.getToken(Vertices[i].Point.y);
            parser.getToken(Vertices[i].Point.z);
            parser.getToken(j); // zakodowany kolor
            parser.ignoreToken();
            Vertices[i].Normal.x = ((j)&0xFF) / 255.0; // R
            Vertices[i].Normal.y = ((j >> 8) & 0xFF) / 255.0; // G
            Vertices[i].Normal.z = ((j >> 16) & 0xFF) / 255.0; // B
        }
    }
    // Visible=true; //się potem wyłączy w razie potrzeby
    // iFlags|=0x0200; //wczytano z pliku tekstowego (jest właścicielem tablic)
    if (iNumVerts < 1)
        iFlags &= ~0x3F; // cykl renderowania uzależniony od potomnych
    return iNumVerts; // do określenia wielkości VBO
};

int TSubModel::TriangleAdd(TModel3d *m, int tex, int tri)
{ // dodanie trójkątów do submodelu, używane przy tworzeniu E3D terenu
    TSubModel *s = this;
    while (s ? (s->TextureID != tex) : false)
    { // szukanie submodelu o danej teksturze
        if (s == this)
            s = Child;
        else
            s = s->Next;
    }
    if (!s)
    {
        if (TextureID <= 0)
            s = this; // użycie głównego
        else
        { // dodanie nowego submodelu do listy potomnych
            s = new TSubModel();
            m->AddTo(this, s);
        }
        // s->asTexture=AnsiString(TTexturesManager::GetName(tex).c_str());
        s->TextureNameSet(TTexturesManager::GetName(tex).c_str());
        s->TextureID = tex;
        s->eType = GL_TRIANGLES;
        // iAnimOwner=0; //roboczy wskaźnik na wierzchołek
    }
    if (s->iNumVerts < 0)
        s->iNumVerts = tri; // bo na początku jest -1, czyli że nie wiadomo
    else
        s->iNumVerts += tri; // aktualizacja ilości wierzchołków
    return s->iNumVerts - tri; // zwraca pozycję tych trójkątów w submodelu
};

float8 *__fastcall TSubModel::TrianglePtr(int tex, int pos, int *la, int *ld, int *ls)
{ // zwraca wskaźnik do wypełnienia tabeli wierzchołków, używane przy tworzeniu E3D terenu
    TSubModel *s = this;
    while (s ? s->TextureID != tex : false)
    { // szukanie submodelu o danej teksturze
        if (s == this)
            s = Child;
        else
            s = s->Next;
    }
    if (!s)
        return NULL; // coś nie tak poszło
    if (!s->Vertices)
    { // utworznie tabeli trójkątów
        s->Vertices = new float8[s->iNumVerts];
        // iVboPtr=pos; //pozycja submodelu w tabeli wierzchołków
        // pos+=iNumVerts; //rezerwacja miejsca w tabeli
        s->iVboPtr = iInstance; // pozycja submodelu w tabeli wierzchołków
        iInstance += s->iNumVerts; // pozycja dla następnego
    }
    s->ColorsSet(la, ld, ls); // ustawienie kolorów świateł
    return s->Vertices + pos; // wskaźnik na wolne miejsce w tabeli wierzchołków
};

void TSubModel::DisplayLists()
{ // utworznie po jednej skompilowanej liście dla każdego submodelu
    if (Global::bUseVBO)
        return; // Ra: przy VBO to się nie przyda
    // iFlags|=0x4000; //wyłączenie przeliczania wierzchołków, bo nie są zachowane
    if (eType < TP_ROTATOR)
    {
        if (iNumVerts > 0)
        {
            uiDisplayList = glGenLists(1);
            glNewList(uiDisplayList, GL_COMPILE);
            glColor3fv(f4Diffuse); // McZapkie-240702: zamiast ub
#ifdef USE_VERTEX_ARRAYS
            // ShaXbee-121209: przekazywanie wierzcholkow hurtem
            glVertexPointer(3, GL_DOUBLE, sizeof(GLVERTEX), &Vertices[0].Point.x);
            glNormalPointer(GL_DOUBLE, sizeof(GLVERTEX), &Vertices[0].Normal.x);
            glTexCoordPointer(2, GL_FLOAT, sizeof(GLVERTEX), &Vertices[0].tu);
            glDrawArrays(eType, 0, iNumVerts);
#else
            glBegin(eType);
            for (int i = 0; i < iNumVerts; i++)
            {
                /*
                    glNormal3dv(&Vertices[i].Normal.x);
                    glTexCoord2f(Vertices[i].tu,Vertices[i].tv);
                    glVertex3dv(&Vertices[i].Point.x);
                */
                glNormal3fv(&Vertices[i].Normal.x);
                glTexCoord2f(Vertices[i].tu, Vertices[i].tv);
                glVertex3fv(&Vertices[i].Point.x);
            };
            glEnd();
#endif
            glEndList();
        }
    }
    else if (eType == TP_FREESPOTLIGHT)
    {
        uiDisplayList = glGenLists(1);
        glNewList(uiDisplayList, GL_COMPILE);
        glBindTexture(GL_TEXTURE_2D, 0);
        //     if (eType==smt_FreeSpotLight)
        //      {
        //       if (iFarAttenDecay==0)
        //         glColor3f(Diffuse[0],Diffuse[1],Diffuse[2]);
        //      }
        //      else
        // TODO: poprawic zeby dzialalo
        // glColor3f(f4Diffuse[0],f4Diffuse[1],f4Diffuse[2]);
        glColorMaterial(GL_FRONT, GL_EMISSION);
        glDisable(GL_LIGHTING); // Tolaris-030603: bo mu punkty swiecace sie blendowaly
        glBegin(GL_POINTS);
        glVertex3f(0, 0, 0);
        glEnd();
        glEnable(GL_LIGHTING);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
        glMaterialfv(GL_FRONT, GL_EMISSION, emm2);
        glEndList();
    }
    else if (eType == TP_STARS)
    { // punkty świecące dookólnie
        uiDisplayList = glGenLists(1);
        glNewList(uiDisplayList, GL_COMPILE);
        glBindTexture(GL_TEXTURE_2D, 0); // tekstury nie ma
        glColorMaterial(GL_FRONT, GL_EMISSION);
        glDisable(GL_LIGHTING); // Tolaris-030603: bo mu punkty swiecace sie blendowaly
        glBegin(GL_POINTS);
        for (int i = 0; i < iNumVerts; i++)
        {
            glColor3f(Vertices[i].Normal.x, Vertices[i].Normal.y, Vertices[i].Normal.z);
            // glVertex3dv(&Vertices[i].Point.x);
            glVertex3fv(&Vertices[i].Point.x);
        };
        glEnd();
        glEnable(GL_LIGHTING);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
        glMaterialfv(GL_FRONT, GL_EMISSION, emm2);
        glEndList();
    }
    // SafeDeleteArray(Vertices); //przy VBO muszą zostać do załadowania całego modelu
    if (Child)
        Child->DisplayLists();
    if (Next)
        Next->DisplayLists();
};

void TSubModel::InitialRotate(bool doit)
{ // konwersja układu współrzędnych na zgodny ze scenerią
    if (iFlags & 0xC000) // jeśli jest animacja albo niejednostkowy transform
    { // niejednostkowy transform jest mnożony i wystarczy zabawy
        if (doit)
        { // obrót lewostronny
            if (!fMatrix) // macierzy może nie być w dodanym "bananie"
            {
                fMatrix = new float4x4(); // tworzy macierz o przypadkowej zawartości
                fMatrix->Identity(); // a zaczynamy obracanie od jednostkowej
            }
            iFlags |= 0x8000; // po obróceniu będzie raczej niejedynkowy matrix
            fMatrix->InitialRotate(); // zmiana znaku X oraz zamiana Y i Z
            if (fMatrix->IdentityIs())
                iFlags &= ~0x8000; // jednak jednostkowa po obróceniu
        }
        if (Child)
            Child->InitialRotate(
                false); // potomnych nie obracamy już, tylko ewentualnie optymalizujemy
        else if (Global::iConvertModels & 2) // optymalizacja jest opcjonalna
            if ((iFlags & 0xC000) == 0x8000) // o ile nie ma animacji
            { // jak nie ma potomnych, można wymnożyć przez transform i wyjedynkować go
                float4x4 *mat = GetMatrix(); // transform submodelu
                if (Vertices)
                {
                    for (int i = 0; i < iNumVerts; ++i)
                        Vertices[i].Point = (*mat) * Vertices[i].Point;
                    (*mat)(3)[0] = (*mat)(3)[1] = (*mat)(3)[2] =
                        0.0; // zerujemy przesunięcie przed obracaniem normalnych
                    if (eType != TP_STARS) // gwiazdki mają kolory zamiast normalnych, to ich wtedy
                                           // nie ruszamy
                        for (int i = 0; i < iNumVerts; ++i)
                            Vertices[i].Normal = SafeNormalize((*mat) * Vertices[i].Normal);
                }
                mat->Identity(); // jedynkowanie transformu po przeliczeniu wierzchołków
                iFlags &= ~0x8000; // transform jedynkowy
            }
    }
    else // jak jest jednostkowy i nie ma animacji
        if (doit)
    { // jeśli jest jednostkowy transform, to przeliczamy wierzchołki, a mnożenie podajemy dalej
        double t;
        if (Vertices)
            for (int i = 0; i < iNumVerts; ++i)
            {
                Vertices[i].Point.x = -Vertices[i].Point.x; // zmiana znaku X
                t = Vertices[i].Point.y; // zamiana Y i Z
                Vertices[i].Point.y = Vertices[i].Point.z;
                Vertices[i].Point.z = t;
                // wektory normalne również trzeba przekształcić, bo się źle oświetlają
                Vertices[i].Normal.x = -Vertices[i].Normal.x; // zmiana znaku X
                t = Vertices[i].Normal.y; // zamiana Y i Z
                Vertices[i].Normal.y = Vertices[i].Normal.z;
                Vertices[i].Normal.z = t;
            }
        if (Child)
            Child->InitialRotate(doit); // potomne ewentualnie obrócimy
    }
    if (Next)
        Next->InitialRotate(doit);
};

void TSubModel::ChildAdd(TSubModel *SubModel)
{ // dodanie submodelu potemnego (uzależnionego)
    // Ra: zmiana kolejności, żeby kolejne móc renderować po aktualnym (było przed)
    if (SubModel)
        SubModel->NextAdd(Child); // Ra: zmiana kolejności renderowania
    Child = SubModel;
};

void TSubModel::NextAdd(TSubModel *SubModel)
{ // dodanie submodelu kolejnego (wspólny przodek)
    if (Next)
        Next->NextAdd(SubModel);
    else
        Next = SubModel;
};

int TSubModel::FlagsCheck()
{ // analiza koniecznych zmian pomiędzy submodelami
    // samo pomijanie glBindTexture() nie poprawi wydajności
    // ale można sprawdzić, czy można w ogóle pominąć kod do tekstur (sprawdzanie replaceskin)
    int i;
    if (Child)
    { // Child jest renderowany po danym submodelu
        if (Child->TextureID) // o ile ma teksturę
            if (Child->TextureID != TextureID) // i jest ona inna niż rodzica
                Child->iFlags |= 0x80; // to trzeba sprawdzać, jak z teksturami jest
        i = Child->FlagsCheck();
        iFlags |= 0x00FF0000 & ((i << 16) | (i) | (i >> 8)); // potomny, rodzeństwo i dzieci
        if (eType == TP_TEXT)
        { // wyłączenie renderowania Next dla znaków wyświetlacza tekstowego
            TSubModel *p = Child;
            while (p)
            {
                p->iFlags &= 0xC0FFFFFF;
                p = p->Next;
            }
        }
    }
    if (Next)
    { // Next jest renderowany po danym submodelu (kolejność odwrócona po wczytaniu T3D)
        if (TextureID) // o ile dany ma teksturę
            if ((TextureID != Next->TextureID) ||
                (i & 0x00800000)) // a ma inną albo dzieci zmieniają
                iFlags |= 0x80; // to dany submodel musi sobie ją ustawiać
        i = Next->FlagsCheck();
        iFlags |= 0xFF000000 & ((i << 24) | (i << 8) | (i)); // następny, kolejne i ich dzieci
        // tekstury nie ustawiamy tylko wtedy, gdy jest taka sama jak Next i jego dzieci nie
        // zmieniają
    }
    return iFlags;
};

void TSubModel::SetRotate(float3 vNewRotateAxis, float fNewAngle)
{ // obrócenie submodelu wg podanej osi (np. wskazówki w kabinie)
    v_RotateAxis = vNewRotateAxis;
    f_Angle = fNewAngle;
    if (fNewAngle != 0.0)
    {
        b_Anim = at_Rotate;
        b_aAnim = at_Rotate;
    }
    iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetRotateXYZ(float3 vNewAngles)
{ // obrócenie submodelu o podane kąty wokół osi lokalnego układu
    v_Angles = vNewAngles;
    b_Anim = at_RotateXYZ;
    b_aAnim = at_RotateXYZ;
    iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetRotateXYZ(vector3 vNewAngles)
{ // obrócenie submodelu o podane kąty wokół osi lokalnego układu
    v_Angles.x = vNewAngles.x;
    v_Angles.y = vNewAngles.y;
    v_Angles.z = vNewAngles.z;
    b_Anim = at_RotateXYZ;
    b_aAnim = at_RotateXYZ;
    iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetTranslate(float3 vNewTransVector)
{ // przesunięcie submodelu (np. w kabinie)
    v_TransVector = vNewTransVector;
    b_Anim = at_Translate;
    b_aAnim = at_Translate;
    iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetTranslate(vector3 vNewTransVector)
{ // przesunięcie submodelu (np. w kabinie)
    v_TransVector.x = vNewTransVector.x;
    v_TransVector.y = vNewTransVector.y;
    v_TransVector.z = vNewTransVector.z;
    b_Anim = at_Translate;
    b_aAnim = at_Translate;
    iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetRotateIK1(float3 vNewAngles)
{ // obrócenie submodelu o podane kąty wokół osi lokalnego układu
    v_Angles = vNewAngles;
    iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

struct ToLower
{
    char operator()(char input) { return tolower(input); }
};

TSubModel *__fastcall TSubModel::GetFromName(AnsiString search, bool i)
{
    return GetFromName(search.c_str(), i);
};

TSubModel *__fastcall TSubModel::GetFromName(char *search, bool i)
{
    TSubModel *result;
    // std::transform(search.begin(),search.end(),search.begin(),ToLower());
    // search=search.LowerCase();
    // AnsiString name=AnsiString();
    if (pName && search)
        if ((i ? stricmp(pName, search) : strcmp(pName, search)) == 0)
            return this;
        else if (pName == search)
            return this; // oba NULL
    if (Next)
    {
        result = Next->GetFromName(search);
        if (result)
            return result;
    }
    if (Child)
    {
        result = Child->GetFromName(search);
        if (result)
            return result;
    }
    return NULL;
};

// WORD hbIndices[18]={3,0,1,5,4,2,1,0,4,1,5,3,2,3,5,2,4,0};

void TSubModel::RaAnimation(TAnimType a)
{ // wykonanie animacji niezależnie od renderowania
    switch (a)
    { // korekcja położenia, jeśli submodel jest animowany
    case at_Translate: // Ra: było "true"
        if (iAnimOwner != iInstance)
            break; // cudza animacja
        glTranslatef(v_TransVector.x, v_TransVector.y, v_TransVector.z);
        break;
    case at_Rotate: // Ra: było "true"
        if (iAnimOwner != iInstance)
            break; // cudza animacja
        glRotatef(f_Angle, v_RotateAxis.x, v_RotateAxis.y, v_RotateAxis.z);
        break;
    case at_RotateXYZ:
        if (iAnimOwner != iInstance)
            break; // cudza animacja
        glTranslatef(v_TransVector.x, v_TransVector.y, v_TransVector.z);
        glRotatef(v_Angles.x, 1.0, 0.0, 0.0);
        glRotatef(v_Angles.y, 0.0, 1.0, 0.0);
        glRotatef(v_Angles.z, 0.0, 0.0, 1.0);
        break;
    case at_SecondsJump: // sekundy z przeskokiem
        glRotatef(floor(GlobalTime->mr) * 6.0, 0.0, 1.0, 0.0);
        break;
    case at_MinutesJump: // minuty z przeskokiem
        glRotatef(GlobalTime->mm * 6.0, 0.0, 1.0, 0.0);
        break;
    case at_HoursJump: // godziny skokowo 12h/360°
        glRotatef(GlobalTime->hh * 30.0 * 0.5, 0.0, 1.0, 0.0);
        break;
    case at_Hours24Jump: // godziny skokowo 24h/360°
        glRotatef(GlobalTime->hh * 15.0 * 0.25, 0.0, 1.0, 0.0);
        break;
    case at_Seconds: // sekundy płynnie
        glRotatef(GlobalTime->mr * 6.0, 0.0, 1.0, 0.0);
        break;
    case at_Minutes: // minuty płynnie
        glRotatef(GlobalTime->mm * 6.0 + GlobalTime->mr * 0.1, 0.0, 1.0, 0.0);
        break;
    case at_Hours: // godziny płynnie 12h/360°
        // glRotatef(GlobalTime->hh*30.0+GlobalTime->mm*0.5+GlobalTime->mr/120.0,0.0,1.0,0.0);
        glRotatef(2.0 * Global::fTimeAngleDeg, 0.0, 1.0, 0.0);
        break;
    case at_Hours24: // godziny płynnie 24h/360°
        // glRotatef(GlobalTime->hh*15.0+GlobalTime->mm*0.25+GlobalTime->mr/240.0,0.0,1.0,0.0);
        glRotatef(Global::fTimeAngleDeg, 0.0, 1.0, 0.0);
        break;
    case at_Billboard: // obrót w pionie do kamery
    {
        matrix4x4 mat; // potrzebujemy współrzędne przesunięcia środka układu współrzędnych
                       // submodelu
        glGetDoublev(GL_MODELVIEW_MATRIX, mat.getArray()); // pobranie aktualnej matrycy
        float3 gdzie = float3(mat[3][0], mat[3][1],
                              mat[3][2]); // początek układu współrzędnych submodelu względem kamery
        glLoadIdentity(); // macierz jedynkowa
        glTranslatef(gdzie.x, gdzie.y, gdzie.z); // początek układu zostaje bez zmian
        glRotated(atan2(gdzie.x, gdzie.z) * 180.0 / M_PI, 0.0, 1.0,
                  0.0); // jedynie obracamy w pionie o kąt
    }
    break;
    case at_Wind: // ruch pod wpływem wiatru (wiatr będziemy liczyć potem...)
        glRotated(1.5 * sin(M_PI * GlobalTime->mr / 6.0), 0.0, 1.0, 0.0);
        break;
    case at_Sky: // animacja nieba
        glRotated(Global::fLatitudeDeg, 1.0, 0.0, 0.0); // ustawienie osi OY na północ
        // glRotatef(Global::fTimeAngleDeg,0.0,1.0,0.0); //obrót dobowy osi OX
        glRotated(-fmod(Global::fTimeAngleDeg, 360.0), 0.0, 1.0, 0.0); // obrót dobowy osi OX
        break;
    case at_IK11: // ostatni element animacji szkieletowej (podudzie, stopa)
        glRotatef(v_Angles.z, 0.0, 1.0, 0.0); // obrót względem osi pionowej (azymut)
        glRotatef(v_Angles.x, 1.0, 0.0, 0.0); // obrót względem poziomu (deklinacja)
        break;
    case at_DigiClk: // animacja zegara cyfrowego
    { // ustawienie animacji w submodelach potomnych
        TSubModel *sm = ChildGet();
        do
        { // pętla po submodelach potomnych i obracanie ich o kąt zależy od czasu
            if (sm->pName)
            { // musi mieć niepustą nazwę
                if ((*sm->pName) >= '0')
                    if ((*sm->pName) <= '5') // zegarek ma 6 cyfr maksymalnie
                        sm->SetRotate(float3(0, 1, 0), -Global::fClockAngleDeg[(*sm->pName) - '0']);
            }
            sm = sm->NextGet();
        } while (sm);
    }
    break;
    }
    if (mAnimMatrix) // można by to dać np. do at_Translate
    {
        glMultMatrixf(mAnimMatrix->readArray());
        mAnimMatrix = NULL; // jak animator będzie potrzebował, to ustawi ponownie
    }
};

void TSubModel::RenderDL()
{ // główna procedura renderowania przez DL
    if (iVisible && (fSquareDist >= fSquareMinDist) && (fSquareDist < fSquareMaxDist))
    {
        if (iFlags & 0xC000)
        {
            glPushMatrix();
            if (fMatrix)
                glMultMatrixf(fMatrix->readArray());
            if (b_Anim)
                RaAnimation(b_Anim);
        }
        if (eType < TP_ROTATOR)
        { // renderowanie obiektów OpenGL
            if (iAlpha & iFlags & 0x1F) // rysuj gdy element nieprzezroczysty
            {
                if (TextureID < 0) // && (ReplacableSkinId!=0))
                { // zmienialne skóry
                    glBindTexture(GL_TEXTURE_2D, ReplacableSkinId[-TextureID]);
                    // TexAlpha=!(iAlpha&1); //zmiana tylko w przypadku wymienej tekstury
                }
                else
                    glBindTexture(GL_TEXTURE_2D, TextureID); // również 0
                if (Global::fLuminance < fLight)
                {
                    glMaterialfv(GL_FRONT, GL_EMISSION, f4Diffuse); // zeby swiecilo na kolorowo
                    glCallList(uiDisplayList); // tylko dla siatki
                    glMaterialfv(GL_FRONT, GL_EMISSION, emm2);
                }
                else
                    glCallList(uiDisplayList); // tylko dla siatki
            }
        }
        else if (eType == TP_FREESPOTLIGHT)
        { // wersja DL
            matrix4x4 mat; // macierz opisuje układ renderowania względem kamery
            glGetDoublev(GL_MODELVIEW_MATRIX, mat.getArray());
            // kąt między kierunkiem światła a współrzędnymi kamery
            vector3 gdzie = mat * vector3(0, 0, 0); // pozycja punktu świecącego względem kamery
            fCosViewAngle = DotProduct(Normalize(mat * vector3(0, 0, 1) - gdzie), Normalize(gdzie));
            if (fCosViewAngle > fCosFalloffAngle) // kąt większy niż maksymalny stożek swiatła
            {
                double Distdimm = 1.0;
                if (fCosViewAngle < fCosHotspotAngle) // zmniejszona jasność między Hotspot a
                                                      // Falloff
                    if (fCosFalloffAngle < fCosHotspotAngle)
                        Distdimm = 1.0 -
                                   (fCosHotspotAngle - fCosViewAngle) /
                                       (fCosHotspotAngle - fCosFalloffAngle);
                glColor3f(f4Diffuse[0] * Distdimm, f4Diffuse[1] * Distdimm,
                          f4Diffuse[2] * Distdimm);
                /*  TODO: poprawic to zeby dzialalo
                              if (iFarAttenDecay>0)
                               switch (iFarAttenDecay)
                               {
                                case 1:
                                    Distdimm=fFarDecayRadius/(1+sqrt(fSquareDist));  //dorobic od
                   kata
                                break;
                                case 2:
                                    Distdimm=fFarDecayRadius/(1+fSquareDist);  //dorobic od kata
                                break;
                               }
                              if (Distdimm>1)
                               Distdimm=1;
                              glColor3f(Diffuse[0]*Distdimm,Diffuse[1]*Distdimm,Diffuse[2]*Distdimm);
                */
                //           glPopMatrix();
                //        return;
                glCallList(uiDisplayList); // wyświetlenie warunkowe
            }
        }
        else if (eType == TP_STARS)
        {
            // glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
            if (Global::fLuminance < fLight)
            {
                glMaterialfv(GL_FRONT, GL_EMISSION, f4Diffuse); // zeby swiecilo na kolorowo
                glCallList(uiDisplayList); // narysuj naraz wszystkie punkty z DL
                glMaterialfv(GL_FRONT, GL_EMISSION, emm2);
            }
        }
        if (Child != NULL)
            if (iAlpha & iFlags & 0x001F0000)
                Child->RenderDL();
        if (iFlags & 0xC000)
            glPopMatrix();
    }
    if (b_Anim < at_SecondsJump)
        b_Anim = at_None; // wyłączenie animacji dla kolejnego użycia subm
    if (Next)
        if (iAlpha & iFlags & 0x1F000000)
            Next->RenderDL(); // dalsze rekurencyjnie
}; // Render

void TSubModel::RenderAlphaDL()
{ // renderowanie przezroczystych przez DL
    if (iVisible && (fSquareDist >= fSquareMinDist) && (fSquareDist < fSquareMaxDist))
    {
        if (iFlags & 0xC000)
        {
            glPushMatrix();
            if (fMatrix)
                glMultMatrixf(fMatrix->readArray());
            if (b_aAnim)
                RaAnimation(b_aAnim);
        }
        if (eType < TP_ROTATOR)
        { // renderowanie obiektów OpenGL
            if (iAlpha & iFlags & 0x2F) // rysuj gdy element przezroczysty
            {
                if (TextureID < 0) // && (ReplacableSkinId!=0))
                { // zmienialne skóry
                    glBindTexture(GL_TEXTURE_2D, ReplacableSkinId[-TextureID]);
                    // TexAlpha=iAlpha&1; //zmiana tylko w przypadku wymienej tekstury
                }
                else
                    glBindTexture(GL_TEXTURE_2D, TextureID); // również 0
                if (Global::fLuminance < fLight)
                {
                    glMaterialfv(GL_FRONT, GL_EMISSION, f4Diffuse); // zeby swiecilo na kolorowo
                    glCallList(uiDisplayList); // tylko dla siatki
                    glMaterialfv(GL_FRONT, GL_EMISSION, emm2);
                }
                else
                    glCallList(uiDisplayList); // tylko dla siatki
            }
        }
        else if (eType == TP_FREESPOTLIGHT)
        {
            // dorobić aureolę!
        }
        if (Child != NULL)
            if (eType == TP_TEXT)
            { // tekst renderujemy w specjalny sposób, zamiast submodeli z łańcucha Child
                int i, j = pasText->Length();
                TSubModel *p;
                char c;
                if (!smLetter)
                { // jeśli nie ma tablicy, to ją stworzyć; miejsce nieodpowiednie, ale tymczasowo
                  // może być
                    smLetter = new TSubModel *[256]; // tablica wskaźników submodeli dla
                                                     // wyświetlania tekstu
                    ZeroMemory(smLetter, 256 * sizeof(TSubModel *)); // wypełnianie zerami
                    p = Child;
                    while (p)
                    {
                        smLetter[*p->pName] = p;
                        p = p->Next; // kolejny znak
                    }
                }
                for (i = 1; i <= j; ++i)
                {
                    p = smLetter[(*pasText)[i]]; // znak do wyświetlenia
                    if (p)
                    { // na razie tylko jako przezroczyste
                        p->RenderAlphaDL();
                        if (p->fMatrix)
                            glMultMatrixf(p->fMatrix->readArray()); // przesuwanie widoku
                    }
                }
            }
            else if (iAlpha & iFlags & 0x002F0000)
                Child->RenderAlphaDL();
        if (iFlags & 0xC000)
            glPopMatrix();
    }
    if (b_aAnim < at_SecondsJump)
        b_aAnim = at_None; // wyłączenie animacji dla kolejnego użycia submodelu
    if (Next != NULL)
        if (iAlpha & iFlags & 0x2F000000)
            Next->RenderAlphaDL();
}; // RenderAlpha

void TSubModel::RenderVBO()
{ // główna procedura renderowania przez VBO
    if (iVisible && (fSquareDist >= fSquareMinDist) && (fSquareDist < fSquareMaxDist))
    {
        if (iFlags & 0xC000)
        {
            glPushMatrix();
            if (fMatrix)
                glMultMatrixf(fMatrix->readArray());
            if (b_Anim)
                RaAnimation(b_Anim);
        }
        if (eType < TP_ROTATOR)
        { // renderowanie obiektów OpenGL
            if (iAlpha & iFlags & 0x1F) // rysuj gdy element nieprzezroczysty
            {
                if (TextureID < 0) // && (ReplacableSkinId!=0))
                { // zmienialne skóry
                    glBindTexture(GL_TEXTURE_2D, ReplacableSkinId[-TextureID]);
                    // TexAlpha=!(iAlpha&1); //zmiana tylko w przypadku wymienej tekstury
                }
                else
                    glBindTexture(GL_TEXTURE_2D, TextureID); // również 0
                glColor3fv(f4Diffuse); // McZapkie-240702: zamiast ub
                // glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,f4Diffuse); //to samo, co glColor
                if (Global::fLuminance < fLight)
                {
                    glMaterialfv(GL_FRONT, GL_EMISSION, f4Diffuse); // zeby swiecilo na kolorowo
                    glDrawArrays(eType, iVboPtr,
                                 iNumVerts); // narysuj naraz wszystkie trójkąty z VBO
                    glMaterialfv(GL_FRONT, GL_EMISSION, emm2);
                }
                else
                    glDrawArrays(eType, iVboPtr,
                                 iNumVerts); // narysuj naraz wszystkie trójkąty z VBO
            }
        }
        else if (eType == TP_FREESPOTLIGHT)
        { // wersja VBO
            matrix4x4 mat; // macierz opisuje układ renderowania względem kamery
            glGetDoublev(GL_MODELVIEW_MATRIX, mat.getArray());
            // kąt między kierunkiem światła a współrzędnymi kamery
            vector3 gdzie = mat * vector3(0, 0, 0); // pozycja punktu świecącego względem kamery
            fCosViewAngle = DotProduct(Normalize(mat * vector3(0, 0, 1) - gdzie), Normalize(gdzie));
            if (fCosViewAngle > fCosFalloffAngle) // kąt większy niż maksymalny stożek swiatła
            {
                double Distdimm = 1.0;
                if (fCosViewAngle < fCosHotspotAngle) // zmniejszona jasność między Hotspot a
                                                      // Falloff
                    if (fCosFalloffAngle < fCosHotspotAngle)
                        Distdimm = 1.0 -
                                   (fCosHotspotAngle - fCosViewAngle) /
                                       (fCosHotspotAngle - fCosFalloffAngle);

                /*  TODO: poprawic to zeby dzialalo

                2- Inverse (Applies inverse decay. The formula is luminance=R0/R, where R0 is
                 the radial source of the light if no attenuation is used, or the Near End
                 value of the light if Attenuation is used. R is the radial distance of the
                  illuminated surface from R0.)

                3- Inverse Square (Applies inverse-square decay. The formula for this is (R0/R)^2.
                 This is actually the "real-world" decay of light, but you might find it too dim
                 in the world of computer graphics.)

                <light>.DecayRadius -- The distance over which the decay occurs.

                             if (iFarAttenDecay>0)
                              switch (iFarAttenDecay)
                              {
                               case 1:
                                   Distdimm=fFarDecayRadius/(1+sqrt(fSquareDist));  //dorobic od
                kata
                               break;
                               case 2:
                                   Distdimm=fFarDecayRadius/(1+fSquareDist);  //dorobic od kata
                               break;
                              }
                             if (Distdimm>1)
                              Distdimm=1;

                */
                glBindTexture(GL_TEXTURE_2D, 0); // nie teksturować
                // glColor3f(f4Diffuse[0],f4Diffuse[1],f4Diffuse[2]);
                // glColorMaterial(GL_FRONT,GL_EMISSION);
                float color[4] = {f4Diffuse[0] * Distdimm, f4Diffuse[1] * Distdimm,
                                  f4Diffuse[2] * Distdimm, 0};
                // glColor3f(f4Diffuse[0]*Distdimm,f4Diffuse[1]*Distdimm,f4Diffuse[2]*Distdimm);
                glColorMaterial(GL_FRONT, GL_EMISSION);
                glDisable(GL_LIGHTING); // Tolaris-030603: bo mu punkty swiecace sie blendowaly
                glColor3fv(color); // inaczej są białe
                glMaterialfv(GL_FRONT, GL_EMISSION, color);
                glDrawArrays(GL_POINTS, iVboPtr, iNumVerts); // narysuj wierzchołek z VBO
                glEnable(GL_LIGHTING);
                glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // co ma ustawiać glColor
                glMaterialfv(GL_FRONT, GL_EMISSION, emm2); // bez tego słupy się świecą
            }
        }
        else if (eType == TP_STARS)
        {
            // glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
            if (Global::fLuminance < fLight)
            { // Ra: pewnie można by to zrobić lepiej, bez powtarzania StartVBO()
                pRoot->EndVBO(); // Ra: to też nie jest zbyt ładne
                if (pRoot->StartColorVBO())
                { // wyświetlanie kolorowych punktów zamiast trójkątów
                    glBindTexture(GL_TEXTURE_2D, 0); // tekstury nie ma
                    glColorMaterial(GL_FRONT, GL_EMISSION);
                    glDisable(GL_LIGHTING); // Tolaris-030603: bo mu punkty swiecace sie blendowaly
                    // glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
                    glDrawArrays(GL_POINTS, iVboPtr,
                                 iNumVerts); // narysuj naraz wszystkie punkty z VBO
                    glEnable(GL_LIGHTING);
                    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
                    // glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
                    pRoot->EndVBO();
                    pRoot->StartVBO();
                }
            }
        }
        /*Ra: tu coś jest bez sensu...
            else
            {
             glBindTexture(GL_TEXTURE_2D, 0);
        //        if (eType==smt_FreeSpotLight)
        //         {
        //          if (iFarAttenDecay==0)
        //            glColor3f(Diffuse[0],Diffuse[1],Diffuse[2]);
        //         }
        //         else
        //TODO: poprawic zeby dzialalo
             glColor3f(f4Diffuse[0],f4Diffuse[1],f4Diffuse[2]);
             glColorMaterial(GL_FRONT,GL_EMISSION);
             glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
             //glBegin(GL_POINTS);
             glDrawArrays(GL_POINTS,iVboPtr,iNumVerts);  //narysuj wierzchołek z VBO
             //       glVertex3f(0,0,0);
             //glEnd();
             glEnable(GL_LIGHTING);
             glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
             glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
             //glEndList();
            }
        */
        if (Child != NULL)
            if (iAlpha & iFlags & 0x001F0000)
                Child->RenderVBO();
        if (iFlags & 0xC000)
            glPopMatrix();
    }
    if (b_Anim < at_SecondsJump)
        b_Anim = at_None; // wyłączenie animacji dla kolejnego użycia submodelu
    if (Next)
        if (iAlpha & iFlags & 0x1F000000)
            Next->RenderVBO(); // dalsze rekurencyjnie
}; // RaRender

void TSubModel::RenderAlphaVBO()
{ // renderowanie przezroczystych przez VBO
    if (iVisible && (fSquareDist >= fSquareMinDist) && (fSquareDist < fSquareMaxDist))
    {
        if (iFlags & 0xC000)
        {
            glPushMatrix(); // zapamiętanie matrycy
            if (fMatrix)
                glMultMatrixf(fMatrix->readArray());
            if (b_aAnim)
                RaAnimation(b_aAnim);
        }
        glColor3fv(f4Diffuse);
        if (eType < TP_ROTATOR)
        { // renderowanie obiektów OpenGL
            if (iAlpha & iFlags & 0x2F) // rysuj gdy element przezroczysty
            {
                if (TextureID < 0) // && (ReplacableSkinId!=0))
                { // zmienialne skory
                    glBindTexture(GL_TEXTURE_2D, ReplacableSkinId[-TextureID]);
                    // TexAlpha=iAlpha&1; //zmiana tylko w przypadku wymienej tekstury
                }
                else
                    glBindTexture(GL_TEXTURE_2D, TextureID); // również 0
                if (Global::fLuminance < fLight)
                {
                    glMaterialfv(GL_FRONT, GL_EMISSION, f4Diffuse); // zeby swiecilo na kolorowo
                    glDrawArrays(eType, iVboPtr,
                                 iNumVerts); // narysuj naraz wszystkie trójkąty z VBO
                    glMaterialfv(GL_FRONT, GL_EMISSION, emm2);
                }
                else
                    glDrawArrays(eType, iVboPtr,
                                 iNumVerts); // narysuj naraz wszystkie trójkąty z VBO
            }
        }
        else if (eType == TP_FREESPOTLIGHT)
        {
            // dorobić aureolę!
        }
        if (Child)
            if (iAlpha & iFlags & 0x002F0000)
                Child->RenderAlphaVBO();
        if (iFlags & 0xC000)
            glPopMatrix();
    }
    if (b_aAnim < at_SecondsJump)
        b_aAnim = at_None; // wyłączenie animacji dla kolejnego użycia submodelu
    if (Next)
        if (iAlpha & iFlags & 0x2F000000)
            Next->RenderAlphaVBO();
}; // RaRenderAlpha

//---------------------------------------------------------------------------

void TSubModel::RaArrayFill(CVertNormTex *Vert)
{ // wypełnianie tablic VBO
    if (Child)
        Child->RaArrayFill(Vert);
    if ((eType < TP_ROTATOR) || (eType == TP_STARS))
        for (int i = 0; i < iNumVerts; ++i)
        {
            Vert[iVboPtr + i].x = Vertices[i].Point.x;
            Vert[iVboPtr + i].y = Vertices[i].Point.y;
            Vert[iVboPtr + i].z = Vertices[i].Point.z;
            Vert[iVboPtr + i].nx = Vertices[i].Normal.x;
            Vert[iVboPtr + i].ny = Vertices[i].Normal.y;
            Vert[iVboPtr + i].nz = Vertices[i].Normal.z;
            Vert[iVboPtr + i].u = Vertices[i].tu;
            Vert[iVboPtr + i].v = Vertices[i].tv;
        }
    else if (eType == TP_FREESPOTLIGHT)
        Vert[iVboPtr].x = Vert[iVboPtr].y = Vert[iVboPtr].z = 0.0;
    if (Next)
        Next->RaArrayFill(Vert);
};

void TSubModel::Info()
{ // zapisanie informacji o submodelu do obiektu pomocniczego
    TSubModelInfo *info = TSubModelInfo::pTable + TSubModelInfo::iCurrent;
    info->pSubModel = this;
    if (fMatrix && (iFlags & 0x8000)) // ma matrycę i jest ona niejednostkowa
        info->iTransform = info->iTotalTransforms++;
    if (TextureID > 0)
    { // jeśli ma teksturę niewymienną
        for (int i = 0; i < info->iCurrent; ++i)
            if (TextureID == info->pTable[i].pSubModel->TextureID) // porównanie z wcześniejszym
            {
                info->iTexture = info->pTable[i].iTexture; // taki jaki już był
                break; // koniec sprawdzania
            }
        if (info->iTexture < 0) // jeśli nie znaleziono we wcześniejszych
        {
            info->iTexture = ++info->iTotalTextures; // przydzielenie numeru tekstury w pliku (od 1)
            AnsiString t = AnsiString(pTexture);
            if (t.SubString(t.Length() - 3, 4) == ".tga")
                t.Delete(t.Length() - 3, 4);
            else if (t.SubString(t.Length() - 3, 4) == ".dds")
                t.Delete(t.Length() - 3, 4);
            if (t != AnsiString(pTexture))
            { // jeśli się zmieniło
                // pName=new char[token.length()+1]; //nie ma sensu skracać tabeli
                strcpy(pTexture, t.c_str());
            }
            info->iTextureLen = t.Length() + 1; // przygotowanie do zapisania, z zerem na końcu
        }
    }
    else
        info->iTexture = TextureID; // nie ma albo wymienna
    // if (asName.Length())
    if (pName)
    {
        info->iName = info->iTotalNames++; // przydzielenie numeru nazwy w pliku (od 0)
        info->iNameLen = strlen(pName) + 1; // z zerem na końcu
    }
    ++info->iCurrent; // przejście do kolejnego obiektu pomocniczego
    if (Child)
    {
        info->iChild = info->iCurrent;
        Child->Info();
    }
    if (Next)
    {
        info->iNext = info->iCurrent;
        Next->Info();
    }
};

void TSubModel::InfoSet(TSubModelInfo *info)
{ // ustawienie danych wg obiektu pomocniczego do zapisania w pliku
    int ile = (char *)&uiDisplayList - (char *)&eType; // ilość bajtów pomiędzy tymi zmiennymi
    ZeroMemory(this, sizeof(TSubModel)); // zerowaie całości
    CopyMemory(this, info->pSubModel, ile); // skopiowanie pamięci 1:1
    iTexture = info->iTexture; // numer nazwy tekstury, a nie numer w OpenGL
    TextureID = info->iTexture; // numer tekstury w OpenGL
    iName = info->iName; // numer nazwy w obszarze nazw
    iMatrix = info->iTransform; // numer macierzy
    Next = (TSubModel *)info->iNext; // numer następnego
    Child = (TSubModel *)info->iChild; // numer potomnego
    iFlags &= ~0x200; // nie jest wczytany z tekstowego
    // asTexture=asName="";
    pTexture = pName = NULL;
};

void TSubModel::BinInit(TSubModel *s, float4x4 *m, float8 *v, TStringPack *t,
                                   TStringPack *n, bool dynamic)
{ // ustawienie wskaźników w submodelu
    iVisible = 1; // tymczasowo używane
    Child = ((int)Child > 0) ? s + (int)Child : NULL; // zerowy nie może być potomnym
    Next = ((int)Next > 0) ? s + (int)Next : NULL; // zerowy nie może być następnym
    fMatrix = ((iMatrix >= 0) && m) ? m + iMatrix : NULL;
    // if (n&&(iName>=0)) asName=AnsiString(n->String(iName)); else asName="";
    if (n && (iName >= 0))
    {
        pName = n->String(iName);
        AnsiString s = AnsiString(pName);
        if (!s.IsEmpty())
        { // jeśli dany submodel jest zgaszonym światłem, to domyślnie go ukrywamy
            if (s.SubString(1, 8) == "Light_On") // jeśli jest światłem numerowanym
                iVisible = 0; // to domyślnie wyłączyć, żeby się nie nakładało z obiektem
                              // "Light_Off"
            else if (dynamic) // inaczej wyłączało smugę w latarniach
                if (s.SubString(s.Length() - 2, 3) ==
                    "_on") // jeśli jest kontrolką w stanie zapalonym
                    iVisible = 0; // to domyślnie wyłączyć, żeby się nie nakładało z obiektem "_off"
        }
    }
    else
        pName = NULL;
    if (iTexture > 0)
    { // obsługa stałej tekstury
        // TextureID=TTexturesManager::GetTextureID(t->String(TextureID));
        // asTexture=AnsiString(t->String(iTexture));
        pTexture = t->String(iTexture);
        AnsiString t = AnsiString(pTexture);
        if (t.LastDelimiter("/\\") == 0)
            t.Insert(Global::asCurrentTexturePath, 1);
        TextureID = TTexturesManager::GetTextureID(szTexturePath,
                                                   Global::asCurrentTexturePath.c_str(), t.c_str());
        // TexAlpha=TTexturesManager::GetAlpha(TextureID); //zmienna robocza
        // ustawienie cyklu przezroczyste/nieprzezroczyste zależnie od własności stałej tekstury
        // iFlags=(iFlags&~0x30)|(TTexturesManager::GetAlpha(TextureID)?0x20:0x10);
        // //0x10-nieprzezroczysta, 0x20-przezroczysta
        if (Opacity < 1.0) // przezroczystość z tekstury brana tylko dla Opacity 0!
            iFlags |= TTexturesManager::GetAlpha(TextureID) ?
                          0x20 :
                          0x10; // 0x10-nieprzezroczysta, 0x20-przezroczysta
        else
            iFlags |= 0x10; // normalnie nieprzezroczyste
    }
    b_aAnim = b_Anim; // skopiowanie animacji do drugiego cyklu
    iFlags &= ~0x0200; // wczytano z pliku binarnego (nie jest właścicielem tablic)
    Vertices = v + iVboPtr;
    // if (!iNumVerts) eType=-1; //tymczasowo zmiana typu, żeby się nie renderowało na siłę
};
void TSubModel::AdjustDist()
{ // aktualizacja odległości faz LoD, zależna od rozdzielczości pionowej oraz multisamplingu
    if (fSquareMaxDist > 0.0)
        fSquareMaxDist *= Global::fDistanceFactor;
    if (fSquareMinDist > 0.0)
        fSquareMinDist *= Global::fDistanceFactor;
    // if (fNearAttenStart>0.0) fNearAttenStart*=Global::fDistanceFactor;
    // if (fNearAttenEnd>0.0) fNearAttenEnd*=Global::fDistanceFactor;
    if (Child)
        Child->AdjustDist();
    if (Next)
        Next->AdjustDist();
};

void TSubModel::ColorsSet(int *a, int *d, int *s)
{ // ustawienie kolorów dla modelu terenu
    int i;
    if (a)
        for (i = 0; i < 4; ++i)
            f4Ambient[i] = a[i] / 255.0;
    if (d)
        for (i = 0; i < 4; ++i)
            f4Diffuse[i] = d[i] / 255.0;
    if (s)
        for (i = 0; i < 4; ++i)
            f4Specular[i] = s[i] / 255.0;
};
void TSubModel::ParentMatrix(float4x4 *m)
{ // pobranie transformacji względem wstawienia modelu
    // jeśli nie zostało wykonane Init() (tzn. zaraz po wczytaniu T3D), to dodatkowy obrót
    // obrót T3D jest wymagany np. do policzenia wysokości pantografów
    *m = float4x4(*fMatrix); // skopiowanie, bo będziemy mnożyć
    // m(3)[1]=m[3][1]+0.054; //w górę o wysokość ślizgu (na razie tak)
    TSubModel *sm = this;
    while (sm->Parent)
    { // przenieść tę funkcję do modelu
        if (sm->Parent->GetMatrix())
            *m = *sm->Parent->GetMatrix() * *m;
        sm = sm->Parent;
    }
    // dla ostatniego może być potrzebny dodatkowy obrót, jeśli wczytano z T3D, a nie obrócono
    // jeszcze
};
float TSubModel::MaxY(const float4x4 &m)
{ // obliczenie maksymalnej wysokości, na początek ślizgu w pantografie
    if (eType != 4)
        return 0; // tylko dla trójkątów liczymy
    if (iNumVerts < 1)
        return 0;
    if (!Vertices)
        return 0;
    float y, my = m[0][1] * Vertices[0].Point.x + m[1][1] * Vertices[0].Point.y +
                  m[2][1] * Vertices[0].Point.z + m[3][1];
    for (int i = 1; i < iNumVerts; ++i)
    {
        y = m[0][1] * Vertices[i].Point.x + m[1][1] * Vertices[i].Point.y +
            m[2][1] * Vertices[i].Point.z + m[3][1];
        if (my < y)
            my = y;
    }
    return my;
};
//---------------------------------------------------------------------------

__fastcall TModel3d::TModel3d()
{
    // Materials=NULL;
    // MaterialsCount=0;
    Root = NULL;
    iFlags = 0;
    iSubModelsCount = 0;
    iModel = NULL; // tylko jak wczytany model binarny
    iNumVerts = 0; // nie ma jeszcze wierzchołków
};
/*
__fastcall TModel3d::TModel3d(char *FileName)
{
//    Root=NULL;
//    Materials=NULL;
//    MaterialsCount=0;
 Root=NULL;
 SubModelsCount=0;
 iFlags=0;
 LoadFromFile(FileName);
};
*/
__fastcall TModel3d::~TModel3d()
{
    // SafeDeleteArray(Materials);
    if (iFlags & 0x0200)
    { // wczytany z pliku tekstowego, submodele sprzątają same
        SafeDelete(Root); // submodele się usuną rekurencyjnie
    }
    else
    { // wczytano z pliku binarnego (jest właścicielem tablic)
        m_pVNT = NULL; // nie usuwać tego, bo wskazuje na iModel
        Root = NULL;
        delete[] iModel; // usuwamy cały wczytany plik i to wystarczy
    }
    // później się jeszcze usuwa obiekt z którego dziedziczymy tabelę VBO
};

TSubModel *__fastcall TModel3d::AddToNamed(const char *Name, TSubModel *SubModel)
{
    TSubModel *sm = Name ? GetFromName(Name) : NULL;
    AddTo(sm, SubModel); // szukanie nadrzędnego
    return sm; // zwracamy wskaźnik do nadrzędnego submodelu
};

void TModel3d::AddTo(TSubModel *tmp, TSubModel *SubModel)
{ // jedyny poprawny sposób dodawania submodeli, inaczej mogą zginąć przy zapisie E3D
    if (tmp)
    { // jeśli znaleziony, podłączamy mu jako potomny
        tmp->ChildAdd(SubModel);
    }
    else
    { // jeśli nie znaleziony, podczepiamy do łańcucha głównego
        SubModel->NextAdd(Root); // Ra: zmiana kolejności renderowania wymusza zmianę tu
        Root = SubModel;
    }
    ++iSubModelsCount; // teraz jest o 1 submodel więcej
    iFlags |= 0x0200; // submodele są oddzielne
};

TSubModel *__fastcall TModel3d::GetFromName(const char *sName)
{ // wyszukanie submodelu po nazwie
    if (!sName)
        return Root; // potrzebne do terenu z E3D
    if (iFlags & 0x0200) // wczytany z pliku tekstowego, wyszukiwanie rekurencyjne
        return Root ? Root->GetFromName(sName) : NULL;
    else // wczytano z pliku binarnego, można wyszukać iteracyjnie
    {
        // for (int i=0;i<iSubModelsCount;++i)
        return Root ? Root->GetFromName(sName) : NULL;
    }
};

/*
TMaterial* TModel3d::GetMaterialFromName(char *sName)
{
    AnsiString tmp=AnsiString(sName).Trim();
    for (int i=0; i<MaterialsCount; i++)
        if (strcmp(sName,Materials[i].Name.c_str())==0)
//        if (Trim()==Materials[i].Name.tmp)
            return Materials+i;
    return Materials;
}
*/

bool TModel3d::LoadFromFile(char *FileName, bool dynamic)
{ // wczytanie modelu z pliku
    AnsiString name = AnsiString(FileName).LowerCase();
    int i = name.LastDelimiter(".");
    if (i)
        if (name.SubString(i, name.Length() - i + 1) == ".t3d")
            name.Delete(i, 4);
    asBinary = name + ".e3d";
    if (FileExists(asBinary))
    {
        LoadFromBinFile(asBinary.c_str(), dynamic);
        asBinary = ""; // wyłączenie zapisu
        Init();
    }
    else
    {
        if (FileExists(name + ".t3d"))
        {
            LoadFromTextFile(FileName, dynamic); // wczytanie tekstowego
            if (!dynamic) // pojazdy dopiero po ustawieniu animacji
                Init(); // generowanie siatek i zapis E3D
        }
    }
    return Root ? (iSubModelsCount > 0) : false; // brak pliku albo problem z wczytaniem
};

void TModel3d::LoadFromBinFile(char *FileName, bool dynamic)
{ // wczytanie modelu z pliku binarnego
    WriteLog("Loading - binary model: " + AnsiString(FileName));
    int i = 0, j, k, ch, size;
    TFileStream *fs = new TFileStream(AnsiString(FileName), fmOpenRead);
    size = fs->Size >> 2;
    iModel = new int[size]; // ten wskaźnik musi być w modelu, aby zwolnić pamięć
    fs->Read(iModel, fs->Size); // wczytanie pliku
    delete fs;
    float4x4 *m = NULL; // transformy
    // zestaw kromek:
    while ((i << 2) < size) // w pliku może być kilka modeli
    {
        ch = iModel[i]; // nazwa kromki
        j = i + (iModel[i + 1] >> 2); // początek następnej kromki
        if (ch == 'E3D0') // główna: 'E3D0',len,pod-kromki
        { // tylko tę kromkę znamy, może kiedyś jeszcze DOF się zrobi
            i += 2;
            while (i < j)
            { // przetwarzanie kromek wewnętrznych
                ch = iModel[i]; // nazwa kromki
                k = (iModel[i + 1] >> 2); // długość aktualnej kromki
                switch (ch)
                {
                case 'MDL0': // zmienne modelu: 'E3D0',len,(informacje o modelu)
                    break;
                case 'VNT0': // wierzchołki: 'VNT0',len,(32 bajty na wierzchołek)
                    iNumVerts = (k - 2) >> 3;
                    m_nVertexCount = iNumVerts;
                    m_pVNT = (CVertNormTex *)(iModel + i + 2);
                    break;
                case 'SUB0': // submodele: 'SUB0',len,(256 bajtów na submodel)
                    iSubModelsCount = (k - 2) / 64;
                    Root = (TSubModel *)(iModel + i + 2); // numery na wskaźniki przetworzymy
                                                          // później
                    break;
                case 'SUB1': // submodele: 'SUB1',len,(320 bajtów na submodel)
                    iSubModelsCount = (k - 2) / 80;
                    Root = (TSubModel *)(iModel + i + 2); // numery na wskaźniki przetworzymy
                                                          // później
                    for (ch = 1; ch < iSubModelsCount;
                         ++ch) // trzeba przesunąć bliżej, bo 256 wystarczy
                        MoveMemory(((char *)Root) + 256 * ch, ((char *)Root) + 320 * ch, 256);
                    break;
                case 'TRA0': // transformy: 'TRA0',len,(64 bajty na transform)
                    m = (float4x4 *)(iModel + i + 2); // tabela transformów
                    break;
                case 'TRA1': // transformy: 'TRA1',len,(128 bajtów na transform)
                    m = (float4x4 *)(iModel + i + 2); // tabela transformów
                    for (ch = 0; ch < ((k - 2) >> 1); ++ch)
                        *(((float *)m) + ch) = *(((double *)m) + ch); // przepisanie double do float
                    break;
                case 'IDX1': // indeksy 1B: 'IDX2',len,(po bajcie na numer wierzchołka)
                    break;
                case 'IDX2': // indeksy 2B: 'IDX2',len,(po 2 bajty na numer wierzchołka)
                    break;
                case 'IDX4': // indeksy 4B: 'IDX4',len,(po 4 bajty na numer wierzchołka)
                    break;
                case 'TEX0': // tekstury: 'TEX0',len,(łańcuchy zakończone zerem - pliki tekstur)
                    Textures.Init((char *)(iModel + i)); //łącznie z nagłówkiem
                    break;
                case 'TIX0': // indeks nazw tekstur
                    Textures.InitIndex((int *)(iModel + i)); //łącznie z nagłówkiem
                    break;
                case 'NAM0': // nazwy: 'NAM0',len,(łańcuchy zakończone zerem - nazwy submodeli)
                    Names.Init((char *)(iModel + i)); //łącznie z nagłówkiem
                    break;
                case 'NIX0': // indeks nazw submodeli
                    Names.InitIndex((int *)(iModel + i)); //łącznie z nagłówkiem
                    break;
                }
                i += k; // przejście do kolejnej kromki
            }
        }
        i = j;
    }
    for (i = 0; i < iSubModelsCount; ++i)
    { // aktualizacja wskaźników w submodelach
        Root[i].BinInit(Root, m, (float8 *)m_pVNT, &Textures, &Names, dynamic);
        if (Root[i].ChildGet())
            Root[i].ChildGet()->Parent = Root + i; // wpisanie wskaźnika nadrzędnego do potmnego
        if (Root[i].NextGet())
            Root[i].NextGet()->Parent =
                Root[i].Parent; // skopiowanie wskaźnika nadrzędnego do kolejnego
    }
    iFlags &= ~0x0200;
    return;
};

void TModel3d::LoadFromTextFile(char *FileName, bool dynamic)
{ // wczytanie submodelu z pliku tekstowego
    WriteLog("Loading - text model: " + AnsiString(FileName));
    iFlags |= 0x0200; // wczytano z pliku tekstowego (właścicielami tablic są submodle)
    cParser parser(FileName, cParser::buffer_FILE); // Ra: tu powinno być "models\\"...
    TSubModel *SubModel;
    std::string token;
    parser.getToken(token);
    iNumVerts = 0; // w konstruktorze to jest
    while (token != "" || parser.eof())
    {
        std::string parent;
        // parser.getToken(parent);
        parser.getTokens(1, false); // nazwa submodelu nadrzędnego bez zmieny na małe
        parser >> parent;
        if (parent == "")
            break;
        SubModel = new TSubModel();
        iNumVerts += SubModel->Load(parser, this, iNumVerts, dynamic);
        SubModel->Parent = AddToNamed(
            parent.c_str(), SubModel); // będzie potrzebne do wyliczenia pozycji, np. pantografu
        // iSubModelsCount++;
        parser.getToken(token);
    }
    // Ra: od wersji 334 przechylany jest cały model, a nie tylko pierwszy submodel
    // ale bujanie kabiny nadal używa bananów :( od 393 przywrócone, ale z dodatkowym warunkiem
    if (Global::iConvertModels & 4)
    { // automatyczne banany czasem psuły przechylanie kabin...
        if (dynamic && Root)
        {
            if (Root->NextGet()) // jeśli ma jakiekolwiek kolejne
            { // dynamic musi mieć "banana", bo tylko pierwszy obiekt jest animowany, a następne nie
                SubModel = new TSubModel(); // utworzenie pustego
                SubModel->ChildAdd(Root);
                Root = SubModel;
                ++iSubModelsCount;
            }
            Root->WillBeAnimated(); // bo z tym jest dużo problemów
        }
    }
}

void TModel3d::Init()
{ // obrócenie początkowe układu współrzędnych, dla pojazdów wykonywane po analizie animacji
    if (iFlags & 0x8000)
        return; // operacje zostały już wykonane
    if (Root)
    {
        if (iFlags & 0x0200) // jeśli wczytano z pliku tekstowego
        { // jest jakiś dziwny błąd, że obkręcany ma być tylko ostatni submodel głównego łańcucha
            // TSubModel *p=Root;
            // do
            //{p->InitialRotate(true); //ostatniemu należy się konwersja układu współrzędnych
            // p=p->NextGet();
            //}
            // while (p->NextGet())
            // Root->InitialRotate(false); //a poprzednim tylko optymalizacja
            Root->InitialRotate(true); // argumet określa, czy wykonać pierwotny obrót
        }
        iFlags |= Root->FlagsCheck() | 0x8000; // flagi całego modelu
        if (!asBinary.IsEmpty()) // jeśli jest podana nazwa
        {
            if (Global::iConvertModels) // i włączony zapis
                SaveToBinFile(asBinary.c_str()); // utworzy tablicę (m_pVNT)
            asBinary = ""; // zablokowanie powtórnego zapisu
        }
        if (iNumVerts)
        {
            if (Global::fDistanceFactor !=
                1.0) // trochę zaoszczędzi czasu na modelach z wieloma submocelami
                Root->AdjustDist(); // aktualizacja odległości faz LoD, zależnie od rozdzielczości
                                    // pionowej oraz multisamplingu
            if (Global::bUseVBO)
            {
                if (!m_pVNT) // jeśli nie ma jeszcze tablicy (wczytano z pliku tekstowego)
                { // tworzenie tymczasowej tablicy z wierzchołkami całego modelu
                    MakeArray(iNumVerts); // tworzenie tablic dla VBO
                    Root->RaArrayFill(m_pVNT); // wypełnianie tablicy
                    BuildVBOs(); // tworzenie VBO i usuwanie tablicy z pamięci
                }
                else
                    BuildVBOs(false); // tworzenie VBO bez usuwania tablicy z pamięci
            }
            else
            { // przygotowanie skompilowanych siatek dla DisplayLists
                Root->DisplayLists(); // tworzenie skompilowanej listy dla submodelu
            }
            // if (Root->TextureID) //o ile ma teksturę
            // Root->iFlags|=0x80; //konieczność ustawienia tekstury
        }
    }
};

void TModel3d::SaveToBinFile(char *FileName)
{ // zapis modelu binarnego
    WriteLog("Saving E3D binary model.");
    int i, zero = 0;
    TSubModelInfo *info = new TSubModelInfo[iSubModelsCount];
    info->Reset();
    Root->Info(); // zebranie informacji o submodelach
    int len; //łączna długość pliku
    int sub; // ilość submodeli (w bajtach)
    int tra; // wielkość obszaru transformów
    int vnt; // wielkość obszaru wierzchołków
    int tex = 0; // wielkość obszaru nazw tekstur
    int nam = 0; // wielkość obszaru nazw submodeli
    sub = 8 + sizeof(TSubModel) * iSubModelsCount;
    tra = info->iTotalTransforms ? 8 + 64 * info->iTotalTransforms : 0;
    vnt = 8 + 32 * iNumVerts;
    for (i = 0; i < iSubModelsCount; ++i)
    {
        tex += info[i].iTextureLen;
        nam += info[i].iNameLen;
    }
    if (tex)
        tex += 9; // 8 na nagłówek i jeden ciąg pusty (tylko znacznik końca)
    if (nam)
        nam += 8;
    len = 8 + sub + tra + vnt + tex + ((-tex) & 3) + nam + ((-nam) & 3);
    TSubModel *roboczy = new TSubModel(); // bufor używany do zapisywania
    // AnsiString *asN=&roboczy->asName,*asT=&roboczy->asTexture;
    // roboczy->FirstInit(); //żeby delete nie usuwało czego nie powinno
    TFileStream *fs = new TFileStream(AnsiString(FileName), fmCreate);
    fs->Write("E3D0", 4); // kromka główna
    fs->Write(&len, 4);
    {
        fs->Write("SUB0", 4); // dane submodeli
        fs->Write(&sub, 4);
        for (i = 0; i < iSubModelsCount; ++i)
        {
            roboczy->InfoSet(info + i);
            fs->Write(roboczy, sizeof(TSubModel)); // zapis jednego submodelu
        }
    }
    if (tra)
    { // zapis transformów
        fs->Write("TRA0", 4); // transformy
        fs->Write(&tra, 4);
        for (i = 0; i < iSubModelsCount; ++i)
            if (info[i].iTransform >= 0)
                fs->Write(info[i].pSubModel->GetMatrix(), 16 * 4);
    }
    { // zapis wierzchołków
        MakeArray(iNumVerts); // tworzenie tablic dla VBO
        Root->RaArrayFill(m_pVNT); // wypełnianie tablicy
        fs->Write("VNT0", 4); // wierzchołki
        fs->Write(&vnt, 4);
        fs->Write(m_pVNT, 32 * iNumVerts);
    }
    if (tex) // może być jeden submodel ze zmienną teksturą i nazwy nie będzie
    { // zapis nazw tekstur
        fs->Write("TEX0", 4); // nazwy tekstur
        i = (tex + 3) & ~3; // zaokrąglenie w górę
        fs->Write(&i, 4);
        fs->Write(&zero, 1); // ciąg o numerze zero nie jest używany, ma tylko znacznik końca
        for (i = 0; i < iSubModelsCount; ++i)
            if (info[i].iTextureLen)
                fs->Write(info[i].pSubModel->pTexture, info[i].iTextureLen);
        if ((-tex) & 3)
            fs->Write(&zero, ((-tex) & 3)); // wyrównanie do wielokrotności 4 bajtów
    }
    if (nam) // może być jeden anonimowy submodel w modelu
    { // zapis nazw submodeli
        fs->Write("NAM0", 4); // nazwy submodeli
        i = (nam + 3) & ~3; // zaokrąglenie w górę
        fs->Write(&i, 4);
        for (i = 0; i < iSubModelsCount; ++i)
            if (info[i].iNameLen)
                fs->Write(info[i].pSubModel->pName, info[i].iNameLen);
        if ((-nam) & 3)
            fs->Write(&zero, ((-nam) & 3)); // wyrównanie do wielokrotności 4 bajtów
    }
    delete fs;
    // roboczy->FirstInit(); //żeby delete nie usuwało czego nie powinno
    // roboczy->iFlags=0; //żeby delete nie usuwało czego nie powinno
    // roboczy->asName)=asN;
    //&roboczy->asTexture=asT;
    delete roboczy;
    delete[] info;
};

void TModel3d::BreakHierarhy() { Error("Not implemented yet :("); };

/*
void TModel3d::Render(vector3 pPosition,double fAngle,GLuint ReplacableSkinId,int iAlpha)
{
//    glColor3f(1.0f,1.0f,1.0f);
//    glColor3f(0.0f,0.0f,0.0f);
 glPushMatrix();

 glTranslated(pPosition.x,pPosition.y,pPosition.z);
 if (fAngle!=0)
  glRotatef(fAngle,0,1,0);
/*
 matrix4x4 Identity;
 Identity.Identity();

    matrix4x4 CurrentMatrix;
    glGetdoublev(GL_MODELVIEW_MATRIX,CurrentMatrix.getArray());
    vector3 pos=vector3(0,0,0);
    pos=CurrentMatrix*pos;
    fSquareDist=SquareMagnitude(pos);
  * /
    fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition());

#ifdef _DEBUG
    if (Root)
        Root->Render(ReplacableSkinId,iAlpha);
#else
    Root->Render(ReplacableSkinId,iAlpha);
#endif
    glPopMatrix();
};
*/

void TModel3d::Render(double fSquareDistance, GLuint *ReplacableSkinId, int iAlpha)
{
    iAlpha ^= 0x0F0F000F; // odwrócenie flag tekstur, aby wyłapać nieprzezroczyste
    if (iAlpha & iFlags & 0x1F1F001F) // czy w ogóle jest co robić w tym cyklu?
    {
        TSubModel::fSquareDist = fSquareDistance; // zmienna globalna!
        Root->ReplacableSet(ReplacableSkinId, iAlpha);
        Root->RenderDL();
    }
};

void TModel3d::RenderAlpha(double fSquareDistance, GLuint *ReplacableSkinId, int iAlpha)
{
    if (iAlpha & iFlags & 0x2F2F002F)
    {
        TSubModel::fSquareDist = fSquareDistance; // zmienna globalna!
        Root->ReplacableSet(ReplacableSkinId, iAlpha);
        Root->RenderAlphaDL();
    }
};

/*
void TModel3d::RaRender(vector3 pPosition,double fAngle,GLuint *ReplacableSkinId,int
iAlpha)
{
//    glColor3f(1.0f,1.0f,1.0f);
//    glColor3f(0.0f,0.0f,0.0f);
 glPushMatrix(); //zapamiętanie matrycy przekształcenia
 glTranslated(pPosition.x,pPosition.y,pPosition.z);
 if (fAngle!=0)
  glRotatef(fAngle,0,1,0);
/*
 matrix4x4 Identity;
 Identity.Identity();

 matrix4x4 CurrentMatrix;
 glGetdoublev(GL_MODELVIEW_MATRIX,CurrentMatrix.getArray());
 vector3 pos=vector3(0,0,0);
 pos=CurrentMatrix*pos;
 fSquareDist=SquareMagnitude(pos);
*/
/*
 fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition()); //zmienna globalna!
 if (StartVBO())
 {//odwrócenie flag, aby wyłapać nieprzezroczyste
  Root->ReplacableSet(ReplacableSkinId,iAlpha^0x0F0F000F);
  Root->RaRender();
  EndVBO();
 }
 glPopMatrix(); //przywrócenie ustawień przekształcenia
};
*/

void TModel3d::RaRender(double fSquareDistance, GLuint *ReplacableSkinId, int iAlpha)
{ // renderowanie specjalne, np. kabiny
    iAlpha ^= 0x0F0F000F; // odwrócenie flag tekstur, aby wyłapać nieprzezroczyste
    if (iAlpha & iFlags & 0x1F1F001F) // czy w ogóle jest co robić w tym cyklu?
    {
        TSubModel::fSquareDist = fSquareDistance; // zmienna globalna!
        if (StartVBO())
        { // odwrócenie flag, aby wyłapać nieprzezroczyste
            Root->ReplacableSet(ReplacableSkinId, iAlpha);
            Root->pRoot = this;
            Root->RenderVBO();
            EndVBO();
        }
    }
};

void TModel3d::RaRenderAlpha(double fSquareDistance, GLuint *ReplacableSkinId,
                                        int iAlpha)
{ // renderowanie specjalne, np. kabiny
    if (iAlpha & iFlags & 0x2F2F002F) // czy w ogóle jest co robić w tym cyklu?
    {
        TSubModel::fSquareDist = fSquareDistance; // zmienna globalna!
        if (StartVBO())
        {
            Root->ReplacableSet(ReplacableSkinId, iAlpha);
            Root->RenderAlphaVBO();
            EndVBO();
        }
    }
};

/*
void TModel3d::RaRenderAlpha(vector3 pPosition,double fAngle,GLuint *ReplacableSkinId,int
iAlpha)
{
 glPushMatrix();
 glTranslatef(pPosition.x,pPosition.y,pPosition.z);
 if (fAngle!=0)
  glRotatef(fAngle,0,1,0);
 fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition()); //zmienna globalna!
 if (StartVBO())
 {Root->ReplacableSet(ReplacableSkinId,iAlpha);
  Root->RaRenderAlpha();
  EndVBO();
 }
 glPopMatrix();
};
*/

//-----------------------------------------------------------------------------
// 2011-03-16 cztery nowe funkcje renderowania z możliwością pochylania obiektów
//-----------------------------------------------------------------------------

void TModel3d::Render(vector3 *vPosition, vector3 *vAngle, GLuint *ReplacableSkinId,
                                 int iAlpha)
{ // nieprzezroczyste, Display List
    glPushMatrix();
    glTranslated(vPosition->x, vPosition->y, vPosition->z);
    if (vAngle->y != 0.0)
        glRotated(vAngle->y, 0.0, 1.0, 0.0);
    if (vAngle->x != 0.0)
        glRotated(vAngle->x, 1.0, 0.0, 0.0);
    if (vAngle->z != 0.0)
        glRotated(vAngle->z, 0.0, 0.0, 1.0);
    TSubModel::fSquareDist =
        SquareMagnitude(*vPosition - Global::GetCameraPosition()); // zmienna globalna!
    // odwrócenie flag, aby wyłapać nieprzezroczyste
    Root->ReplacableSet(ReplacableSkinId, iAlpha ^ 0x0F0F000F);
    Root->RenderDL();
    glPopMatrix();
};
void TModel3d::RenderAlpha(vector3 *vPosition, vector3 *vAngle, GLuint *ReplacableSkinId,
                                      int iAlpha)
{ // przezroczyste, Display List
    glPushMatrix();
    glTranslated(vPosition->x, vPosition->y, vPosition->z);
    if (vAngle->y != 0.0)
        glRotated(vAngle->y, 0.0, 1.0, 0.0);
    if (vAngle->x != 0.0)
        glRotated(vAngle->x, 1.0, 0.0, 0.0);
    if (vAngle->z != 0.0)
        glRotated(vAngle->z, 0.0, 0.0, 1.0);
    TSubModel::fSquareDist =
        SquareMagnitude(*vPosition - Global::GetCameraPosition()); // zmienna globalna!
    Root->ReplacableSet(ReplacableSkinId, iAlpha);
    Root->RenderAlphaDL();
    glPopMatrix();
};
void TModel3d::RaRender(vector3 *vPosition, vector3 *vAngle, GLuint *ReplacableSkinId,
                                   int iAlpha)
{ // nieprzezroczyste, VBO
    glPushMatrix();
    glTranslated(vPosition->x, vPosition->y, vPosition->z);
    if (vAngle->y != 0.0)
        glRotated(vAngle->y, 0.0, 1.0, 0.0);
    if (vAngle->x != 0.0)
        glRotated(vAngle->x, 1.0, 0.0, 0.0);
    if (vAngle->z != 0.0)
        glRotated(vAngle->z, 0.0, 0.0, 1.0);
    TSubModel::fSquareDist =
        SquareMagnitude(*vPosition - Global::GetCameraPosition()); // zmienna globalna!
    if (StartVBO())
    { // odwrócenie flag, aby wyłapać nieprzezroczyste
        Root->ReplacableSet(ReplacableSkinId, iAlpha ^ 0x0F0F000F);
        Root->RenderVBO();
        EndVBO();
    }
    glPopMatrix();
};
void TModel3d::RaRenderAlpha(vector3 *vPosition, vector3 *vAngle,
                                        GLuint *ReplacableSkinId, int iAlpha)
{ // przezroczyste, VBO
    glPushMatrix();
    glTranslated(vPosition->x, vPosition->y, vPosition->z);
    if (vAngle->y != 0.0)
        glRotated(vAngle->y, 0.0, 1.0, 0.0);
    if (vAngle->x != 0.0)
        glRotated(vAngle->x, 1.0, 0.0, 0.0);
    if (vAngle->z != 0.0)
        glRotated(vAngle->z, 0.0, 0.0, 1.0);
    TSubModel::fSquareDist =
        SquareMagnitude(*vPosition - Global::GetCameraPosition()); // zmienna globalna!
    if (StartVBO())
    {
        Root->ReplacableSet(ReplacableSkinId, iAlpha);
        Root->RenderAlphaVBO();
        EndVBO();
    }
    glPopMatrix();
};

//-----------------------------------------------------------------------------
// 2012-02 funkcje do tworzenia terenu z E3D
//-----------------------------------------------------------------------------

int TModel3d::TerrainCount()
{ // zliczanie kwadratów kilometrowych (główna linia po Next) do tworznia tablicy
    int i = 0;
    TSubModel *r = Root;
    while (r)
    {
        r = r->NextGet();
        ++i;
    }
    return i;
};
TSubModel *__fastcall TModel3d::TerrainSquare(int n)
{ // pobieranie wskaźnika do submodelu (n)
    int i = 0;
    TSubModel *r = Root;
    while (i < n)
    {
        r = r->NextGet();
        ++i;
    }
    r->UnFlagNext(); // blokowanie wyświetlania po Next głównej listy
    return r;
};
void TModel3d::TerrainRenderVBO(int n)
{ // renderowanie terenu z VBO
    glPushMatrix();
    // glTranslated(vPosition->x,vPosition->y,vPosition->z);
    // if (vAngle->y!=0.0) glRotated(vAngle->y,0.0,1.0,0.0);
    // if (vAngle->x!=0.0) glRotated(vAngle->x,1.0,0.0,0.0);
    // if (vAngle->z!=0.0) glRotated(vAngle->z,0.0,0.0,1.0);
    // TSubModel::fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition()); //zmienna
    // globalna!
    if (StartVBO())
    { // odwrócenie flag, aby wyłapać nieprzezroczyste
        // Root->ReplacableSet(ReplacableSkinId,iAlpha^0x0F0F000F);
        TSubModel *r = Root;
        while (r)
        {
            if (r->iVisible == n) // tylko jeśli ma być widoczny w danej ramce (problem dla
                                  // 0==false)
                r->RenderVBO(); // sub kolejne (Next) się nie wyrenderują
            r = r->NextGet();
        }
        EndVBO();
    }
    glPopMatrix();
};
