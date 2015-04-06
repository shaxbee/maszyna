//---------------------------------------------------------------------------

#include "Mover.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
// Ra: tu należy przenosić funcje z mover.pas, które nie są z niego wywoływane.
// Jeśli jakieś zmienne nie są używane w mover.pas, też można je przenosić.
// Przeniesienie wszystkiego na raz zrobiło by zbyt wielki chaos do ogarnięcia.

const dEpsilon = 0.01; // 1cm (zależy od typu sprzęgu...)

__fastcall TMoverParameters::TMoverParameters(double VelInitial, AnsiString TypeNameInit,
                                              AnsiString NameInit, int LoadInitial,
                                              AnsiString LoadTypeInitial, int Cab)
    : T_MoverParameters(VelInitial, TypeNameInit, NameInit, LoadInitial, LoadTypeInitial, Cab)
{ // główny konstruktor
    DimHalf.x = 0.5 * Dim.W; // połowa szerokości, OX jest w bok?
    DimHalf.y = 0.5 * Dim.L; // połowa długości, OY jest do przodu?
    DimHalf.z = 0.5 * Dim.H; // połowa wysokości, OZ jest w górę?
    // BrakeLevelSet(-2); //Pascal ustawia na 0, przestawimy na odcięcie (CHK jest jeszcze nie
    // wczytane!)
    bPantKurek3 = true; // domyślnie zbiornik pantografu połączony jest ze zbiornikiem głównym
    iProblem = 0; // pojazd w pełni gotowy do ruchu
    iLights[0] = iLights[1] = 0; //światła zgaszone
};

double TMoverParameters::Distance(const TLocation &Loc1, const TLocation &Loc2,
                                             const TDimension &Dim1, const TDimension &Dim2)
{ // zwraca odległość pomiędzy pojazdami (Loc1) i (Loc2) z uwzględnieneim ich długości (kule!)
    return hypot(Loc2.X - Loc1.X, Loc1.Y - Loc2.Y) - 0.5 * (Dim2.L + Dim1.L);
};

double TMoverParameters::Distance(const vector3 &s1, const vector3 &s2,
                                             const vector3 &d1, const vector3 &d2){
    // obliczenie odległości prostopadłościanów o środkach (s1) i (s2) i wymiarach (d1) i (d2)
    // return 0.0; //będzie zgłaszać warning - funkcja do usunięcia, chyba że się przyda...
};

double TMoverParameters::CouplerDist(Byte Coupler)
{ // obliczenie odległości pomiędzy sprzęgami (kula!)
    return Couplers[Coupler].CoupleDist =
               Distance(Loc, Couplers[Coupler].Connected->Loc, Dim,
                        Couplers[Coupler].Connected->Dim); // odległość pomiędzy sprzęgami (kula!)
};

bool TMoverParameters::Attach(Byte ConnectNo, Byte ConnectToNr,
                                         TMoverParameters *ConnectTo, Byte CouplingType,
                                         bool Forced)
{ //łączenie do swojego sprzęgu (ConnectNo) pojazdu (ConnectTo) stroną (ConnectToNr)
    // Ra: zwykle wykonywane dwukrotnie, dla każdego pojazdu oddzielnie
    // Ra: trzeba by odróżnić wymóg dociśnięcia od uszkodzenia sprzęgu przy podczepianiu AI do
    // składu
    if (ConnectTo) // jeśli nie pusty
    {
        if (ConnectToNr != 2)
            Couplers[ConnectNo].ConnectedNr = ConnectToNr; // 2=nic nie podłączone
        TCouplerType ct = ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr]
                              .CouplerType; // typ sprzęgu podłączanego pojazdu
        Couplers[ConnectNo].Connected =
            ConnectTo; // tak podpiąć (do siebie) zawsze można, najwyżej będzie wirtualny
        CouplerDist(ConnectNo); // przeliczenie odległości pomiędzy sprzęgami
        if (CouplingType == ctrain_virtual)
            return false; // wirtualny więcej nic nie robi
        if (Forced ? true : ((Couplers[ConnectNo].CoupleDist <= dEpsilon) &&
                             (Couplers[ConnectNo].CouplerType != NoCoupler) &&
                             (Couplers[ConnectNo].CouplerType == ct)))
        { // stykaja sie zderzaki i kompatybilne typy sprzegow, chyba że łączenie na starcie
            if (Couplers[ConnectNo].CouplingFlag ==
                ctrain_virtual) // jeśli wcześniej nie było połączone
            { // ustalenie z której strony rysować sprzęg
                Couplers[ConnectNo].Render = true; // tego rysować
                ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Render = false; // a tego nie
            };
            Couplers[ConnectNo].CouplingFlag = CouplingType; // ustawienie typu sprzęgu
            // if (CouplingType!=ctrain_virtual) //Ra: wirtualnego nie łączymy zwrotnie!
            //{//jeśli łączenie sprzęgiem niewirtualnym, ustawiamy połączenie zwrotne
            ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag = CouplingType;
            ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Connected = this;
            ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CoupleDist =
                Couplers[ConnectNo].CoupleDist;
            return true;
            //}
            // podłączenie nie udało się - jest wirtualne
        }
    }
    return false; // brak podłączanego pojazdu, zbyt duża odległość, niezgodny typ sprzęgu, brak
                  // sprzęgu, brak haka
};

bool TMoverParameters::Attach(Byte ConnectNo, Byte ConnectToNr,
                                         T_MoverParameters *ConnectTo, Byte CouplingType,
                                         bool Forced)
{ //łączenie do (ConnectNo) pojazdu (ConnectTo) stroną (ConnectToNr)
    return Attach(ConnectNo, ConnectToNr, (TMoverParameters *)ConnectTo, CouplingType, Forced);
};

int TMoverParameters::DettachStatus(Byte ConnectNo)
{ // Ra: sprawdzenie, czy odległość jest dobra do rozłączania
    // powinny być 3 informacje: =0 sprzęg już rozłączony, <0 da się rozłączyć. >0 nie da się
    // rozłączyć
    if (!Couplers[ConnectNo].Connected)
        return 0; // nie ma nic, to rozłączanie jest OK
    if ((Couplers[ConnectNo].CouplingFlag & ctrain_coupler) == 0)
        return -Couplers[ConnectNo].CouplingFlag; // hak nie połączony - rozłączanie jest OK
    if (TestFlag(DamageFlag, dtrain_coupling))
        return -Couplers[ConnectNo].CouplingFlag; // hak urwany - rozłączanie jest OK
    // ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale
    // za to teraz dziala odczepianie... :) }
    // if (CouplerType==Articulated) return false; //sprzęg nie do rozpięcia - może być tylko urwany
    // Couplers[ConnectNo].CoupleDist=Distance(Loc,Couplers[ConnectNo].Connected->Loc,Dim,Couplers[ConnectNo].Connected->Dim);
    CouplerDist(ConnectNo);
    if (Couplers[ConnectNo].CouplerType == Screw ? Couplers[ConnectNo].CoupleDist < 0.0 : true)
        return -Couplers[ConnectNo].CouplingFlag; // można rozłączać, jeśli dociśnięty
    return (Couplers[ConnectNo].CoupleDist > 0.2) ? -Couplers[ConnectNo].CouplingFlag :
                                                    Couplers[ConnectNo].CouplingFlag;
};

bool TMoverParameters::Dettach(Byte ConnectNo)
{ // rozlaczanie
    if (!Couplers[ConnectNo].Connected)
        return true; // nie ma nic, to odczepiono
    // with Couplers[ConnectNo] do
    int i = DettachStatus(ConnectNo); // stan sprzęgu
    if (i < 0)
    { // gdy scisniete zderzaki, chyba ze zerwany sprzeg (wirtualnego nie odpinamy z drugiej strony)
        // Couplers[ConnectNo].Connected=NULL; //lepiej zostawic bo przeciez trzeba kontrolowac
        // zderzenia odczepionych
        Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag =
            0; // pozostaje sprzęg wirtualny
        Couplers[ConnectNo].CouplingFlag = 0; // pozostaje sprzęg wirtualny
        return true;
    }
    else if (i > 0)
    { // odłączamy węże i resztę, pozostaje sprzęg fizyczny, który wymaga dociśnięcia (z wirtualnym
      // nic)
        Couplers[ConnectNo].CouplingFlag &= ctrain_coupler;
        Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag =
            Couplers[ConnectNo].CouplingFlag;
    }
    return false; // jeszcze nie rozłączony
};

void TMoverParameters::SetCoupleDist()
{ // przeliczenie odległości sprzęgów
    if (Couplers[0].Connected)
    {
        CouplerDist(0);
        if (CategoryFlag & 2)
        { // Ra: dla samochodów zderzanie kul to za mało
        }
    }
    if (Couplers[1].Connected)
    {
        CouplerDist(1);
        if (CategoryFlag & 2)
        { // Ra: dla samochodów zderzanie kul to za mało
        }
    }
};

bool TMoverParameters::DirectionForward()
{
    if ((MainCtrlPosNo > 0) && (ActiveDir < 1) && (MainCtrlPos == 0))
    {
        ++ActiveDir;
        DirAbsolute = ActiveDir * CabNo;
        if (DirAbsolute)
            if (Battery) // jeśli bateria jest już załączona
                BatterySwitch(true); // to w ten oto durny sposób aktywuje się CA/SHP
        SendCtrlToNext("Direction", ActiveDir, CabNo);
        return true;
    }
    else if ((ActiveDir == 1) && (MainCtrlPos == 0) && (TrainType == dt_EZT))
        return MinCurrentSwitch(true); //"wysoki rozruch" EN57
    return false;
};

// Nastawianie hamulców

void TMoverParameters::BrakeLevelSet(double b)
{ // ustawienie pozycji hamulca na wartość (b) w zakresie od -2 do BrakeCtrlPosNo
    // jedyny dopuszczalny sposób przestawienia hamulca zasadniczego
    if (fBrakeCtrlPos == b)
        return; // nie przeliczać, jak nie ma zmiany
    fBrakeCtrlPos = b;
    BrakeCtrlPosR = b;
    if (fBrakeCtrlPos < Handle->GetPos(bh_MIN))
        fBrakeCtrlPos = Handle->GetPos(bh_MIN); // odcięcie
    else if (fBrakeCtrlPos > Handle->GetPos(bh_MAX))
        fBrakeCtrlPos = Handle->GetPos(bh_MAX);
    int x = floor(fBrakeCtrlPos); // jeśli odwołujemy się do BrakeCtrlPos w pośrednich, to musi być
                                  // obcięte a nie zaokrągone
    while ((x > BrakeCtrlPos) && (BrakeCtrlPos < BrakeCtrlPosNo)) // jeśli zwiększyło się o 1
        if (!T_MoverParameters::IncBrakeLevelOld())
            break; // wyjście awaryjne
    while ((x < BrakeCtrlPos) && (BrakeCtrlPos >= -1)) // jeśli zmniejszyło się o 1
        if (!T_MoverParameters::DecBrakeLevelOld())
            break;
    BrakePressureActual = BrakePressureTable[BrakeCtrlPos + 2]; // skopiowanie pozycji
    /*
    //youBy: obawiam sie, ze tutaj to nie dziala :P
    //Ra 2014-03: było tak zrobione, że działało - po każdej zmianie pozycji była wywoływana ta
    funkcja
    // if (BrakeSystem==Pneumatic?BrakeSubsystem==Oerlikon:false) //tylko Oerlikon akceptuje ułamki
     if(false)
      if (fBrakeCtrlPos>0.0)
      {//wartości pośrednie wyliczamy tylko dla hamowania
       double u=fBrakeCtrlPos-double(x); //ułamek ponad wartość całkowitą
       if (u>0.0)
       {//wyliczamy wartości ważone
        BrakePressureActual.PipePressureVal+=-u*BrakePressureActual.PipePressureVal+u*BrakePressureTable[BrakeCtrlPos+1+2].PipePressureVal;
        //BrakePressureActual.BrakePressureVal+=-u*BrakePressureActual.BrakePressureVal+u*BrakePressureTable[BrakeCtrlPos+1].BrakePressureVal;
    //to chyba nie będzie tak działać, zwłaszcza w EN57
        BrakePressureActual.FlowSpeedVal+=-u*BrakePressureActual.FlowSpeedVal+u*BrakePressureTable[BrakeCtrlPos+1+2].FlowSpeedVal;
       }
      }
    */
};

bool TMoverParameters::BrakeLevelAdd(double b)
{ // dodanie wartości (b) do pozycji hamulca (w tym ujemnej)
    // zwraca false, gdy po dodaniu było by poza zakresem
    BrakeLevelSet(fBrakeCtrlPos + b);
    return b > 0.0 ? (fBrakeCtrlPos < BrakeCtrlPosNo) :
                     (BrakeCtrlPos > -1.0); // true, jeśli można kontynuować
};

bool TMoverParameters::IncBrakeLevel()
{ // nowa wersja na użytek AI, false gdy osiągnięto pozycję BrakeCtrlPosNo
  return BrakeLevelAdd(1.0); };

bool TMoverParameters::DecBrakeLevel()
{ // nowa wersja na użytek AI, false gdy osiągnięto pozycję -1 return BrakeLevelAdd(-1.0); };

bool TMoverParameters::ChangeCab(int direction)
{ // zmiana kabiny i resetowanie ustawien
    if (abs(ActiveCab + direction) < 2)
    {
        //  if (ActiveCab+direction=0) then LastCab:=ActiveCab;
        ActiveCab = ActiveCab + direction;
        if ((BrakeSystem == Pneumatic) && (BrakeCtrlPosNo > 0))
        {
            //    if (BrakeHandle==FV4a)   //!!!POBIERAĆ WARTOŚĆ Z KLASY ZAWORU!!!
            //     BrakeLevelSet(-2); //BrakeCtrlPos=-2;
            //    else if ((BrakeHandle==FVel6)||(BrakeHandle==St113))
            //     BrakeLevelSet(2);
            //    else
            //     BrakeLevelSet(1);
            BrakeLevelSet(Handle->GetPos(bh_NP));
            LimPipePress = PipePress;
            ActFlowSpeed = 0;
        }
        else
            // if (TrainType=dt_EZT) and (BrakeCtrlPosNo>0) then
            //  BrakeCtrlPos:=5; //z Megapacka
            // else
            //    BrakeLevelSet(0); //BrakeCtrlPos=0;
            BrakeLevelSet(Handle->GetPos(bh_NP));
        //   if not TestFlag(BrakeStatus,b_dmg) then
        //    BrakeStatus:=b_off; //z Megapacka
        MainCtrlPos = 0;
        ScndCtrlPos = 0;
        // Ra: to poniżej jest bez sensu - można przejść nie wyłączając
        // if ((EngineType!=DieselEngine)&&(EngineType!=DieselElectric))
        //{
        // Mains=false;
        // CompressorAllow=false;
        // ConverterAllow=false;
        //}
        // ActiveDir=0;
        // DirAbsolute=0;
        return true;
    }
    return false;
};

bool TMoverParameters::CurrentSwitch(int direction)
{ // rozruch wysoki (true) albo niski (false)
    // Ra: przeniosłem z Train.cpp, nie wiem czy ma to sens
    if (MaxCurrentSwitch(direction))
    {
        if (TrainType != dt_EZT)
            return (MinCurrentSwitch(direction));
    }
    if (EngineType == DieselEngine) // dla 2Ls150
        if (ShuntModeAllow)
            if (ActiveDir == 0) // przed ustawieniem kierunku
                ShuntMode = direction;
    return false;
};

void TMoverParameters::UpdatePantVolume(double dt)
{ // KURS90 - sprężarka pantografów; Ra 2014-07: teraz jest to zbiornik rozrządu, chociaż to jeszcze
  // nie tak
    if (EnginePowerSource.SourceType == CurrentCollector) // tylko jeśli pantografujący
    {
        // Ra 2014-07: zasadniczo, to istnieje zbiornik rozrządu i zbiornik pantografów - na razie
        // mamy razem
        // Ra 2014-07: kurek trójdrogowy łączy spr.pom. z pantografami i wyłącznikiem ciśnieniowym
        // WS
        // Ra 2014-07: zbiornika rozrządu nie pompuje się tu, tylko pantografy; potem można zamknąć
        // WS i odpalić resztę
        if ((TrainType == dt_EZT) ? (PantPress < ScndPipePress) :
                                    bPantKurek3) // kurek zamyka połączenie z ZG
        { // zbiornik pantografu połączony ze zbiornikiem głównym - małą sprężarką się tego nie
          // napompuje
            // Ra 2013-12: Niebugocław mówi, że w EZT nie ma potrzeby odcinać kurkiem
            PantPress = EnginePowerSource.CollectorParameters
                            .MaxPress; // ograniczenie ciśnienia do MaxPress (tylko w pantografach!)
            if (PantPress > ScndPipePress)
                PantPress = ScndPipePress; // oraz do ScndPipePress
            PantVolume = (PantPress + 1) * 0.1; // objętość, na wypadek odcięcia kurkiem
        }
        else
        { // zbiornik główny odcięty, można pompować pantografy
            if (PantCompFlag && Battery) // włączona bateria i mała sprężarka
                PantVolume += dt * (TrainType == dt_EZT ? 0.003 : 0.005) *
                              (2 * 0.45 - ((0.1 / PantVolume / 10) - 0.1)) /
                              0.45; // napełnianie zbiornika pantografów
            // Ra 2013-12: Niebugocław mówi, że w EZT nabija 1.5 raz wolniej niż jak było 0.005
            PantPress = (10.0 * PantVolume) - 1.0; // tu by się przydała objętość zbiornika
        }
        if (!PantCompFlag && (PantVolume > 0.1))
            PantVolume -= dt * 0.0003; // nieszczelności: 0.0003=0.3l/s
        if (Mains) // nie wchodzić w funkcję bez potrzeby
            if (EngineType == ElectricSeriesMotor) // nie dotyczy... czego właściwie?
                if (PantPress < EnginePowerSource.CollectorParameters.MinPress)
                    if ((TrainType & (dt_EZT | dt_ET40 | dt_ET41 | dt_ET42)) ?
                            (GetTrainsetVoltage() < EnginePowerSource.CollectorParameters.MinV) :
                            true) // to jest trochę proteza; zasilanie członu może być przez sprzęg
                                  // WN
                        if (MainSwitch(false))
                            EventFlag = true; // wywalenie szybkiego z powodu niskiego ciśnienia
        if (TrainType != dt_EZT) // w EN57 pompuje się tylko w silnikowym
            // pierwotnie w CHK pantografy miały również rozrządcze EZT
            for (int b = 0; b <= 1; ++b)
                if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
                    if (Couplers[b].Connected->PantVolume <
                        PantVolume) // bo inaczej trzeba w obydwu członach przestawiać
                        Couplers[b].Connected->PantVolume =
                            PantVolume; // przekazanie ciśnienia do sąsiedniego członu
        // czy np. w ET40, ET41, ET42 pantografy członów mają połączenie pneumatyczne?
        // Ra 2014-07: raczej nie - najpierw się załącza jeden człon, a potem można podnieść w
        // drugim
    }
    else
    { // a tu coś dla SM42 i SM31, aby pokazywać na manometrze
        PantPress = CntrlPipePress;
    }
};

void TMoverParameters::UpdateBatteryVoltage(double dt)
{ // przeliczenie obciążenia baterii
    double sn1, sn2, sn3, sn4, sn5; // Ra: zrobić z tego amperomierz NN
    if ((BatteryVoltage > 0) && (EngineType != DieselEngine) && (EngineType != WheelsDriven) &&
        (NominalBatteryVoltage > 0))
    {
        if ((NominalBatteryVoltage / BatteryVoltage < 1.22) && Battery)
        { // 110V
            if (!ConverterFlag)
                sn1 = (dt * 2.0); // szybki spadek do ok 90V
            else
                sn1 = 0;
            if (ConverterFlag)
                sn2 = -(dt * 2.0); // szybki wzrost do 110V
            else
                sn2 = 0;
            if (Mains)
                sn3 = (dt * 0.05);
            else
                sn3 = 0;
            if (iLights[0] & 63) // 64=blachy, nie ciągną prądu //rozpisać na poszczególne
                                 // żarówki...
                sn4 = dt * 0.003;
            else
                sn4 = 0;
            if (iLights[1] & 63) // 64=blachy, nie ciągną prądu
                sn5 = dt * 0.001;
            else
                sn5 = 0;
        };
        if ((NominalBatteryVoltage / BatteryVoltage >= 1.22) && Battery)
        { // 90V
            if (PantCompFlag)
                sn1 = (dt * 0.0046);
            else
                sn1 = 0;
            if (ConverterFlag)
                sn2 = -(dt * 50); // szybki wzrost do 110V
            else
                sn2 = 0;
            if (Mains)
                sn3 = (dt * 0.001);
            else
                sn3 = 0;
            if (iLights[0] & 63) // 64=blachy, nie ciągną prądu
                sn4 = (dt * 0.0030);
            else
                sn4 = 0;
            if (iLights[1] & 63) // 64=blachy, nie ciągną prądu
                sn5 = (dt * 0.0010);
            else
                sn5 = 0;
        };
        if (!Battery)
        {
            if (NominalBatteryVoltage / BatteryVoltage < 1.22)
                sn1 = dt * 50;
            else
                sn1 = 0;
            sn2 = dt * 0.000001;
            sn3 = dt * 0.000001;
            sn4 = dt * 0.000001;
            sn5 = dt * 0.000001; // bardzo powolny spadek przy wyłączonych bateriach
        };
        BatteryVoltage -= (sn1 + sn2 + sn3 + sn4 + sn5);
        if (NominalBatteryVoltage / BatteryVoltage > 1.57)
            if (MainSwitch(false) && (EngineType != DieselEngine) && (EngineType != WheelsDriven))
                EventFlag = true; // wywalanie szybkiego z powodu zbyt niskiego napiecia
        if (BatteryVoltage > NominalBatteryVoltage)
            BatteryVoltage = NominalBatteryVoltage; // wstrzymanie ładowania pow. 110V
        if (BatteryVoltage < 0.01)
            BatteryVoltage = 0.01;
    }
    else if (NominalBatteryVoltage == 0)
        BatteryVoltage = 0;
    else
        BatteryVoltage = 90;
};

/* Ukrotnienie EN57:
1 //układ szeregowy
2 //układ równoległy
3 //bocznik 1
4 //bocznik 2
5 //bocznik 3
6 //do przodu
7 //do tyłu
8 //1 przyspieszenie
9 //minus obw. 2 przyspieszenia
10 //jazda na oporach
11 //SHP
12A //podnoszenie pantografu przedniego
12B //podnoszenie pantografu tylnego
13A //opuszczanie pantografu przedniego
13B //opuszczanie wszystkich pantografów
14 //załączenie WS
15 //rozrząd (WS, PSR, wał kułakowy)
16 //odblok PN
18 //sygnalizacja przetwornicy głównej
19 //luzowanie EP
20 //hamowanie EP
21 //rezerwa** (1900+: zamykanie drzwi prawych)
22 //zał. przetwornicy głównej
23 //wył. przetwornicy głównej
24 //zał. przetw. oświetlenia
25 //wył. przetwornicy oświetlenia
26 //sygnalizacja WS
28 //sprężarka
29 //ogrzewanie
30 //rezerwa* (1900+: zamykanie drzwi lewych)
31 //otwieranie drzwi prawych
32H //zadziałanie PN siln. trakcyjnych
33 //sygnał odjazdu
34 //rezerwa (sygnalizacja poślizgu)
35 //otwieranie drzwi lewych
ZN //masa
*/

double TMoverParameters::ComputeMovement(double dt, double dt1, const TTrackShape &Shape,
                                                    TTrackParam &Track,
                                                    TTractionParam &ElectricTraction,
                                                    const TLocation &NewLoc, TRotation &NewRot)
{ // trzeba po mału przenosić tu tę funkcję
    double d;
    T_MoverParameters::ComputeMovement(dt, dt1, Shape, Track, ElectricTraction, NewLoc, NewRot);
    if (Power > 1.0) // w rozrządczym nie (jest błąd w FIZ!) - Ra 2014-07: teraz we wszystkich
        UpdatePantVolume(dt); // Ra 2014-07: obsługa zbiornika rozrządu oraz pantografów

    if (EngineType == WheelsDriven)
        d = CabNo * dL; // na chwile dla testu
    else
        d = dL;
    DistCounter = DistCounter + fabs(dL) / 1000.0;
    dL = 0;

    // koniec procedury, tu nastepuja dodatkowe procedury pomocnicze

    // sprawdzanie i ewentualnie wykonywanie->kasowanie poleceń
    if (LoadStatus > 0) // czas doliczamy tylko jeśli trwa (roz)ładowanie
        LastLoadChangeTime = LastLoadChangeTime + dt; // czas (roz)ładunku
    RunInternalCommand();
    // automatyczny rozruch
    if (EngineType == ElectricSeriesMotor)
        if (AutoRelayCheck())
            SetFlag(SoundFlag, sound_relay);
    /*
     else                  {McZapkie-041003: aby slychac bylo przelaczniki w sterowniczym}
      if (EngineType=None) and (MainCtrlPosNo>0) then
       for b:=0 to 1 do
        with Couplers[b] do
         if TestFlag(CouplingFlag,ctrain_controll) then
          if Connected^.Power>0.01 then
           SoundFlag:=SoundFlag or Connected^.SoundFlag;
    */
    if (EngineType == DieselEngine)
        if (dizel_Update(dt))
            SetFlag(SoundFlag, sound_relay);
    // uklady hamulcowe:
    if (VeselVolume > 0)
        Compressor = CompressedVolume / VeselVolume;
    else
    {
        Compressor = 0;
        CompressorFlag = false;
    };
    ConverterCheck();
    if (CompressorSpeed > 0.0) // sprężarka musi mieć jakąś niezerową wydajność
        CompressorCheck(dt); //żeby rozważać jej załączenie i pracę
    UpdateBrakePressure(dt);
    UpdatePipePressure(dt);
    UpdateBatteryVoltage(dt);
    UpdateScndPipePressure(dt); // druga rurka, youBy
    // hamulec antypoślizgowy - wyłączanie
    if ((BrakeSlippingTimer > 0.8) && (ASBType != 128)) // ASBSpeed=0.8
        Hamulec->ASB(0);
    // SetFlag(BrakeStatus,-b_antislip);
    BrakeSlippingTimer = BrakeSlippingTimer + dt;
    // sypanie piasku - wyłączone i piasek się nie kończy - błędy AI
    // if AIControllFlag then
    // if SandDose then
    //  if Sand>0 then
    //   begin
    //     Sand:=Sand-NPoweredAxles*SandSpeed*dt;
    //     if Random<dt then SandDose:=false;
    //   end
    //  else
    //   begin
    //     SandDose:=false;
    //     Sand:=0;
    //   end;
    // czuwak/SHP
    // if (Vel>10) and (not DebugmodeFlag) then
    if (!DebugModeFlag)
        SecuritySystemCheck(dt1);
    return d;
};

double TMoverParameters::FastComputeMovement(double dt, const TTrackShape &Shape,
                                                        TTrackParam &Track, const TLocation &NewLoc,
                                                        TRotation &NewRot)
{ // trzeba po mału przenosić tu tę funkcję
    double d;
    T_MoverParameters::FastComputeMovement(dt, Shape, Track, NewLoc, NewRot);
    if (Power > 1.0) // w rozrządczym nie (jest błąd w FIZ!)
        UpdatePantVolume(dt); // Ra 2014-07: obsługa zbiornika rozrządu oraz pantografów
    if (EngineType == WheelsDriven)
        d = CabNo * dL; // na chwile dla testu
    else
        d = dL;
    DistCounter = DistCounter + fabs(dL) / 1000.0;
    dL = 0;

    // koniec procedury, tu nastepuja dodatkowe procedury pomocnicze

    // sprawdzanie i ewentualnie wykonywanie->kasowanie poleceń
    if (LoadStatus > 0) // czas doliczamy tylko jeśli trwa (roz)ładowanie
        LastLoadChangeTime = LastLoadChangeTime + dt; // czas (roz)ładunku
    RunInternalCommand();
    if (EngineType == DieselEngine)
        if (dizel_Update(dt))
            SetFlag(SoundFlag, sound_relay);
    // uklady hamulcowe:
    if (VeselVolume > 0)
        Compressor = CompressedVolume / VeselVolume;
    else
    {
        Compressor = 0;
        CompressorFlag = false;
    };
    ConverterCheck();
    if (CompressorSpeed > 0.0) // sprężarka musi mieć jakąś niezerową wydajność
        CompressorCheck(dt); //żeby rozważać jej załączenie i pracę
    UpdateBrakePressure(dt);
    UpdatePipePressure(dt);
    UpdateScndPipePressure(dt); // druga rurka, youBy
    UpdateBatteryVoltage(dt);
    // hamulec antyposlizgowy - wyłączanie
    if ((BrakeSlippingTimer > 0.8) && (ASBType != 128)) // ASBSpeed=0.8
        Hamulec->ASB(0);
    BrakeSlippingTimer = BrakeSlippingTimer + dt;
    return d;
};

double TMoverParameters::ShowEngineRotation(int VehN)
{ // pokazywanie obrotów silnika, również dwóch dalszych pojazdów (3×SN61)
    int b;
    switch (VehN)
    { // numer obrotomierza
    case 1:
        return fabs(enrot);
    case 2:
        for (b = 0; b <= 1; ++b)
            if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
                if (Couplers[b].Connected->Power > 0.01)
                    return fabs(Couplers[b].Connected->enrot);
        break;
    case 3: // to nie uwzględnia ewentualnego odwrócenia pojazdu w środku
        for (b = 0; b <= 1; ++b)
            if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
                if (Couplers[b].Connected->Power > 0.01)
                    if (TestFlag(Couplers[b].Connected->Couplers[b].CouplingFlag, ctrain_controll))
                        if (Couplers[b].Connected->Couplers[b].Connected->Power > 0.01)
                            return fabs(Couplers[b].Connected->Couplers[b].Connected->enrot);
        break;
    };
    return 0.0;
};

void TMoverParameters::ConverterCheck()
{ // sprawdzanie przetwornicy
    if (ConverterAllow && Mains)
        ConverterFlag = true;
    else
        ConverterFlag = false;
};

int TMoverParameters::ShowCurrent(Byte AmpN)
{ // odczyt amperażu
    switch (EngineType)
    {
    case ElectricInductionMotor:
        switch (AmpN)
        { // do asynchronicznych
        case 1:
            return WindingRes * Mm / Vadd;
        case 2:
            return dizel_fill * WindingRes;
        default:
            return T_MoverParameters::ShowCurrent(AmpN);
        }
        break;
    case DieselElectric:
        return fabs(Im);
        break;
    default:
        return T_MoverParameters::ShowCurrent(AmpN);
    }
};
