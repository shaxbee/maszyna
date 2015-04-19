//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#pragma hdrstop
#pragma warning ( disable : 4305 )	     // 'truncation from 'double' to 'float'
#pragma warning ( disable : 4244 )

#include "commons.h"
#include "commons_usr.h"
//#include "Globals.h"


//#define M_PI 3.14

bool DebugModeFlag = true;
//101206 Ra: trapezoidalne drogi i tory



  void drawSphere(double r, int lats, int longs) {
      int i, j;
      for(i = 0; i <= lats; i++) {
          double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
          double z0  = sin(lat0);
          double zr0 =  cos(lat0);
 
          double lat1 = M_PI * (-0.5 + (double) i / lats);
          double z1 = sin(lat1);
          double zr1 = cos(lat1);

          glFrontFace(GL_CW);
          glBegin(GL_QUAD_STRIP);
          for(j = 0; j <= longs; j++) {
              double lng = 2 * M_PI * (double) (j - 1) / longs;
              double x = cos(lng);
              double y = sin(lng);
 
              glNormal3f(x * zr0, y * zr0, z0);
              glVertex3f(x * zr0, y * zr0, z0);
              glNormal3f(x * zr1, y * zr1, z1);
              glVertex3f(x * zr1, y * zr1, z1);
          }
          glEnd();
		  glFrontFace(GL_CCW);
      }
  }


TSwitchExtension::TSwitchExtension()
{//na pocz¹tku wszystko puste
    CurrentIndex=0;
    pNexts[0]=NULL; //wskaŸniki do kolejnych odcinków ruchu
    pNexts[1]=NULL;
    pPrevs[0]=NULL;
    pPrevs[1]=NULL;
    fOffset1=fOffset2=fDesiredOffset1=fDesiredOffset2=0.0; //po³o¿enie zasadnicze 
    bMovement=false; //nie potrzeba przeliczaæ fOffset1
}
TSwitchExtension::~TSwitchExtension()
{
}

TTrack::TTrack()
{//tworzenie nowego odcinka ruchu
    pNext=pPrev= NULL;
    Segment= NULL;
    SwitchExtension=NULL;
    TextureID1= 0;
    fTexLength= 4.0;
    TextureID2= 0;
    fTexHeight= 0.6; //nowy profil podsypki ;)
    fTexWidth= 0.9;
    fTexSlope= 0.9;
    //HelperPts= NULL; //nie potrzebne, ale niech zostanie
    iCategoryFlag= 1;
    fTrackWidth= 1.435;
    fFriction= 0.15;
    fSoundDistance= -1;
    iQualityFlag= 20;
    iDamageFlag= 0;
    eEnvironment= e_flat;
    bVisible= true;
    Event0= NULL;
    Event1= NULL;
    Event2= NULL;
    Eventall0= NULL;
    Eventall1= NULL;
    Eventall2= NULL;
    fVelocity= -1;
    fTrackLength= 100.0;
    fRadius= 0; //promieñ wybranego toru zwrotnicy
    fRadiusTable[0]= 0; //dwa promienie nawet dla prostego
    fRadiusTable[1]= 0;
    iNumDynamics= 0;
    ScannedFlag=false;
    DisplayListID=0;
    iTrapezoid=0; //parametry kszta³tu: 0-standard, 1-przechy³ka, 2-trapez, 3-oba
}

TTrack::~TTrack()
{
    //SafeDeleteArray(HelperPts);
    switch (eType)
    {
        case tt_Switch:
            SafeDelete(SwitchExtension);
        break;
        case tt_Turn: //oba usuwane
            SafeDelete(SwitchExtension);
        case tt_Normal:
            SafeDelete(Segment);
        break;
    }
}

void  TTrack::Init()
{
    switch (eType)
    {
        case tt_Switch:
            SwitchExtension= new TSwitchExtension();
        break;
        case tt_Normal:
            Segment= new TSegment();
        break;
        case tt_Turn: //oba potrzebne
            SwitchExtension= new TSwitchExtension();
            Segment= new TSegment();
        break;
    }
}

void  TTrack::ConnectPrevPrev(TTrack *pTrack)
{
    if (pTrack)
    {
        pPrev= pTrack;
        bPrevSwitchDirection= true;
        pTrack->pPrev= this;
        pTrack->bPrevSwitchDirection= true;
    }
}

void  TTrack::ConnectPrevNext(TTrack *pTrack)
{
    if (pTrack)
    {
        pPrev= pTrack;
        bPrevSwitchDirection= false;
        pTrack->pNext= this;
        pTrack->bNextSwitchDirection= false;
        if (bVisible)
         if (pTrack->bVisible)
          if (eType==tt_Normal) //jeœli ³¹czone s¹ dwa normalne
           if (pTrack->eType==tt_Normal)
            if ((fTrackWidth!=pTrack->fTrackWidth) //Ra: jeœli kolejny ma inne wymiary
             || (fTexHeight!=pTrack->fTexWidth)
             || (fTexWidth!=pTrack->fTexWidth)
             || (fTexSlope!=pTrack->fTexSlope))
             pTrack->iTrapezoid|=2; //to rysujemy potworka
    }
}

void  TTrack::ConnectNextPrev(TTrack *pTrack)
{
    if (pTrack)
    {
        pNext= pTrack;
        bNextSwitchDirection= false;
        pTrack->pPrev= this;
        pTrack->bPrevSwitchDirection= false;
        if (bVisible)
         if (pTrack->bVisible)
          if (eType==tt_Normal) //jeœli ³¹czone s¹ dwa normalne
           if (pTrack->eType==tt_Normal)
            if ((fTrackWidth!=pTrack->fTrackWidth) //Ra: jeœli kolejny ma inne wymiary
             || (fTexHeight!=pTrack->fTexWidth)
             || (fTexWidth!=pTrack->fTexWidth)
             || (fTexSlope!=pTrack->fTexSlope))
             iTrapezoid|=2; //to rysujemy potworka
    }
}

void  TTrack::ConnectNextNext(TTrack *pTrack)
{
    if (pTrack)
    {
        pNext= pTrack;
        bNextSwitchDirection= true;
        pTrack->pNext= this;
        pTrack->bNextSwitchDirection= true;
    }
}

/*
bool  TTrack::ConnectNext1(TTrack *pNewNext)
{
    if (pNewNext)
    {
        if (pNext1)
            pNext1->pPrev= NULL;
        pNext1= pNewNext;
        pNext1->pPrev= this;
        return true;
    }
    return false;
}

bool  TTrack::ConnectNext2(TTrack *pNewNext)
{
    if (pNewNext)
    {
        if (pNext2)
            pNext2->pPrev= NULL;
        pNext2= pNewNext;
        pNext2->pPrev= this;
        return true;
    }
    return false;
}

bool  TTrack::ConnectPrev1(TTrack *pNewPrev)
{
    if (pNewPrev)
    {
        if (pPrev)
        {
            if (pPrev->pNext1==this)
                pPrev->pNext1= NULL;
            if (pPrev->pNext2==this)
                pPrev->pNext2= NULL;
        }
        pPrev= pNewPrev;
        pPrev->pNext1= this;
        return true;
    }
    return false;
}

bool  TTrack::ConnectPrev2(TTrack *pNewPrev)
{
    if (pNewPrev)
    {
        if (pPrev)
        {
            if (pPrev->pNext1==this)
                pPrev->pNext1= NULL;
            if (pPrev->pNext2==this)
                pPrev->pNext2= NULL;
        }
        pPrev= pNewPrev;
        pPrev->pNext2= this;
        return true;
    }
    return false;
}
*/
vector3  MakeCPoint(vector3 p, double d, double a1, double a2)
{
    vector3 cp= vector3(0,0,1);
    cp.RotateX(DegToRad(a2));
    cp.RotateY(DegToRad(a1));
    cp= cp*d+p;

    return cp;

}

vector3  LoadPoint(cParser *parser)
{//pobranie wspó³rzêdnych punktu
    vector3 p;
    std::string token;
    parser->getTokens(3);
    *parser >> p.x >> p.y >> p.z;
    return p;
}

/* Ra: to siê niczym nie ró¿ni od powy¿szego
vector3  LoadCPoint(cParser *parser)
{//pobranie wspó³rzêdnych wektora kontrolnego
    vector3 cp;
    std::string token;
    parser->getTokens(3);
    *parser >> cp.x >> cp.y >> cp.z;
    return cp;
}
*/

/* Ra: to chyba kiedyœ by³a zwrotnica...
const vector3 sw1pts[]= { vector3(1.378,0.0,25.926),vector3(0.378,0.0,16.926),
                          vector3(0.0,0.0,26.0), vector3(0.0,0.0,26.0),
                          vector3(0.0,0.0,6.0) };

const vector3 sw1pt1= vector3(0.0,0.0,26.0);
const vector3 sw1cpt1= vector3(0.0,0.0,26.0);
const vector3 sw1pt2= vector3(1.378,0.0,25.926);
const vector3 sw1cpt2= vector3(0.378,0.0,16.926);
const vector3 sw1pt3= vector3(0.0,0.0,0.0);
const vector3 sw1cpt3= vector3(0.0,0.0,6.0);
*/

char* TTrack::stdstrtochar(std::string var)
{
	char* ReturnString = new char[100];
	char szBuffer[100];
	LPCSTR lpMyString = var.c_str();

    sprintf(szBuffer,"%s",lpMyString);

	strcpy( ReturnString, szBuffer );

	return ReturnString;
}

void  TTrack::Load(cParser *parser, vector3 pOrigin, std::string name)
{//pobranie obiektu trajektorii ruchu
    vector3 pt,vec,p1,p2,cp1,cp2,p3,cp3,p4,cp4,p5,cp5,swpt[3],swcp[3],dir;
    double a1,a2,r1,r2,d1,d2,a;
	std::string str;
    bool bCurve;
    int i;//,state; //Ra: teraz ju¿ nie ma pocz¹tkowego stanu zwrotnicy we wpisie
    std::string token;

	asName = name;

    parser->getTokens();
    *parser >> token;
    str= token; // CZY MA BYC c_str()???????????????

    if (str=="normal")
     {
        eType= tt_Normal;
        iCategoryFlag= 1;
     }
    else
    if (str=="switch")
     {
        eType= tt_Switch;
        iCategoryFlag= 1;
     }
    else
    if (str=="turn")
     {//Ra: to bêdzie obrotnica
        eType= tt_Turn;
        iCategoryFlag= 1;
     }
    else
    if (str=="road")
     {
        eType= tt_Normal;
        iCategoryFlag= 2;
     }
    else
    if (str=="cross")
     {//Ra: to bêdzie skrzy¿owanie dróg
        eType= tt_Cross;
        iCategoryFlag= 2;
     }
    else
    if (str=="river")
     {
        eType= tt_Normal;
        iCategoryFlag= 4;
     }
    else
       eType= tt_Unknown;

	if (DebugModeFlag) WriteLogSS("STYPE: ", str.c_str());  // SUBTYPE - switch, normal !!!!!!!!!

    parser->getTokens(4);
    *parser >> fTrackLength >> fTrackWidth >> fFriction >> fSoundDistance;
//    fTrackLength= Parser->GetNextSymbol().ToDouble();                       //track length 100502
//    fTrackWidth= Parser->GetNextSymbol().ToDouble();                        //track width
//    fFriction= Parser->GetNextSymbol().ToDouble();                          //friction coeff.
//    fSoundDistance= Parser->GetNextSymbol().ToDouble();   //snd
    parser->getTokens(2);
    *parser >> iQualityFlag >> iDamageFlag;
//    iQualityFlag= Parser->GetNextSymbol().ToInt();   //McZapkie: qualityflag
//    iDamageFlag= Parser->GetNextSymbol().ToInt();   //damage
    parser->getTokens();
    *parser >> token;
    str= token;  //environment
    if (str=="flat")
     {
       eEnvironment= e_flat;
     }
    else
    if (str=="mountains" || str=="mountain")
     {
       eEnvironment= e_mountains;
     }
    else
    if (str=="canyon")
     {
       eEnvironment= e_canyon;
     }
    else
    if (str=="tunnel")
     {
       eEnvironment= e_tunnel;
     }
    else
    if (str=="bridge")
     {
       eEnvironment= e_bridge;
     }
    else
    if (str=="bank")
     {
       eEnvironment= e_bank;
     }
    else
       {
        eEnvironment= e_unknown;
		char tolog[100];
		sprintf(tolog,"Unknown track environment: %s", str);
        //Error("Unknown track environment: \""+str+"\"");
		MessageBox(0,tolog,"Error",MB_OK);
       }
    parser->getTokens();
    *parser >> token;
    bVisible= (token.compare( "vis" ) == 0 );   //visible

	iRAILTYPE = 1; // DEFAULT RAIL

    if (bVisible)
     {
      parser->getTokens();
      *parser >> token;
      str= token;   //railtex
	  TextureID1= TTexturesManager::GetTextureID(stdstrtochar(str),0);
      parser->getTokens();
      *parser >> fTexLength; //tex tile length
      parser->getTokens();
      *parser >> token;
      str= token.c_str();   //sub || railtex
      TextureID2= TTexturesManager::GetTextureID(stdstrtochar(str),0);
      parser->getTokens(3);
      *parser >> fTexHeight >> fTexWidth >> fTexSlope;
//      fTexHeight= Parser->GetNextSymbol().ToDouble(); //tex sub height
//      fTexWidth= Parser->GetNextSymbol().ToDouble(); //tex sub width
//      fTexSlope= Parser->GetNextSymbol().ToDouble(); //tex sub slope width
     }
    else
     {
     if (DebugModeFlag)
      WriteLog("unvis");
     }
    Init();
    double segsize=0.5f; //Ra: 5m przy ma³ych promieniach 5m Ÿle wygl¹da (drogi, tramwaje)
    switch (eType)
    {//Ra: ³uki by³y segmentowane co 5m albo 314-k¹tem foremnym
     //Ra: zmieni³em na minimum 0.5m, za to 209-k¹t
        case tt_Turn: //obrotnica jest prawie jak zwyk³y tor
        case tt_Normal:
            p1= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P1
            parser->getTokens();
            *parser >> r1; //pobranie przechy³ki w P1
            cp1= LoadPoint(parser); //pobranie wspó³rzêdnych punktów kontrolnych
            cp2= LoadPoint(parser);
            p2= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P2
            parser->getTokens(2);
            *parser >> r2 >> fRadius; //pobranie przechy³ki w P1 i promienia

            if (fRadius!=0) //gdy podany promieñ
               segsize=Min0R(0.5,0.3+fabs(fRadius)*0.03); //do 250m - 5, potem 1 co 50m

            if ((((p1+p1+p2)/3.0-p1-cp1).Length()<0.02)||(((p1+p2+p2)/3.0-p2+cp1).Length()<0.02))
             cp1=cp2=vector3(0,0,0); //"prostowanie" prostych z kontrolnymi, dok³adnoœæ 2cm

            if ((cp1==vector3(0,0,0)) && (cp2==vector3(0,0,0))) //Ra: hm, czasem dla prostego s¹ podane...
             Segment->Init(p1,p2,segsize,r1,r2); //gdy prosty, kontrolne wyliczane przy zmiennej przechy³ce
            else
             Segment->Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2); //gdy ³uk (ustawia bCurve=true)
            if ((r1!=0)||(r2!=0)) iTrapezoid=1; //s¹ przechy³ki do uwzglêdniania w rysowaniu
            if (eType==tt_Turn) //obrotnica ma doklejkê
            {
			SwitchExtension= new TSwitchExtension(); //zwrotnica ma doklejkê
             SwitchExtension->Segments->Init(p1,p2,segsize); //kopia oryginalnego toru
            }
        break;

        case tt_Cross: //skrzy¿owanie dróg - 4 punkty z wektorami kontrolnymi
        case tt_Switch: //zwrotnica
            //problemy z animacj¹ iglic powstaje, gdzy odcinek prosty ma zmienn¹ przechy³ê
            //wtedy dzieli sie na dodatkowe odcinki (po 0.2m, bo R=0) i animacjê diabli bior¹
            //Ra: na razie nie podejmujê siê przerabiania iglic
            //Init w TSegment ignoruje przechy³ki o sumie mniejszej od 0.21°

            SwitchExtension= new TSwitchExtension(); //zwrotnica ma doklejkê

            p1= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P1
            parser->getTokens();
            *parser >> r1;
            cp1= LoadPoint(parser);
            cp2= LoadPoint(parser);
            p2= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P2
            parser->getTokens(2);
            *parser >> r2 >> fRadiusTable[0];

            if (fRadiusTable[0]>0)
               segsize=Min0R(5.0,0.2+fRadiusTable[0]*0.02);
            else
            {cp1=(p1+p1+p2)/3.0-p1; cp2=(p1+p2+p2)/3.0-p2; segsize=5.0;} //u³omny prosty

            if (!(cp1==vector3(0,0,0)) && !(cp2==vector3(0,0,0)))
                SwitchExtension->Segments[0].Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
            else
                SwitchExtension->Segments[0].Init(p1,p2,segsize,r1,r2);

            p1= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P3
            parser->getTokens();
            *parser >> r1;
            cp1= LoadPoint(parser);
            cp2= LoadPoint(parser);
            p2= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P4
            parser->getTokens(2);
            *parser >> r2 >> fRadiusTable[1];

            if (fRadiusTable[1]>0)
               segsize=Min0R(5.0,0.2+fRadiusTable[1]*0.02);
            else
               segsize=5.0;

            if (!(cp1==vector3(0,0,0)) && !(cp2==vector3(0,0,0)))
                SwitchExtension->Segments[1].Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
            else
                SwitchExtension->Segments[1].Init(p1,p2,segsize,r1,r2);

            Switch(0); //na sta³e w po³o¿eniu 0 - nie ma pocz¹tkowego stanu zwrotnicy we wpisie

            //Ra: zamieniæ póŸniej na iloczyn wektorowy
            {vector3 v1,v2;
             double a1,a2;
             v1=SwitchExtension->Segments[0].FastGetPoint_1()-SwitchExtension->Segments[0].FastGetPoint_0();
             v2=SwitchExtension->Segments[1].FastGetPoint_1()-SwitchExtension->Segments[1].FastGetPoint_0();
             a1=atan2(v1.x,v1.z);
             a2=atan2(v2.x,v2.z);
             a2=a2-a1;
             while (a2>M_PI) a2=a2-2*M_PI;
             while (a2<-M_PI) a2=a2+2*M_PI;
             SwitchExtension->RightSwitch=a2<0; //lustrzany uk³ad OXY...
            }
        break;
    }
    parser->getTokens();
    *parser >> token;
    str= token;
    while (str!="endtrack")
    {
        if (str=="rail")
         {
            parser->getTokens();
            *parser >> token;
            asRAILTYPE= token;
			if (asRAILTYPE == "s49l") iRAILTYPE = 1;
         }
        else
        if (str=="event0")
         {
            parser->getTokens();
            *parser >> token;
            asEvent0Name= token;
         }
        else
        if (str=="event1")
         {
            parser->getTokens();
            *parser >> token;
            asEvent1Name= token.c_str();
         }
        else
        if (str=="event2")
         {
            parser->getTokens();
            *parser >> token;
            asEvent2Name= token.c_str();
         }
        else
        if (str=="eventall0")
         {
            parser->getTokens();
            *parser >> token;
            asEventall0Name= token;
         }
        else
        if (str=="eventall1")
         {
            parser->getTokens();
            *parser >> token;
            asEventall1Name= token.c_str();
         }
        else
        if (str=="eventall2")
         {
            parser->getTokens();
            *parser >> token;
            asEventall2Name= token;
         }
        else
        if (str=="velocity")
         {
           parser->getTokens();
           *parser >> fVelocity;
//            fVelocity= Parser->GetNextSymbol().ToDouble(); //*0.28; McZapkie-010602
         }
        else
		{
			char tolog[100];
			sprintf(tolog,"Unknown track property: %s", str);
		    MessageBox(0, tolog, "ERROR", MB_OK);
        	//Error("Unknown track property: \""+str+"\"");
		}
       parser->getTokens(); *parser >> token;
	   str= token;
    }
}

bool  TTrack::AssignEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2)
{
    bool bError= false;
    if (!Event0)
    {
        if (NewEvent0)
        {
            Event0= NewEvent0;
            asEvent0Name= "";
        }
        else
        {
            if (!asEvent0Name.empty())
            {
		  		char tolog[100];
		     	sprintf(tolog,"Event0 %s does not exist", asEvent0Name);
				MessageBox(0, tolog, "ERROR", MB_OK);
               // Error(AnsiString("Event0 \"")+asEvent0Name+AnsiString("\" does not exist"));
                bError= true;
            }
        }
    }
    else
    {
        //Error(AnsiString("Event 0 cannot be assigned to track, track already has one"));
		MessageBox(0,"Event 0 cannot be assigned to track, track already has one", "Error", MB_OK);
        bError= true;
    }

    if (!Event1)
    {
        if (NewEvent1)
        {
            Event1= NewEvent1;
            asEvent1Name= "";
        }
        else
        {
            if (!asEvent0Name.empty())
            {//Ra: tylko w logu informacja
                //---WriteLog(AnsiString("Event1 \"")+asEvent1Name+AnsiString("\" does not exist").c_str());
                bError= true;
            }
        }
    }
    else
    {
        //----Error(AnsiString("Event 1 cannot be assigned to track, track already has one"));
        bError= true;
    }

    if (!Event2)
    {
        if (NewEvent2)
        {
            Event2= NewEvent2;
            asEvent2Name= "";
        }
        else
        {
            if (!asEvent0Name.empty())
            {//Ra: tylko w logu informacja
                //----WriteLog(AnsiString("Event2 \"")+asEvent2Name+AnsiString("\" does not exist"));
                bError= true;
            }
        }
    }
    else
    {
        //----Error(AnsiString("Event 2 cannot be assigned to track, track already has one"));
        bError= true;
    }
    return !bError;
}

bool  TTrack::AssignallEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2)
{
    bool bError= false;
    if (!Eventall0)
    {
        if (NewEvent0)
        {
            Eventall0= NewEvent0;
            asEventall0Name= "";
        }
        else
        {
            if (!asEvent0Name.empty())
            {
                //Error(AnsiString("Eventall 0 \"")+asEventall0Name+AnsiString("\" does not exist"));
				MessageBox(0,"Eventall 0  does not exist","Error",MB_OK);
                bError= true;
            }
        }
    }
    else
    {
       // Error(AnsiString("Eventall 0 cannot be assigned to track, track already has one"));
		MessageBox(0,"Eventall 0 cannot be assigned to track, track already has one","Error",MB_OK);
        bError= true;
    }

    if (!Eventall1)
    {
        if (NewEvent1)
        {
            Eventall1= NewEvent1;
            asEventall1Name= "";
        }
        else
        {
            if (!asEvent0Name.empty())
            {//Ra: tylko w logu informacja
				char tolog[100];
		        sprintf(tolog,"Eventall 1 %s does not exist", asEventall1Name);
                WriteLog(tolog);
                bError= true;
            }
        }
    }
    else
    {
        //Error(AnsiString("Eventall 1 cannot be assigned to track, track already has one"));
		MessageBox(0,"Eventall 1 cannot be assigned to track, track already has one","Error",MB_OK);
        bError= true;
    }

    if (!Eventall2)
    {
        if (NewEvent2)
        {
            Eventall2= NewEvent2;
            asEventall2Name= "";
        }
        else
        {
            if (!asEvent0Name.empty())
            {//Ra: tylko w logu informacja
				char tolog[100];
				sprintf(tolog, " Eventall 2 \ %s  does not exist", asEventall2Name);
                WriteLog(tolog);
                bError= true;
            }
        }
    }
    else
    {
        //Error(AnsiString("Eventall 2 cannot be assigned to track, track already has one"));
		MessageBox(0,"Eventall 2 cannot be assigned to track, track already has one","Error",MB_OK);
        bError= true;
    }
    return !bError;
}


//ABu: przeniesione z Track.h i poprawione!!!
bool  TTrack::AddDynamicObject(TDynamicObject *Dynamic)
    {
        if (iNumDynamics<iMaxNumDynamics)
        {
            Dynamics[iNumDynamics]= Dynamic;
            iNumDynamics++;
            Dynamic->MyTrack = this; //ABu: Na ktorym torze jestesmy.
            return true;
        }
        else
        {
            WriteLog("Cannot add dynamic to track");
            return false;
        }
		
		return true;
    };

void  TTrack::MoveMe(vector3 pPosition)
{
    if(SwitchExtension)
    {
        SwitchExtension->Segments[0].MoveMe(1*pPosition);
        SwitchExtension->Segments[1].MoveMe(1*pPosition);
        SwitchExtension->Segments[2].MoveMe(3*pPosition); //Ra: 3 razy?
        SwitchExtension->Segments[3].MoveMe(4*pPosition);
    }
    else
    {
       Segment->MoveMe(pPosition);
    };

    ResourceManager::Unregister(this);

};


const int numPts= 4;
const int nnumPts= 12;
const vector3 szyna[nnumPts]= //szyna - vextor3(x,y,mapowanie tekstury)
{vector3( 0.111,0.000,0.00),
 vector3( 0.045,0.025,0.15),
 vector3( 0.045,0.110,0.25),
 vector3( 0.071,0.140,0.35), //albo tu 0.073
 vector3( 0.072,0.170,0.40),
 vector3( 0.052,0.180,0.45),
 vector3( 0.020,0.180,0.55),
 vector3( 0.000,0.170,0.60),
 vector3( 0.001,0.140,0.65), //albo tu -0.001
 vector3( 0.027,0.110,0.75), //albo zostanie asymetryczna
 vector3( 0.027,0.025,0.85),
 vector3(-0.039,0.000,1.00)
};
/*
const int S49nnumPts= 50;
const vector3 szynaS49[S49nnumPts]= //szyna - vextor3(x,y,mapowanie tekstury)
{
 // JEDNA POLOWA SZYNY
 vector3( 0.098,0.000,0.00), 
 vector3( 0.098,0.009,0.05),
 vector3( 0.098,0.001,0.25),
 vector3( 0.096,0.011,0.35), //albo tu 0.073
 vector3( 0.089,0.011,0.40),
 vector3( 0.086,0.012,0.45),
 vector3( 0.065,0.017,0.55),
 vector3( 0.056,0.020,0.60),
 vector3( 0.051,0.022,0.65), //albo tu -0.001
 vector3( 0.048,0.024,0.75), //albo zostanie asymetryczna
 vector3( 0.045,0.027,0.85),
 vector3( 0.044,0.036,1.00),
 vector3( 0.044,0.056,0.00),
 vector3( 0.044,0.070,0.15),
 vector3( 0.044,0.090,0.25),
 vector3( 0.044,0.096,0.35), //albo tu 0.073
 vector3( 0.046,0.100,0.40),
 vector3( 0.049,0.102,0.45),
 vector3( 0.073,0.011,0.55),
 vector3( 0.071,0.139,0.60),
 vector3( 0.069,0.142,0.65), //albo tu -0.001
 vector3( 0.066,0.145,0.75), //albo zostanie asymetryczna
 vector3( 0.062,0.147,0.85),
 vector3( 0.056,0.149,1.00),
 vector3( 0.037,0.149,1.00),

 // DRUGA POLOWA
 vector3( 0.036,0.149,0.00), 
 vector3( 0.017,0.149,0.15),
 vector3( 0.011,0.147,0.25),
 vector3( 0.011,0.145,0.35), //albo tu 0.073
 vector3( 0.007,0.142,0.40),
 vector3( 0.004,0.139,0.45),
 vector3( 0.002,0.011,0.55),
 vector3( 0.000,0.102,0.60),
 vector3( 0.027,0.100,0.65), //albo tu -0.001
 vector3( 0.028,0.096,0.75), //albo zostanie asymetryczna
 vector3( 0.029,0.090,0.85),
 vector3( 0.029,0.070,1.00),
 vector3( 0.029,0.056,0.00),
 vector3( 0.028,0.036,0.15),
 vector3( 0.028,0.027,0.25),
 vector3( 0.025,0.024,0.35), //albo tu 0.073
 vector3( 0.021,0.022,0.60),
 vector3( 0.016,0.020,0.65),
 vector3( 0.007,0.017,0.70),
 vector3(-0.013,0.012,0.75),
 vector3(-0.016,0.011,0.80), //albo tu -0.001
 vector3(-0.023,0.011,0.85), //albo zostanie asymetryczna
 vector3(-0.024,0.010,0.90),
 vector3(-0.025,0.009,0.95),
 vector3(-0.025,0.000,1.00)
};
*/

float szp = 0.03;
const int S49LnnumPts= 34;
const vector3 szynaS49L[S49LnnumPts]= //szyna - vextor3(x,y,mapowanie tekstury)
{
 // JEDNA POLOWA SZYNY
 vector3( 0.062+szp, 0.000,0.00), 
 vector3( 0.062+szp, 0.008,0.15),
 vector3( 0.061+szp, 0.010,0.25),
 vector3( 0.059+szp, 0.011,0.35), //albo tu 0.073
 vector3( 0.050+szp, 0.012,0.40),
 vector3( 0.018+szp, 0.020,0.45),
 vector3( 0.013+szp, 0.023,0.10),
 vector3( 0.009+szp, 0.026,0.15),
 vector3( 0.007+szp, 0.053,0.20), //albo tu -0.001
 vector3( 0.008+szp, 0.096,0.25), //albo zostanie asymetryczna
 vector3( 0.011+szp, 0.102,0.30),
 vector3( 0.036+szp, 0.110,0.32),
 vector3( 0.034+szp, 0.140,0.35),
 vector3( 0.031+szp, 0.144,0.36),
 vector3( 0.025+szp, 0.148,0.37),
 vector3( 0.019+szp, 0.149,0.40), //albo tu 0.073
 vector3( 0.010+szp, 0.149,0.45),

 // DRUGA POLOWA
 vector3(-0.010+szp, 0.149,0.55), 
 vector3(-0.019+szp, 0.149,0.60),
 vector3(-0.025+szp, 0.148,0.63),
 vector3(-0.031+szp, 0.144,0.64), //albo tu 0.073
 vector3(-0.034+szp, 0.140,0.66),
 vector3(-0.036+szp, 0.110,0.68),
 vector3(-0.011+szp, 0.102,0.70),
 vector3(-0.008+szp, 0.096,0.75),
 vector3(-0.007+szp, 0.053,0.80), //albo tu -0.001
 vector3(-0.009+szp, 0.026,0.85), //albo zostanie asymetryczna
 vector3(-0.013+szp, 0.023,0.87),
 vector3(-0.018+szp, 0.020,0.90),
 vector3(-0.050+szp, 0.012,0.92),
 vector3(-0.059+szp, 0.011,0.95),
 vector3(-0.061+szp, 0.010,0.97),
 vector3(-0.062+szp, 0.008,0.98), //albo tu 0.073
 vector3(-0.062+szp, 0.000,1.00),

};


const int RI60NnnumPts= 64;
const vector3 szynaRI60N[RI60NnnumPts]= //szyna - vextor3(x,y,mapowanie tekstury)
{
// PIERWSZA POLOWA NA PLUSIE 32v
vector3(0.0057+szp, 0.0001,1.00),

vector3(0.0057+szp, 0.1003,1.00),
vector3(0.0058+szp, 0.1062,1.00),
vector3(0.0080+szp, 0.1105,1.00),
vector3(0.0121+szp, 0.1137,1.00),

vector3(0.0438+szp, 0.1207,1.00),
vector3(0.0499+szp, 0.1234,1.00),
vector3(0.0554+szp, 0.1263,1.00),
vector3(0.0611+szp, 0.1318,1.00),
vector3(0.0651+szp, 0.1371,1.00),
vector3(0.0680+szp, 0.1435,1.00),

vector3(0.0723+szp, 0.1718,1.00),
vector3(0.0715+szp, 0.1742,1.00),
vector3(0.0697+szp, 0.1757,1.00),
vector3(0.0674+szp, 0.1762,1.00),

vector3(0.0540+szp, 0.1759,0.27),
vector3(0.0528+szp, 0.1754,0.28),
vector3(0.0519+szp, 0.1748,0.29),
vector3(0.0511+szp, 0.1729,0.30),

vector3(0.0478+szp, 0.1484,0.34),
vector3(0.0458+szp, 0.1417,0.35),
vector3(0.0408+szp, 0.1356,0.36),
vector3(0.0362+szp, 0.1336,0.37),
vector3(0.0306+szp, 0.1333,0.38),
vector3(0.0244+szp, 0.1357,0.39),
vector3(0.0207+szp, 0.1400,0.40),

vector3(0.0187+szp, 0.1452,0.410),
vector3(0.0166+szp, 0.1610,0.420),
vector3(0.0154+szp, 0.1689,0.430),
vector3(0.0125+szp, 0.1742,0.440),
vector3(0.0088+szp, 0.1774,0.450),
vector3(0.0042+szp, 0.1793,0.460),

// DRUGA POLOWA NA MINUSIE 32verts
vector3(-0.0023+szp, 0.1804,0.460),
vector3(-0.0111+szp, 0.1808,0.500),
vector3(-0.0217+szp, 0.1810,0.550),

vector3(-0.0265+szp, 0.1807,0.570),
vector3(-0.0321+szp, 0.1797,0.580),
vector3(-0.0370+szp, 0.1785,0.585),
vector3(-0.0382+szp, 0.1777,0.590),
vector3(-0.0392+szp, 0.1764,0.595),
vector3(-0.0397+szp, 0.1741,0.60),

vector3(-0.0396+szp, 0.1570,0.62),
vector3(-0.0392+szp, 0.1547,0.63),
vector3(-0.0380+szp, 0.1525,0.64),
vector3(-0.0359+szp, 0.1493,0.65),
vector3(-0.0324+szp, 0.1462,0.66),
vector3(-0.0298+szp, 0.1452,0.67),

vector3(-0.0138+szp, 0.1421,0.68),
vector3(-0.0106+szp, 0.1407,0.69),
vector3(-0.0084+szp, 0.1386,0.695),
vector3(-0.0072+szp, 0.1361,0.70),
vector3(-0.0064+szp, 0.1327,0.705),

vector3(-0.0061+szp, 0.0256,0.88),
vector3(-0.0065+szp, 0.0229,0.89),
vector3(-0.0073+szp, 0.0211,0.90),
vector3(-0.0094+szp, 0.0186,0.91),
vector3(-0.0120+szp, 0.0171,0.92),
vector3(-0.0143+szp, 0.0166,0.93),

vector3(-0.0875+szp, 0.0085,0.975),
vector3(-0.0889+szp, 0.0080,0.98),
vector3(-0.0895+szp, 0.0067,0.985),
vector3(-0.0895+szp, 0.0016,0.99),
vector3(-0.0892+szp, 0.0006,0.995),
vector3(-0.0881+szp, 0.0000,1.00)

};

const vector3 iglica[nnumPts]= //iglica - vextor3(x,y,mapowanie tekstury)
{vector3( 0.010,0.000,0.00),
 vector3( 0.010,0.025,0.15),
 vector3( 0.010,0.110,0.25),
 vector3( 0.010,0.140,0.35),
 vector3( 0.010,0.170,0.40),
 vector3( 0.010,0.180,0.45),
 vector3( 0.000,0.180,0.55),
 vector3( 0.000,0.170,0.60),
 vector3( 0.000,0.140,0.65),
 vector3( 0.000,0.110,0.75),
 vector3( 0.000,0.025,0.85),
 vector3(-0.040,0.000,1.00) //1mm wiêcej, ¿eby nie nachodzi³y tekstury?
};


const vector3 iglicaS49L[S49LnnumPts]= //szyna - vextor3(x,y,mapowanie tekstury)
{
 // JEDNA POLOWA SZYNY
 vector3( 0.010+szp, 0.000,0.00), 
 vector3( 0.010+szp, 0.008,0.15),
 vector3( 0.010+szp, 0.010,0.25),
 vector3( 0.010+szp, 0.011,0.35), //albo tu 0.073
 vector3( 0.010+szp, 0.012,0.40),
 vector3( 0.010+szp, 0.020,0.45),
 vector3( 0.010+szp, 0.023,0.10),
 vector3( 0.010+szp, 0.026,0.15),
 vector3( 0.010+szp, 0.053,0.20), //albo tu -0.001
 vector3( 0.010+szp, 0.096,0.25), //albo zostanie asymetryczna
 vector3( 0.010+szp, 0.102,0.30),
 vector3( 0.010+szp, 0.110,0.32),
 vector3( 0.010+szp, 0.140,0.35),
 vector3( 0.010+szp, 0.144,0.36),
 vector3( 0.010+szp, 0.148,0.37),
 vector3( 0.010+szp, 0.149,0.40), //albo tu 0.073
 vector3( 0.010+szp, 0.149,0.45),

 // DRUGA POLOWA
 vector3(-0.000+szp, 0.149,0.55), 
 vector3(-0.000+szp, 0.149,0.60),
 vector3(-0.000+szp, 0.148,0.63),
 vector3(-0.000+szp, 0.144,0.64), //albo tu 0.073
 vector3(-0.000+szp, 0.140,0.66),
 vector3(-0.000+szp, 0.110,0.68),
 vector3(-0.000+szp, 0.102,0.70),
 vector3(-0.000+szp, 0.096,0.75),
 vector3(-0.000+szp, 0.053,0.80), //albo tu -0.001
 vector3(-0.000+szp, 0.026,0.85), //albo zostanie asymetryczna
 vector3(-0.000+szp, 0.023,0.87),
 vector3(-0.000+szp, 0.020,0.90),
 vector3(-0.000+szp, 0.012,0.92),
 vector3(-0.000+szp, 0.011,0.95),
 vector3(-0.000+szp, 0.010,0.97),
 vector3(-0.000+szp, 0.008,0.98), //albo tu 0.073
 vector3(-0.000+szp, 0.000,1.00),
};

const vector3 iglicaRI60N[RI60NnnumPts]= //szyna - vextor3(x,y,mapowanie tekstury)
{
// PIERWSZA POLOWA NA PLUSIE 32v
vector3(0.0057+szp, 0.0001,1.00),

vector3(0.0057+szp, 0.1003,1.00),
vector3(0.0058+szp, 0.1062,1.00),
vector3(0.0080+szp, 0.1105,1.00),
vector3(0.0121+szp, 0.1137,1.00),

vector3(0.0438+szp, 0.1207,1.00),
vector3(0.0499+szp, 0.1234,1.00),
vector3(0.0554+szp, 0.1263,1.00),
vector3(0.0611+szp, 0.1318,1.00),
vector3(0.0651+szp, 0.1371,1.00),
vector3(0.0680+szp, 0.1435,1.00),

vector3(0.0723+szp, 0.1718,1.00),
vector3(0.0715+szp, 0.1742,1.00),
vector3(0.0697+szp, 0.1757,1.00),
vector3(0.0674+szp, 0.1762,1.00),

vector3(0.0540+szp, 0.1759,0.27),
vector3(0.0528+szp, 0.1754,0.28),
vector3(0.0519+szp, 0.1748,0.29),
vector3(0.0511+szp, 0.1729,0.30),

vector3(0.0478+szp, 0.1484,0.34),
vector3(0.0458+szp, 0.1417,0.35),
vector3(0.0408+szp, 0.1356,0.36),
vector3(0.0362+szp, 0.1336,0.37),
vector3(0.0306+szp, 0.1333,0.38),
vector3(0.0244+szp, 0.1357,0.39),
vector3(0.0207+szp, 0.1400,0.40),

vector3(0.0187+szp, 0.1452,0.410),
vector3(0.0166+szp, 0.1610,0.420),
vector3(0.0154+szp, 0.1689,0.430),
vector3(0.0125+szp, 0.1742,0.440),
vector3(0.0088+szp, 0.1774,0.450),
vector3(0.0042+szp, 0.1793,0.460),

// DRUGA POLOWA NA MINUSIE 32verts
vector3(-0.0023+szp, 0.1804,0.460),
vector3(-0.0111+szp, 0.1808,0.500),
vector3(-0.0217+szp, 0.1810,0.550),

vector3(-0.0265+szp, 0.1807,0.570),
vector3(-0.0321+szp, 0.1797,0.580),
vector3(-0.0370+szp, 0.1785,0.585),
vector3(-0.0382+szp, 0.1777,0.590),
vector3(-0.0392+szp, 0.1764,0.595),
vector3(-0.0397+szp, 0.1741,0.60),

vector3(-0.0396+szp, 0.1570,0.62),
vector3(-0.0392+szp, 0.1547,0.63),
vector3(-0.0380+szp, 0.1525,0.64),
vector3(-0.0359+szp, 0.1493,0.65),
vector3(-0.0324+szp, 0.1462,0.66),
vector3(-0.0298+szp, 0.1452,0.67),

vector3(-0.0138+szp, 0.1421,0.68),
vector3(-0.0106+szp, 0.1407,0.69),
vector3(-0.0084+szp, 0.1386,0.695),
vector3(-0.0072+szp, 0.1361,0.70),
vector3(-0.0064+szp, 0.1327,0.705),

vector3(-0.0061+szp, 0.0256,0.88),
vector3(-0.0065+szp, 0.0229,0.89),
vector3(-0.0073+szp, 0.0211,0.90),
vector3(-0.0094+szp, 0.0186,0.91),
vector3(-0.0120+szp, 0.0171,0.92),
vector3(-0.0143+szp, 0.0166,0.93),

vector3(-0.0875+szp, 0.0085,0.975),
vector3(-0.0889+szp, 0.0080,0.98),
vector3(-0.0895+szp, 0.0067,0.985),
vector3(-0.0895+szp, 0.0016,0.99),
vector3(-0.0892+szp, 0.0006,0.995),
vector3(-0.0881+szp, 0.0000,1.00)

};



void TTrack::Compile()
{//przygotowanie trójk¹tów do wyœwielenia - model proceduralny
 GLuint  TextureID3;
	if (Global::bManageNodes)
	{
     if(DisplayListID) Release(); //zwolnienie zasobów

     if(Global::bManageNodes)
       {
        DisplayListID=glGenLists(1); //otwarcie nowej listy
        glNewList(DisplayListID,GL_COMPILE);
       };
	}

    glColor3f(1.0f,1.0f,1.0f);

    //McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
    GLfloat  ambientLight[4]  = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat  diffuseLight[4]  = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat  specularLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};

    switch (eEnvironment)
    {//modyfikacje oœwietlenia zale¿nie od œrodowiska
        case e_canyon: //wykop
            for(int li=0; li<3; li++)
            {
                ambientLight[li]= Global::ambientDayLight[li]*0.7;
                diffuseLight[li]= Global::diffuseDayLight[li]*0.3;
                specularLight[li]= Global::specularDayLight[li]*0.4;
            }
            glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
	        glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
        break;
        case e_tunnel: //tunel
            for(int li=0; li<3; li++)
            {
                ambientLight[li]= Global::ambientDayLight[li]*0.2;
                diffuseLight[li]= Global::diffuseDayLight[li]*0.1;
                specularLight[li]= Global::specularDayLight[li]*0.2;
            }
            glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
            glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
            glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
        break;
    }

    double fHTW=0.5*fabs(fTrackWidth);
    double side=fabs(fTexWidth); //szerokœæ podsypki na zewn¹trz szyny albo pobocza
    double rozp=fHTW+side+fabs(fTexSlope); //brzeg zewnêtrzny
    double fHTW2,side2,rozp2,fTexHeight2;
    if (iTrapezoid&2) //ten bit oznacza, ¿e istnieje odpowiednie pNext
    {//Ra: na tym siê lubi wieszaæ, ciekawe co za œmieci siê pod³¹czaj¹...
     fHTW2=0.5*fabs(pNext->fTrackWidth); //po³owa rozstawu/nawierzchni
     side2=fabs(pNext->fTexWidth);
     rozp2=fHTW2+side2+fabs(pNext->fTexSlope);
     fTexHeight2=pNext->fTexHeight;
     //zabezpieczenia przed zawieszeniem - logowaæ to?
     if (fHTW2>5.0*fHTW) {fHTW2=fHTW; WriteLog("niedopasowanie 1");};
     if (side2>5.0*side) {side2=side; WriteLog("niedopasowanie 2");};
     if (rozp2>5.0*rozp) {rozp2=rozp; WriteLog("niedopasowanie 3");};
     if (fabs(fTexHeight2)>5.0*fabs(fTexHeight)) {fTexHeight2=fTexHeight; WriteLog("niedopasowanie 4");};
    }
    else //gdy nie ma nastêpnego albo jest nieodpowiednim koñcem podpiêty
    {fHTW2=fHTW; side2=side; rozp2=rozp; fTexHeight2=fTexHeight;}
    double roll1,roll2;
    switch (iCategoryFlag)
    {
     case 1:   //tor
     {
      Segment->GetRolls(roll1,roll2);
      double sin1=sin(roll1),cos1=cos(roll1),sin2=sin(roll2),cos2=cos(roll2);
      // zwykla szyna: //Ra: czemu g³ówki s¹ asymetryczne na wysokoœci 0.140?
      vector3 rpts1[24],rpts2[24],rpts3[24],rpts4[24];

	  vector3 s49pts1[100], s49pts2[100];
	  vector3 s49Lpts1[68], s49Lpts2[68];
	  vector3 ri60pts1[128], ri60pts2[128];

	  // DLA XXX
      int i;
      for (i=0;i<12;++i)
      {
	   rpts1[i] = vector3((fHTW+szyna[i].x)*cos1+szyna[i].y*sin1, -(fHTW+szyna[i].x)*sin1+szyna[i].y*cos1, szyna[i].z);
       rpts2[11-i] = vector3((-fHTW-szyna[i].x)*cos1+szyna[i].y*sin1,-(-fHTW-szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z);
      }

	  // DLA S49
	  if (iRAILTYPE == 1) 
      for (i=0;i<34;++i)
      {
	   s49Lpts1[i] = vector3((fHTW+szynaS49L[i].x)*cos1+szynaS49L[i].y*sin1, -(fHTW+szynaS49L[i].x)*sin1+szynaS49L[i].y*cos1, szynaS49L[i].z);
       s49Lpts2[33-i] = vector3((-fHTW-szynaS49L[i].x)*cos1+szynaS49L[i].y*sin1,-(-fHTW-szynaS49L[i].x)*sin1+szynaS49L[i].y*cos1,szynaS49L[i].z);
      }

	  // DLA Ri60N
	  if (iRAILTYPE == 3) 
      for (i=0;i<64;++i)
      {
	   ri60pts1[i] = vector3((fHTW+szynaRI60N[i].x)*cos1+szynaRI60N[i].y*sin1, -(fHTW+szynaRI60N[i].x)*sin1+szynaRI60N[i].y*cos1, szynaRI60N[i].z);
       ri60pts2[63-i] = vector3((-fHTW-szynaRI60N[i].x)*cos1+szynaRI60N[i].y*sin1,-(-fHTW-szynaRI60N[i].x)*sin1+szynaRI60N[i].y*cos1,szynaRI60N[i].z);
      }

	  // DLA XXX
      if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
       for (i=0;i<12;++i)
       {rpts1[12+i]=vector3((fHTW2+szyna[i].x)*cos2+szyna[i].y*sin2,-(fHTW2+szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z);
        rpts2[23-i]=vector3((-fHTW2-szyna[i].x)*cos2+szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z);
       }

	  // DLA S49
	  if (iRAILTYPE == 1) 
      if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
       for (i=0;i<34;++i)
       {s49Lpts1[34+i]=vector3((fHTW2+szynaS49L[i].x)*cos2+szynaS49L[i].y*sin2,-(fHTW2+szynaS49L[i].x)*sin2+szynaS49L[i].y*cos2,szynaS49L[i].z);
        s49Lpts2[67-i]=vector3((-fHTW2-szynaS49L[i].x)*cos2+szynaS49L[i].y*sin2,-(-fHTW2-szynaS49L[i].x)*sin2+szynaS49L[i].y*cos2,szynaS49L[i].z);
       }

	  // DLA Ri60N
	  if (iRAILTYPE == 3) 
      if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
       for (i=0;i<64;++i)
       {
		ri60pts1[64+i]=vector3((fHTW2+szynaRI60N[i].x)*cos2+szynaRI60N[i].y*sin2,-(fHTW2+szynaRI60N[i].x)*sin2+szynaRI60N[i].y*cos2,szynaRI60N[i].z);
        ri60pts2[127-i]=vector3((-fHTW2-szynaRI60N[i].x)*cos2+szynaRI60N[i].y*sin2,-(-fHTW2-szynaRI60N[i].x)*sin2+szynaRI60N[i].y*cos2,szynaRI60N[i].z);
       }

      switch (eType) //dalej zale¿nie od typu
      {
       case tt_Turn: //obrotnica jak zwyk³y tor, tylko animacja dochodzi
        if (InMovement()) //jeœli siê krêci
        {//wyznaczamy wspó³rzêdne koñców, przy za³o¿eniu sta³ego œródka i d³ugoœci
         double hlen=0.5*SwitchExtension->Segments->GetLength(); //po³owa d³ugoœci
         //SwitchExtension->fOffset1=SwitchExtension->pAnim?SwitchExtension->pAnim->AngleGet():0.0; //pobranie k¹ta z modelu
//---         TAnimContainer *ac=SwitchExtension->pModel?SwitchExtension->pModel->GetContainer(NULL):NULL;
//---         if (ac)
//---          SwitchExtension->fOffset1=ac?180+ac->AngleGet():0.0; //pobranie k¹ta z modelu
         double sina=hlen*sin(DegToRad(SwitchExtension->fOffset1)),cosa=hlen*cos(DegToRad(SwitchExtension->fOffset1));
         vector3 middle=SwitchExtension->Segments->FastGetPoint(0.5);
         Segment->Init(middle+vector3(sina,0.0,cosa),middle-vector3(sina,0.0,cosa),5.0);
        }
       case tt_Normal:
        if (TextureID2)
        {//podsypka z podk³adami jest tylko dla zwyk³ego toru
         vector3 bpts1[8]; //punkty g³ównej p³aszczyzny nie przydaj¹ siê do robienia boków
         if (iTrapezoid) //trapez albo przechy³ki
         {//podsypka z podkladami trapezowata
          //ewentualnie poprawiæ mapowanie, ¿eby œrodek mapowa³ siê na 1.435/4.671 ((0.3464,0.6536)
          //bo siê tekstury podsypki rozje¿d¿aj¹ po zmianie proporcji profilu
          bpts1[0]=vector3(rozp,-fTexHeight,0.0); //lewy brzeg
          bpts1[1]=vector3((fHTW+side)*cos1,-(fHTW+side)*sin1,0.33); //krawêdŸ za³amania
          bpts1[2]=vector3(-bpts1[1].x,-bpts1[1].y,0.67); //prawy brzeg pocz¹tku symetrycznie
          bpts1[3]=vector3(-rozp,-fTexHeight,1.0); //prawy skos
          bpts1[4]=vector3(rozp2,-fTexHeight2,0.0); //lewy brzeg
          bpts1[5]=vector3((fHTW2+side2)*cos2,-(fHTW2+side2)*sin2,0.33); //krawêdŸ za³amania
          bpts1[6]=vector3(-bpts1[5].x,-bpts1[5].y,0.67); //prawy brzeg pocz¹tku symetrycznie
          bpts1[7]=vector3(-rozp2,-fTexHeight2,1.0); //prawy skos
         }
         else
         {
		  bpts1[0]=vector3(rozp,-fTexHeight,0.0); //lewy brzeg         [^^^]
          bpts1[1]=vector3(fHTW+side,0.0,0.33); //krawêdŸ za³amania     \ /
          bpts1[2]=vector3(-fHTW-side,0.0,0.67); //druga                } {
          bpts1[3]=vector3(-rozp,-fTexHeight,1.0); //prawy skos       _/   \_
         }
         glBindTexture(GL_TEXTURE_2D, TextureID2);
         Segment->RenderLoft(bpts1,iTrapezoid?-4:4,fTexLength, 0, Global::tracksegmentlen);
        }
        if (TextureID1)
        {// szyny
		//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
         glBindTexture(GL_TEXTURE_2D, TextureID1);

		 if (iRAILTYPE == 0) 
		     {
              Segment->RenderLoft(rpts1, iTrapezoid?-nnumPts:nnumPts, fTexLength, 0, Global::tracksegmentlen);
              Segment->RenderLoft(rpts2, iTrapezoid?-nnumPts:nnumPts, fTexLength, 0, Global::tracksegmentlen);
		     }
		 if (iRAILTYPE == 1) 
		     {
              Segment->RenderLoft(s49Lpts1, iTrapezoid?-S49LnnumPts:S49LnnumPts, fTexLength, 0, Global::tracksegmentlen);
              Segment->RenderLoft(s49Lpts2, iTrapezoid?-S49LnnumPts:S49LnnumPts, fTexLength, 0, Global::tracksegmentlen);
		     }
		 if (iRAILTYPE == 3) 
		     {
              Segment->RenderLoft(ri60pts1, iTrapezoid?-RI60NnnumPts:RI60NnnumPts, fTexLength, 0, Global::tracksegmentlen);
              Segment->RenderLoft(ri60pts2, iTrapezoid?-RI60NnnumPts:RI60NnnumPts, fTexLength, 0, Global::tracksegmentlen);
		     }
		 //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        }
        break;

// ROZJAZDY ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		
	   case tt_Switch: //dla zwrotnicy dwa razy szyny
        if (TextureID1)
        {//iglice liczone tylko dla zwrotnic


bool bswitchpod = true;
      // GENEROWANIE PODSYPEK POD ROZJAZDAMI
      if (bswitchpod)
      {
      double fHTW;
      double side;
      double slop;
      double rozp;
      double hypot1;
      int numPts = 4;

      vector3 bpts1[8]; //punkty g³ównej p³aszczyzny nie przydaj¹ siê do robienia boków
	  vector3 bpts2[8]; 

      if (pNext) TextureID3 = pNext->TextureID2;                                // POBIERANIE TEKSTURY DLA PODSYPKI ROZJAZDU
      if (pPrev) TextureID3 = pPrev->TextureID2;

      if (pNext) fHTW =0.5*fabs(pNext->fTrackWidth);                             //po³owa szerokoœci
      if (pNext) side =fabs(pNext->fTexWidth);                                   //szerokœæ podsypki na zewn¹trz szyny albo pobocza
      if (pNext) slop =fabs(pNext->fTexSlope);                                   //szerokoœæ pochylenia
      if (pNext) rozp =fHTW+side+slop;                                           //brzeg zewnêtrzny
      if (pNext) hypot1 =hypot(slop,fTexHeight);                                 //rozmiar pochylenia do liczenia normalnych

      if (pNext) fHTW =0.5*fabs(pNext->fTrackWidth);                             //po³owa szerokoœci
      if (pNext) side =fabs(pNext->fTexWidth);                                   //szerokœæ podsypki na zewn¹trz szyny albo pobocza
      if (pNext) slop =fabs(pNext->fTexSlope);                                   //szerokoœæ pochylenia
      if (pPrev) rozp =fHTW+side+slop;                                           //brzeg zewnêtrzny
      if (pPrev) hypot1 =hypot(slop,fTexHeight);                                 //rozmiar pochylenia do liczenia normalnych

      if (hypot1==0.0) hypot1=1.0;
      vector3 normal1=vector3(fTexSlope/hypot1,fTexHeight/hypot1,0.0);          //wektor normalny

      if ((pNext) || (pPrev))
         {
		  bpts1[0]=vector3(rozp,-fTexHeight-0.01, 0.0); //lewy brzeg       
          bpts1[1]=vector3(fHTW+side,0.0-0.01, 0.33); //krawêdŸ za³amania    
          bpts1[2]=vector3(-fHTW-side,0.0-0.01, 0.67); //druga              
          bpts1[3]=vector3(-rozp,-fTexHeight-0.01, 1.0); //prawy skos       

		  bpts2[0]=vector3(rozp,-fTexHeight,0.0); //lewy brzeg         
          bpts2[1]=vector3(fHTW+side,0.0,0.33); //krawêdŸ za³amania    
          bpts2[2]=vector3(-fHTW-side,0.0,0.67); //druga                
          bpts2[3]=vector3(-rozp,-fTexHeight,1.0); //prawy skos      

          glBindTexture(GL_TEXTURE_2D, TextureID3);
          SwitchExtension->Segments[0].RenderLoft(bpts1, iTrapezoid?-4:4, fTexLength, 0, 3);
          SwitchExtension->Segments[1].RenderLoft(bpts2, iTrapezoid?-4:4, fTexLength, 0, 3);

         }
      }
	
//^KONIEC RYSOWANIA PODSYPEK


         // XXX
         vector3 rpts3[24],rpts4[24];
		 vector3 S49Lpts3[68], S49Lpts4[68];
		 vector3 ri60pts3[128], ri60pts4[128];
         for (i=0;i<12;++i)
         {rpts3[i]    = vector3((fHTW+iglica[i].x)*cos1+iglica[i].y*sin1,-(fHTW+iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
          rpts4[11-i] = vector3((-fHTW-iglica[i].x)*cos1+iglica[i].y*sin1,-(-fHTW-iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
         }

		 // S49
		 if (iRAILTYPE == 1) 
         for (i=0;i<34;++i)
         {S49Lpts3[i]   = vector3((fHTW+iglicaS49L[i].x)*cos1+iglicaS49L[i].y*sin1,-(fHTW+iglicaS49L[i].x)*sin1+iglicaS49L[i].y*cos1,iglicaS49L[i].z);
          S49Lpts4[33-i]= vector3((-fHTW-iglicaS49L[i].x)*cos1+iglicaS49L[i].y*sin1,-(-fHTW-iglicaS49L[i].x)*sin1+iglicaS49L[i].y*cos1,iglicaS49L[i].z);
         }

		 // Ri60N
		 if (iRAILTYPE == 3) 
         for (i=0;i<64;++i)
         {ri60pts3[i]   = vector3((fHTW+iglicaRI60N[i].x)*cos1+iglicaRI60N[i].y*sin1,-(fHTW+iglicaRI60N[i].x)*sin1+iglicaRI60N[i].y*cos1,iglicaRI60N[i].z);
          ri60pts4[63-i]= vector3((-fHTW-iglicaRI60N[i].x)*cos1+iglicaRI60N[i].y*sin1,-(-fHTW-iglicaRI60N[i].x)*sin1+iglicaRI60N[i].y*cos1,iglicaRI60N[i].z);
         }

		 // XXX
         if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
          for (i=0;i<12;++i)
          {rpts3[12+i] = vector3((fHTW2+iglica[i].x)*cos2+iglica[i].y*sin2,-(fHTW2+iglica[i].x)*sin2+iglica[i].y*cos2,iglica[i].z);
           rpts4[23-i] = vector3((-fHTW2-iglica[i].x)*cos2+iglica[i].y*sin2,-(-fHTW2-iglica[i].x)*sin2+iglica[i].y*cos2,iglica[i].z);
          }

		 // S49
		 if (iRAILTYPE == 1) 
         if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
          for (i=0;i<34;++i)
          {S49Lpts3[34+i]=vector3((fHTW2+iglica[i].x)*cos2+iglica[i].y*sin2,-(fHTW2+iglica[i].x)*sin2+iglica[i].y*cos2,iglica[i].z);
           S49Lpts4[67-i]=vector3((-fHTW2-iglica[i].x)*cos2+iglica[i].y*sin2,-(-fHTW2-iglica[i].x)*sin2+iglica[i].y*cos2,iglica[i].z);
          }

		 // S49
		 if (iRAILTYPE == 3) 
         if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
          for (i=0;i<64;++i)
          {ri60pts3[64+i]=vector3((fHTW2+iglicaRI60N[i].x)*cos2+iglicaRI60N[i].y*sin2,-(fHTW2+iglicaRI60N[i].x)*sin2+iglicaRI60N[i].y*cos2,iglicaRI60N[i].z);
           ri60pts4[127-i]=vector3((-fHTW2-iglicaRI60N[i].x)*cos2+iglicaRI60N[i].y*sin2,-(-fHTW2-iglicaRI60N[i].x)*sin2+iglicaRI60N[i].y*cos2,iglicaRI60N[i].z);
          }

         if (InMovement())
         {//Ra: trochê bez sensu, ¿e tu jest animacja
          double v=SwitchExtension->fDesiredOffset1-SwitchExtension->fOffset1;
          SwitchExtension->fOffset1+=sign(v)*Timer::GetDeltaTime()*0.1;
          //Ra: trzeba daæ to do klasy...
          if (SwitchExtension->fOffset1<=0.00)
          {SwitchExtension->fOffset1; //1cm?
           SwitchExtension->bMovement=false; //koniec animacji
          }
          if (SwitchExtension->fOffset1>=fMaxOffset)
          {SwitchExtension->fOffset1=fMaxOffset; //maksimum-1cm?
           SwitchExtension->bMovement=false; //koniec animacji
          }
         }
//McZapkie-130302 - poprawione rysowanie szyn
/* //stara wersja - dziwne prawe zwrotnice
         glBindTexture(GL_TEXTURE_2D, TextureID1);
         SwitchExtension->Segments[0].RenderLoft(rpts1,nnumPts,fTexLength); //lewa szyna normalna ca³a
         SwitchExtension->Segments[0].RenderLoft(rpts2,nnumPts,fTexLength,2); //prawa szyna za iglic¹
         SwitchExtension->Segments[0].RenderSwitchRail(rpts2,rpts4,nnumPts,fTexLength,2,-SwitchExtension->fOffset1); //prawa iglica
         glBindTexture(GL_TEXTURE_2D, TextureID2);
         SwitchExtension->Segments[1].RenderLoft(rpts1,nnumPts,fTexLength,2); //lewa szyna za iglic¹
         SwitchExtension->Segments[1].RenderSwitchRail(rpts1,rpts3,nnumPts,fTexLength,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
         SwitchExtension->Segments[1].RenderLoft(rpts2,nnumPts,fTexLength); //prawa szyna normalnie ca³a
*/
         if (SwitchExtension->RightSwitch)
         {//nowa wersja z SPKS, ale odwrotnie lewa/prawa
          glBindTexture(GL_TEXTURE_2D, TextureID1);
		  //xxx
		  if (iRAILTYPE == 0) 
		  {
          SwitchExtension->Segments[0].RenderLoft(rpts1,nnumPts,fTexLength,2);
          SwitchExtension->Segments[0].RenderSwitchRail(rpts1,rpts3,nnumPts,fTexLength,2,SwitchExtension->fOffset1);
          SwitchExtension->Segments[0].RenderLoft(rpts2,nnumPts,fTexLength);
		  }
          //S49
		  if (iRAILTYPE == 1) 
		  {
          SwitchExtension->Segments[0].RenderLoft(s49Lpts1,S49LnnumPts,fTexLength,2);
          SwitchExtension->Segments[0].RenderSwitchRail(s49Lpts1,S49Lpts3,S49LnnumPts,fTexLength,2,SwitchExtension->fOffset1);
          SwitchExtension->Segments[0].RenderLoft(s49Lpts2,S49LnnumPts,fTexLength);
		  }
          //Ri60N
		  if (iRAILTYPE == 3) 
		  {
          SwitchExtension->Segments[0].RenderLoft(ri60pts1,RI60NnnumPts,fTexLength,1);
          SwitchExtension->Segments[0].RenderSwitchRail(ri60pts1,ri60pts3,RI60NnnumPts,fTexLength,1,SwitchExtension->fOffset1);
          SwitchExtension->Segments[0].RenderLoft(ri60pts2,RI60NnnumPts,fTexLength);
		  }

          glBindTexture(GL_TEXTURE_2D, TextureID2);
		  //xxx
		  if (iRAILTYPE == 0) 
		  {
          SwitchExtension->Segments[1].RenderLoft(rpts1,nnumPts,fTexLength);
          SwitchExtension->Segments[1].RenderLoft(rpts2,nnumPts,fTexLength,2);
          SwitchExtension->Segments[1].RenderSwitchRail(rpts2,rpts4,nnumPts,fTexLength,2,-fMaxOffset+SwitchExtension->fOffset1);
		  }
          //S49
		  if (iRAILTYPE == 1) 
		  {
          SwitchExtension->Segments[1].RenderLoft(s49Lpts1,S49LnnumPts,fTexLength);
          SwitchExtension->Segments[1].RenderLoft(s49Lpts2,S49LnnumPts,fTexLength,2);
          SwitchExtension->Segments[1].RenderSwitchRail(s49Lpts2,S49Lpts4,S49LnnumPts,fTexLength,2,-fMaxOffset+SwitchExtension->fOffset1);
		  }
          //Ri60N
		  if (iRAILTYPE == 3) 
		  {
          SwitchExtension->Segments[1].RenderLoft(ri60pts1,RI60NnnumPts,fTexLength);
          SwitchExtension->Segments[1].RenderLoft(ri60pts2,RI60NnnumPts,fTexLength,1);
          SwitchExtension->Segments[1].RenderSwitchRail(ri60pts2,ri60pts4,RI60NnnumPts,fTexLength,1,-fMaxOffset+SwitchExtension->fOffset1);
		  }
          //WriteLog("Kompilacja prawej"); WriteLog(AnsiString(SwitchExtension->fOffset1).c_str());
         }
         else
         {//lewa dzia³a lepiej ni¿ prawa
 		  if (iRAILTYPE == 0) 
		  {
          glBindTexture(GL_TEXTURE_2D, TextureID1);
          SwitchExtension->Segments[0].RenderLoft(rpts1,nnumPts,fTexLength); //lewa szyna normalna ca³a
          SwitchExtension->Segments[0].RenderLoft(rpts2,nnumPts,fTexLength,2); //prawa szyna za iglic¹
          SwitchExtension->Segments[0].RenderSwitchRail(rpts2,rpts4,nnumPts,fTexLength,2,-SwitchExtension->fOffset1); //prawa iglica
          glBindTexture(GL_TEXTURE_2D, TextureID2);
          SwitchExtension->Segments[1].RenderLoft(rpts1,nnumPts,fTexLength,2); //lewa szyna za iglic¹
          SwitchExtension->Segments[1].RenderSwitchRail(rpts1,rpts3,nnumPts,fTexLength,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
          SwitchExtension->Segments[1].RenderLoft(rpts2,nnumPts,fTexLength); //prawa szyna normalnie ca³a
		  }

		  // S49
		  if (iRAILTYPE == 1) 
		  {
          glBindTexture(GL_TEXTURE_2D, TextureID1);
          SwitchExtension->Segments[0].RenderLoft(s49Lpts1,S49LnnumPts,fTexLength); //lewa szyna normalna ca³a
          SwitchExtension->Segments[0].RenderLoft(s49Lpts2,S49LnnumPts,fTexLength,2); //prawa szyna za iglic¹
          SwitchExtension->Segments[0].RenderSwitchRail(s49Lpts2,S49Lpts4,S49LnnumPts,fTexLength,2,-SwitchExtension->fOffset1); //prawa iglica
          glBindTexture(GL_TEXTURE_2D, TextureID2);
          SwitchExtension->Segments[1].RenderLoft(s49Lpts1,S49LnnumPts,fTexLength,2); //lewa szyna za iglic¹
          SwitchExtension->Segments[1].RenderSwitchRail(s49Lpts1,S49Lpts3,S49LnnumPts,fTexLength,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
          SwitchExtension->Segments[1].RenderLoft(s49Lpts2,S49LnnumPts,fTexLength); //prawa szyna normalnie ca³a
		  }

		  // ri60
		  if (iRAILTYPE == 3) 
		  {
          glBindTexture(GL_TEXTURE_2D, TextureID1);
          SwitchExtension->Segments[0].RenderLoft(ri60pts1, RI60NnnumPts, fTexLength); //lewa szyna normalna ca³a
          SwitchExtension->Segments[0].RenderLoft(ri60pts2, RI60NnnumPts, fTexLength,2); //prawa szyna za iglic¹
          SwitchExtension->Segments[0].RenderSwitchRail(ri60pts2, ri60pts4, RI60NnnumPts,fTexLength,2,-SwitchExtension->fOffset1); //prawa iglica
          glBindTexture(GL_TEXTURE_2D, TextureID2);
          SwitchExtension->Segments[1].RenderLoft(ri60pts1, RI60NnnumPts, fTexLength,2); //lewa szyna za iglic¹
          SwitchExtension->Segments[1].RenderSwitchRail(ri60pts1, ri60pts3, RI60NnnumPts, fTexLength,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
          SwitchExtension->Segments[1].RenderLoft(ri60pts2, RI60NnnumPts, fTexLength); //prawa szyna normalnie ca³a
		  }
          //WriteLog("Kompilacja lewej"); WriteLog(AnsiString(SwitchExtension->fOffset1).c_str());
         }
        }
        break;
      }
     } //koniec obs³ugi torów
     break;
     case 2:   //McZapkie-260302 - droga - rendering
//McZapkie:240702-zmieniony zakres widzialnosci
     {vector3 bpts1[4]; //punkty g³ównej p³aszczyzny przydaj¹ siê do robienia boków
      if (TextureID1||TextureID2) //punkty siê przydadz¹, nawet jeœli nawierzchni nie ma
      {//double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
       double max=fTexLength; //test: szerokoœæ proporcjonalna do d³ugoœci
       double map1=max>0.0?fHTW/max:0.5; //obciêcie tekstury od strony 1
       double map2=max>0.0?fHTW2/max:0.5; //obciêcie tekstury od strony 2
       if (iTrapezoid) //trapez albo przechy³ki
       {//nawierzchnia trapezowata
        Segment->GetRolls(roll1,roll2);
        bpts1[0]=vector3(fHTW*cos(roll1),-fHTW*sin(roll1),0.5-map1); //lewy brzeg pocz¹tku
        bpts1[1]=vector3(-bpts1[0].x,-bpts1[0].y,0.5+map1); //prawy brzeg pocz¹tku symetrycznie
        bpts1[2]=vector3(fHTW2*cos(roll2),-fHTW2*sin(roll2),0.5-map2); //lewy brzeg koñca
        bpts1[3]=vector3(-bpts1[2].x,-bpts1[2].y,0.5+map2); //prawy brzeg pocz¹tku symetrycznie
       }
       else
       {bpts1[0]=vector3( fHTW,0.0,0.5-map1); //zawsze standardowe mapowanie
        bpts1[1]=vector3(-fHTW,0.0,0.5+map1);
       }
      }
      if (TextureID1) //jeœli podana by³a tekstura, generujemy trójk¹ty
      {//tworzenie trójk¹tów nawierzchni szosy
       glBindTexture(GL_TEXTURE_2D, TextureID1);
       Segment->RenderLoft(bpts1,iTrapezoid?-2:2,fTexLength);
      }
      if (TextureID2)
      {//pobocze drogi - poziome przy przechy³ce (a mo¿e krawê¿nik i chodnik zrobiæ jak w Midtown Madness 2?)
       glBindTexture(GL_TEXTURE_2D, TextureID2);
       vector3 rpts1[6],rpts2[6]; //wspó³rzêdne przekroju i mapowania dla prawej i lewej strony
       rpts1[0]=vector3(rozp,-fTexHeight,0.0); //lewy brzeg podstawy
       rpts1[1]=vector3(bpts1[0].x+side,bpts1[0].y,0.5), //lewa krawêdŸ za³amania
       rpts1[2]=vector3(bpts1[0].x,bpts1[0].y,1.0); //lewy brzeg pobocza (mapowanie mo¿e byæ inne
       rpts2[0]=vector3(bpts1[1].x,bpts1[1].y,1.0); //prawy brzeg pobocza
       rpts2[1]=vector3(bpts1[1].x-side,bpts1[1].y,0.5); //prawa krawêdŸ za³amania
       rpts2[2]=vector3(-rozp,-fTexHeight,0.0); //prawy brzeg podstawy
       if (iTrapezoid) //trapez albo przechy³ki
       {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
        rpts1[3]=vector3(rozp2,-fTexHeight2,0.0); //lewy brzeg lewego pobocza
        rpts1[4]=vector3(bpts1[2].x+side2,bpts1[2].y,0.5); //krawêdŸ za³amania
        rpts1[5]=vector3(bpts1[2].x,bpts1[2].y,1.0); //brzeg pobocza
        rpts2[3]=vector3(bpts1[3].x,bpts1[3].y,1.0);
        rpts2[4]=vector3(bpts1[3].x-side2,bpts1[3].y,0.5);
        rpts2[5]=vector3(-rozp2,-fTexHeight2,0.0); //prawy brzeg prawego pobocza
        Segment->RenderLoft(rpts1,-3,fTexLength);
        Segment->RenderLoft(rpts2,-3,fTexLength);
       }
       else
       {//pobocza zwyk³e, brak przechy³ki
        Segment->RenderLoft(rpts1,3,fTexLength);
        Segment->RenderLoft(rpts2,3,fTexLength);
       }
      }
     }
     break;
     case 4:   //McZapkie-260302 - rzeka- rendering
      //Ra: rzeki na razie bez zmian, przechy³ki na pewno nie maj¹
         vector3 bpts1[numPts]= { vector3(fHTW,0.0,0.0),vector3(fHTW,0.2,0.33),
                                vector3(-fHTW,0.0,0.67),vector3(-fHTW,0.0,1.0) };
         //Ra: dziwnie ten kszta³t wygl¹da
         if(TextureID1)
         {
             glBindTexture(GL_TEXTURE_2D, TextureID1);
             Segment->RenderLoft(bpts1,numPts,fTexLength);
             if(TextureID2)
             {//brzegi rzeki prawie jak pobocze derogi, tylko inny znak ma wysokoœæ
                 vector3 rpts1[3]= { vector3(rozp,fTexHeight,0.0),
                                     vector3(fHTW+side,0.0,0.5),
                                     vector3(fHTW,0.0,1.0) };
                 vector3 rpts2[3]= { vector3(-fHTW,0.0,1.0),
                                     vector3(-fHTW-side,0.0,0.5),
                                     vector3(-rozp,fTexHeight,0.1) }; //Ra: po kiego 0.1?
                 glBindTexture(GL_TEXTURE_2D, TextureID2);      //brzeg rzeki
                 Segment->RenderLoft(rpts1,3,fTexLength);
                 Segment->RenderLoft(rpts2,3,fTexLength);
             }
         }
         break;
    }

    if (Global::bManageNodes)        glEndList();

};

/*
 void solidCone(GLdouble base, GLdouble height, GLint slices, GLint stacks)

 {
 GLUquadricObj *qo;
 qo = gluNewQuadric();


 gluQuadricDrawStyle(qo,GLU_FILL);
 gluQuadricNormals(qo,GLU_SMOOTH);
 gluQuadricOrientation(qo,GLU_OUTSIDE);
 gluCylinder(qo,0.2,0.02,0.4,6,1);

 gluDeleteQuadric(qo); 

 }
*/
void TTrack::Release()
{
    glDeleteLists(DisplayListID,1);
    DisplayListID=0;
};

bool  TTrack::Render()
{
	//Global::bManageNodes = false;
    if(bVisible && SquareMagnitude(Global::pCameraPosition-Segment->FastGetPoint(0.5)) < 50000)
    {
	//	glEnable(GL_DEPTH_TEST);
		if (!Global::bManageNodes) Compile(); // NORMALNE RENDEROWANIE BEZ LISTY

		if (Global::bManageNodes)
        if(!DisplayListID)
        {
            Compile();
            ResourceManager::Register(this);
        };
		
        //SetLastUsage(Timer::GetSimulationTime());
        if (Global::bManageNodes) glCallList(DisplayListID);
        if (Global::bManageNodes) if (InMovement()) Release(); //zwrotnica w trakcie animacji do odrysowania
    };

    for (int i=0; i<iNumDynamics; i++)
    {
      //-- Dynamics[i]->Render();
    }

 glLightfv(GL_LIGHT0, GL_AMBIENT, Global::ambientDayLight);
 glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight);
 glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight);

 /*
if (Global::trackmarkers)
if(bVisible && SquareMagnitude(Global::pCameraPosition-Segment->FastGetPoint(0.5)) < 40000)
{

              vector3 pos1,pos2,pos3;
              pos1= Segment->FastGetPoint_0();
              pos2= Segment->FastGetPoint(0.5);
              pos3= Segment->FastGetPoint_1();
              //glDisable(GL_DEPTH_TEST);


             // glColor3f(1.0,0.9,0.0); 
			 // DRAWSPHERE4 (pos2.x, pos2.y, pos2.z, 0.7, Global::fonttexturex);
			 // drawSphere3(pos2.x, pos2.y, pos2.z, 0.5, 14);

			 // glColor3ub(255,255,0);
             // DRAWHLINE(pos2.x, pos2.y, pos2.z, 4.5, 0);
             
//DRAWSPHERE4 (pos2.x, pos2.y, pos2.z, 0.2, 6);

             drawSphere3(pos2.x, pos2.y, pos2.z, 0.1, 6, 1); // CENTRALNY PUNKT ODCINKA TORU

             char szlabel[100];
             sprintf(szlabel, "name: %s L=%1.2f", asName.c_str(), fTrackLength);
             print3Dlabel(pos2.x, pos2.y+1, pos2.z, 0.007, szlabel , 1);	// FLAT TEXT IN 3D SPACE
           
			 //glDisable (GL_DEPTH_TEST);
			 glShadeModel(GL_FLAT);
			  Segment->Render();
              glEnable(GL_LIGHTING);
              glDisable(GL_BLEND);


}
*/
   ScannedFlag=false;


   //WriteLog("END TTrack::Render()");
   return true;

}

bool  TTrack::RenderAlpha()
{
    glColor3f(1.0f,1.0f,1.0f);
//McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
    GLfloat  ambientLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
    GLfloat  diffuseLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
    GLfloat  specularLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
    switch (eEnvironment)
    {
     case e_canyon:
      {
        for (int li=0; li<3; li++)
         {
           ambientLight[li]= Global::ambientDayLight[li]*0.8;
           diffuseLight[li]= Global::diffuseDayLight[li]*0.4;
           specularLight[li]= Global::specularDayLight[li]*0.5;
         }
        glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
	    glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
      }
     break;
     case e_tunnel:
      {
        for (int li=0; li<3; li++)
         {
           ambientLight[li]= Global::ambientDayLight[li]*0.2;
           diffuseLight[li]= Global::diffuseDayLight[li]*0.1;
           specularLight[li]= Global::specularDayLight[li]*0.2;
         }
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
	glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
      }
     break;
    }

    for (int i=0; i<iNumDynamics; i++)
    {
        //if(SquareMagnitude(Global::pCameraPosition-Dynamics[i]->GetPosition())<20000)
      //--  Dynamics[i]->RenderAlpha();
    }
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
   return true;
}

bool  TTrack::CheckDynamicObject(TDynamicObject *Dynamic)
{
    for (int i=0; i<iNumDynamics; i++)
        if (Dynamic==Dynamics[i])
            return true;
    return false;
};

bool  TTrack::RemoveDynamicObject(TDynamicObject *Dynamic)
{
    for (int i=0; i<iNumDynamics; i++)
    {
        if (Dynamic==Dynamics[i])
        {
            iNumDynamics--;
            for (i; i<iNumDynamics; i++)
                Dynamics[i]= Dynamics[i+1];
            return true;

        }
    }
    //Error("Cannot remove dynamic from track");
	MessageBox(0,"Cannot remove dynamic from track", "Error", MB_OK);
    return false;
}

bool  TTrack::InMovement()
{//tory animowane (zwrotnica, obrotnica) maj¹ SwitchExtension
 if (SwitchExtension)
 {if (eType==tt_Switch)
   return SwitchExtension->bMovement; //ze zwrotnic¹ ³atwiej
  if (eType==tt_Turn)
   if (SwitchExtension->pModel)
   {if (!SwitchExtension->CurrentIndex) return false; //0=zablokowana siê nie animuje
    //trzeba ka¿dorazowo porównywaæ z k¹tem modelu
    //----TAnimContainer *ac=SwitchExtension->pModel?SwitchExtension->pModel->GetContainer(NULL):NULL;
    //----return ac?(ac->AngleGet()!=SwitchExtension->fOffset1):false;
    //return true; //jeœli jest taki obiekt
   }
 }
 return false;
};
void  TTrack::Assign(TGroundNode *gn,TAnimContainer *ac)
{//Ra: wi¹zanie toru z modelem obrotnicy
 //if (eType==tt_Turn) SwitchExtension->pAnim=p;
};
void  TTrack::Assign(TGroundNode *gn,TAnimModel *am)
{//Ra: wi¹zanie toru z modelem obrotnicy
 if (eType==tt_Turn)
 {SwitchExtension->pModel=am;
  SwitchExtension->pMyNode=gn;
 }
};

bool  TTrack::Switch(int i)
{
    if (SwitchExtension)
     if (eType==tt_Switch)
     {//przek³adanie zwrotnicy jak zwykle
        SwitchExtension->fDesiredOffset1= fMaxOffset*double(NextMask[i]); //od punktu 1
        //SwitchExtension->fDesiredOffset2= fMaxOffset*double(PrevMask[i]); //od punktu 2
        SwitchExtension->CurrentIndex= i;
        Segment= SwitchExtension->Segments+i; //wybranie aktywnej drogi
        pNext= SwitchExtension->pNexts[NextMask[i]]; //prze³¹czenie koñców
        pPrev= SwitchExtension->pPrevs[PrevMask[i]];
        bNextSwitchDirection= SwitchExtension->bNextSwitchDirection[NextMask[i]];
        bPrevSwitchDirection= SwitchExtension->bPrevSwitchDirection[PrevMask[i]];
        fRadius= fRadiusTable[i]; //McZapkie: wybor promienia toru
        if (DisplayListID) //jeœli istnieje siatka renderu
         SwitchExtension->bMovement=true; //bêdzie animacja
        else
         SwitchExtension->fOffset1=SwitchExtension->fDesiredOffset1; //nie ma siê co bawiæ
        return true;
     }
     else
     {//blokowanie (0, szuka torów) lub odblokowanie (1, roz³¹cza) obrotnicy
      SwitchExtension->CurrentIndex=i; //zapamiêtanie stanu zablokowania
      if (i)
      {//roz³¹czenie obrotnicy od s¹siednich torów
       if (pPrev)
        if (bPrevSwitchDirection)
         pPrev->pPrev=NULL;
        else
         pPrev->pNext=NULL;
       if (pNext)
        if (bPrevSwitchDirection)
         pNext->pNext=NULL;
        else
         pNext->pPrev=NULL;
       pNext=pPrev=NULL;
       fVelocity=0.0; //AI, nie ruszaj siê!
      }
      else
      {//zablokowanie pozycji i po³¹czenie do s¹siednich torów
      //-- Global::pGround->TrackJoin(SwitchExtension->pMyNode);
       if (pNext||pPrev)
        fVelocity=6.0; //jazda dozwolona
      }
      return true;
     }
    //Error("Cannot switch normal track");
	MessageBox(0,"Cannot switch normal track","Error",MB_OK);

    return false;
};


double  TTrack::WidthTotal()
{//szerokoœæ z poboczem
 if (iCategoryFlag&2) //jesli droga
  if (fTexHeight>=0.0) //i ma boki zagiête w dó³ (chodnik jest w górê)
   return 2.0*fabs(fTexWidth)+0.5*fabs(fTrackWidth+fTrackWidth2); //dodajemy pobocze
 return 0.5*fabs(fTrackWidth+fTrackWidth2); //a tak tylko zwyk³a œrednia szerokoœæ
};

//---------------------------------------------------------------------------

#pragma package(smart_init)
