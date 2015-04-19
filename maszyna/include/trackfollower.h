//TRKFOLL.H 283

#ifndef TrackfollowerH
#define TrackfollowerH

#include "segment.h" 
#include "Track.h"

class TDynamicObject;

class TTrackFollower
{
private:
    TTrack *pCurrentTrack;
    TSegment *pCurrentSegment;
    double fCurrentDistance;
    double fDirection;
    bool ComputatePosition();
    TDynamicObject *Owner;
    int iEventFlag; //McZapkie-020602: informacja o tym czy wyzwalac zdarzenie: 0,1,2,3
    int iEventallFlag; 

public:
 vector3 pPosition; //wspó³rzêdne XYZ w uk³adzie scenerii
 vector3 vAngles; //x:przechy³ka, y:pochylenie, z:kierunek w planie (w radianach)
    TTrackFollower();
    ~TTrackFollower();
    inline void SetCurrentTrack(TTrack *pTrack)
    {
        pCurrentTrack= pTrack;
        pCurrentSegment= (pCurrentTrack?pCurrentTrack->CurrentSegment():NULL);
    };
    bool Move(double fDistance, bool bPrimary= false);
    inline TTrack* GetTrack() { return pCurrentTrack; };
//    inline double GetRoll() { return pCurrentSegment->GetRoll(fCurrentDistance)*fDirection; };
    inline double GetDirection() { return fDirection; };
    inline double GetTranslation() { return fCurrentDistance; };  //ABu-030403
//    inline double __fastcall GetLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2)
//    { return pCurrentSegment->ComputeLength(p1,cp1,cp2,p2); };
//    inline double __fastcall GetRadius(double L, double d);  //McZapkie-150503
    bool Init(TTrack *pTrack, TDynamicObject *NewOwner=NULL, double fDir=1.0f);
    bool Render();
};
//---------------------------------------------------------------------------
#endif
