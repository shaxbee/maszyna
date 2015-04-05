//---------------------------------------------------------------------------
#ifndef CameraH
#define CameraH

#include "dumb3d.h"
using namespace Math3D;

//---------------------------------------------------------------------------
enum TCameraType
{ // tryby pracy kamery
    tp_Follow, // jazda z pojazdem
    tp_Free, // stoi na scenerii
    tp_Satelite // widok z góry (nie używany)
};

class TCamera
{
  private:
    vector3 pOffset; // nie używane (zerowe)
  public: // McZapkie: potrzebuje do kiwania na boki
    double Pitch;
    double Yaw; // w środku: 0=do przodu; na zewnątrz: 0=na południe
    double Roll;
    TCameraType Type;
    vector3 Pos; // współrzędne obserwatora
    vector3 LookAt; // współrzędne punktu, na który ma patrzeć
    vector3 vUp;
    vector3 Velocity;
    vector3 OldVelocity; // lepiej usredniac zeby nie bylo rozbiezne przy malym FPS
    vector3 CrossPos;
    double CrossDist;
    void Init(vector3 NPos, vector3 NAngle);
    void Reset() { Pitch = Yaw = Roll = 0; };
    void OnCursorMove(double x, double y);
    void Update();
    vector3 GetDirection();
    // vector3 inline GetCrossPos() { return Pos+GetDirection()*CrossDist+CrossPos; };

    bool SetMatrix();
    void SetCabMatrix(vector3 &p);
    void RaLook();
    void Stop();
    // bool GetMatrix(matrix4x4 &Matrix);
    vector3 PtNext, PtPrev;
};
#endif
