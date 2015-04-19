//---------------------------------------------------------------------------
#ifndef Model3dH
#define Model3dH

#include <windows.h>
#include <gl/glew.h>
//#include "geometry.h"
#include "Parser.h"
#include "dumb3d.h"
//#include "ground.h"
//using namespace Math3D;

struct GLVERTEX
{
    vector3 Point;
    vector3 Normal;
    float tu,tv;
};

class TMaterialColor
{
public:
    __fastcall TMaterialColor() {};
    __fastcall TMaterialColor(char V)
    {
        r=g=b=V;
    };

    __fastcall TMaterialColor(char R, char G, char B)
    {
        r=R; g=G; b=B;
    };

    char r,g,b;
};


typedef enum { smt_Unknown, smt_Mesh, smt_Point, smt_FreeSpotLight, smt_Text} TSubModelType;

typedef enum { at_None, at_Rotate, at_RotateXYZ } TAnimType;

class TModel3d;

class TSubModel
{
  private:
      TSubModelType eType;
      GLuint TextureID;
	//  
      bool TexAlpha;        //McZapkie-141202: zeby bylo wiadomo czy sortowac ze wzgledu na przezroczystosc
//      bool TexHash;
      GLuint uiDisplayList;
      double Transparency;
      bool bWire;
      double fWireSize;
      double fSquareMaxDist;
      double fSquareMinDist;
  //McZapkie-050702: parametry dla swiatla:
      double fNearAttenStart;
      double fNearAttenEnd;
      bool bUseNearAtten;      //te 3 zmienne okreslaja rysowanie aureoli wokol zrodla swiatla
      int iFarAttenDecay;      //ta zmienna okresla typ zaniku natezenia swiatla (0:brak, 1,2: potega 1/R)
      double fFarDecayRadius;  //normalizacja j.w.
      double fcosFalloffAngle; //cosinus kata stozka pod ktorym widac swiatlo
      double fcosHotspotAngle; //cosinus kata stozka pod ktorym widac aureole i zwiekszone natezenie swiatla
      double fCosViewAngle;    //cos kata pod jakim sie teraz patrzy

      int Index;
      matrix4x4 Matrix;

      //ABu: te same zmienne, ale zdublowane dla Render i RenderAlpha,
      //bo sie chrzanilo przemieszczanie obiektow.

      double f_Angle, f_aAngle;
      vector3 v_RotateAxis, v_aRotateAxis;
      vector3 v_Angles, v_aAngles;
      double f_DesiredAngle, f_aDesiredAngle;
      double f_RotateSpeed, f_aRotateSpeed;
      vector3 v_TransVector, v_aTransVector;
      vector3 v_DesiredTransVector, v_aDesiredTransVector;
      double f_TranslateSpeed, f_aTranslateSpeed;
int iFlags; //flagi informacyjne:

      TSubModel *Next;
      TSubModel *Child;
  //    vector3 HitBoxPts[6];
      int __fastcall SeekFaceNormal(DWORD *Masks, int f, DWORD dwMask, vector3 pt, GLVERTEX *Vertices, int iNumVerts);
  public:
      GLuint TextureID2;
      TAnimType b_Anim, b_aAnim;
      int iTotVerts;
      bool Visible;
      std::string Name;
	  GLVERTEX *Vertices;
 static int iInstance; //identyfikator egzemplarza, który aktualnie renderuje model

      __fastcall TSubModel();
	  __fastcall ~TSubModel();
      bool __fastcall FirstInit();
      void __fastcall LoadT3D(cParser& Parser, int NIndex, TModel3d *Model);
	  void __fastcall LoadQ3D(int NIndex, TModel3d *Model);
      void __fastcall AddChild(TSubModel *SubModel);
      void __fastcall AddNext(TSubModel *SubModel);
      void __fastcall SetRotate(vector3 vNewRotateAxis, double fNewAngle);
      void __fastcall SetRotateXYZ(vector3 vNewAngles);
      void __fastcall SetTranslate(vector3 vNewTransVector);
      void __fastcall Render(GLuint ReplacableSkinId, bool recompile);
      void __fastcall ReCompile();
      void __fastcall RenderAlpha(GLuint ReplacableSkinId);      
      inline matrix4x4* __fastcall GetMatrix() { return &Matrix; };
      matrix4x4* __fastcall GetTransform();
      inline void __fastcall Hide() { Visible= false; };
	  TSubModel* __fastcall GetFromName(std::string search);
	  void __fastcall WillBeAnimated() {if (this) iFlags|=0x4000;};
  } ;


class TModel3d
{
private:
    int MaterialsCount;
    bool TractionPart;
    TSubModel *Root;
public:
    inline TSubModel* __fastcall GetSMRoot() {return(Root);};
    int SubModelsCount;
	int facescount;
	int iNumVerts;

	//GLVERTEX *Vertices;
    double Radius;
    __fastcall TModel3d();
    __fastcall TModel3d(char *FileName);
    __fastcall ~TModel3d();
	TSubModel* __fastcall GetFromName(std::string sName);
//    TMaterial* __fastcall GetMaterialFromName(char *sName);
    bool __fastcall AddTo(const char *Name, TSubModel *SubModel);
    bool __fastcall LoadFromQ3D(char *FileName, bool dynamic);
    bool __fastcall LoadFromTextFile(char *FileName, bool dynamic);
    bool __fastcall LoadFromFile(char *FileName, bool dynamic);
    void __fastcall SaveToFile(char *FileName);
    void __fastcall BreakHierarhy();
    void __fastcall Render( vector3 pPosition, double fAngle= 0, GLuint ReplacableSkinId= 0, float maxd=0);
    void __fastcall Render( double fSquareDistance, GLuint ReplacableSkinId= 0);
    void __fastcall RenderAlpha( vector3 pPosition, double fAngle= 0, GLuint ReplacableSkinId= 0);
    void __fastcall RenderAlpha( double fSquareDistance, GLuint ReplacableSkinId= 0);
    inline int __fastcall GetSubModelsCount() { return (SubModelsCount); };
};

typedef TModel3d *PModel3d;
//---------------------------------------------------------------------------
#endif
