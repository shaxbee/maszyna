//---------------------------------------------------------------------------

#ifndef FadeSoundH
#define FadeSoundH
#include "commons.h"
#include "commons_usr.h"
#include "AdvSound.h"

class TFadeSound
{
    PSound Sound;
    double fFade;
    double dt,fTime;
    TSoundState State;
public:
    __fastcall TFadeSound();
    __fastcall ~TFadeSound();
    void __fastcall Init(char *Name, float fNewFade);
    void __fastcall TurnOn();
    void __fastcall TurnOff();
    bool __fastcall Playing() {return (State==ss_Commencing || State==ss_Starting); };
    void __fastcall Free();
    void __fastcall Update();
    void __fastcall Volume(long vol);

};

//---------------------------------------------------------------------------
#endif
