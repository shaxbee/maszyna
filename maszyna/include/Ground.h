//---------------------------------------------------------------------------

#ifndef groundH
#define groundH

#include "../commons.h"
#include "../commons_usr.h"
#include "ResourceManager.h"
#include "VBO.h"
#include "dumb3d.h"

//#include "Track.h"
//#include "dumb3d.h"
//#include "Geometry.h"
//#include "QueryParserComp.h"
//#include "AnimModel.h"

//#include "DynObj.h"
//#include "Train.h"
//#include "Sound.h"
//#include "MemCell.h"
//#include "Traction.h"
//#include "EvLaunch.h"
//#include "TractionPower.h"
#include "mtable.h"
//#include "Geom.h"

#define SafeFree(a) if (a!=NULL) free(a)
#define SafeDelete(a) { delete (a); a=NULL; }
#define SafeDeleteArray(a) { delete[] (a); a=NULL; }

//#include "parser.h" //Tolaris-010603
//#include "ResourceManager.h"

using namespace Math3D;
const int TP_FWEE= 7000;
const int TP_MODEL= 1000;
const int TP_SEMAPHORE= 1002;
const int TP_DYNAMIC= 1004;
const int TP_SOUND= 1005;
const int TP_TRACK= 1006;
const int TP_GEOMETRY= 1007;
const int TP_MEMCELL= 1008;
const int TP_EVLAUNCH= 1009; //MC
const int TP_TRACTION= 1010;
const int TP_TRACTIONPOWERSOURCE= 1011; //MC

typedef int TGroundNodeType;

struct TGroundVertex
{
    vector3 Point;
    vector3 Normal;
    float tu,tv;
};



class TGroundNode: public Resource
{
private:
public:
    TDynamicObject *NearestDynObj;
    double DistToDynObj;

    TGroundNodeType iType;
    union
    {
        void *Pointer;
        TAnimModel *Model;
        TDynamicObject *DynamicObject;
        vector3 *Points;
        TTrack *pTrack;
        TGroundVertex *Vertices;
        TTraction *Traction;
   //     TMemCell *MemCell;
   //     TEventLauncher *EvLaunch;
 
    //    TTractionPowerSource *TractionPowerSource;
   //     TRealSound *pStaticSound;

    };
	std::string asName;
    union
    {
        int iNumVerts;
        int iNumPts;
        int iState;
    };
    vector3 pCenter; //œrodek do przydzielenia sektora

    double fAngle;
    double fSquareRadius;
    double fSquareMinRadius;
    GLuint TextureID;
    GLuint DisplayListID;
    bool TexAlpha;
    float fLineThickness; //McZapkie-120702: grubosc linii
//    int Status;  //McZapkie-170303: status dzwieku
    int Ambient[4],Diffuse[4],Specular[4];
    bool bVisible;
    bool bStatic;
    bool bAllocated;
    TGroundNode *Next; //lista wszystkich, ostatni na koñcu
    TGroundNode *Next2; //lista w sektorze
    __fastcall TGroundNode();
    __fastcall ~TGroundNode();
    bool __fastcall Init(int n);
    void __fastcall InitCenter();
    void __fastcall InitNormals();

    void __fastcall MoveMe(vector3 pPosition);

    //bool __fastcall Disable();
	inline TGroundNode* __fastcall Find(std::string &asNameToFind)
    {
        if (asNameToFind==asName) return this; else if (Next) return Next->Find(asNameToFind);
        return NULL;
    };
    inline TGroundNode* __fastcall Find(std::string &asNameToFind, TGroundNodeType iNodeType )
    {
        if ((iNodeType==iType) && (asNameToFind==asName))
            return this;
        else
            if (Next) return Next->Find(asNameToFind,iNodeType);
        return NULL;
    };

    void Compile();
    void Release();

    bool __fastcall GetTraction();
    bool __fastcall Render();
    bool __fastcall RenderAlpha(); //McZapkie-131202: dwuprzebiegowy rendering
};

class TSubRect
{
private:
public:
    TGroundNode* pRootNode;
    __fastcall TSubRect() { pRootNode=NULL; };
    __fastcall ~TSubRect() {  };
//    __fastcall ~TSubRect() { SafeDelete(pRootNode); };   /* TODO -cBUG : Attention, remember to delete those nodes */
    void __fastcall AddNode(TGroundNode *Node) { Node->Next2= pRootNode; pRootNode= Node; };
//    __fastcall Render() { if (pRootNode) pRootNode->Render(); };
};

const int iNumSubRects= 10;

class TGroundRect
{
private:
    TSubRect *pSubRects;
    void __fastcall Init() { pSubRects= new TSubRect[iNumSubRects*iNumSubRects]; };

public:
    __fastcall TGroundRect() { pSubRects=NULL; };
    __fastcall ~TGroundRect() { SafeDeleteArray(pSubRects); };

    TSubRect* __fastcall SafeGetRect( int iCol, int iRow) { if (!pSubRects) Init();  return pSubRects+iRow*iNumSubRects+iCol; };
    TSubRect* __fastcall FastGetRect( int iCol, int iRow) { return ( pSubRects ? pSubRects+iRow*iNumSubRects+iCol : NULL ); };
};

const int iNumRects= 500;
const double fHalfNumRects= iNumRects/2;

const int iTotalNumSubRects= iNumRects*iNumSubRects;
const double fHalfTotalNumSubRects= iTotalNumSubRects/2;

const double fSubRectSize= 100.0f;
const double fRectSize= fSubRectSize*iNumSubRects;


class TGround
{
public:
    TDynamicObject *LastDyn; //ABu: paskudnie, ale na bardzo szybko moze jakos przejdzie...
//    TTrain *pTrain;


    __fastcall TGround();
    __fastcall ~TGround();
    void __fastcall Free();
    bool __fastcall Init(LPSTR asFile);
	bool __fastcall LoadTrainset(std::string asTrainSetFile, std::string TN, std::string TT, double TD, double TV);
    bool __fastcall InitEvents();
    bool __fastcall InitTracks();
    bool __fastcall InitLaunchers();  
	bool __fastcall Include(cParser& parser); 
    TGroundNode* __fastcall FindTrack(vector3 Point, int &iConnection, TGroundNode *Exclude);
    TGroundNode* __fastcall CreateGroundNode();
    TGroundNode* __fastcall AddGroundNode(cParser* parser);
    bool __fastcall AddGroundNode(double x, double z, TGroundNode *Node)
    {
        TSubRect *tmp= GetSubRect(x,z);
        if (tmp)
        {
            tmp->AddNode(Node);
            return true;
        }
        else
            return false;
    };
//    bool __fastcall Include(TQueryParserComp *Parser);
    TGroundNode* __fastcall GetVisible( std::string asName );
	TGroundNode* __fastcall GetNode( std::string asName );
    bool __fastcall AddDynamic(TGroundNode *Node);
    void __fastcall MoveGroundNode(vector3 pPosition);
    bool __fastcall Update(double dt, int iter);
//--    bool __fastcall AddToQuery(TEvent *Event, TDynamicObject *Node);
    bool __fastcall GetTraction(vector3 pPosition, TDynamicObject *model);
    bool __fastcall Render(vector3 pPosition);
    bool __fastcall RenderAlpha(vector3 pPosition);
    bool __fastcall CheckQuery();

    TGroundNode* __fastcall FindGroundNode(std::string &asNameToFind );
	TGroundNode* __fastcall FindDynamic( std::string asNameToFind );


	inline TGroundNode* __fastcall FindGroundNode( std::string asNameToFind, TGroundNodeType iNodeType )
    {
        TGroundNode *Current;
        for (Current= RootNode; Current!=NULL; Current= Current->Next)
            if ((Current->iType==iNodeType) && (Current->asName==asNameToFind))
                return Current;
        return NULL;
    }

//Winger - to smierdzi
/*    inline TGroundNode* __fastcall FindTraction( TGroundNodeType iNodeType )
    {
        TGroundNode *Current;
        TGroundNode *CurrDynObj;
        char trrx, trry, trrz;
        for (Current= RootNode; Current!=NULL; Current= Current->Next)
            if (Current->iType==iNodeType) // && (Current->Points->x )
//              if
                {
                trrx= char(Current->Points->x);
                trry= char(Current->Points->y);
                trrz= char(Current->Points->z);
                if (trrx!=0)
                        {
                        WriteLog("Znalazlem trakcje, qrwa!", trrx + trry + trrz);
                        return Current;
                        }
                }
        return NULL;
    }
*/
    TSubRect* __fastcall GetSubRect(double x, double z) { return GetSubRect(GetColFromX(x),GetRowFromZ(z)); };
    TSubRect* __fastcall FastGetSubRect(double x, double z) { return FastGetSubRect(GetColFromX(x),GetRowFromZ(z)); };
    TSubRect* __fastcall GetSubRect(int iCol, int iRow);
    TSubRect* __fastcall FastGetSubRect(int iCol, int iRow);
    int __fastcall GetRowFromZ(double z) { return (z/fSubRectSize+fHalfTotalNumSubRects); };
    int __fastcall GetColFromX(double x) { return (x/fSubRectSize+fHalfTotalNumSubRects); };
//--    TEvent* __fastcall FindEvent(LPSTR asEventName);
    void __fastcall TrackJoin(TGroundNode *Current);
private:
    TGroundNode *RootNode; //lista wêz³ów
//    TGroundNode *FirstVisible,*LastVisible;
    TGroundNode *RootDynamic; //lista pojazdów

    TGroundRect Rects[iNumRects][iNumRects]; //mapa kwadratów kilometrowych

//--    TEvent *RootEvent; //lista zdarzeñ
//--    TEvent *QueryRootEvent,*tmpEvent,*tmp2Event,*OldQRE;

    void __fastcall OpenGLUpdate(HDC hDC);
//    TWorld World;

    int iNumNodes;
    vector3 pOrigin;
    vector3 aRotate;
    bool bInitDone;
};
//---------------------------------------------------------------------------
#endif
