//---------------------------------------------------------------------------


#pragma hdrstop

#include "commons.h"
#include "commons_usr.h"

namespace Timer {

double DeltaTime, DeltaRenderTime;
double fFPS = 0.0f;
double fLastTime = 0.0f;
DWORD dwFrames = 0L;
double fSimulationTime = 0;
double fSoundTimer = 0;
double fSinceStart = 0;

double GetTime() { return fSimulationTime; }

double GetDeltaTime() { // czas symulacji (stoi gdy pauza)
  return DeltaTime;
}

double GetDeltaRenderTime() { // czas renderowania (do poruszania się)
  return DeltaRenderTime;
}

double GetfSinceStart() { return fSinceStart; }

void SetDeltaTime(double t) { DeltaTime = t; }

double GetSimulationTime() { return fSimulationTime; }

void SetSimulationTime(double t) { fSimulationTime = t; }

bool GetSoundTimer() { // Ra: być może, by dźwięki nie modyfikowały się zbyt
                       // często, po 0.1s zeruje się ten licznik
  return (fSoundTimer == 0.0f);
}

double GetFPS() { return fFPS; }

void ResetTimers() {
  // double CurrentTime=
  GetTickCount();
  DeltaTime = 0.1;
  DeltaRenderTime = 0;
  fSoundTimer = 0;
};

LONGLONG fr, count, oldCount;
// LARGE_INTEGER fr,count;
void UpdateTimers(bool pause) {
  QueryPerformanceFrequency((LARGE_INTEGER *)&fr);
  QueryPerformanceCounter((LARGE_INTEGER *)&count);
  DeltaRenderTime = double(count - oldCount) / double(fr);
  if (!pause) {
    DeltaTime = Global::fTimeSpeed * DeltaRenderTime;
    fSoundTimer += DeltaTime;
    if (fSoundTimer > 0.1)
      fSoundTimer = 0;
    /*
      double CurrentTime= double(count)/double(fr);//GetTickCount();
      DeltaTime= (CurrentTime-OldTime);
      OldTime= CurrentTime;
    */
    if (DeltaTime > 1)
      DeltaTime = 1;
  } else
    DeltaTime = 0; // wszystko stoi, bo czas nie płynie
  oldCount = count;

  // Keep track of the time lapse and frame count
  double fTime = GetTickCount() * 0.001f; // Get current time in seconds
  ++dwFrames; // licznik ramek
  // update the frame rate once per second
  if (fTime - fLastTime > 1.0f) {
    fFPS = dwFrames / (fTime - fLastTime);
    fLastTime = fTime;
    dwFrames = 0L;
  }
  fSimulationTime += DeltaTime;
};
};

//---------------------------------------------------------------------------

