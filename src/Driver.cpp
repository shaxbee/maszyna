//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Driver.h"
#include <mtable.hpp>
#include "DynObj.h"
#include <math.h>
#include "Globals.h"
#include "Event.h"
#include "Ground.h"
#include "MemCell.h"
#include "World.h"
#include "dir.h"

#define LOGVELOCITY 0
#define LOGORDERS 0
#define LOGSTOPS 1
#define LOGBACKSCAN 0
#define LOGPRESS 0
/*

Moduł obsługujący sterowanie pojazdami (składami pociągów, samochodami).
Ma działać zarówno jako AI oraz przy prowadzeniu przez człowieka. W tym
drugim przypadku jedynie informuje za pomocą napisów o tym, co by zrobił
w tym pierwszym. Obejmuje zarówno maszynistę jak i kierownika pociągu
(dawanie sygnału do odjazdu).

Przeniesiona tutaj została zawartość ai_driver.pas przerobiona na C++.
Również niektóre funkcje dotyczące składów z DynObj.cpp.

Teoria jest wtedy kiedy wszystko wiemy, ale nic nie działa.
Praktyka jest wtedy, kiedy wszystko działa, ale nikt nie wie dlaczego.
Tutaj łączymy teorię z praktyką - tu nic nie działa i nikt nie wie dlaczego…

*/

// zrobione:
// 0. pobieranie komend z dwoma parametrami
// 1. przyspieszanie do zadanej predkosci, ew. hamowanie jesli przekroczona
// 2. hamowanie na zadanym odcinku do zadanej predkosci (ze stabilizacja przyspieszenia)
// 3. wychodzenie z sytuacji awaryjnych: bezpiecznik nadmiarowy, poslizg
// 4. przygotowanie pojazdu do drogi, zmiana kierunku ruchu
// 5. dwa sposoby jazdy - manewrowy i pociagowy
// 6. dwa zestawy psychiki: spokojny i agresywny
// 7. przejscie na zestaw spokojny jesli wystepuje duzo poslizgow lub wybic nadmiarowego.
// 8. lagodne ruszanie (przedluzony czas reakcji na 2 pierwszych nastawnikach)
// 9. unikanie jazdy na oporach rozruchowych
// 10. logowanie fizyki //Ra: nie przeniesione do C++
// 11. kasowanie czuwaka/SHP
// 12. procedury wspomagajace "patrzenie" na odlegle semafory
// 13. ulepszone procedury sterowania
// 14. zglaszanie problemow z dlugim staniem na sygnale S1
// 15. sterowanie EN57
// 16. zmiana kierunku //Ra: z przesiadką po ukrotnieniu
// 17. otwieranie/zamykanie drzwi
// 18. Ra: odczepianie z zahamowaniem i podczepianie
// 19. dla Humandriver: tasma szybkosciomierza - zapis do pliku!

// do zrobienia:
// 1. kierownik pociagu
// 2. madrzejsze unikanie grzania oporow rozruchowych i silnika
// 3. unikanie szarpniec, zerwania pociagu itp
// 4. obsluga innych awarii
// 5. raportowanie problemow, usterek nie do rozwiazania
// 7. samouczacy sie algorytm hamowania

// stałe
const double EasyReactionTime = 0.5; //[s] przebłyski świadomości dla zwykłej jazdy
const double HardReactionTime = 0.2;
const double EasyAcceleration = 0.5; //[m/ss]
const double HardAcceleration = 0.9;
const double PrepareTime = 2.0; //[s] przebłyski świadomości przy odpalaniu
bool WriteLogFlag = false;

AnsiString StopReasonTable[] = { // przyczyny zatrzymania ruchu AI
    "", // stopNone, //nie ma powodu - powinien jechać
    "Off", // stopSleep, //nie został odpalony, to nie pojedzie
    "Semaphore", // stopSem, //semafor zamknięty
    "Time", // stopTime, //czekanie na godzinę odjazdu
    "End of track", // stopEnd, //brak dalszej części toru
    "Change direction", // stopDir, //trzeba stanąć, by zmienić kierunek jazdy
    "Joining", // stopJoin, //zatrzymanie przy (p)odczepianiu
    "Block", // stopBlock, //przeszkoda na drodze ruchu
    "A command", // stopComm, //otrzymano taką komendę (niewiadomego pochodzenia)
    "Out of station", // stopOut, //komenda wyjazdu poza stację (raczej nie powinna zatrzymywać!)
    "Radiostop", // stopRadio, //komunikat przekazany radiem (Radiostop)
    "External", // stopExt, //przesłany z zewnątrz
    "Error", // stopError //z powodu błędu w obliczeniu drogi hamowania
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void TSpeedPos::Clear()
{
    iFlags = 0; // brak flag to brak reakcji
    fVelNext = -1.0; // prędkość bez ograniczeń
    fDist = 0.0;
    vPos = vector3(0, 0, 0);
    trTrack = NULL; // brak wskaźnika
};

void TSpeedPos::CommandCheck()
{ // sprawdzenie typu komendy w evencie i określenie prędkości
    TCommandType command = evEvent->Command();
    double value1 = evEvent->ValueGet(1);
    double value2 = evEvent->ValueGet(2);
    if (command == cm_ShuntVelocity)
    { // prędkość manewrową zapisać, najwyżej AI zignoruje przy analizie tabelki
        fVelNext = value1; // powinno być value2, bo druga określa "za"?
        iFlags |= 0x200;
    }
    else if (command == cm_SetVelocity)
    { // w semaforze typu "m" jest ShuntVelocity dla Ms2 i SetVelocity dla S1
        // SetVelocity * 0    -> można jechać, ale stanąć przed
        // SetVelocity 0 20   -> stanąć przed, potem można jechać 20 (SBL)
        // SetVelocity -1 100 -> można jechać, przy następnym ograniczenie (SBL)
        // SetVelocity 40 -1  -> PutValues: jechać 40 aż do minięcia (koniec ograniczenia(
        fVelNext = value1;
        iFlags &= ~0xE00; // nie manewrowa, nie przystanek, nie zatrzymać na SBL
        if (value1 == 0.0) // jeśli pierwsza zerowa
            if (value2 != 0.0) // a druga nie
            { // S1 na SBL, można przejechać po zatrzymaniu (tu nie mamy prędkości ani odległości)
                fVelNext = value2; // normalnie będzie zezwolenie na jazdę, aby się usunął z tabelki
                iFlags |= 0x800; // flaga, że ma zatrzymać; na pewno nie zezwoli na manewry
            }
    }
    else if (command == cm_PassengerStopPoint) // nie ma dostępu do rozkładu
    { // przystanek, najwyżej AI zignoruje przy analizie tabelki
        if ((iFlags & 0x400) == 0)
            fVelNext = 0.0; // TrainParams->IsStop()?0.0:-1.0; //na razie tak
        iFlags |= 0x400; // niestety nie da się w tym miejscu współpracować z rozkładem
    }
    else if (command == cm_SetProximityVelocity)
    { // ignorować
        fVelNext = -1;
    }
    else if (command == cm_OutsideStation)
    { // w trybie manewrowym: skanować od niej wstecz i stanąć po wyjechaniu za sygnalizator i
      // zmienić kierunek
        // w trybie pociągowym: można przyspieszyć do wskazanej prędkości (po zjechaniu z rozjazdów)
        fVelNext = -1;
        iFlags |= 0x2100; // W5
    }
    else
    { // inna komenda w evencie skanowanym powoduje zatrzymanie i wysłanie tej komendy
        iFlags &= ~0xE00; // nie manewrowa, nie przystanek, nie zatrzymać na SBL
        fVelNext = 0; // jak nieznana komenda w komórce sygnałowej, to ma stać
    }
};

bool TSpeedPos::Update(vector3 *p, vector3 *dir, double &len)
{ // przeliczenie odległości od punktu (*p), w kierunku (*dir), zaczynając od pojazdu
    // dla kolejnych pozycji podawane są współrzędne poprzedniego obiektu w (*p)
    vector3 v = vPos - *p; // wektor od poprzedniego obiektu (albo pojazdu) do punktu zmiany
    fDist =
        v.Length(); // długość wektora to odległość pomiędzy czołem a sygnałem albo początkiem toru
    // v.SafeNormalize(); //normalizacja w celu określenia znaku (nie potrzebna?)
    if (len == 0.0)
    { // jeżeli liczymy względem pojazdu
        double iska = dir ? dir->x * v.x + dir->z * v.z :
                            fDist; // iloczyn skalarny to rzut na chwilową prostą ruchu
        if (iska < 0.0) // iloczyn skalarny jest ujemny, gdy punkt jest z tyłu
        { // jeśli coś jest z tyłu, to dokładna odległość nie ma już większego znaczenia
            fDist = -fDist; // potrzebne do badania wyjechania składem poza ograniczenie
            if (iFlags & 32) // 32 ustawione, gdy obiekt już został minięty
            { // jeśli minięty (musi być minięty również przez końcówkę składu)
            }
            else
            {
                iFlags ^= 32; // 32-minięty - będziemy liczyć odległość względem przeciwnego końca
                              // toru (nadal może być z przodu i ogdaniczać)
                if ((iFlags & 0x43) == 3) // tylko jeśli (istotny) tor, bo eventy są punktowe
                    if (trTrack) // może być NULL, jeśli koniec toru (????)
                        vPos =
                            (iFlags & 4) ?
                                trTrack->CurrentSegment()->FastGetPoint_0() :
                                trTrack->CurrentSegment()->FastGetPoint_1(); // drugi koniec istotny
            }
        }
        else if (fDist < 50.0) // przy dużym kącie łuku iloczyn skalarny bardziej zaniży odległość
                               // niż cięciwa
            fDist = iska; // ale przy małych odległościach rzut na chwilową prostą ruchu da
                          // dokładniejsze wartości
    }
    if (fDist > 0.0) // nie może być 0.0, a przypadkiem mogło by się trafić i było by źle
        if ((iFlags & 32) == 0) // 32 ustawione, gdy obiekt już został minięty
        { // jeśli obiekt nie został minięty, można od niego zliczać narastająco (inaczej może być
          // problem z wektorem kierunku)
            len = fDist = len + fDist; // zliczanie dlugości narastająco
            *p = vPos; // nowy punkt odniesienia
            *dir = Normalize(v); // nowy wektor kierunku od poprzedniego obiektu do aktualnego
        }
    if (iFlags & 2) // jeśli tor
    {
        if (trTrack) // może być NULL, jeśli koniec toru (???)
        {
            fVelNext = trTrack->VelocityGet(); // aktualizacja prędkości (może być zmieniana
                                               // eventem)
            int i;
            if ((i = iFlags & 0xF0000000) != 0)
            { // jeśli skrzyżowanie, ograniczyć prędkość przy skręcaniu
                if (abs(i) > 0x10000000) //±1 to jazda na wprost, ±2 nieby też, ale z przecięciem
                                         //głównej drogi - chyba że jest równorzędne...
                    fVelNext = 30.0; // uzależnić prędkość od promienia; albo niech będzie
                                     // ograniczona w skrzyżowaniu (velocity z ujemną wartością)
                if ((iFlags & 32) == 0) // jeśli nie wjechał
                    if (trTrack->iNumDynamics > 0) // a skrzyżowanie zawiera pojazd
                        fVelNext =
                            0.0; // to zabronić wjazdu (chyba że ten z przodu też jedzie prosto)
            }
            if (iFlags & 8) // jeśli odcinek zmienny
            {
                if (bool(trTrack->GetSwitchState() & 1) !=
                    bool(iFlags & 16)) // czy stan się zmienił?
                { // Ra: zakładam, że są tylko 2 możliwe stany
                    iFlags ^= 16;
                    // fVelNext=trTrack->VelocityGet(); //nowa prędkość
                    if ((iFlags & 32) == 0)
                        return true; // jeszcze trzeba skanowanie wykonać od tego toru
                    // problem jest chyba, jeśli zwrotnica się przełoży zaraz po zjechaniu z niej
                    // na Mydelniczce potrafi skanować na wprost mimo pojechania na bok
                }
                // poniższe nie dotyczy trybu łączenia?
                if ((iFlags & 32) ? false :
                                    trTrack->iNumDynamics >
                                        0) // jeśli jeszcze nie wjechano na tor, a coś na nim jest
                    fDist -= 30.0, fVelNext = 0.0; // to niech stanie w zwiększonej odległości
                // else if (fVelNext==0.0) //jeśli została wyzerowana
                // fVelNext=trTrack->VelocityGet(); //odczyt prędkości
            }
        }
    }
    else if (iFlags & 0x100) // jeśli event
    { // odczyt komórki pamięci najlepiej by było zrobić jako notyfikację, czyli zmiana komórki
      // wywoła jakąś podaną funkcję
        CommandCheck(); // sprawdzenie typu komendy w evencie i określenie prędkości
    }
    return false;
};

AnsiString TSpeedPos::TableText()
{ // pozycja tabelki prędkości
    if (iFlags & 0x1)
    { // o ile pozycja istotna
        if (iFlags & 0x2) // jeśli tor
            return "Flags=#" + IntToHex(iFlags, 8) + ", Dist=" + FloatToStrF(fDist, ffFixed, 7, 1) +
                   ", Vel=" + AnsiString(fVelNext) + ", Track=" + trTrack->NameGet();
        else if (iFlags & 0x100) // jeśli event
            return "Flags=#" + IntToHex(iFlags, 8) + ", Dist=" + FloatToStrF(fDist, ffFixed, 7, 1) +
                   ", Vel=" + AnsiString(fVelNext) + ", Event=" + evEvent->asName;
    }
    return "Empty";
}

bool TSpeedPos::Set(TEvent *e, double d)
{ // zapamiętanie zdarzenia
    fDist = d;
    iFlags = 0x101; // event+istotny
    evEvent = e;
    vPos = e->PositionGet(); // współrzędne eventu albo komórki pamięci (zrzutować na tor?)
    CommandCheck(); // sprawdzenie typu komendy w evencie i określenie prędkości
    return fVelNext == 0.0; // true gdy zatrzymanie, wtedy nie ma po co skanować dalej
};

void TSpeedPos::Set(TTrack *t, double d, int f)
{ // zapamiętanie zmiany prędkości w torze
    fDist = d; // odległość do początku toru
    trTrack = t; // TODO: (t) może być NULL i nie odczytamy końca poprzedniego :/
    if (trTrack)
    {
        iFlags = f | (trTrack->eType == tt_Normal ? 2 : 10); // zapamiętanie kierunku wraz z typem
        if (iFlags & 8)
            if (trTrack->GetSwitchState() & 1)
                iFlags |= 16;
        fVelNext = trTrack->VelocityGet();
        if (trTrack->iDamageFlag & 128)
            fVelNext = 0.0; // jeśli uszkodzony, to też stój
        if (iFlags & 64)
            fVelNext = (trTrack->iCategoryFlag & 1) ?
                           0.0 :
                           20.0; // jeśli koniec, to pociąg stój, a samochód zwolnij
        vPos = (bool(iFlags & 4) != bool(iFlags & 64)) ?
                   trTrack->CurrentSegment()->FastGetPoint_1() :
                   trTrack->CurrentSegment()->FastGetPoint_0();
    }
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void TController::TableClear()
{ // wyczyszczenie tablicy
    iFirst = iLast = 0;
    iTableDirection = 0; // nieznany
    for (int i = 0; i < iSpeedTableSize; ++i) // czyszczenie tabeli prędkości
        sSpeedTable[i].Clear();
    tLast = NULL;
    fLastVel = -1;
    eSignSkip = NULL; // nic nie pomijamy
};

TEvent *__fastcall TController::CheckTrackEvent(double fDirection, TTrack *Track)
{ // sprawdzanie eventów na podanym torze do podstawowego skanowania
    TEvent *e = (fDirection > 0) ? Track->evEvent2 : Track->evEvent1;
    if (!e)
        return NULL;
    if (e->bEnabled)
        return NULL;
    // jednak wszystkie W4 do tabelki, bo jej czyszczenie na przystanku wprowadza zamieszanie
    return e;
}

bool TController::TableAddNew()
{ // zwiększenie użytej tabelki o jeden rekord
    iLast = (iLast + 1) % iSpeedTableSize;
    // TODO: jeszcze sprawdzić, czy się na iFirst nie nałoży
    // TODO: wstawić tu wywołanie odtykacza - teraz jest to w TableTraceRoute()
    // TODO: jeśli ostatnia pozycja zajęta, ustawiać dodatkowe flagi - teraz jest to w
    // TableTraceRoute()
    // TODO: przydało by się też posortować tabelkę wg odległości (ale nie w tym miejscu)
    return true; // false gdy się nałoży
};

bool TController::TableNotFound(TEvent *e)
{ // sprawdzenie, czy nie został już dodany do tabelki (np. podwójne W4 robi problemy)
    int i, j = (iLast + 1) % iSpeedTableSize; // j, aby sprawdzić też ostatnią pozycję
    for (i = iFirst; i != j; i = (i + 1) % iSpeedTableSize)
        if ((sSpeedTable[i].iFlags & 0x101) == 0x101) // o ile używana pozycja
            if (sSpeedTable[i].evEvent == e)
                return false; // już jest, drugi raz dodawać nie ma po co
    return true; // nie ma, czyli można dodać
};

void TController::TableTraceRoute(double fDistance, TDynamicObject *pVehicle)
{ // skanowanie trajektorii na odległość (fDistance) od (pVehicle) w kierunku przodu składu i
  // uzupełnianie tabelki
    if (!iDirection) // kierunek pojazdu z napędem
    { // jeśli kierunek jazdy nie jest okreslony
        iTableDirection = 0; // czekamy na ustawienie kierunku
    }
    TTrack *pTrack; // zaczynamy od ostatniego analizowanego toru
    // double fDistChVel=-1; //odległość do toru ze zmianą prędkości
    double fTrackLength; // długość aktualnego toru (krótsza dla pierwszego)
    double fCurrentDistance; // aktualna przeskanowana długość
    TEvent *pEvent;
    double fLastDir; // kierunek na ostatnim torze
    if (iTableDirection != iDirection)
    { // jeśli zmiana kierunku, zaczynamy od toru ze wskazanym pojazdem
        pTrack = pVehicle->RaTrackGet(); // odcinek, na którym stoi
        fLastDir = pVehicle->DirectionGet() *
                   pVehicle->RaDirectionGet(); // ustalenie kierunku skanowania na torze
        fCurrentDistance = 0; // na razie nic nie przeskanowano
        fTrackLength = pVehicle->RaTranslationGet(); // pozycja na tym torze (odległość od Point1)
        if (fLastDir > 0) // jeśli w kierunku Point2 toru
            fTrackLength =
                pTrack->Length() - fTrackLength; // przeskanowana zostanie odległość do Point2
        fLastVel = pTrack->VelocityGet(); // aktualna prędkość
        iTableDirection =
            iDirection; // ustalenie w jakim kierunku jest wypełniana tabelka względem pojazdu
        iFirst = iLast = 0;
        tLast = NULL; //żaden nie sprawdzony
    }
    else
    { // kontynuacja skanowania od ostatnio sprawdzonego toru (w ostatniej pozycji zawsze jest tor)
        if (sSpeedTable[iLast].iFlags & 0x10000) // zatkanie
        { // jeśli zapełniła się tabelka
            if ((iLast + 1) % iSpeedTableSize == iFirst) // jeśli nadal jest zapełniona
                return; // nic się nie da zrobić
            if ((iLast + 2) % iSpeedTableSize == iFirst) // musi być jeszcze miejsce wolne na
                                                         // ewentualny event, bo tor jeszcze nie
                                                         // sprawdzony
                return; // już lepiej, ale jeszcze nie tym razem
            sSpeedTable[iLast].iFlags &= 0xBE; // kontynuować próby doskanowania
        }
        else if (VelNext == 0)
            return; // znaleziono semafor lub tor z prędkością zero i nie ma co dalej sprawdzać
        pTrack = sSpeedTable[iLast].trTrack; // ostatnio sprawdzony tor
        if (!pTrack)
            return; // koniec toru, to nie ma co sprawdzać (nie ma prawa tak być)
        fLastDir = sSpeedTable[iLast].iFlags & 4 ?
                       -1.0 :
                       1.0; // flaga ustawiona, gdy Point2 toru jest bliżej
        fCurrentDistance = sSpeedTable[iLast].fDist; // aktualna odległość do jego Point1
        fTrackLength =
            sSpeedTable[iLast].iFlags & 0x60 ? 0.0 : pTrack->Length(); // nie doliczać długości gdy:
                                                                       // 32-minięty początek,
                                                                       // 64-jazda do końca toru
    }
    if (fCurrentDistance < fDistance)
    { // jeśli w ogóle jest po co analizować
        --iLast; // jak coś się znajdzie, zostanie wpisane w tę pozycję, którą właśnie odczytano
        while (fCurrentDistance < fDistance)
        {
            if (pTrack != tLast) // ostatni zapisany w tabelce nie był jeszcze sprawdzony
            { // jeśli tor nie był jeszcze sprawdzany
                if ((pEvent = CheckTrackEvent(fLastDir, pTrack)) !=
                    NULL) // jeśli jest semafor na tym torze
                { // trzeba sprawdzić tabelkę, bo dodawanie drugi raz tego samego przystanku nie
                  // jest korzystne
                    if (TableNotFound(pEvent)) // jeśli nie ma
                        if (TableAddNew())
                        {
                            if (sSpeedTable[iLast].Set(pEvent,
                                                       fCurrentDistance)) // dodanie odczytu sygnału
                                fDistance = fCurrentDistance; // jeśli sygnał stop, to nie ma
                                                              // potrzeby dalej skanować
                        }
                } // event dodajemy najpierw, żeby móc sprawdzić, czy tor został dodany po
                  // odczytaniu prędkości następnego
                if ((pTrack->VelocityGet() == 0.0) // zatrzymanie
                    || (pTrack->iAction) // jeśli tor ma własności istotne dla skanowania
                    || (pTrack->VelocityGet() != fLastVel)) // następuje zmiana prędkości
                { // odcinek dodajemy do tabelki, gdy jest istotny dla ruchu
                    if (TableAddNew())
                    { // teraz dodatkowo zapamiętanie wybranego segmentu dla skrzyżowania
                        sSpeedTable[iLast].Set(
                            pTrack, fCurrentDistance,
                            fLastDir < 0 ?
                                5 :
                                1); // dodanie odcinka do tabelki z flagą kierunku wejścia
                        if (pTrack->eType == tt_Cross) // na skrzyżowaniach trzeba wybrać segment,
                                                       // po którym pojedzie pojazd
                        { // dopiero tutaj jest ustalany kierunek segmentu na skrzyżowaniu
                            sSpeedTable[iLast].iFlags |=
                                (pTrack->CrossSegment((fLastDir < 0) ? tLast->iPrevDirection :
                                                                       tLast->iNextDirection,
                                                      iRouteWanted) &
                                 15)
                                << 28; // ostatnie 4 bity pola flag
                            sSpeedTable[iLast].iFlags &=
                                ~4; // usunięcie flagi kierunku, bo może być błędna
                            if (sSpeedTable[iLast].iFlags < 0)
                                sSpeedTable[iLast].iFlags |=
                                    4; // ustawienie flagi kierunku na podstawie wybranego segmentu
                            if (int(fLastDir) * sSpeedTable[iLast].iFlags < 0)
                                fLastDir = -fLastDir;
                            if (AIControllFlag) // dla AI na razie losujemy kierunek na kolejnym
                                                // skrzyżowaniu
                                iRouteWanted = 1 + random(3);
                        }
                    }
                }
                else if ((pTrack->fRadius != 0.0) // odległość na łuku lepiej aproksymować cięciwami
                         || (tLast ? tLast->fRadius != 0.0 : false)) // koniec łuku też jest istotny
                { // albo dla liczenia odległości przy pomocy cięciw - te usuwać po przejechaniu
                    if (TableAddNew())
                        sSpeedTable[iLast].Set(pTrack, fCurrentDistance,
                                               fLastDir < 0 ? 0x85 :
                                                              0x81); // dodanie odcinka do tabelki
                }
            }
            fCurrentDistance +=
                fTrackLength; // doliczenie kolejnego odcinka do przeskanowanej długości
            tLast = pTrack; // odhaczenie, że sprawdzony
            // Track->ScannedFlag=true; //do pokazywania przeskanowanych torów
            fLastVel = pTrack->VelocityGet(); // prędkość na poprzednio sprawdzonym odcinku
            pTrack = pTrack->Neightbour(
                (pTrack->eType == tt_Cross) ? (sSpeedTable[iLast].iFlags >> 28) : int(fLastDir),
                fLastDir); // może być NULL
            /*
               if (fLastDir>0)
               {//jeśli szukanie od Point1 w kierunku Point2
                pTrack=pTrack->CurrentNext(); //może być NULL
                if (pTrack) //jeśli dalej brakuje toru, to zostajemy na tym samym, z tą samą
               orientacją
                 if (tLast->iNextDirection)
                  fLastDir=-fLastDir; //można by zamiętać i zmienić tylko jeśli jest pTrack
               }
               else //if (fDirection<0)
               {//jeśli szukanie od Point2 w kierunku Point1
                pTrack=pTrack->CurrentPrev(); //może być NULL
                if (pTrack) //jeśli dalej brakuje toru, to zostajemy na tym samym, z tą samą
               orientacją
                 if (!tLast->iPrevDirection)
                  fLastDir=-fLastDir;
               }
            */
            if (pTrack)
            { // jeśli kolejny istnieje
                if (tLast)
                    if (pTrack->VelocityGet() < 0 ? tLast->VelocityGet() > 0 :
                                                    pTrack->VelocityGet() > tLast->VelocityGet())
                    { // jeśli kolejny ma większą prędkość niż poprzedni, to zapamiętać poprzedni
                      // (do czasu wyjechania)
                        if ((sSpeedTable[iLast].iFlags & 3) == 3 ?
                                (sSpeedTable[iLast].trTrack != tLast) :
                                true) // jeśli nie był dodany do tabelki
                            if (TableAddNew())
                                sSpeedTable[iLast].Set(
                                    tLast, fCurrentDistance,
                                    (fLastDir > 0 ? pTrack->iPrevDirection :
                                                    pTrack->iNextDirection) ?
                                        1 :
                                        5); // zapisanie toru z ograniczeniem prędkości
                    }
                if (((iLast + 3) % iSpeedTableSize == iFirst) ?
                        true :
                        ((iLast + 2) % iSpeedTableSize == iFirst)) // czy tabelka się nie zatka?
                { // jest ryzyko nieznalezienia ograniczenia - ograniczyć prędkość do pozwalającej
                  // na zatrzymanie na końcu przeskanowanej drogi
                    TablePurger(); // usunąć pilnie zbędne pozycje
                    if (((iLast + 3) % iSpeedTableSize == iFirst) ?
                            true :
                            ((iLast + 2) % iSpeedTableSize == iFirst)) // czy tabelka się nie zatka?
                    { // jeśli odtykacz nie pomógł (TODO: zwiększyć rozmiar tabelki)
                        if (TableAddNew())
                            sSpeedTable[iLast].Set(
                                pTrack, fCurrentDistance,
                                fLastDir < 0 ?
                                    0x10045 :
                                    0x10041); // zapisanie toru jako końcowego (ogranicza prędkosć)
                        // zapisać w logu, że należy poprawić scenerię?
                        return; // nie skanujemy dalej, bo nie ma miejsca
                    }
                }
                fTrackLength = pTrack->Length(); // zwiększenie skanowanej odległości tylko jeśli
                                                 // istnieje dalszy tor
            }
            else
            { // definitywny koniec skanowania, chyba że dalej puszczamy samochód po gruncie...
                if (TableAddNew()) // kolejny, bo się cofnęliśmy o 1
                    sSpeedTable[iLast].Set(
                        tLast, fCurrentDistance,
                        fLastDir < 0 ? 0x45 : 0x41); // zapisanie ostatniego sprawdzonego toru
                return; // to ostatnia pozycja, bo NULL nic nie da, a może się podpiąć obrotnica,
                        // czy jakieś transportery
            }
        }
        if (TableAddNew())
            sSpeedTable[iLast].Set(pTrack, fCurrentDistance,
                                   fLastDir < 0 ? 4 : 0); // zapisanie ostatniego sprawdzonego toru
    }
};

void TController::TableCheck(double fDistance)
{ // przeliczenie odległości w tabelce, ewentualnie doskanowanie (bez analizy prędkości itp.)
    if (iTableDirection != iDirection)
        TableTraceRoute(fDistance,
                        pVehicles[1]); // jak zmiana kierunku, to skanujemy od końca składu
    else if (iTableDirection)
    { // trzeba sprawdzić, czy coś się zmieniło
        vector3 dir =
            pVehicles[0]->VectorFront() * pVehicles[0]->DirectionGet(); // wektor kierunku jazdy
        vector3 pos = pVehicles[0]->HeadPosition(); // zaczynamy od pozycji pojazdu
        // double lastspeed=-1; //prędkość na torze do usunięcia
        double len = 0.0; // odległość będziemy zliczać narastająco
        for (int i = iFirst; i != iLast; i = (i + 1) % iSpeedTableSize)
        { // aktualizacja rekordów z wyjątkiem ostatniego
            if (sSpeedTable[i].iFlags & 1) // jeśli pozycja istotna
            {
                if (sSpeedTable[i].Update(&pos, &dir, len))
                {
                    iLast = i; // wykryta zmiana zwrotnicy - konieczne ponowne przeskanowanie
                               // dalszej części
                    break; // nie kontynuujemy pętli, trzeba doskanować ciąg dalszy
                }
                if (sSpeedTable[i].iFlags & 2) // jeśli odcinek
                {
                    if (sSpeedTable[i].fDist < -fLength) // a skład wyjechał całą długością poza
                    { // degradacja pozycji
                        sSpeedTable[i].iFlags &= ~1; // nie liczy się
                    }
                    else if ((sSpeedTable[i].iFlags & 0xF0000028) ==
                             0x20) // jest z tyłu (najechany) i nie jest zwrotnicą ani skrzyżowaniem
                        if (sSpeedTable[i].fVelNext < 0) // a nie ma ograniczenia prędkości
                            sSpeedTable[i].iFlags =
                                0; // to nie ma go po co trzymać (odtykacz usunie ze środka)
                }
                else if (sSpeedTable[i].iFlags & 0x100) // jeśli event
                {
                    if (sSpeedTable[i].fDist < (sSpeedTable[i].evEvent->Type == tp_PutValues ?
                                                    -fLength :
                                                    0)) // jeśli jest z tyłu
                        if ((mvOccupied->CategoryFlag & 1) ? false :
                                                             sSpeedTable[i].fDist < -fLength)
                        { // pociąg staje zawsze, a samochód tylko jeśli nie przejedzie całą
                          // długością (może być zaskoczony zmianą)
                            sSpeedTable[i].iFlags &= ~1; // degradacja pozycji dla samochodu;
                                                         // semafory usuwane tylko przy sprawdzaniu,
                                                         // bo wysyłają komendy
                        }
                }
                // if (sSpeedTable[i].fDist<-20.0*fLength) //jeśli to coś jest 20 razy dalej niż
                // długość składu
                //{sSpeedTable[i].iFlags&=~1; //to jest to jakby błąd w scenerii
                // //WriteLog("Error: too distant object in scan table");
                //}
                // if (sSpeedTable[i].fDist>20.0*fLength) //jeśli to coś jest 20 razy dalej niż
                // długość składu
                //{sSpeedTable[i].iFlags&=~1; //to jest to jakby błąd w scenerii
                // //WriteLog("Error: too distant object in scan table");
                //}
            }
            if (i == iFirst) // jeśli jest pierwszą pozycją tabeli
            { // pozbycie się początkowej pozycji
                if ((sSpeedTable[i].iFlags & 1) ==
                    0) // jeśli pozycja istotna (po Update() może się zmienić)
                    // if (iFirst!=iLast) //ostatnia musi zostać - to załatwia for()
                    iFirst = (iFirst + 1) %
                             iSpeedTableSize; // kolejne sprawdzanie będzie już od następnej pozycji
            }
        }
        sSpeedTable[iLast].Update(&pos, &dir, len); // aktualizacja ostatniego
        if (sSpeedTable[iLast].fDist < fDistance)
            TableTraceRoute(fDistance, pVehicles[1]); // doskanowanie dalszego odcinka
    }
};

TCommandType TController::TableUpdate(double &fVelDes, double &fDist, double &fNext,
                                                 double &fAcc)
{ // ustalenie parametrów, zwraca typ komendy, jeśli sygnał podaje prędkość do jazdy
    // fVelDes - prędkość zadana
    // fDist - dystans w jakim należy rozważyć ruch
    // fNext - prędkość na końcu tego dystansu
    // fAcc - zalecane przyspieszenie w chwili obecnej - kryterium wyboru dystansu
    double a; // przyspieszenie
    double v; // prędkość
    double d; // droga
    TCommandType go = cm_Unknown;
    eSignNext = NULL;
    int i, k = iLast - iFirst + 1;
    if (k < 0)
        k += iSpeedTableSize; // ilość pozycji do przeanalizowania
    iDrivigFlags &=
        ~(moveTrackEnd | moveSwitchFound); // te flagi są ustawiane tutaj, w razie potrzeby
    for (i = iFirst; k > 0; --k, i = (i + 1) % iSpeedTableSize)
    { // sprawdzenie rekordów od (iFirst) do (iLast), o ile są istotne
        if (sSpeedTable[i].iFlags & 1) // badanie istotności
        { // o ile dana pozycja tabelki jest istotna
            if (sSpeedTable[i].iFlags & 0x400)
            { // jeśli przystanek, trzeba obsłużyć wg rozkładu
                if (sSpeedTable[i].evEvent->CommandGet() != asNextStop)
                { // jeśli nazwa nie jest zgodna
                    if (sSpeedTable[i].fDist < -fLength) // jeśli został przejechany
                        sSpeedTable[i].iFlags =
                            0; // to można usunąć (nie mogą być usuwane w skanowaniu)
                    continue; // ignorowanie jakby nie było tej pozycji
                }
                else if (iDrivigFlags &
                         moveStopPoint) // jeśli pomijanie W4, to nie sprawdza czasu odjazdu
                { // tylko gdy nazwa zatrzymania się zgadza
                    if (!TrainParams->IsStop())
                    { // jeśli nie ma tu postoju
                        sSpeedTable[i].fVelNext = -1; // maksymalna prędkość w tym miejscu
                        if (sSpeedTable[i].fDist <
                            200.0) // przy 160km/h jedzie 44m/s, to da dokładność rzędu 5 sekund
                        { // zaliczamy posterunek w pewnej odległości przed (choć W4 nie zasłania
                          // już semafora)
#if LOGSTOPS
                            WriteLog(pVehicle->asName + " as " + TrainParams->TrainName + ": at " +
                                     AnsiString(GlobalTime->hh) + ":" + AnsiString(GlobalTime->mm) +
                                     " skipped " + asNextStop); // informacja
#endif
                            fLastStopExpDist = mvOccupied->DistCounter + 0.250 +
                                               0.001 * fLength; // przy jakim dystansie (stanie
                                                                // licznika) ma przesunąć na
                                                                // następny postój
                            TrainParams->UpdateMTable(
                                GlobalTime->hh, GlobalTime->mm,
                                asNextStop.SubString(20, asNextStop.Length()));
                            TrainParams->StationIndexInc(); // przejście do następnej
                            asNextStop =
                                TrainParams->NextStop(); // pobranie kolejnego miejsca zatrzymania
                            // TableClear(); //aby od nowa sprawdziło W4 z inną nazwą już - to nie
                            // jest dobry pomysł
                            sSpeedTable[i].iFlags = 0; // nie liczy się już
                            sSpeedTable[i].fVelNext = -1; // jechać
                            continue; // nie analizować prędkości
                        }
                    } // koniec obsługi przelotu na W4
                    else
                    { // zatrzymanie na W4
                        if (!eSignNext)
                            eSignNext = sSpeedTable[i].evEvent;
                        if (mvOccupied->Vel > 0.3) // jeśli jedzie (nie trzeba czekać, aż się
                                                   // drgania wytłumią - drzwi zamykane od 1.0)
                            sSpeedTable[i].fVelNext = 0; // to będzie zatrzymanie
                        // else if
                        // ((iDrivigFlags&moveStopCloser)?sSpeedTable[i].fDist<=fMaxProximityDist*(AIControllFlag?1.0:10.0):true)
                        else if ((iDrivigFlags & moveStopCloser) ?
                                     sSpeedTable[i].fDist + fLength <=
                                         Max0R(sSpeedTable[i].evEvent->ValueGet(2),
                                               fMaxProximityDist + fLength) :
                                     true)
                        // Ra 2F1I: odległość plus długość pociągu musi być mniejsza od długości
                        // peronu, chyba że pociąg jest dłuższy, to wtedy minimalna
                        // jeśli długość peronu ((sSpeedTable[i].evEvent->ValueGet(2)) nie podana,
                        // przyjąć odległość fMinProximityDist
                        { // jeśli się zatrzymał przy W4, albo stał w momencie zobaczenia W4
                            if (!AIControllFlag) // AI tylko sobie otwiera drzwi
                                iDrivigFlags &= ~moveStopCloser; // w razie przełączenia na AI ma
                                                                 // nie podciągać do W4, gdy
                                                                 // użytkownik zatrzymał za daleko
                            if ((iDrivigFlags & moveDoorOpened) == 0)
                            { // drzwi otwierać jednorazowo
                                iDrivigFlags |= moveDoorOpened; // nie wykonywać drugi raz
                                if (mvOccupied->DoorOpenCtrl == 1) //(mvOccupied->TrainType==dt_EZT)
                                { // otwieranie drzwi w EZT
                                    if (AIControllFlag) // tylko AI otwiera drzwi EZT, użytkownik
                                                        // musi samodzielnie
                                        if (!mvOccupied->DoorLeftOpened &&
                                            !mvOccupied->DoorRightOpened)
                                        { // otwieranie drzwi
                                            int p2 =
                                                int(floor(sSpeedTable[i].evEvent->ValueGet(2))) %
                                                10; // p7=platform side (1:left, 2:right, 3:both)
                                            int lewe = (iDirection > 0) ? 1 : 2; // jeśli jedzie do
                                                                                 // tyłu, to drzwi
                                                                                 // otwiera
                                                                                 // odwrotnie
                                            int prawe = (iDirection > 0) ? 2 : 1;
                                            if (p2 & lewe)
                                                mvOccupied->DoorLeft(true);
                                            if (p2 & prawe)
                                                mvOccupied->DoorRight(true);
                                            // if (p2&3) //żeby jeszcze poczekał chwilę, zanim
                                            // zamknie
                                            // WaitingSet(10); //10 sekund (wziąć z rozkładu????)
                                        }
                                }
                                else
                                { // otwieranie drzwi w składach wagonowych - docelowo wysyłać
                                  // komendę zezwolenia na otwarcie drzwi
                                    int p7, lewe,
                                        prawe; // p7=platform side (1:left, 2:right, 3:both)
                                    p7 = int(floor(sSpeedTable[i].evEvent->ValueGet(2))) %
                                         10; // tu będzie jeszcze długość peronu zaokrąglona do 10m
                                             // (20m bezpieczniej, bo nie modyfikuje bitu 1)
                                    TDynamicObject *p = pVehicles[0]; // pojazd na czole składu
                                    while (p)
                                    { // otwieranie drzwi w pojazdach - flaga zezwolenia była by
                                      // lepsza
                                        lewe = (p->DirectionGet() > 0) ? 1 : 2; // jeśli jedzie do
                                                                                // tyłu, to drzwi
                                                                                // otwiera odwrotnie
                                        prawe = 3 - lewe;
                                        p->MoverParameters->BatterySwitch(true); // wagony muszą
                                                                                 // mieć baterię
                                                                                 // załączoną do
                                                                                 // otwarcia
                                                                                 // drzwi...
                                        if (p7 & lewe)
                                            p->MoverParameters->DoorLeft(true);
                                        if (p7 & prawe)
                                            p->MoverParameters->DoorRight(true);
                                        p = p->Next(); // pojazd podłączony z tyłu (patrząc od
                                                       // czoła)
                                    }
                                    // if (p7&3) //żeby jeszcze poczekał chwilę, zanim zamknie
                                    // WaitingSet(10); //10 sekund (wziąć z rozkładu????)
                                }
                                if (fStopTime >
                                    -5) // na końcu rozkładu się ustawia 60s i tu by było skrócenie
                                    WaitingSet(10); // 10 sekund (wziąć z rozkładu????) - czekanie
                                                    // niezależne od sposobu obsługi drzwi, bo
                                                    // opóźnia również kierownika
                            }
                            if (TrainParams->UpdateMTable(
                                    GlobalTime->hh, GlobalTime->mm,
                                    asNextStop.SubString(20, asNextStop.Length())))
                            { // to się wykona tylko raz po zatrzymaniu na W4
                                if (TrainParams->CheckTrainLatency() < 0.0)
                                    iDrivigFlags |= moveLate; // odnotowano spóźnienie
                                else
                                    iDrivigFlags &= ~moveLate; // przyjazd o czasie
                                if (TrainParams->DirectionChange()) // jeśli "@" w rozkładzie, to
                                                                    // wykonanie dalszych komend
                                { // wykonanie kolejnej komendy, nie dotyczy ostatniej stacji
                                    if (iDrivigFlags & movePushPull) // SN61 ma się też nie ruszać,
                                                                     // chyba że ma wagony
                                    {
                                        iDrivigFlags |= moveStopHere; // EZT ma stać przy peronie
                                        if (OrderNextGet() != Change_direction)
                                        {
                                            OrderPush(Change_direction); // zmiana kierunku
                                            OrderPush(TrainParams->StationIndex <
                                                              TrainParams->StationCount ?
                                                          Obey_train :
                                                          Shunt); // to dalej wg rozkładu
                                        }
                                    }
                                    else // a dla lokomotyw...
                                        iDrivigFlags &=
                                            ~(moveStopPoint | moveStopHere); // pozwolenie na
                                                                             // przejechanie za W4
                                                                             // przed czasem i nie
                                                                             // ma stać
                                    JumpToNextOrder(); // przejście do kolejnego rozkazu (zmiana
                                                       // kierunku, odczepianie)
                                    iDrivigFlags &= ~moveStopCloser; // ma nie podjeżdżać pod W4 po
                                                                     // przeciwnej stronie
                                    sSpeedTable[i].iFlags = 0; // ten W4 nie liczy się już zupełnie
                                                               // (nie wyśle SetVelocity)
                                    sSpeedTable[i].fVelNext = -1; // jechać
                                    continue; // nie analizować prędkości
                                }
                            }
                            if (OrderCurrentGet() == Shunt)
                            {
                                OrderNext(Obey_train); // uruchomić jazdę pociągową
                                CheckVehicles(); // zmienić światła
                            }
                            if (TrainParams->StationIndex < TrainParams->StationCount)
                            { // jeśli są dalsze stacje, czekamy do godziny odjazdu
                                if (TrainParams->IsTimeToGo(GlobalTime->hh, GlobalTime->mm))
                                { // z dalszą akcją czekamy do godziny odjazdu
                                    // if (TrainParams->CheckTrainLatency()<0.0) //jak się ma odjazd
                                    // do czasu odjazdu?
                                    // iDrivigFlags|=moveLate1; //oflagować, gdy odjazd ze
                                    // spóźnieniem, będzie jechał forsowniej
                                    fLastStopExpDist =
                                        mvOccupied->DistCounter + 0.050 +
                                        0.001 * fLength; // przy jakim dystansie (stanie licznika)
                                                         // ma przesunąć na następny postój
                                    //         Controlled->    //zapisać odległość do przejechania
                                    TrainParams->StationIndexInc(); // przejście do następnej
                                    asNextStop =
                                        TrainParams
                                            ->NextStop(); // pobranie kolejnego miejsca zatrzymania
// TableClear(); //aby od nowa sprawdziło W4 z inną nazwą już - to nie jest dobry pomysł
#if LOGSTOPS
                                    WriteLog(pVehicle->asName + " as " + TrainParams->TrainName +
                                             ": at " + AnsiString(GlobalTime->hh) + ":" +
                                             AnsiString(GlobalTime->mm) + " next " +
                                             asNextStop); // informacja
#endif
                                    if (int(floor(sSpeedTable[i].evEvent->ValueGet(1))) & 1)
                                        iDrivigFlags |= moveStopHere; // nie podjeżdżać do semafora,
                                                                      // jeśli droga nie jest wolna
                                    iDrivigFlags |= moveStopCloser; // do następnego W4 podjechać
                                                                    // blisko (z dociąganiem)
                                    iDrivigFlags &= ~moveStartHorn; // bez trąbienia przed odjazdem
                                    sSpeedTable[i].iFlags =
                                        0; // nie liczy się już zupełnie (nie wyśle SetVelocity)
                                    sSpeedTable[i].fVelNext = -1; // można jechać za W4
                                    if (go == cm_Unknown) // jeśli nie było komendy wcześniej
                                        go = cm_Ready; // gotów do odjazdu z W4 (semafor może
                                                       // zatrzymać)
                                    if (tsGuardSignal) // jeśli mamy głos kierownika, to odegrać
                                        iDrivigFlags |= moveGuardSignal;
                                    continue; // nie analizować prędkości
                                } // koniec startu z zatrzymania
                            } // koniec obsługi początkowych stacji
                            else
                            { // jeśli dojechaliśmy do końca rozkładu
#if LOGSTOPS
                                WriteLog(pVehicle->asName + " as " + TrainParams->TrainName +
                                         ": at " + AnsiString(GlobalTime->hh) + ":" +
                                         AnsiString(GlobalTime->mm) +
                                         " end of route."); // informacja
#endif
                                asNextStop = TrainParams->NextStop(); // informacja o końcu trasy
                                TrainParams->NewName("none"); // czyszczenie nieaktualnego rozkładu
                                // TableClear(); //aby od nowa sprawdziło W4 z inną nazwą już - to
                                // nie jest dobry pomysł
                                iDrivigFlags &=
                                    ~(moveStopCloser |
                                      moveStopPoint); // ma nie podjeżdżać pod W4 i ma je pomijać
                                sSpeedTable[i].iFlags =
                                    0; // W4 nie liczy się już (nie wyśle SetVelocity)
                                sSpeedTable[i].fVelNext = -1; // można jechać za W4
                                fLastStopExpDist = -1.0f; // nie ma rozkładu, nie ma usuwania stacji
                                WaitingSet(60); // tak ze 2 minuty, aż wszyscy wysiądą
                                JumpToNextOrder(); // wykonanie kolejnego rozkazu (Change_direction
                                                   // albo Shunt)
                                iDrivigFlags |= moveStopHere | moveStartHorn; // ma się nie ruszać
                                                                              // aż do momentu
                                                                              // podania sygnału
                                continue; // nie analizować prędkości
                            } // koniec obsługi ostatniej stacji
                        } // if (MoverParameters->Vel==0.0)
                    } // koniec obsługi zatrzymania na W4
                } // koniec warunku pomijania W4 podczas zmiany czoła
                else
                { // skoro pomijanie, to jechać i ignorować W4
                    sSpeedTable[i].iFlags = 0; // W4 nie liczy się już (nie zatrzymuje jazdy)
                    sSpeedTable[i].fVelNext = -1;
                    continue; // nie analizować prędkości
                }
            } // koniec obsługi W4
            v = sSpeedTable[i].fVelNext; // odczyt prędkości do zmiennej pomocniczej
            if (sSpeedTable[i].iFlags &
                8) // zwrotnice są usuwane z tabelki dopiero po zjechaniu z nich
                iDrivigFlags |=
                    moveSwitchFound; // rozjazd z przodu/pod ogranicza np. sens skanowania wstecz
            else if (sSpeedTable[i].iFlags & 0x100) // W4 może się deaktywować
            { // jeżeli event, może być potrzeba wysłania komendy, aby ruszył
                if (sSpeedTable[i].iFlags & 0x2000)
                { // jeśli W5, to reakcja zależna od trybu jazdy
                    if (OrderCurrentGet() & Obey_train)
                    { // w trybie pociągowym: można przyspieszyć do wskazanej prędkości (po
                      // zjechaniu z rozjazdów)
                        v = -1.0; // ignorować?
                        if (sSpeedTable[i].fDist < 0.0) // jeśli wskaźnik został minięty
                        {
                            VelSignal = v; //!!! ustawienie, gdy przejechany jest lepsze niż wcale,
                                           //ale to jeszcze nie to
                            //       iStationStart=TrainParams->StationIndex; //zaktualizować
                            //       wyświetlanie rozkładu
                        }
                        else if (!(iDrivigFlags & moveSwitchFound)) // jeśli rozjazdy już minięte
                            VelSignal = v; //!!! to też koniec ograniczenia
                    }
                    else
                    { // w trybie manewrowym: skanować od niego wstecz, stanąć po wyjechaniu za
                      // sygnalizator i zmienić kierunek
                        v = 0.0; // zmiana kierunku może być podanym sygnałem, ale wypadało by
                                 // zmienić światło wcześniej
                        if (!(iDrivigFlags & moveSwitchFound)) // jeśli nie ma rozjazdu
                            iDrivigFlags |= moveTrackEnd; // to dalsza jazda trwale ograniczona (W5,
                                                          // koniec toru)
                    }
                }
                else if (sSpeedTable[i].iFlags & 0x800)
                { // jeśli S1 na SBL
                    if (mvOccupied->Vel < 2.0) // stanąć nie musi, ale zwolnić przynajmniej
                        if (sSpeedTable[i].fDist < fMaxProximityDist) // jest w maksymalnym zasięgu
                        {
                            eSignSkip = sSpeedTable[i]
                                            .evEvent; // to można go pominąć (wziąć drugą prędkosć)
                            iDrivigFlags |= moveVisibility; // jazda na widoczność - skanować
                                                            // możliwość kolizji i nie podjeżdżać
                                                            // zbyt blisko
                            // usunąć flagę po podjechaniu blisko semafora zezwalającego na jazdę
                            // ostrożnie interpretować sygnały - semafor może zezwalać na jazdę
                            // pociągu z przodu!
                        }
                    if (eSignSkip != sSpeedTable[i].evEvent) // jeśli ten SBL nie jest do pominięcia
                        v = sSpeedTable[i].evEvent->ValueGet(1); // to ma 0 odczytywać
                }
                if ((mvOccupied->CategoryFlag & 1) ?
                        sSpeedTable[i].fDist > pVehicles[0]->fTrackBlock - 20.0 :
                        false) // jak sygnał jest dalej niż zawalidroga
                    v = 0.0; // to może być podany dla tamtego: jechać tak, jakby tam stop był
                else
                { // zawalidrogi nie ma (albo pojazd jest samochodem), sprawdzić sygnał
                    if (sSpeedTable[i].iFlags & 0x200) // jeśli Tm - w zasadzie to sprawdzić
                                                       // komendę!
                    { // jeśli podana prędkość manewrowa
                        if ((OrderCurrentGet() & Obey_train) ? v == 0.0 : false)
                        { // jeśli tryb pociągowy a tarcze ma ShuntVelocity 0 0
                            v = -1; // ignorować, chyba że prędkość stanie się niezerowa
                            if (sSpeedTable[i].iFlags & 0x20) // a jak przejechana
                                sSpeedTable[i].iFlags = 0; // to można usunąć, bo podstawowy automat
                                                           // usuwa tylko niezerowe
                        }
                        else if (go <= cm_Ready) // jeśli jeszcze nie ma komendy
                            if (v != 0.0) // komenda jest tylko gdy ma jechać, bo stoi na podstawie
                                          // tabelki
                            { // jeśli nie było komendy wcześniej - pierwsza się liczy - ustawianie
                              // VelSignal
                                go = cm_ShuntVelocity; // w trybie pociągowym tylko jeśli włącza
                                                       // tryb manewrowy (v!=0.0)
                                // Ra 2014-06: (VelSignal) nie może być tu ustawiane, bo Tm może być
                                // daleko
                                // VelSignal=v; //nie do końca tak, to jest druga prędkość
                                if (VelSignal == 0.0)
                                    VelSignal = v; // aby stojący ruszył
                                if (sSpeedTable[i].fDist < 0.0) // jeśli przejechany
                                {
                                    VelSignal = v; //!!! ustawienie, gdy przejechany jest lepsze niż
                                                   //wcale, ale to jeszcze nie to
                                    sSpeedTable[i].iFlags =
                                        0; // to można usunąć (nie mogą być usuwane w skanowaniu)
                                }
                            }
                    }
                    else // if (sSpeedTable[i].iFlags&0x100) //jeśli semafor !!! Komendę trzeba
                         // sprawdzić !!!!
                        if (go <= cm_Ready) // jeśli nie było komendy wcześniej - pierwsza się liczy
                                            // - ustawianie VelSignal
                        if (v < 0.0 ? true : v >= 1.0) // bo wartość 0.1 służy do hamowania tylko
                        {
                            go = cm_SetVelocity; // może odjechać
                            // Ra 2014-06: (VelSignal) nie może być tu ustawiane, bo semafor może
                            // być daleko
                            // VelSignal=v; //nie do końca tak, to jest druga prędkość; -1 nie
                            // wpisywać...
                            if (VelSignal == 0.0)
                                VelSignal = v; // aby stojący ruszył
                            if (sSpeedTable[i].fDist < 0.0) // jeśli przejechany
                            {
                                VelSignal = v; //!!! ustawienie, gdy przejechany jest lepsze niż
                                               //wcale, ale to jeszcze nie to
                                if (sSpeedTable[i].iFlags & 0x100) // jeśli semafor
                                    if ((sSpeedTable[i].evEvent != eSignSkip) ?
                                            true :
                                            (sSpeedTable[i].fVelNext != 0.0)) // ale inny niż ten,
                                                                              // na którym minięto
                                                                              // S1, chyba że się
                                                                              // już zmieniło
                                        iDrivigFlags &= ~moveVisibility; // sygnał zezwalający na
                                                                         // jazdę wyłącza jazdę na
                                                                         // widoczność (S1 na SBL)
                                sSpeedTable[i].iFlags =
                                    0; // to można usunąć (nie mogą być usuwane w skanowaniu)
                            }
                        }
                        else if (sSpeedTable[i].evEvent->StopCommand())
                        { // jeśli prędkość jest zerowa, a komórka zawiera komendę
                            eSignNext = sSpeedTable[i].evEvent; // dla informacji
                            if (iDrivigFlags &
                                moveStopHere) // jeśli ma stać, dostaje komendę od razu
                                go = cm_Command; // komenda z komórki, do wykonania po zatrzymaniu
                            else if (sSpeedTable[i].fDist <= 20.0) // jeśli ma dociągnąć, to niech
                                                                   // dociąga (moveStopCloser
                                                                   // dotyczy dociągania do W4, nie
                                                                   // semafora)
                                go = cm_Command; // komenda z komórki, do wykonania po zatrzymaniu
                        }
                } // jeśli nie ma zawalidrogi
            } // jeśli event
            if (v >= 0.0)
            { // pozycje z prędkością -1 można spokojnie pomijać
                d = sSpeedTable[i].fDist;
                if ((sSpeedTable[i].iFlags & 0x20) ?
                        false :
                        d > 0.0) // sygnał lub ograniczenie z przodu (+32=przejechane)
                { // 2014-02: jeśli stoi, a ma do przejechania kawałek, to niech jedzie
                    if ((mvOccupied->Vel == 0.0) ?
                            ((sSpeedTable[i].iFlags & 0x501) == 0x501) && (d > fMaxProximityDist) :
                            false)
                        a = (iDrivigFlags & moveStopCloser) ? fAcc : 0.0; // ma podjechać bliżej -
                                                                          // czy na pewno w tym
                                                                          // miejscu taki warunek?
                    else
                    {
                        a = (v * v - mvOccupied->Vel * mvOccupied->Vel) /
                            (25.92 * d); // przyspieszenie: ujemne, gdy trzeba hamować
                        if (d < fMinProximityDist) // jak jest już blisko
                            if (v < fVelDes)
                                fVelDes = v; // ograniczenie aktualnej prędkości
                    }
                }
                else if (sSpeedTable[i].iFlags & 2) // jeśli tor
                { // tor ogranicza prędkość, dopóki cały skład nie przejedzie,
                    // d=fLength+d; //zamiana na długość liczoną do przodu
                    if (v >= 1.0) // EU06 się zawieszało po dojechaniu na koniec toru postojowego
                        if (d < -fLength)
                            continue; // zapętlenie, jeśli już wyjechał za ten odcinek
                    if (v < fVelDes)
                        fVelDes =
                            v; // ograniczenie aktualnej prędkości aż do wyjechania za ograniczenie
                    // if (v==0.0) fAcc=-0.9; //hamowanie jeśli stop
                    continue; // i tyle wystarczy
                }
                else // event trzyma tylko jeśli VelNext=0, nawet po przejechaniu (nie powinno
                     // dotyczyć samochodów?)
                    a = (v == 0.0 ? -1.0 : fAcc); // ruszanie albo hamowanie
                if (a < fAcc)
                { // mniejsze przyspieszenie to mniejsza możliwość rozpędzenia się albo konieczność
                  // hamowania
                    // jeśli droga wolna, to może być a>1.0 i się tu nie załapuje
                    // if (mvOccupied->Vel>10.0)
                    fAcc = a; // zalecane przyspieszenie (nie musi być uwzględniane przez AI)
                    fNext = v; // istotna jest prędkość na końcu tego odcinka
                    fDist = d; // dlugość odcinka
                }
                else if ((fAcc > 0) && (v > 0) && (v <= fNext))
                { // jeśli nie ma wskazań do hamowania, można podać drogę i prędkość na jej końcu
                    fNext = v; // istotna jest prędkość na końcu tego odcinka
                    fDist = d; // dlugość odcinka (kolejne pozycje mogą wydłużać drogę, jeśli
                               // prędkość jest stała)
                }
            } // if (v>=0.0)
            if (fNext >= 0.0)
            { // jeśli ograniczenie
                if ((sSpeedTable[i].iFlags & 0x101) == 0x101) // tylko sygnał przypisujemy
                    if (!eSignNext) // jeśli jeszcze nic nie zapisane tam
                        eSignNext = sSpeedTable[i].evEvent; // dla informacji
                if (fNext == 0.0)
                    break; // nie ma sensu analizować tabelki dalej
            }
        } // if (sSpeedTable[i].iFlags&1)
    } // for
    return go;
};

void TController::TablePurger()
{ // odtykacz: usuwa mniej istotne pozycje ze środka tabelki, aby uniknąć zatkania
    //(np. brak ograniczenia pomiędzy zwrotnicami, usunięte sygnały, minięte odcinki łuku)
    int i, j, k = iLast - iFirst; // może być 15 albo 16 pozycji, ostatniej nie ma co sprawdzać
    if (k < 0)
        k += iSpeedTableSize; // ilość pozycji do przeanalizowania
    for (i = iFirst; k > 0; --k, i = (i + 1) % iSpeedTableSize)
    { // sprawdzenie rekordów od (iFirst) do (iLast), o ile są istotne
        if ((sSpeedTable[i].iFlags & 1) ?
                (sSpeedTable[i].fVelNext < 0) && ((sSpeedTable[i].iFlags & 0xAB) == 0xA3) :
                true)
        { // jeśli jest to minięty (0x20) tor (0x03) do liczenia cięciw (0x80), a nie zwrotnica
          // (0x08)
            for (; k > 0; --k, i = (i + 1) % iSpeedTableSize)
                sSpeedTable[i] = sSpeedTable[(i + 1) % iSpeedTableSize]; // skopiowanie
            // WriteLog("Odtykacz usuwa pozycję");
            iLast = (iLast - 1 + iSpeedTableSize) % iSpeedTableSize; // cofnięcie z zawinięciem
            return;
        }
    }
    // jeśli powyższe odtykane nie pomoże, można usunąć coś więcej, albo powiększyć tabelkę
    TSpeedPos *t = new TSpeedPos[iSpeedTableSize + 16]; // zwiększenie
    k = iLast - iFirst + 1; // tym razem wszystkie
    if (k < 0)
        k += iSpeedTableSize; // ilość pozycji do przeanalizowania
    for (j = -1, i = iFirst; k > 0; --k)
    { // przepisywanie rekordów iFirst..iLast na 0..k
        t[++j] = sSpeedTable[i];
        i = (i + 1) % iSpeedTableSize; // kolejna pozycja mogą być zawinięta
    }
    iFirst = 0; // teraz będzie od zera
    iLast = j; // ostatnia
    delete[] sSpeedTable; // to już nie potrzebne
    sSpeedTable = t; // bo jest nowe
    iSpeedTableSize += 16;
    // WriteLog("Tabelka powiększona do "+AnsiString(iSpeedTableSize)+" pozycji");
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

__fastcall TController::TController(bool AI, TDynamicObject *NewControll, bool InitPsyche,
                                    bool primary // czy ma aktywnie prowadzić?
                                    )
{
    iEngineActive = 0;
    LastUpdatedTime = 0.0;
    ElapsedTime = 0.0;
    // inicjalizacja zmiennych
    Psyche = InitPsyche;
    VelDesired = 0.0; // prędkosć początkowa
    VelforDriver = -1;
    LastReactionTime = 0.0;
    HelpMeFlag = false;
    // fProximityDist=1; //nie używane
    ActualProximityDist = 1;
    vCommandLocation.x = 0;
    vCommandLocation.y = 0;
    vCommandLocation.z = 0;
    VelSignal = 0.0; // normalnie na początku ma stać, no chyba że jedzie
    VelLimit = -1.0; // brak ograniczenia prędkości
    VelNext = 120.0;
    AIControllFlag = AI;
    pVehicle = NewControll;
    ControllingSet(); // utworzenie połączenia do sterowanego pojazdu
    pVehicles[0] = pVehicle->GetFirstDynamic(0); // pierwszy w kierunku jazdy (Np. Pc1)
    pVehicles[1] = pVehicle->GetFirstDynamic(1); // ostatni w kierunku jazdy (końcówki)
    /*
     switch (mvOccupied->CabNo)
     {
      case -1: SendCtrlBroadcast("CabActivisation",1); break;
      case  1: SendCtrlBroadcast("CabActivisation",2); break;
      default: AIControllFlag:=False; //na wszelki wypadek
     }
    */
    iDirection = 0;
    iDirectionOrder = mvOccupied->CabNo; // 1=do przodu (w kierunku sprzęgu 0)
    VehicleName = mvOccupied->Name;
    // TrainParams=NewTrainParams;
    // if (TrainParams)
    // asNextStop=TrainParams->NextStop();
    // else
    TrainParams = new TTrainParameters("none"); // rozkład jazdy
    // OrderCommand="";
    // OrderValue=0;
    OrdersClear();
    MaxVelFlag = false;
    MinVelFlag = false; // Ra: to nie jest używane
    iDriverFailCount = 0;
    Need_TryAgain = false; // true, jeśli druga pozycja w elektryku nie załapała
    Need_BrakeRelease = true;
    deltalog = 0.05; // 1.0;

    if (WriteLogFlag)
    {
        mkdir("physicslog\\");
        LogFile.open(AnsiString("physicslog\\" + VehicleName + ".dat").c_str(),
                     std::ios::in | std::ios::out | std::ios::trunc);
#if LOGPRESS == 0
        LogFile << AnsiString(" Time [s]   Velocity [m/s]  Acceleration [m/ss]   Coupler.Dist[m]  "
                              "Coupler.Force[N]  TractionForce [kN]  FrictionForce [kN]   "
                              "BrakeForce [kN]    BrakePress [MPa]   PipePress [MPa]   "
                              "MotorCurrent [A]    MCP SCP BCP LBP DmgFlag Command CVal1 CVal2")
                       .c_str() << "\r\n";
#endif
#if LOGPRESS == 1
        LogFile << AnsiString("t\tVel\tAcc\tPP\tVVP\tBP\tBVP\tCVP").c_str() << "\n";
#endif
        LogFile.flush();
    }
    /*
      if (WriteLogFlag)
      {
       assignfile(AILogFile,VehicleName+".txt");
       rewrite(AILogFile);
       writeln(AILogFile,"AI driver log: started OK");
       close(AILogFile);
      }
    */

    // VelMargin=2; //Controlling->Vmax*0.015;
    fWarningDuration = 0.0; // nic do wytrąbienia
    WaitingExpireTime = 31.0; // tyle ma czekać, zanim się ruszy
    WaitingTime = 0.0;
    fMinProximityDist = 30.0; // stawanie między 30 a 60 m przed przeszkodą
    fMaxProximityDist = 50.0;
    iVehicleCount = -2; // wartość neutralna
    // Prepare2press=false; //bez dociskania
    eStopReason = stopSleep; // na początku śpi
    fLength = 0.0;
    fMass = 0.0; //[kg]
    eSignNext = NULL; // sygnał zmieniający prędkość, do pokazania na [F2]
    fShuntVelocity = 40; // domyślna prędkość manewrowa
    fStopTime = 0.0; // czas postoju przed dalszą jazdą (np. na przystanku)
    iDrivigFlags = moveStopPoint; // podjedź do W4 możliwie blisko
    iDrivigFlags |= moveStopHere; // nie podjeżdżaj do semafora, jeśli droga nie jest wolna
    iDrivigFlags |= moveStartHorn; // podaj sygnał po podaniu wolnej drogi
    if (primary)
        iDrivigFlags |= movePrimary; // aktywnie prowadzące pojazd
    Ready = false;
    if (mvOccupied->CategoryFlag & 2)
    { // samochody: na podst. http://www.prawko-kwartnik.info/hamowanie.html
        // fDriverBraking=0.0065; //mnożone przez (v^2+40*v) [km/h] daje prawie drogę hamowania [m]
        fDriverBraking = 0.03; // coś nie hamują te samochody zbyt dobrze
        fDriverDist = 5.0; // 5m - zachowywany odstęp przed kolizją
        fVelPlus = 10.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
        fVelMinus = 2.0; // margines prędkości powodujący załączenie napędu
    }
    else
    { // pociągi i statki
        fDriverBraking = 0.06; // mnożone przez (v^2+40*v) [km/h] daje prawie drogę hamowania [m]
        fDriverDist = 50.0; // 50m - zachowywany odstęp przed kolizją
        fVelPlus = 5.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
        fVelMinus = 5.0; // margines prędkości powodujący załączenie napędu
    }
    SetDriverPsyche(); // na końcu, bo wymaga ustawienia zmiennych
    AccDesired = AccPreferred;
    fVelMax = -1; // ustalenie prędkości dla składu
    fBrakeTime = 0.0; // po jakim czasie przekręcić hamulec
    iVehicles = 0; // na wszelki wypadek
    iSpeedTableSize = 16;
    sSpeedTable = new TSpeedPos[iSpeedTableSize];
    TableClear();
    iRadioChannel = 1; // numer aktualnego kanału radiowego
    fActionTime = 0.0;
    eAction = actSleep;
    tsGuardSignal = NULL; // komunikat od kierownika
    iGuardRadio = 0; // nie przez radio
    iStationStart = 0; // nic?
    // fAccThreshold może podlegać uczeniu się - hamowanie powinno być rejestrowane, a potem
    // analizowane
    fAccThreshold =
        (mvOccupied->TrainType & dt_EZT) ? -0.6 : -0.2; // próg opóźnienia dla zadziałania hamulca
    fLastStopExpDist = -1.0f;
    iRouteWanted = 3; // powiedzmy, że ma jechać prosto (1=w lewo)
    iCoupler = 0; // sprzęg; niezerowy gdy ma być podłączanie; samo podłączanie w trybie Connect
                  // (wcześniej może być np. Prepare_engine)
    fOverhead1 = 3000.0; // informacja o napięciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
    fOverhead2 = -1.0; // informacja o sposobie jazdy (-1=normalnie, 0=bez prądu, >0=z opuszczonym i
                       // ograniczeniem prędkości)
    iOverheadZero = 0; // suma bitowa jezdy bezprądowej, bity ustawiane przez pojazdy z
                       // podniesionymi pantografami
    iOverheadDown = 0; // suma bitowa opuszczenia pantografów, bity ustawiane przez pojazdy z
                       // podniesionymi pantografami
    fAccDesiredAv = 0.0; // uśrednione przyspieszenie z kolejnych przebłysków świadomości, żeby
                         // ograniczyć migotanie
    fVoltage = 0.0; // uśrednione napięcie sieci: przy spadku poniżej wartości minimalnej opóźnić
                    // rozruch o losowy czas
};

void TController::CloseLog()
{
    if (WriteLogFlag)
    {
        LogFile.close();
        // if WriteLogFlag)
        // CloseFile(AILogFile);
        /*  append(AIlogFile);
          writeln(AILogFile,ElapsedTime5:2,": QUIT");
          close(AILogFile); */
    }
};

__fastcall TController::~TController()
{ // wykopanie mechanika z roboty
    delete tsGuardSignal;
    delete TrainParams;
    delete[] sSpeedTable;
    CloseLog();
};

AnsiString TController::Order2Str(TOrders Order)
{ // zamiana kodu rozkazu na opis
    if (Order & Change_direction)
        return "Change_direction"; // może być nałożona na inną i wtedy ma priorytet
    if (Order == Wait_for_orders)
        return "Wait_for_orders";
    if (Order == Prepare_engine)
        return "Prepare_engine";
    if (Order == Shunt)
        return "Shunt";
    if (Order == Connect)
        return "Connect";
    if (Order == Disconnect)
        return "Disconnect";
    if (Order == Obey_train)
        return "Obey_train";
    if (Order == Release_engine)
        return "Release_engine";
    if (Order == Jump_to_first_order)
        return "Jump_to_first_order";
    /* Ra: wersja ze switch nie działa prawidłowo (czemu?)
     switch (Order)
     {
      Wait_for_orders:     return "Wait_for_orders";
      Prepare_engine:      return "Prepare_engine";
      Shunt:               return "Shunt";
      Change_direction:    return "Change_direction";
      Obey_train:          return "Obey_train";
      Release_engine:      return "Release_engine";
      Jump_to_first_order: return "Jump_to_first_order";
     }
    */
    return "Undefined!";
}

AnsiString TController::OrderCurrent()
{ // pobranie aktualnego rozkazu celem wyświetlenia
    return AnsiString(OrderPos) + ". " + Order2Str(OrderList[OrderPos]);
};

void TController::OrdersClear()
{ // czyszczenie tabeli rozkazów na starcie albo po dojściu do końca
    OrderPos = 0;
    OrderTop = 1; // szczyt stosu rozkazów
    for (int b = 0; b < maxorders; b++)
        OrderList[b] = Wait_for_orders;
#if LOGORDERS
    WriteLog("--> OrdersClear");
#endif
};

void TController::Activation()
{ // umieszczenie obsady w odpowiednim członie, wykonywane wyłącznie gdy steruje AI
    iDirection = iDirectionOrder; // kierunek (względem sprzęgów pojazdu z AI) właśnie został
                                  // ustalony (zmieniony)
    if (iDirection)
    { // jeśli jest ustalony kierunek
        TDynamicObject *old = pVehicle, *d = pVehicle; // w tym siedzi AI
        TController *drugi; // jakby były dwa, to zamienić miejscami, a nie robić wycieku pamięci
                            // poprzez nadpisanie
        int brake = mvOccupied->LocalBrakePos;
        while (mvControlling->MainCtrlPos) // samo zapętlenie DecSpeed() nie wystarcza :/
            DecSpeed(true); // wymuszenie zerowania nastawnika jazdy
        while (mvOccupied->ActiveDir < 0)
            mvOccupied->DirectionForward(); // kierunek na 0
        while (mvOccupied->ActiveDir > 0)
            mvOccupied->DirectionBackward();
        if (TestFlag(d->MoverParameters->Couplers[iDirectionOrder < 0 ? 1 : 0].CouplingFlag,
                     ctrain_controll))
        {
            mvControlling->MainSwitch(
                false); // dezaktywacja czuwaka, jeśli przejście do innego członu
            mvOccupied->DecLocalBrakeLevel(10); // zwolnienie hamulca w opuszczanym pojeździe
            //   mvOccupied->BrakeLevelSet((mvOccupied->BrakeHandle==FVel6)?4:-2); //odcięcie na
            //   zaworze maszynisty, FVel6 po drugiej stronie nie luzuje
            mvOccupied->BrakeLevelSet(
                mvOccupied->Handle->GetPos(bh_NP)); // odcięcie na zaworze maszynisty
        }
        mvOccupied->ActiveCab = mvOccupied->CabNo; // użytkownik moze zmienić ActiveCab wychodząc
        mvOccupied->CabDeactivisation(); // tak jest w Train.cpp
        // przejście AI na drugą stronę EN57, ET41 itp.
        while (TestFlag(d->MoverParameters->Couplers[iDirection < 0 ? 1 : 0].CouplingFlag,
                        ctrain_controll))
        { // jeśli pojazd z przodu jest ukrotniony, to przechodzimy do niego
            d = iDirection * d->DirectionGet() < 0 ? d->Next() :
                                                     d->Prev(); // przechodzimy do następnego członu
            if (d)
            {
                drugi = d->Mechanik; // zapamiętanie tego, co ewentualnie tam siedzi, żeby w razie
                                     // dwóch zamienić miejscami
                d->Mechanik = this; // na razie bilokacja
                d->MoverParameters->SetInternalCommand(
                    "", 0, 0); // usunięcie ewentualnie zalegającej komendy (Change_direction?)
                if (d->DirectionGet() != pVehicle->DirectionGet()) // jeśli są przeciwne do siebie
                    iDirection = -iDirection; // to będziemy jechać w drugą stronę względem
                                              // zasiedzianego pojazdu
                pVehicle->Mechanik = drugi; // wsadzamy tego, co ewentualnie był (podwójna trakcja)
                pVehicle->MoverParameters->CabNo = 0; // wyłączanie kabin po drodze
                pVehicle->MoverParameters->ActiveCab = 0; // i zaznaczenie, że nie ma tam nikogo
                pVehicle = d; // a mechu ma nowy pojazd (no, człon)
            }
            else
                break; // jak koniec składu, to mechanik dalej nie idzie
        }
        if (pVehicle != old)
        { // jeśli zmieniony został pojazd prowadzony
            Global::pWorld->CabChange(old, pVehicle); // ewentualna zmiana kabiny użytkownikowi
            ControllingSet(); // utworzenie połączenia do sterowanego pojazdu (może się zmienić) -
                              // silnikowy dla EZT
        }
        if (mvControlling->EngineType ==
            DieselEngine) // dla 2Ls150 - przed ustawieniem kierunku - można zmienić tryb pracy
            if (mvControlling->ShuntModeAllow)
                mvControlling->CurrentSwitch(
                    (OrderList[OrderPos] & Shunt) ||
                    (fMass > 224000.0)); // do tego na wzniesieniu może nie dać rady na liniowym
        // Ra: to przełączanie poniżej jest tu bez sensu
        mvOccupied->ActiveCab =
            iDirection; // aktywacja kabiny w prowadzonym pojeżdzie (silnikowy może być odwrotnie?)
        // mvOccupied->CabNo=iDirection;
        // mvOccupied->ActiveDir=0; //żeby sam ustawił kierunek
        mvOccupied->CabActivisation(); // uruchomienie kabin w członach
        DirectionForward(true); // nawrotnik do przodu
        if (brake) // hamowanie tylko jeśli był wcześniej zahamowany (bo możliwe, że jedzie!)
            mvOccupied->IncLocalBrakeLevel(brake); // zahamuj jak wcześniej
        CheckVehicles(); // sprawdzenie składu, AI zapali światła
        TableClear(); // resetowanie tabelki skanowania torów
    }
};

void TController::AutoRewident()
{ // autorewident: nastawianie hamulców w składzie
    int r = 0, g = 0, p = 0; // ilości wagonów poszczególnych typów
    TDynamicObject *d = pVehicles[0]; // pojazd na czele składu
    // 1. Zebranie informacji o składzie pociągu — przejście wzdłuż składu i odczyt parametrów:
    //   · ilość wagonów -> są zliczane, wszystkich pojazdów jest (iVehicles)
    //   · długość (jako suma) -> jest w (fLength)
    //   · masa (jako suma) -> jest w (fMass)
    while (d)
    { // klasyfikacja pojazdów wg BrakeDelays i mocy (licznik)
        if (d->MoverParameters->Power <
            1) // - lokomotywa - Power>1 - ale może być nieczynna na końcu...
            if (TestFlag(d->MoverParameters->BrakeDelays, bdelay_R))
                ++r; // - wagon pospieszny - jest R
            else if (TestFlag(d->MoverParameters->BrakeDelays, bdelay_G))
                ++g; // - wagon towarowy - jest G (nie ma R)
            else
                ++p; // - wagon osobowy - reszta (bez G i bez R)
        d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
    }
    // 2. Określenie typu pociągu i nastawy:
    int ustaw; //+16 dla pasażerskiego
    if (r + g + p == 0)
        ustaw = 16 + bdelay_R; // lokomotywa luzem (może być wieloczłonowa)
    else
    { // jeśli są wagony
        ustaw = (g < min(4, r + p) ? 16 : 0);
        if (ustaw) // jeśli towarowe < Min(4, pospieszne+osobowe)
        { // to skład pasażerski - nastawianie pasażerskiego
            ustaw += (g && (r < g + p)) ? bdelay_P : bdelay_R;
            // jeżeli towarowe>0 oraz pospiesze<=towarowe+osobowe to P (0)
            // inaczej R (2)
        }
        else
        { // inaczej towarowy - nastawianie towarowego
            if ((fLength < 300.0) && (fMass < 600000.0)) //[kg]
                ustaw |= bdelay_P; // jeżeli długość<300 oraz masa<600 to P (0)
            else if ((fLength < 500.0) && (fMass < 1300000.0))
                ustaw |= bdelay_R; // jeżeli długość<500 oraz masa<1300 to GP (2)
            else
                ustaw |= bdelay_G; // inaczej G (1)
        }
        // zasadniczo na sieci PKP kilka lat temu na P/GP jeździły tylko kontenerowce o
        // rozkładowej 90 km/h. Pozostałe jeździły 70 km/h i były nastawione na G.
    }
    d = pVehicles[0]; // pojazd na czele składu
    p = 0; // będziemy tu liczyć wagony od lokomotywy dla nastawy GP
    while (d)
    { // 3. Nastawianie
        switch (ustaw)
        {
        case bdelay_P: // towarowy P - lokomotywa na G, reszta na P.
            d->MoverParameters->BrakeDelaySwitch(d->MoverParameters->Power > 1 ? bdelay_G :
                                                                                 bdelay_P);
            break;
        case bdelay_G: // towarowy G - wszystko na G, jeśli nie ma to P (powinno się wyłączyć
                       // hamulec)
            d->MoverParameters->BrakeDelaySwitch(
                TestFlag(d->MoverParameters->BrakeDelays, bdelay_G) ? bdelay_G : bdelay_P);
            break;
        case bdelay_R: // towarowy GP - lokomotywa oraz 5 pierwszych pojazdów przy niej na G, reszta
                       // na P
            if (d->MoverParameters->Power > 1)
            {
                d->MoverParameters->BrakeDelaySwitch(bdelay_G);
                p = 0; // a jak będzie druga w środku?
            }
            else
                d->MoverParameters->BrakeDelaySwitch(++p <= 5 ? bdelay_G : bdelay_P);
            break;
        case 16 + bdelay_R: // pasażerski R - na R, jeśli nie ma to P
            d->MoverParameters->BrakeDelaySwitch(
                TestFlag(d->MoverParameters->BrakeDelays, bdelay_R) ? bdelay_R : bdelay_P);
            break;
        case 16 + bdelay_P: // pasażerski P - wszystko na P
            d->MoverParameters->BrakeDelaySwitch(bdelay_P);
            break;
        }
        d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
    }
};

bool TController::CheckVehicles(TOrders user)
{ // sprawdzenie stanu posiadanych pojazdów w składzie i zapalenie świateł
    TDynamicObject *p; // roboczy wskaźnik na pojazd
    iVehicles = 0; // ilość pojazdów w składzie
    int d = mvOccupied->DirAbsolute; // który sprzęg jest z przodu
    if (!d) // jeśli nie ma ustalonego kierunku
        d = mvOccupied->CabNo; // to jedziemy wg aktualnej kabiny
    iDirection = d; // ustalenie kierunku jazdy (powinno zrobić PrepareEngine?)
    d = d >= 0 ? 0 : 1; // kierunek szukania czoła (numer sprzęgu)
    pVehicles[0] = p = pVehicle->FirstFind(d); // pojazd na czele składu
    // liczenie pojazdów w składzie i ustalenie parametrów
    int dir = d = 1 - d; // a dalej będziemy zliczać od czoła do tyłu
    fLength = 0.0; // długość składu do badania wyjechania za ograniczenie
    fMass = 0.0; // całkowita masa do liczenia stycznej składowej grawitacji
    fVelMax = -1; // ustalenie prędkości dla składu
    bool main = true; // czy jest głównym sterującym
    iDrivigFlags |= moveOerlikons; // zakładamy, że są same Oerlikony
    // Ra 2014-09: ustawić moveMultiControl, jeśli wszystkie są w ukrotnieniu (i skrajne mają
    // kabinę?)
    while (p)
    { // sprawdzanie, czy jest głównym sterującym, żeby nie było konfliktu
        if (p->Mechanik) // jeśli ma obsadę
            if (p->Mechanik != this) // ale chodzi o inny pojazd, niż aktualnie sprawdzający
                if (p->Mechanik->iDrivigFlags & movePrimary) // a tamten ma priorytet
                    if ((iDrivigFlags & movePrimary) && (mvOccupied->DirAbsolute) &&
                        (mvOccupied->BrakeCtrlPos >= -1)) // jeśli rządzi i ma kierunek
                        p->Mechanik->iDrivigFlags &= ~movePrimary; // dezaktywuje tamtego
                    else
                        main = false; // nici z rządzenia
        ++iVehicles; // jest jeden pojazd więcej
        pVehicles[1] = p; // zapamiętanie ostatniego
        fLength += p->MoverParameters->Dim.L; // dodanie długości pojazdu
        fMass += p->MoverParameters->TotalMass; // dodanie masy łącznie z ładunkiem
        if (fVelMax < 0 ? true : p->MoverParameters->Vmax < fVelMax)
            fVelMax = p->MoverParameters->Vmax; // ustalenie maksymalnej prędkości dla składu
        /* //youBy: bez przesady, to jest proteza, napelniac mozna, a nawet trzeba, ale z umiarem!
          //uwzględnić jeszcze wyłączenie hamulca
          if
          ((p->MoverParameters->BrakeSystem!=Pneumatic)&&(p->MoverParameters->BrakeSystem!=ElectroPneumatic))
           iDrivigFlags&=~moveOerlikons; //no jednak nie
          else if (p->MoverParameters->BrakeSubsystem!=Oerlikon)
           iDrivigFlags&=~moveOerlikons; //wtedy też nie */
        p = p->Neightbour(dir); // pojazd podłączony od wskazanej strony
    }
    if (main)
        iDrivigFlags |= movePrimary; // nie znaleziono innego, można się porządzić
    /* //tabelka z listą pojazdów jest na razie nie potrzebna
     delete[] pVehicles;
     pVehicles=new TDynamicObject*[iVehicles];
    */
    ControllingSet(); // ustalenie członu do sterowania (może być inny niż zasiedziany)
    int pantmask = 1;
    if (iDrivigFlags & movePrimary)
    { // jeśli jest aktywnie prowadzącym pojazd, może zrobić własny porządek
        p = pVehicles[0];
        while (p)
        {
            if (TrainParams)
                if (p->asDestination == "none")
                    p->DestinationSet(TrainParams->Relation2); // relacja docelowa, jeśli nie było
            if (AIControllFlag) // jeśli prowadzi komputer
                p->RaLightsSet(0, 0); // gasimy światła
            if (p->MoverParameters->EnginePowerSource.SourceType == CurrentCollector)
            { // jeśli pojazd posiada pantograf, to przydzielamy mu maskę, którą będzie informował o
              // jeździe bezprądowej
                p->iOverheadMask = pantmask;
                pantmask << 1; // przesunięcie bitów, max. 32 pojazdy z pantografami w składzie
            }
            d = p->DirectionSet(d ? 1 : -1); // zwraca położenie następnego (1=zgodny,0=odwrócony -
                                             // względem czoła składu)
            p->fScanDist = 300.0; // odległość skanowania w poszukiwaniu innych pojazdów
            p->ctOwner = this; // dominator oznacza swoje terytorium
            p = p->Next(); // pojazd podłączony od tyłu (licząc od czoła)
        }
        if (AIControllFlag)
        { // jeśli prowadzi komputer
            if (OrderCurrentGet() == Obey_train) // jeśli jazda pociągowa
            {
                Lights(1 + 4 + 16, 2 + 32 + 64); //światła pociągowe (Pc1) i końcówki (Pc5)
#if LOGPRESS == 0
                AutoRewident(); // nastawianie hamulca do jazdy pociągowej
#endif
            }
            else if (OrderCurrentGet() & (Shunt | Connect))
            {
                Lights(16, (pVehicles[1]->MoverParameters->CabNo) ?
                               1 :
                               0); //światła manewrowe (Tb1) na pojeździe z napędem
                if (OrderCurrentGet() & Connect) // jeśli łączenie, skanować dalej
                    pVehicles[0]->fScanDist =
                        5000.0; // odległość skanowania w poszukiwaniu innych pojazdów
            }
            else if (OrderCurrentGet() == Disconnect)
                if (mvOccupied->ActiveDir > 0) // jak ma kierunek do przodu
                    Lights(16, 0); //światła manewrowe (Tb1) tylko z przodu, aby nie pozostawić
                                   //odczepionego ze światłem
                else // jak dociska
                    Lights(0, 16); //światła manewrowe (Tb1) tylko z przodu, aby nie pozostawić
                                   //odczepionego ze światłem
        }
        else // Ra 2014-02: lepiej tu niż w pętli obsługującej komendy, bo tam się zmieni informacja
             // o składzie
            switch (user) // gdy człowiek i gdy nastąpiło połącznie albo rozłączenie
            {
            case Change_direction:
                while (OrderCurrentGet() & (Change_direction))
                    JumpToNextOrder(); // zmianę kierunku też można olać, ale zmienić kierunek
                                       // skanowania!
                break;
            case Connect:
                while (OrderCurrentGet() & (Change_direction))
                    JumpToNextOrder(); // zmianę kierunku też można olać, ale zmienić kierunek
                                       // skanowania!
                if (OrderCurrentGet() & (Connect))
                { // jeśli miało być łączenie, zakładamy, że jest dobrze (sprawdzić?)
                    iCoupler = 0; // koniec z doczepianiem
                    iDrivigFlags &= ~moveConnect; // zdjęcie flagi doczepiania
                    JumpToNextOrder(); // wykonanie następnej komendy
                    if (OrderCurrentGet() & (Change_direction))
                        JumpToNextOrder(); // zmianę kierunku też można olać, ale zmienić kierunek
                                           // skanowania!
                }
                break;
            case Disconnect:
                while (OrderCurrentGet() & (Change_direction))
                    JumpToNextOrder(); // zmianę kierunku też można olać, ale zmienić kierunek
                                       // skanowania!
                if (OrderCurrentGet() & (Disconnect))
                { // wypadało by sprawdzić, czy odczepiono wagony w odpowiednim miejscu
                  // (iVehicleCount)
                    JumpToNextOrder(); // wykonanie następnej komendy
                    if (OrderCurrentGet() & (Change_direction))
                        JumpToNextOrder(); // zmianę kierunku też można olać, ale zmienić kierunek
                                           // skanowania!
                }
            }
        // Ra 2014-09: tymczasowo prymitywne ustawienie warunku pod kątem SN61
        if ((mvOccupied->TrainType == dt_EZT) || (iVehicles == 1))
            iDrivigFlags |= movePushPull; // zmiana czoła przez zmianę kabiny
        else
            iDrivigFlags &= ~movePushPull; // zmiana czoła przez manewry
    } // blok wykonywany, gdy aktywnie prowadzi
    return true;
}

void TController::Lights(int head, int rear)
{ // zapalenie świateł w skłądzie
    pVehicles[0]->RaLightsSet(head, -1); // zapalenie przednich w pierwszym
    pVehicles[1]->RaLightsSet(-1, rear); // zapalenie końcówek w ostatnim
}

void TController::DirectionInitial()
{ // ustawienie kierunku po wczytaniu trainset (może jechać na wstecznym
    mvOccupied->CabActivisation(); // załączenie rozrządu (wirtualne kabiny)
    if (mvOccupied->Vel > 0.0)
    { // jeśli na starcie jedzie
        iDirection = iDirectionOrder =
            (mvOccupied->V > 0 ? 1 : -1); // początkowa prędkość wymusza kierunek jazdy
        DirectionForward(mvOccupied->V * mvOccupied->CabNo >= 0.0); // a dalej ustawienie nawrotnika
    }
    CheckVehicles(); // sprawdzenie świateł oraz skrajnych pojazdów do skanowania
};

int TController::OrderDirectionChange(int newdir, TMoverParameters *Vehicle)
{ // zmiana kierunku jazdy, niezależnie od kabiny
    int testd = newdir;
    if (Vehicle->Vel < 0.5)
    { // jeśli prawie stoi, można zmienić kierunek, musi być wykonane dwukrotnie, bo za pierwszym
      // razem daje na zero
        switch (newdir * Vehicle->CabNo)
        { // DirectionBackward() i DirectionForward() to zmiany względem kabiny
        case -1: // if (!Vehicle->DirectionBackward()) testd=0; break;
            DirectionForward(false);
            break;
        case 1: // if (!Vehicle->DirectionForward()) testd=0; break;
            DirectionForward(true);
            break;
        }
        if (testd == 0)
            VelforDriver = -1; // kierunek został zmieniony na żądany, można jechać
    }
    else // jeśli jedzie
        VelforDriver = 0; // ma się zatrzymać w celu zmiany kierunku
    if ((Vehicle->ActiveDir == 0) && (VelforDriver < Vehicle->Vel)) // Ra: to jest chyba bez sensu
        IncBrake(); // niech hamuje
    if (Vehicle->ActiveDir == testd * Vehicle->CabNo)
        VelforDriver = -1; // można jechać, bo kierunek jest zgodny z żądanym
    if (Vehicle->TrainType == dt_EZT)
        if (Vehicle->ActiveDir > 0)
            // if () //tylko jeśli jazda pociągowa (tego nie wiemy w momencie odpalania silnika)
            Vehicle->DirectionForward(); // Ra: z przekazaniem do silnikowego
    return (int)VelforDriver; // zwraca prędkość mechanika
}

void TController::WaitingSet(double Seconds)
{ // ustawienie odczekania po zatrzymaniu (ustawienie w trakcie jazdy zatrzyma)
    fStopTime = -Seconds; // ujemna wartość oznacza oczekiwanie (potem >=0.0)
}

void TController::SetVelocity(double NewVel, double NewVelNext, TStopReason r)
{ // ustawienie nowej prędkości
    WaitingTime = -WaitingExpireTime; // przypisujemy -WaitingExpireTime, a potem porównujemy z
                                      // zerem
    MaxVelFlag = False; // Ra: to nie jest używane
    MinVelFlag = False; // Ra: to nie jest używane
    /* nie używane
     if ((NewVel>NewVelNext) //jeśli oczekiwana większa niż następna
      || (NewVel<mvOccupied->Vel)) //albo aktualna jest mniejsza niż aktualna
      fProximityDist=-800.0; //droga hamowania do zmiany prędkości
     else
      fProximityDist=-300.0; //Ra: ujemne wartości są ignorowane
    */
    if (NewVel == 0.0) // jeśli ma stanąć
    {
        if (r != stopNone) // a jest powód podany
            eStopReason = r; // to zapamiętać nowy powód
    }
    else
    {
        eStopReason = stopNone; // podana prędkość, to nie ma powodów do stania
        // to całe poniżej to warunki zatrąbienia przed ruszeniem
        if (OrderList[OrderPos] ?
                OrderList[OrderPos] & (Obey_train | Shunt | Connect | Prepare_engine) :
                true) // jeśli jedzie w dowolnym trybie
            if ((mvOccupied->Vel <
                 1.0)) // jesli stoi (na razie, bo chyba powinien też, gdy hamuje przed semaforem)
                if (iDrivigFlags & moveStartHorn) // jezeli trąbienie włączone
                    if (!(iDrivigFlags & (moveStartHornDone | moveConnect))) // jeśli nie zatrąbione
                                                                             // i nie jest to moment
                                                                             // podłączania składu
                        if (mvOccupied->CategoryFlag & 1) // tylko pociągi trąbią (unimogi tylko na
                                                          // torach, więc trzeba raczej sprawdzać
                                                          // tor)
                            if ((NewVel >= 1.0) || (NewVel < 0.0)) // o ile prędkość jest znacząca
                            { // fWarningDuration=0.3; //czas trąbienia
                                // if (AIControllFlag) //jak siedzi krasnoludek, to włączy trąbienie
                                // mvOccupied->WarningSignal=pVehicle->iHornWarning; //wysokość tonu
                                // (2=wysoki)
                                // iDrivigFlags|=moveStartHornDone; //nie trąbić aż do ruszenia
                                iDrivigFlags |= moveStartHornNow; // zatrąb po odhamowaniu
                            }
    }
    VelSignal = NewVel; // prędkość zezwolona na aktualnym odcinku
    VelNext = NewVelNext; // prędkość przy następnym obiekcie
}

/* //funkcja do niczego nie potrzebna (ew. do przesunięcia pojazdu o odległość NewDist)
bool TController::SetProximityVelocity(double NewDist,double NewVelNext)
{//informacja o prędkości w pewnej odległości
#if 0
 if (NewVelNext==0.0)
  WaitingTime=0.0; //nie trzeba już czekać
 //if ((NewVelNext>=0)&&((VelNext>=0)&&(NewVelNext<VelNext))||(NewVelNext<VelSignal))||(VelNext<0))
 {MaxVelFlag=False; //Ra: to nie jest używane
  MinVelFlag=False; //Ra: to nie jest używane
  VelNext=NewVelNext;
  fProximityDist=NewDist; //dodatnie: przeliczyć do punktu; ujemne: wziąć dosłownie
  return true;
 }
 //else return false
#endif
}
*/

void TController::SetDriverPsyche()
{
    // double maxdist=0.5; //skalowanie dystansu od innego pojazdu, zmienic to!!!
    if ((Psyche == Aggressive) && (OrderList[OrderPos] == Obey_train))
    {
        ReactionTime = HardReactionTime; // w zaleznosci od charakteru maszynisty
        // if (pOccupied)
        if (mvOccupied->CategoryFlag & 2)
        {
            WaitingExpireTime = 1; // tyle ma czekać samochód, zanim się ruszy
            AccPreferred = 3.0; //[m/ss] agresywny
        }
        else
        {
            WaitingExpireTime = 61; // tyle ma czekać, zanim się ruszy
            AccPreferred = HardAcceleration; // agresywny
        }
    }
    else
    {
        ReactionTime = EasyReactionTime; // spokojny
        if (mvOccupied->CategoryFlag & 2)
        {
            WaitingExpireTime = 3; // tyle ma czekać samochód, zanim się ruszy
            AccPreferred = 2.0; //[m/ss]
        }
        else
        {
            WaitingExpireTime = 65; // tyle ma czekać, zanim się ruszy
            AccPreferred = EasyAcceleration;
        }
    }
    if (mvControlling && mvOccupied)
    { // with Controlling do
        if (mvControlling->MainCtrlPos < 3)
            ReactionTime = mvControlling->InitialCtrlDelay + ReactionTime;
        if (mvOccupied->BrakeCtrlPos > 1)
            ReactionTime = 0.5 * ReactionTime;
        /*
          if (mvOccupied->Vel>0.1) //o ile jedziemy
          {//sprawdzenie jazdy na widoczność
           TCoupling
          *c=pVehicles[0]->MoverParameters->Couplers+(pVehicles[0]->DirectionGet()>0?0:1); //sprzęg
          z przodu składu
           if (c->Connected) //a mamy coś z przodu
            if (c->CouplingFlag==0) //jeśli to coś jest podłączone sprzęgiem wirtualnym
            {//wyliczanie optymalnego przyspieszenia do jazdy na widoczność (Ra: na pewno tutaj?)
             double k=c->Connected->Vel; //prędkość pojazdu z przodu (zakładając, że jedzie w tę
          samą stronę!!!)
             if (k<=mvOccupied->Vel) //porównanie modułów prędkości [km/h]
             {if (pVehicles[0]->fTrackBlock<fMaxProximityDist) //porównianie z minimalną odległością
          kolizyjną
               k=-AccPreferred; //hamowanie maksymalne, bo jest za blisko
              else
              {//jeśli tamten jedzie szybciej, to nie potrzeba modyfikować przyspieszenia
               double d=(pVehicles[0]->fTrackBlock-0.5*fabs(mvOccupied->V)-fMaxProximityDist);
          //bezpieczna odległość za poprzednim
               //a=(v2*v2-v1*v1)/(25.92*(d-0.5*v1))
               //(v2*v2-v1*v1)/2 to różnica energii kinetycznych na jednostkę masy
               //jeśli v2=50km/h,v1=60km/h,d=200m => k=(192.9-277.8)/(25.92*(200-0.5*16.7)=-0.0171
          [m/s^2]
               //jeśli v2=50km/h,v1=60km/h,d=100m => k=(192.9-277.8)/(25.92*(100-0.5*16.7)=-0.0357
          [m/s^2]
               //jeśli v2=50km/h,v1=60km/h,d=50m  => k=(192.9-277.8)/(25.92*( 50-0.5*16.7)=-0.0786
          [m/s^2]
               //jeśli v2=50km/h,v1=60km/h,d=25m  => k=(192.9-277.8)/(25.92*( 25-0.5*16.7)=-0.1967
          [m/s^2]
               if (d>0) //bo jak ujemne, to zacznie przyspieszać, aby się zderzyć
                k=(k*k-mvOccupied->Vel*mvOccupied->Vel)/(25.92*d); //energia kinetyczna dzielona
          przez masę i drogę daje przyspieszenie
               else
                k=0.0; //może lepiej nie przyspieszać -AccPreferred; //hamowanie
               //WriteLog(pVehicle->asName+" "+AnsiString(k));
              }
              if (d<fBrakeDist) //bo z daleka nie ma co hamować
               AccPreferred=Min0R(k,AccPreferred);
             }
            }
          }
        */
    }
};

bool TController::PrepareEngine()
{ // odpalanie silnika
    bool OK;
    bool voltfront, voltrear;
    voltfront = false;
    voltrear = false;
    LastReactionTime = 0.0;
    ReactionTime = PrepareTime;
    iDrivigFlags |= moveActive; // może skanować sygnały i reagować na komendy
    // with Controlling do
    if (((mvControlling->EnginePowerSource.SourceType ==
          CurrentCollector) /*||(mvOccupied->TrainType==dt_EZT)*/))
    {
        if (mvControlling
                ->GetTrainsetVoltage()) // sprawdzanie, czy zasilanie jest może w innym członie
        {
            voltfront = true;
            voltrear = true;
        }
    }
    //   begin
    //     if Couplers[0].Connected<>nil)
    //     begin
    //       if Couplers[0].Connected^.PantFrontVolt or Couplers[0].Connected^.PantRearVolt)
    //         voltfront:=true
    //       else
    //         voltfront:=false;
    //     end
    //     else
    //        voltfront:=false;
    //     if Couplers[1].Connected<>nil)
    //     begin
    //      if Couplers[1].Connected^.PantFrontVolt or Couplers[1].Connected^.PantRearVolt)
    //        voltrear:=true
    //      else
    //        voltrear:=false;
    //     end
    //     else
    //        voltrear:=false;
    //   end
    else
        // if EnginePowerSource.SourceType<>CurrentCollector)
        if (mvOccupied->TrainType != dt_EZT)
        voltfront = true; // Ra 2014-06: to jest wirtualny prąd dla spalinowych???
    if (AIControllFlag) // jeśli prowadzi komputer
    { // część wykonawcza dla sterowania przez komputer
        mvOccupied->BatterySwitch(true);
        if (mvControlling->EnginePowerSource.SourceType == CurrentCollector)
        { // jeśli silnikowy jest pantografującym
            if (mvControlling->PantPress > 4.3)
            { // jeżeli jest wystarczające ciśnienie w pantografach
                if ((!mvControlling->bPantKurek3) ||
                    (mvControlling->PantPress <=
                     mvControlling->ScndPipePress)) // kurek przełączony albo główna już pompuje
                    mvControlling->PantCompFlag = false; // sprężarkę pantografów można już wyłączyć
                mvControlling->PantFront(true);
                mvControlling->PantRear(true);
            }
            else if (mvControlling->PantPress < 4.2) //żeby nie załączał zaraz po przekroczeniu 4.0
            { // załączenie małej sprężarki
                mvControlling->bPantKurek3 =
                    false; // odłączenie zbiornika głównego, bo z nim nie da rady napompować
                mvControlling->PantCompFlag = true; // załączenie sprężarki pantografów
            }
        }
        // if (mvOccupied->TrainType==dt_EZT)
        //{//Ra 2014-12: po co to tutaj?
        // mvControlling->PantFront(true);
        // mvControlling->PantRear(true);
        //}
        // if (mvControlling->EngineType==DieselElectric)
        // mvControlling->Battery=true; //Ra: to musi być tak?
    }
    if (mvControlling->PantFrontVolt || mvControlling->PantRearVolt || voltfront || voltrear)
    { // najpierw ustalamy kierunek, jeśli nie został ustalony
        if (!iDirection) // jeśli nie ma ustalonego kierunku
            if (mvOccupied->V == 0)
            { // ustalenie kierunku, gdy stoi
                iDirection = mvOccupied->CabNo; // wg wybranej kabiny
                if (!iDirection) // jeśli nie ma ustalonego kierunku
                    if ((mvControlling->PantFrontVolt != 0.0) ||
                        (mvControlling->PantRearVolt != 0.0) || voltfront || voltrear)
                    {
                        if (mvOccupied->Couplers[1].CouplingFlag ==
                            ctrain_virtual) // jeśli z tyłu nie ma nic
                            iDirection = -1; // jazda w kierunku sprzęgu 1
                        if (mvOccupied->Couplers[0].CouplingFlag ==
                            ctrain_virtual) // jeśli z przodu nie ma nic
                            iDirection = 1; // jazda w kierunku sprzęgu 0
                    }
            }
            else // ustalenie kierunku, gdy jedzie
                if ((mvControlling->PantFrontVolt != 0.0) || (mvControlling->PantRearVolt != 0.0) ||
                    voltfront || voltrear)
                if (mvOccupied->V < 0) // jedzie do tyłu
                    iDirection = -1; // jazda w kierunku sprzęgu 1
                else // jak nie do tyłu, to do przodu
                    iDirection = 1; // jazda w kierunku sprzęgu 0
        if (AIControllFlag) // jeśli prowadzi komputer
        { // część wykonawcza dla sterowania przez komputer
            if (mvControlling->ConvOvldFlag)
            { // wywalił bezpiecznik nadmiarowy przetwornicy
                while (DecSpeed(true))
                    ; // zerowanie napędu
                mvControlling->ConvOvldFlag = false; // reset nadmiarowego
            }
            else if (!mvControlling->Mains)
            {
                // if TrainType=dt_SN61)
                //   begin
                //      OK:=(OrderDirectionChange(ChangeDir,Controlling)=-1);
                //      OK:=IncMainCtrl(1);
                //   end;
                while (DecSpeed(true))
                    ; // zerowanie napędu
                OK = mvControlling->MainSwitch(true);
                if (mvControlling->EngineType == DieselEngine)
                { // Ra 2014-06: dla SN61 trzeba wrzucić pierwszą pozycję - nie wiem, czy tutaj...
                  // kiedyś działało...
                    if (!mvControlling->MainCtrlPos)
                    {
                        if (mvControlling->RList[0].R ==
                            0.0) // gdy na pozycji 0 dawka paliwa jest zerowa, to zgaśnie
                            mvControlling->IncMainCtrl(1); // dlatego trzeba zwiększyć pozycję
                        if (!mvControlling->ScndCtrlPos) // jeśli bieg nie został ustawiony
                            if (!mvControlling->MotorParam[0].AutoSwitch) // gdy biegi ręczne
                                if (mvControlling->MotorParam[0].mIsat == 0.0) // bl,mIsat,fi,mfi
                                    mvControlling->IncScndCtrl(1); // pierwszy bieg
                    }
                }
            }
            else
            { // Ra: iDirection określa, w którą stronę jedzie skład względem sprzęgów pojazdu z AI
                OK = (OrderDirectionChange(iDirection, mvOccupied) == -1);
                // w EN57 sprężarka w ra jest zasilana z silnikowego
                mvControlling->CompressorSwitch(true);
                mvControlling->ConverterSwitch(true);
                mvControlling->CompressorSwitch(true);
            }
        }
        else
            OK = mvControlling->Mains;
    }
    else
        OK = false;
    OK = OK && (mvOccupied->ActiveDir != 0) && (mvControlling->CompressorAllow);
    if (OK)
    {
        if (eStopReason == stopSleep) // jeśli dotychczas spał
            eStopReason == stopNone; // teraz nie ma powodu do stania
        iEngineActive = 1;
        return true;
    }
    else
    {
        iEngineActive = 0;
        return false;
    }
};

bool TController::ReleaseEngine()
{ // wyłączanie silnika (test wyłączenia, a część wykonawcza tylko jeśli steruje komputer)
    bool OK = false;
    LastReactionTime = 0.0;
    ReactionTime = PrepareTime;
    if (AIControllFlag)
    { // jeśli steruje komputer
        if (mvOccupied->DoorOpenCtrl == 1)
        { // zamykanie drzwi
            if (mvOccupied->DoorLeftOpened)
                mvOccupied->DoorLeft(false);
            if (mvOccupied->DoorRightOpened)
                mvOccupied->DoorRight(false);
        }
        if (mvOccupied->ActiveDir == 0)
            if (mvControlling->Mains)
            {
                mvControlling->CompressorSwitch(false);
                mvControlling->ConverterSwitch(false);
                if (mvControlling->EnginePowerSource.SourceType == CurrentCollector)
                {
                    mvControlling->PantFront(false);
                    mvControlling->PantRear(false);
                }
                OK = mvControlling->MainSwitch(false);
            }
            else
                OK = true;
    }
    else if (mvOccupied->ActiveDir == 0)
        OK = mvControlling->Mains; // tylko to testujemy dla pojazdu człowieka
    if (AIControllFlag)
        if (!mvOccupied->DecBrakeLevel()) // tu moze zmieniać na -2, ale to bez znaczenia
            if (!mvOccupied->IncLocalBrakeLevel(1))
            {
                while (DecSpeed(true))
                    ; // zerowanie nastawników
                while (mvOccupied->ActiveDir > 0)
                    mvOccupied->DirectionBackward();
                while (mvOccupied->ActiveDir < 0)
                    mvOccupied->DirectionForward();
            }
    OK = OK && (mvOccupied->Vel < 0.01);
    if (OK)
    { // jeśli się zatrzymał
        iEngineActive = 0;
        eStopReason = stopSleep; // stoimy z powodu wyłączenia
        eAction = actSleep; //śpi (wygaszony)
        if (AIControllFlag)
        {
            Lights(0, 0); // gasimy światła
            mvOccupied->BatterySwitch(false);
        }
        OrderNext(Wait_for_orders); //żeby nie próbował coś robić dalej
        TableClear(); // zapominamy ograniczenia
        iDrivigFlags &= ~moveActive; // ma nie skanować sygnałów i nie reagować na komendy
    }
    return OK;
}

bool TController::IncBrake()
{ // zwiększenie hamowania
    bool OK = false;
    switch (mvOccupied->BrakeSystem)
    {
    case Individual:
        OK = mvOccupied->IncLocalBrakeLevel(1 + floor(0.5 + fabs(AccDesired)));
        break;
    case Pneumatic:
        if ((mvOccupied->Couplers[0].Connected == NULL) &&
            (mvOccupied->Couplers[1].Connected == NULL))
            OK = mvOccupied->IncLocalBrakeLevel(
                1 + floor(0.5 + fabs(AccDesired))); // hamowanie lokalnym bo luzem jedzie
        else
        {
            if (mvOccupied->BrakeCtrlPos + 1 == mvOccupied->BrakeCtrlPosNo)
            {
                if (AccDesired < -1.5) // hamowanie nagle
                    OK = mvOccupied->IncBrakeLevel();
                else
                    OK = false;
            }
            else
            {
                /*
                    if (AccDesired>-0.2) and ((Vel<20) or (Vel-VelNext<10)))
                            begin
                              if BrakeCtrlPos>0)
                               OK:=IncBrakeLevel
                              else;
                               OK:=IncLocalBrakeLevel(1);   //finezyjne hamowanie lokalnym
                             end
                           else
                */
                // dodane dla towarowego
                if (mvOccupied->BrakeDelayFlag == bdelay_G ?
                        -AccDesired * 6.6 > Min0R(2, mvOccupied->BrakeCtrlPos) :
                        true)
                {
                    OK = mvOccupied->IncBrakeLevel();
                }
                else
                    OK = false;
            }
        }
        if (mvOccupied->BrakeCtrlPos > 0)
            mvOccupied->BrakeReleaser(0);
        break;
    case ElectroPneumatic:
        if (mvOccupied->fBrakeCtrlPos != mvOccupied->Handle->GetPos(bh_EPB))
        {
            mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EPB));
            if (mvOccupied->Handle->GetPos(bh_EPR) - mvOccupied->Handle->GetPos(bh_EPN) < 0.1)
                mvOccupied->SwitchEPBrake(1); // to nie chce działać
            OK = true;
        }
        else
            OK = false;
        //   if (mvOccupied->BrakeCtrlPos<mvOccupied->BrakeCtrlPosNo)
        //    if
        //    (mvOccupied->BrakePressureTable[mvOccupied->BrakeCtrlPos+1+2].BrakeType==ElectroPneumatic)
        //    //+2 to indeks Pascala
        //     OK=mvOccupied->IncBrakeLevel();
        //    else
        //     OK=false;
    }
    return OK;
}

bool TController::DecBrake()
{ // zmniejszenie siły hamowania
    bool OK = false;
    switch (mvOccupied->BrakeSystem)
    {
    case Individual:
        OK = mvOccupied->DecLocalBrakeLevel(1 + floor(0.5 + fabs(AccDesired)));
        break;
    case Pneumatic:
        if (mvOccupied->BrakeCtrlPos > 0)
            OK = mvOccupied->DecBrakeLevel();
        if (!OK)
            OK = mvOccupied->DecLocalBrakeLevel(2);
        if (mvOccupied->PipePress < 3.0)
            Need_BrakeRelease = true;
        break;
    case ElectroPneumatic:
        if (mvOccupied->fBrakeCtrlPos != mvOccupied->Handle->GetPos(bh_EPR))
        {
            mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EPR));
            if (mvOccupied->Handle->GetPos(bh_EPR) - mvOccupied->Handle->GetPos(bh_EPN) < 0.1)
                mvOccupied->SwitchEPBrake(1);
            OK = true;
        }
        else
            OK = false;
        if (!OK)
            OK = mvOccupied->DecLocalBrakeLevel(2);
        break;
    }
    return OK;
};

bool TController::IncSpeed()
{ // zwiększenie prędkości; zwraca false, jeśli dalej się nie da zwiększać
    if (tsGuardSignal) // jeśli jest dźwięk kierownika
        if (tsGuardSignal->GetStatus() & DSBSTATUS_PLAYING) // jeśli gada, to nie jedziemy
            return false;
    bool OK = true;
    if (iDrivigFlags & moveDoorOpened)
        Doors(false); // zamykanie drzwi - tutaj wykonuje tylko AI (zmienia fActionTime)
    if (fActionTime < 0.0) // gdy jest nakaz poczekać z jazdą, to nie ruszać
        return false;
    if (mvControlling->SlippingWheels)
        return false; // jak poślizg, to nie przyspieszamy
    switch (mvOccupied->EngineType)
    {
    case None: // McZapkie-041003: wagon sterowniczy
        if (mvControlling->MainCtrlPosNo > 0) // jeśli ma czym kręcić
            iDrivigFlags |= moveIncSpeed; // ustawienie flagi jazdy
        return false;
    case ElectricSeriesMotor:
        if (mvControlling->EnginePowerSource.SourceType == CurrentCollector) // jeśli pantografujący
        {
            if (fOverhead2 >= 0.0) // a jazda bezprądowa ustawiana eventami (albo opuszczenie)
                return false; // to nici z ruszania
            if (iOverheadZero) // jazda bezprądowa z poziomu toru ustawia bity
                return false; // to nici z ruszania
        }
        if (!mvControlling->FuseFlag) //&&mvControlling->StLinFlag) //yBARC
            if ((mvControlling->MainCtrlPos == 0) ||
                (mvControlling->StLinFlag)) // youBy polecił dodać 2012-09-08 v367
                // na pozycji 0 przejdzie, a na pozostałych będzie czekać, aż się załączą liniowe
                // (zgaśnie DelayCtrlFlag)
                if (Ready || (iDrivigFlags & movePress))
                    if (fabs(mvControlling->Im) <
                        (fReady < 0.4 ? mvControlling->Imin : mvControlling->IminLo))
                    { // Ra: wywalał nadmiarowy, bo Im może być ujemne; jak nie odhamowany, to nie
                      // przesadzać z prądem
                        if ((mvOccupied->Vel <= 30) ||
                            (mvControlling->Imax > mvControlling->ImaxLo) ||
                            (fVoltage + fVoltage <
                             mvControlling->EnginePowerSource.CollectorParameters.MinV +
                                 mvControlling->EnginePowerSource.CollectorParameters.MaxV))
                        { // bocznik na szeregowej przy ciezkich bruttach albo przy wysokim rozruchu
                          // pod górę albo przy niskim napięciu
                            if (mvControlling->MainCtrlPos ?
                                    mvControlling->RList[mvControlling->MainCtrlPos].R > 0.0 :
                                    true) // oporowa
                            {
                                OK = (mvControlling->DelayCtrlFlag ?
                                          true :
                                          mvControlling->IncMainCtrl(1)); // kręcimy nastawnik jazdy
                                if ((OK) &&
                                    (mvControlling->MainCtrlPos ==
                                     1)) // czekaj na 1 pozycji, zanim się nie włączą liniowe
                                    iDrivigFlags |= moveIncSpeed;
                                else
                                    iDrivigFlags &= ~moveIncSpeed; // usunięcie flagi czekania
                            }
                            else // jeśli bezoporowa (z wyjątekiem 0)
                                OK = false; // to dać bocznik
                        }
                        else
                        { // przekroczone 30km/h, można wejść na jazdę równoległą
                            if (mvControlling->ScndCtrlPos) // jeśli ustawiony bocznik
                                if (mvControlling->MainCtrlPos <
                                    mvControlling->MainCtrlPosNo - 1) // a nie jest ostatnia pozycja
                                    mvControlling->DecScndCtrl(2); // to bocznik na zero po chamsku
                                                                   // (ktoś miał to poprawić...)
                            OK = mvControlling->IncMainCtrl(1);
                        }
                        if ((mvControlling->MainCtrlPos > 2) &&
                            (mvControlling->Im == 0)) // brak prądu na dalszych pozycjach
                            Need_TryAgain = true; // nie załączona lokomotywa albo wywalił
                                                  // nadmiarowy
                        else if (!OK) // nie da się wrzucić kolejnej pozycji
                            OK = mvControlling->IncScndCtrl(1); // to dać bocznik
                    }
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        break;
    case Dumb:
    case DieselElectric:
        if (!mvControlling->FuseFlag)
            if (Ready || (iDrivigFlags & movePress)) //{(BrakePress<=0.01*MaxBrakePress)}
            {
                OK = mvControlling->IncMainCtrl(1);
                if (!OK)
                    OK = mvControlling->IncScndCtrl(1);
            }
        break;
    case WheelsDriven:
        if (!mvControlling->CabNo)
            mvControlling->CabActivisation();
        if (sin(mvControlling->eAngle) > 0)
            mvControlling->IncMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        else
            mvControlling->DecMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        break;
    case DieselEngine:
        if (mvControlling->ShuntModeAllow)
        { // dla 2Ls150 można zmienić tryb pracy, jeśli jest w liniowym i nie daje rady (wymaga
          // zerowania kierunku)
            // mvControlling->ShuntMode=(OrderList[OrderPos]&Shunt)||(fMass>224000.0);
        }
        if ((mvControlling->Vel > mvControlling->dizel_minVelfullengage) &&
            (mvControlling->RList[mvControlling->MainCtrlPos].Mn > 0))
            OK = mvControlling->IncMainCtrl(1);
        if (mvControlling->RList[mvControlling->MainCtrlPos].Mn == 0)
            OK = mvControlling->IncMainCtrl(1);
        if (!mvControlling->Mains)
        {
            mvControlling->MainSwitch(true);
            mvControlling->ConverterSwitch(true);
            mvControlling->CompressorSwitch(true);
        }
        break;
    }
    return OK;
}

bool TController::DecSpeed(bool force)
{ // zmniejszenie prędkości (ale nie hamowanie)
    bool OK = false; // domyślnie false, aby wyszło z pętli while
    switch (mvOccupied->EngineType)
    {
    case None: // McZapkie-041003: wagon sterowniczy
        iDrivigFlags &= ~moveIncSpeed; // usunięcie flagi jazdy
        if (force) // przy aktywacji kabiny jest potrzeba natychmiastowego wyzerowania
            if (mvControlling->MainCtrlPosNo > 0) // McZapkie-041003: wagon sterowniczy, np. EZT
                mvControlling->DecMainCtrl(1 + (mvControlling->MainCtrlPos > 2 ? 1 : 0));
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        return false;
    case ElectricSeriesMotor:
        OK = mvControlling->DecScndCtrl(2); // najpierw bocznik na zero
        if (!OK)
            OK = mvControlling->DecMainCtrl(1 + (mvControlling->MainCtrlPos > 2 ? 1 : 0));
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        break;
    case Dumb:
    case DieselElectric:
        OK = mvControlling->DecScndCtrl(2);
        if (!OK)
            OK = mvControlling->DecMainCtrl(2 + (mvControlling->MainCtrlPos / 2));
        break;
    case WheelsDriven:
        if (!mvControlling->CabNo)
            mvControlling->CabActivisation();
        if (sin(mvControlling->eAngle) < 0)
            mvControlling->IncMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        else
            mvControlling->DecMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        break;
    case DieselEngine:
        if ((mvControlling->Vel > mvControlling->dizel_minVelfullengage))
        {
            if (mvControlling->RList[mvControlling->MainCtrlPos].Mn > 0)
                OK = mvControlling->DecMainCtrl(1);
        }
        else
            while ((mvControlling->RList[mvControlling->MainCtrlPos].Mn > 0) &&
                   (mvControlling->MainCtrlPos > 1))
                OK = mvControlling->DecMainCtrl(1);
        if (force) // przy aktywacji kabiny jest potrzeba natychmiastowego wyzerowania
            OK = mvControlling->DecMainCtrl(1 + (mvControlling->MainCtrlPos > 2 ? 1 : 0));
        break;
    }
    return OK;
};

void TController::SpeedSet()
{ // Ra: regulacja prędkości, wykonywana w każdym przebłysku świadomości AI
    // ma dokręcać do bezoporowych i zdejmować pozycje w przypadku przekroczenia prądu
    switch (mvOccupied->EngineType)
    {
    case None: // McZapkie-041003: wagon sterowniczy
        if (fActionTime >= -1.0)
            mvOccupied->DepartureSignal = false; // trochę niech pobuczy, zanim pojedzie
        if (mvControlling->MainCtrlPosNo > 0)
        { // jeśli ma czym kręcić
            // TODO: sprawdzanie innego czlonu //if (!FuseFlagCheck())
            if ((AccDesired < fAccGravity - 0.05) ||
                (mvOccupied->Vel > VelDesired)) // jeśli nie ma przyspieszać
                mvControlling->DecMainCtrl(2); // na zero
            else if (fActionTime >= 0.0)
            { // jak już można coś poruszać, przetok rozłączać od razu
                if (iDrivigFlags & moveIncSpeed)
                { // jak ma jechać
                    if (fReady < 0.4) // 0.05*Controlling->MaxBrakePress)
                    { // jak jest odhamowany
                        if (mvOccupied->ActiveDir > 0)
                            mvOccupied->DirectionForward(); //żeby EN57 jechały na drugiej nastawie
                        {
                            if (mvControlling->MainCtrlPos &&
                                !mvControlling
                                     ->StLinFlag) // jak niby jedzie, ale ma rozłączone liniowe
                                mvControlling->DecMainCtrl(
                                    2); // to na zero i czekać na przewalenie kułakowego
                            else
                                switch (mvControlling->MainCtrlPos)
                                { // ruch nastawnika uzależniony jest od aktualnie ustawionej
                                  // pozycji
                                case 0:
                                    if (mvControlling->MainCtrlActualPos) // jeśli kułakowy nie jest
                                                                          // wyzerowany
                                        break; // to czekać na wyzerowanie
                                    mvControlling->IncMainCtrl(1); // przetok; bez "break", bo nie
                                                                   // ma czekania na 1. pozycji
                                case 1:
                                    if (VelDesired >= 20)
                                        mvControlling->IncMainCtrl(1); // szeregowa
                                case 2:
                                    if (VelDesired >= 50)
                                        mvControlling->IncMainCtrl(1); // równoległa
                                case 3:
                                    if (VelDesired >= 80)
                                        mvControlling->IncMainCtrl(1); // bocznik 1
                                case 4:
                                    if (VelDesired >= 90)
                                        mvControlling->IncMainCtrl(1); // bocznik 2
                                case 5:
                                    if (VelDesired >= 100)
                                        mvControlling->IncMainCtrl(1); // bocznik 3
                                }
                            if (mvControlling->MainCtrlPos) // jak załączył pozycję
                            {
                                fActionTime = -5.0; // niech trochę potrzyma
                                mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
                            }
                        }
                    }
                }
                else
                {
                    while (mvControlling->MainCtrlPos)
                        mvControlling->DecMainCtrl(1); // na zero
                    fActionTime = -5.0; // niech trochę potrzyma
                    mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
                }
            }
        }
        break;
    case ElectricSeriesMotor:
        if ((!mvControlling->StLinFlag) && (!mvControlling->DelayCtrlFlag) &&
            (!iDrivigFlags & moveIncSpeed)) // styczniki liniowe rozłączone    yBARC
            //    if (iDrivigFlags&moveIncSpeed) {} //jeśli czeka na załączenie liniowych
            //    else
            while (DecSpeed())
                ; // zerowanie napędu
        else if (Ready || (iDrivigFlags & movePress)) // o ile może jechać
            if (fAccGravity < -0.10) // i jedzie pod górę większą niż 10 promil
            { // procedura wjeżdżania na ekstremalne wzniesienia
                if (fabs(mvControlling->Im) >
                    0.85 * mvControlling->Imax) // a prąd jest większy niż 85% nadmiarowego
                    // if (mvControlling->Imin*mvControlling->Voltage/(fMass*fAccGravity)<-2.8) //a
                    // na niskim się za szybko nie pojedzie
                    if (mvControlling->Imax * mvControlling->Voltage / (fMass * fAccGravity) <
                        -2.8) // a na niskim się za szybko nie pojedzie
                    { // włączenie wysokiego rozruchu;
                      // (I*U)[A*V=W=kg*m*m/sss]/(m[kg]*a[m/ss])=v[m/s]; 2.8m/ss=10km/h
                        if (mvControlling->RList[mvControlling->MainCtrlPos].Bn > 1)
                        { // jeśli jedzie na równoległym, to zbijamy do szeregowego, aby włączyć
                          // wysoki rozruch
                            if (mvControlling->ScndCtrlPos > 0) // jeżeli jest bocznik
                                mvControlling->DecScndCtrl(
                                    2); // wyłączyć bocznik, bo może blokować skręcenie NJ
                            do // skręcanie do bezoporowej na szeregowym
                                mvControlling->DecMainCtrl(1); // kręcimy nastawnik jazdy o 1 wstecz
                            while (mvControlling->MainCtrlPos ?
                                       mvControlling->RList[mvControlling->MainCtrlPos].Bn > 1 :
                                       false); // oporowa zapętla
                        }
                        if (mvControlling->Imax < mvControlling->ImaxHi) // jeśli da się na wysokim
                            mvControlling->CurrentSwitch(
                                true); // rozruch wysoki (za to może się ślizgać)
                        if (ReactionTime > 0.1)
                            ReactionTime = 0.1; // orientuj się szybciej
                    } // if (Im>Imin)
                if (fabs(mvControlling->Im) > 0.75 * mvControlling->ImaxHi) // jeśli prąd jest duży
                    mvControlling->SandDoseOn(); // piaskujemy tory, coby się nie ślizgać
                if ((fabs(mvControlling->Im) > 0.96 * mvControlling->Imax) ||
                    mvControlling->SlippingWheels) // jeśli prąd jest duży (można 690 na 750)
                    if (mvControlling->ScndCtrlPos > 0) // jeżeli jest bocznik
                        mvControlling->DecScndCtrl(2); // zmniejszyć bocznik
                    else
                        mvControlling->DecMainCtrl(1); // kręcimy nastawnik jazdy o 1 wstecz
            }
            else // gdy nie jedzie ambitnie pod górę
            { // sprawdzenie, czy rozruch wysoki jest potrzebny
                if (mvControlling->Imax > mvControlling->ImaxLo)
                    if (mvOccupied->Vel >= 30.0) // jak się rozpędził
                        if (fAccGravity > -0.02) // a i pochylenie mnijsze niż 2‰
                            mvControlling->CurrentSwitch(false); // rozruch wysoki wyłącz
                // dokręcanie do bezoporowej, bo IncSpeed() może nie być wywoływane
                // if (mvOccupied->Vel<VelDesired)
                // if (AccDesired>-0.1) //nie ma hamować
                //  if (Controlling->RList[MainCtrlPos].R>0.0)
                //   if (Im<1.3*Imin) //lekkie przekroczenie miimalnego prądu jest dopuszczalne
                //    IncMainCtrl(1); //zwieksz nastawnik skoro możesz - tak aby się ustawic na
                //    bezoporowej
            }
        break;
    case Dumb:
    case DieselElectric:
        break;
    // WheelsDriven :
    // begin
    //  OK:=False;
    // end;
    case DieselEngine:
        // Ra 2014-06: "automatyczna" skrzynia biegów...
        if (!mvControlling->MotorParam[mvControlling->ScndCtrlPos].AutoSwitch) // gdy biegi ręczne
            if ((mvControlling->ShuntMode ? mvControlling->AnPos : 1.0) * mvControlling->Vel >
                0.6 * mvControlling->MotorParam[mvControlling->ScndCtrlPos].mfi)
            // if (mvControlling->enrot>0.95*mvControlling->dizel_nMmax) //youBy: jeśli obroty >
            // 0,95 nmax, wrzuć wyższy bieg - Ra: to nie działa
            { // jak prędkość większa niż 0.6 maksymalnej na danym biegu, wrzucić wyższy
                mvControlling->DecMainCtrl(2);
                if (mvControlling->IncScndCtrl(1))
                    if (mvControlling->MotorParam[mvControlling->ScndCtrlPos].mIsat ==
                        0.0) // jeśli bieg jałowy
                        mvControlling->IncScndCtrl(1); // to kolejny
            }
            else if ((mvControlling->ShuntMode ? mvControlling->AnPos : 1.0) * mvControlling->Vel <
                     mvControlling->MotorParam[mvControlling->ScndCtrlPos].fi)
            { // jak prędkość mniejsza niż minimalna na danym biegu, wrzucić niższy
                mvControlling->DecMainCtrl(2);
                mvControlling->DecScndCtrl(1);
                if (mvControlling->MotorParam[mvControlling->ScndCtrlPos].mIsat ==
                    0.0) // jeśli bieg jałowy
                    if (mvControlling->ScndCtrlPos) // a jeszcze zera nie osiągnięto
                        mvControlling->DecScndCtrl(1); // to kolejny wcześniejszy
                    else
                        mvControlling->IncScndCtrl(1); // a jak zeszło na zero, to powrót
            }
        break;
    }
};

void TController::Doors(bool what)
{ // otwieranie/zamykanie drzwi w składzie albo (tylko AI) EZT
    if (what)
    { // otwarcie
    }
    else
    { // zamykanie
        if (mvOccupied->DoorOpenCtrl == 1)
        { // jeśli drzwi sterowane z kabiny
            if (AIControllFlag)
                if (mvOccupied->DoorLeftOpened || mvOccupied->DoorRightOpened)
                { // AI zamyka drzwi przed odjazdem
                    if (mvOccupied->DoorClosureWarning)
                        mvOccupied->DepartureSignal = true; // załącenie bzyczka
                    mvOccupied->DoorLeft(false); // zamykanie drzwi
                    mvOccupied->DoorRight(false);
                    // Ra: trzeba by ustawić jakiś czas oczekiwania na zamknięcie się drzwi
                    fActionTime = -1.5 - 0.1 * random(10); // czekanie sekundę, może trochę dłużej
                    iDrivigFlags &= ~moveDoorOpened; // nie wykonywać drugi raz
                }
        }
        else
        { // jeśli nie, to zamykanie w składzie wagonowym
            TDynamicObject *p = pVehicles[0]; // pojazd na czole składu
            while (p)
            { // zamykanie drzwi w pojazdach - flaga zezwolenia była by lepsza
                p->MoverParameters->DoorLeft(false); // w lokomotywie można by nie zamykać...
                p->MoverParameters->DoorRight(false);
                p = p->Next(); // pojazd podłączony z tyłu (patrząc od czoła)
            }
            // WaitingSet(5); //10 sekund tu to za długo, opóźnia odjazd o pół minuty
            fActionTime = -1.5 - 0.1 * random(10); // czekanie sekundę, może trochę dłużej
            iDrivigFlags &= ~moveDoorOpened; // zostały zamknięte - nie wykonywać drugi raz
        }
    }
};

void TController::RecognizeCommand()
{ // odczytuje i wykonuje komendę przekazaną lokomotywie
    TCommand *c = &mvOccupied->CommandIn;
    PutCommand(c->Command, c->Value1, c->Value2, c->Location, stopComm);
    c->Command = ""; // usunięcie obsłużonej komendy
}

void TController::PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2,
                                        const TLocation &NewLocation, TStopReason reason)
{ // wysłanie komendy przez event PutValues, jak pojazd ma obsadę, to wysyła tutaj, a nie do pojazdu
  // bezpośrednio
    vector3 sl;
    sl.x = -NewLocation.X; // zamiana na współrzędne scenerii
    sl.z = NewLocation.Y;
    sl.y = NewLocation.Z;
    if (!PutCommand(NewCommand, NewValue1, NewValue2, &sl, reason))
        mvOccupied->PutCommand(NewCommand, NewValue1, NewValue2, NewLocation);
}

bool TController::PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2,
                                        const vector3 *NewLocation, TStopReason reason)
{ // analiza komendy
    if (NewCommand == "CabSignal")
    { // SHP wyzwalane jest przez człon z obsadą, ale obsługiwane przez silnikowy
        // nie jest to najlepiej zrobione, ale bez symulacji obwodów lepiej nie będzie
        // Ra 2014-04: jednak przeniosłem do rozrządczego
        mvOccupied->PutCommand(NewCommand, NewValue1, NewValue2, mvOccupied->Loc);
        mvOccupied->RunInternalCommand(); // rozpoznaj komende bo lokomotywa jej nie rozpoznaje
        return true; // załatwione
    }
    if (NewCommand == "Overhead")
    { // informacja o stanie sieci trakcyjnej
        fOverhead1 =
            NewValue1; // informacja o napięciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
        fOverhead2 = NewValue2; // informacja o sposobie jazdy (-1=normalnie, 0=bez prądu, >0=z
                                // opuszczonym i ograniczeniem prędkości)
        return true; // załatwione
    }
    else if (NewCommand == "Emergency_brake") // wymuszenie zatrzymania, niezależnie kto prowadzi
    { // Ra: no nadal nie jest zbyt pięknie
        SetVelocity(0, 0, reason);
        mvOccupied->PutCommand("Emergency_brake", 1.0, 1.0, mvOccupied->Loc);
        return true; // załatwione
    }
    else if (NewCommand.Pos("Timetable:") == 1)
    { // przypisanie nowego rozkładu jazdy, również prowadzonemu przez użytkownika
        NewCommand.Delete(1, 10); // zostanie nazwa pliku z rozkładem
#if LOGSTOPS
        WriteLog("New timetable for " + pVehicle->asName + ": " + NewCommand); // informacja
#endif
        if (!TrainParams)
            TrainParams = new TTrainParameters(NewCommand); // rozkład jazdy
        else
            TrainParams->NewName(NewCommand); // czyści tabelkę przystanków
        delete tsGuardSignal;
        tsGuardSignal = NULL; // wywalenie kierownika
        if (NewCommand != "none")
        {
            if (!TrainParams->LoadTTfile(
                    Global::asCurrentSceneryPath, floor(NewValue2 + 0.5),
                    NewValue1)) // pierwszy parametr to przesunięcie rozkładu w czasie
            {
                if (ConversionError == -8)
                    ErrorLog("Missed timetable: " + NewCommand);
                WriteLog("Cannot load timetable file " + NewCommand + "\r\nError " +
                         ConversionError + " in position " + TrainParams->StationCount);
                NewCommand = ""; // puste, dla wymiennej tekstury
            }
            else
            { // inicjacja pierwszego przystanku i pobranie jego nazwy
                TrainParams->UpdateMTable(GlobalTime->hh, GlobalTime->mm,
                                          TrainParams->NextStationName);
                TrainParams->StationIndexInc(); // przejście do następnej
                iStationStart = TrainParams->StationIndex;
                asNextStop = TrainParams->NextStop();
                iDrivigFlags |= movePrimary; // skoro dostał rozkład, to jest teraz głównym
                NewCommand = Global::asCurrentSceneryPath + NewCommand + ".wav"; // na razie jeden
                if (FileExists(NewCommand))
                { // wczytanie dźwięku odjazdu podawanego bezpośrenido
                    tsGuardSignal = new TTextSound();
                    tsGuardSignal->Init(NewCommand.c_str(), 30, pVehicle->GetPosition().x,
                                        pVehicle->GetPosition().y, pVehicle->GetPosition().z,
                                        false);
                    // rsGuardSignal->Stop();
                    iGuardRadio = 0; // nie przez radio
                }
                else
                {
                    NewCommand.Insert("radio", NewCommand.Length() - 3); // wstawienie przed kropką
                    if (FileExists(NewCommand))
                    { // wczytanie dźwięku odjazdu w wersji radiowej (słychać tylko w kabinie)
                        tsGuardSignal = new TTextSound();
                        tsGuardSignal->Init(NewCommand.c_str(), -1, pVehicle->GetPosition().x,
                                            pVehicle->GetPosition().y, pVehicle->GetPosition().z,
                                            false);
                        iGuardRadio = iRadioChannel;
                    }
                }
                NewCommand = TrainParams->Relation2; // relacja docelowa z rozkładu
            }
            // jeszcze poustawiać tekstury na wyświetlaczach
            TDynamicObject *p = pVehicles[0];
            while (p)
            {
                p->DestinationSet(NewCommand); // relacja docelowa
                p = p->Next(); // pojazd podłączony od tyłu (licząc od czoła)
            }
        }
        if (NewLocation) // jeśli podane współrzędne eventu/komórki ustawiającej rozkład (trainset
                         // nie podaje)
        {
            vector3 v = *NewLocation - pVehicle->GetPosition(); // wektor do punktu sterującego
            vector3 d = pVehicle->VectorFront(); // wektor wskazujący przód
            iDirectionOrder = ((v.x * d.x + v.z * d.z) * NewValue1 > 0) ?
                                  1 :
                                  -1; // do przodu, gdy iloczyn skalarny i prędkość dodatnie
            /*
              if (NewValue1!=0.0) //jeśli ma jechać
               if (iDirectionOrder==0) //a kierunek nie był określony (normalnie określany przez
              reardriver/headdriver)
                iDirectionOrder=NewValue1>0?1:-1; //ustalenie kierunku jazdy względem sprzęgów
               else
                if (NewValue1<0) //dla ujemnej prędkości
                 iDirectionOrder=-iDirectionOrder; //ma jechać w drugą stronę
            */
            if (AIControllFlag) // jeśli prowadzi komputer
                Activation(); // umieszczenie obsługi we właściwym członie, ustawienie nawrotnika w
                              // przód
        }
        /*
          else
           if (!iDirectionOrder)
          {//jeśli nie ma kierunku, trzeba ustalić
           if (mvOccupied->V!=0.0)
            iDirectionOrder=mvOccupied->V>0?1:-1;
           else
            iDirectionOrder=mvOccupied->ActiveCab;
           if (!iDirectionOrder) iDirectionOrder=1;
          }
        */
        // jeśli wysyłane z Trainset, to wszystko jest już odpowiednio ustawione
        // if (!NewLocation) //jeśli wysyłane z Trainset
        // if (mvOccupied->CabNo*mvOccupied->V*NewValue1<0) //jeśli zadana prędkość niezgodna z
        // aktualnym kierunkiem jazdy
        //  DirectionForward(false); //jedziemy do tyłu (nawrotnik do tyłu)
        // CheckVehicles(); //sprawdzenie składu, AI zapali światła
        TableClear(); // wyczyszczenie tabelki prędkości, bo na nowo trzeba określić kierunek i
                      // sprawdzić przystanki
        OrdersInit(
            fabs(NewValue1)); // ustalenie tabelki komend wg rozkładu oraz prędkości początkowej
        // if (NewValue1!=0.0) if (!AIControllFlag) DirectionForward(NewValue1>0.0); //ustawienie
        // nawrotnika użytkownikowi (propaguje się do członów)
        // if (NewValue1>0)
        // TrainNumber=floor(NewValue1); //i co potem ???
        return true; // załatwione
    }
    if (NewCommand == "SetVelocity")
    {
        if (NewLocation)
            vCommandLocation = *NewLocation;
        if ((NewValue1 != 0.0) && (OrderList[OrderPos] != Obey_train))
        { // o ile jazda
            if (!iEngineActive)
                OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw, światła ustawi
                                           // JumpToNextOrder()
            // if (OrderList[OrderPos]!=Obey_train) //jeśli nie pociągowa
            OrderNext(Obey_train); // to uruchomić jazdę pociągową (od razu albo po odpaleniu
                                   // silnika
            OrderCheck(); // jeśli jazda pociągowa teraz, to wykonać niezbędne operacje
        }
        if (NewValue1 != 0.0) // jeśli jechać
            iDrivigFlags &= ~moveStopHere; // podjeżanie do semaforów zezwolone
        else
            iDrivigFlags |= moveStopHere; // stać do momentu podania komendy jazdy
        SetVelocity(NewValue1, NewValue2, reason); // bylo: nic nie rob bo SetVelocity zewnetrznie
                                                   // jest wywolywane przez dynobj.cpp
    }
    else if (NewCommand == "SetProximityVelocity")
    {
        /*
          if (SetProximityVelocity(NewValue1,NewValue2))
           if (NewLocation)
            vCommandLocation=*NewLocation;
        */
    }
    else if (NewCommand == "ShuntVelocity")
    { // uruchomienie jazdy manewrowej bądź zmiana prędkości
        if (NewLocation)
            vCommandLocation = *NewLocation;
        // if (OrderList[OrderPos]=Obey_train) and (NewValue1<>0))
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        OrderNext(Shunt); // zamieniamy w aktualnej pozycji, albo dodajey za odpaleniem silnika
        if (NewValue1 != 0.0)
        {
            // if (iVehicleCount>=0) WriteLog("Skasowano ilosć wagonów w ShuntVelocity!");
            iVehicleCount = -2; // wartość neutralna
            CheckVehicles(); // zabrać to do OrderCheck()
        }
        // dla prędkości ujemnej przestawić nawrotnik do tyłu? ale -1=brak ograniczenia !!!!
        // if (iDrivigFlags&movePress) WriteLog("Skasowano docisk w ShuntVelocity!");
        iDrivigFlags &= ~movePress; // bez dociskania
        SetVelocity(NewValue1, NewValue2, reason);
        if (NewValue1 != 0.0)
            iDrivigFlags &= ~moveStopHere; // podjeżanie do semaforów zezwolone
        else
            iDrivigFlags |= moveStopHere; // ma stać w miejscu
        if (fabs(NewValue1) > 2.0) // o ile wartość jest sensowna (-1 nie jest konkretną wartością)
            fShuntVelocity = fabs(NewValue1); // zapamiętanie obowiązującej prędkości dla manewrów
    }
    else if (NewCommand == "Wait_for_orders")
    { // oczekiwanie; NewValue1 - czas oczekiwania, -1 = na inną komendę
        if (NewValue1 > 0.0 ? NewValue1 > fStopTime : false)
            fStopTime = NewValue1; // Ra: włączenie czekania bez zmiany komendy
        else
            OrderList[OrderPos] = Wait_for_orders; // czekanie na komendę (albo dać OrderPos=0)
    }
    else if (NewCommand == "Prepare_engine")
    { // włączenie albo wyłączenie silnika (w szerokim sensie)
        OrdersClear(); // czyszczenie tabelki rozkazów, aby nic dalej nie robił
        if (NewValue1 == 0.0)
            OrderNext(Release_engine); // wyłączyć silnik (przygotować pojazd do jazdy)
        else if (NewValue1 > 0.0)
            OrderNext(Prepare_engine); // odpalić silnik (wyłączyć wszystko, co się da)
        // po załączeniu przejdzie do kolejnej komendy, po wyłączeniu na Wait_for_orders
    }
    else if (NewCommand == "Change_direction")
    {
        TOrders o = OrderList[OrderPos]; // co robił przed zmianą kierunku
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        if (NewValue1 == 0.0)
            iDirectionOrder = -iDirection; // zmiana na przeciwny niż obecny
        else if (NewLocation) // jeśli podane współrzędne eventu/komórki ustawiającej rozkład
                              // (trainset nie podaje)
        {
            vector3 v = *NewLocation - pVehicle->GetPosition(); // wektor do punktu sterującego
            vector3 d = pVehicle->VectorFront(); // wektor wskazujący przód
            iDirectionOrder = ((v.x * d.x + v.z * d.z) * NewValue1 > 0) ?
                                  1 :
                                  -1; // do przodu, gdy iloczyn skalarny i prędkość dodatnie
            // iDirectionOrder=1; else if (NewValue1<0.0) iDirectionOrder=-1;
        }
        if (iDirectionOrder != iDirection)
            OrderNext(Change_direction); // zadanie komendy do wykonania
        if (o >= Shunt) // jeśli jazda manewrowa albo pociągowa
            OrderNext(o); // to samo robić po zmianie
        else if (!o) // jeśli wcześniej było czekanie
            OrderNext(Shunt); // to dalej jazda manewrowa
        if (mvOccupied->Vel >= 1.0) // jeśli jedzie
            iDrivigFlags &= ~moveStartHorn; // to bez trąbienia po ruszeniu z zatrzymania
        // Change_direction wykona się samo i następnie przejdzie do kolejnej komendy
    }
    else if (NewCommand == "Obey_train")
    {
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        OrderNext(Obey_train);
        // if (NewValue1>0) TrainNumber=floor(NewValue1); //i co potem ???
        OrderCheck(); // jeśli jazda pociągowa teraz, to wykonać niezbędne operacje
    }
    else if (NewCommand == "Shunt")
    { // NewValue1 - ilość wagonów (-1=wszystkie); NewValue2: 0=odczep, 1..63=dołącz, -1=bez zmian
        //-3,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1), zmienić kierunek i czekać w
        //trybie pociągowym
        //-2,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1), zmienić kierunek i czekać
        //-2, y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i czekać
        //-1,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i jechać w powrotną stronę
        //-1, y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i jechać dalej
        //-1, 0 - tryb manewrowy bez zmian (odczepianie z pozostawieniem wagonów nie ma sensu)
        // 0, 0 - odczepienie lokomotywy
        // 1,-y - podłączyć się do składu (sprzęgiem y>=1), a następnie odczepić i zabrać (x)
        // wagonów
        // 1, 0 - odczepienie lokomotywy z jednym wagonem
        iDrivigFlags &= ~moveStopHere; // podjeżanie do semaforów zezwolone
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        if (NewValue2 != 0) // jeśli podany jest sprzęg
        {
            iCoupler = floor(fabs(NewValue2)); // jakim sprzęgiem
            OrderNext(Connect); // najpierw połącz pojazdy
            if (NewValue1 >= 0.0) // jeśli ilość wagonów inna niż wszystkie
            { // to po podpięciu należy się odczepić
                iDirectionOrder = -iDirection; // zmiana na ciągnięcie
                OrderPush(Change_direction); // najpierw zmień kierunek, bo odczepiamy z tyłu
                OrderPush(Disconnect); // a odczep już po zmianie kierunku
            }
            else if (NewValue2 < 0.0) // jeśli wszystkie, a sprzęg ujemny, to tylko zmiana kierunku
                                      // po podczepieniu
            { // np. Shunt -1 -3
                iDirectionOrder = -iDirection; // jak się podczepi, to jazda w przeciwną stronę
                OrderNext(Change_direction);
            }
            WaitingTime =
                0.0; // nie ma co dalej czekać, można zatrąbić i jechać, chyba że już jedzie
        }
        else // if (NewValue2==0.0) //zerowy sprzęg
            if (NewValue1 >= 0.0) // jeśli ilość wagonów inna niż wszystkie
        { // będzie odczepianie, ale jeśli wagony są z przodu, to trzeba najpierw zmienić kierunek
            if ((mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 1 : 0].CouplingFlag ==
                 0) ? // z tyłu nic
                    (mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 0 : 1].CouplingFlag > 0) :
                    false) // a z przodu skład
            {
                iDirectionOrder = -iDirection; // zmiana na ciągnięcie
                OrderNext(Change_direction); // najpierw zmień kierunek (zastąpi Disconnect)
                OrderPush(Disconnect); // a odczep już po zmianie kierunku
            }
            else if (mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 1 : 0].CouplingFlag >
                     0) // z tyłu coś
                OrderNext(Disconnect); // jak ciągnie, to tylko odczep (NewValue1) wagonów
            WaitingTime =
                0.0; // nie ma co dalej czekać, można zatrąbić i jechać, chyba że już jedzie
        }
        if (NewValue1 == -1.0)
        {
            iDrivigFlags &= ~moveStopHere; // ma jechać
            WaitingTime = 0.0; // nie ma co dalej czekać, można zatrąbić i jechać
        }
        if (NewValue1 < -1.5) // jeśli -2/-3, czyli czekanie z ruszeniem na sygnał
            iDrivigFlags |= moveStopHere; // nie podjeżdżać do semafora, jeśli droga nie jest wolna
                                          // (nie dotyczy Connect)
        if (NewValue1 < -2.5) // jeśli
            OrderNext(Obey_train); // to potem jazda pociągowa
        else
            OrderNext(Shunt); // to potem manewruj dalej
        CheckVehicles(); // sprawdzić światła
        // if ((iVehicleCount>=0)&&(NewValue1<0)) WriteLog("Skasowano ilosć wagonów w Shunt!");
        if (NewValue1 != iVehicleCount)
            iVehicleCount = floor(NewValue1); // i co potem ? - trzeba zaprogramowac odczepianie
        /*
          if (NewValue1!=-1.0)
           if (NewValue2!=0.0)

            if (VelDesired==0)
             SetVelocity(20,0); //to niech jedzie
        */
    }
    else if (NewCommand == "Jump_to_first_order")
        JumpToFirstOrder();
    else if (NewCommand == "Jump_to_order")
    {
        if (NewValue1 == -1.0)
            JumpToNextOrder();
        else if ((NewValue1 >= 0) && (NewValue1 < maxorders))
        {
            OrderPos = floor(NewValue1);
            if (!OrderPos)
                OrderPos = 1; // zgodność wstecz: dopiero pierwsza uruchamia
#if LOGORDERS
            WriteLog("--> Jump_to_order");
            OrdersDump();
#endif
        }
        /*
          if (WriteLogFlag)
          {
           append(AIlogFile);
           writeln(AILogFile,ElapsedTime:5:2," - new order: ",Order2Str( OrderList[OrderPos])," @
          ",OrderPos);
           close(AILogFile);
          }
        */
    }
    /* //ta komenda jest teraz skanowana, więc wysyłanie jej eventem nie ma sensu
     else if (NewCommand=="OutsideStation") //wskaznik W5
     {
      if (OrderList[OrderPos]==Obey_train)
       SetVelocity(NewValue1,NewValue2,stopOut); //koniec stacji - predkosc szlakowa
      else //manewry - zawracaj
      {
       iDirectionOrder=-iDirection; //zmiana na przeciwny niż obecny
       OrderNext(Change_direction); //zmiana kierunku
       OrderNext(Shunt); //a dalej manewry
       iDrivigFlags&=~moveStartHorn; //bez trąbienia po zatrzymaniu
      }
     }
    */
    else if (NewCommand == "Warning_signal")
    {
        if (AIControllFlag) // poniższa komenda nie jest wykonywana przez użytkownika
            if (NewValue1 > 0)
            {
                fWarningDuration = NewValue1; // czas trąbienia
                mvOccupied->WarningSignal = (NewValue2 > 1) ? 2 : 1; // wysokość tonu
            }
    }
    else if (NewCommand == "Radio_channel")
    { // wybór kanału radiowego (którego powinien używać AI, ręczny maszynista musi go ustawić sam)
        if (NewValue1 >= 0) // wartości ujemne są zarezerwowane, -1 = nie zmieniać kanału
        {
            iRadioChannel = NewValue1;
            if (iGuardRadio)
                iGuardRadio = iRadioChannel; // kierownikowi też zmienić
        }
        // NewValue2 może zawierać dodatkowo oczekiwany kod odpowiedzi, np. dla W29 "nawiązać
        // łączność radiową z dyżurnym ruchu odcinkowym"
    }
    else
        return false; // nierozpoznana - wysłać bezpośrednio do pojazdu
    return true; // komenda została przetworzona
};

void TController::PhysicsLog()
{ // zapis logu - na razie tylko wypisanie parametrów
    if (LogFile.is_open())
    {
#if LOGPRESS == 0
        LogFile << ElapsedTime << " " << fabs(11.31 * mvOccupied->WheelDiameter * mvOccupied->nrot)
                << " ";
        LogFile << mvControlling->AccS << " " << mvOccupied->Couplers[1].Dist << " "
                << mvOccupied->Couplers[1].CForce << " ";
        LogFile << mvOccupied->Ft << " " << mvOccupied->Ff << " " << mvOccupied->Fb << " "
                << mvOccupied->BrakePress << " ";
        LogFile << mvOccupied->PipePress << " " << mvControlling->Im << " "
                << int(mvControlling->MainCtrlPos) << "   ";
        LogFile << int(mvControlling->ScndCtrlPos) << "   " << int(mvOccupied->BrakeCtrlPos)
                << "   " << int(mvOccupied->LocalBrakePos) << "   ";
        LogFile << int(mvControlling->ActiveDir) << "   " << mvOccupied->CommandIn.Command.c_str()
                << " " << mvOccupied->CommandIn.Value1 << " ";
        LogFile << mvOccupied->CommandIn.Value2 << " " << int(mvControlling->SecuritySystem.Status)
                << " " << int(mvControlling->SlippingWheels) << "\r\n";
#endif
#if LOGPRESS == 1
        LogFile << ElapsedTime << "\t" << fabs(11.31 * mvOccupied->WheelDiameter * mvOccupied->nrot)
                << "\t";
        LogFile << Controlling->AccS << "\t";
        LogFile << mvOccupied->PipePress << "\t" << mvOccupied->CntrlPipePress << "\t"
                << mvOccupied->BrakePress << "\t";
        LogFile << mvOccupied->Volume << "\t" << mvOccupied->Hamulec->GetCRP() << "\n";
#endif
        LogFile.flush();
    }
};

bool TController::UpdateSituation(double dt)
{ // uruchamiać przynajmniej raz na sekundę
    if ((iDrivigFlags & movePrimary) == 0)
        return true; // pasywny nic nie robi
    double AbsAccS;
    // double VelReduced; //o ile km/h może przekroczyć dozwoloną prędkość bez hamowania
    bool UpdateOK = false;
    if (AIControllFlag)
    { // yb: zeby EP nie musial sie bawic z ciesnieniem w PG
        //  if (mvOccupied->BrakeSystem==ElectroPneumatic)
        //   mvOccupied->PipePress=0.5; //yB: w SPKS są poprawnie zrobione pozycje
        if (mvControlling->SlippingWheels)
        {
            mvControlling->SandDoseOn(); // piasku!
            // Controlling->SlippingWheels=false; //a to tu nie ma sensu, flaga używana w dalszej
            // części
        }
    }
    // ABu-160305 testowanie gotowości do jazdy
    // Ra: przeniesione z DynObj, skład użytkownika też jest testowany, żeby mu przekazać, że ma
    // odhamować
    Ready = true; // wstępnie gotowy
    fReady = 0.0; // założenie, że odhamowany
    fAccGravity = 0.0; // przyspieszenie wynikające z pochylenia
    double dy; // składowa styczna grawitacji, w przedziale <0,1>
    TDynamicObject *p = pVehicles[0]; // pojazd na czole składu
    while (p)
    { // sprawdzenie odhamowania wszystkich połączonych pojazdów
        if (Ready) // bo jak coś nie odhamowane, to dalej nie ma co sprawdzać
            // if (p->MoverParameters->BrakePress>=0.03*p->MoverParameters->MaxBrakePress)
            if (p->MoverParameters->BrakePress >= 0.4) // wg UIC określone sztywno na 0.04
            {
                Ready = false; // nie gotowy
                // Ra: odluźnianie przeładowanych lokomotyw, ciągniętych na zimno - prowizorka...
                if (AIControllFlag) // skład jak dotąd był wyluzowany
                {
                    if (mvOccupied->BrakeCtrlPos == 0) // jest pozycja jazdy
                        if ((p->MoverParameters->PipePress - 5.0) >
                            -0.1) // jeśli ciśnienie jak dla jazdy
                            if (p->MoverParameters->Hamulec->GetCRP() >
                                p->MoverParameters->PipePress + 0.12) // za dużo w zbiorniku
                                p->MoverParameters->BrakeReleaser(1); // indywidualne luzowanko
                    if (p->MoverParameters->Power > 0.01) // jeśli ma silnik
                        if (p->MoverParameters->FuseFlag) // wywalony nadmiarowy
                            Need_TryAgain = true; // reset jak przy wywaleniu nadmiarowego
                }
            }
        if (fReady < p->MoverParameters->BrakePress)
            fReady = p->MoverParameters->BrakePress; // szukanie najbardziej zahamowanego
        if ((dy = p->VectorFront().y) != 0.0) // istotne tylko dla pojazdów na pochyleniu
            fAccGravity -= p->DirectionGet() * p->MoverParameters->TotalMassxg *
                           dy; // ciężar razy składowa styczna grawitacji
        p = p->Next(); // pojazd podłączony z tyłu (patrząc od czoła)
    }
    if (iDirection)
        fAccGravity /=
            iDirection *
            fMass; // siłę generują pojazdy na pochyleniu ale działa ona całość składu, więc a=F/m
    if (!Ready) // v367: jeśli wg powyższych warunków skład nie jest odhamowany
        if (fAccGravity < -0.05) // jeśli ma pod górę na tyle, by się stoczyć
            // if (mvOccupied->BrakePress<0.08) //to wystarczy, że zadziałają liniowe (nie ma ich
            // jeszcze!!!)
            if (fReady < 0.8) // delikatniejszy warunek, obejmuje wszystkie wagony
                Ready = true; //żeby uznać za odhamowany
    HelpMeFlag = false;
    // Winger 020304
    if (AIControllFlag)
    {
        if (mvControlling->EnginePowerSource.SourceType == CurrentCollector)
        {
            if (mvOccupied->ScndPipePress > 4.3) // gdy główna sprężarka bezpiecznie nabije
                                                 // ciśnienie
                mvControlling->bPantKurek3 =
                    true; // to można przestawić kurek na zasilanie pantografów z głównej pneumatyki
            fVoltage =
                0.5 * (fVoltage +
                       fabs(mvControlling->RunningTraction.TractionVoltage)); // uśrednione napięcie
                                                                              // sieci: przy spadku
                                                                              // poniżej wartości
                                                                              // minimalnej opóźnić
                                                                              // rozruch o losowy
                                                                              // czas
            if (fVoltage < mvControlling->EnginePowerSource.CollectorParameters
                               .MinV) // gdy rozłączenie WS z powodu niskiego napięcia
                if (fActionTime >= 0) // jeśli czas oczekiwania nie został ustawiony
                    fActionTime =
                        -2 - random(10); // losowy czas oczekiwania przed ponownym załączeniem jazdy
        }
        if (mvOccupied->Vel > 0.0)
        { // jeżeli jedzie
            if (iDrivigFlags & moveDoorOpened) // jeśli drzwi otwarte
                if (mvOccupied->Vel > 1.0) // nie zamykać drzwi przy drganiach, bo zatrzymanie na W4
                                           // akceptuje niewielkie prędkości
                    Doors(false);
            // przy prowadzeniu samochodu trzeba każdą oś odsuwać oddzielnie, inaczej kicha wychodzi
            if (mvOccupied->CategoryFlag & 2) // jeśli samochód
                // if (fabs(mvOccupied->OffsetTrackH)<mvOccupied->Dim.W) //Ra: szerokość drogi tu
                // powinna być?
                if (!mvOccupied->ChangeOffsetH(-0.01 * mvOccupied->Vel * dt)) // ruch w poprzek
                                                                              // drogi
                    mvOccupied->ChangeOffsetH(0.01 * mvOccupied->Vel *
                                              dt); // Ra: co to miało być, to nie wiem
            if (mvControlling->EnginePowerSource.SourceType == CurrentCollector)
            {
                if ((fOverhead2 >= 0.0) || iOverheadZero)
                { // jeśli jazda bezprądowa albo z opuszczonym pantografem
                    while (DecSpeed(true))
                        ; // zerowanie napędu
                }
                if ((fOverhead2 > 0.0) || iOverheadDown)
                { // jazda z opuszczonymi pantografami
                    mvControlling->PantFront(false);
                    mvControlling->PantRear(false);
                }
                else
                { // jeśli nie trzeba opuszczać pantografów
                    if (iDirection >= 0) // jak jedzie w kierunku sprzęgu 0
                        mvControlling->PantRear(true); // jazda na tylnym
                    else
                        mvControlling->PantFront(true);
                }
                if (mvOccupied->Vel > 10) // opuszczenie przedniego po rozpędzeniu się
                {
                    if (mvControlling->EnginePowerSource.CollectorParameters.CollectorsNo >
                        1) // o ile jest więcej niż jeden
                        if (iDirection >= 0) // jak jedzie w kierunku sprzęgu 0
                        { // poczekać na podniesienie tylnego
                            if (mvControlling->PantRearVolt !=
                                0.0) // czy jest napięcie zasilające na tylnym?
                                mvControlling->PantFront(false); // opuszcza od sprzęgu 0
                        }
                        else
                        { // poczekać na podniesienie przedniego
                            if (mvControlling->PantFrontVolt !=
                                0.0) // czy jest napięcie zasilające na przednim?
                                mvControlling->PantRear(false); // opuszcza od sprzęgu 1
                        }
                }
            }
        }
        if (iDrivigFlags & moveStartHornNow) // czy ma zatrąbić przed ruszeniem?
            if (Ready) // gotów do jazdy
                if (iEngineActive) // jeszcze się odpalić musi
                    if (fStopTime >= 0) // i nie musi czekać
                    { // uruchomienie trąbienia
                        fWarningDuration = 0.3; // czas trąbienia
                        // if (AIControllFlag) //jak siedzi krasnoludek, to włączy trąbienie
                        mvOccupied->WarningSignal =
                            pVehicle->iHornWarning; // wysokość tonu (2=wysoki)
                        iDrivigFlags |= moveStartHornDone; // nie trąbić aż do ruszenia
                        iDrivigFlags &= ~moveStartHornNow; // trąbienie zostało zorganizowane
                    }
    }
    ElapsedTime += dt;
    WaitingTime += dt;
    fBrakeTime -=
        dt; // wpisana wartość jest zmniejszana do 0, gdy ujemna należy zmienić nastawę hamulca
    fStopTime += dt; // zliczanie czasu postoju, nie ruszy dopóki ujemne
    fActionTime += dt; // czas używany przy regulacji prędkości i zamykaniu drzwi
    if (WriteLogFlag)
    {
        if (LastUpdatedTime > deltalog)
        { // zapis do pliku DAT
            PhysicsLog();
            if (fabs(mvOccupied->V) > 0.1) // Ra: [m/s]
                deltalog = 0.05; // 0.2;
            else
                deltalog = 0.05; // 1.0;
            LastUpdatedTime = 0.0;
        }
        else
            LastUpdatedTime = LastUpdatedTime + dt;
    }
    // Ra: skanowanie również dla prowadzonego ręcznie, aby podpowiedzieć prędkość
    if ((LastReactionTime > Min0R(ReactionTime, 2.0)))
    {
        // Ra: nie wiem czemu ReactionTime potrafi dostać 12 sekund, to jest przegięcie, bo przeżyna
        // STÓJ
        // yB: otóż jest to jedna trzecia czasu napełniania na towarowym; może się przydać przy
        // wdrażaniu hamowania, żeby nie ruszało kranem jak głupie
        // Ra: ale nie może się budzić co pół minuty, bo przeżyna semafory
        // Ra: trzeba by tak:
        // 1. Ustalić istotną odległość zainteresowania (np. 3×droga hamowania z V.max).
        fBrakeDist = fDriverBraking * mvOccupied->Vel *
                     (40.0 + mvOccupied->Vel); // przybliżona droga hamowania
        // dla hamowania -0.2 [m/ss] droga wynosi 0.389*Vel*Vel [km/h], czyli 600m dla 40km/h, 3.8km
        // dla 100km/h i 9.8km dla 160km/h
        // dla hamowania -0.4 [m/ss] droga wynosi 0.096*Vel*Vel [km/h], czyli 150m dla 40km/h, 1.0km
        // dla 100km/h i 2.5km dla 160km/h
        // ogólnie droga hamowania przy stałym opóźnieniu to Vel*Vel/(3.6*3.6*a) [m]
        // fBrakeDist powinno być wyznaczane dla danego składu za pomocą sieci neuronowych, w
        // zależności od prędkości i siły (ciśnienia) hamowania
        // następnie w drugą stronę, z drogi hamowania i chwilowej prędkości powinno być wyznaczane
        // zalecane ciśnienie
        if (fMass > 1000000.0)
            fBrakeDist *= 2.0; // korekta dla ciężkich, bo przeżynają - da to coś?
        if (mvOccupied->BrakeDelayFlag == bdelay_G)
            fBrakeDist =
                fBrakeDist +
                2 *
                    mvOccupied
                        ->Vel; // dla nastawienia G koniecznie należy wydłużyć drogę na czas reakcji
        // double scanmax=(mvOccupied->Vel>0.0)?3*fDriverDist+fBrakeDist:10.0*fDriverDist;
        double scanmax = (mvOccupied->Vel > 5.0) ?
                             400 + fBrakeDist :
                             50.0 * fDriverDist; // 1000m dla stojących pociągów; Ra 2015-01: przy
                                                 // dłuższej drodze skanowania AI jeździ spokojniej
        // 2. Sprawdzić, czy tabelka pokrywa założony odcinek (nie musi, jeśli jest STOP).
        // 3. Sprawdzić, czy trajektoria ruchu przechodzi przez zwrotnice - jeśli tak, to sprawdzić,
        // czy stan się nie zmienił.
        // 4. Ewentualnie uzupełnić tabelkę informacjami o sygnałach i ograniczeniach, jeśli się
        // "zużyła".
        TableCheck(scanmax); // wypełnianie tabelki i aktualizacja odległości
        // 5. Sprawdzić stany sygnalizacji zapisanej w tabelce, wyznaczyć prędkości.
        // 6. Z tabelki wyznaczyć krytyczną odległość i prędkość (najmniejsze przyspieszenie).
        // 7. Jeśli jest inny pojazd z przodu, ewentualnie skorygować odległość i prędkość.
        // 8. Ustalić częstotliwość świadomości AI (zatrzymanie precyzyjne - częściej, brak atrakcji
        // - rzadziej).
        if (AIControllFlag)
        { // tu bedzie logika sterowania
            if (mvOccupied->CommandIn.Command != "")
                if (!mvOccupied->RunInternalCommand()) // rozpoznaj komende bo lokomotywa jej nie
                                                       // rozpoznaje
                    RecognizeCommand(); // samo czyta komendę wstawioną do pojazdu?
            if (mvOccupied->SecuritySystem.Status > 1) // jak zadziałało CA/SHP
                if (!mvOccupied->SecuritySystemReset()) // to skasuj
                    // if
                    // ((TestFlag(mvOccupied->SecuritySystem.Status,s_ebrake))&&(mvOccupied->BrakeCtrlPos==0)&&(AccDesired>0.0))
                    if ((TestFlag(mvOccupied->SecuritySystem.Status, s_SHPebrake) ||
                         TestFlag(mvOccupied->SecuritySystem.Status, s_CAebrake)) &&
                        (mvOccupied->BrakeCtrlPos == 0) && (AccDesired > 0.0))
                        mvOccupied->BrakeLevelSet(
                            0); //!!! hm, może po prostu normalnie sterować hamulcem?
        }
        switch (OrderList[OrderPos])
        { // ustalenie prędkości przy doczepianiu i odczepianiu, dystansów w pozostałych przypadkach
        case Connect: // podłączanie do składu
            if (iDrivigFlags & moveConnect)
            { // jeśli stanął już blisko, unikając zderzenia i można próbować podłączyć
                fMinProximityDist = -0.01;
                fMaxProximityDist = 0.0; //[m] dojechać maksymalnie
                fVelPlus = 0.5; // dopuszczalne przekroczenie prędkości na ograniczeniu bez
                                // hamowania
                fVelMinus = 0.5; // margines prędkości powodujący załączenie napędu
                if (AIControllFlag)
                { // to robi tylko AI, wersję dla człowieka trzeba dopiero zrobić
                    // sprzęgi sprawdzamy w pierwszej kolejności, bo jak połączony, to koniec
                    bool ok; // true gdy się podłączy (uzyskany sprzęg będzie zgodny z żądanym)
                    if (pVehicles[0]->DirectionGet() > 0) // jeśli sprzęg 0
                    { // sprzęg 0 - próba podczepienia
                        if (pVehicles[0]->MoverParameters->Couplers[0].Connected) // jeśli jest coś
                                                                                  // wykryte (a
                                                                                  // chyba jest,
                                                                                  // nie?)
                            if (pVehicles[0]->MoverParameters->Attach(
                                    0, 2, pVehicles[0]->MoverParameters->Couplers[0].Connected,
                                    iCoupler))
                            {
                                // pVehicles[0]->dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                                // pVehicles[0]->dsbCouplerAttach->Play(0,0,0);
                            }
                        // WriteLog("CoupleDist[0]="+AnsiString(pVehicles[0]->MoverParameters->Couplers[0].CoupleDist)+",
                        // Connected[0]="+AnsiString(pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag));
                        ok = (pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag ==
                              iCoupler); // udało się? (mogło częściowo)
                    }
                    else // if (pVehicles[0]->MoverParameters->DirAbsolute<0) //jeśli sprzęg 1
                    { // sprzęg 1 - próba podczepienia
                        if (pVehicles[0]->MoverParameters->Couplers[1].Connected) // jeśli jest coś
                                                                                  // wykryte (a
                                                                                  // chyba jest,
                                                                                  // nie?)
                            if (pVehicles[0]->MoverParameters->Attach(
                                    1, 2, pVehicles[0]->MoverParameters->Couplers[1].Connected,
                                    iCoupler))
                            {
                                // pVehicles[0]->dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                                // pVehicles[0]->dsbCouplerAttach->Play(0,0,0);
                            }
                        // WriteLog("CoupleDist[1]="+AnsiString(Controlling->Couplers[1].CoupleDist)+",
                        // Connected[0]="+AnsiString(Controlling->Couplers[1].CouplingFlag));
                        ok = (pVehicles[0]->MoverParameters->Couplers[1].CouplingFlag ==
                              iCoupler); // udało się? (mogło częściowo)
                    }
                    if (ok)
                    { // jeżeli został podłączony
                        iCoupler = 0; // dalsza jazda manewrowa już bez łączenia
                        iDrivigFlags &= ~moveConnect; // zdjęcie flagi doczepiania
                        SetVelocity(0, 0, stopJoin); // wyłączyć przyspieszanie
                        CheckVehicles(); // sprawdzić światła nowego składu
                        JumpToNextOrder(); // wykonanie następnej komendy
                    }
                    else
                        SetVelocity(2.0, 0.0); // jazda w ustawionym kierunku z prędkością 2 (18s)
                } // if (AIControllFlag) //koniec zblokowania, bo była zmienna lokalna
                /* //Ra 2014-02: lepiej tam, bo jak tam się odźwieży skład, to tu pVehicles[0]
                   będzie czymś innym
                     else
                     {//jeśli człowiek ma podłączyć, to czekamy na zmianę stanu sprzęgów na końcach
                   dotychczasowego składu
                      bool ok; //true gdy się podłączy (uzyskany sprzęg będzie zgodny z żądanym)
                      if (pVehicles[0]->DirectionGet()>0) //jeśli sprzęg 0
                       ok=(pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag>0);
                   //==iCoupler); //udało się? (mogło częściowo)
                      else //if (pVehicles[0]->MoverParameters->DirAbsolute<0) //jeśli sprzęg 1
                       ok=(pVehicles[0]->MoverParameters->Couplers[1].CouplingFlag>0);
                   //==iCoupler); //udało się? (mogło częściowo)
                      if (ok)
                      {//jeżeli został podłączony
                       iDrivigFlags&=~moveConnect; //zdjęcie flagi doczepiania
                       JumpToNextOrder(); //wykonanie następnej komendy
                      }
                     }
                */
            }
            else
            { // jak daleko, to jazda jak dla Shunt na kolizję
                fMinProximityDist = 0.0;
                fMaxProximityDist = 5.0; //[m] w takim przedziale odległości powinien stanąć
                fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez
                                // hamowania
                fVelMinus = 1.0; // margines prędkości powodujący załączenie napędu
                // VelReduced=5; //[km/h]
                // if (mvOccupied->Vel<0.5) //jeśli już prawie stanął
                if (pVehicles[0]->fTrackBlock <=
                    20.0) // przy zderzeniu fTrackBlock nie jest miarodajne
                    iDrivigFlags |=
                        moveConnect; // początek podczepiania, z wyłączeniem sprawdzania fTrackBlock
            }
            break;
        case Disconnect: // 20.07.03 - manewrowanie wagonami
            fMinProximityDist = 1.0;
            fMaxProximityDist = 10.0; //[m]
            fVelPlus = 1.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 0.5; // margines prędkości powodujący załączenie napędu
            if (AIControllFlag)
            {
                if (iVehicleCount >= 0) // jeśli była podana ilość wagonów
                {
                    if (iDrivigFlags & movePress) // jeśli dociskanie w celu odczepienia
                    { // 3. faza odczepiania.
                        SetVelocity(2, 0); // jazda w ustawionym kierunku z prędkością 2
                        if ((mvControlling->MainCtrlPos > 0) ||
                            (mvOccupied->BrakeSystem == ElectroPneumatic)) // jeśli jazda
                        {
                            // WriteLog("Odczepianie w kierunku
                            // "+AnsiString(mvOccupied->DirAbsolute));
                            TDynamicObject *p =
                                pVehicle; // pojazd do odczepienia, w (pVehicle) siedzi AI
                            int d; // numer sprzęgu, który sprawdzamy albo odczepiamy
                            int n = iVehicleCount; // ile wagonów ma zostać
                            do
                            { // szukanie pojazdu do odczepienia
                                d = p->DirectionGet() > 0 ?
                                        0 :
                                        1; // numer sprzęgu od strony czoła składu
                                // if (p->MoverParameters->Couplers[d].CouplerType==Articulated)
                                // //jeśli sprzęg typu wózek (za mało)
                                if (p->MoverParameters->Couplers[d].CouplingFlag &
                                    ctrain_depot) // jeżeli sprzęg zablokowany
                                    // if (p->GetTrack()->) //a nie stoi na torze warsztatowym
                                    // (ustalić po czym poznać taki tor)
                                    ++n; // to  liczymy człony jako jeden
                                p->MoverParameters->BrakeReleaser(
                                    1); // wyluzuj pojazd, aby dało się dopychać
                                p->MoverParameters->BrakeLevelSet(
                                    0); // hamulec na zero, aby nie hamował
                                if (n)
                                { // jeśli jeszcze nie koniec
                                    p = p->Prev(); // kolejny w stronę czoła składu (licząc od
                                                   // tyłu), bo dociskamy
                                    if (!p)
                                        iVehicleCount = -2,
                                        n = 0; // nie ma co dalej sprawdzać, doczepianie zakończone
                                }
                            } while (n--);
                            if (p ? p->MoverParameters->Couplers[d].CouplingFlag == 0 : true)
                                iVehicleCount = -2; // odczepiono, co było do odczepienia
                            else if (!p->Dettach(d)) // zwraca maskę bitową połączenia; usuwa
                                                     // własność pojazdów
                            { // tylko jeśli odepnie
                                // WriteLog("Odczepiony od strony ");
                                iVehicleCount = -2;
                            } // a jak nie, to dociskać dalej
                        }
                        if (iVehicleCount >= 0) // zmieni się po odczepieniu
                            if (!mvOccupied->DecLocalBrakeLevel(1))
                            { // dociśnij sklad
                                // WriteLog("Dociskanie");
                                // mvOccupied->BrakeReleaser(); //wyluzuj lokomotywę
                                // Ready=true; //zamiast sprawdzenia odhamowania całego składu
                                IncSpeed(); // dla (Ready)==false nie ruszy
                            }
                    }
                    if ((mvOccupied->Vel == 0.0) && !(iDrivigFlags & movePress))
                    { // 2. faza odczepiania: zmień kierunek na przeciwny i dociśnij
                        // za radą yB ustawiamy pozycję 3 kranu (ruszanie kranem w innych miejscach
                        // powino zostać wyłączone)
                        // WriteLog("Zahamowanie składu");
                        // while ((mvOccupied->BrakeCtrlPos>3)&&mvOccupied->DecBrakeLevel());
                        // while ((mvOccupied->BrakeCtrlPos<3)&&mvOccupied->IncBrakeLevel());
                        mvOccupied->BrakeLevelSet(mvOccupied->BrakeSystem == ElectroPneumatic ? 1 :
                                                                                                3);
                        double p = mvOccupied->BrakePressureActual
                                       .PipePressureVal; // tu może być 0 albo -1 nawet
                        if (p < 3.9)
                            p = 3.9; // TODO: zabezpieczenie przed dziwnymi CHK do czasu wyjaśnienia
                                     // sensu 0 oraz -1 w tym miejscu
                        if (mvOccupied->BrakeSystem == ElectroPneumatic ?
                                mvOccupied->BrakePress > 2 :
                                mvOccupied->PipePress < p + 0.1)
                        { // jeśli w miarę został zahamowany (ciśnienie mniejsze niż podane na
                          // pozycji 3, zwyle 0.37)
                            if (mvOccupied->BrakeSystem == ElectroPneumatic)
                                mvOccupied->BrakeLevelSet(0); // wyłączenie EP, gdy wystarczy (może
                                                              // nie być potrzebne, bo na początku
                                                              // jest)
                            // WriteLog("Luzowanie lokomotywy i zmiana kierunku");
                            mvOccupied->BrakeReleaser(1); // wyluzuj lokomotywę; a ST45?
                            mvOccupied->DecLocalBrakeLevel(10); // zwolnienie hamulca
                            iDrivigFlags |= movePress; // następnie będzie dociskanie
                            DirectionForward(mvOccupied->ActiveDir <
                                             0); // zmiana kierunku jazdy na przeciwny (dociskanie)
                            CheckVehicles(); // od razu zmienić światła (zgasić) - bez tego się nie
                                             // odczepi
                            fStopTime = 0.0; // nie ma na co czekać z odczepianiem
                        }
                    }
                } // odczepiania
                else // to poniżej jeśli ilość wagonów ujemna
                    if (iDrivigFlags & movePress)
                { // 4. faza odczepiania: zwolnij i zmień kierunek
                    SetVelocity(0, 0, stopJoin); // wyłączyć przyspieszanie
                    if (!DecSpeed()) // jeśli już bardziej wyłączyć się nie da
                    { // ponowna zmiana kierunku
                        // WriteLog("Ponowna zmiana kierunku");
                        DirectionForward(mvOccupied->ActiveDir <
                                         0); // zmiana kierunku jazdy na właściwy
                        iDrivigFlags &= ~movePress; // koniec dociskania
                        JumpToNextOrder(); // zmieni światła
                        TableClear(); // skanowanie od nowa
                        iDrivigFlags &= ~moveStartHorn; // bez trąbienia przed ruszeniem
                        SetVelocity(fShuntVelocity, fShuntVelocity); // ustawienie prędkości jazdy
                    }
                }
            }
            break;
        case Shunt:
            // na jaką odleglość i z jaką predkością ma podjechać
            fMinProximityDist = 2.0;
            fMaxProximityDist = 4.0; //[m]
            fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 3.0; // margines prędkości powodujący załączenie napędu
            if (fVelMinus > 0.1 * fShuntVelocity)
                fVelMinus =
                    0.1 *
                    fShuntVelocity; // były problemy z jazdą np. 3km/h podczas ładowania wagonów
            break;
        case Obey_train:
            // na jaka odleglosc i z jaka predkoscia ma podjechac do przeszkody
            if (mvOccupied->CategoryFlag & 1) // jeśli pociąg
            {
                fMinProximityDist = 10.0;
                fMaxProximityDist =
                    (mvOccupied->Vel > 0.0) ?
                        20.0 :
                        50.0; //[m] jak stanie za daleko, to niech nie dociąga paru metrów
                if (iDrivigFlags & moveLate)
                {
                    fVelMinus = 1.0; // jeśli spóźniony, to gna
                    fVelPlus =
                        5.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
                }
                else
                { // gdy nie musi się sprężać
                    fVelMinus =
                        int(0.05 * VelDesired); // margines prędkości powodujący załączenie napędu
                    if (fVelMinus > 5.0)
                        fVelMinus = 5.0;
                    else if (fVelMinus < 1.0)
                        fVelMinus = 1.0; //żeby nie ruszał przy 0.1
                    fVelPlus = int(
                        0.5 +
                        0.05 * VelDesired); // normalnie dopuszczalne przekroczenie to 5% prędkości
                    if (fVelPlus > 5.0)
                        fVelPlus = 5.0; // ale nie więcej niż 5km/h
                }
            }
            else // samochod (sokista też)
            {
                fMinProximityDist = 7.0;
                fMaxProximityDist = 10.0; //[m]
                fVelPlus =
                    10.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
                fVelMinus = 2.0; // margines prędkości powodujący załączenie napędu
            }
            // VelReduced=4; //[km/h]
            break;
        default:
            fMinProximityDist = 0.01;
            fMaxProximityDist = 2.0; //[m]
            fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 5.0; // margines prędkości powodujący załączenie napędu
        } // switch
        switch (OrderList[OrderPos])
        { // co robi maszynista
        case Prepare_engine: // odpala silnik
            // if (AIControllFlag)
            if (PrepareEngine()) // dla użytkownika tylko sprawdza, czy uruchomił
            { // gotowy do drogi?
                SetDriverPsyche();
                // OrderList[OrderPos]:=Shunt; //Ra: to nie może tak być, bo scenerie robią
                // Jump_to_first_order i przechodzi w manewrowy
                JumpToNextOrder(); // w następnym jest Shunt albo Obey_train, moze też być
                                   // Change_direction, Connect albo Disconnect
                // if OrderList[OrderPos]<>Wait_for_Orders)
                // if BrakeSystem=Pneumatic)  //napelnianie uderzeniowe na wstepie
                //  if BrakeSubsystem=Oerlikon)
                //   if (BrakeCtrlPos=0))
                //    DecBrakeLevel;
            }
            break;
        case Release_engine:
            if (ReleaseEngine()) // zdana maszyna?
                JumpToNextOrder();
            break;
        case Jump_to_first_order:
            if (OrderPos > 1)
                OrderPos = 1; // w zerowym zawsze jest czekanie
            else
                ++OrderPos;
#if LOGORDERS
            WriteLog("--> Jump_to_first_order");
            OrdersDump();
#endif
            break;
        case Wait_for_orders: // jeśli czeka, też ma skanować, żeby odpalić się od semafora
        /*
             if ((mvOccupied->ActiveDir!=0))
             {//jeśli jest wybrany kierunek jazdy, można ustalić prędkość jazdy
              VelDesired=fVelMax; //wstępnie prędkość maksymalna dla pojazdu(-ów), będzie następnie
           ograniczana
              SetDriverPsyche(); //ustawia AccPreferred (potrzebne tu?)
              //Ra: odczyt (ActualProximityDist), (VelNext) i (AccPreferred) z tabelki prędkosci
              AccDesired=AccPreferred; //AccPreferred wynika z osobowości mechanika
              VelNext=VelDesired; //maksymalna prędkość wynikająca z innych czynników niż
           trajektoria ruchu
              ActualProximityDist=scanmax; //funkcja Update() może pozostawić wartości bez zmian
              //hm, kiedyś semafory wysyłały SetVelocity albo ShuntVelocity i ustawły tak VelSignal
           - a teraz jak to zrobić?
              TCommandType
           comm=TableUpdate(mvOccupied->Vel,VelDesired,ActualProximityDist,VelNext,AccDesired);
           //szukanie optymalnych wartości
             }
        */
        // break;
        case Shunt:
        case Obey_train:
        case Connect:
        case Disconnect:
        case Change_direction: // tryby wymagające jazdy
        case Change_direction | Shunt: // zmiana kierunku podczas manewrów
        case Change_direction | Connect: // zmiana kierunku podczas podłączania
            if (OrderList[OrderPos] != Obey_train) // spokojne manewry
            {
                VelSignal = Min0R(VelSignal, 40); // jeśli manewry, to ograniczamy prędkość
                if (AIControllFlag)
                { // to poniżej tylko dla AI
                    if (iVehicleCount >= 0) // jeśli jest co odczepić
                        if (!(iDrivigFlags & movePress))
                            if (mvOccupied->Vel > 0.0)
                                if (!iCoupler) // jeśli nie ma wcześniej potrzeby podczepienia
                                {
                                    SetVelocity(0, 0, stopJoin); // 1. faza odczepiania: zatrzymanie
                                    // WriteLog("Zatrzymanie w celu odczepienia");
                                }
                }
            }
            else
                SetDriverPsyche(); // Ra: było w PrepareEngine(), potrzebne tu?
            // no albo przypisujemy -WaitingExpireTime, albo porównujemy z WaitingExpireTime
            // if
            // ((VelSignal==0.0)&&(WaitingTime>WaitingExpireTime)&&(mvOccupied->RunningTrack.Velmax!=0.0))
            if (OrderList[OrderPos] &
                (Shunt | Obey_train | Connect)) // odjechać sam może tylko jeśli jest w trybie jazdy
            { // automatyczne ruszanie po odstaniu albo spod SBL
                if ((VelSignal == 0.0) && (WaitingTime > 0.0) &&
                    (mvOccupied->RunningTrack.Velmax != 0.0))
                { // jeśli stoi, a upłynął czas oczekiwania i tor ma niezerową prędkość
                    /*
                         if (WriteLogFlag)
                          {
                            append(AIlogFile);
                            writeln(AILogFile,ElapsedTime:5:2,": ",Name," V=0 waiting time expired!
                       (",WaitingTime:4:1,")");
                            close(AILogFile);
                          }
                    */
                    if ((OrderList[OrderPos] & (Obey_train | Shunt)) ?
                            (iDrivigFlags & moveStopHere) :
                            false)
                        WaitingTime = -WaitingExpireTime; // zakaz ruszania z miejsca bez otrzymania
                                                          // wolnej drogi
                    else if (mvOccupied->CategoryFlag & 1)
                    { // jeśli pociąg
                        if (AIControllFlag)
                        {
                            PrepareEngine(); // zmieni ustawiony kierunek
                            SetVelocity(20, 20); // jak się nastał, to niech jedzie 20km/h
                            WaitingTime = 0.0;
                            fWarningDuration = 1.5; // a zatrąbić trochę
                            mvOccupied->WarningSignal = 1;
                        }
                        else
                            SetVelocity(20, 20); // użytkownikowi zezwalamy jechać
                    }
                    else
                    { // samochód ma stać, aż dostanie odjazd, chyba że stoi przez kolizję
                        if (eStopReason == stopBlock)
                            if (pVehicles[0]->fTrackBlock > fDriverDist)
                                if (AIControllFlag)
                                {
                                    PrepareEngine(); // zmieni ustawiony kierunek
                                    SetVelocity(-1, -1); // jak się nastał, to niech jedzie
                                    WaitingTime = 0.0;
                                }
                                else
                                    SetVelocity(-1,
                                                -1); // użytkownikowi pozwalamy jechać (samochodem?)
                    }
                }
                else if ((VelSignal == 0.0) && (VelNext > 0.0) && (mvOccupied->Vel < 1.0))
                    if (iCoupler ? true : (iDrivigFlags & moveStopHere) == 0) // Ra: tu jest coś nie
                                                                              // tak, bo bez tego
                                                                              // warunku ruszało w
                                                                              // manewrowym !!!!
                        SetVelocity(VelNext, VelNext, stopSem); // omijanie SBL
            } // koniec samoistnego odjeżdżania
            if (AIControllFlag)
                if ((HelpMeFlag) || (mvControlling->DamageFlag > 0))
                {
                    HelpMeFlag = false;
                    /*
                          if (WriteLogFlag)
                           with Controlling do
                            {
                              append(AIlogFile);
                              writeln(AILogFile,ElapsedTime:5:2,": ",Name," HelpMe!
                       (",DamageFlag,")");
                              close(AILogFile);
                            }
                    */
                }
            if (AIControllFlag)
                if (OrderList[OrderPos] &
                    Change_direction) // może być zmieszane z jeszcze jakąś komendą
                { // sprobuj zmienic kierunek
                    SetVelocity(0, 0, stopDir); // najpierw trzeba się zatrzymać
                    if (mvOccupied->Vel < 0.1)
                    { // jeśli się zatrzymał, to zmieniamy kierunek jazdy, a nawet kabinę/człon
                        Activation(); // ustawienie zadanego wcześniej kierunku i ewentualne
                                      // przemieszczenie AI
                        PrepareEngine();
                        JumpToNextOrder(); // następnie robimy, co jest do zrobienia (Shunt albo
                                           // Obey_train)
                        if (OrderList[OrderPos] & (Shunt | Connect)) // jeśli dalej mamy manewry
                            if ((iDrivigFlags & moveStopHere) == 0) // o ile nie stać w miejscu
                            { // jechać od razu w przeciwną stronę i nie trąbić z tego tytułu
                                iDrivigFlags &= ~moveStartHorn; // bez trąbienia przed ruszeniem
                                SetVelocity(fShuntVelocity, fShuntVelocity); // to od razu jedziemy
                            }
                        // iDrivigFlags|=moveStartHorn; //a później już można trąbić
                        /*
                               if (WriteLogFlag)
                               {
                                append(AIlogFile);
                                writeln(AILogFile,ElapsedTime:5:2,": ",Name," Direction changed!");
                                close(AILogFile);
                               }
                        */
                    }
                    // else
                    // VelSignal:=0.0; //na wszelki wypadek niech zahamuje
                } // Change_direction (tylko dla AI)
            // ustalanie zadanej predkosci
            if (AIControllFlag) // jeśli prowadzi AI
                if (!iEngineActive) // jeśli silnik nie odpalony, to próbować naprawić
                    if (OrderList[OrderPos] & (Change_direction | Connect | Disconnect | Shunt |
                                               Obey_train)) // jeśli coś ma robić
                        PrepareEngine(); // to niech odpala do skutku
            if (iDrivigFlags & moveActive) // jeśli może skanować sygnały i reagować na komendy
            { // jeśli jest wybrany kierunek jazdy, można ustalić prędkość jazdy
                // Ra: tu by jeszcze trzeba było wstawić uzależnienie (VelDesired) od odległości od
                // przeszkody
                // no chyba żeby to uwzgldnić już w (ActualProximityDist)
                VelDesired = fVelMax; // wstępnie prędkość maksymalna dla pojazdu(-ów), będzie
                                      // następnie ograniczana
                if (TrainParams) // jeśli ma rozkład
                    if (TrainParams->TTVmax > 0.0) // i ograniczenie w rozkładzie
                        VelDesired = Min0R(VelDesired,
                                           TrainParams->TTVmax); // to nie przekraczać rozkladowej
                SetDriverPsyche(); // ustawia AccPreferred (potrzebne tu?)
                // Ra: odczyt (ActualProximityDist), (VelNext) i (AccPreferred) z tabelki prędkosci
                AccDesired = AccPreferred; // AccPreferred wynika z osobowości mechanika
                VelNext = VelDesired; // maksymalna prędkość wynikająca z innych czynników niż
                                      // trajektoria ruchu
                ActualProximityDist = scanmax; // funkcja Update() może pozostawić wartości bez
                                               // zmian
                // hm, kiedyś semafory wysyłały SetVelocity albo ShuntVelocity i ustawły tak
                // VelSignal - a teraz jak to zrobić?
                TCommandType comm = TableUpdate(VelDesired, ActualProximityDist, VelNext,
                                                AccDesired); // szukanie optymalnych wartości
                // if (VelSignal!=VelDesired) //jeżeli prędkość zalecana jest inna (ale tryb też
                // może być inny)
                switch (comm)
                { // ustawienie VelSignal - trochę proteza = do przemyślenia
                case cm_Ready: // W4 zezwolił na jazdę
                    TableCheck(
                        scanmax); // ewentualne doskanowanie trasy za W4, który zezwolił na jazdę
                    TableUpdate(VelDesired, ActualProximityDist, VelNext,
                                AccDesired); // aktualizacja po skanowaniu
                    // if (comm!=cm_SetVelocity) //jeśli dalej jest kolejny W4, to ma zwrócić
                    // cm_SetVelocity
                    if (VelNext == 0.0)
                        break; // ale jak coś z przodu zamyka, to ma stać
                    if (iDrivigFlags & moveStopCloser)
                        VelSignal = VelDesired; // niech jedzie, jak W4 puściło - nie, ma czekać na
                                                // sygnał z sygnalizatora!
                case cm_SetVelocity: // od wersji 357 semafor nie budzi wyłączonej lokomotywy
                    if (!(OrderList[OrderPos] &
                          ~(Obey_train | Shunt))) // jedzie w dowolnym trybie albo Wait_for_orders
                        if (fabs(VelSignal) >=
                            1.0) // 0.1 nie wysyła się do samochodow, bo potem nie ruszą
                            PutCommand("SetVelocity", VelSignal, VelNext,
                                       NULL); // komenda robi dodatkowe operacje
                    break;
                case cm_ShuntVelocity: // od wersji 357 Tm nie budzi wyłączonej lokomotywy
                    if (!(OrderList[OrderPos] &
                          ~(Obey_train | Shunt))) // jedzie w dowolnym trybie albo Wait_for_orders
                        PutCommand("ShuntVelocity", VelSignal, VelNext, NULL);
                    else if (iCoupler) // jeśli jedzie w celu połączenia
                        SetVelocity(VelSignal, VelNext);
                    break;
                case cm_Command: // komenda z komórki
                    if (!(OrderList[OrderPos] &
                          ~(Obey_train | Shunt))) // jedzie w dowolnym trybie albo Wait_for_orders
                        if (mvOccupied->Vel < 0.1) // dopiero jak stanie
                        // iDrivigFlags|=moveStopHere moveStopCloser) //chyba że stanął za daleko
                        // (SU46 w WK staje za daleko)
                        {
                            PutCommand(eSignNext->CommandGet(), eSignNext->ValueGet(1),
                                       eSignNext->ValueGet(2), NULL);
                            eSignNext->StopCommandSent(); // się wykonało już
                        }
                    break;
                }
                if (VelNext == 0.0)
                    if (!(OrderList[OrderPos] &
                          ~(Shunt | Connect))) // jedzie w Shunt albo Connect, albo Wait_for_orders
                    { // jeżeli wolnej drogi nie ma, a jest w trybie manewrowym albo oczekiwania
                        // if
                        // ((OrderList[OrderPos]&Connect)?pVehicles[0]->fTrackBlock>ActualProximityDist:true)
                        // //pomiar odległości nie działa dobrze?
                        // w trybie Connect skanować do tyłu tylko jeśli przed kolejnym sygnałem nie
                        // ma taboru do podłączenia
                        // Ra 2F1H: z tym (fTrackBlock) to nie jest najlepszy pomysł, bo lepiej by
                        // było porównać z odległością od sygnalizatora z przodu
                        if ((OrderList[OrderPos] | Connect) ? pVehicles[0]->fTrackBlock > 2000 :
                                                              true)
                            if ((comm = BackwardScan()) != cm_Unknown) // jeśli w drugą można jechać
                            { // należy sprawdzać odległość od znalezionego sygnalizatora,
                                // aby w przypadku prędkości 0.1 wyciągnąć najpierw skład za
                                // sygnalizator
                                // i dopiero wtedy zmienić kierunek jazdy, oczekując podania
                                // prędkości >0.5
                                if (comm == cm_Command) // jeśli komenda Shunt
                                    iDrivigFlags |=
                                        moveStopHere; // to ją odbierz bez przemieszczania się (np.
                                                      // odczep wagony po dopchnięciu do końca toru)
                                iDirectionOrder = -iDirection; // zmiana kierunku jazdy
                                OrderList[OrderPos] = TOrders(OrderList[OrderPos] |
                                                              Change_direction); // zmiana kierunku
                                                                                 // bez psucia
                                                                                 // kolejnych komend
                            }
                    }
                double vel = mvOccupied->Vel; // prędkość w kierunku jazdy
                if (iDirection * mvOccupied->V < 0)
                    vel = -vel; // ujemna, gdy jedzie w przeciwną stronę, niż powinien
                if (VelDesired < 0.0)
                    VelDesired = fVelMax; // bo VelDesired<0 oznacza prędkość maksymalną
                // Ra: jazda na widoczność
                if (pVehicles[0]->fTrackBlock < 1000.0) // przy 300m stał z zapamiętaną kolizją
                { // Ra 2F3F: przy jeździe pociągowej nie powinien dojeżdżać do poprzedzającego
                  // składu
                    if ((mvOccupied->CategoryFlag & 1) ?
                            ((OrderCurrentGet() & (Connect | Obey_train)) == Obey_train) :
                            false) // jeśli jesteśmy pociągiem a jazda pociągowa i nie ściąganie ze
                                   // szlaku
                    {
                        pVehicles[0]->ABuScanObjects(pVehicles[0]->DirectionGet(),
                                                     1000.0); // skanowanie sprawdzające
                        // Ra 2F3F: i jest problem, jak droga za semaforem kieruje na jakiś pojazd
                        // (np. w Skwarkach na ET22)
                        if (pVehicles[0]->fTrackBlock < 1000.0) // i jeśli nadal coś jest
                            if (VelNext != 0.0) // a następny sygnał zezwala na jazdę
                                if (pVehicles[0]->fTrackBlock <
                                    ActualProximityDist) // i jest bliżej (tu by trzeba było wstawić
                                                         // odległość do semafora, z pominięciem SBL
                                    VelDesired = 0.0; // to stoimy
                    }
                    else
                        pVehicles[0]->ABuScanObjects(pVehicles[0]->DirectionGet(),
                                                     300.0); // skanowanie sprawdzające
                }
                // if (mvOccupied->Vel>=0.1) //o ile jedziemy; jak stoimy to też trzeba jakoś
                // zatrzymywać
                if ((iDrivigFlags & moveConnect) == 0) // przy końcówce podłączania nie hamować
                { // sprawdzenie jazdy na widoczność
                    TCoupling *c =
                        pVehicles[0]->MoverParameters->Couplers +
                        (pVehicles[0]->DirectionGet() > 0 ? 0 : 1); // sprzęg z przodu składu
                    if (c->Connected) // a mamy coś z przodu
                        if (c->CouplingFlag ==
                            0) // jeśli to coś jest podłączone sprzęgiem wirtualnym
                        { // wyliczanie optymalnego przyspieszenia do jazdy na widoczność
                            double k = c->Connected->Vel; // prędkość pojazdu z przodu (zakładając,
                                                          // że jedzie w tę samą stronę!!!)
                            if (k < vel + 10) // porównanie modułów prędkości [km/h]
                            { // zatroszczyć się trzeba, jeśli tamten nie jedzie znacząco szybciej
                                double d =
                                    pVehicles[0]->fTrackBlock - 0.5 * vel -
                                    fMaxProximityDist; // odległość bezpieczna zależy od prędkości
                                if (d < 0) // jeśli odległość jest zbyt mała
                                { // AccPreferred=-0.9; //hamowanie maksymalne, bo jest za blisko
                                    if (k < 10.0) // k - prędkość tego z przodu
                                    { // jeśli tamten porusza się z niewielką prędkością albo stoi
                                        if (OrderCurrentGet() & Connect)
                                        { // jeśli spinanie, to jechać dalej
                                            AccPreferred = 0.2; // nie hamuj
                                            VelNext = VelDesired = 2.0; // i pakuj się na tamtego
                                        }
                                        else // a normalnie to hamować
                                        {
                                            AccPreferred = -1.0; // to hamuj maksymalnie
                                            VelNext = VelDesired = 0.0; // i nie pakuj się na
                                                                        // tamtego
                                        }
                                    }
                                    else // jeśli oba jadą, to przyhamuj lekko i ogranicz prędkość
                                    {
                                        if (k < vel) // jak tamten jedzie wolniej
                                            if (d < fBrakeDist) // a jest w drodze hamowania
                                            {
                                                if (AccPreferred > fAccThreshold)
                                                    AccPreferred =
                                                        fAccThreshold; // to przyhamuj troszkę
                                                VelNext = VelDesired = int(k); // to chyba już sobie
                                                                               // dohamuje według
                                                                               // uznania
                                            }
                                    }
                                    ReactionTime = 0.1; // orientuj się, bo jest goraco
                                }
                                else
                                { // jeśli odległość jest większa, ustalić maksymalne możliwe
                                  // przyspieszenie (hamowanie)
                                    k = (k * k - vel * vel) / (25.92 * d); // energia kinetyczna
                                                                           // dzielona przez masę i
                                                                           // drogę daje
                                                                           // przyspieszenie
                                    if (k > 0.0)
                                        k *= 1.5; // jedź szybciej, jeśli możesz
                                    // double ak=(c->Connected->V>0?1.0:-1.0)*c->Connected->AccS;
                                    // //przyspieszenie tamtego
                                    if (d < fBrakeDist) // a jest w drodze hamowania
                                        if (k < AccPreferred)
                                        { // jeśli nie ma innych powodów do wolniejszej jazdy
                                            AccPreferred = k;
                                            if (VelNext > c->Connected->Vel)
                                            {
                                                VelNext =
                                                    c->Connected
                                                        ->Vel; // ograniczenie do prędkości tamtego
                                                ActualProximityDist =
                                                    d; // i odległość od tamtego jest istotniejsza
                                            }
                                            ReactionTime = 0.2; // zwiększ czujność
                                        }
#if LOGVELOCITY
                                    WriteLog("Collision: AccPreferred=" + AnsiString(k));
#endif
                                }
                            }
                        }
                }
                // sprawdzamy możliwe ograniczenia prędkości
                if (OrderCurrentGet() & (Shunt | Obey_train)) // w Connect nie, bo moveStopHere
                                                              // odnosi się do stanu po połączeniu
                    if (iDrivigFlags & moveStopHere) // jeśli ma czekać na wolną drogę
                        if (vel == 0.0) // a stoi
                            if (VelNext == 0.0) // a wyjazdu nie ma
                                VelDesired = 0.0; // to ma stać
                if (fStopTime < 0) // czas postoju przed dalszą jazdą (np. na przystanku)
                    VelDesired = 0.0; // jak ma czekać, to nie ma jazdy
                // else if (VelSignal<0)
                // VelDesired=fVelMax; //ile fabryka dala (Ra: uwzględione wagony)
                else if (VelSignal >= 0) // VelSignal>0 jest ograniczeniem prędkości (z semafora)
                    VelDesired = Min0R(VelDesired, VelSignal);
                if (mvOccupied->RunningTrack.Velmax >=
                    0) // ograniczenie prędkości z trajektorii ruchu
                    VelDesired =
                        Min0R(VelDesired,
                              mvOccupied->RunningTrack.Velmax); // uwaga na ograniczenia szlakowej!
                if (VelforDriver >= 0) // tu jest zero przy zmianie kierunku jazdy
                    VelDesired = Min0R(VelDesired, VelforDriver); // Ra: tu może być 40, jeśli
                                                                  // mechanik nie ma znajomości
                                                                  // szlaaku, albo kierowca jeździ
                                                                  // 70
                if (TrainParams)
                    if (TrainParams->CheckTrainLatency() < 5.0)
                        if (TrainParams->TTVmax > 0.0)
                            VelDesired = Min0R(
                                VelDesired,
                                TrainParams
                                    ->TTVmax); // jesli nie spozniony to nie przekraczać rozkladowej
                if (VelDesired > 0.0)
                    if (VelNext > 0.0)
                    { // jeśli można jechać, to odpalić dźwięk kierownika oraz zamknąć drzwi w
                      // składzie
                        if (iDrivigFlags & moveGuardSignal)
                        { // komunikat od kierownika tu, bo musi być wolna droga i odczekany czas
                          // stania
                            iDrivigFlags &= ~moveGuardSignal; // tylko raz nadać
                            tsGuardSignal->Stop();
                            // w zasadzie to powinien mieć flagę, czy jest dźwiękiem radiowym, czy
                            // bezpośrednim
                            // albo trzeba zrobić dwa dźwięki, jeden bezpośredni, słyszalny w
                            // pobliżu, a drugi radiowy, słyszalny w innych lokomotywach
                            // na razie zakładam, że to nie jest dźwięk radiowy, bo trzeba by zrobić
                            // obsługę kanałów radiowych itd.
                            if (!iGuardRadio) // jeśli nie przez radio
                                tsGuardSignal->Play(
                                    1.0, 0, !FreeFlyModeFlag,
                                    pVehicle->GetPosition()); // dla true jest głośniej
                            else
                                // if (iGuardRadio==iRadioChannel) //zgodność kanału
                                // if (!FreeFlyModeFlag) //obserwator musi być w środku pojazdu
                                // (albo może mieć radio przenośne) - kierownik mógłby powtarzać
                                // przy braku reakcji
                                if (SquareMagnitude(pVehicle->GetPosition() -
                                                    Global::pCameraPosition) <
                                    2000 * 2000) // w odległości mniejszej niż 2km
                                tsGuardSignal->Play(
                                    1.0, 0, true,
                                    pVehicle->GetPosition()); // dźwięk niby przez radio
                        }
                        if (iDrivigFlags & moveDoorOpened) // jeśli drzwi otwarte
                            if (!mvOccupied
                                     ->DoorOpenCtrl) // jeśli drzwi niesterowane przez maszynistę
                                Doors(false); // a EZT zamknie dopiero po odegraniu komunikatu
                                              // kierownika
                    }
                if (mvOccupied->V == 0.0)
                    AbsAccS = fAccGravity; // Ra 2014-03: jesli skład stoi, to działa na niego
                                           // składowa styczna grawitacji
                else
                    AbsAccS = iDirection * mvOccupied->AccS; // przyspieszenie chwilowe, liczone
                                                             // jako różnica skierowanej prędkości w
                                                             // czasie
// if (mvOccupied->V<0.0) AbsAccS=-AbsAccS; //Ra 2014-03: to trzeba przemyśleć
// if (vel<0) //jeżeli się stacza w tył; 2014-03: to jest bez sensu, bo vel>=0
// AbsAccS=-AbsAccS; //to przyspieszenie też działa wtedy w nieodpowiednią stronę
// AbsAccS+=fAccGravity; //wypadkowe przyspieszenie (czy to ma sens?)
#if LOGVELOCITY
                // WriteLog("VelDesired="+AnsiString(VelDesired)+",
                // VelSignal="+AnsiString(VelSignal));
                WriteLog("Vel=" + AnsiString(vel) + ", AbsAccS=" + AnsiString(AbsAccS) +
                         ", AccGrav=" + AnsiString(fAccGravity));
#endif
                // ustalanie zadanego przyspieszenia
                //(ActualProximityDist) - odległość do miejsca zmniejszenia prędkości
                //(AccPreferred) - wynika z psychyki oraz uwzglęnia już ewentualne zderzenie z
                //pojazdem z przodu, ujemne gdy należy hamować
                //(AccDesired) - uwzględnia sygnały na drodze ruchu, ujemne gdy należy hamować
                //(fAccGravity) - chwilowe przspieszenie grawitacyjne, ujemne działa przeciwnie do
                //zadanego kierunku jazdy
                //(AbsAccS) - chwilowe przyspieszenie pojazu (uwzględnia grawitację), ujemne działa
                //przeciwnie do zadanego kierunku jazdy
                //(AccDesired) porównujemy z (fAccGravity) albo (AbsAccS)
                // if ((VelNext>=0.0)&&(ActualProximityDist>=0)&&(mvOccupied->Vel>=VelNext)) //gdy
                // zbliza sie i jest za szybko do NOWEGO
                if ((VelNext >= 0.0) && (ActualProximityDist <= scanmax) && (vel >= VelNext))
                { // gdy zbliża się i jest za szybki do nowej prędkości, albo stoi na zatrzymaniu
                    if (vel > 0.0)
                    { // jeśli jedzie
                        if ((vel < VelNext) ?
                                (ActualProximityDist > fMaxProximityDist * (1 + 0.1 * vel)) :
                                false) // dojedz do semafora/przeszkody
                        { // jeśli jedzie wolniej niż można i jest wystarczająco daleko, to można
                          // przyspieszyć
                            if (AccPreferred > 0.0) // jeśli nie ma zawalidrogi
                                AccDesired = AccPreferred;
                            // VelDesired:=Min0R(VelDesired,VelReduced+VelNext);
                        }
                        else if (ActualProximityDist > fMinProximityDist)
                        { // jedzie szybciej, niż trzeba na końcu ActualProximityDist, ale jeszcze
                          // jest daleko
                            if (vel <
                                VelNext + 40.0) // dwustopniowe hamowanie - niski przy małej różnicy
                            { // jeśli jedzie wolniej niż VelNext+35km/h //Ra: 40, żeby nie
                              // kombinował na zwrotnicach
                                if (VelNext == 0.0)
                                { // jeśli ma się zatrzymać, musi być to robione precyzyjnie i
                                  // skutecznie
                                    if (ActualProximityDist <
                                        fMaxProximityDist) // jak minął już maksymalny dystans
                                    { // po prostu hamuj (niski stopień) //ma stanąć, a jest w
                                      // drodze hamowania albo ma jechać
                                        AccDesired = fAccThreshold; // hamowanie tak, aby stanąć
                                        VelDesired = 0.0; // Min0R(VelDesired,VelNext);
                                    }
                                    else if (ActualProximityDist > fBrakeDist)
                                    { // jeśli ma stanąć, a mieści się w drodze hamowania
                                        if (vel < 10.0) // jeśli prędkość jest łatwa do zatrzymania
                                        { // tu jest trochę problem, bo do punktu zatrzymania dobija
                                          // na raty
                                            // AccDesired=AccDesired<0.0?0.0:0.1*AccPreferred;
                                            AccDesired = AccPreferred; // proteza trochę; jak tu
                                                                       // wychodzi 0.05, to loki
                                                                       // mają problem utrzymać
                                                                       // takie przyspieszenie
                                        }
                                        else if (vel <= 30.0) // trzymaj 30 km/h
                                            AccDesired = Min0R(0.5 * AccDesired,
                                                               AccPreferred); // jak jest tu 0.5, to
                                                                              // samochody się
                                                                              // dobijają do siebie
                                        else
                                            AccDesired = 0.0;
                                    }
                                    else // 25.92 (=3.6*3.6*2) - przelicznik z km/h na m/s
                                        if (vel <
                                            VelNext + fVelPlus) // jeśli niewielkie przekroczenie
                                        // AccDesired=0.0;
                                        AccDesired = Min0R(0.0, AccPreferred); // proteza trochę: to
                                                                               // niech nie hamuje,
                                                                               // chyba że coś z
                                                                               // przodu
                                    else
                                        AccDesired = -(vel * vel) /
                                                     (25.92 * (ActualProximityDist +
                                                               0.1)); //-fMinProximityDist));//-0.1;
                                                                      ////mniejsze opóźnienie przy
                                                                      //małej różnicy
                                    ReactionTime = 0.1; // i orientuj się szybciej, jak masz stanąć
                                }
                                else if (vel < VelNext + fVelPlus) // jeśli niewielkie
                                                                   // przekroczenie, ale ma jechać
                                    AccDesired =
                                        Min0R(0.0, AccPreferred); // to olej (zacznij luzować)
                                else
                                { // jeśli większe przekroczenie niż fVelPlus [km/h], ale ma jechać
                                    // Ra 2F1I: jak było (VelNext+fVelPlus) tu, to hamował zbyt
                                    // późno przed 40, a potem zbyt mocno i zwalniał do 30
                                    AccDesired = (VelNext * VelNext - vel * vel) /
                                                 (25.92 * ActualProximityDist +
                                                  0.1); // mniejsze opóźnienie przy małej różnicy
                                    if (ActualProximityDist < fMaxProximityDist)
                                        ReactionTime = 0.1; // i orientuj się szybciej, jeśli w
                                                            // krytycznym przedziale
                                }
                            }
                            else // przy dużej różnicy wysoki stopień (1,25 potrzebnego opoznienia)
                                AccDesired = (VelNext * VelNext - vel * vel) /
                                             (20.73 * ActualProximityDist +
                                              0.1); // najpierw hamuje mocniej, potem zluzuje
                            if (AccPreferred < AccDesired)
                                AccDesired = AccPreferred; //(1+abs(AccDesired))
                            // ReactionTime=0.5*mvOccupied->BrakeDelay[2+2*mvOccupied->BrakeDelayFlag];
                            // //aby szybkosc hamowania zalezala od przyspieszenia i opoznienia
                            // hamulcow
                            // fBrakeTime=0.5*mvOccupied->BrakeDelay[2+2*mvOccupied->BrakeDelayFlag];
                            // //aby szybkosc hamowania zalezala od przyspieszenia i opoznienia
                            // hamulcow
                        }
                        else
                        { // jest bliżej niż fMinProximityDist
                            VelDesired =
                                Min0R(VelDesired, VelNext); // utrzymuj predkosc bo juz blisko
                            if (vel <
                                VelNext + fVelPlus) // jeśli niewielkie przekroczenie, ale ma jechać
                                AccDesired = Min0R(0.0, AccPreferred); // to olej (zacznij luzować)
                            ReactionTime = 0.1; // i orientuj się szybciej
                        }
                    }
                    else // zatrzymany (vel==0.0)
                        // if (iDrivigFlags&moveStopHere) //to nie dotyczy podczepiania
                        // if ((VelNext>0.0)||(ActualProximityDist>fMaxProximityDist*1.2))
                        if (VelNext > 0.0)
                        AccDesired = AccPreferred; // można jechać
                    else // jeśli daleko jechać nie można
                        if (ActualProximityDist >
                            fMaxProximityDist) // ale ma kawałek do sygnalizatora
                    { // if ((iDrivigFlags&moveStopHere)?false:AccPreferred>0)
                        if (AccPreferred > 0)
                            AccDesired = AccPreferred; // dociagnij do semafora;
                        else
                            VelDesired = 0.0; //,AccDesired=-fabs(fAccGravity); //stoj (hamuj z siłą
                                              //równą składowej stycznej grawitacji)
                    }
                    else
                        VelDesired = 0.0; // VelNext=0 i stoi bliżej niż fMaxProximityDist
                }
                else // gdy jedzie wolniej niż potrzeba, albo nie ma przeszkód na drodze
                    AccDesired = (VelDesired != 0.0 ? AccPreferred : -0.01); // normalna jazda
                // koniec predkosci nastepnej
                if ((VelDesired >= 0.0) &&
                    (vel > VelDesired)) // jesli jedzie za szybko do AKTUALNEGO
                    if (VelDesired == 0.0) // jesli stoj, to hamuj, ale i tak juz za pozno :)
                        AccDesired = -0.9; // hamuj solidnie
                    else if ((vel < VelDesired + fVelPlus)) // o 5 km/h to olej
                    {
                        if ((AccDesired > 0.0))
                            AccDesired = 0.0;
                    }
                    else
                        AccDesired = fAccThreshold; // hamuj tak średnio
                // koniec predkosci aktualnej
                if (fAccThreshold > -0.3) // bez sensu, ale dla towarowych korzystnie
                { // Ra 2014-03: to nie uwzględnia odległości i zaczyna hamować, jak tylko zobaczy
                  // W4
                    if ((AccDesired > 0.0) &&
                        (VelNext >= 0.0)) // wybieg bądź lekkie hamowanie, warunki byly zamienione
                        if (vel > VelNext + 100.0) // lepiej zaczac hamowac
                            AccDesired = fAccThreshold;
                        else if (vel > VelNext + 70.0)
                            AccDesired = 0.0; // nie spiesz się, bo będzie hamowanie
                    // koniec wybiegu i hamowania
                }
                if (AIControllFlag)
                { // część wykonawcza tylko dla AI, dla człowieka jedynie napisy
                    if (mvControlling->ConvOvldFlag ||
                        !mvControlling->Mains) // WS może wywalić z powodu błędu w drutach
                    { // wywalił bezpiecznik nadmiarowy przetwornicy
                        // while (DecSpeed()); //zerowanie napędu
                        // Controlling->ConvOvldFlag=false; //reset nadmiarowego
                        PrepareEngine(); // próba ponownego załączenia
                    }
                    // włączanie bezpiecznika
                    if ((mvControlling->EngineType == ElectricSeriesMotor) ||
                        (mvControlling->TrainType & dt_EZT) ||
                        (mvControlling->EngineType == DieselElectric))
                        if (mvControlling->FuseFlag || Need_TryAgain)
                        {
                            Need_TryAgain =
                                false; // true, jeśli druga pozycja w elektryku nie załapała
                            // if (!Controlling->DecScndCtrl(1)) //kręcenie po mału
                            // if (!Controlling->DecMainCtrl(1)) //nastawnik jazdy na 0
                            mvControlling->DecScndCtrl(2); // nastawnik bocznikowania na 0
                            mvControlling->DecMainCtrl(2); // nastawnik jazdy na 0
                            mvControlling->MainSwitch(
                                true); // Ra: dodałem, bo EN57 stawały po wywaleniu
                            if (!mvControlling->FuseOn())
                                HelpMeFlag = true;
                            else
                            {
                                ++iDriverFailCount;
                                if (iDriverFailCount > maxdriverfails)
                                    Psyche = Easyman;
                                if (iDriverFailCount > maxdriverfails * 2)
                                    SetDriverPsyche();
                            }
                        }
                    if (mvOccupied->BrakeSystem == Pneumatic) // napełnianie uderzeniowe
                        if (mvOccupied->BrakeHandle == FV4a)
                        {
                            if (mvOccupied->BrakeCtrlPos == -2)
                                mvOccupied->BrakeLevelSet(0);
                            //        if
                            //        ((mvOccupied->BrakeCtrlPos<0)&&(mvOccupied->PipeBrakePress<0.01))//{(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)})
                            //         mvOccupied->IncBrakeLevel();
                            if ((mvOccupied->PipePress < 3.0) && (AccDesired > -0.03))
                                mvOccupied->BrakeReleaser(1);
                            if ((mvOccupied->BrakeCtrlPos == 0) && (AbsAccS < 0.0) &&
                                (AccDesired > -0.03))
                                // if FuzzyLogicAI(CntrlPipePress-PipePress,0.01,1))
                                //         if
                                //         ((mvOccupied->BrakePress>0.5)&&(mvOccupied->LocalBrakePos<0.5))//{((Volume/BrakeVVolume/10)<0.485)})
                                if ((mvOccupied->EqvtPipePress < 4.95) &&
                                    (fReady > 0.35)) //{((Volume/BrakeVVolume/10)<0.485)})
                                {
                                    if (iDrivigFlags &
                                        moveOerlikons) // a reszta składu jest na to gotowa
                                        mvOccupied->BrakeLevelSet(-1); // napełnianie w Oerlikonie
                                }
                                else if (Need_BrakeRelease)
                                {
                                    Need_BrakeRelease = false;
                                    mvOccupied->BrakeReleaser(1);
                                    // DecBrakeLevel(); //z tym by jeszcze miało jakiś sens
                                }
                            //        if
                            //        ((mvOccupied->BrakeCtrlPos<0)&&(mvOccupied->BrakePress<0.3))//{(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)})
                            if ((mvOccupied->BrakeCtrlPos < 0) &&
                                (mvOccupied->EqvtPipePress >
                                 (fReady < 0.25 ?
                                      5.1 :
                                      5.2))) //{(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)})
                                mvOccupied->IncBrakeLevel();
                        }
#if LOGVELOCITY
                    WriteLog("Dist=" + FloatToStrF(ActualProximityDist, ffFixed, 7, 1) +
                             ", VelDesired=" + FloatToStrF(VelDesired, ffFixed, 7, 1) +
                             ", AccDesired=" + FloatToStrF(AccDesired, ffFixed, 7, 3) +
                             ", VelSignal=" + AnsiString(VelSignal) + ", VelNext=" +
                             AnsiString(VelNext));
#endif
                    if (AccDesired > 0.1)
                        if (vel < 10.0) // Ra 2F1H: jeśli prędkość jest mała, a można przyspieszać,
                                        // to nie ograniczać przyspieszenia do 0.5m/ss
                            AccDesired = 0.9; // przy małych prędkościach może być trudno utrzymać
                                              // małe przyspieszenie
                    // Ra 2F1I: wyłączyć kiedyś to uśrednianie i przeanalizować skanowanie, czemu
                    // migocze
                    if (AccDesired > -0.15) // hamowania lepeiej nie uśredniać
                        AccDesired = fAccDesiredAv =
                            0.2 * AccDesired +
                            0.8 * fAccDesiredAv; // uśrednione, żeby ograniczyć migotanie
                    if (VelDesired == 0.0)
                        if (AccDesired >= -0.01)
                            AccDesired = -0.01; // Ra 2F1J: jeszcze jedna prowizoryczna łatka
                    if (AccDesired >= 0.0)
                        if (iDrivigFlags & movePress)
                            mvOccupied->BrakeReleaser(1); // wyluzuj lokomotywę - może być więcej!
                        else if (OrderList[OrderPos] !=
                                 Disconnect) // przy odłączaniu nie zwalniamy tu hamulca
                            if ((AccDesired > 0.0) ||
                                (fAccGravity * fAccGravity <
                                 0.001)) // luzuj tylko na plaskim lub przy ruszaniu
                            {
                                while (DecBrake())
                                    ; // jeśli przyspieszamy, to nie hamujemy
                                if (mvOccupied->BrakePress > 0.4)
                                    mvOccupied->BrakeReleaser(
                                        1); // wyluzuj lokomotywę, to szybciej ruszymy
                            }
                    // margines dla prędkości jest doliczany tylko jeśli oczekiwana prędkość jest
                    // większa od 5km/h
                    if (!(iDrivigFlags & movePress))
                    { // jeśli nie dociskanie
                        if (AccDesired < -0.1)
                            while (DecSpeed())
                                ; // jeśli hamujemy, to nie przyspieszamy
                        else if (((fAccGravity < -0.01) ? AccDesired < 0.0 :
                                                          AbsAccS > AccDesired) ||
                                 (vel > VelDesired)) // jak za bardzo przyspiesza albo prędkość
                                                     // przekroczona
                            DecSpeed(); // pojedyncze cofnięcie pozycji, bo na zero to przesada
                    }
                    // yB: usunięte różne dziwne warunki, oddzielamy część zadającą od wykonawczej
                    // zwiekszanie predkosci
                    // Ra 2F1H: jest konflikt histerezy pomiędzy nastawioną pozycją a uzyskiwanym
                    // przyspieszeniem - utrzymanie pozycji powoduje przekroczenie przyspieszenia
                    if (AbsAccS <
                        AccDesired) // jeśli przyspieszenie pojazdu jest mniejsze niż żądane oraz
                        if (vel < VelDesired - fVelMinus) // jeśli prędkość w kierunku czoła jest
                                                          // mniejsza od dozwolonej o margines
                            if ((ActualProximityDist > fMaxProximityDist) ? true : (vel < VelNext))
                                IncSpeed(); // to można przyspieszyć
                    // if ((AbsAccS<AccDesired)&&(vel<VelDesired))
                    // if (!MaxVelFlag) //Ra: to nie jest używane
                    // if (!IncSpeed()) //Ra: to tutaj jest bez sensu, bo nie dociągnie do
                    // bezoporowej
                    // MaxVelFlag=true; //Ra: to nie jest używane
                    // else
                    // MaxVelFlag=false; //Ra: to nie jest używane
                    // if (Vel<VelDesired*0.85) and (AccDesired>0) and
                    // (EngineType=ElectricSeriesMotor)
                    // and (RList[MainCtrlPos].R>0.0) and (not DelayCtrlFlag))
                    // if (Im<Imin) and Ready=True {(BrakePress<0.01*MaxBrakePress)})
                    //  IncMainCtrl(1); //zwieksz nastawnik skoro możesz - tak aby się ustawic na
                    //  bezoporowej

                    // yB: usunięte różne dziwne warunki, oddzielamy część zadającą od wykonawczej
                    // zmniejszanie predkosci
                    if (mvOccupied->TrainType &
                        dt_EZT) // właściwie, to warunek powinien być na działający EP
                    { // Ra: to dobrze hamuje EP w EZT
                        if ((AccDesired <= fAccThreshold) ? // jeśli hamować - u góry ustawia się
                                                            // hamowanie na fAccThreshold
                                ((AbsAccS > AccDesired) || (mvOccupied->BrakeCtrlPos < 0)) :
                                false) // hamować bardziej, gdy aktualne opóźnienie hamowania
                                       // mniejsze niż (AccDesired)
                            IncBrake();
                        else if (OrderList[OrderPos] !=
                                 Disconnect) // przy odłączaniu nie zwalniamy tu hamulca
                            if (AbsAccS <
                                AccDesired -
                                    0.05) // jeśli opóźnienie większe od wymaganego (z histerezą)
                            { // luzowanie, gdy za dużo
                                if (mvOccupied->BrakeCtrlPos >= 0)
                                    DecBrake(); // tutaj zmniejszało o 1 przy odczepianiu
                            }
                            else if (mvOccupied->Handle->TimeEP)
                            {
                                if (mvOccupied->Handle->GetPos(bh_EPR) -
                                        mvOccupied->Handle->GetPos(bh_EPN) <
                                    0.1)
                                    mvOccupied->SwitchEPBrake(0);
                                else
                                    mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EPN));
                            }
                        //         else if (mvOccupied->BrakeCtrlPos<0) IncBrake(); //ustawienie
                        //         jazdy (pozycja 0)
                        //         else if (mvOccupied->BrakeCtrlPos>0) DecBrake();
                    }
                    else
                    { // a stara wersja w miarę dobrze działa na składy wagonowe
                        //       if (mvOccupied->Handle->Time)
                        //         mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_MB));
                        //         //najwyzej sobie przestawi
                        if (((fAccGravity < -0.05) && (vel < 0)) ||
                            ((AccDesired < fAccGravity - 0.1) &&
                             (AbsAccS >
                              AccDesired + 0.05))) // u góry ustawia się hamowanie na fAccThreshold
                            // if not MinVelFlag)
                            if (fBrakeTime < 0 ? true : (AccDesired < fAccGravity - 0.3) ||
                                                            (mvOccupied->BrakeCtrlPos <= 0))
                                if (!IncBrake()) // jeśli upłynął czas reakcji hamulca, chyba że
                                                 // nagłe albo luzował
                                    MinVelFlag = true;
                                else
                                {
                                    MinVelFlag = false;
                                    fBrakeTime =
                                        3 +
                                        0.5 *
                                            (mvOccupied
                                                 ->BrakeDelay[2 + 2 * mvOccupied->BrakeDelayFlag] -
                                             3);
                                    // Ra: ten czas należy zmniejszyć, jeśli czas dojazdu do
                                    // zatrzymania jest mniejszy
                                    fBrakeTime *= 0.5; // Ra: tymczasowo, bo przeżyna S1
                                }
                        if ((AccDesired < fAccGravity - 0.05) && (AbsAccS < AccDesired - 0.2))
                        // if ((AccDesired<0.0)&&(AbsAccS<AccDesired-0.1)) //ST44 nie hamuje na
                        // czas, 2-4km/h po minięciu tarczy
                        // if (fBrakeTime<0)
                        { // jak hamuje, to nie tykaj kranu za często
                            // yB: luzuje hamulec dopiero przy różnicy opóźnień rzędu 0.2
                            if (OrderList[OrderPos] !=
                                Disconnect) // przy odłączaniu nie zwalniamy tu hamulca
                                DecBrake(); // tutaj zmniejszało o 1 przy odczepianiu
                            fBrakeTime =
                                (mvOccupied->BrakeDelay[1 + 2 * mvOccupied->BrakeDelayFlag]) / 3.0;
                            fBrakeTime *= 0.5; // Ra: tymczasowo, bo przeżyna S1
                        }
                    }
                    // Mietek-end1
                    SpeedSet(); // ciągla regulacja prędkości
#if LOGVELOCITY
                    WriteLog("BrakePos=" + AnsiString(mvOccupied->BrakeCtrlPos) + ", MainCtrl=" +
                             AnsiString(mvControlling->MainCtrlPos));
#endif

                    /* //Ra: mamy teraz wskażnik na człon silnikowy, gorzej jak są dwa w
                       ukrotnieniu...
                          //zapobieganie poslizgowi w czlonie silnikowym; Ra: Couplers[1] powinno
                       być
                          if (Controlling->Couplers[0].Connected!=NULL)
                           if (TestFlag(Controlling->Couplers[0].CouplingFlag,ctrain_controll))
                            if (Controlling->Couplers[0].Connected->SlippingWheels)
                             if (Controlling->ScndCtrlPos>0?!Controlling->DecScndCtrl(1):true)
                             {
                              if (!Controlling->DecMainCtrl(1))
                               if (mvOccupied->BrakeCtrlPos==mvOccupied->BrakeCtrlPosNo)
                                mvOccupied->DecBrakeLevel();
                              ++iDriverFailCount;
                             }
                    */
                    // zapobieganie poslizgowi u nas
                    if (mvControlling->SlippingWheels)
                    {
                        if (!mvControlling->DecScndCtrl(2)) // bocznik na zero
                            mvControlling->DecMainCtrl(1);
                        if (mvOccupied->BrakeCtrlPos ==
                            mvOccupied->BrakeCtrlPosNo) // jeśli ostatnia pozycja hamowania
                            mvOccupied->DecBrakeLevel(); // to cofnij hamulec
                        else
                            mvControlling->AntiSlippingButton();
                        ++iDriverFailCount;
                        mvControlling->SlippingWheels = false; // flaga już wykorzystana
                    }
                    if (iDriverFailCount > maxdriverfails)
                    {
                        Psyche = Easyman;
                        if (iDriverFailCount > maxdriverfails * 2)
                            SetDriverPsyche();
                    }
                } // if (AIControllFlag)
                else
                { // tu mozna dać komunikaty tekstowe albo słowne: przyspiesz, hamuj (lekko,
                  // średnio, mocno)
                }
            } // kierunek różny od zera
            else
            { // tutaj, gdy pojazd jest wyłączony
                if (!AIControllFlag) // jeśli sterowanie jest w gestii użytkownika
                    if (mvOccupied->Battery) // czy użytkownik załączył baterię?
                        if (mvOccupied->ActiveDir) // czy ustawił kierunek
                        { // jeśli tak, to uruchomienie skanowania
                            CheckVehicles(); // sprawdzić skład
                            TableClear(); // resetowanie tabelki skanowania
                            PrepareEngine(); // uruchomienie
                        }
            }
            if (AIControllFlag)
            { // odhamowywanie składu po zatrzymaniu i zabezpieczanie lokomotywy
                if ((OrderList[OrderPos] & (Disconnect | Connect)) ==
                    0) // przy (p)odłączaniu nie zwalniamy tu hamulca
                    if ((mvOccupied->V == 0.0) && ((VelDesired == 0.0) || (AccDesired == 0.0)))
                        if ((mvOccupied->BrakeCtrlPos < 1) || !mvOccupied->DecBrakeLevel())
                            mvOccupied->IncLocalBrakeLevel(1); // dodatkowy na pozycję 1
            }
            break; // rzeczy robione przy jezdzie
        } // switch (OrderList[OrderPos])
        // kasowanie licznika czasu
        LastReactionTime = 0.0;
        UpdateOK = true;
    } // if ((LastReactionTime>Min0R(ReactionTime,2.0)))
    else
        LastReactionTime += dt;

    if ((fLastStopExpDist > 0.0) && (mvOccupied->DistCounter > fLastStopExpDist))
    {
        iStationStart = TrainParams->StationIndex; // zaktualizować wyświetlanie rozkładu
        fLastStopExpDist = -1.0f; // usunąć licznik
    }

    if (AIControllFlag)
    {
        if (fWarningDuration > 0.0) // jeśli pozostało coś do wytrąbienia
        { // trąbienie trwa nadal
            fWarningDuration = fWarningDuration - dt;
            if (fWarningDuration < 0.05)
                mvOccupied->WarningSignal = 0; // a tu się kończy
            if (ReactionTime > fWarningDuration)
                ReactionTime =
                    fWarningDuration; // wcześniejszy przebłysk świadomości, by zakończyć trąbienie
        }
        if (mvOccupied->Vel >=
            3.0) // jesli jedzie, można odblokować trąbienie, bo się wtedy nie włączy
        {
            iDrivigFlags &= ~moveStartHornDone; // zatrąbi dopiero jak następnym razem stanie
            iDrivigFlags |= moveStartHorn; // i trąbić przed następnym ruszeniem
        }
        return UpdateOK;
    }
    else // if (AIControllFlag)
        return false; // AI nie obsługuje
}

void TController::JumpToNextOrder()
{ // wykonanie kolejnej komendy z tablicy rozkazów
    if (OrderList[OrderPos] != Wait_for_orders)
    {
        if (OrderList[OrderPos] & Change_direction) // jeśli zmiana kierunku
            if (OrderList[OrderPos] != Change_direction) // ale nałożona na coś
            {
                OrderList[OrderPos] =
                    TOrders(OrderList[OrderPos] &
                            ~Change_direction); // usunięcie zmiany kierunku z innej komendy
                OrderCheck();
                return;
            }
        if (OrderPos < maxorders - 1)
            ++OrderPos;
        else
            OrderPos = 0;
    }
    OrderCheck();
#if LOGORDERS
    WriteLog("--> JumpToNextOrder");
    OrdersDump(); // normalnie nie ma po co tego wypisywać
#endif
};

void TController::JumpToFirstOrder()
{ // taki relikt
    OrderPos = 1;
    if (OrderTop == 0)
        OrderTop = 1;
    OrderCheck();
#if LOGORDERS
    WriteLog("--> JumpToFirstOrder");
    OrdersDump(); // normalnie nie ma po co tego wypisywać
#endif
};

void TController::OrderCheck()
{ // reakcja na zmianę rozkazu
    if (OrderList[OrderPos] & (Shunt | Connect | Obey_train))
        CheckVehicles(); // sprawdzić światła
    if (OrderList[OrderPos] & Change_direction) // może być nałożona na inną i wtedy ma priorytet
        iDirectionOrder = -iDirection; // trzeba zmienić jawnie, bo się nie domyśli
    else if (OrderList[OrderPos] == Obey_train)
        iDrivigFlags |= moveStopPoint; // W4 są widziane
    else if (OrderList[OrderPos] == Disconnect)
        iVehicleCount = iVehicleCount < 0 ? 0 : iVehicleCount; // odczepianie lokomotywy
    else if (OrderList[OrderPos] == Connect)
        iDrivigFlags &= ~moveStopPoint; // podczas jazdy na połączenie nie zwracać uwagi na W4
    else if (OrderList[OrderPos] == Wait_for_orders)
        OrdersClear(); // czyszczenie rozkazów i przeskok do zerowej pozycji
}

void TController::OrderNext(TOrders NewOrder)
{ // ustawienie rozkazu do wykonania jako następny
    if (OrderList[OrderPos] == NewOrder)
        return; // jeśli robi to, co trzeba, to koniec
    if (!OrderPos)
        OrderPos = 1; // na pozycji zerowej pozostaje czekanie
    OrderTop = OrderPos; // ale może jest czymś zajęty na razie
    if (NewOrder >= Shunt) // jeśli ma jechać
    { // ale może być zajęty chwilowymi operacjami
        while (OrderList[OrderTop] ? OrderList[OrderTop] < Shunt : false) // jeśli coś robi
            ++OrderTop; // pomijamy wszystkie tymczasowe prace
    }
    else
    { // jeśli ma ustawioną jazdę, to wyłączamy na rzecz operacji
        while (OrderList[OrderTop] ?
                   (OrderList[OrderTop] < Shunt) && (OrderList[OrderTop] != NewOrder) :
                   false) // jeśli coś robi
            ++OrderTop; // pomijamy wszystkie tymczasowe prace
    }
    OrderList[OrderTop++] = NewOrder; // dodanie rozkazu jako następnego
#if LOGORDERS
    WriteLog("--> OrderNext");
    OrdersDump(); // normalnie nie ma po co tego wypisywać
#endif
}

void TController::OrderPush(TOrders NewOrder)
{ // zapisanie na stosie kolejnego rozkazu do wykonania
    if (OrderPos == OrderTop) // jeśli miałby być zapis na aktalnej pozycji
        if (OrderList[OrderPos] < Shunt) // ale nie jedzie
            ++OrderTop; // niektóre operacje muszą zostać najpierw dokończone => zapis na kolejnej
    if (OrderList[OrderTop] != NewOrder) // jeśli jest to samo, to nie dodajemy
        OrderList[OrderTop++] = NewOrder; // dodanie rozkazu na stos
    // if (OrderTop<OrderPos) OrderTop=OrderPos;
    if (OrderTop >= maxorders)
        ErrorLog("Commands overflow: The program will now crash");
#if LOGORDERS
    WriteLog("--> OrderPush");
    OrdersDump(); // normalnie nie ma po co tego wypisywać
#endif
}

void TController::OrdersDump()
{ // wypisanie kolejnych rozkazów do logu
    WriteLog("Orders for " + pVehicle->asName + ":");
    for (int b = 0; b < maxorders; ++b)
    {
        WriteLog(AnsiString(b) + ": " + Order2Str(OrderList[b]) + (OrderPos == b ? " <-" : ""));
        if (b) // z wyjątkiem pierwszej pozycji
            if (OrderList[b] == Wait_for_orders) // jeśli końcowa komenda
                break; // dalej nie trzeba
    }
};

TOrders TController::OrderCurrentGet() { return OrderList[OrderPos]; }

TOrders TController::OrderNextGet() { return OrderList[OrderPos + 1]; }

void TController::OrdersInit(double fVel)
{ // wypełnianie tabelki rozkazów na podstawie rozkładu
    // ustawienie kolejności komend, niezależnie kto prowadzi
    // Mechanik->OrderPush(Wait_for_orders); //czekanie na lepsze czasy
    // OrderPos=OrderTop=0; //wypełniamy od pozycji 0
    OrdersClear(); // usunięcie poprzedniej tabeli
    OrderPush(Prepare_engine); // najpierw odpalenie silnika
    if (TrainParams->TrainName == AnsiString("none"))
    { // brak rozkładu to jazda manewrowa
        if (fVel > 0.05) // typowo 0.1 oznacza gotowość do jazdy, 0.01 tylko załączenie silnika
            OrderPush(Shunt); // jeśli nie ma rozkładu, to manewruje
    }
    else
    { // jeśli z rozkładem, to jedzie na szlak
        if ((fVel > 0.0) && (fVel < 0.02))
            OrderPush(Shunt); // dla prędkości 0.01 włączamy jazdę manewrową
        else if (TrainParams ?
                     (TrainParams->TimeTable[1].StationWare.Pos(
                          "@") ? // jeśli obrót na pierwszym przystanku
                          ((iDrivigFlags &
                            movePushPull) ? // SZT również! SN61 zależnie od wagonów...
                               (TrainParams->TimeTable[1].StationName == TrainParams->Relation1) :
                               false) :
                          false) :
                     true)
            OrderPush(Shunt); // a teraz start będzie w manewrowym, a tryb pociągowy włączy W4
        else
            // jeśli start z pierwszej stacji i jednocześnie jest na niej zmiana kierunku, to EZT ma
            // mieć Shunt
            OrderPush(Obey_train); // dla starych scenerii start w trybie pociagowym
        if (DebugModeFlag) // normalnie nie ma po co tego wypisywać
            WriteLog("/* Timetable: " + TrainParams->ShowRelation());
        TMTableLine *t;
        for (int i = 0; i <= TrainParams->StationCount; ++i)
        {
            t = TrainParams->TimeTable + i;
            if (DebugModeFlag) // normalnie nie ma po co tego wypisywać
                WriteLog(AnsiString(t->StationName) + " " + AnsiString((int)t->Ah) + ":" +
                         AnsiString((int)t->Am) + ", " + AnsiString((int)t->Dh) + ":" +
                         AnsiString((int)t->Dm) + " " + AnsiString(t->StationWare));
            if (AnsiString(t->StationWare).Pos("@"))
            { // zmiana kierunku i dalsza jazda wg rozkładu
                if (iDrivigFlags & movePushPull) // SZT również! SN61 zależnie od wagonów...
                { // jeśli skład zespolony, wystarczy zmienić kierunek jazdy
                    OrderPush(Change_direction); // zmiana kierunku
                }
                else
                { // dla zwykłego składu wagonowego odczepiamy lokomotywę
                    OrderPush(Disconnect); // odczepienie lokomotywy
                    OrderPush(Shunt); // a dalej manewry
                    if (i <= TrainParams->StationCount) // 130827: to się jednak nie sprawdza
                    { //"@" na ostatniej robi tylko odpięcie
                        // OrderPush(Change_direction); //zmiana kierunku
                        // OrderPush(Shunt); //jazda na drugą stronę składu
                        // OrderPush(Change_direction); //zmiana kierunku
                        // OrderPush(Connect); //jazda pod wagony
                    }
                }
                if (i < TrainParams->StationCount) // jak nie ostatnia stacja
                    OrderPush(Obey_train); // to dalej wg rozkładu
            }
        }
        if (DebugModeFlag) // normalnie nie ma po co tego wypisywać
            WriteLog("*/");
        OrderPush(Shunt); // po wykonaniu rozkładu przełączy się na manewry
    }
    // McZapkie-100302 - to ma byc wyzwalane ze scenerii
    if (fVel == 0.0)
        SetVelocity(0, 0, stopSleep); // jeśli nie ma prędkości początkowej, to śpi
    else
    { // jeśli podana niezerowa prędkość
        if ((fVel >= 1.0) ||
            (fVel < 0.02)) // jeśli ma jechać - dla 0.01 ma podjechać manewrowo po podaniu sygnału
            iDrivigFlags = (iDrivigFlags & ~moveStopHere) |
                           moveStopCloser; // to do następnego W4 ma podjechać blisko
        else
            iDrivigFlags |= moveStopHere; // czekać na sygnał
        JumpToFirstOrder();
        if (fVel >= 1.0) // jeśli ma jechać
            SetVelocity(fVel, -1); // ma ustawić żądaną prędkość
        else
            SetVelocity(0, 0, stopSleep); // prędkość w przedziale (0;1) oznacza, że ma stać
    }
#if LOGORDERS
    WriteLog("--> OrdersInit");
#endif
    if (DebugModeFlag) // normalnie nie ma po co tego wypisywać
        OrdersDump(); // wypisanie kontrolne tabelki rozkazów
    // McZapkie! - zeby w ogole AI ruszyl to musi wykonac powyzsze rozkazy
    // Ale mozna by je zapodac ze scenerii
};

AnsiString TController::StopReasonText()
{ // informacja tekstowa o przyczynie zatrzymania
    if (eStopReason != 7) // zawalidroga będzie inaczej
        return StopReasonTable[eStopReason];
    else
        return "Blocked by " + (pVehicles[0]->PrevAny()->GetName());
};

//----------------------------------------------------------------------------------------------------------------------
// McZapkie: skanowanie semaforów
// Ra: stare funkcje skanujące, używane podczas manewrów do szukania sygnalizatora z tyłu
//- nie reagują na PutValues, bo nie ma takiej potrzeby
//- rozpoznają tylko zerową prędkość (jako koniec toru i brak podstaw do dalszego skanowania)
//----------------------------------------------------------------------------------------------------------------------

/* //nie używane
double TController::Distance(vector3 &p1,vector3 &n,vector3 &p2)
{//Ra:obliczenie odległości punktu (p1) od płaszczyzny o wektorze normalnym (n) przechodzącej przez
(p2)
 return n.x*(p1.x-p2.x)+n.y*(p1.y-p2.y)+n.z*(p1.z-p2.z); //ax1+by1+cz1+d, gdzie d=-(ax2+by2+cz2)
};
*/

bool TController::BackwardTrackBusy(TTrack *Track)
{ // najpierw sprawdzamy, czy na danym torze są pojazdy z innego składu
    if (Track->iNumDynamics)
    { // jeśli tylko z własnego składu, to tor jest wolny
        for (int i = 0; i < Track->iNumDynamics; ++i)
            if (Track->Dynamics[i]->ctOwner != this) // jeśli jest jakiś cudzy
                return true; // to tor jest zajęty i skanowanie nie obowiązuje
    }
    return false; // wolny
};

TEvent *__fastcall TController::CheckTrackEventBackward(double fDirection, TTrack *Track)
{ // sprawdzanie eventu w torze, czy jest sygnałowym - skanowanie do tyłu
    TEvent *e = (fDirection > 0) ? Track->evEvent2 : Track->evEvent1;
    if (e)
        if (!e->bEnabled) // jeśli sygnałowy (nie dodawany do kolejki)
            if (e->Type == tp_GetValues) // PutValues nie może się zmienić
                return e;
    return NULL;
};

TTrack *__fastcall TController::BackwardTraceRoute(double &fDistance, double &fDirection,
                                                   TTrack *Track, TEvent *&Event)
{ // szukanie sygnalizatora w kierunku przeciwnym jazdy (eventu odczytu komórki pamięci)
    TTrack *pTrackChVel = Track; // tor ze zmianą prędkości
    TTrack *pTrackFrom; // odcinek poprzedni, do znajdywania końca dróg
    double fDistChVel = -1; // odległość do toru ze zmianą prędkości
    double fCurrentDistance = pVehicle->RaTranslationGet(); // aktualna pozycja na torze
    double s = 0;
    if (fDirection > 0) // jeśli w kierunku Point2 toru
        fCurrentDistance = Track->Length() - fCurrentDistance;
    if (BackwardTrackBusy(Track))
    { // jak tor zajęty innym składem, to nie ma po co skanować
        fDistance = 0; // to na tym torze stoimy
        return NULL; // stop, skanowanie nie dało sensownych rezultatów
    }
    if ((Event = CheckTrackEventBackward(fDirection, Track)) != NULL)
    { // jeśli jest semafor na tym torze
        fDistance = 0; // to na tym torze stoimy
        return Track;
    }
    if ((Track->VelocityGet() == 0.0) || (Track->iDamageFlag & 128))
    { // jak prędkosć 0 albo uszkadza, to nie ma po co skanować
        fDistance = 0; // to na tym torze stoimy
        return NULL; // stop, skanowanie nie dało sensownych rezultatów
    }
    while (s < fDistance)
    {
        // Track->ScannedFlag=true; //do pokazywania przeskanowanych torów
        pTrackFrom = Track; // zapamiętanie aktualnego odcinka
        s += fCurrentDistance; // doliczenie kolejnego odcinka do przeskanowanej długości
        if (fDirection > 0)
        { // jeśli szukanie od Point1 w kierunku Point2
            if (Track->iNextDirection)
                fDirection = -fDirection;
            Track = Track->CurrentNext(); // może być NULL
        }
        else // if (fDirection<0)
        { // jeśli szukanie od Point2 w kierunku Point1
            if (!Track->iPrevDirection)
                fDirection = -fDirection;
            Track = Track->CurrentPrev(); // może być NULL
        }
        if (Track == pTrackFrom)
            Track = NULL; // koniec, tak jak dla torów
        if (Track ?
                (Track->VelocityGet() == 0.0) || (Track->iDamageFlag & 128) ||
                    BackwardTrackBusy(Track) :
                true)
        { // gdy dalej toru nie ma albo zerowa prędkość, albo uszkadza pojazd
            fDistance = s;
            return NULL; // zwraca NULL, że skanowanie nie dało sensownych rezultatów
        }
        fCurrentDistance = Track->Length();
        if ((Event = CheckTrackEventBackward(fDirection, Track)) != NULL)
        { // znaleziony tor z eventem
            fDistance = s;
            return Track;
        }
    }
    Event = NULL; // jak dojdzie tu, to nie ma semafora
    if (fDistChVel < 0)
    { // zwraca ostatni sprawdzony tor
        fDistance = s;
        return Track;
    }
    fDistance = fDistChVel; // odległość do zmiany prędkości
    return pTrackChVel; // i tor na którym się zmienia
}

// sprawdzanie zdarzeń semaforów i ograniczeń szlakowych
void TController::SetProximityVelocity(double dist, double vel, const vector3 *pos)
{ // Ra:przeslanie do AI prędkości
    /*
     //!!!! zastąpić prawidłową reakcją AI na SetProximityVelocity !!!!
     if (vel==0)
     {//jeśli zatrzymanie, to zmniejszamy dystans o 10m
      dist-=10.0;
     };
     if (dist<0.0) dist=0.0;
     if ((vel<0)?true:dist>0.1*(MoverParameters->Vel*MoverParameters->Vel-vel*vel)+50)
     {//jeśli jest dalej od umownej drogi hamowania
    */
    PutCommand("SetProximityVelocity", dist, vel, pos);
    /*
     }
     else
     {//jeśli jest zagrożenie, że przekroczy
      Mechanik->SetVelocity(floor(0.2*sqrt(dist)+vel),vel,stopError);
     }
     */
}

TCommandType TController::BackwardScan()
{ // sprawdzanie zdarzeń semaforów z tyłu pojazdu, zwraca komendę
    // dzięki temu będzie można stawać za wskazanym sygnalizatorem, a zwłaszcza jeśli będzie jazda
    // na kozioł
    // ograniczenia prędkości nie są wtedy istotne, również koniec toru jest do niczego nie
    // przydatny
    // zwraca true, jeśli należy odwrócić kierunek jazdy pojazdu
    if ((OrderList[OrderPos] & ~(Shunt | Connect)))
        return cm_Unknown; // skanowanie sygnałów tylko gdy jedzie w trybie manewrowym albo czeka na
                           // rozkazy
    vector3 sl;
    int startdir =
        -pVehicles[0]->DirectionGet(); // kierunek jazdy względem sprzęgów pojazdu na czele
    if (startdir == 0) // jeśli kabina i kierunek nie jest określony
        return cm_Unknown; // nie robimy nic
    double scandir =
        startdir * pVehicles[0]->RaDirectionGet(); // szukamy od pierwszej osi w wybranym kierunku
    if (scandir !=
        0.0) // skanowanie toru w poszukiwaniu eventów GetValues (PutValues nie są przydatne)
    { // Ra: przy wstecznym skanowaniu prędkość nie ma znaczenia
        // scanback=pVehicles[1]->NextDistance(fLength+1000.0); //odległość do następnego pojazdu,
        // 1000 gdy nic nie ma
        double scanmax = 1000; // 1000m do tyłu, żeby widział przeciwny koniec stacji
        double scandist = scanmax; // zmodyfikuje na rzeczywiście przeskanowane
        TEvent *e = NULL; // event potencjalnie od semafora
        // opcjonalnie może być skanowanie od "wskaźnika" z przodu, np. W5, Tm=Ms1, koniec toru
        TTrack *scantrack = BackwardTraceRoute(scandist, scandir, pVehicles[0]->RaTrackGet(),
                                               e); // wg drugiej osi w kierunku ruchu
        vector3 dir = startdir * pVehicles[0]->VectorFront(); // wektor w kierunku jazdy/szukania
        if (!scantrack) // jeśli wstecz wykryto koniec toru
            return cm_Unknown; // to raczej nic się nie da w takiej sytuacji zrobić
        else
        { // a jeśli są dalej tory
            double vmechmax; // prędkość ustawiona semaforem
            if (e)
            { // jeśli jest jakiś sygnał na widoku
#if LOGBACKSCAN
                AnsiString edir =
                    pVehicle->asName + " - " + AnsiString((scandir > 0) ? "Event2 " : "Event1 ");
#endif
                // najpierw sprawdzamy, czy semafor czy inny znak został przejechany
                vector3 pos = pVehicles[1]->RearPosition(); // pozycja tyłu
                vector3 sem; // wektor do sygnału
                if (e->Type == tp_GetValues)
                { // przesłać info o zbliżającym się semaforze
#if LOGBACKSCAN
                    edir += "(" + (e->Params[8].asGroundNode->asName) + "): ";
#endif
                    sl = e->PositionGet(); // położenie komórki pamięci
                    sem = sl - pos; // wektor do komórki pamięci od końca składu
                    // sem=e->Params[8].asGroundNode->pCenter-pos; //wektor do komórki pamięci
                    if (dir.x * sem.x + dir.z * sem.z < 0) // jeśli został minięty
                    // if ((mvOccupied->CategoryFlag&1)?(VelNext!=0.0):true) //dla pociągu wymagany
                    // sygnał zezwalający
                    { // iloczyn skalarny jest ujemny, gdy sygnał stoi z tyłu
#if LOGBACKSCAN
                        WriteLog(edir + "- ignored as not passed yet");
#endif
                        return cm_Unknown; // nic
                    }
                    vmechmax = e->ValueGet(1); // prędkość przy tym semaforze
                    // przeliczamy odległość od semafora - potrzebne by były współrzędne początku
                    // składu
                    // scandist=(pos-e->Params[8].asGroundNode->pCenter).Length()-0.5*mvOccupied->Dim.L-10;
                    // //10m luzu
                    scandist = sem.Length() - 2; // 2m luzu przy manewrach wystarczy
                    if (scandist < 0)
                        scandist = 0; // ujemnych nie ma po co wysyłać
                    bool move = false; // czy AI w trybie manewerowym ma dociągnąć pod S1
                    if (e->Command() == cm_SetVelocity)
                        if ((vmechmax == 0.0) ? (OrderCurrentGet() & (Shunt | Connect)) :
                                                (OrderCurrentGet() &
                                                 Connect)) // przy podczepianiu ignorować wyjazd?
                            move = true; // AI w trybie manewerowym ma dociągnąć pod S1
                        else
                        { //
                            if ((scandist > fMinProximityDist) ?
                                    (mvOccupied->Vel > 0.0) && (OrderCurrentGet() != Shunt) :
                                    false)
                            { // jeśli semafor jest daleko, a pojazd jedzie, to informujemy o
                              // zmianie prędkości
// jeśli jedzie manewrowo, musi dostać SetVelocity, żeby sie na pociągowy przełączył
// Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGBACKSCAN
                                // WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+"
                                // "+AnsiString(vmechmax));
                                WriteLog(edir);
#endif
                                // SetProximityVelocity(scandist,vmechmax,&sl);
                                return (vmechmax > 0) ? cm_SetVelocity : cm_Unknown;
                            }
                            else // ustawiamy prędkość tylko wtedy, gdy ma ruszyć, stanąć albo ma
                                 // stać
                            // if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //jeśli stoi lub ma
                            // stanąć/stać
                            { // semafor na tym torze albo lokomtywa stoi, a ma ruszyć, albo ma
                              // stanąć, albo nie ruszać
// stop trzeba powtarzać, bo inaczej zatrąbi i pojedzie sam
// PutCommand("SetVelocity",vmechmax,e->Params[9].asMemCell->Value2(),&sl,stopSem);
#if LOGBACKSCAN
                                WriteLog(edir + "SetVelocity " + AnsiString(vmechmax) + " " +
                                         AnsiString(e->Params[9].asMemCell->Value2()));
#endif
                                return (vmechmax > 0) ? cm_SetVelocity : cm_Unknown;
                            }
                        }
                    if (OrderCurrentGet() ? OrderCurrentGet() & (Shunt | Connect) :
                                            true) // w Wait_for_orders też widzi tarcze
                    { // reakcja AI w trybie manewrowym dodatkowo na sygnały manewrowe
                        if (move ? true : e->Command() == cm_ShuntVelocity)
                        { // jeśli powyżej było SetVelocity 0 0, to dociągamy pod S1
                            if ((scandist > fMinProximityDist) ?
                                    (mvOccupied->Vel > 0.0) || (vmechmax == 0.0) :
                                    false)
                            { // jeśli tarcza jest daleko, to:
                                //- jesli pojazd jedzie, to informujemy o zmianie prędkości
                                //- jeśli stoi, to z własnej inicjatywy może podjechać pod zamkniętą
                                //tarczę
                                if (mvOccupied->Vel > 0.0) // tylko jeśli jedzie
                                { // Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGBACKSCAN
                                    // WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+"
                                    // "+AnsiString(vmechmax));
                                    WriteLog(edir);
#endif
                                    // SetProximityVelocity(scandist,vmechmax,&sl);
                                    return (iDrivigFlags & moveTrackEnd) ?
                                               cm_ChangeDirection :
                                               cm_Unknown; // jeśli jedzie na W5 albo koniec toru,
                                                           // to można zmienić kierunek
                                }
                            }
                            else // ustawiamy prędkość tylko wtedy, gdy ma ruszyć, albo stanąć albo
                                 // ma stać pod tarczą
                            { // stop trzeba powtarzać, bo inaczej zatrąbi i pojedzie sam
                                // if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //jeśli jedzie
                                // lub ma stanąć/stać
                                { // nie dostanie komendy jeśli jedzie i ma jechać
// PutCommand("ShuntVelocity",vmechmax,e->Params[9].asMemCell->Value2(),&sl,stopSem);
#if LOGBACKSCAN
                                    WriteLog(edir + "ShuntVelocity " + AnsiString(vmechmax) + " " +
                                             AnsiString(e->ValueGet(2)));
#endif
                                    return (vmechmax > 0) ? cm_ShuntVelocity : cm_Unknown;
                                }
                            }
                            if ((vmechmax != 0.0) && (scandist < 100.0))
                            { // jeśli Tm w odległości do 100m podaje zezwolenie na jazdę, to od
                              // razu ją ignorujemy, aby móc szukać kolejnej
// eSignSkip=e; //wtedy uznajemy ignorowaną przy poszukiwaniu nowej
#if LOGBACKSCAN
                                WriteLog(edir + "- will be ignored due to Ms2");
#endif
                                return (vmechmax > 0) ? cm_ShuntVelocity : cm_Unknown;
                            }
                        } // if (move?...
                    } // if (OrderCurrentGet()==Shunt)
                    if (!e->bEnabled) // jeśli skanowany
                        if (e->StopCommand()) // a podłączona komórka ma komendę
                            return cm_Command; // to też się obrócić
                } // if (e->Type==tp_GetValues)
            } // if (e)
        } // if (scantrack)
    } // if (scandir!=0.0)
    return cm_Unknown; // nic
};

AnsiString TController::NextStop()
{ // informacja o następnym zatrzymaniu, wyświetlane pod [F1]
    if (asNextStop.Length() < 20)
        return ""; // nie zawiera nazwy stacji, gdy dojechał do końca
    // dodać godzinę odjazdu
    if (!TrainParams)
        return ""; // tu nie powinno nigdy wejść
    TMTableLine *t = TrainParams->TimeTable + TrainParams->StationIndex;
    if (t->Dh >= 0) // jeśli jest godzina odjazdu
        return asNextStop.SubString(20, 30) + AnsiString(" ") + AnsiString(int(t->Dh)) +
               AnsiString(":") + AnsiString(int(100 + t->Dm)).SubString(2, 2); // odjazd
    else if (t->Ah >= 0) // przyjazd
        return asNextStop.SubString(20, 30) + AnsiString(" (") + AnsiString(int(t->Ah)) +
               AnsiString(":") + AnsiString(int(100 + t->Am)).SubString(2, 2) +
               AnsiString(")"); // przyjazd
    return "";
};

//-----------koniec skanowania semaforow

void TController::TakeControl(bool yes)
{ // przejęcie kontroli przez AI albo oddanie
    if (AIControllFlag == yes)
        return; // już jest jak ma być
    if (yes) //żeby nie wykonywać dwa razy
    { // teraz AI prowadzi
        AIControllFlag = AIdriver;
        pVehicle->Controller = AIdriver;
        iDirection = 0; // kierunek jazdy trzeba dopiero zgadnąć
        // gdy zgaszone światła, flaga podjeżdżania pod semafory pozostaje bez zmiany
        if (OrderCurrentGet()) // jeśli coś robi
            PrepareEngine(); // niech sprawdzi stan silnika
        else // jeśli nic nie robi
            if (pVehicle->iLights[mvOccupied->CabNo < 0 ? 1 : 0] &
                21) // któreś ze świateł zapalone?
        { // od wersji 357 oczekujemy podania komend dla AI przez scenerię
            OrderNext(Prepare_engine);
            if (pVehicle->iLights[mvOccupied->CabNo < 0 ? 1 : 0] & 4) // górne światło zapalone
                OrderNext(Obey_train); // jazda pociągowa
            else
                OrderNext(Shunt); // jazda manewrowa
            if (mvOccupied->Vel >= 1.0) // jeśli jedzie (dla 0.1 ma stać)
                iDrivigFlags &= ~moveStopHere; // to ma nie czekać na sygnał, tylko jechać
            else
                iDrivigFlags |= moveStopHere; // a jak stoi, to niech czeka
        }
        /* od wersji 357 oczekujemy podania komend dla AI przez scenerię
          if (OrderCurrentGet())
          {if (OrderCurrentGet()<Shunt)
           {OrderNext(Prepare_engine);
            if (pVehicle->iLights[mvOccupied->CabNo<0?1:0]&4) //górne światło
             OrderNext(Obey_train); //jazda pociągowa
            else
             OrderNext(Shunt); //jazda manewrowa
           }
          }
          else //jeśli jest w stanie Wait_for_orders
           JumpToFirstOrder(); //uruchomienie?
          // czy dac ponizsze? to problematyczne
          //SetVelocity(pVehicle->GetVelocity(),-1); //utrzymanie dotychczasowej?
          if (pVehicle->GetVelocity()>0.0)
           SetVelocity(-1,-1); //AI ustali sobie odpowiednią prędkość
        */
        // Activation(); //przeniesie użytkownika w ostatnio wybranym kierunku
        CheckVehicles(); // ustawienie świateł
        TableClear(); // ponowne utworzenie tabelki, bo człowiek mógł pojechać niezgodnie z
                      // sygnałami
    }
    else
    { // a teraz użytkownik
        AIControllFlag = Humandriver;
        pVehicle->Controller = Humandriver;
    }
};

void TController::DirectionForward(bool forward)
{ // ustawienie jazdy do przodu dla true i do tyłu dla false (zależy od kabiny)
    while (mvControlling->MainCtrlPos) // samo zapętlenie DecSpeed() nie wystarcza
        DecSpeed(true); // wymuszenie zerowania nastawnika jazdy, inaczej się może zawiesić
    if (forward)
        while (mvOccupied->ActiveDir <= 0)
            mvOccupied->DirectionForward(); // do przodu w obecnej kabinie
    else
        while (mvOccupied->ActiveDir >= 0)
            mvOccupied->DirectionBackward(); // do tyłu w obecnej kabinie
    if (mvOccupied->EngineType == DieselEngine) // specjalnie dla SN61
        if (iDrivigFlags & moveActive) // jeśli był już odpalony
            if (mvControlling->RList[mvControlling->MainCtrlPos].Mn == 0)
                mvControlling->IncMainCtrl(1); //żeby nie zgasł
};

AnsiString TController::Relation() { // zwraca relację pociągu return TrainParams->ShowRelation(); };

AnsiString TController::TrainName() { // zwraca relację pociągu return TrainParams->TrainName; };

int TController::StationCount() { // zwraca ilość stacji (miejsc zatrzymania) return TrainParams->StationCount; };

int TController::StationIndex() { // zwraca indeks aktualnej stacji (miejsca zatrzymania) return TrainParams->StationIndex; };

bool TController::IsStop() { // informuje, czy jest zatrzymanie na najbliższej stacji return TrainParams->IsStop(); };

void TController::MoveTo(TDynamicObject *to)
{ // przesunięcie AI do innego pojazdu (przy zmianie kabiny)
    // mvOccupied->CabDeactivisation(); //wyłączenie kabiny w opuszczanym
    pVehicle->Mechanik = to->Mechanik; //żeby się zamieniły, jak jest jakieś drugie
    pVehicle = to;
    ControllingSet(); // utworzenie połączenia do sterowanego pojazdu
    pVehicle->Mechanik = this;
    // iDirection=0; //kierunek jazdy trzeba dopiero zgadnąć
};

void TController::ControllingSet()
{ // znajduje człon do sterowania w EZT będzie to silnikowy
    // problematyczne jest sterowanie z członu biernego, dlatego damy AI silnikowy
    // dzięki temu będzie wirtualna kabina w silnikowym, działająca w rozrządczym
    // w plikach FIZ zostały zgubione ujemne maski sprzęgów, stąd problemy z EZT
    mvOccupied = pVehicle->MoverParameters; // domyślny skrót do obiektu parametrów
    mvControlling = pVehicle->ControlledFind()->MoverParameters; // poszukiwanie członu sterowanego
};

AnsiString TController::TableText(int i)
{ // pozycja tabelki prędkości
    i = (iFirst + i) % iSpeedTableSize; // numer pozycji
    if (i != iLast) // w (iLast) znajduje się kolejny tor do przeskanowania, ale nie jest ona
                    // aktywną
        return sSpeedTable[i].TableText();
    return ""; // wskaźnik końca
};

int TController::CrossRoute(TTrack *tr)
{ // zwraca numer segmentu dla skrzyżowania (tr)
    // pożądany numer segmentu jest określany podczas skanowania drogi
    // droga powinna być określona sposobem przejazdu przez skrzyżowania albo współrzędnymi miejsca
    // docelowego
    for (int i = iFirst; i != iLast; i = (i + 1) % iSpeedTableSize)
    { // trzeba przejrzeć tabelę skanowania w poszukiwaniu (tr)
        // i jak się znajdzie, to zwrócić zapamiętany numer segmentu i kierunek przejazdu
        // (-6..-1,1..6)
        if ((sSpeedTable[i].iFlags & 3) == 3) // jeśli pozycja istotna (1) oraz odcinek (2)
            if (sSpeedTable[i].trTrack == tr) // jeśli pozycja odpowiadająca skrzyżowaniu (tr)
                return (sSpeedTable[i].iFlags >> 28); // najstarsze 4 bity jako liczba -8..7
    }
    return 0; // nic nie znaleziono?
};

void TController::RouteSwitch(int d)
{ // ustawienie kierunku jazdy z kabiny
    d &= 3;
    if (d)
        if (iRouteWanted != d)
        { // nowy kierunek
            iRouteWanted = d; // zapamiętanie
            if (mvOccupied->CategoryFlag & 2) // jeśli samochód
                for (int i = iFirst; i != iLast; i = (i + 1) % iSpeedTableSize)
                { // szukanie pierwszego skrzyżowania i resetowanie kierunku na nim
                    if ((sSpeedTable[i].iFlags & 3) ==
                        3) // jeśli pozycja istotna (1) oraz odcinek (2)
                        if ((sSpeedTable[i].iFlags & 32) == 0) // odcinek nie może być miniętym
                            if (sSpeedTable[i].trTrack->eType == tt_Cross) // jeśli skrzyżowanie
                            { // obcięcie tabelki skanowania przed skrzyżowaniem, aby ponownie
                              // wybrać drogę
                                iLast = i - 1; // ponowne skanowanie skrzyżowania (w zwrotnicach
                                               // jest iLast=i, ale tam jest prościej)
                                if (iLast < 0)
                                    iLast += iSpeedTableSize; // bo tabelka jest zapętlona
                                return;
                            }
                }
        }
};
AnsiString TController::OwnerName()
{
    return pVehicle ? pVehicle->MoverParameters->Name : AnsiString("none");
};
