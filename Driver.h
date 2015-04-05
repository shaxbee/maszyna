//---------------------------------------------------------------------------

#ifndef DriverH
#define DriverH

#include "Classes.h"
#include "dumb3d.h"
#include <fstream>
using namespace Math3D;

enum TOrders
{ // rozkazy dla AI
    Wait_for_orders = 0, // czekanie na dostarczenie następnych rozkazów
    // operacje tymczasowe
    Prepare_engine = 1, // włączenie silnika
    Release_engine = 2, // wyłączenie silnika
    Change_direction = 4, // zmiana kierunku (bez skanowania sygnalizacji)
    Connect = 8, // podłączanie wagonów (z częściowym skanowaniem sygnalizacji)
    Disconnect = 0x10, // odłączanie wagonów (bez skanowania sygnalizacji)
    // jazda
    Shunt = 0x20, // tryb manewrowy
    Obey_train = 0x40, // tryb pociągowy
    Jump_to_first_order = 0x60 // zapęlenie do pierwszej pozycji (po co?)
};

enum TMovementStatus
{ // flagi bitowe ruchu (iDrivigFlags)
    moveStopCloser = 1, // podjechać blisko W4 (nie podjeżdżać na początku ani po zmianie czoła)
    moveStopPoint = 2, // stawać na W4 (wyłączone podczas zmiany czoła)
    moveActive = 4, // pojazd jest załączony i skanuje
    movePress = 8, // dociskanie przy odłączeniu (zamiast zmiennej Prepare2press)
    moveConnect = 0x10, // jest blisko innego pojazdu i można próbować podłączyć
    movePrimary = 0x20, // ma priorytet w składzie (master)
    moveLate = 0x40, // flaga spóźnienia, włączy bardziej
    moveStopHere = 0x80, // nie podjeżdżać do semafora, jeśli droga nie jest wolna
    moveStartHorn = 0x100, // podawaj sygnał po podaniu wolnej drogi
    moveStartHornNow = 0x200, // podaj sygnał po odhamowaniu
    moveStartHornDone = 0x400, // podano sygnał po podaniu wolnej drogi
    moveOerlikons = 0x800, // skład wyłącznie z zaworami? Oerlikona
    moveIncSpeed = 0x1000, // załączenie jazdy (np. dla EZT)
    moveTrackEnd = 0x2000, // dalsza jazda do przodu trwale ograniczona (W5, koniec toru)
    moveSwitchFound = 0x4000, // na drodze skanowania do przodu jest rozjazd
    moveGuardSignal = 0x8000, // sygnał od kierownika (minął czas postoju)
    moveVisibility = 0x10000, // jazda na widoczność po przejechaniu S1 na SBL
    moveDoorOpened = 0x20000, // drzwi zostały otwarte - doliczyć czas na zamknięcie
    movePushPull = 0x40000 // zmiana czoła przez zmianę kabiny - nie odczepiać przy zmianie kierunku
};

enum TStopReason
{ // powód zatrzymania, dodawany do SetVelocity 0 - w zasadzie do usunięcia
    stopNone, // nie ma powodu - powinien jechać
    stopSleep, // nie został odpalony, to nie pojedzie
    stopSem, // semafor zamknięty
    stopTime, // czekanie na godzinę odjazdu
    stopEnd, // brak dalszej części toru
    stopDir, // trzeba stanąć, by zmienić kierunek jazdy
    stopJoin, // stoi w celu połączenia wagonów
    stopBlock, // przeszkoda na drodze ruchu
    stopComm, // otrzymano taką komendę (niewiadomego pochodzenia)
    stopOut, // komenda wyjazdu poza stację (raczej nie powinna zatrzymywać!)
    stopRadio, // komunikat przekazany radiem (Radiostop)
    stopExt, // komenda z zewnątrz
    stopError // z powodu błędu w obliczeniu drogi hamowania
};

enum TAction
{ // przechowanie aktualnego stanu AI od poprzedniego przebłysku świadomości
    actUnknown, // stan nieznany (domyślny na początku)
    actPantUp, // podnieś pantograf (info dla użytkownika)
    actConv, // załącz przetwornicę (info dla użytkownika)
    actCompr, // załącz sprężarkę (info dla użytkownika)
    actSleep, //śpi (wygaszony)
    actDrive, // jazda
    actGo, // ruszanie z miejsca
    actSlow, // przyhamowanie przed ograniczeniem
    sctStop, // hamowanie w celu precyzyjnego zatrzymania
    actIdle, // luzowanie składu przed odjazdem
    actRelease, // luzowanie składu po zmniejszeniu prędkości
    actConnect, // dojazd w celu podczepienia
    actWait, // czekanie na przystanku
    actReady, // zgłoszona gotowość do odjazdu od kierownika
    actEmergency, // hamowanie awaryjne
    actGoUphill, // ruszanie pod górę
    actTest, // hamowanie kontrolne (podczas jazdy)
    actTrial // próba hamulca (na postoju)
};

class TSpeedPos
{ // pozycja tabeli prędkości dla AI
  public:
    double fDist; // aktualna odległość (ujemna gdy minięte)
    double fVelNext; // prędkość obowiązująca od tego miejsca
    // double fAcc;
    int iFlags;
    // 1=istotny,2=tor,4=odwrotnie,8-zwrotnica (może się zmienić),16-stan
    // zwrotnicy,32-minięty,64=koniec,128=łuk
    // 0x100=event,0x200=manewrowa,0x400=przystanek,0x800=SBL,0x1000=wysłana komenda,0x2000=W5
    // 0x10000=zatkanie
    vector3 vPos; // współrzędne XYZ do liczenia odległości
    struct
    {
        TTrack *trTrack; // wskaźnik na tor o zmiennej prędkości (zwrotnica, obrotnica)
        TEvent *evEvent; // połączenie z eventem albo komórką pamięci
    };
    void CommandCheck();

  public:
    void Clear();
    bool Update(vector3 *p, vector3 *dir, double &len);
    bool Set(TEvent *e, double d);
    void Set(TTrack *t, double d, int f);
    AnsiString TableText();
};

//----------------------------------------------------------------------------
static const bool Aggressive = true;
static const bool Easyman = false;
static const bool AIdriver = true;
static const bool Humandriver = false;
static const int maxorders = 32; // ilość rozkazów w tabelce
static const int maxdriverfails = 4; // ile błędów może zrobić AI zanim zmieni nastawienie
extern bool WriteLogFlag; // logowanie parametrów fizycznych
//----------------------------------------------------------------------------

class TController
{
  private: // obsługa tabelki prędkości (musi mieć możliwość odhaczania stacji w rozkładzie)
    TSpeedPos *sSpeedTable; // najbliższe zmiany prędkości
    int iSpeedTableSize; // wielkość tabelki
    int iFirst; // aktualna pozycja w tabeli (modulo iSpeedTableSize)
    int iLast; // ostatnia wypełniona pozycja w tabeli <iFirst (modulo iSpeedTableSize)
    int iTableDirection; // kierunek zapełnienia tabelki względem pojazdu z AI
    double fLastVel; // prędkość na poprzednio sprawdzonym torze
    TTrack *tLast; // ostatni analizowany tor
    TEvent *eSignSkip; // można pominąć ten SBL po zatrzymaniu
  private: // parametry aktualnego składu
    double fLength; // długość składu (do wyciągania z ograniczeń)
    double fMass; // całkowita masa do liczenia stycznej składowej grawitacji
    double fAccGravity; // przyspieszenie składowej stycznej grawitacji
  public:
    TEvent *eSignNext; // sygnał zmieniający prędkość, do pokazania na [F2]
    AnsiString asNextStop; // nazwa następnego punktu zatrzymania wg rozkładu
    int iStationStart; // numer pierwszej stacji pokazywanej na podglądzie rozkładu
  private: // parametry sterowania pojazdem (stan, hamowanie)
    double fShuntVelocity; // maksymalna prędkość manewrowania, zależy m.in. od składu
    int iVehicles; // ilość pojazdów w składzie
    int iEngineActive; // ABu: Czy silnik byl juz zalaczony; Ra: postęp w załączaniu
    // vector3 vMechLoc; //pozycja pojazdu do liczenia odległości od semafora (?)
    bool Psyche;
    int iDrivigFlags; // flagi bitowe ruchu
    double fDriverBraking; // po pomnożeniu przez v^2 [km/h] daje ~drogę hamowania [m]
    double fDriverDist; // dopuszczalna odległość podjechania do przeszkody
    double fVelMax; // maksymalna prędkość składu (sprawdzany każdy pojazd)
    double fBrakeDist; // przybliżona droga hamowania
    double fAccThreshold; // próg opóźnienia dla zadziałania hamulca
  public:
    double fLastStopExpDist; // odległość wygasania ostateniego przystanku
    double ReactionTime; // czas reakcji Ra: czego i na co? świadomości AI
    double fBrakeTime; // wpisana wartość jest zmniejszana do 0, gdy ujemna należy zmienić nastawę
                       // hamulca
  private:
    double fReady; // poziom odhamowania wagonów
    bool Ready; // ABu: stan gotowosci do odjazdu - sprawdzenie odhamowania wagonow
    double LastUpdatedTime; // czas od ostatniego logu
    double ElapsedTime; // czas od poczatku logu
    double deltalog; // przyrost czasu
    double LastReactionTime;
    double fActionTime; // czas używany przy regulacji prędkości i zamykaniu drzwi
    TAction eAction; // aktualny stan
    bool HelpMeFlag; // wystawiane True jesli cos niedobrego sie dzieje
  public:
    bool AIControllFlag; // rzeczywisty/wirtualny maszynista
    int iRouteWanted; // oczekiwany kierunek jazdy (0-stop,1-lewo,2-prawo,3-prosto) np. odpala
                      // migacz lub czeka na stan zwrotnicy
  private:
    TDynamicObject *pVehicle; // pojazd w którym siedzi sterujący
    TDynamicObject *
        pVehicles[2]; // skrajne pojazdy w składzie (niekoniecznie bezpośrednio sterowane)
    TMoverParameters *mvControlling; // jakim pojazdem steruje (może silnikowym w EZT)
    TMoverParameters *mvOccupied; // jakim pojazdem hamuje
    Mtable::TTrainParameters *TrainParams; // rozkład jazdy zawsze jest, nawet jeśli pusty
    // int TrainNumber; //numer rozkladowy tego pociagu
    // AnsiString OrderCommand; //komenda pobierana z pojazdu
    // double OrderValue; //argument komendy
    int iRadioChannel; // numer aktualnego kanału radiowego
    TTextSound *tsGuardSignal; // komunikat od kierownika
    int iGuardRadio; // numer kanału radiowego kierownika (0, gdy nie używa radia)
  public:
    double AccPreferred; // preferowane przyspieszenie (wg psychiki kierującego, zmniejszana przy
                         // wykryciu kolizji)
    double AccDesired; // przyspieszenie, jakie ma utrzymywać (<0:nie przyspieszaj,<-0.1:hamuj)
    double VelDesired; // predkość, z jaką ma jechać, wynikająca z analizy tableki; <=VelSignal
    double fAccDesiredAv; // uśrednione przyspieszenie z kolejnych przebłysków świadomości, żeby
                          // ograniczyć migotanie
  private:
    double VelforDriver; // prędkość, używana przy zmianie kierunku (ograniczenie przy nieznajmości
                         // szlaku?)
    double VelSignal; // predkość zadawana przez semafor (funkcją SetVelocity())
    double VelLimit; // predkość zadawana przez event jednokierunkowego ograniczenia prędkości
                     // (PutValues albo komendą)
  public:
    double VelNext; // prędkość, jaka ma być po przejechaniu długości ProximityDist
  private:
    // double fProximityDist; //odleglosc podawana w SetProximityVelocity(); >0:przeliczać do
    // punktu, <0:podana wartość
  public:
    double
        ActualProximityDist; // odległość brana pod uwagę przy wyliczaniu prędkości i przyspieszenia
  private:
    vector3 vCommandLocation; // polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego
                              // odnosi sie komenda
    TOrders OrderList[maxorders]; // lista rozkazów
    int OrderPos, OrderTop; // rozkaz aktualny oraz wolne miejsce do wstawiania nowych
    std::ofstream LogFile; // zapis parametrow fizycznych
    std::ofstream AILogFile; // log AI
    bool MaxVelFlag;
    bool MinVelFlag;
    int iDirection; // kierunek jazdy względem sprzęgów pojazdu, w którym siedzi AI (1=przód,-1=tył)
    int iDirectionOrder; //żadany kierunek jazdy (służy do zmiany kierunku)
    int iVehicleCount; // ilość pojazdów do odłączenia albo zabrania ze składu (-1=wszystkie)
    int iCoupler; // maska sprzęgu, jaką należy użyć przy łączeniu (po osiągnięciu trybu Connect), 0
                  // gdy jazda bez łączenia
    int iDriverFailCount; // licznik błędów AI
    bool Need_TryAgain; // true, jeśli druga pozycja w elektryku nie załapała
    bool Need_BrakeRelease;

  public:
    double fMinProximityDist; // minimalna oległość do przeszkody, jaką należy zachować
    double fOverhead1; // informacja o napięciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
    double fOverhead2; // informacja o sposobie jazdy (-1=normalnie, 0=bez prądu, >0=z opuszczonym i
                       // ograniczeniem prędkości)
    int iOverheadZero; // suma bitowa jezdy bezprądowej, bity ustawiane przez pojazdy z
                       // podniesionymi pantografami
    int iOverheadDown; // suma bitowa opuszczenia pantografów, bity ustawiane przez pojazdy z
                       // podniesionymi pantografami
    double fVoltage; // uśrednione napięcie sieci: przy spadku poniżej wartości minimalnej opóźnić
                     // rozruch o losowy czas
  private:
    double fMaxProximityDist; // akceptowalna odległość stanięcia przed przeszkodą
    TStopReason eStopReason; // powód zatrzymania przy ustawieniu zerowej prędkości
    AnsiString VehicleName;
    double fVelPlus; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
    double fVelMinus; // margines obniżenia prędkości, powodujący załączenie napędu
    double fWarningDuration; // ile czasu jeszcze trąbić
    double fStopTime; // czas postoju przed dalszą jazdą (np. na przystanku)
    double WaitingTime; // zliczany czas oczekiwania do samoistnego ruszenia
    double WaitingExpireTime; // maksymlany czas oczekiwania do samoistnego ruszenia
    // TEvent* eSignLast; //ostatnio znaleziony sygnał, o ile nie minięty
  private: //---//---//---//---// koniec zmiennych, poniżej metody //---//---//---//---//
    void SetDriverPsyche();
    bool PrepareEngine();
    bool ReleaseEngine();
    bool IncBrake();
    bool DecBrake();
    bool IncSpeed();
    bool DecSpeed(bool force = false);
    void SpeedSet();
    void Doors(bool what);
    void RecognizeCommand(); // odczytuje komende przekazana lokomotywie
    void Activation(); // umieszczenie obsady w odpowiednim członie
    void ControllingSet(); // znajduje człon do sterowania
    void AutoRewident(); // ustawia hamulce w składzie
  public:
    Mtable::TTrainParameters *__fastcall Timetable() { return TrainParams; };
    void PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2,
                               const _mover::TLocation &NewLocation, TStopReason reason = stopComm);
    bool PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2,
                               const vector3 *NewLocation, TStopReason reason = stopComm);
    bool UpdateSituation(double dt); // uruchamiac przynajmniej raz na sekundę
    // procedury dotyczace rozkazow dla maszynisty
    void SetVelocity(double NewVel, double NewVelNext,
                                TStopReason r = stopNone); // uaktualnia informacje o prędkości
    bool SetProximityVelocity(
        double NewDist,
        double NewVelNext); // uaktualnia informacje o prędkości przy nastepnym semaforze
  public:
    void JumpToNextOrder();
    void JumpToFirstOrder();
    void OrderPush(TOrders NewOrder);
    void OrderNext(TOrders NewOrder);
    TOrders OrderCurrentGet();
    TOrders OrderNextGet();
    bool CheckVehicles(TOrders user = Wait_for_orders);

  private:
    void CloseLog();
    void OrderCheck();

  public:
    void OrdersInit(double fVel);
    void OrdersClear();
    void OrdersDump();
    TController(bool AI, TDynamicObject *NewControll, bool InitPsyche,
                           bool primary = true // czy ma aktywnie prowadzić?
                           );
    AnsiString OrderCurrent();
    void WaitingSet(double Seconds);

  private:
    AnsiString Order2Str(TOrders Order);
    void DirectionForward(bool forward);
    int OrderDirectionChange(int newdir, TMoverParameters *Vehicle);
    void Lights(int head, int rear);
    double Distance(vector3 &p1, vector3 &n, vector3 &p2);

  private: // Ra: metody obsługujące skanowanie toru
    TEvent *__fastcall CheckTrackEvent(double fDirection, TTrack *Track);
    bool TableCheckEvent(TEvent *e);
    bool TableAddNew();
    bool TableNotFound(TEvent *e);
    void TableClear();
    TEvent *__fastcall TableCheckTrackEvent(double fDirection, TTrack *Track);
    void TableTraceRoute(double fDistance, TDynamicObject *pVehicle = NULL);
    void TableCheck(double fDistance);
    TCommandType TableUpdate(double &fVelDes, double &fDist, double &fNext,
                                        double &fAcc);
    void TablePurger();

  private: // Ra: stare funkcje skanujące, używane do szukania sygnalizatora z tyłu
    bool BackwardTrackBusy(TTrack *Track);
    TEvent *__fastcall CheckTrackEventBackward(double fDirection, TTrack *Track);
    TTrack *__fastcall BackwardTraceRoute(double &fDistance, double &fDirection, TTrack *Track,
                                          TEvent *&Event);
    void SetProximityVelocity(double dist, double vel, const vector3 *pos);
    TCommandType BackwardScan();

  public:
    void PhysicsLog();
    AnsiString StopReasonText();
    ~TController();
    AnsiString NextStop();
    void TakeControl(bool yes);
    AnsiString Relation();
    AnsiString TrainName();
    int StationCount();
    int StationIndex();
    bool IsStop();
    bool Primary() { return this ? bool(iDrivigFlags & movePrimary) : false; };
    int inline DrivigFlags() { return iDrivigFlags; };
    void MoveTo(TDynamicObject *to);
    void DirectionInitial();
    AnsiString TableText(int i);
    int CrossRoute(TTrack *tr);
    void RouteSwitch(int d);
    AnsiString OwnerName();
};

#endif
