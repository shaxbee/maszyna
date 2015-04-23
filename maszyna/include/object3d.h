
#ifndef object3d_H
#define object3d_H

#include "../commons.h"
#include "../commons_usr.h"
#include "ResourceManager.h"
#include "dumb3d.h"
#include "float3d.h"
#include "VBO.h"



enum TAnimationType //rodzaj animacji
{
	qat_None, //brak
	qat_Rotate, //obrót wzglêdem wektora o k¹t
	qat_RotateXYZ, //obrót wzglêdem osi o k¹ty
	qat_Translate, //przesuniêcie
	qat_SecondsJump, //sekundy z przeskokiem
	qat_MinutesJump, //minuty z przeskokiem
	qat_HoursJump, //godziny z przeskokiem 12h/360°
	qat_Hours24Jump, //godziny z przeskokiem 24h/360°
	qat_Seconds, //sekundy p³ynnie
	qat_Minutes, //minuty p³ynnie
	qat_Hours, //godziny p³ynnie 12h/360°
	qat_Hours24, //godziny p³ynnie 24h/360°
	qat_Billboard, //obrót w pionie do kamery
	qat_Wind, //ruch pod wp³ywem wiatru
	qat_Sky, //animacja nieba
	qat_IK = 0x100, //odwrotna kinematyka - submodel steruj¹cy (np. staw skokowy)
	qat_IK11 = 0x101, //odwrotna kinematyka - submodel nadrzêdny do sterowango (np. stopa)
	qat_IK21 = 0x102, //odwrotna kinematyka - submodel nadrzêdny do sterowango (np. podudzie)
	qat_IK22 = 0x103, //odwrotna kinematyka - submodel nadrzêdny do nadrzêdnego sterowango (np. udo)
	qat_Undefined = 0x800000FF //animacja chwilowo nieokreœlona
};

class TObject3d;
class TSubObject;
class TSubObjectInfo;


class TQStringPack
{
	char *data;
	//+0 - 4 bajty: typ kromki
	//+4 - 4 bajty: d³ugoœæ ³¹cznie z nag³ówkiem
	//+8 - obszar ³añcuchów znakowych, ka¿dy zakoñczony zerem
	int *index;
	//+0 - 4 bajty: typ kromki
	//+4 - 4 bajty: d³ugoœæ ³¹cznie z nag³ówkiem
	//+8 - tabela indeksów
public:
	char* String(int n);
	char* StringAt(int n) { return data + 9 + n; };
	TQStringPack() { data = NULL; index = NULL; };
	void Init(char *d) { data = d; };
	void InitIndex(int *i) { index = i; };
};
class TSubObjectInfo
{//klasa z informacjami o submodelach, do tworzenia pliku binarnego
public:
	TSubObject *pSubModel; //wskaŸnik na submodel
	int iTransform; //numer transformu (-1 gdy brak)
	int iName; //numer nazwy
	int iTexture; //numer tekstury
	int iNameLen; //d³ugoœæ nazwy
	int iTextureLen; //d³ugoœæ tekstury
	int iNext, iChild; //numer nastêpnego i potomnego
	static int iTotalTransforms; //iloœæ transformów
	static int iTotalNames; //iloœæ nazw
	static int iTotalTextures; //iloœæ tekstur
	static int iCurrent; //aktualny obiekt
	static TSubObjectInfo* pTable; //tabele obiektów pomocniczych
	TSubObjectInfo()
	{
		pSubModel = NULL;
		iTransform = iName = iTexture = iNext = iChild = -1; //nie ma
		iNameLen = iTextureLen = 0;
	}
	void Reset()
	{
		pTable = this; //ustawienie wskaŸnika tabeli obiektów
		iTotalTransforms = iTotalNames = iTotalTextures = iCurrent = 0; //zerowanie liczników
	}
	~TSubObjectInfo() {};
};


class TSubObject
{//klasa submodelu - pojedyncza siatka, punkt œwietlny albo grupa punktów
	//Ra: ta klasa ma mieæ wielkoœæ 256 bajtów, aby pokry³a siê z formatem binarnym
	//Ra: nie przestawiaæ zmiennych, bo wczytuj¹ siê z pliku binarnego!
private:
	TSubObject *Next;
	TSubObject *Child;
	int eType; //Ra: modele binarne daj¹ wiêcej mo¿liwoœci ni¿ mesh z³o¿ony z trójk¹tów
	int iName; //numer ³añcucha z nazw¹ submodelu, albo -1 gdy anonimowy
public: //chwilowo
	TAnimationType b_Anim;
private:
	int iFlags; //flagi informacyjne:
	//bit  0: =1 faza rysowania zale¿y od wymiennej tekstury 0
	//bit  1: =1 faza rysowania zale¿y od wymiennej tekstury 1
	//bit  2: =1 faza rysowania zale¿y od wymiennej tekstury 2
	//bit  3: =1 faza rysowania zale¿y od wymiennej tekstury 3
	//bit  4: =1 rysowany w fazie nieprzezroczystych (sta³a tekstura albo brak)
	//bit  5: =1 rysowany w fazie przezroczystych (sta³a tekstura)
	//bit  7: =1 ta sama tekstura, co poprzedni albo nadrzêdny
	//bit  8: =1 wierzcho³ki wyœwietlane z indeksów
	//bit  9: =1 wczytano z pliku tekstowego (jest w³aœcicielem tablic)
	//bit 13: =1 wystarczy przesuniêcie zamiast mno¿enia macierzy (trzy jedynki)
	//bit 14: =1 wymagane przechowanie macierzy (animacje)
	//bit 15: =1 wymagane przechowanie macierzy (transform niejedynkowy)
	union
	{//transform, nie ka¿dy submodel musi mieæ
		float4x4 *fMatrix; //pojedyncza precyzja wystarcza
		//matrix4x4 *dMatrix; //do testu macierz podwójnej precyzji
		int iMatrix; //w pliku binarnym jest numer matrycy
	};
	int iNumVerts; //iloœæ wierzcho³ków (1 dla FreeSpotLight)
	int iVboPtr; //pocz¹tek na liœcie wierzcho³ków albo indeksów
	int iTexture; //numer nazwy tekstury, -1 wymienna, 0 brak
	float fVisible; //próg jasnoœci œwiat³a do za³¹czenia submodelu
	float fLight; //próg jasnoœci œwiat³a do zadzia³ania selfillum
	float f4Ambient[4];
	float f4Diffuse[4]; //float ze wzglêdu na glMaterialfv()
	float f4Specular[4];
	float f4Emision[4];
	float fWireSize; //nie u¿ywane, ale wczytywane
	float fSquareMaxDist;
	float fSquareMinDist;
	//McZapkie-050702: parametry dla swiatla:
	float fNearAttenStart;
	float fNearAttenEnd;
	int bUseNearAtten;      //te 3 zmienne okreslaja rysowanie aureoli wokol zrodla swiatla
	int iFarAttenDecay;      //ta zmienna okresla typ zaniku natezenia swiatla (0:brak, 1,2: potega 1/R)
	float fFarDecayRadius;  //normalizacja j.w.
	float fCosFalloffAngle; //cosinus k¹ta sto¿ka pod którym widaæ œwiat³o
	float fCosHotspotAngle; //cosinus k¹ta sto¿ka pod którym widaæ aureolê i zwiêkszone natê¿enie œwiat³a
	float fCosViewAngle;    //cos kata pod jakim sie teraz patrzy
	//Ra: dalej s¹ zmienne robocze, mo¿na je przestawiaæ z zachowaniem rozmiaru klasy
	int TextureID; //numer tekstury, -1 wymienna, 0 brak
	int bWire; //nie u¿ywane, ale wczytywane
	//short TexAlpha;  //Ra: nie u¿ywane ju¿
	GLuint uiDisplayList; //roboczy numer listy wyœwietlania
	float Opacity; //nie u¿ywane, ale wczytywane
	//ABu: te same zmienne, ale zdublowane dla Render i RenderAlpha,
	//bo sie chrzanilo przemieszczanie obiektow.
	//Ra: ju¿ siê nie chrzani
	float f_Angle;
	float3 v_RotateAxis;
	float3 v_Angles;
public: //chwilowo
	float3 v_TransVector;
	float8 *Vertices; //roboczy wskaŸnik - wczytanie T3D do VBO
	int iAnimOwner; //roboczy numer egzemplarza, który ustawi³ animacjê
	TAnimationType b_aAnim; //kody animacji oddzielnie, bo zerowane
public:
	float4x4 *mAnimMatrix; //macierz do animacji kwaternionowych (nale¿y do AnimContainer)
	char space[8]; //wolne miejsce na przysz³e zmienne (zmniejszyæ w miarê potrzeby)
public:
	TSubObject **smLetter; //wskaŸnik na tablicê submdeli do generoania tekstu (docelowo zapisaæ do E3D)
	TSubObject *Parent; //nadrzêdny, np. do wymna¿ania macierzy
	int iVisible; //roboczy stan widocznoœci
	//std::string asTexture; //robocza nazwa tekstury do zapisania w pliku binarnym
	//std::string asName; //robocza nazwa
	char *pTexture; //robocza nazwa tekstury do zapisania w pliku binarnym
	char *pName; //robocza nazwa
private:
	//int SeekFaceNormal(DWORD *Masks, int f,DWORD dwMask,vector3 *pt,GLVERTEX *Vertices);
	int SeekFaceNormal(DWORD *Masks, int f, DWORD dwMask, float3 *pt, float8 *Vertices);
	void RaAnimation(TAnimationType a);
public:
	static int iInstance; //identyfikator egzemplarza, który aktualnie renderuje model
	static GLuint *ReplacableSkinId;
	static int iAlpha; //maska bitowa dla danego przebiegu
	static double fSquareDist;
	static TModel3d* pRoot;
	static std::string* pasText; //tekst dla wyœwietlacza (!!!! do przemyœlenia)
	TSubObject();
	~TSubObject();
	void FirstInit();
	int Load(cParser& Parser, TModel3d *Model, int Pos, bool dynamic);
	void ChildAdd(TSubObject *SubModel);
	void NextAdd(TSubObject *SubModel);
	TSubObject* NextGet() { return Next; };
	TSubObject* ChildGet() { return Child; };
	int TriangleAdd(TModel3d *m, int tex, int tri);
	float8* TrianglePtr(int tex, int pos, int *la, int *ld, int*ls);
	//float8* TrianglePtr(const char *tex,int tri);
	//void SetRotate(vector3 vNewRotateAxis,float fNewAngle);
	void SetRotate(float3 vNewRotateAxis, float fNewAngle);
	void SetRotateXYZ(vector3 vNewAngles);
	void SetRotateXYZ(float3 vNewAngles);
	void SetTranslate(vector3 vNewTransVector);
	void SetTranslate(float3 vNewTransVector);
	void SetRotateIK1(float3 vNewAngles);
  //TSubObject* GetFromName(std::string search, bool i = true);
	TSubObject* GetFromNameQ(char *search, bool i = true);
	void RenderDL();
	void RenderAlphaDL();
	void RenderVBO();
	void RenderAlphaVBO();
	//inline matrix4x4* GetMatrix() {return dMatrix;};
	inline float4x4* GetMatrix() { return fMatrix; };
	//matrix4x4* GetTransform() {return Matrix;};
	inline void Hide() { iVisible = 0; };
	void RaArrayFill(CVertNormTex *Vert);
	//void Render();
	int FlagsCheck();
	void WillBeAnimated() { if (this) iFlags |= 0x4000; };
	void InitialRotate(bool doit);
	void DisplayLists();
	void Info();
	void InfoSet(TSubObjectInfo *info);
	void BinInit(TSubObject *s, float4x4 *m, float8 *v, TQStringPack *t, TQStringPack *n = NULL, bool dynamic = false);
	void ReplacableSet(GLuint *r, int a)
	{
		ReplacableSkinId = r; iAlpha = a;
	};
	void TextureNameSet(const char *n);
	void NameSet(const char *n);
	//Ra: funkcje do budowania terenu z E3D
	int Flags() { return iFlags; };
	void UnFlagNext() { iFlags &= 0x00FFFFFF; };
	void ColorsSet(int *a, int *d, int*s);
	inline float3 Translation1Get()
	{
		return fMatrix ? *(fMatrix->TranslationGet()) + v_TransVector : v_TransVector;
	}
	inline float3 Translation2Get()
	{
		return *(fMatrix->TranslationGet()) + Child->Translation1Get();
	}
	void ParentMatrix(float4x4 *m);
	float MaxY(const float4x4 &m);
	void AdjustDist();
};


class TObject3d : public CMesh
{
private:
	TSubObject *Root; //drzewo submodeli
	int iFlags; //Ra: czy submodele maj¹ przezroczyste tekstury
public: 
	int iNumVerts; //iloœæ wierzcho³ków (gdy nie ma VBO, to m_nVertexCount=0)
private:
	TQStringPack Textures; //nazwy tekstur
	TQStringPack Names; //nazwy submodeli
	int *iModel; //zawartoœæ pliku binarnego
	int iSubModelsCount; //Ra: u¿ywane do tworzenia binarnych
	std::string asBinary; //nazwa pod któr¹ zapisaæ model binarny
public:
	inline TSubObject* GetSMRoot() { return(Root); };
	TObject3d();
    TObject3d(char *FileName);
	~TObject3d();
	TSubObject* GetFromNameQ(char *sName);
	TSubObject* AddToNamed(const char *Name, TSubObject *SubModel);
	void AddTo(TSubObject *tmp, TSubObject *SubModel);
	void LoadFromTextFile(std::string FileName, bool dynamic);
	void LoadFromBinFile(char *FileName, bool dynamic);
	bool LoadFromFile(std::string FileName, bool dynamic);
	void SaveToBinFile(char *FileName);
	void BreakHierarhy();
	//renderowanie specjalne
	void Render(double fSquareDistance, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RenderAlpha(double fSquareDistance, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RaRender(double fSquareDistance, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RaRenderAlpha(double fSquareDistance, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	//jeden k¹t obrotu
	void Render(vector3 pPosition, double fAngle = 0, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RenderAlpha(vector3 pPosition, double fAngle = 0, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RaRender(vector3 pPosition, double fAngle = 0, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RaRenderAlpha(vector3 pPosition, double fAngle = 0, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	//trzy k¹ty obrotu
	void Render(vector3* vPosition, vector3* vAngle, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RenderAlpha(vector3* vPosition, vector3* vAngle, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RaRender(vector3* vPosition, vector3* vAngle, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	void RaRenderAlpha(vector3* vPosition, vector3* vAngle, GLuint *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
	//inline int GetSubModelsCount() { return (SubModelsCount); };
	int Flags() { return iFlags; };
	void Init();
	char* NameGet() { return Root ? Root->pName : NULL; };
	int TerrainCount();
	TSubObject* TerrainSquare(int n);
	void TerrainRenderVBO(int n);
};




//---------------------------------------------------------------------------
#endif
