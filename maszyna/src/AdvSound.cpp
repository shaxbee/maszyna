//---------------------------------------------------------------------------

#pragma hdrstop
#include "commons.h"
#include "commons_usr.h"

//---------------------------------------------------------------------------


TAdvancedSound::TAdvancedSound() {
  //    SoundStart=SoundCommencing=SoundShut= NULL;
  State = ss_Off;
  fTime = 0;
  fStartLength = 0;
  fShutLength = 0;
}

TAdvancedSound::~TAdvancedSound() { // Ra: stopowanie się sypie
  // SoundStart.Stop();
  // SoundCommencing.Stop();
  // SoundShut.Stop();
}

void TAdvancedSound::Free() {}

void TAdvancedSound::Init(char *NameOn, char *Name, char *NameOff,
                          double DistanceAttenuation, vector3 pPosition) {
  SoundStart.Init(NameOn, DistanceAttenuation, pPosition.x, pPosition.y,
                  pPosition.z, true);
  SoundCommencing.Init(Name, DistanceAttenuation, pPosition.x, pPosition.y,
                       pPosition.z, true);
  SoundShut.Init(NameOff, DistanceAttenuation, pPosition.x, pPosition.y,
                 pPosition.z, true);
  fStartLength = SoundStart.GetWaveTime();
  fShutLength = SoundShut.GetWaveTime();
  SoundStart.AM = 1.0;
  SoundStart.AA = 0.0;
  SoundStart.FM = 1.0;
  SoundStart.FA = 0.0;
  SoundCommencing.AM = 1.0;
  SoundCommencing.AA = 0.0;
  SoundCommencing.FM = 1.0;
  SoundCommencing.FA = 0.0;
  defAM = 1.0;
  defFM = 1.0;
  SoundShut.AM = 1.0;
  SoundShut.AA = 0.0;
  SoundShut.FM = 1.0;
  SoundShut.FA = 0.0;
}

void TAdvancedSound::Load(cParser Parser, vector3 pPosition) // !!! TUTAJ W PARAMETRZE BYLO TQueryParserComp, ZAMIENILEM NA cParser !!!
{                                                            // CZY TO BEDZIE DZIALAC?

  std::string tokenS;
  double tokenD;

  Parser.getTokens();
  Parser >> tokenS;
  std::string NameOn = tokenS;

  Parser.getTokens();
  Parser >> tokenS;
  std::string Name = tokenS;

  Parser.getTokens();
  Parser >> tokenS;
  std::string NameOff = tokenS;

  Parser.getTokens();
  Parser >> tokenD;
  double DistanceAttenuation = tokenD;

  Init(stdstrtochar(NameOn), stdstrtochar(Name), stdstrtochar(NameOff), DistanceAttenuation, pPosition);
}

void TAdvancedSound::TurnOn(bool ListenerInside, vector3 NewPosition) {
  // hunter-311211: nie trzeba czekac na ponowne odtworzenie dzwieku, az sie
  // wylaczy
  if ((State == ss_Off || State == ss_ShuttingDown) && (SoundStart.AM > 0)) {
    SoundStart.ResetPosition();
    SoundCommencing.ResetPosition();
    SoundStart.Play(1, 0, ListenerInside, NewPosition);
    //        SoundStart->SetVolume(-10000);
    State = ss_Starting;
    fTime = 0;
  }
}

void TAdvancedSound::TurnOff(bool ListenerInside, vector3 NewPosition) {
  if ((State == ss_Commencing || State == ss_Starting) && (SoundShut.AM > 0)) {
    SoundStart.Stop();
    SoundCommencing.Stop();
    SoundShut.ResetPosition();
    SoundShut.Play(1, 0, ListenerInside, NewPosition);
    State = ss_ShuttingDown;
    fTime = fShutLength;
    //        SoundShut->SetVolume(0);
  }
}

void TAdvancedSound::Update(bool ListenerInside, vector3 NewPosition) {
  if ((State == ss_Commencing) && (SoundCommencing.AM > 0)) {
    //        SoundCommencing->SetFrequency();
    SoundShut.Stop(); // hunter-311211
    SoundCommencing.Play(1, DSBPLAY_LOOPING, ListenerInside, NewPosition);
  } else if (State == ss_Starting) {
    fTime += Timer::GetDeltaTime();
    //        SoundStart->SetVolume(-1000*(4-fTime)/4);
    if (fTime >= fStartLength) {
      State = ss_Commencing;
      SoundCommencing.ResetPosition();
      SoundCommencing.Play(1, DSBPLAY_LOOPING, ListenerInside, NewPosition);
      SoundStart.Stop();

    } else
      SoundStart.Play(1, 0, ListenerInside, NewPosition);
  } else if (State == ss_ShuttingDown) {
    fTime -= Timer::GetDeltaTime();
    //        SoundShut->SetVolume(-1000*(4-fTime)/4);
    if (fTime <= 0) {
      State = ss_Off;
      SoundShut.Stop();
    } else
      SoundShut.Play(1, 0, ListenerInside, NewPosition);
  }
}

void TAdvancedSound::UpdateAF(
    double A, double F, bool ListenerInside,
    vector3 NewPosition) { // update, ale z amplituda i czestotliwoscia
  if ((State == ss_Commencing) && (SoundCommencing.AM > 0)) {
    SoundShut.Stop(); // hunter-311211
    SoundCommencing.Play(long(A), DSBPLAY_LOOPING, ListenerInside, NewPosition);
  } else if (State == ss_Starting) {
    fTime += Timer::GetDeltaTime();
    //        SoundStart->SetVolume(-1000*(4-fTime)/4);
    if (fTime >= fStartLength) {
      State = ss_Commencing;
      SoundCommencing.ResetPosition();
      SoundCommencing.Play(long(A), DSBPLAY_LOOPING, ListenerInside, NewPosition);
      SoundStart.Stop();
    } else
      SoundStart.Play(long(A), 0, ListenerInside, NewPosition);
  } else if (State == ss_ShuttingDown) {
    fTime -= Timer::GetDeltaTime();
    //        SoundShut->SetVolume(-1000*(4-fTime)/4);
    if (fTime <= 0) {
      State = ss_Off;
      SoundShut.Stop();
    } else
      SoundShut.Play(long(A), 0, ListenerInside, NewPosition);
  }
  SoundCommencing.AdjFreq(F, Timer::GetDeltaTime());
}

void TAdvancedSound::CopyIfEmpty(
    TAdvancedSound &
        s) { // skopiowanie, gdyby był potrzebny, a nie został wczytany
  if ((fStartLength > 0.0) || (fShutLength > 0.0))
    return; // coś jest
  SoundStart = s.SoundStart;
  SoundCommencing = s.SoundCommencing;
  SoundShut = s.SoundShut;
  State = s.State;
  fStartLength = s.fStartLength;
  fShutLength = s.fShutLength;
  defAM = s.defAM;
  defFM = s.defFM;
};
