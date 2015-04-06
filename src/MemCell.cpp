//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Driver.h"
#include "mctools.hpp"
#include "MemCell.h"
#include "Event.h"
#include "parser.h"

#include "Usefull.h"
#include "Globals.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

__fastcall TMemCell::TMemCell(vector3 *p)
{
    fValue1 = fValue2 = 0;
    szText = new char[256]; // musi być dla automatycznie tworzonych komórek dla odcinków
                            // izolowanych
    vPosition =
        p ? *p : vector3(0, 0, 0); // ustawienie współrzędnych, bo do TGroundNode nie ma dostępu
    bCommand = false; // komenda wysłana
    OnSent = NULL;
}

__fastcall TMemCell::~TMemCell() { SafeDeleteArray(szText); }

void TMemCell::Init() {}

void TMemCell::UpdateValues(char *szNewText, double fNewValue1, double fNewValue2,
                                       int CheckMask)
{
    if (CheckMask & update_memadd)
    { // dodawanie wartości
        if (TestFlag(CheckMask, update_memstring))
            strcat(szText, szNewText);
        if (TestFlag(CheckMask, update_memval1))
            fValue1 += fNewValue1;
        if (TestFlag(CheckMask, update_memval2))
            fValue2 += fNewValue2;
    }
    else
    {
        if (TestFlag(CheckMask, update_memstring))
            strcpy(szText, szNewText);
        if (TestFlag(CheckMask, update_memval1))
            fValue1 = fNewValue1;
        if (TestFlag(CheckMask, update_memval2))
            fValue2 = fNewValue2;
    }
    if (TestFlag(CheckMask, update_memstring))
        CommandCheck(); // jeśli zmieniony tekst, próbujemy rozpoznać komendę
}

TCommandType TMemCell::CommandCheck()
{ // rozpoznanie komendy
    if (strcmp(szText, "SetVelocity") == 0) // najpopularniejsze
    {
        eCommand = cm_SetVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else if (strcmp(szText, "ShuntVelocity") == 0) // w tarczach manewrowych
    {
        eCommand = cm_ShuntVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else if (strcmp(szText, "Change_direction") == 0) // zdarza się
    {
        eCommand = cm_ChangeDirection;
        bCommand = true; // do wysłania
    }
    else if (strcmp(szText, "OutsideStation") == 0) // zdarza się
    {
        eCommand = cm_OutsideStation;
        bCommand = false; // tego nie powinno być w komórce
    }
    else if (strncmp(szText, "PassengerStopPoint:", 19) == 0) // porównanie początków
    {
        eCommand = cm_PassengerStopPoint;
        bCommand = false; // tego nie powinno być w komórce
    }
    else if (strcmp(szText, "SetProximityVelocity") == 0) // nie powinno tego być
    {
        eCommand = cm_SetProximityVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else
    {
        eCommand = cm_Unknown; // ciąg nierozpoznany (nie jest komendą)
        bCommand = true; // do wysłania
    }
    return eCommand;
}

bool TMemCell::Load(cParser *parser)
{
    std::string token;
    parser->getTokens(1, false); // case sensitive
    *parser >> token;
    SafeDeleteArray(szText);
    szText = new char[256]; // musi być bufor do łączenia tekstów
    strcpy(szText, token.c_str());
    parser->getTokens();
    *parser >> fValue1;
    parser->getTokens();
    *parser >> fValue2;
    parser->getTokens();
    *parser >> token;
    if (token.compare("none") != 0) // gdy różne od "none"
        asTrackName = AnsiString(token.c_str()); // sprawdzane przez IsEmpty()
    parser->getTokens();
    *parser >> token;
    if (token.compare("endmemcell") != 0)
        Error("endmemcell statement missing");
    CommandCheck();
    return true;
}

void TMemCell::PutCommand(TController *Mech, vector3 *Loc)
{ // wysłanie zawartości komórki do AI
    if (Mech)
        Mech->PutCommand(szText, fValue1, fValue2, Loc);
}

bool TMemCell::Compare(char *szTestText, double fTestValue1, double fTestValue2,
                                  int CheckMask)
{ // porównanie zawartości komórki pamięci z podanymi wartościami
    if (TestFlag(CheckMask, conditional_memstring))
    { // porównać teksty
        char *pos = StrPos(szTestText, "*"); // zwraca wskaźnik na pozycję albo NULL
        if (pos)
        { // porównanie fragmentu łańcucha
            int i = pos - szTestText; // ilość porównywanych znaków
            if (i) // jeśli nie jest pierwszym znakiem
                if (AnsiString(szTestText, i) != AnsiString(szText, i))
                    return false; // początki o długości (i) są różne
        }
        else if (AnsiString(szTestText) != AnsiString(szText))
            return false; //łąńcuchy są różne
    }
    // tekst zgodny, porównać resztę
    return ((!TestFlag(CheckMask, conditional_memval1) || (fValue1 == fTestValue1)) &&
            (!TestFlag(CheckMask, conditional_memval2) || (fValue2 == fTestValue2)));
};

bool TMemCell::Render() { return true; }

bool TMemCell::IsVelocity()
{ // sprawdzenie, czy event odczytu tej komórki ma być do skanowania, czy do kolejkowania
    if (eCommand == cm_SetVelocity)
        return true;
    if (eCommand == cm_ShuntVelocity)
        return true;
    return (eCommand == cm_SetProximityVelocity);
};

void TMemCell::StopCommandSent()
{ //
    if (!bCommand)
        return;
    bCommand = false;
    if (OnSent) // jeśli jest event
        Global::AddToQuery(OnSent, NULL);
};

void TMemCell::AssignEvents(TEvent *e) { // powiązanie eventu OnSent = e; };
