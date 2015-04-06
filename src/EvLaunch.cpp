//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "system.hpp"
#pragma hdrstop

#include "mtable.hpp"
#include "Timer.h"
#include "Globals.h"
#include "EvLaunch.h"
#include "Event.h"

#include "Usefull.h"
#include "MemCell.h"
#include "parser.h"
#include "Console.h"

//---------------------------------------------------------------------------

__fastcall TEventLauncher::TEventLauncher()
{ // ustawienie początkowych wartości dla wszystkich zmiennych
    iKey = 0;
    DeltaTime = -1;
    UpdatedTime = 0;
    fVal1 = fVal2 = 0;
    szText = NULL;
    iHour = iMinute = -1; // takiego czasu nigdy nie będzie
    dRadius = 0;
    Event1 = Event2 = NULL;
    MemCell = NULL;
    iCheckMask = 0;
}

__fastcall TEventLauncher::~TEventLauncher() { SafeDeleteArray(szText); }

void TEventLauncher::Init() {}

bool TEventLauncher::Load(cParser *parser)
{ // wczytanie wyzwalacza zdarzeń
    AnsiString str;
    std::string token;
    parser->getTokens();
    *parser >> dRadius; // promień działania
    if (dRadius > 0.0)
        dRadius *= dRadius; // do kwadratu, pod warunkiem, że nie jest ujemne
    parser->getTokens(); // klawisz sterujący
    *parser >> token;
    str = AnsiString(token.c_str());
    if (str != "none")
    {
        if (str.Length() == 1)
            iKey = VkKeyScan(str[1]); // jeden znak jest konwertowany na kod klawisza
        else
            iKey = str.ToIntDef(0); // a jak więcej, to jakby numer klawisza jest
    }
    parser->getTokens();
    *parser >> DeltaTime;
    if (DeltaTime < 0)
        DeltaTime = -DeltaTime; // dla ujemnego zmieniamy na dodatni
    else if (DeltaTime > 0)
    { // wartość dodatnia oznacza wyzwalanie o określonej godzinie
        iMinute = int(DeltaTime) % 100; // minuty są najmłodszymi cyframi dziesietnymi
        iHour = int(DeltaTime - iMinute) / 100; // godzina to setki
        DeltaTime = 0; // bez powtórzeń
        WriteLog("EventLauncher at " + IntToStr(iHour) + ":" +
                 IntToStr(iMinute)); // wyświetlenie czasu
    }
    parser->getTokens();
    *parser >> token;
    asEvent1Name = AnsiString(token.c_str()); // pierwszy event
    parser->getTokens();
    *parser >> token;
    asEvent2Name = AnsiString(token.c_str()); // drugi event
    if ((asEvent2Name == "end") || (asEvent2Name == "condition"))
    { // drugiego eventu może nie być, bo są z tym problemy, ale ciii...
        str = asEvent2Name; // rozpoznane słowo idzie do dalszego przetwarzania
        asEvent2Name = "none"; // a drugiego eventu nie ma
    }
    else
    { // gdy są dwa eventy
        parser->getTokens();
        *parser >> token;
        str = AnsiString(token.c_str());
    }
    if (str == AnsiString("condition"))
    { // obsługa wyzwalania warunkowego
        parser->getTokens();
        *parser >> token;
        asMemCellName = AnsiString(token.c_str());
        parser->getTokens();
        *parser >> token;
        SafeDeleteArray(szText);
        szText = new char[256];
        strcpy(szText, token.c_str());
        if (token.compare("*") != 0) //*=nie brać command pod uwagę
            iCheckMask |= conditional_memstring;
        parser->getTokens();
        *parser >> token;
        if (token.compare("*") != 0) //*=nie brać wartości 1. pod uwagę
        {
            iCheckMask |= conditional_memval1;
            str = AnsiString(token.c_str());
            fVal1 = str.ToDouble();
        }
        else
            fVal1 = 0;
        parser->getTokens();
        *parser >> token;
        if (token.compare("*") != 0) //*=nie brać wartości 2. pod uwagę
        {
            iCheckMask |= conditional_memval2;
            str = AnsiString(token.c_str());
            fVal2 = str.ToDouble();
        }
        else
            fVal2 = 0;
        parser->getTokens(); // słowo zamykające
        *parser >> token;
    }
    return true;
};

bool TEventLauncher::Render()
{ //"renderowanie" wyzwalacza
    bool bCond = false;
    if (iKey != 0)
    {
        if (Global::bActive) // tylko jeśli okno jest aktywne
            bCond = (Console::Pressed(iKey)); // czy klawisz wciśnięty
    }
    if (DeltaTime > 0)
    {
        if (UpdatedTime > DeltaTime)
        {
            UpdatedTime = 0; // naliczanie od nowa
            bCond = true;
        }
        else
            UpdatedTime += Timer::GetDeltaTime(); // aktualizacja naliczania czasu
    }
    else
    { // jeśli nie cykliczny, to sprawdzić czas
        if (GlobalTime->hh == iHour)
        {
            if (GlobalTime->mm == iMinute)
            { // zgodność czasu uruchomienia
                if (UpdatedTime < 10)
                {
                    UpdatedTime = 20; // czas do kolejnego wyzwolenia?
                    bCond = true;
                }
            }
        }
        else
            UpdatedTime = 1;
    }
    if (bCond) // jeśli spełniony został warunek
    {
        if ((iCheckMask != 0) && MemCell) // sprawdzanie warunku na komórce pamięci
            bCond = MemCell->Compare(szText, fVal1, fVal2, iCheckMask);
    }
    return bCond; // sprawdzanie dRadius w Ground.cpp
}

bool TEventLauncher::IsGlobal()
{ // sprawdzenie, czy jest globalnym wyzwalaczem czasu
    if (DeltaTime == 0)
        if (iHour >= 0)
            if (iMinute >= 0)
                if (dRadius < 0.0) // bez ograniczenia zasięgu
                    return true;
    return false;
};
//---------------------------------------------------------------------------

#pragma package(smart_init)
