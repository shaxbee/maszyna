// TRKFOLL.CPP 283

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

//#include    "system.hpp"
//#include    "classes.hpp"


#pragma hdrstop
#include "commons.h"
#include "commons_usr.h"
#include "segment.h" 
#include "trackfollower.h"
#include "Globals.h"

#define M_PI 3.14

  void draw_axle_symbol(double r, int lats, int longs) {
      int i, j;
      for(i = 0; i <= lats; i++) {
          double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
          double z0  = sin(lat0);
          double zr0 =  cos(lat0);
 
          double lat1 = M_PI * (-0.5 + (double) i / lats);
          double z1 = sin(lat1);
          double zr1 = cos(lat1);

		  glColor4f(0.9,0.2,0.2,0.8);
		  glPushMatrix();
          glScalef(0.3,0.3,0.3);
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
		  glPopMatrix();
		  glFrontFace(GL_CCW);
      }
  }


TTrackFollower::TTrackFollower()
{
    pCurrentTrack= NULL;
    pCurrentSegment= NULL;
    fCurrentDistance= 0;
    pPosition= vector3(0,0,0);
    fDirection= 1;
}

TTrackFollower::~TTrackFollower()
{
}

bool TTrackFollower::Init(TTrack *pTrack, TDynamicObject *NewOwner, double fDir)
{
    fDirection= fDir;
    Owner= NewOwner;
    SetCurrentTrack(pTrack);
    iEventFlag=0;
    iEventallFlag=0;    
    if ((pCurrentSegment))// && (pCurrentSegment->GetLength()<fFirstDistance))
        return false;

    return true;

}

bool TTrackFollower::Move(double fDistance, bool bPrimary)
{
    fDistance*= fDirection;
    double s;
    bool bCanSkip;
    while (true)
    {
		/*
        if (fDistance<0)
         {
          if (Owner->MoverParameters->CabNo!=0) //McZapkie-280503: wyzwalanie event tylko dla pojazdow z obsada
           if (TestFlag(iEventFlag,1))
            if (iSetFlag(iEventFlag,-1))
              if (bPrimary && pCurrentTrack->Event1 && pCurrentTrack->Event1->fStartTime<=0)
               {
                //--  Global::pGround->AddToQuery(pCurrentTrack->Event1,Owner);
               }
          if (TestFlag(iEventallFlag,1))        //McZapkie-280503: wyzwalanie eventall dla wszystkich pojazdow
            if (iSetFlag(iEventallFlag,-1))
              if (bPrimary && pCurrentTrack->Eventall1 && pCurrentTrack->Eventall1->fStartTime<=0)
               {
                  Global::pGround->AddToQuery(pCurrentTrack->Eventall1,Owner);
               }
         }
        if (fDistance>0)
         {
          if (Owner->MoverParameters->CabNo!=0)
           if (TestFlag(iEventFlag,2))
            if (iSetFlag(iEventFlag,-2))
              if (bPrimary && pCurrentTrack->Event2 && pCurrentTrack->Event2->fStartTime<=0)
               {
                  Global::pGround->AddToQuery(pCurrentTrack->Event2,Owner);
               }
          if (TestFlag(iEventallFlag,2))
            if (iSetFlag(iEventallFlag,-2))
              if (bPrimary && pCurrentTrack->Eventall2 && pCurrentTrack->Eventall2->fStartTime<=0)
               {
                  Global::pGround->AddToQuery(pCurrentTrack->Eventall2,Owner);
               }
         }
        if (fDistance==0) //McZapkie-140602: wyzwalanie zdarzenia gdy pojazd stoi
         {
          if (Owner->MoverParameters->CabNo!=0)
           if (pCurrentTrack->Event0)
            if (pCurrentTrack->Event0->fStartTime<=0 && pCurrentTrack->Event0->fDelay!=0)
               {
                  Global::pGround->AddToQuery(pCurrentTrack->Event0,Owner);
               }
          if (pCurrentTrack->Eventall0)
            if (pCurrentTrack->Eventall0->fStartTime<=0 && pCurrentTrack->Eventall0->fDelay!=0)
               {
                  Global::pGround->AddToQuery(pCurrentTrack->Eventall0,Owner);
               }
         }
		*/
        if (!pCurrentSegment)
            return false;
        s= fCurrentDistance+fDistance;
        (pCurrentTrack->eType);
        if (s<0)
        {
            bCanSkip= bPrimary & pCurrentTrack->CheckDynamicObject(Owner);
            if (bCanSkip)
                pCurrentTrack->RemoveDynamicObject(Owner);
            if (pCurrentTrack->bPrevSwitchDirection)
            {
                SetCurrentTrack(pCurrentTrack->CurrentPrev());
                fCurrentDistance= 0;
                fDistance= -s;
                fDirection= -fDirection;
                if (pCurrentTrack==NULL)
                {
                    //--Error(Owner->MoverParameters->Name+" at NULL track");
                    return false;
                }
            }
            else
            {
                SetCurrentTrack(pCurrentTrack->CurrentPrev());
                if (pCurrentTrack==NULL)
                {
                    //--Error(Owner->MoverParameters->Name+" at NULL track");
                    return false;
                }
                fCurrentDistance= pCurrentSegment->GetLength();
                fDistance= s;
            }
            if (bCanSkip)
             {
                pCurrentTrack->AddDynamicObject(Owner);
                iEventFlag= 3; //McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
                iEventallFlag= 3; //McZapkie-280503: jw, dla eventall1,2
             }
//event1 przesuniete na gore
            continue;
        }
        else
        if (s>pCurrentSegment->GetLength())
        {
            bCanSkip= bPrimary & pCurrentTrack->CheckDynamicObject(Owner);
            if (bCanSkip)
                pCurrentTrack->RemoveDynamicObject(Owner);
            if (pCurrentTrack->bNextSwitchDirection)
            {
                fDistance= -(s-pCurrentSegment->GetLength());
                SetCurrentTrack(pCurrentTrack->CurrentNext());
                if (pCurrentTrack==NULL)
                {
                    //--Error(Owner->MoverParameters->Name+" at NULL track");
                    return false;
                }
                fCurrentDistance= pCurrentSegment->GetLength();
                fDirection= -fDirection;
            }
            else
            {
                fDistance= s-pCurrentSegment->GetLength();
                SetCurrentTrack(pCurrentTrack->CurrentNext());
                fCurrentDistance= 0;
                if (pCurrentTrack==NULL)
                {
                    //--Error(Owner->MoverParameters->Name+" at NULL track");
                    return false;
                }
            }
            if (bCanSkip)
             {
                pCurrentTrack->AddDynamicObject(Owner);
                iEventFlag= 3; //McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
                iEventallFlag= 3;
             }
//event2 przesuniete na gore
            continue;
        }
        else
        {
            if (bPrimary)
            {
                if (Owner->MoverParameters->CabNo!=0)
                 {
                  //if (pCurrentTrack->Event1 && pCurrentTrack->Event1->fDelay<=-1.0f)
                  //  Global::pGround->AddToQuery(pCurrentTrack->Event1,Owner);
                  //if (pCurrentTrack->Event2 && pCurrentTrack->Event2->fDelay<=-1.0f)
                  //  Global::pGround->AddToQuery(pCurrentTrack->Event2,Owner);
                 }
                //if (pCurrentTrack->Eventall1 && pCurrentTrack->Eventall1->fDelay<=-1.0f)
                //    Global::pGround->AddToQuery(pCurrentTrack->Eventall1,Owner);
                //if (pCurrentTrack->Eventall2 && pCurrentTrack->Eventall2->fDelay<=-1.0f)
                //    Global::pGround->AddToQuery(pCurrentTrack->Eventall2,Owner);
            }
            fCurrentDistance= s;
            fDistance= 0;
            return ComputatePosition();
        }
    }
}


bool TTrackFollower::ComputatePosition()
{
    if (pCurrentSegment)
    {
        pPosition= pCurrentSegment->GetPoint(fCurrentDistance);
        return true;
    }
    return false;
}

bool TTrackFollower::Render()
{
    glPushMatrix();
        glTranslatef(pPosition.x, pPosition.y, pPosition.z);
        glRotatef(-90,1,0,0);
        //glutSolidCone(5,10,4,1);
		draw_axle_symbol(0.5, 12, 12);
    glPopMatrix();

	return true;
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
