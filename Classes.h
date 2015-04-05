//---------------------------------------------------------------------------

#ifndef ClassesH
#define ClassesH
//---------------------------------------------------------------------------
// Ra: zestaw klas do robienia wskaźników, aby uporządkować nagłówki
//---------------------------------------------------------------------------
class TTrack; // odcinek trajektorii
class TEvent;
class TTrain; // pojazd sterowany
class TDynamicObject; // pojazd w scenerii
class TGroundNode; // statyczny obiekt scenerii
class TAnimModel; // opakowanie egzemplarz modelu
class TAnimContainer; // fragment opakowania egzemplarza modelu
// class TModel3d; //siatka modelu wspólna dla egzemplarzy
class TSubModel; // fragment modelu (tu do wyświetlania terenu)
class TMemCell; // komórka pamięci
class cParser;
class TRealSound; // dźwięk ze współrzędnymi XYZ
class TTextSound; // dźwięk ze stenogramem
class TEventLauncher;
class TTraction; // drut
class TTractionPowerSource; // zasilanie drutów

class TMoverParameters;
namespace _mover
{
class TLocation;
class TRotation;
};

namespace Mtable
{
class TTrainParameters; // rozkład jazdy
};

class TController; // obiekt sterujący pociągiem (AI)
class TNames; // obiekt sortujący nazwy

typedef enum
{ // binarne odpowiedniki komend w komórce pamięci
    cm_Unknown, // ciąg nierozpoznany (nie jest komendą)
    cm_Ready, // W4 zezwala na odjazd, ale semafor może zatrzymać
    cm_SetVelocity,
    cm_ShuntVelocity,
    cm_SetProximityVelocity,
    cm_ChangeDirection,
    cm_PassengerStopPoint,
    cm_OutsideStation,
    cm_Shunt,
    cm_Command // komenda pobierana z komórki
} TCommandType;

#endif
