//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#pragma hdrstop
#include "commons.h"
#include "commons_usr.h"
//#include "math.h"

//#include "McZapkie\mctools.hpp"

using namespace std;

TRealSound::TRealSound() {
  pSound = NULL;
  dSoundAtt = -1;
  AM = 0.0;
  AA = 0.0;
  FM = 0.0;
  FA = 0.0;
  vSoundPosition.x = 0;
  vSoundPosition.y = 0;
  vSoundPosition.z = 0;
  fDistance = fPreviousDistance = 0.0;
  fFrequency = 22050.0; // częstotliwość samplowania pliku
  iDoppler = 0; // normlanie jest załączony; !=0 - modyfikacje
  bLoopPlay = false; // dźwięk wyłączony
}

TRealSound::~TRealSound() {
  // if (this) if (pSound) pSound->Stop();
}

void TRealSound::Free() {}

void TRealSound::Init(char *SoundName, double DistanceAttenuation, double X, double Y, double Z, bool Dynamic, bool freqmod, 
	                                                                                                           double rmin)
{
  // Nazwa=SoundName; //to tak raczej nie zadziała, (SoundName) jest tymczasowe
  pSound = TSoundsManager::GetFromName(SoundName, Dynamic, &fFrequency);
  if (pSound) {
    if (freqmod)
      if (fFrequency != 22050.0) { // dla modulowanych nie może być zmiany
                                   // mnożnika, bo częstotliwość w nagłówku byłą
                                   // ignorowana, a mogła być inna niż 22050
        fFrequency = 22050.0;
        WriteLogSS("Bad sound: " + chartostdstr(SoundName) + ", as modulated, should have 22.05kHz in header", "ERROR");
      }
    AM = 1.0;
    pSound->SetVolume(DSBVOLUME_MIN);
  } else { // nie ma dźwięku, to jest wysyp
    AM = 0;
	WriteLogSS("Missed sound: " + chartostdstr(SoundName), "ERROR");
  }
  if (DistanceAttenuation > 0.0) {
    dSoundAtt = DistanceAttenuation * DistanceAttenuation;
    vSoundPosition.x = X;
    vSoundPosition.y = Y;
    vSoundPosition.z = Z;
    if (rmin < 0)
      iDoppler = 1; // wyłączenie efektu Dopplera, np. dla dźwięku ptaków
  } else
    dSoundAtt = -1;
};

double TRealSound::ListenerDistance(vector3 ListenerPosition) {
  if (dSoundAtt == -1) {
    return 0.0;
  } else {
    return SquareMagnitude(ListenerPosition - vSoundPosition);
  }
}

void TRealSound::Play(long Volume, int Looping, bool ListenerInside,  vector3 NewPosition) // Volume BYLO double
{
  if (!pSound)
    return;
  long int vol;
  double dS;
  // double Distance;
  DWORD stat;
  if ((Global::bSoundEnabled) && (AM != 0)) {
    if (Volume > 1.0)
      Volume = 1;
    fPreviousDistance = fDistance;
    fDistance = 0.0; //??
    if (dSoundAtt > 0.0) {
      vSoundPosition = NewPosition;
      dS = dSoundAtt; //*dSoundAtt; //bo odleglosc podawana w kwadracie
      fDistance = ListenerDistance(Global::pCameraPosition);
      if (ListenerInside) // osłabianie dźwięków z odległością
        Volume = Volume * dS / (dS + fDistance);
      else
        Volume = Volume * dS /
                 (dS + 2 * fDistance); // podwójne dla ListenerInside=false
    }
    if (iDoppler) //
    { // Ra 2014-07: efekt Dopplera nie zawsze jest wskazany
      // if (FreeFlyModeFlag) //gdy swobodne latanie - nie sprawdza się to
      fPreviousDistance = fDistance; // to efektu Dopplera nie będzie
    }
    if (Looping) // dźwięk zapętlony można wyłączyć i zostanie włączony w miarę
                 // potrzeby
      bLoopPlay = true; // dźwięk wyłączony
    // McZapkie-010302 - babranie tylko z niezbyt odleglymi dźwiękami
    if ((dSoundAtt == -1) || (fDistance < 20.0 * dS)) {
      //   vol=2*Volume+1;
      //   if (vol<1) vol=1;
      //   vol=10000*(log(vol)-1);
      //   vol=10000*(vol-1);
      // int glos=1;
      // Volume=Volume*glos; //Ra: whatta hella is this
      if (Volume < 0.0)
        Volume = 0.0;
      vol = -5000.0 + 5000.0 * Volume;
      if (vol >= 0)
        vol = -1;
      if (Timer::GetSoundTimer() || !Looping) // Ra: po co to jest?
        pSound->SetVolume(vol); // Attenuation, in hundredths of a decibel (dB).
      pSound->GetStatus(&stat);
      if (!(stat & DSBSTATUS_PLAYING))
        pSound->Play(0, 0, Looping);
    } else // wylacz dzwiek bo daleko
    { // Ra 2014-09: oddalanie się nie może być powodem do wyłączenie dźwięku
      /*
      // Ra: stara wersja, ale podobno lepsza
         pSound->GetStatus(&stat);
         if (bLoopPlay) //jeśli zapętlony, to zostanie ponownie włączony, o ile
      znajdzie się bliżej
          if (stat&DSBSTATUS_PLAYING)
           pSound->Stop();
      // Ra: wyłączyłem, bo podobno jest gorzej niż wcześniej
         //ZiomalCl: dźwięk po wyłączeniu sam się nie włączy, gdy wrócimy w
      rejon odtwarzania
         pSound->SetVolume(DSBVOLUME_MIN); //dlatego lepiej go wyciszyć na czas
      oddalenia się
         pSound->GetStatus(&stat);
         if (!(stat&DSBSTATUS_PLAYING))
          pSound->Play(0,0,Looping); //ZiomalCl: włączenie odtwarzania rownież i
      tu, gdyż jesli uruchamiamy dźwięk poza promieniem, nie uruchomi się on w
      ogóle
      */
    }
  }
};

void TRealSound::Start(){// włączenie dźwięku

};

void TRealSound::Stop() {
  DWORD stat;
  if (pSound)
    if ((Global::bSoundEnabled) && (AM != 0)) {
      bLoopPlay = false; // dźwięk wyłączony
      pSound->GetStatus(&stat);
      if (stat & DSBSTATUS_PLAYING)
        pSound->Stop();
    }
};

void TRealSound::AdjFreq(double Freq,
                         double dt) // McZapkie TODO: dorobic tu efekt Dopplera
// Freq moze byc liczba dodatnia mniejsza od 1 lub wieksza od 1
{
  double df, Vlist;
  if ((Global::bSoundEnabled) && (AM != 0)) {
    if (dt > 0)
    // efekt Dopplera
    {
      Vlist = (sqrt(fPreviousDistance) - sqrt(fDistance)) / dt;
      df = Freq * (1 + Vlist / 299.8);
    } else
      df = Freq;
    if (Timer::GetSoundTimer()) {
      df = fFrequency * df; // TODO - brac czestotliwosc probkowania z wav
      pSound->SetFrequency(
          (df < DSBFREQUENCY_MIN
               ? DSBFREQUENCY_MIN : (df > DSBFREQUENCY_MAX ? DSBFREQUENCY_MAX : df)));
    }
  }
}

double TRealSound::GetWaveTime() // McZapkie: na razie tylko dla 22KHz/8bps
{ // używana do pomiaru czasu dla dźwięków z początkiem i końcem
  if (!pSound)
    return 0.0;
  double WaveTime;
  DSBCAPS caps;
  caps.dwSize = sizeof(caps);
  pSound->GetCaps(&caps);
  WaveTime = caps.dwBufferBytes;
  return WaveTime / fFrequency; //(pSound->);  // wielkosc w bajtach przez
                                //czestotliwosc probkowania
}

void TRealSound::SetPan(int Pan) { pSound->SetPan(Pan); }

int TRealSound::GetStatus() {
  DWORD stat;
  if ((Global::bSoundEnabled) && (AM != 0)) {
    pSound->GetStatus(&stat);
    return stat;
  } else
    return 0;
}

void TRealSound::ResetPosition() {
  if (pSound) // Ra: znowu jakiś badziew!
    pSound->SetCurrentPosition(0);
}

template<typename T>
std::string removeSubstrs(basic_string<T>& s,
	const basic_string<T>& p) {
	basic_string<T>::size_type n = p.length();

	for (basic_string<T>::size_type i = s.find(p);
		i != basic_string<T>::npos;
		i = s.find(p))
		s.erase(i, n);
	return s;
}

void TTextSound::Init(char *SoundName, double SoundAttenuation, double X, double Y, double Z, bool Dynamic, bool freqmod, double rmin)
{ // dodatkowo doczytuje plik tekstowy
  TRealSound::Init(SoundName, SoundAttenuation, X, Y, Z, Dynamic, freqmod,rmin);

  fTime = GetWaveTime();
  std::string txt = SoundName;

  std::string fext = ".wav";

 txt =  removeSubstrs(txt, fext);

// struct stat info;
// int ret = -1;



 // txt.Delete(txt.Length() - 3, 4); // obcięcie rozszerzenia
  for (int i = txt.length(); i > 0; --i)
    if (txt[i] == '/')
      txt[i] = '\\'; // bo nie rozumi
  txt += "-" + Global::asLang + ".txt"; // już może być w różnych językach

  //ret = stat(txt.c_str(), &info);

  if (!FileExists(txt))
    txt = "sounds\\" + txt; //ścieżka może nie być podana

  // TODO: komentuje do przetlumaczenia pozniej
  /*
  if (FileExists(txt)) { // wczytanie
    TFileStream *ts = new TFileStream(txt, fmOpenRead);
    asText = AnsiString::StringOfChar(' ', ts->Size);
    ts->Read(asText.c_str(), ts->Size);
    delete ts;
  }
  */

};
void TTextSound::Play(long Volume, int Looping, bool ListenerInside,  vector3 NewPosition) // Volume BYLO double
{
	// TODO: Q komentuje 
	/* 
  if (!asText.IsEmpty()) { // jeśli ma powiązany tekst
    DWORD stat;
    pSound->GetStatus(&stat);
    if (!(stat & DSBSTATUS_PLAYING)) // jeśli nie jest aktualnie odgrywany
    {
      int i;
      AnsiString t = asText;
      do { // na razie zrobione jakkolwiek, docelowo przenieść teksty do tablicy
           // nazw
        i = t.Pos("\r"); // znak nowej linii
        if (!i)
          Global::tranTexts.Add(t.c_str(), fTime, true);
        else {
          Global::tranTexts.Add(t.SubString(1, i - 1).c_str(), fTime, true);
          t.Delete(1, i);
          while (t.IsEmpty() ? false : (unsigned char)(t[1]) < 33)
            t.Delete(1, 1);
        }
      } while (i > 0);
    }
  }
  */
  TRealSound::Play(Volume, Looping, ListenerInside, NewPosition);
};

//---------------------------------------------------------------------------

