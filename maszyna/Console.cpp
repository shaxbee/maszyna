#pragma hdrstop

#include "commons.h"
#include "commons_usr.h"
//#include "PoKeys55.h"
//#include "LPT.h"

//---------------------------------------------------------------------------

//Ra: klasa statyczna gromadz¹ca sygna³y steruj¹ce oraz informacje zwrotne
//Ra: stan wejœcia zmieniany klawiatur¹ albo dedykowanym urz¹dzeniem
//Ra: stan wyjœcia zmieniany przez symulacjê (mierniki, kontrolki)

/*******************************
Do klawisza klawiatury przypisana jest maska bitowa oraz numer wejœcia.
Naciœniêcie klawisza powoduje wywo³anie procedury ustawienia bitu o podanej
masce na podanym wejœciu. Zwonienie klawisza analogicznie wywo³uje zerowanie
bitu wg maski. Zasadniczo w masce ustawiony jest jeden bit, ale w razie
potrzeby mo¿e byæ ich wiêcej.

Oddzielne wejœcia s¹ wprowadzone po to, by mo¿na by³o u¿ywaæ wiêcej ni¿ 32
bity do sterowania. Podzia³ na wejœcia jest równie¿ ze wzglêdów organizacyjnych,
np. sterowanie œwiat³ami mo¿e mieæ oddzielny numer wejœcia ni¿ prze³¹czanie
radia, poniewa¿ nie ma potrzeby ich uzale¿niaæ (tzn. badaæ wspóln¹ maskê bitow¹).

Do ka¿dego wejœcia podpiêty jest skrypt binarny, charakterystyczny dla danej
konstrukcji pojazdu. Sprawdza on zale¿noœci (w tym uszkodzenia) za pomoc¹
operacji logicznych na maskach bitowych. Do ka¿dego wejœcia jest przypisana
jedna, oddzielna maska 32 bit, ale w razie potrzeby istnieje te¿ mo¿liwoœæ
korzystania z masek innych wejœæ. Skrypt mo¿e te¿ wysy³aæ maski na inne wejœcia,
ale nale¿y unikaæ rekurencji.

Definiowanie wejœæ oraz przeznaczenia ich masek jest w gestii konstruktora
skryptu. Ka¿dy pojazd mo¿e mieæ inny schemat wejœæ i masek, ale w miarê mo¿liwoœci
nale¿y d¹¿yæ do unifikacji. Skrypty mog¹ równie¿ u¿ywaæ dodatkowych masek bitowych.
Maski bitowe odpowiadaj¹ stanom prze³¹czników, czujników, styczników itd.

Dzia³anie jest nastêpuj¹ce:
- na klawiaturze konsoli naciskany jest przycisk
- naciœniêcie przycisku zamieniane jest na maskê bitow¹ oraz numer wejœcia
- wywo³ywany jest skrypt danego wejœcia z ow¹ mask¹
- skrypt sprawdza zale¿noœci i np. modyfikuje w³asnoœci fizyki albo inne maski
- ewentualnie do wyzwalacza czasowego dodana jest maska i numer wejœcia

/*******************************/

/* //kod do przetrawienia:
//aby siê nie w³¹czacz wygaszacz ekranu, co jakiœ czas naciska siê wirtualnie ScrollLock

[DllImport("user32.dll")]
static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);

private static void PressScrollLock()
{//przyciska i zwalnia ScrollLock
 const byte vkScroll = 0x91;
 const byte keyeventfKeyup = 0x2;
 keybd_event(vkScroll, 0x45, 0, (UIntPtr)0);
 keybd_event(vkScroll, 0x45, keyeventfKeyup, (UIntPtr)0);
};

[DllImport("user32.dll")]
private static extern bool SystemParametersInfo(int uAction,int uParam,int &lpvParam,int flags);

public static Int32 GetScreenSaverTimeout()
{
 Int32 value=0;
 SystemParametersInfo(14,0,&value,0);
 return value;
};
*/

//Ra: do poprawienia
void SetLedState(char Code,bool bOn)
{//Ra: bajer do migania LED-ami w klawiaturze

};

//---------------------------------------------------------------------------

int Console::iBits=0; //zmienna statyczna - obiekt Console jest jednen wspólny
int Console::iMode=0;
int Console::iConfig=0;
TPoKeys55 *Console::PoKeys55[2]={NULL,NULL};
TLPT *Console::LPT=NULL;
int Console::iSwitch[8]; //bistabilne w kabinie, za³¹czane z [Shift], wy³¹czane bez
int Console::iButton[8]; //monostabilne w kabinie, za³¹czane podczas trzymania klawisza

__fastcall Console::Console()
{
 PoKeys55[0]=PoKeys55[1]=NULL;
 for (int i=0;i<8;++i)
 {//zerowanie prze³¹czników
  iSwitch[i]=0; //bity 0..127 - bez [Ctrl], 128..255 - z [Ctrl]
  iButton[i]=0; //bity 0..127 - bez [Shift], 128..255 - z [Shift]
 }
};

__fastcall Console::~Console()
{
//- delete PoKeys55[0];
//- delete PoKeys55[1];
};

void __fastcall Console::ModeSet(int m,int h)
{//ustawienie trybu pracy
 iMode=m;
 iConfig=h;
};

int __fastcall Console::On()
{//za³¹czenie konsoli (np. nawi¹zanie komunikacji)
 iSwitch[0]=iSwitch[1]=iSwitch[2]=iSwitch[3]=0; //bity 0..127 - bez [Ctrl]
 iSwitch[4]=iSwitch[5]=iSwitch[6]=iSwitch[7]=0; //bity 128..255 - z [Ctrl]
 switch (iMode)
 {case 1: //kontrolki klawiatury
  case 2: //kontrolki klawiatury
   iConfig=0; //licznik u¿ycia Scroll Lock
  break;
  case 3: //LPT

  break;
  case 4: //PoKeys

  break;
 }
 return 0;
};

void __fastcall Console::Off()
{//wy³¹czenie informacji zwrotnych (reset pulpitu)
 BitsClear(-1);
 if ((iMode==1)||(iMode==2))
  if (iConfig&1) //licznik u¿ycia Scroll Lock
  {//bez sensu to jest, ale mi siê samo w³¹cza
   //-SetLedState(VK_SCROLL,true); //przyciœniêty
   //-SetLedState(VK_SCROLL,false); //zwolniony
  }
 //-delete PoKeys55[0]; PoKeys55[0]=NULL;
 //-delete PoKeys55[1]; PoKeys55[1]=NULL;
 //-delete LPT; LPT=NULL;
};

void __fastcall Console::BitsSet(int mask,int entry)
{//ustawienie bitów o podanej masce (mask) na wejœciu (entry)
 if ((iBits&mask)!=mask) //je¿eli zmiana
 {iBits|=mask;
  BitsUpdate(mask);
 }
};

void __fastcall Console::BitsClear(int mask,int entry)
{//zerowanie bitów o podanej masce (mask) na wejœciu (entry)
 if (iBits&mask) //je¿eli zmiana
 {iBits&=~mask;
  BitsUpdate(mask);
 }
};

void __fastcall Console::BitsUpdate(int mask)
{//aktualizacja stanu interfejsu informacji zwrotnej; (mask) - zakres zmienianych bitów
 switch (iMode)
 {case 1: //sterowanie œwiate³kami klawiatury: CA/SHP+opory
  // if (mask&3) //gdy SHP albo CA
  //  SetLedState(VK_CAPITAL,iBits&3);
  // if (mask&4) //gdy jazda na oporach
  // {//Scroll Lock ma jakoœ dziwnie... zmiana stanu na przeciwny
  //  //-SetLedState(VK_SCROLL,true); //przyciœniêty
  //  //-SetLedState(VK_SCROLL,false); //zwolniony
  //  ++iConfig; //licznik u¿ycia Scroll Lock
  // }
  break;
  case 2: //sterowanie œwiate³kami klawiatury: CA+SHP
	  /*
   if (mask&2) //gdy CA
    SetLedState(VK_CAPITAL,iBits&2);
   if (mask&1) //gdy SHP
   {//Scroll Lock ma jakoœ dziwnie... zmiana stanu na przeciwny
    SetLedState(VK_SCROLL,true); //przyciœniêty
    SetLedState(VK_SCROLL,false); //zwolniony
    ++iConfig; //licznik u¿ycia Scroll Lock
	
   }
   */
   break;
  case 3: //LPT Marcela z modyfikacj¹ (jazda na oporach zamiast brzêczyka)

   break;
  case 4: //PoKeys55 wg Marcela - wersja druga z koñca 2012
   if (PoKeys55[0])
   {//pewnie trzeba bêdzie to dodatkowo buforowaæ i oczekiwaæ na potwierdzenie

   }
   break;
 }
};

bool __fastcall Console::Pressed(int x)
{//na razie tak - czyta siê tylko klawiatura
 return Global::bActive&&(GetKeyState(x)<0);
};

void __fastcall Console::ValueSet(int x,double y)
{//ustawienie wartoœci (y) na kanale analogowym (x)

};

void __fastcall Console::Update()
{//funkcja powinna byæ wywo³ywana regularnie, np. raz w ka¿dej ramce ekranowej

};

float __fastcall Console::AnalogGet(int x)
{//pobranie wartoœci analogowej

	return 0.0;
};

unsigned char __fastcall Console::DigitalGet(int x)
{//pobranie wartoœci cyfrowej

	return '0';
};

void __fastcall Console::OnKeyDown(int k)
{//naciœniêcie klawisza z powoduje wy³¹czenie, a
 if (k&0x10000) //jeœli [Shift]
 {//ustawienie bitu w tabeli prze³¹czników bistabilnych
  if (k&0x20000) //jeœli [Ctrl], to zestaw dodatkowy
   iSwitch[4+(char(k)>>5)]|=1<<(k&31); //za³¹cz bistabliny dodatkowy
  else
  {//z [Shift] w³¹czenie bitu bistabilnego i dodatkowego monostabilnego
   iSwitch[char(k)>>5]|=1<<(k&31); //za³¹cz bistabliny podstawowy
   iButton[4+(char(k)>>5)]|=(1<<(k&31)); //za³¹cz monostabilny dodatkowy
  }
 }
 else
 {//zerowanie bitu w tabeli prze³¹czników bistabilnych
  if (k&0x20000) //jeœli [Ctrl], to zestaw dodatkowy
   iSwitch[4+(char(k)>>5)]&=~(1<<(k&31)); //wy³¹cz bistabilny dodatkowy
  else
  {iSwitch[char(k)>>5]&=~(1<<(k&31)); //wy³¹cz bistabilny podstawowy
   iButton[char(k)>>5]|=1<<(k&31); //za³¹cz monostabilny podstawowy
  }
 }
};
void __fastcall Console::OnKeyUp(int k)
{//puszczenie klawisza w zasadzie nie ma znaczenia dla iSwitch, ale zeruje iButton
 if ((k&0x20000)==0) //monostabilne tylko bez [Ctrl]
  if (k&0x10000) //jeœli [Shift]
   iButton[4+(char(k)>>5)]&=~(1<<(k&31)); //wy³¹cz monostabilny dodatkowy
  else
   iButton[char(k)>>5]&=~(1<<(k&31)); //wy³¹cz monostabilny podstawowy
};

