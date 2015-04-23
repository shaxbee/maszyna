//---------------------------------------------------------------------------

#ifndef segmentH
#define segmentH

#include "commons.h"
#include "commons_usr.h"
//#include "usefull.h"
//#include "Geometry.h"


//110405 Ra: klasa punktów przekroju z normalnymi

class vector6 : public vector3
{//punkt przekroju wraz z wektorem normalnym
public:
 vector3 n;
 __fastcall vector6()
 {x=y=z=n.x=n.z=0.0; n.y=1.0;};
 __fastcall vector6(double a,double b,double c,double d,double e,double f)
 //{x=a; y=b; z=c; n.x=d; n.y=e; n.z=f;};
 {x=a; y=b; z=c; n.x=0.0; n.y=1.0; n.z=0.0;}; //Ra: bo na razie s¹ z tym problemy
 __fastcall vector6(double a,double b,double c)
 {x=a; y=b; z=c; n.x=0.0; n.y=1.0; n.z=0.0;};
};

class TSegment
{
private:
    vector3 Point1,CPointOut,CPointIn,Point2;
	double fRoll1, fRoll2;
	double fLength;
	double *fTsBuffer;
    double fStep;
    vector3 GetFirstDerivative(double fTime);
	float RombergIntegral(double fA, double fB);
	float GetTFromS(double s);
public:
    bool bCurve;
    TSegment();
    ~TSegment() { SafeDeleteArray(fTsBuffer); };
    bool Init(vector3 NewPoint1, vector3 NewPoint2, double fNewStep,
                         double fNewRoll1= 0, double fNewRoll2= 0);
    bool Init(vector3 NewPoint1, vector3 NewCPointOut,
                         vector3 NewCPointIn, vector3 NewPoint2, double fNewStep,
                         double fNewRoll1= 0, double fNewRoll2= 0, bool bIsCurve= true);
    inline double ComputeLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2);  //McZapkie-150503
    inline vector3 GetDirection1() { return CPointOut-Point1; };
    inline vector3 GetDirection2() { return CPointIn-Point2; };
    vector3 GetDirection(double fDistance);
    vector3 GetDirection() { return CPointOut; };
    vector3 FastGetDirection(double fDistance, double fOffset);
    vector3 GetPoint(double fDistance);
    vector3 FastGetPoint(double t);
    inline vector3 FastGetPoint_0() {return Point1;};
    inline vector3 FastGetPoint_1() {return Point2;};
    inline double GetRoll(double s)
    {
        s/= fLength;
        return ((1.0-s)*fRoll1+s*fRoll2);
    }
    void GetRolls(double &r1,double &r2)
    {//pobranie przechy³ek (do generowania trójk¹tów)
        r1=fRoll1; r2=fRoll2;
    }
    void RenderLoft(const vector3 *ShapePoints, int iNumShapePoints,
    double fTextureLength, int iSkip=0, int iQualityFactor=1);
    void RenderSwitchRail(const vector3 *ShapePoints1, const vector3 *ShapePoints2,
                            int iNumShapePoints,double fTextureLength, int iSkip=0, double fOffsetX=0.0f);
    void Render();
    inline double GetLength() { return fLength; };
    void MoveMe(vector3 pPosition) { Point1+=pPosition; Point2+=pPosition; if(bCurve) {CPointIn+=pPosition; CPointOut+=pPosition;}}
};

//---------------------------------------------------------------------------
#endif
