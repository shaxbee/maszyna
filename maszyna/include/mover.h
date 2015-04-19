// MOVER.H 291 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifndef moverH
#define moverH
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>

#pragma hdrstop

//using namespace std;
//namespace Mover
//{
//-- type declarations -------------------------------------------------------
class  TMoverParameters;
typedef TMoverParameters* PMoverParameters;  // bylo **

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TLocation
{
	double X;
	double Y;
	double Z;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TRotation
{
	double Rx;
	double Ry;
	double Rz;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TDimension
{
	double W;
	double L;
	double H;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TCommand
{
	std::string Command;
	double Value1;
	double Value2;
	TLocation Location;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TTrackShape
{
	double R;
	double Len;
	double dHtrack;
	double dHrail;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TTrackParam
{
	double Width;
	double friction;
	int CategoryFlag;// bylo int
	int QualityFlag;// bylo int
	int DamageFlag;// bylo int
	double Velmax;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TTractionParam
{
	double TractionVoltage;
	double TractionFreq;
	double TractionMaxCurrent;
	double TractionResistivity;
} ;


enum TCouplerType { NoCoupler, Articulated, Bare, Chain, Screw, Automatic };

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TCoupling
{
	double SpringKB;
	double SpringKC;
	double beta;
	double DmaxB;
	double FmaxB;
	double DmaxC;
	double FmaxC;
	TCouplerType CouplerType;
	int CouplingFlag; // bylo int
	int AllowedFlag; // bylo int
	bool Render;
	double CoupleDist;
	TMoverParameters* Connected; // BYLY 2 **
	int ConnectedNr;  // bylo int
	double CForce;
	double Dist;
	bool CheckCollision;
} ;

typedef TCoupling TCouplers[2];
//typedef int TCouplerNr[2];//ABu: nr sprzegu z ktorym polaczony


enum TBrakeSystem { Individual, Pneumatic, ElectroPneumatic };
enum TBrakeSubsystem {ss_None, ss_W, ss_K, ss_KK, ss_Hik, ss_ESt, ss_KE, ss_LSt, ss_MT, ss_Dako};
enum TBrakeValve { NoValve, W, W_Lu_VI, W_Lu_L, W_Lu_XR, K, Kg, Kp, Kss, Kkg, Kkp, Kks, Hikg1, Hikss, Hikp1, KE, SW, EStED, NESt3, ESt3, LSt, ESt4, ESt3AL2, EP1, EP2, M483, CV1_L_TR, CV1, CV1_R, Other};
enum TBrakeHandle { NoHandle, West, FV4a, M394, M254, FVel1, FVel6, D2, Knorr, FD1, BS2, testH, St113};
    //typy hamulcow indywidualnych
enum TLocalBrake { NoBrake, ManualBrake, PneumaticBrake, HydraulicBrake };

typedef double TBrakeDelayTable[4];

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TBrakePressure
{
	double PipePressureVal;
	double BrakePressureVal;
	double FlowSpeedVal;
	TBrakeSystem BrakeType;
} ;

typedef TBrakePressure TBrakePressureTable[13];

enum TEngineTypes { None, Dumb, WheelsDriven, ElectricSeriesMotor, DieselEngine, SteamEngine, DieselElectric };
enum TPowerType { NoPower, BioPower, MechPower, ElectricPower, SteamPower };
enum TFuelType { Undefined, Coal, Oil };

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TGrateType
{
	TFuelType FuelType;
	double GrateSurface;
	double FuelTransportSpeed;
	double IgnitionTemperature;
	double MaxTemperature;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TBoilerType
{
	double BoilerVolume;
	double BoilerHeatSurface;
	double SuperHeaterSurface;
	double MaxWaterVolume;
	double MinWaterVolume;
	double MaxPressure;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TCurrentCollector
{
	double MinH;
	double MaxH;
	double CSW;
} ;

enum TPowerSource { NotDefined, InternalSource, Transducer, Generator, Accumulator, CurrentCollector, 
	PowerCable, Heater };

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TPowerParameters
{
	double MaxVoltage;
	double MaxCurrent;
	double IntR;
	TPowerSource SourceType;
	union
	{
		struct 
		{
			TGrateType Grate;
			TBoilerType Boiler;
			
		};
		struct 
		{
			TPowerType PowerTrans;
			double SteamPressure;
			
		};
		struct 
		{
			int CollectorsNo;
			TCurrentCollector CollectorParameters;
			
		};
		struct 
		{
			double MaxCapacity;
			TPowerSource RechargeSource;
			
		};
		struct 
		{
			TEngineTypes GeneratorEngine;
			
		};
		struct 
		{
			double InputVoltage;
			
		};
		struct 
		{
			TPowerType PowerType;
			
		};
		
	};
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TScheme
{
	int Relay;
	double R;
	int Bn;
	int Mn;
	bool AutoSwitch;
	int ScndAct;
} ;

typedef TScheme TSchemeTable[65];

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TDEScheme
{
	double RPM;
	double GenPower;
	double Umax;
	double Imax;
} ;

typedef TDEScheme TDESchemeTable[33];

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TShuntScheme
{
	double Umin;
	double Umax;
	double Pmin;
	double Pmax;
} ;

typedef TShuntScheme TShuntSchemeTable[33];

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TMPTRelay
{
	double Iup;
	double Idown;
} ;

typedef TMPTRelay TMPTRelayTable[8];

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TMotorParameters
{
	double mfi;
	double mIsat;
	double fi;
	double Isat;
	bool AutoSwitch;
} ;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
struct TSecuritySystem
{
	int SystemType;
	double AwareDelay;
	double SoundSignalDelay;
	double EmergencyBrakeDelay;
	int Status;
	double SystemTimer;
	double SystemSoundTimer;
	double SystemBrakeTimer;
	int VelocityAllowed;
	int NextVelocityAllowed;
	bool RadioStop;
} ;

struct mover__2
{
	int NToothM;
	int NToothW;
	double Ratio;
} ;


// GLOWNY OBIEKT ***********************************************************************************
class  TMoverParameters 
{
public:
	double dMoveLen;
	std::string filename;
	int CategoryFlag;                       // 1 - pociag, 2 - samochod, 4 - statek, 8 - samolot
	std::string TypeName;                   // nazwa serii/typu
	int TrainType;                          //typ: EZT/elektrowoz - Winger 040304
	TEngineTypes EngineType;                //typ napedu
	TPowerParameters EnginePowerSource;     //zrodlo mocy dla silnikow
	TPowerParameters SystemPowerSource;     //zrodlo mocy dla systemow sterowania/przetwornic/sprezarek
	TPowerParameters HeatingPowerSource;    //zrodlo mocy dla ogrzewania
	TPowerParameters AlterHeatPowerSource;  //alternatywne zrodlo mocy dla ogrzewania
	TPowerParameters LightPowerSource;      //zrodlo mocy dla oswietlenia
	TPowerParameters AlterLightPowerSource; //alternatywne mocy dla oswietlenia
	double Vmax;                            //max. predkosc kontrukcyjna, 
	double Mass;                            //masa wlasna,
	double Power;                           //moc
	double Mred;                            //Ra: zredukowane masy wiruj¹ce; potrzebne do obliczeñ hamowania
	double TotalMass;                       //wyliczane przez ComputeMass
	double HeatingPower;                    //moc pobierana na ogrzewanie
	double LightPower;                      //moc pobierana na oswietlenie
	int BatteryVoltage;                     //Winger - baterie w elektrykach}
	TDimension Dim;                         //wymiary
	double Cx;                              //wsp. op. aerodyn.
	double WheelDiameter;                   //srednica kol napednych
	double TrackW;                          //nominalna szerokosc toru [m]
	double AxleInertialMoment;              //moment bezwladnosci zestawu kolowego
	std::string AxleArangement;             //uklad osi np. Bo'Bo' albo 1'C
	int NPoweredAxles;                      //ilosc osi napednych liczona z powyzszego
	int NAxles;                             //ilosc wszystkich osi j.w.
	int BearingType;                        //lozyska: 0 - slizgowe, 1 - toczne
	double ADist;                           //odlegosc osi oraz czopow skretu
	double BDist;
	int NBpA;                               //ilosc el. ciernych na os: 0 1 2 lub 4
	int SandCapacity;                       //zasobnik piasku [kg]}
	TBrakeSystem BrakeSystem;               //rodzaj hamulca zespolonego
	TBrakeSubsystem BrakeSubsystem;
	double MBPM;                            //masa najwiekszego cisnienia
	TLocalBrake LocalBrake;                 //rodzaj hamulca indywidualnego
	TBrakePressure BrakePressureTable[13];  //wyszczegolnienie cisnien w rurze
	int ASBType;                            //0: brak hamulca przeciwposlizgowego, 1: reczny, 2: automat
	int TurboTest;
	double MaxBrakeForce;                   //maksymalna sila nacisku hamulca
	double MaxBrakePress;
	double P2FTrans;
	double TrackBrakeForce;                 //sila nacisku hamulca szynowego
	BYTE BrakeMethod;                        //flaga rodzaju hamulca
	double HighPipePress;                   //max. robocze cisnienie w przewodzie glownym
	double LowPipePress;                    //min. robocze cisnienie w przewodzie glownym 
	double DeltaPipePress;                  //roznica miedzy max i min
	double CntrlPipePress;                  //cisnienie z zbiorniku sterujacym
	double BrakeVolume;                     //pojemnosc powietrza w ukladzie hamulcowym, w ukladzie glownej sprezarki [m^3] }
	double BrakeVVolume;
	double VeselVolume;
	int BrakeCylNo;                         //ilosc cylindrow ham.
	double BrakeCylRadius;
	double BrakeCylDist;
	double BrakeCylMult[4];
	int BCMFlag;                            //promien cylindra, skok cylindra, przekladnia hamulcowa
	double MinCompressor;                   //cisnienie wlaczania, zalaczania sprezarki
	double MaxCompressor;
	double CompressorSpeed;                 //wydajnosc sprezarki
	double BrakeDelay[4];                   //opoznienie hamowania/odhamowania t/o
	int BrakeCtrlPosNo;                     //ilosc pozycji hamulca}
	int MainCtrlPosNo;                      //ilosc pozycji nastawnika}
	int ScndCtrlPosNo;                      //ilosc pozycji bocznika
	bool ScndInMain;                        //zaleznosc bocznika od nastawnika
	TSecuritySystem SecuritySystem;
	TScheme RList[65];                      //lista rezystorow rozruchowych i polaczen silnikow, dla dizla: napelnienia}
	int RlistSize;
	TMotorParameters MotorParam[9];         //rozne parametry silnika przy bocznikowaniach
	mover__2 Transmision;
	double NominalVoltage;                  //nominalne napiecie silnika
	double WindingRes;
	double CircuitRes;                      //rezystancje silnika i obwodu
	int IminLo;                             //prady przelacznika automatycznego rozruchu, uzywane tez przez ai_driver}
	int IminHi;
	int ImaxLo;                             //maksymalny prad niskiego i wysokiego rozruchu
	int ImaxHi;
	double nmax;                            //maksymalna dop. ilosc obrotow /s
	double InitialCtrlDelay;                //ile sek. opoznienia po wl. silnika
	double CtrlDelay;                       //ile sek. opoznienia miedzy kolejnymi poz.
	int AutoRelayType;                      //0 -brak, 1 - jest, 2 - opcja
	bool CoupledCtrl;                       //czy mainctrl i scndctrl sa sprzezone
  //TCouplerNr CouplerNr[2];
	bool IsCoupled;                         //czy jest sprzezony ale jedzie z tylu
	int DynamicBrakeType;
	int RVentType;                          //0 - brak, 1 - jest, 2 - automatycznie wlaczany
	double RVentnmax;                       //maks. obroty wentylatorow oporow rozruchowych
	double RVentCutOff;                     //rezystancja wylaczania wentylatorow dla RVentType=2
	int CompressorPower;                    //0: bezp. z obwodow silnika, 1: z przetwornicy, reczne, 2: w przetwornicy, stale}
	int SmallCompressorPower;               //Winger ZROBIC
	double dizel_Mmax;
	double dizel_nMmax;
	double dizel_Mnmax;
	double dizel_nmax;
	double dizel_nominalfill;
	double dizel_Mstand;
	double dizel_nmax_cutoff;
	double dizel_nmin;
	double dizel_minVelfullengage;
	double dizel_AIM;
	double dizel_engageDia;
	double dizel_engageMaxForce;
	double dizel_engagefriction;
	double AnPos;                           // pozycja sterowania dokladnego (analogowego)
	bool AnalogCtrl;
	bool AnMainCtrl;
	bool ShuntModeAllow;
	bool ShuntMode;
	bool Flat;
	double Vhyp;
	TDEScheme DElist[33];
	double Vadd;
	TMPTRelay MPTRelay[8];
	int RelayType;
	TShuntScheme SST[33];
	double PowerCorRatio;                   //Wspolczynnik korekcyjny    
	double Ftmax;
	int MaxLoad;                            //masa w T lub ilosc w sztukach - ladownosc
	std::string LoadAccepted;               //co moze byc zaladowane, 
	std::string LoadQuantity;               //jednostki miary ladunku
	double OverLoadFactor;                  //ile razy moze byc przekroczona ladownosc}
	double LoadSpeed;                       //szybkosc naladunku jednostki/s}
	double UnLoadSpeed;                     //szybkosc rozladunku jednostki/s}
	int DoorOpenCtrl;                       //0: przez pasazera, 1: przez maszyniste, 2: samoczynne (zamykanie)
	int DoorCloseCtrl;
	double DoorStayOpen;                    //jak dlugo otwarte w przypadku DoorCloseCtrl=2
	bool DoorClosureWarning;                //czy jest ostrzeganie przed zamknieciem
	double DoorOpenSpeed;                   //predkosc otwierania 
	double DoorCloseSpeed;                  //i zamykania w j.u. 
	double DoorMaxShiftL;                   //szerokosc otwarcia lub kat
	double DoorMaxShiftR;
	int DoorOpenMethod;                     //sposob otwarcia - 1: przesuwne, 2: obrotowe

	TLocation Loc;
	TRotation Rot;
	std::string Name;                       //nazwa wlasna
	TCoupling Couplers[2];                  //urzadzenia zderzno-sprzegowe, polaczenia miedzy wagonami}
	int ScanCounter;                        //pomocnicze do skanowania sprzegow
	bool EventFlag;                         //!o True jesli cos nietypowego sie wydarzy
	int SoundFlag;                          //!o patrz stale sound_ 
	double DistCounter;                     //! licznik kilometrow 
    double DistCounterD;                    //LICZNIK DNIOWY 
    double DriveTime;                       //CZAS JAZDY NALICZANY W SEKUNDACH W WORLD.CPP 
    double MaxVelo;                         //NAJWIEKSZA UZYSKANA PREDKOSC 
	double V;                               //predkosc w m/s
	double Vel;                             //predkosc w km/h
	double AccS;                            //przyspieszenie styczne
	double AccN;                            //przyspieszenie normalne w m/s^2
	double AccV;
	double nrot;                            //! rotacja kol [obr/s]
	double EnginePower;                     //! chwilowa moc silnikow 
	double dL;                              //przesuniecie pojazdu 
	double Fb;                              //sila hamowania
	double Ff;                              //sila tarcia
	double FTrain;                          //! sila pociagowa
	double FStand;                          // sila oporow ruchu
	double FTotal;                          //! calkowita sila dzialajaca na pojazd
	double UnitBrakeForce;                  //!s si³a hamowania przypadaj¹ca na jeden element
	bool SlippingWheels;                    //! poslizg kol,
	bool SandDose;                          //sypanie piasku
	double Sand;                            //ilosc piasku
	double BrakeSlippingTimer;              //pomocnicza zmienna do wylaczania przeciwposlizgu
	double dpBrake;
	double dpPipe;
	double dpMainValve;
	double dpLocalValve;
	double ScndPipePress;                   //cisnienie w przewodzie zasilajacym
	double BrakePress;                      //!o cisnienie w cylindrach hamulcowych
	double LocBrakePress;                   //!o cisnienie w cylindrach hamulcowych z pomocniczego
	double PipeBrakePress;                  //!o cisnienie w cylindrach hamulcowych z przewodu
	double PipePress;                       //!o cisnienie w przewodzie glownym
	double PPP;                             //!o poprzednie cisnienie w przewodzie glownym
	double Volume;                          //objetosc spr. powietrza w zbiorniku hamulca
	double CompressedVolume;                //objetosc spr. powietrza w ukl. zasilania
	double PantVolume;                      //objetosc spr. powietrza w ukl. pantografu
	double Compressor;                      //! cisnienie w ukladzie zasilajacym
	bool CompressorFlag;                    //!o czy wlaczona sprezarka
	bool PantCompFlag;                      //!o czy wlaczona sprezarka pantografow
	bool CompressorAllow;                   //! zezwolenie na uruchomienie sprezarki  NBMX
	bool ConverterFlag;                     //!  czy wlaczona przetwornica NBMX
	bool ConverterAllow;                    //zezwolenie na prace przetwornicy NBMX
	int BrakeCtrlPos;                       //nastawa hamulca zespolonego
	int LocalBrakePos;                      //nastawa hamulca indywidualnego
	int BrakeStatus;                        //0 - odham, 1 - ham., 2 - uszk., 4 - odluzniacz, 8 - antyposlizg, 16 - uzyte EP, 32 - pozycja R, 64 - powrot z R}
	bool EmergencyBrakeFlag;                //hamowanie nagle
	int BrakeDelayFlag;                     //nastawa opoznienia ham. osob/towar/posp/exp 0/1/2/4
	int BrakeDelays;                        //nastawy mozliwe do uzyskania
	bool DynamicBrakeFlag;                  //czy wlaczony hamulec elektrodymiczny
	double LimPipePress;                    //stabilizator cisnienia
	double ActFlowSpeed;                    //szybkosc stabilizatora
	int DamageFlag;                         //kombinacja bitowa stalych dtrain_* 
	int DerailReason;                       //przyczyna wykolejenia
	TCommand CommandIn;
	std::string CommandOut;
	std::string CommandLast;                //Ra: ostatnio wykonana komenda do podgl¹du
	double ValueOut;                        //argument komendy która ma byc przekazana na zewnatrz
	TTrackShape RunningShape;               //geometria toru po ktorym jedzie pojazd
	TTrackParam RunningTrack;               //parametry toru po ktorym jedzie pojazd
	double OffsetTrackH;                    //przesuniecie poz. i pion. w/m toru
	double OffsetTrackV;
	bool Mains;                             //polozenie glownego wylacznika
	int MainCtrlPos;                        //polozenie glownego nastawnika
	int ScndCtrlPos;                        //polozenie dodatkowego nastawnika
	int ActiveDir;                          //czy lok. jest wlaczona i w ktorym kierunku:
	int CabNo;                              //! numer kabiny: 1 lub -1. W przeciwnym razie brak sterowania - rozrzad
	int DirAbsolute;                        //zadany kierunek jazdy wzglêdem sprzêgów (1=w strone 0,-1=w stronê 1)
	int ActiveCab;                          //! numer kabiny, w ktorej sie jest
	int LastCab;                            //numer kabiny przed zmiana 
	double LastSwitchingTime;               //czas ostatniego przelaczania czegos
	int WarningSignal;                      //0: nie trabi, 1,2: trabi
	bool DepartureSignal;                   //sygnal odjazdu
	bool InsideConsist;
	TTractionParam RunningTraction;         //parametry sieci trakcyjnej najblizej lokomotywy
	double enrot;                           //ilosc obrotow
	double Im;                              //prad silnika
	double Itot;                            //prad calkowity
	double Mm;
	double Mw;
	double Fw;
	double Ft;
	int Imin;                               //prad przelaczania automatycznego rozruchu
	int Imax;                               //prad bezpiecznika
	double Voltage;                         //aktualne napiecie sieci zasilajacej
	int MainCtrlActualPos;                  //wskaznik Rlist
	int ScndCtrlActualPos;                  //wskaznik MotorParam
	bool DelayCtrlFlag;                     //opoznienie w zalaczaniu
	double LastRelayTime;                   //czas ostatniego przelaczania stycznikow
	bool AutoRelayFlag;                     //mozna zmieniac jesli AutoRelayType=2
	bool FuseFlag;                          //!o bezpiecznik nadmiarowy
	bool StLinFlag;                         //!o styczniki liniowe
	bool ResistorsFlag;                     //!o jazda rezystorowa
	double RventRot;                        //!s obroty wentylatorow rozruchowych
	bool UnBrake;                           //w EZT - nacisniete odhamowywanie

	//-zmienne dla lokomotywy spalinowej z przekladnia mechaniczna
	double dizel_fill;
	double dizel_engagestate;
	double dizel_engage;
	double dizel_automaticgearstatus;
	bool dizel_enginestart;
	double dizel_engagedeltaomega;

	//-zmienne dla drezyny recznej
	double PulseForce;                      //przylozona sila
	double PulseForceTimer;
	int PulseForceCount;

	double eAngle;
	int Load;                               //masa w T lub ilosc w sztukach - zaladowane
	std::string LoadType;                   //co jest zaladowane
	int LoadStatus;                         //+1=trwa rozladunek,+2=trwa zaladunek,+4=zakoñczono,0=zaktualizowany model
	double LastLoadChangeTime;              //ostatni raz (roz)³adowania
	bool DoorLeftOpened;                    //stan drzwi lewych
	bool DoorRightOpened;                   //stan drzwi prawych
	bool PantFrontUp;
	bool PantRearUp;
	bool PantFrontSP;
	bool PantRearSP;
	int PantFrontStart;
	int PantRearStart;
	bool PantFrontVolt;
	bool PantRearVolt;
	std::string PantSwitchType;
	std::string ConvSwitchType;
	bool Heating;                           //ogrzewanie 'Winger 020304
	int DoubleTr;                           //trakcja ukrotniona - przedni pojazd 'Winger 160304
	bool PhysicActivation;
	double FrictConst1;
	double FrictConst2s;
	double FrictConst2d;
	double TotalMassxg;                     //masa calkowita pomnozona przez przyspieszenie ziemskie

	std::string brakesystem_str;
	std::string brakesubsystem_str;

	// FUNKCJE I PROCEDURY ***************************************************************************
	bool __fastcall GetTrainsetVoltage(void);
	bool __fastcall Physic_ReActivation(void);
	void __fastcall PantCheck(void);
	double __fastcall LocalBrakeRatio(void);
	double __fastcall PipeRatio(void);
	double __fastcall RealPipeRatio(void);
	double __fastcall BrakeVP(void);
	bool __fastcall DynamicBrakeSwitch(bool Switch);
	bool __fastcall SendCtrlBroadcast(std::string CtrlCommand, double ctrlvalue);
	bool __fastcall SendCtrlToNext(std::string CtrlCommand, double ctrlvalue, double dir);
	bool __fastcall CabActivisation(void);
	bool __fastcall CabDeactivisation(void);
	bool __fastcall IncMainCtrl(int CtrlSpeed);
	bool __fastcall DecMainCtrl(int CtrlSpeed);
	bool __fastcall IncScndCtrl(int CtrlSpeed);
	bool __fastcall DecScndCtrl(int CtrlSpeed);
	bool __fastcall AddPulseForce(int Multipler);
	bool __fastcall SandDoseOn(void);
	void __fastcall SecuritySystemRESET();
	bool __fastcall SecuritySystemReset(void);
	void __fastcall SecuritySystemCheck(double dt);
	bool __fastcall IncBrakeLevel(void);
	bool __fastcall DecBrakeLevel(void);
	bool __fastcall IncLocalBrakeLevel(int CtrlSpeed);
	bool __fastcall DecLocalBrakeLevel(int CtrlSpeed);
	bool __fastcall IncLocalBrakeLevelFAST(void);
	bool __fastcall DecLocalBrakeLevelFAST(void);
	bool __fastcall EmergencyBrakeSwitch(bool Switch);
	bool __fastcall AntiSlippingBrake(void);
	bool __fastcall BrakeReleaser(void);
	bool __fastcall SwitchEPBrake(int state);
	bool __fastcall AntiSlippingButton(void);
	bool __fastcall IncBrakePress(double &brake, double PressLimit, double dp);
	bool __fastcall DecBrakePress(double &brake, double PressLimit, double dp);
	bool __fastcall BrakeDelaySwitch(int BDS);
	bool __fastcall IncBrakeMult(void);
	bool __fastcall DecBrakeMult(void);
	void __fastcall UpdateBrakePressure(double dt);
	void __fastcall UpdatePipePressure(double dt);
	void __fastcall CompressorCheck(double dt);
	void __fastcall UpdatePantVolume(double dt);
	void __fastcall UpdateScndPipePressure(double dt);
	bool __fastcall Attach(int ConnectNo, int ConnectToNr, PMoverParameters ConnectTo, int CouplingType);
	bool __fastcall DettachDistance(int ConnectNo);
	bool __fastcall Dettach(int ConnectNo);
	void __fastcall ComputeConstans(void);
	void __fastcall SetCoupleDist(void);
	double __fastcall ComputeMass(void);
	double __fastcall Adhesive(double staticfriction);
	double __fastcall TractionForce(double dt);
	double __fastcall FrictionForce(double R, int TDamage);
	double __fastcall BrakeForce(const TTrackParam &Track);
	double __fastcall CouplerForce(int CouplerN, double dt);
	void __fastcall CollisionDetect(int CouplerN, double dt);
	double __fastcall ComputeRotatingWheel(double WForce, double dt, double n);
	bool __fastcall SetInternalCommand(std::string NewCommand, double NewValue1, double NewValue2);
	double __fastcall GetExternalCommand(std::string &Command);
	bool __fastcall RunCommand(std::string command, double CValue1, double CValue2);
	bool __fastcall RunInternalCommand(void);
	void __fastcall PutCommand(std::string NewCommand, double NewValue1, double NewValue2, const TLocation &NewLocation);
	bool __fastcall DirectionForward(void);
	bool __fastcall DirectionBackward(void);
	bool __fastcall MainSwitch(bool State);
	bool __fastcall ChangeCab(int direction);
	bool __fastcall ConverterSwitch(bool State);
	bool __fastcall CompressorSwitch(bool State);
	void __fastcall ConverterCheck(void);
	bool __fastcall FuseOn(void);
	bool __fastcall FuseFlagCheck(void);
	void __fastcall FuseOff(void);
	int __fastcall ShowCurrent(int AmpN);
	int __fastcall ShowEngineRotation(int VehN);
	double __fastcall v2n(void);
	double __fastcall Current(double n, double U);
	double __fastcall Momentum(double I);
	bool __fastcall CutOffEngine(void);
	bool __fastcall MaxCurrentSwitch(bool State);
	bool __fastcall ResistorsFlagCheck(void);
	bool __fastcall MinCurrentSwitch(bool State);
	bool __fastcall AutoRelaySwitch(bool State);
	bool __fastcall AutoRelayCheck(void);
	bool __fastcall dizel_EngageSwitch(double state);
	bool __fastcall dizel_EngageChange(double dt);
	bool __fastcall dizel_AutoGearCheck(void);
	double __fastcall dizel_fillcheck(int mcp);
	double __fastcall dizel_Momentum(double dizel_fill, double n, double dt);
	bool __fastcall dizel_Update(double dt);
	bool __fastcall LoadingDone(double LSpeed, std::string LoadInit);
	void __fastcall ComputeTotalForce(double dt, double dt1, bool FullVer);
	double __fastcall ComputeMovement(double dt, double dt1, const TTrackShape &Shape, TTrackParam &Track
		, TTractionParam &ElectricTraction, const TLocation &NewLoc, TRotation &NewRot);
	double __fastcall FastComputeMovement(double dt, const TTrackShape &Shape, TTrackParam &Track, const 
		TLocation &NewLoc, TRotation &NewRot);
	bool __fastcall ChangeOffsetH(double DeltaOffset);
	__fastcall TMoverParameters(const TLocation &LocInitial, const TRotation &RotInitial, double VelInitial
		, std::string TypeNameInit, std::string NameInit, int LoadInitial, std::string LoadTypeInitial, int Cab);
	bool __fastcall LoadChkFile(std::string chkpath);
	bool __fastcall CheckLocomotiveParameters(bool ReadyFlag, int Dir);
	std::string __fastcall EngineDescription(int what);
	bool __fastcall DoorLeft(bool State);
	bool __fastcall DoorRight(bool State);
	bool __fastcall PantFront(bool State);
	bool __fastcall PantRear(bool State);
	bool __fastcall ReadBPT(std::string name, int size);
    bool __fastcall ReadMPT(std::string name, int size);
	bool __fastcall ReadRES(std::string name, int size);
	// Q
	TBrakeSystem getbrakesystem(std::string brakesys);
	TBrakeSubsystem getbraketype(std::string braketype);
	TLocalBrake getlocalbrake(std::string xstr);
	int getdynamicbrake(std::string dynamicbrake);
	int getbrakedelays(std::string xstr, int &oBrakeDelays);
	TPowerSource getpowersource(std::string xstr);
	bool gettransmission(std::string trans);
public:

	/* TObject.Create */ inline __fastcall TMoverParameters(void){ }

	/* TObject.Destroy */ inline __fastcall ~TMoverParameters(void) { }

};


//-- var, const, procedure ---------------------------------------------------

static const bool Go = true;
static const bool Hold = false;
static const short int ResArraySize = 0x40;
static const short int MotorParametersArraySize = 0x8;
static const short int maxcc = 0x4;
static const short int LocalBrakePosNo = 0xa;
static const short int MainBrakeMaxPos = 0xa;
static const short int dtrack_railwear = 0x2;
static const short int dtrack_freerail = 0x4;
static const short int dtrack_thinrail = 0x8;
static const short int dtrack_railbend = 0x10;
static const short int dtrack_plants = 0x20;
static const short int dtrack_nomove = 0x40;
static const int dtrack_norail = 0x80;
static const short int dtrain_thinwheel = 0x1;
static const short int dtrain_loadshift = 0x1;
static const short int dtrain_wheelwear = 0x2;
static const short int dtrain_bearing = 0x4;
static const short int dtrain_coupling = 0x8;
static const short int dtrain_ventilator = 0x10;
static const short int dtrain_loaddamage = 0x10;
static const short int dtrain_engine = 0x20;
static const short int dtrain_loaddestroyed = 0x20;
static const short int dtrain_axle = 0x40;
static const int dtrain_out = 0x80;
#define p_elengproblem  (1.000000E-02)
#define p_elengdamage  (1.000000E-01)
#define p_coupldmg  (2.000000E-02)
#define p_derail  (1.000000E-03)
#define p_accn  (1.000000E-01)
#define p_slippdmg  (1.000000E-03)
static const short int ctrain_virtual = 0x0;
static const short int ctrain_coupler = 0x1;
static const short int ctrain_pneumatic = 0x2;
static const short int ctrain_controll = 0x4;
static const short int ctrain_power = 0x8;
static const short int ctrain_passenger = 0x10;
static const short int ctrain_scndpneumatic = 0x20;
static const short int ctrain_heating = 0x40;
static const short int dbrake_none = 0x0;
static const short int dbrake_passive = 0x1;
static const short int dbrake_switch = 0x2;
static const short int dbrake_reversal = 0x4;
static const short int dbrake_automatic = 0x8;
static const short int bdelay_P = 0x0;
static const short int bdelay_G = 0x1;
static const short int bdelay_R = 0x2;
static const short int bdelay_E = 0x4;
static const short int b_off = 0x0;
static const short int b_on = 0x1;
static const short int b_dmg = 0x2;
static const short int b_release = 0x4;
static const short int b_antislip = 0x8;
static const short int b_epused = 0x10;
static const short int b_Rused = 0x20;
static const short int b_Ractive = 0x40;
static const short int bp_classic = 0x0;
static const short int bp_diameter = 0x1;
static const short int bp_magnetic = 0x2;
static const short int s_waiting = 0x1;
static const short int s_aware = 0x2;
static const short int s_active = 0x4;
static const short int s_alarm = 0x8;
static const short int s_ebrake = 0x10;
static const short int sound_none = 0x0;
static const short int sound_loud = 0x1;
static const short int sound_couplerstretch = 0x2;
static const short int sound_bufferclamp = 0x4;
static const short int sound_bufferbump = 0x8;
static const short int sound_relay = 0x10;
static const short int sound_manyrelay = 0x20;
static const short int sound_brakeacc = 0x40;

#define Spg  (5.067000E-01)
extern bool PhysicActivationFlag;
const short int dt_Default = 0x0;
const short int dt_EZT = 0x1;
const short int dt_ET41 = 0x2;
const short int dt_ET42 = 0x4;
const short int dt_PseudoDiesel = 0x8;
const short int dt_ET22 = 0x10;
const short int dt_SN61 = 0x20;
const short int dt_181 = 0x40;
extern double __fastcall Distance(const TLocation &Loc1, const TLocation &Loc2, const TDimension &Dim1, const TDimension &Dim2);


//} // NAMESPACE END



#endif
