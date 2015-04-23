//---------------------------------------------------------------------------

#ifndef trackfollowerH
#define trackfollowerH

#include "Track.h"
#include "segment.h"

inline bool iSetFlag(int Flag, int Value)
{
bool ret = false;
	if (Value > 0) 
		if ((Flag && Value) == 0)
		{
		Flag = Flag + Value;
		ret = true; //true, gdy by³o wczeœniej 0 i zosta³o ustawione
		}
	if (Value < 0 )
		if ((Flag && abs(Value)) == abs(Value))
		{
		Flag = Flag + Value; //Value jest ujemne, czyli zerowanie flagi
		ret = true; //true, gdy by³o wczeœniej 1 i zosta³o wyzerowane
		}
	return ret;
}


class TDynamicObject;

class TTrackFollower
{//oœ poruszaj¹ca siê po torze
private:
 TTrack *pCurrentTrack; //na którym torze siê znajduje
 TSegment *pCurrentSegment; //zwrotnice mog¹ mieæ dwa segmenty
 double fCurrentDistance; //przesuniêcie wzglêdem Point1 w stronê Point2
 double fDirection; //ustawienie wzglêdem toru: -1.0 albo 1.0, mno¿one przez dystans
 bool ComputatePosition(); //przeliczenie pozycji na torze
 TDynamicObject *Owner; //pojazd posiadaj¹cy
 int iEventFlag; //McZapkie-020602: informacja o tym czy wyzwalac zdarzenie: 0,1,2,3
 int iEventallFlag;

public:
 double fOffsetH; //Ra: odleg³oœæ œrodka osi od osi toru (dla samochodów)
 vector3 pPosition; //wspó³rzêdne XYZ w uk³adzie scenerii
 vector3 vAngles; //x:przechy³ka, y:pochylenie, z:kierunek w planie (w radianach)
 TTrackFollower();
 ~TTrackFollower();
 void SetCurrentTrack(TTrack *pTrack,int end);
 bool Move(double fDistance,bool bPrimary);
 inline TTrack* GetTrack() {return pCurrentTrack;};
 inline double GetRoll() {return vAngles.x;}; //przechy³ka policzona przy ustalaniu pozycji
 //{return pCurrentSegment->GetRoll(fCurrentDistance)*fDirection;}; //zamiast liczyæ mo¿na pobraæ
 inline double GetDirection() {return fDirection;}; //zwrot na torze
 inline double GetTranslation() {return fCurrentDistance;};  //ABu-030403
 //inline double GetLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2)
 //{ return pCurrentSegment->ComputeLength(p1,cp1,cp2,p2); };
 //inline double GetRadius(double L, double d);  //McZapkie-150503
 bool Init(TTrack *pTrack,TDynamicObject *NewOwner,double fDir);
 void Render(float fNr);
};
//---------------------------------------------------------------------------
#endif
