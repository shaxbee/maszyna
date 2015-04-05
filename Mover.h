#ifndef MoverH
#define MoverH

//---------------------------------------------------------------------------
// Ra: Niestety "_mover.hpp" się nieprawidłowo generuje - przekłada sobie TCoupling na sam koniec.
// Przy wszelkich poprawkach w "_mover.pas" trzeba skopiować ręcznie "_mover.hpp" do "mover.hpp" i
// poprawić błędy! Tak aż do wydzielnia TCoupling z Pascala do C++...
// Docelowo obsługę sprzęgów (łączenie, rozłączanie, obliczanie odległości, przesył komend)
// trzeba przenieść na poziom DynObj.cpp.
// Obsługę silniników też trzeba wydzielić do osobnego modułu, bo każdy osobno może mieć poślizg.

#include "dumb3d.h"
using namespace Math3D;

enum TProblem               // lista problemów taboru, które uniemożliwiają jazdę
{                           // flagi bitowe
    pr_Hamuje = 1,          // pojazd ma załączony hamulec lub zatarte osie
    pr_Pantografy = 2,      // pojazd wymaga napompowania pantografów
    pr_Ostatni = 0x80000000 // ostatnia flaga bitowa
};

class TMoverParameters
{ // Ra: wrapper na kod pascalowy, przejmujący jego funkcje
  public:
    vector3 vCoulpler[2]; // powtórzenie współrzędnych sprzęgów z DynObj :/
    vector3 DimHalf;      // połowy rozmiarów do obliczeń geometrycznych
    // int WarningSignal; //0: nie trabi, 1,2: trabi syreną o podanym numerze
    unsigned char WarningSignal; // tymczasowo 8bit, ze względu na funkcje w MTools
    double fBrakeCtrlPos;        // płynna nastawa hamulca zespolonego
    bool bPantKurek3; // kurek trójdrogowy (pantografu): true=połączenie z ZG, false=połączenie z
                      // małą sprężarką
    int iProblem;   // flagi problemów z taborem, aby AI nie musiało porównywać; 0=może jechać
    int iLights[2]; // bity zapalonych świateł tutaj, żeby dało się liczyć pobór prądu
  private:
    double CouplerDist(int Coupler);

  public:
    TMoverParameters(double VelInitial, std::string TypeNameInit, std::string NameInit,
                     int LoadInitial, std::string LoadTypeInitial, int Cab);
    // obsługa sprzęgów
    double Distance(const vector3 &Loc1, const vector3 &Loc2, const vector3 &Dim1,
                    const vector3 &Dim2);
    bool Attach(int ConnectNo, int ConnectToNr, TMoverParameters *ConnectTo, int CouplingType,
                bool Forced = false);
    int DettachStatus(int ConnectNo);
    bool Dettach(int ConnectNo);
    void SetCoupleDist();
    bool DirectionForward();
    void BrakeLevelSet(double b);
    bool BrakeLevelAdd(double b);
    bool IncBrakeLevel(); // wersja na użytek AI
    bool DecBrakeLevel();
    bool ChangeCab(int direction);
    bool CurrentSwitch(int direction);
    void UpdateBatteryVoltage(double dt);
    double ComputeMovement(double dt, double dt1, const TTrackShape &Shape, TTrackParam &Track,
                           TTractionParam &ElectricTraction, const TLocation &NewLoc,
                           TRotation &NewRot);
    double FastComputeMovement(double dt, const TTrackShape &Shape, TTrackParam &Track,
                               const TLocation &NewLoc, TRotation &NewRot);
    double ShowEngineRotation(int VehN);
    // double GetTrainsetVoltage(void);
    // bool Physic_ReActivation(void);
    // double LocalBrakeRatio(void);
    // double ManualBrakeRatio(void);
    // double PipeRatio(void);
    // double RealPipeRatio(void);
    // double BrakeVP(void);
    // bool DynamicBrakeSwitch(bool Switch);
    // bool SendCtrlBroadcast(std::string CtrlCommand, double ctrlvalue);
    // bool SendCtrlToNext(std::string CtrlCommand, double ctrlvalue, double dir);
    // bool CabActivisation(void);
    // bool CabDeactivisation(void);
    // bool IncMainCtrl(int CtrlSpeed);
    // bool DecMainCtrl(int CtrlSpeed);
    // bool IncScndCtrl(int CtrlSpeed);
    // bool DecScndCtrl(int CtrlSpeed);
    // bool AddPulseForce(int Multipler);
    // bool SandDoseOn(void);
    // bool SecuritySystemReset(void);
    // void SecuritySystemCheck(double dt);
    // bool BatterySwitch(bool State);
    // bool EpFuseSwitch(bool State);
    // bool IncBrakeLevelOld(void);
    // bool DecBrakeLevelOld(void);
    // bool IncLocalBrakeLevel(int CtrlSpeed);
    // bool DecLocalBrakeLevel(int CtrlSpeed);
    // bool IncLocalBrakeLevelFAST(void);
    // bool DecLocalBrakeLevelFAST(void);
    // bool IncManualBrakeLevel(int CtrlSpeed);
    // bool DecManualBrakeLevel(int CtrlSpeed);
    // bool EmergencyBrakeSwitch(bool Switch);
    // bool AntiSlippingBrake(void);
    // bool BrakeReleaser(int state);
    // bool SwitchEPBrake(int state);
    // bool AntiSlippingButton(void);
    // bool IncBrakePress(double &brake, double PressLimit, double dp);
    // bool DecBrakePress(double &brake, double PressLimit, double dp);
    // bool BrakeDelaySwitch(int BDS);
    // bool IncBrakeMult(void);
    // bool DecBrakeMult(void);
    // void UpdateBrakePressure(double dt);
    // void UpdatePipePressure(double dt);
    // void CompressorCheck(double dt);
    void UpdatePantVolume(double dt);
    // void UpdateScndPipePressure(double dt);
    // void UpdateBatteryVoltage(double dt);
    // double GetDVc(double dt);
    // void ComputeConstans(void);
    // double ComputeMass(void);
    // double Adhesive(double staticfriction);
    // double TractionForce(double dt);
    // double FrictionForce(double R, int TDamage);
    // double BrakeForce(const TTrackParam &Track);
    // double CouplerForce(int CouplerN, double dt);
    // void CollisionDetect(int CouplerN, double dt);
    // double ComputeRotatingWheel(double WForce, double dt, double n);
    // bool SetInternalCommand(std::string NewCommand, double NewValue1, double
    // NewValue2);
    // double GetExternalCommand(std::string &Command);
    // bool RunCommand(std::string command, double CValue1, double CValue2);
    // bool RunInternalCommand(void);
    // void PutCommand(std::string NewCommand, double NewValue1, double NewValue2, const
    // TLocation
    //	&NewLocation);
    // bool DirectionBackward(void);
    // bool MainSwitch(bool State);
    // bool ConverterSwitch(bool State);
    // bool CompressorSwitch(bool State);
    void ConverterCheck();
    // bool FuseOn(void);
    // bool FuseFlagCheck(void);
    // void FuseOff(void);
    int ShowCurrent(int AmpN);
    // double v2n(void);
    // double current(double n, double U);
    // double Momentum(double I);
    // double MomentumF(double I, double Iw, int SCP);
    // bool CutOffEngine(void);
    // bool MaxCurrentSwitch(bool State);
    // bool ResistorsFlagCheck(void);
    // bool MinCurrentSwitch(bool State);
    // bool AutoRelaySwitch(bool State);
    // bool AutoRelayCheck(void);
    // bool dizel_EngageSwitch(double state);
    // bool dizel_EngageChange(double dt);
    // bool dizel_AutoGearCheck(void);
    // double dizel_fillcheck(int mcp);
    // double dizel_Momentum(double dizel_fill, double n, double dt);
    // bool dizel_Update(double dt);
    // bool LoadingDone(double LSpeed, std::string LoadInit);
    // void ComputeTotalForce(double dt, double dt1, bool FullVer);
    // double ComputeMovement(double dt, double dt1, const TTrackShape &Shape,
    // TTrackParam &Track
    //	, TTractionParam &ElectricTraction, const TLocation &NewLoc, TRotation &NewRot);
    // double FastComputeMovement(double dt, const TTrackShape &Shape, TTrackParam
    // &Track, const
    //	TLocation &NewLoc, TRotation &NewRot);
    // bool ChangeOffsetH(double DeltaOffset);
    //__fastcall T_MoverParameters(double VelInitial, std::string TypeNameInit, std::string
    // NameInit,
    // int LoadInitial
    //	, std::string LoadTypeInitial, int Cab);
    // bool LoadChkFile(std::string chkpath);
    // bool CheckLocomotiveParameters(bool ReadyFlag, int Dir);
    // std::string EngineDescription(int what);
    // bool DoorLeft(bool State);
    // bool DoorRight(bool State);
    // bool DoorBlockedFlag(void);
    // bool PantFront(bool State);
    // bool PantRear(bool State);
    //
};

#endif
