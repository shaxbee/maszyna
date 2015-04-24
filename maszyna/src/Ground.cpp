//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#pragma hdrstop

#include "commons.h"
#include "commons_usr.h"

//#include "Timer.h"
//#include "Texture.h"
//#include "Ground.h"
//#include "Globals.h"
//#include "Event.h"
//#include "EvLaunch.h"
//#include "TractionPower.h"
//#include "Traction.h"
//#include "Track.h"
#include "RealSound.h"
//#include "AnimModel.h"
//#include "MemCell.h"
//#include "mtable.hpp"
//#include "DynObj.h"
#include "Data.h"
#include "parser.h" //Tolaris-010603
//#include "Driver.h"
#include "Console.h"
#include "Names.h"
#include "logs.h"

#define _PROBLEND 1
//---------------------------------------------------------------------------


bool bCondition; // McZapkie: do testowania warunku na event multiple
std::string LogComment;

//---------------------------------------------------------------------------
// Obiekt renderujący siatkę jest sztucznie tworzonym obiektem pomocniczym,
// grupującym siatki obiektów dla danej tekstury. Obiektami składowymi mogą
// byc trójkąty terenu, szyny, podsypki, a także proste modele np. słupy.
// Obiekty składowe dodane są do listy TSubRect::nMeshed z listą zrobioną na
// TGroundNode::nNext3, gdzie są posortowane wg tekstury. Obiekty renderujące
// są wpisane na listę TSubRect::nRootMesh (TGroundNode::nNext2) oraz na
// odpowiednie listy renderowania, gdzie zastępują obiekty składowe (nNext3).
// Problematyczne są tory/drogi/rzeki, gdzie używane sa 2 tekstury. Dlatego
// tory są zdublowane jako TP_TRACK oraz TP_DUMMYTRACK. Jeśli tekstura jest
// tylko jedna (np. zwrotnice), nie jest używany TP_DUMMYTRACK.
//---------------------------------------------------------------------------
TGroundNode::TGroundNode() { // nowy obiekt terenu - pusty

 iType=GL_POINTS;
 Vertices=NULL;
 Next=nNext2=NULL;
 pCenter=vector3(0,0,0);
 iCount=0; //wierzchołków w trójkącie
 //iNumPts=0; //punktów w linii
 TextureID=0;
 iFlags=0; //tryb przezroczystości nie zbadany
 DisplayListID=0;
 Pointer=NULL; //zerowanie wskaźnika kontekstowego
 bVisible=false; //czy widoczny
 fSquareRadius=10000*10000;
 fSquareMinRadius=0;
 asName="";
 //Color= TMaterialColor(1);
 //fAngle=0; //obrót dla modelu
 //fLineThickness=1.0; //mm dla linii
 for (int i=0;i<3;i++)
 {
  Ambient[i]=Global::whiteLight[i]*255;
  Diffuse[i]=Global::whiteLight[i]*255;
  Specular[i]=Global::noLight[i]*255;
 }
 nNext3=NULL; //nie wyświetla innych
 iVboPtr=-1; //indeks w VBO sektora (-1: nie używa VBO)
 iVersion=0; //wersja siatki

}

TGroundNode::~TGroundNode() 
{

  // if (iFlags&0x200) //czy obiekt został utworzony?
  switch (iType) {
  case TP_MEMCELL:
    SafeDelete(MemCell);
    break;
  case TP_EVLAUNCH:
    SafeDelete(EvLaunch);
    break;
  case TP_TRACTION:
    //SafeDelete(hvTraction);
    break;
  case TP_TRACTIONPOWERSOURCE:
    //SafeDelete(psTractionPowerSource);
    break;
  case TP_TRACK:
    SafeDelete(pTrack);
    break;
  case TP_DYNAMIC:
    //SafeDelete(DynamicObject);
    break;
  case TP_MODEL:
    if (iFlags & 0x200) // czy model został utworzony?
      delete Model;
    Model = NULL;
    break;
  case TP_TERRAIN: { // pierwsze nNode zawiera model E3D, reszta to trójkąty
    for (int i = 1; i < iCount; ++i)
      nNode->Vertices = NULL; // zerowanie wskaźników w kolejnych elementach, bo
                              // nie są do usuwania
    delete[] nNode; // usunięcie tablicy i pierwszego elementu
  }
  case TP_SUBMODEL: // dla formalności, nie wymaga usuwania
    break;
  case GL_LINES:
  case GL_LINE_STRIP:
  case GL_LINE_LOOP:
    SafeDeleteArray(Points);
    break;
  case GL_TRIANGLE_STRIP:
  case GL_TRIANGLE_FAN:
  case GL_TRIANGLES:
    SafeDeleteArray(Vertices);
    break;
  }

}

void TGroundNode::Init(int n) { // utworzenie tablicy wierzchołków
  bVisible = false;
  iNumVerts = n;
  Vertices = new TGroundVertex[iNumVerts];
}

TGroundNode::TGroundNode(TGroundNodeType t, int n) { // utworzenie obiektu
  TGroundNode(); // domyślne ustawienia
  iNumVerts = n;
  if (iNumVerts)
    Vertices = new TGroundVertex[iNumVerts];
  iType = t;
  switch (iType) { // zależnie od typu
  case TP_TRACK:
//--    pTrack = new TTrack(this);
    break;
  }
}



void TGroundNode::InitCenter()
{//obliczenie środka ciężkości obiektu
	for (int i = 0; i<iNumVerts; i++)
		pCenter += Vertices[i].Point;
	pCenter /= iNumVerts;
}

void TGroundNode::InitNormals()
{//obliczenie wektorów normalnych
	vector3 v1, v2, v3, v4, v5, n1, n2, n3, n4;
	int i;
	float tu, tv;
	switch (iType)
	{
	case GL_TRIANGLE_STRIP:
		v1 = Vertices[0].Point - Vertices[1].Point;
		v2 = Vertices[1].Point - Vertices[2].Point;
		n1 = SafeNormalize(CrossProduct(v1, v2));
		if (Vertices[0].Normal == vector3(0, 0, 0))
			Vertices[0].Normal = n1;
		v3 = Vertices[2].Point - Vertices[3].Point;
		n2 = SafeNormalize(CrossProduct(v3, v2));
		if (Vertices[1].Normal == vector3(0, 0, 0))
			Vertices[1].Normal = (n1 + n2)*0.5;

		for (i = 2; i<iNumVerts - 2; i += 2)
		{
			v4 = Vertices[i - 1].Point - Vertices[i].Point;
			v5 = Vertices[i].Point - Vertices[i + 1].Point;
			n3 = SafeNormalize(CrossProduct(v3, v4));
			n4 = SafeNormalize(CrossProduct(v5, v4));
			if (Vertices[i].Normal == vector3(0, 0, 0))
				Vertices[i].Normal = (n1 + n2 + n3) / 3;
			if (Vertices[i + 1].Normal == vector3(0, 0, 0))
				Vertices[i + 1].Normal = (n2 + n3 + n4) / 3;
			n1 = n3;
			n2 = n4;
			v3 = v5;
		}
		if (Vertices[i].Normal == vector3(0, 0, 0))
			Vertices[i].Normal = (n1 + n2) / 2;
		if (Vertices[i + 1].Normal == vector3(0, 0, 0))
			Vertices[i + 1].Normal = n2;
		break;
	case GL_TRIANGLE_FAN:

		break;
	case GL_TRIANGLES:
		for (i = 0; i<iNumVerts; i += 3)
		{
			v1 = Vertices[i + 0].Point - Vertices[i + 1].Point;
			v2 = Vertices[i + 1].Point - Vertices[i + 2].Point;
			n1 = SafeNormalize(CrossProduct(v1, v2));
			if (Vertices[i + 0].Normal == vector3(0, 0, 0))
				Vertices[i + 0].Normal = (n1);
			if (Vertices[i + 1].Normal == vector3(0, 0, 0))
				Vertices[i + 1].Normal = (n1);
			if (Vertices[i + 2].Normal == vector3(0, 0, 0))
				Vertices[i + 2].Normal = (n1);
			tu = floor(Vertices[i + 0].tu);
			tv = floor(Vertices[i + 0].tv);
			Vertices[i + 1].tv -= tv;
			Vertices[i + 2].tv -= tv;
			Vertices[i + 0].tv -= tv;
			Vertices[i + 1].tu -= tu;
			Vertices[i + 2].tu -= tu;
			Vertices[i + 0].tu -= tu;
		}
		break;
	}
}


void TGroundNode::MoveMe(vector3 pPosition)
{//przesuwanie obiektów scenerii o wektor w celu redukcji trzęsienia
	pCenter += pPosition;
	switch (iType)
	{
	/*
	case TP_TRACTION:
		Traction->pPoint1 += pPosition;
		Traction->pPoint2 += pPosition;
		Traction->pPoint3 += pPosition;
		Traction->pPoint4 += pPosition;
		Traction->Optimize();
		break;
		*/
	case TP_MODEL:
	case TP_DYNAMIC:
	case TP_MEMCELL:
	case TP_EVLAUNCH:
		break;
	case TP_TRACK:
		pTrack->MoveMe(pPosition);
		break;
	case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
		pStaticSound->vSoundPosition += pPosition;
		break;
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
		for (int i = 0; i<iNumPts; i++)
			Points[i] += pPosition;
		ResourceManager::Unregister(this);
		break;
	default:
		for (int i = 0; i<iNumVerts; i++)
			Vertices[i].Point += pPosition;
		ResourceManager::Unregister(this);
	}
}

void TGroundNode::RaRenderVBO()
{//renderowanie z domyslnego bufora VBO
	glColor3ub(Diffuse[0], Diffuse[1], Diffuse[2]);
	if (TextureID)
		glBindTexture(GL_TEXTURE_2D, TextureID); // Ustaw aktywną teksturę
	glDrawArrays(iType, iVboPtr, iNumVerts);   // Narysuj naraz wszystkie trójkąty
}


void TGroundNode::RenderVBO()
{//renderowanie obiektu z VBO - faza nieprzezroczystych
	double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
	if ((mgn>fSquareRadius || (mgn<fSquareMinRadius)) && (iType != TP_EVLAUNCH)) //McZapkie-070602: nie rysuj odleglych obiektow ale sprawdzaj wyzwalacz zdarzen
		return;
	int i, a;
	switch (iType)
	{
	case TP_TRACTION: return;
//-	case TP_TRACK: if (iNumVerts) pTrack->RaRenderVBO(iVboPtr); return;
//-	case TP_MODEL: Model->RenderVBO(&pCenter); return;
	case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
		if ((pStaticSound->GetStatus()&DSBSTATUS_PLAYING) == DSBPLAY_LOOPING)
		{
			pStaticSound->Play(1, DSBPLAY_LOOPING, true, pStaticSound->vSoundPosition);
			pStaticSound->AdjFreq(1.0, Timer::GetDeltaTime());
		}
		return; //Ra: TODO sprawdzić, czy dźwięki nie są tylko w RenderHidden
	case TP_MEMCELL: return;
//-	case TP_EVLAUNCH:
//-		if (EvLaunch->Render())
//-			if ((EvLaunch->dRadius<0) || (mgn<EvLaunch->dRadius))
//-			{
//-				if (Console::Pressed(VK_SHIFT) && EvLaunch->Event2 != NULL)
//-					Global::AddToQuery(EvLaunch->Event2, NULL);
//-				else
//-					if (EvLaunch->Event1 != NULL)
//-						Global::AddToQuery(EvLaunch->Event1, NULL);
//-			}
//-		return;
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
		if (iNumPts)
		{
			float linealpha = 255000 * fLineThickness / (mgn + 1.0);
			if (linealpha>255) linealpha = 255;
			float r, g, b;
			r = floor(Diffuse[0] * Global::ambientDayLight[0]);  //w zaleznosci od koloru swiatla
			g = floor(Diffuse[1] * Global::ambientDayLight[1]);
			b = floor(Diffuse[2] * Global::ambientDayLight[2]);
			glColor4ub(r, g, b, linealpha); //przezroczystosc dalekiej linii
			//glDisable(GL_LIGHTING); //nie powinny świecić
			glDrawArrays(iType, iVboPtr, iNumPts); //rysowanie linii
			//glEnable(GL_LIGHTING);
		}
		return;
	default:
		if (iVboPtr >= 0)
			RaRenderVBO();
	};
	return;
};

void TGroundNode::RenderAlphaVBO()
{//renderowanie obiektu z VBO - faza przezroczystych
	double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
	float r, g, b;
	if (mgn<fSquareMinRadius) return;
	if (mgn>fSquareRadius) return;
	int i, a;
#ifdef _PROBLEND
	if ((PROBLEND)) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
	{
		glDisable(GL_BLEND);
		glAlphaFunc(GL_GREATER, 0.45);     // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
	};
#endif
	switch (iType)
	{
	case TP_TRACTION:
		if (bVisible)
		{
#ifdef _PROBLEND
			glEnable(GL_BLEND);
			glAlphaFunc(GL_GREATER, 0.04);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
//-			Traction->RenderVBO(mgn, iVboPtr);
		}
		return;
	case TP_MODEL:
#ifdef _PROBLEND
		glEnable(GL_BLEND);
		glAlphaFunc(GL_GREATER, 0.04);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
//-		Model->RenderAlphaVBO(&pCenter);
		return;
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
		if (iNumPts)
		{
			float linealpha = 255000 * fLineThickness / (mgn + 1.0);
			if (linealpha>255) linealpha = 255;
			r = Diffuse[0] * Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
			g = Diffuse[1] * Global::ambientDayLight[1];
			b = Diffuse[2] * Global::ambientDayLight[2];
			glColor4ub(r, g, b, linealpha); //przezroczystosc dalekiej linii
			//glDisable(GL_LIGHTING); //nie powinny świecić
			glDrawArrays(iType, iVboPtr, iNumPts); //rysowanie linii
			//glEnable(GL_LIGHTING);
#ifdef _PROBLEND
			glEnable(GL_BLEND);
			glAlphaFunc(GL_GREATER, 0.04);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
		}
#ifdef _PROBLEND
		glEnable(GL_BLEND);
		glAlphaFunc(GL_GREATER, 0.04);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
		return;
	default:
		if (iVboPtr >= 0)
		{
			RaRenderVBO();
#ifdef _PROBLEND
			glEnable(GL_BLEND);
			glAlphaFunc(GL_GREATER, 0.04);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
			return;
		}
	}
#ifdef _PROBLEND
	glEnable(GL_BLEND);
	glAlphaFunc(GL_GREATER, 0.04);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
	return;
}



void TGroundNode::Compile(bool many)
{//tworzenie skompilowanej listy w wyświetlaniu DL
	if (!many)
	{//obsługa pojedynczej listy
		if (DisplayListID) Release();
		if (Global::bManageNodes)
		{
			DisplayListID = glGenLists(1);
			glNewList(DisplayListID, GL_COMPILE);
			iVersion = Global::iReCompile; //aktualna wersja siatek (do WireFrame)
		}
	}
	if ((iType == GL_LINES) || (iType == GL_LINE_STRIP) || (iType == GL_LINE_LOOP))
	{
#ifdef USE_VERTEX_ARRAYS
		glVertexPointer(3, GL_DOUBLE, sizeof(vector3), &Points[0].x);
#endif
		glBindTexture(GL_TEXTURE_2D, 0);
#ifdef USE_VERTEX_ARRAYS
		glDrawArrays(iType, 0, iNumPts);
#else
		glBegin(iType);
		for (int i = 0; i<iNumPts; i++)
			glVertex3dv(&Points[i].x);
		glEnd();
#endif
	}
	else if (iType == GL_TRIANGLE_STRIP || iType == GL_TRIANGLE_FAN || iType == GL_TRIANGLES)
	{//jak nie linie, to trójkąty
		TGroundNode *tri = this;
		do
		{//pętla po obiektach w grupie w celu połączenia siatek
#ifdef USE_VERTEX_ARRAYS
			glVertexPointer(3, GL_DOUBLE, sizeof(TGroundVertex), &tri->Vertices[0].Point.x);
			glNormalPointer(GL_DOUBLE, sizeof(TGroundVertex), &tri->Vertices[0].Normal.x);
			glTexCoordPointer(2, GL_FLOAT, sizeof(TGroundVertex), &tri->Vertices[0].tu);
#endif
			glColor3ub(tri->Diffuse[0], tri->Diffuse[1], tri->Diffuse[2]);
			glBindTexture(GL_TEXTURE_2D, Global::bWireFrame ? 0 : tri->TextureID);
#ifdef USE_VERTEX_ARRAYS
			glDrawArrays(Global::bWireFrame ? GL_LINE_LOOP : tri->iType, 0, tri->iNumVerts);
#else
			glBegin(Global::bWireFrame ? GL_LINE_LOOP : tri->iType);
			for (int i = 0; i<tri->iNumVerts; i++)
			{
				glNormal3d(tri->Vertices[i].Normal.x, tri->Vertices[i].Normal.y, tri->Vertices[i].Normal.z);
				glTexCoord2f(tri->Vertices[i].tu, tri->Vertices[i].tv);
				glVertex3dv(&tri->Vertices[i].Point.x);
			};
			glEnd();
#endif
			/*
			if (tri->pTriGroup) //jeśli z grupy
			{tri=tri->pNext2; //następny w sektorze
			while (tri?!tri->pTriGroup:false) tri=tri->pNext2; //szukamy kolejnego należącego do grupy
			}
			else
			*/
			tri = NULL; //a jak nie, to koniec
		} while (tri);
	}
	else if (iType == TP_MESH)
	{//grupa ze wspólną teksturą - wrzucanie do wspólnego Display List
		if (TextureID)
			glBindTexture(GL_TEXTURE_2D, TextureID); // Ustaw aktywną teksturę
		TGroundNode *n = nNode;
		while (n ? n->TextureID == TextureID : false)
		{//wszystkie obiekty o tej samej testurze
			switch (n->iType)
			{//poszczególne typy różnie się tworzy
			case TP_TRACK:
			case TP_DUMMYTRACK:
				n->pTrack->Compile(); //dodanie trójkątów dla podanej tekstury // Q: BYLO n->pTrack->Compile(TextureID); TODO:
				break;
			}
			n = n->nNext3; //następny z listy
		}
	}
	if (!many)
		if (Global::bManageNodes)
			glEndList();
};

void TGroundNode::Release()
{
	if (DisplayListID)
		glDeleteLists(DisplayListID, 1);
	DisplayListID = 0;
};


void TGroundNode::RenderHidden()
{//renderowanie obiektów niewidocznych
	double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
	switch (iType)
	{
	case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
		if ((pStaticSound->GetStatus()&DSBSTATUS_PLAYING) == DSBPLAY_LOOPING)
		{
			pStaticSound->Play(1, DSBPLAY_LOOPING, true, pStaticSound->vSoundPosition);
			pStaticSound->AdjFreq(1.0, Timer::GetDeltaTime());
		}
		return;
	/*
	case TP_EVLAUNCH:
		if (EvLaunch->Render())
			if ((EvLaunch->dRadius<0) || (mgn<EvLaunch->dRadius))
			{
				WriteLog("Eventlauncher " + asName);
				if (Console::Pressed(VK_SHIFT) && (EvLaunch->Event2))
					Global::AddToQuery(EvLaunch->Event2, NULL);
				else
					if (EvLaunch->Event1)
						Global::AddToQuery(EvLaunch->Event1, NULL);
			}
		return;
		*/
	}
};


void TGroundNode::RenderDL()
{//wyświetlanie obiektu przez Display List
	//-switch (iType)
	//-{//obiekty renderowane niezależnie od odległości
	//-case TP_SUBMODEL:
	//-	TSubModel::fSquareDist = 0;
	//-	return smTerrain->RenderDL();
	//-}
	//if (pTriGroup) if (pTriGroup!=this) return; //wyświetla go inny obiekt
	double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
	if ((mgn>fSquareRadius) || (mgn<fSquareMinRadius)) //McZapkie-070602: nie rysuj odleglych obiektow ale sprawdzaj wyzwalacz zdarzen
		return;
	int i, a;
	//-switch (iType)
	//-{
	//-case TP_TRACK:
	//-	return pTrack->Render();
	//-case TP_MODEL:
	//-	return Model->RenderDL(&pCenter);
	//-}
	// TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
	//if ((iNumVerts&&(iFlags&0x10))||(iNumPts&&(fLineThickness<0)))
	if ((iFlags & 0x10) || (fLineThickness<0))
	{
		if (!DisplayListID || (iVersion != Global::iReCompile)) //Ra: wymuszenie rekompilacji
		{
			Compile();
			if (Global::bManageNodes)
				ResourceManager::Register(this);
		};

		if ((iType == GL_LINES) || (iType == GL_LINE_STRIP) || (iType == GL_LINE_LOOP))
			//if (iNumPts)
		{//wszelkie linie są rysowane na samym końcu
			float r, g, b;
			r = Diffuse[0] * Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
			g = Diffuse[1] * Global::ambientDayLight[1];
			b = Diffuse[2] * Global::ambientDayLight[2];
			glColor4ub(r, g, b, 1.0);
			glCallList(DisplayListID);
			//glColor4fv(Diffuse); //przywrócenie koloru
			//glColor3ub(Diffuse[0],Diffuse[1],Diffuse[2]);
		}
		// GL_TRIANGLE etc
		else
			glCallList(DisplayListID);
		SetLastUsage(Timer::GetSimulationTime());
	};
};


void TGroundNode::RenderAlphaDL()
{
	// SPOSOB NA POZBYCIE SIE RAMKI DOOKOLA TEXTURY ALPHA DLA OBIEKTOW ZAGNIEZDZONYCH W SCN JAKO NODE

	//W GROUND.H dajemy do klasy TGroundNode zmienna bool PROBLEND to samo robimy w klasie TGround
	//nastepnie podczas wczytywania textury dla TRIANGLES w TGround::AddGroundNode
	//sprawdzamy czy w nazwie jest @ i wg tego
	//ustawiamy PROBLEND na true dla wlasnie wczytywanego trojkata (kazdy trojkat jest osobnym nodem)
	//nastepnie podczas renderowania w bool TGroundNode::RenderAlpha()
	//na poczatku ustawiamy standardowe GL_GREATER = 0.04
	//pozniej sprawdzamy czy jest wlaczony PROBLEND dla aktualnie renderowanego noda TRIANGLE, wlasciwie dla kazdego node'a
	//i jezeli tak to odpowiedni GL_GREATER w przeciwnym wypadku standardowy 0.04


	//if (pTriGroup) if (pTriGroup!=this) return; //wyświetla go inny obiekt
	double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
	float r, g, b;
	if (mgn<fSquareMinRadius)
		return;
	if (mgn>fSquareRadius)
		return;
	int i, a;
	switch (iType)
	{
	case TP_TRACTION:
		if (bVisible)
//-			Traction->RenderDL(mgn); // TODO:
		return;
	case TP_MODEL:
		//---Model->RenderAlphaDL(&pCenter);
		return;
	case TP_TRACK:
		//pTrack->RenderAlpha();
		return;
	};

	// TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
	if (
		(iNumVerts && (iFlags & 0x20)) ||
		(iNumPts && (fLineThickness > 0)))
	{
#ifdef _PROBLEND
		if ((PROBLEND)) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
		{
			glDisable(GL_BLEND);
			glAlphaFunc(GL_GREATER, 0.45);     // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
		};
#endif
		if (!DisplayListID) //||Global::bReCompile) //Ra: wymuszenie rekompilacji
		{
			Compile();
			if (Global::bManageNodes)
				ResourceManager::Register(this);
		};

		// GL_LINE, GL_LINE_STRIP, GL_LINE_LOOP
		if (iNumPts)
		{
			float linealpha = 255000 * fLineThickness / (mgn + 1.0);
			if (linealpha>255)
				linealpha = 255;
			r = Diffuse[0] * Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
			g = Diffuse[1] * Global::ambientDayLight[1];
			b = Diffuse[2] * Global::ambientDayLight[2];
			glColor4ub(r, g, b, linealpha); //przezroczystosc dalekiej linii
			glCallList(DisplayListID);
		}
		// GL_TRIANGLE etc
		else
			glCallList(DisplayListID);
		SetLastUsage(Timer::GetSimulationTime());
	};
#ifdef _PROBLEND
	if ((PROBLEND)) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
	{
		glEnable(GL_BLEND);
		glAlphaFunc(GL_GREATER, 0.04);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	};
#endif
}


//------------------------------------------------------------------------------
//------------------ Podstawowy pojemnik terenu - sektor -----------------------
//------------------------------------------------------------------------------
TSubRect::TSubRect()
{
	nRootNode = NULL; //lista wszystkich obiektów jest pusta
	nRenderHidden = nRenderRect = nRenderRectAlpha = nRender = nRenderMixed = nRenderAlpha = nRenderWires = NULL;
	tTrackAnim = NULL; //nic nie animujemy
	tTracks = NULL; //nie ma jeszcze torów
	nRootMesh = nMeshed = NULL; //te listy też są puste
	iNodeCount = 0; //licznik obiektów
	iTracks = 0; //licznik torów
}
TSubRect::~TSubRect()
{
	if (Global::bManageNodes) //Ra: tu się coś sypie
		ResourceManager::Unregister(this); //wyrejestrowanie ze sprzątacza
	//TODO: usunąć obiekty z listy (nRootMesh), bo są one tworzone dla sektora
}


// ********************************************************************************************************************
// void TSubRect::NodeAdd(TGroundNode *Node)
// ********************************************************************************************************************
void TSubRect::NodeAdd(TGroundNode *Node)
{//przyczepienie obiektu do sektora, wstępna kwalifikacja na listy renderowania
	if (!this) return; //zabezpiecznie przed obiektami przekraczającymi obszar roboczy
	//Ra: sortowanie obiektów na listy renderowania:
	//nRenderHidden    - lista obiektów niewidocznych, "renderowanych" również z tyłu
	//nRenderRect      - lista grup renderowanych z sektora
	//nRenderRectAlpha - lista grup renderowanych z sektora z przezroczystością
	//nRender          - lista grup renderowanych z własnych VBO albo DL
	//nRenderAlpha     - lista grup renderowanych z własnych VBO albo DL z przezroczystością
	//nRenderWires     - lista grup renderowanych z własnych VBO albo DL - druty i linie
	//nMeshed          - obiekty do pogrupowania wg tekstur
	GLuint t; //pomocniczy kod tekstury
	switch (Node->iType)
	{
	case TP_SOUND: //te obiekty są sprawdzanie niezależnie od kierunku patrzenia
	case TP_EVLAUNCH:
		Node->nNext3 = nRenderHidden; nRenderHidden = Node; //do listy koniecznych
		break;
	case TP_TRACK: //TODO: tory z cieniem (tunel, canyon) też dać bez łączenia?
		++iTracks; //jeden tor więcej
		Node->pTrack->RaOwnerSet(this); //do którego sektora ma zgłaszać animację
		//if (Global::bUseVBO?false:!Node->pTrack->IsGroupable())
		if (Global::bUseVBO ? true : !Node->pTrack->IsGroupable()) //TODO: tymczasowo dla VBO wyłączone
			RaNodeAdd(Node); //tory ruchome nie są grupowane przy Display Lists (wymagają odświeżania DL)
		else
		{//tory nieruchome mogą być pogrupowane wg tekstury, przy VBO wszystkie
			Node->TextureID = Node->pTrack->TextureGet(0); //pobranie tekstury do sortowania
			t = Node->pTrack->TextureGet(1);
			if (Node->TextureID) //jeżeli jest pierwsza
			{
				if (t && (Node->TextureID != t))
				{//jeśli są dwie różne tekstury, dodajemy drugi obiekt dla danego toru
					TGroundNode *n = new TGroundNode();
					n->iType = TP_DUMMYTRACK; //obiekt renderujący siatki dla tekstury
					n->TextureID = t;
					n->pTrack = Node->pTrack; //wskazuje na ten sam tor
					n->pCenter = Node->pCenter;
					n->fSquareRadius = Node->fSquareRadius;
					n->fSquareMinRadius = Node->fSquareMinRadius;
					n->iFlags = Node->iFlags;
					n->nNext2 = nRootMesh; nRootMesh = n; //podczepienie do listy, żeby usunąć na końcu
					n->nNext3 = nMeshed; nMeshed = n;
				}
			}
			else
				Node->TextureID = t; //jest tylko druga tekstura
			if (Node->TextureID)
			{
				Node->nNext3 = nMeshed; nMeshed = Node;
			} //do podzielenia potem
		}
		break;
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLES:
		//Node->nNext3=nMeshed; nMeshed=Node; //do podzielenia potem
		if (Node->iFlags & 0x20) //czy jest przezroczyste?
		{
			Node->nNext3 = nRenderRectAlpha; nRenderRectAlpha = Node;
		} //DL: do przezroczystych z sektora
		else
			if (Global::bUseVBO)
			{
				Node->nNext3 = nRenderRect; nRenderRect = Node;
			} //VBO: do nieprzezroczystych z sektora
			else
			{
				Node->nNext3 = nRender; nRender = Node;
			} //DL: do nieprzezroczystych wszelakich
		/*
		//Ra: na razie wyłączone do testów VBO
		//if ((Node->iType==GL_TRIANGLE_STRIP)||(Node->iType==GL_TRIANGLE_FAN)||(Node->iType==GL_TRIANGLES))
		if (Node->fSquareMinRadius==0.0) //znikające z bliska nie mogą być optymalizowane
		if (Node->fSquareRadius>=160000.0) //tak od 400m to już normalne trójkąty muszą być
		//if (Node->iFlags&0x10) //i nieprzezroczysty
		{if (pTriGroup) //jeżeli był już jakiś grupujący
		{if (pTriGroup->fSquareRadius>Node->fSquareRadius) //i miał większy zasięg
		Node->fSquareRadius=pTriGroup->fSquareRadius; //zwiększenie zakresu widoczności grupującego
		pTriGroup->pTriGroup=Node; //poprzedniemu doczepiamy nowy
		}
		Node->pTriGroup=Node; //nowy lider ma się sam wyświetlać - wskaźnik na siebie
		pTriGroup=Node; //zapamiętanie lidera
		}
		*/
		break;
	case TP_TRACTION:
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP: //te renderowane na końcu, żeby nie łapały koloru nieba
		Node->nNext3 = nRenderWires; nRenderWires = Node; //lista drutów
		break;
	case TP_MODEL: //modele zawsze wyświetlane z własnego VBO
		//jeśli model jest prosty, można próbować zrobić wspólną siatkę (słupy)
		if ((Node->iFlags & 0x20200020) == 0) //czy brak przezroczystości?
		{
			Node->nNext3 = nRender; nRender = Node;
		} //do nieprzezroczystych
		else if ((Node->iFlags & 0x10100010) == 0) //czy brak nieprzezroczystości?
		{
			Node->nNext3 = nRenderAlpha; nRenderAlpha = Node;
		} //do przezroczystych
		else //jak i take i takie, to będzie dwa razy renderowane...
		{
			Node->nNext3 = nRenderMixed; nRenderMixed = Node;
		} //do mieszanych
		//Node->nNext3=nMeshed; //dopisanie do listy sortowania
		//nMeshed=Node;
		break;
	case TP_MEMCELL:
	case TP_TRACTIONPOWERSOURCE: //a te w ogóle pomijamy
		//  case TP_ISOLATED: //lista torów w obwodzie izolowanym - na razie ignorowana
		break;
	case TP_DYNAMIC:
		return; //tych nie dopisujemy wcale
	}
	Node->nNext2 = nRootNode; //dopisanie do ogólnej listy
	nRootNode = Node;
	++iNodeCount; //licznik obiektów
}


void TSubRect::RaNodeAdd(TGroundNode *Node)
{//finalna kwalifikacja na listy renderowania, jeśli nie obsługiwane grupowo
	switch (Node->iType)
	{
	case TP_TRACK:
		if (Global::bUseVBO)
		{
			Node->nNext3 = nRenderRect; nRenderRect = Node;
		} //VBO: do nieprzezroczystych z sektora
		else
		{
			Node->nNext3 = nRender; nRender = Node;
		} //DL: do nieprzezroczystych
		break;
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLES:
		if (Node->iFlags & 0x20) //czy jest przezroczyste?
		{
			Node->nNext3 = nRenderRectAlpha; nRenderRectAlpha = Node;
		} //DL: do przezroczystych z sektora
		else
			if (Global::bUseVBO)
			{
				Node->nNext3 = nRenderRect; nRenderRect = Node;
			} //VBO: do nieprzezroczystych z sektora
			else
			{
				Node->nNext3 = nRender; nRender = Node;
			} //DL: do nieprzezroczystych wszelakich
		break;
	case TP_MODEL: //modele zawsze wyświetlane z własnego VBO
		if ((Node->iFlags & 0x20200020) == 0) //czy brak przezroczystości?
		{
			Node->nNext3 = nRender; nRender = Node;
		} //do nieprzezroczystych
		else if ((Node->iFlags & 0x10100010) == 0) //czy brak nieprzezroczystości?
		{
			Node->nNext3 = nRenderAlpha; nRenderAlpha = Node;
		} //do przezroczystych
		else //jak i take i takie, to będzie dwa razy renderowane...
		{
			Node->nNext3 = nRenderMixed; nRenderMixed = Node;
		} //do mieszanych
		break;
	case TP_MESH: //grupa ze wspólną teksturą
		//{Node->nNext3=nRenderRect; nRenderRect=Node;} //do nieprzezroczystych z sektora
	{Node->nNext3 = nRender; nRender = Node; } //do nieprzezroczystych
	break;
	case TP_SUBMODEL: //submodele terenu w kwadracie kilometrowym idą do nRootMesh
		//WriteLog("nRootMesh was "+AnsiString(nRootMesh?"not null ":"null ")+IntToHex(int(this),8));
		Node->nNext3 = nRootMesh; //przy VBO musi być inaczej
		nRootMesh = Node;
		break;
	}
}

void TSubRect::Sort()
{//przygotowanie sektora do renderowania
	TGroundNode **n0, *n1, *n2; //wskaźniki robocze
	delete[] tTracks; //usunięcie listy
	tTracks = iTracks ? new TTrack*[iTracks] : NULL; //tworzenie tabeli torów do renderowania pojazdów
	if (tTracks)
	{//wypełnianie tabeli torów
		int i = 0;
		for (n1 = nRootNode; n1; n1 = n1->nNext2) //kolejne obiekty z sektora
			if (n1->iType == TP_TRACK)
				tTracks[i++] = n1->pTrack;
	}
	//sortowanie obiektów w sektorze na listy renderowania
	if (!nMeshed) return; //nie ma nic do sortowania
	bool sorted = false;
	while (!sorted)
	{//sortowanie bąbelkowe obiektów wg tekstury
		sorted = true; //zakładamy posortowanie
		n0 = &nMeshed; //wskaźnik niezbędny do zamieniania obiektów
		n1 = nMeshed; //lista obiektów przetwarzanych na statyczne siatki
		while (n1)
		{//sprawdzanie stanu posortowania obiektów i ewentualne zamiany
			n2 = n1->nNext3; //kolejny z tej listy
			if (n2) //jeśli istnieje
				if (n1->TextureID>n2->TextureID)
				{//zamiana elementów miejscami
					*n0 = n2; //drugi będzie na początku
					n1->nNext3 = n2->nNext3; //ten zza drugiego będzie za pierwszym
					n2->nNext3 = n1; //a za drugim będzie pierwszy
					sorted = false; //potrzebny kolejny przebieg
				}
			n0 = &(n1->nNext3);
			n1 = n2;
		};
	}
	//wyrzucenie z listy obiektów pojedynczych (nie ma z czym ich grupować)
	//nawet jak są pojedyncze, to i tak lepiej, aby były w jednym Display List
	/*
	else
	{//dodanie do zwykłej listy renderowania i usunięcie z grupowego
	*n0=n2; //drugi będzie na początku
	RaNodeAdd(n1); //nie ma go z czym zgrupować; (n1->nNext3) zostanie nadpisane
	n1=n2; //potrzebne do ustawienia (n0)
	}
	*/
	//...
	//przeglądanie listy i tworzenie obiektów renderujących dla danej tekstury
	GLuint t = 0; //pomocniczy kod tekstury
	n1 = nMeshed; //lista obiektów przetwarzanych na statyczne siatki
	while (n1)
	{//dla każdej tekstury powinny istnieć co najmniej dwa obiekty, ale dla DL nie ma to znaczenia
		if (t<n1->TextureID) //jeśli (n1) ma inną teksturę niż poprzednie
		{//można zrobić obiekt renderujący
			t = n1->TextureID;
			n2 = new TGroundNode();
			n2->nNext2 = nRootMesh; nRootMesh = n2; //podczepienie na początku listy
			nRootMesh->iType = TP_MESH; //obiekt renderujący siatki dla tekstury
			nRootMesh->TextureID = t;
			nRootMesh->nNode = n1; //pierwszy element z listy
			nRootMesh->pCenter = n1->pCenter;
			nRootMesh->fSquareRadius = 1e8; //widać bez ograniczeń
			nRootMesh->fSquareMinRadius = 0.0;
			nRootMesh->iFlags = 0x10;
			RaNodeAdd(nRootMesh); //dodanie do odpowiedniej listy renderowania
		}
		n1 = n1->nNext3; //kolejny z tej listy
	};
}


TTrack* TSubRect::FindTrack(vector3 *Point, int &iConnection, TTrack *Exclude)
{//szukanie toru, którego koniec jest najbliższy (*Point)
	TTrack *Track;
	for (int i = 0; i<iTracks; ++i)
		if (tTracks[i] != Exclude) //można użyć tabelę torów, bo jest mniejsza
		{
			iConnection = tTracks[i]->TestPoint(Point);
			if (iConnection >= 0) return tTracks[i]; //szukanie TGroundNode nie jest potrzebne
		}
	/*
	TGroundNode *Current;
	for (Current=nRootNode;Current;Current=Current->Next)
	if ((Current->iType==TP_TRACK)&&(Current->pTrack!=Exclude)) //można użyć tabelę torów
	{
	iConnection=Current->pTrack->TestPoint(Point);
	if (iConnection>=0) return Current;
	}
	*/
	return NULL;
};

bool TSubRect::RaTrackAnimAdd(TTrack *t)
{//aktywacja animacji torów w VBO (zwrotnica, obrotnica)
	if (m_nVertexCount<0) return true; //nie ma animacji, gdy nie widać
	if (tTrackAnim)
		tTrackAnim->RaAnimListAdd(t);
	else
		tTrackAnim = t;
	return false; //będzie animowane...
}


void TSubRect::RaAnimate()
{//wykonanie animacji
	if (!tTrackAnim) return; //nie ma nic do animowania
	if (Global::bUseVBO)
	{//odświeżenie VBO sektora
		if (Global::bOpenGL_1_5) //modyfikacje VBO są dostępne od OpenGL 1.5
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVBOVertices);
		else //dla OpenGL 1.4 z GL_ARB_vertex_buffer_object odświeżenie całego sektora
			Release(); //opróżnienie VBO sektora, aby się odświeżył z nowymi ustawieniami
	}
	tTrackAnim = tTrackAnim->RaAnimate(); //przeliczenie animacji kolejnego
};

TTraction* TSubRect::FindTraction(vector3 *Point, int &iConnection, TTraction *Exclude)
{//szukanie przęsła w sektorze, którego koniec jest najbliższy (*Point)
	TGroundNode *Current;
	for (Current = nRenderWires; Current; Current = Current->nNext3)
		if ((Current->iType == TP_TRACTION) && (Current->Traction != Exclude))
		{
			//-iConnection = Current->Traction->TestPoint(Point);
			//-if (iConnection >= 0) return Current->Traction;
		}
	return NULL;
};

void TSubRect::LoadNodes()
{//utworzenie siatek VBO dla wszystkich node w sektorze
	if (m_nVertexCount >= 0) return; //obiekty były już sprawdzone
	m_nVertexCount = 0; //-1 oznacza, że nie sprawdzono listy obiektów
	if (!nRootNode) return;
	TGroundNode *n = nRootNode;
	while (n)
	{
		switch (n->iType)
		{
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLE_FAN:
		case GL_TRIANGLES:
			n->iVboPtr = m_nVertexCount; //nowy początek
			m_nVertexCount += n->iNumVerts;
			break;
		case GL_LINES:
		case GL_LINE_STRIP:
		case GL_LINE_LOOP:
			n->iVboPtr = m_nVertexCount; //nowy początek
			m_nVertexCount += n->iNumPts; //miejsce w tablicach normalnych i teksturowania się zmarnuje...
			break;
		case TP_TRACK:
			n->iVboPtr = m_nVertexCount; //nowy początek
			n->iNumVerts = n->pTrack->RaArrayPrepare(); //zliczenie wierzchołków
			m_nVertexCount += n->iNumVerts;
			break;
		case TP_TRACTION:
			n->iVboPtr = m_nVertexCount; //nowy początek
			//-n->iNumVerts = n->Traction->RaArrayPrepare(); //zliczenie wierzchołków
			//-m_nVertexCount += n->iNumVerts;
			break;
		}
		n = n->nNext2; //następny z sektora
	}
	if (!m_nVertexCount) return; //jeśli nie ma obiektów do wyświetlenia z VBO, to koniec
	if (Global::bUseVBO)
	{//tylko liczenie wierzchołów, gdy nie ma VBO
		MakeArray(m_nVertexCount);
		n = nRootNode;
		int i;
		while (n)
		{
			if (n->iVboPtr >= 0)
				switch (n->iType)
			{
				case GL_TRIANGLE_STRIP:
				case GL_TRIANGLE_FAN:
				case GL_TRIANGLES:
					for (i = 0; i<n->iNumVerts; ++i)
					{//Ra: trójkąty można od razu wczytywać do takich tablic... to może poczekać
						m_pVNT[n->iVboPtr + i].x = n->Vertices[i].Point.x;
						m_pVNT[n->iVboPtr + i].y = n->Vertices[i].Point.y;
						m_pVNT[n->iVboPtr + i].z = n->Vertices[i].Point.z;
						m_pVNT[n->iVboPtr + i].nx = n->Vertices[i].Normal.x;
						m_pVNT[n->iVboPtr + i].ny = n->Vertices[i].Normal.y;
						m_pVNT[n->iVboPtr + i].nz = n->Vertices[i].Normal.z;
						m_pVNT[n->iVboPtr + i].u = n->Vertices[i].tu;
						m_pVNT[n->iVboPtr + i].v = n->Vertices[i].tv;
					}
					break;
				case GL_LINES:
				case GL_LINE_STRIP:
				case GL_LINE_LOOP:
					for (i = 0; i<n->iNumPts; ++i)
					{
						m_pVNT[n->iVboPtr + i].x = n->Points[i].x;
						m_pVNT[n->iVboPtr + i].y = n->Points[i].y;
						m_pVNT[n->iVboPtr + i].z = n->Points[i].z;
						//miejsce w tablicach normalnych i teksturowania się marnuje...
					}
					break;
				case TP_TRACK:
					if (n->iNumVerts) //bo tory zabezpieczające są niewidoczne
						n->pTrack->RaArrayFill(m_pVNT + n->iVboPtr, m_pVNT);
					break;
				case TP_TRACTION:
					//-if (n->iNumVerts) //druty mogą być niewidoczne...?
					//-	n->Traction->RaArrayFill(m_pVNT + n->iVboPtr);
					break;
			}
			n = n->nNext2; //następny z sektora
		}
		BuildVBOs();
	}
	if (Global::bManageNodes)
		ResourceManager::Register(this); //dodanie do automatu zwalniającego pamięć
}


bool TSubRect::StartVBO()
{//początek rysowania elementów z VBO w sektorze
	SetLastUsage(Timer::GetSimulationTime()); //te z tyłu będą niepotrzebnie zwalniane
	return CMesh::StartVBO();
};

// ********************************************************************************************************************
// void TSubRect::Release()
// ********************************************************************************************************************
void TSubRect::Release()
{//wirtualne zwolnienie zasobów przez sprzątacz albo destruktor
	if (Global::bUseVBO)
		CMesh::Clear(); //usuwanie buforów
};


void TSubRect::RenderDL()
{//renderowanie nieprzezroczystych (DL)
	TGroundNode *node;
	RaAnimate(); //przeliczenia animacji torów w sektorze
	for (node = nRender; node; node = node->nNext3)
		node->RenderDL(); //nieprzezroczyste obiekty (oprócz pojazdów)
	for (node = nRenderMixed; node; node = node->nNext3)
		node->RenderDL(); //nieprzezroczyste z mieszanych modeli
	for (int j = 0; j<iTracks; ++j)
		tTracks[j]->RenderDyn(); //nieprzezroczyste fragmenty pojazdów na torach
};

void TSubRect::RenderAlphaDL()
{//renderowanie przezroczystych modeli oraz pojazdów (DL)
	TGroundNode *node;
	for (node = nRenderMixed; node; node = node->nNext3)
		node->RenderAlphaDL(); //przezroczyste z mieszanych modeli
	for (node = nRenderAlpha; node; node = node->nNext3)
		node->RenderAlphaDL(); //przezroczyste modele
	//for (node=tmp->nRender;node;node=node->nNext3)
	// if (node->iType==TP_TRACK)
	//  node->pTrack->RenderAlpha(); //przezroczyste fragmenty pojazdów na torach
	for (int j = 0; j<iTracks; ++j)
		tTracks[j]->RenderDynAlpha(); //przezroczyste fragmenty pojazdów na torach
};

void TSubRect::RenderVBO()
{//renderowanie nieprzezroczystych (VBO)
	TGroundNode *node;
	RaAnimate(); //przeliczenia animacji torów w sektorze
	LoadNodes(); //czemu tutaj?
	if (StartVBO())
	{
		for (node = nRenderRect; node; node = node->nNext3)
			if (node->iVboPtr >= 0)
				node->RenderVBO(); //nieprzezroczyste obiekty terenu
		EndVBO();
	}
	for (node = nRender; node; node = node->nNext3)
		node->RenderVBO(); //nieprzezroczyste obiekty (oprócz pojazdów)
	for (node = nRenderMixed; node; node = node->nNext3)
		node->RenderVBO(); //nieprzezroczyste z mieszanych modeli
	for (int j = 0; j<iTracks; ++j)
		tTracks[j]->RenderDyn(); //nieprzezroczyste fragmenty pojazdów na torach
};

void TSubRect::RenderAlphaVBO()
{//renderowanie przezroczystych modeli oraz pojazdów (VBO)
	TGroundNode *node;
	for (node = nRenderMixed; node; node = node->nNext3)
		node->RenderAlphaVBO(); //przezroczyste z mieszanych modeli
	for (node = nRenderAlpha; node; node = node->nNext3)
		node->RenderAlphaVBO(); //przezroczyste modele
	//for (node=tmp->nRender;node;node=node->nNext3)
	// if (node->iType==TP_TRACK)
	//  node->pTrack->RenderAlpha(); //przezroczyste fragmenty pojazdów na torach
	for (int j = 0; j<iTracks; ++j)
		tTracks[j]->RenderDynAlpha(); //przezroczyste fragmenty pojazdów na torach
};



//---------------------------------------------------------------------------
//------------------ Kwadrat kilometrowy ------------------------------------
//---------------------------------------------------------------------------
int TGroundRect::iFrameNumber = 0; //licznik wyświetlanych klatek

TGroundRect::TGroundRect()
{
	pSubRects = NULL;
	nTerrain = NULL;
};

TGroundRect::~TGroundRect()
{
	SafeDeleteArray(pSubRects);
};

void TGroundRect::RenderDL()
{//renderowanie kwadratu kilometrowego (DL), jeśli jeszcze nie zrobione
	if (iLastDisplay != iFrameNumber)
	{//tylko jezeli dany kwadrat nie był jeszcze renderowany
		//for (TGroundNode* node=pRender;node;node=node->pNext3)
		// node->Render(); //nieprzezroczyste trójkąty kwadratu kilometrowego
		if (nRender)
		{//łączenie trójkątów w jedną listę - trochę wioska
			if (!nRender->DisplayListID || (nRender->iVersion != Global::iReCompile))
			{//jeżeli nie skompilowany, kompilujemy wszystkie trójkąty w jeden
				nRender->fSquareRadius = 5000.0*5000.0; //aby agregat nigdy nie znikał
				nRender->DisplayListID = glGenLists(1);
				glNewList(nRender->DisplayListID, GL_COMPILE);
				nRender->iVersion = Global::iReCompile; //aktualna wersja siatek
				for (TGroundNode* node = nRender; node; node = node->nNext3) //następny tej grupy
					node->Compile(true);
				glEndList();
			}
			nRender->RenderDL(); //nieprzezroczyste trójkąty kwadratu kilometrowego
		}
		if (nRootMesh)
			nRootMesh->RenderDL();
		iLastDisplay = iFrameNumber; //drugi raz nie potrzeba
	}
};

void TGroundRect::RenderVBO()
{//renderowanie kwadratu kilometrowego (VBO), jeśli jeszcze nie zrobione
	if (iLastDisplay != iFrameNumber)
	{//tylko jezeli dany kwadrat nie był jeszcze renderowany
		LoadNodes(); //ewentualne tworzenie siatek
		if (StartVBO())
		{
			for (TGroundNode* node = nRenderRect; node; node = node->nNext3) //następny tej grupy
				node->RaRenderVBO(); //nieprzezroczyste trójkąty kwadratu kilometrowego
			EndVBO();
			iLastDisplay = iFrameNumber;
		}
		//-if (nTerrain)
		//-	nTerrain->smTerrain->iVisible = iFrameNumber; //ma się wyświetlić w tej ramce
		// TODO:
	}
};



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void TGround::MoveGroundNode(vector3 pPosition)
{//Ra: to wymaga gruntownej reformy
	/*
	TGroundNode *Current;
	for (Current=RootNode;Current!=NULL;Current=Current->Next)
	Current->MoveMe(pPosition);

	TGroundRect *Rectx=new TGroundRect; //kwadrat kilometrowy
	for(int i=0;i<iNumRects;i++)
	for(int j=0;j<iNumRects;j++)
	Rects[i][j]=*Rectx; //kopiowanie zawartości do każdego kwadratu
	delete Rectx;
	for (Current=RootNode;Current!=NULL;Current=Current->Next)
	{//rozłożenie obiektów na mapie
	if (Current->iType!=TP_DYNAMIC)
	{//pojazdów to w ogóle nie dotyczy
	if ((Current->iType!=GL_TRIANGLES)&&(Current->iType!=GL_TRIANGLE_STRIP)?true //~czy trójkąt?
	:(Current->iFlags&0x20)?true //~czy teksturę ma nieprzezroczystą?
	//:(Current->iNumVerts!=3)?true //~czy tylko jeden trójkąt?
	:(Current->fSquareMinRadius!=0.0)?true //~czy widoczny z bliska?
	:(Current->fSquareRadius<=90000.0)) //~czy widoczny z daleka?
	GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
	else //dodajemy do kwadratu kilometrowego
	GetRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
	}
	}
	for (Current=RootDynamic;Current!=NULL;Current=Current->Next)
	{
	Current->pCenter+=pPosition;
	Current->DynamicObject->UpdatePos();
	}
	for (Current=RootDynamic;Current!=NULL;Current=Current->Next)
	Current->DynamicObject->MoverParameters->Physic_ReActivation();
	*/
}

TGround::TGround()
{
	//RootNode=NULL;
	nRootDynamic = NULL;
	QueryRootEvent = NULL;
	tmpEvent = NULL;
	tmp2Event = NULL;
	OldQRE = NULL;
	RootEvent = NULL;
	iNumNodes = 0;
	//pTrain=NULL;
	Global::pGround = this;
	bInitDone = false; //Ra: żeby nie robiło dwa razy FirstInit
	for (int i = 0; i<TP_LAST; i++)
		nRootOfType[i] = NULL; //zerowanie tablic wyszukiwania
	bDynamicRemove = false; //na razie nic do usunięcia
	sTracks = new TNames(); //nazwy torów - na razie tak
}

TGround::~TGround()
{
	Free();
}

void TGround::Free()
{
	TEvent *tmp;
//-	for (TEvent *Current = RootEvent; Current;)
//-	{
//-		tmp = Current;
//-		Current = Current->Next2;
//-		delete tmp;
//-	}
	TGroundNode *tmpn;
	for (int i = 0; i<TP_LAST; ++i)
	{
		for (TGroundNode *Current = nRootOfType[i]; Current;)
		{
			tmpn = Current;
			Current = Current->Next;
			delete tmpn;
		}
		nRootOfType[i] = NULL;
	}
	for (TGroundNode *Current = nRootDynamic; Current;)
	{
		tmpn = Current;
		Current = Current->Next;
		delete tmpn;
	}
	iNumNodes = 0;
	//RootNode=NULL;
	nRootDynamic = NULL;
	delete sTracks;
}

TGroundNode* TGround::FindGroundNode(std::string asNameToFind, TGroundNodeType iNodeType)
{//wyszukiwanie obiektu o podanej nazwie i konkretnym typie
	if ((iNodeType == TP_TRACK) || (iNodeType == TP_MEMCELL) || (iNodeType == TP_MODEL))
	{//wyszukiwanie w drzewie binarnym
		return (TGroundNode*)sTracks->Find(iNodeType, asNameToFind.c_str());
	}
	//standardowe wyszukiwanie liniowe
	TGroundNode *Current;
	for (Current = nRootOfType[iNodeType]; Current; Current = Current->Next)
		if (Current->asName == asNameToFind)
			return Current;
	return NULL;
}



double fTrainSetVel = 0;
double fTrainSetDir = 0;
double fTrainSetDist = 0; //odległość składu od punktu 1 w stronę punktu 2
std::string asTrainSetTrack = "";
int iTrainSetConnection = 0;
bool bTrainSet = false;
std::string asTrainName = "";
int iTrainSetWehicleNumber = 0;
TGroundNode *TrainSetNode = NULL; //poprzedni pojazd do łączenia
TGroundNode *TrainSetDriver = NULL; //pojazd, któremu zostanie wysłany rozkład

TGroundVertex TempVerts[10000]; //tu wczytywane są trójkąty
byte TempConnectionType[200]; //Ra: sprzęgi w składzie; ujemne, gdy odwrotnie



void TGround::RaTriangleDivider(TGroundNode* node)
{//tworzy dodatkowe trójkąty i zmiejsza podany
	//to jest wywoływane przy wczytywaniu trójkątów
	//dodatkowe trójkąty są dodawane do głównej listy trójkątów
	//podział trójkątów na sektory i kwadraty jest dokonywany później w FirstInit
	if (node->iType != GL_TRIANGLES) return; //tylko pojedyncze trójkąty
	if (node->iNumVerts != 3) return; //tylko gdy jeden trójkąt
	double x0 = 1000.0*floor(0.001*node->pCenter.x) - 200.0; double x1 = x0 + 1400.0;
	double z0 = 1000.0*floor(0.001*node->pCenter.z) - 200.0; double z1 = z0 + 1400.0;
	if (
		(node->Vertices[0].Point.x >= x0) && (node->Vertices[0].Point.x <= x1) &&
		(node->Vertices[0].Point.z >= z0) && (node->Vertices[0].Point.z <= z1) &&
		(node->Vertices[1].Point.x >= x0) && (node->Vertices[1].Point.x <= x1) &&
		(node->Vertices[1].Point.z >= z0) && (node->Vertices[1].Point.z <= z1) &&
		(node->Vertices[2].Point.x >= x0) && (node->Vertices[2].Point.x <= x1) &&
		(node->Vertices[2].Point.z >= z0) && (node->Vertices[2].Point.z <= z1))
		return; //trójkąt wystający mniej niż 200m z kw. kilometrowego jest do przyjęcia
	//Ra: przerobić na dzielenie na 2 trójkąty, podział w przecięciu z siatką kilometrową
	//Ra: i z rekurencją będzie dzielić trzy trójkąty, jeśli będzie taka potrzeba
	int divide = -1; //bok do podzielenia: 0=AB, 1=BC, 2=CA; +4=podział po OZ; +8 na x1/z1
	double min = 0, mul; //jeśli przechodzi przez oś, iloczyn będzie ujemny
	x0 += 200.0; x1 -= 200.0; //przestawienie na siatkę
	z0 += 200.0; z1 -= 200.0;
	mul = (node->Vertices[0].Point.x - x0)*(node->Vertices[1].Point.x - x0); //AB na wschodzie
	if (mul<min) min = mul, divide = 0;
	mul = (node->Vertices[1].Point.x - x0)*(node->Vertices[2].Point.x - x0); //BC na wschodzie
	if (mul<min) min = mul, divide = 1;
	mul = (node->Vertices[2].Point.x - x0)*(node->Vertices[0].Point.x - x0); //CA na wschodzie
	if (mul<min) min = mul, divide = 2;
	mul = (node->Vertices[0].Point.x - x1)*(node->Vertices[1].Point.x - x1); //AB na zachodzie
	if (mul<min) min = mul, divide = 8;
	mul = (node->Vertices[1].Point.x - x1)*(node->Vertices[2].Point.x - x1); //BC na zachodzie
	if (mul<min) min = mul, divide = 9;
	mul = (node->Vertices[2].Point.x - x1)*(node->Vertices[0].Point.x - x1); //CA na zachodzie
	if (mul<min) min = mul, divide = 10;
	mul = (node->Vertices[0].Point.z - z0)*(node->Vertices[1].Point.z - z0); //AB na południu
	if (mul<min) min = mul, divide = 4;
	mul = (node->Vertices[1].Point.z - z0)*(node->Vertices[2].Point.z - z0); //BC na południu
	if (mul<min) min = mul, divide = 5;
	mul = (node->Vertices[2].Point.z - z0)*(node->Vertices[0].Point.z - z0); //CA na południu
	if (mul<min) min = mul, divide = 6;
	mul = (node->Vertices[0].Point.z - z1)*(node->Vertices[1].Point.z - z1); //AB na północy
	if (mul<min) min = mul, divide = 12;
	mul = (node->Vertices[1].Point.z - z1)*(node->Vertices[2].Point.z - z1); //BC na północy
	if (mul<min) min = mul, divide = 13;
	mul = (node->Vertices[2].Point.z - z1)*(node->Vertices[0].Point.z - z1); //CA na północy
	if (mul<min) divide = 14;
	//tworzymy jeden dodatkowy trójkąt, dzieląc jeden bok na przecięciu siatki kilometrowej
	TGroundNode* ntri; //wskaźnik na nowy trójkąt
	ntri = new TGroundNode(); //a ten jest nowy
	ntri->iType = GL_TRIANGLES; //kopiowanie parametrów, przydałby się konstruktor kopiujący
	ntri->Init(3);
	ntri->TextureID = node->TextureID;
	ntri->iFlags = node->iFlags;
	for (int j = 0; j<4; ++j)
	{
		ntri->Ambient[j] = node->Ambient[j];
		ntri->Diffuse[j] = node->Diffuse[j];
		ntri->Specular[j] = node->Specular[j];
	}
	ntri->asName = node->asName;
	ntri->fSquareRadius = node->fSquareRadius;
	ntri->fSquareMinRadius = node->fSquareMinRadius;
	ntri->bVisible = node->bVisible; //a są jakieś niewidoczne?
	ntri->Next = nRootOfType[GL_TRIANGLES];
	nRootOfType[GL_TRIANGLES] = ntri; //dopisanie z przodu do listy
	iNumNodes++;
	switch (divide & 3)
	{//podzielenie jednego z boków, powstaje wierzchołek D
	case 0: //podział AB (0-1) -> ADC i DBC
		ntri->Vertices[2] = node->Vertices[2]; //wierzchołek C jest wspólny
		ntri->Vertices[1] = node->Vertices[1]; //wierzchołek B przechodzi do nowego
		//node->Vertices[1].HalfSet(node->Vertices[0],node->Vertices[1]); //na razie D tak
		if (divide & 4)
			node->Vertices[1].SetByZ(node->Vertices[0], node->Vertices[1], divide & 8 ? z1 : z0);
		else
			node->Vertices[1].SetByX(node->Vertices[0], node->Vertices[1], divide & 8 ? x1 : x0);
		ntri->Vertices[0] = node->Vertices[1]; //wierzchołek D jest wspólny
		break;
	case 1: //podział BC (1-2) -> ABD i ADC
		ntri->Vertices[0] = node->Vertices[0]; //wierzchołek A jest wspólny
		ntri->Vertices[2] = node->Vertices[2]; //wierzchołek C przechodzi do nowego
		//node->Vertices[2].HalfSet(node->Vertices[1],node->Vertices[2]); //na razie D tak
		if (divide & 4)
			node->Vertices[2].SetByZ(node->Vertices[1], node->Vertices[2], divide & 8 ? z1 : z0);
		else
			node->Vertices[2].SetByX(node->Vertices[1], node->Vertices[2], divide & 8 ? x1 : x0);
		ntri->Vertices[1] = node->Vertices[2]; //wierzchołek D jest wspólny
		break;
	case 2: //podział CA (2-0) -> ABD i DBC
		ntri->Vertices[1] = node->Vertices[1]; //wierzchołek B jest wspólny
		ntri->Vertices[2] = node->Vertices[2]; //wierzchołek C przechodzi do nowego
		//node->Vertices[2].HalfSet(node->Vertices[2],node->Vertices[0]); //na razie D tak
		if (divide & 4)
			node->Vertices[2].SetByZ(node->Vertices[2], node->Vertices[0], divide & 8 ? z1 : z0);
		else
			node->Vertices[2].SetByX(node->Vertices[2], node->Vertices[0], divide & 8 ? x1 : x0);
		ntri->Vertices[0] = node->Vertices[2]; //wierzchołek D jest wspólny
		break;
	}
	//przeliczenie środków ciężkości obu
	node->pCenter = (node->Vertices[0].Point + node->Vertices[1].Point + node->Vertices[2].Point) / 3.0;
	ntri->pCenter = (ntri->Vertices[0].Point + ntri->Vertices[1].Point + ntri->Vertices[2].Point) / 3.0;
	RaTriangleDivider(node); //rekurencja, bo nawet na TD raz nie wystarczy
	RaTriangleDivider(ntri);
};



TGroundNode* TGround::AddGroundNode(cParser* parser)
{//wczytanie wpisu typu "node"
	//parser->LoadTraction=Global::bLoadTraction; //Ra: tu nie potrzeba powtarzać
	std::string str, str1, str2, str3, Skin, DriverType, asNodeName;
	int nv, ti, i, n;
	double tf, r, rmin, tf1, tf2, tf3, tf4, l, dist, mgn;
	int int1, int2;
	bool bError = false, curve;
	vector3 pt, front, up, left, pos, tv;
	matrix4x4 mat2, mat1, mat;
	GLuint TexID;
	TGroundNode *tmp1;
	TTrack *Track;
	TRealSound *tmpsound;
	std::string token;
	parser->getTokens(2);
	*parser >> r >> rmin;
	parser->getTokens();
	*parser >> token;
	asNodeName = token.c_str();
	parser->getTokens();
	*parser >> token;
	str = token.c_str();
	TGroundNode *tmp, *tmp2;
	tmp = new TGroundNode();
	tmp->asName = (asNodeName == "none" ? "" : asNodeName);
	if (r >= 0) tmp->fSquareRadius = r*r;
	tmp->fSquareMinRadius = rmin*rmin;
	if (str == "triangles")           tmp->iType = GL_TRIANGLES;
	else if (str == "triangle_strip")      tmp->iType = GL_TRIANGLE_STRIP;
	else if (str == "triangle_fan")        tmp->iType = GL_TRIANGLE_FAN;
	else if (str == "lines")               tmp->iType = GL_LINES;
	else if (str == "line_strip")          tmp->iType = GL_LINE_STRIP;
	else if (str == "line_loop")           tmp->iType = GL_LINE_LOOP;
	else if (str == "model")               tmp->iType = TP_MODEL;
	//else if (str=="terrain")             tmp->iType=TP_TERRAIN; //tymczasowo do odwołania
	else if (str == "dynamic")             tmp->iType = TP_DYNAMIC;
	else if (str == "sound")               tmp->iType = TP_SOUND;
	else if (str == "track")               tmp->iType = TP_TRACK;
	else if (str == "memcell")             tmp->iType = TP_MEMCELL;
	else if (str == "eventlauncher")       tmp->iType = TP_EVLAUNCH;
	else if (str == "traction")            tmp->iType = TP_TRACTION;
	else if (str == "tractionpowersource") tmp->iType = TP_TRACTIONPOWERSOURCE;
	// else if (str=="isolated")            tmp->iType=TP_ISOLATED;
	else bError = true;
	//WriteLog("-> node "+str+" "+tmp->asName);
	if (bError)
	{
		WriteLogSS("Scene parse error near " + str, "ERROR");
		for (int i = 0; i<60; ++i)
		{//Ra: skopiowanie dalszej części do logu - taka prowizorka, lepsza niż nic
			parser->getTokens(); //pobranie linijki tekstu nie działa
			*parser >> token;
			WriteLog(token.c_str());
		}
		//if (tmp==RootNode) RootNode=NULL;
		delete tmp;
		return NULL;
	}
	switch (tmp->iType)
	{
/*
	case TP_TRACTION:
		tmp->Traction = new TTraction();
		parser->getTokens();
		*parser >> token;
		tmp->Traction->asPowerSupplyName = token.c_str();
		parser->getTokens(3);
		*parser >> tmp->Traction->NominalVoltage >> tmp->Traction->MaxCurrent >> tmp->Traction->Resistivity;
		parser->getTokens();
		*parser >> token;
		if (token.compare("cu") == 0)
			tmp->Traction->Material = 1;
		else if (token.compare("al") == 0)
			tmp->Traction->Material = 2;
		else
			tmp->Traction->Material = 0;
		parser->getTokens();
		*parser >> tmp->Traction->WireThickness;
		parser->getTokens();
		*parser >> tmp->Traction->DamageFlag;
		parser->getTokens(3);
		*parser >> tmp->Traction->pPoint1.x >> tmp->Traction->pPoint1.y >> tmp->Traction->pPoint1.z;
		tmp->Traction->pPoint1 += pOrigin;
		parser->getTokens(3);
		*parser >> tmp->Traction->pPoint2.x >> tmp->Traction->pPoint2.y >> tmp->Traction->pPoint2.z;
		tmp->Traction->pPoint2 += pOrigin;
		parser->getTokens(3);
		*parser >> tmp->Traction->pPoint3.x >> tmp->Traction->pPoint3.y >> tmp->Traction->pPoint3.z;
		tmp->Traction->pPoint3 += pOrigin;
		parser->getTokens(3);
		*parser >> tmp->Traction->pPoint4.x >> tmp->Traction->pPoint4.y >> tmp->Traction->pPoint4.z;
		tmp->Traction->pPoint4 += pOrigin;
		parser->getTokens();
		*parser >> tf1;
		tmp->Traction->fHeightDifference =
			(tmp->Traction->pPoint3.y - tmp->Traction->pPoint1.y +
			tmp->Traction->pPoint4.y - tmp->Traction->pPoint2.y)*0.5f - tf1;
		parser->getTokens();
		*parser >> tf1;
		if (tf1>0)
			tmp->Traction->iNumSections = (tmp->Traction->pPoint1 - tmp->Traction->pPoint2).Length() / tf1;
		else tmp->Traction->iNumSections = 0;
		parser->getTokens();
		*parser >> tmp->Traction->Wires;
		parser->getTokens();
		*parser >> tmp->Traction->WireOffset;
		parser->getTokens();
		*parser >> token;
		tmp->bVisible = (token.compare("vis") == 0);
		parser->getTokens();
		*parser >> token;
		if (token.compare("endtraction") != 0)
			Error("ENDTRACTION delimiter missing! " + str2 + " found instead.");
		tmp->Traction->Init(); //przeliczenie parametrów
		if (Global::bLoadTraction)
			tmp->Traction->Optimize();
		tmp->pCenter = (tmp->Traction->pPoint2 + tmp->Traction->pPoint1)*0.5f;
		//if (!Global::bLoadTraction) SafeDelete(tmp); //Ra: tak być nie może, bo NULL to błąd
		break;

	case TP_TRACTIONPOWERSOURCE:
		parser->getTokens(3);
		*parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
		tmp->pCenter += pOrigin;
		tmp->TractionPowerSource = new TTractionPowerSource();
		tmp->TractionPowerSource->Load(parser);
		break;
	case TP_MEMCELL:
		parser->getTokens(3);
		*parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
		tmp->pCenter += pOrigin;
		tmp->MemCell = new TMemCell(&tmp->pCenter);
		tmp->MemCell->Load(parser);
		if (!tmp->asName.IsEmpty()) //jest pusta gdy "none"
		{//dodanie do wyszukiwarki
			if (sTracks->Update(TP_MEMCELL, tmp->asName.c_str(), tmp)) //najpierw sprawdzić, czy już jest
			{//przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność wsteczna)
				ErrorLog("Duplicated memcell: " + tmp->asName); //to zgłaszać duplikat
			}
			else
				sTracks->Add(TP_MEMCELL, tmp->asName.c_str(), tmp); //nazwa jest unikalna
		}
		break;
	case TP_EVLAUNCH:
		parser->getTokens(3);
		*parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
		tmp->pCenter += pOrigin;
		tmp->EvLaunch = new TEventLauncher();
		tmp->EvLaunch->Load(parser);
		break;
*/
	case TP_TRACK:
		tmp->pTrack = new TTrack(tmp);
		if (Global::iWriteLogEnabled & 4)
			if (!tmp->asName.empty())
				WriteLog(tmp->asName.c_str());
		tmp->pTrack->Load(parser, pOrigin, tmp->asName); //w nazwie może być nazwa odcinka izolowanego
		if (!tmp->asName.empty()) //jest pusta gdy "none"
		{//dodanie do wyszukiwarki
			if (sTracks->Update(TP_TRACK, tmp->asName.c_str(), tmp)) //najpierw sprawdzić, czy już jest
			{//przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność wsteczna)
				if (tmp->pTrack->iCategoryFlag & 1) //jeśli jest zdublowany tor kolejowy
					WriteLogSS("Duplicated track: " + tmp->asName, "ERROR"); //to zgłaszać duplikat
			}
			else
				sTracks->Add(TP_TRACK, tmp->asName.c_str(), tmp); //nazwa jest unikalna
		}
		tmp->pCenter = (tmp->pTrack->CurrentSegment()->FastGetPoint_0() +
			tmp->pTrack->CurrentSegment()->FastGetPoint(0.5) +
			tmp->pTrack->CurrentSegment()->FastGetPoint_1()) / 3.0;
		break;
	case TP_SOUND:
		tmp->pStaticSound = new TRealSound;
		parser->getTokens(3);
		*parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
		tmp->pCenter += pOrigin;
		parser->getTokens();
		*parser >> token;
		str = token.c_str();
//-		tmp->pStaticSound->Init(str.c_str(), sqrt(tmp->fSquareRadius), tmp->pCenter.x, tmp->pCenter.y, tmp->pCenter.z, false);


		parser->getTokens(); *parser >> token;
		break;
/*
	case TP_DYNAMIC:
		tmp->DynamicObject = new TDynamicObject();
		//tmp->DynamicObject->Load(Parser);
		parser->getTokens();
		*parser >> token;
		str1 = token.c_str(); //katalog
		//McZapkie: doszedl parametr ze zmienialna skora
		parser->getTokens();
		*parser >> token;
		Skin = token.c_str(); //tekstura wymienna
		parser->getTokens();
		*parser >> token;
		str3 = token.c_str(); //McZapkie-131102: model w MMD
		if (bTrainSet)
		{//jeśli pojazd jest umieszczony w składzie
			str = asTrainSetTrack;
			parser->getTokens();
			*parser >> tf1; //Ra: -1 oznacza odwrotne wstawienie, normalnie w składzie 0
			parser->getTokens();
			*parser >> token;
			DriverType = token.c_str(); //McZapkie:010303 - w przyszlosci rozne konfiguracje mechanik/pomocnik itp
			tf3 = fTrainSetVel; //prędkość
			parser->getTokens();
			*parser >> int1;
			if (int1<0) int1 = (-int1) | ctrain_depot; //sprzęg zablokowany (pojazdy nierozłączalne przy manewrach)
			if (tf1 != -1.0)
				if (fabs(tf1)>0.5) //maksymalna odległość między sprzęgami - do przemyślenia
					int1 = 0; //likwidacja sprzęgu, jeśli odległość zbyt duża - to powinno być uwzględniane w fizyce sprzęgów...
			TempConnectionType[iTrainSetWehicleNumber] = int1; //wartość dodatnia
		}
		else
		{//pojazd wstawiony luzem
			fTrainSetDist = 0; //zerowanie dodatkowego przesunięcia
			asTrainName = ""; //puste oznacza jazdę pojedynczego bez rozkładu, "none" jest dla składu (trainset)
			parser->getTokens();
			*parser >> token;
			str = token.c_str(); //track
			parser->getTokens();
			*parser >> tf1; //Ra: -1 oznacza odwrotne wstawienie
			parser->getTokens();
			*parser >> token;
			DriverType = token.c_str(); //McZapkie:010303: obsada
			parser->getTokens();
			*parser >> tf3; //prędkość
			iTrainSetWehicleNumber = 0;
		}
		parser->getTokens();
		*parser >> int2; //ilość ładunku
		if (int2>0)
		{//jeżeli ładunku jest więcej niż 0, to rozpoznajemy jego typ
			parser->getTokens();
			*parser >> token;
			str2 = token.c_str();  //LoadType
			if (str2 == "enddynamic") //idiotoodporność: ładunek bez podanego typu
			{
				str2 = ""; int2 = 0; //ilość bez typu się nie liczy jako ładunek
			}
		}
		else
			str2 = "";  //brak ladunku

		tmp1 = FindGroundNode(str, TP_TRACK); //poszukiwanie toru
		if (tmp1 ? tmp1->pTrack != NULL : false)
		{//jeśli tor znaleziony
			Track = tmp1->pTrack;
			if (!iTrainSetWehicleNumber) //jeśli pierwszy pojazd
				if (Track->Event0) //jeśli tor ma Event0
					if (fabs(fTrainSetVel) <= 1.0) //a skład stoi
						if (fTrainSetDist >= 0.0) //ale może nie sięgać na owy tor
							if (fTrainSetDist<8.0) //i raczej nie sięga
								fTrainSetDist = 8.0; //przesuwamy około pół EU07 dla wstecznej zgodności
			//WriteLog("Dynamic shift: "+AnsiString(fTrainSetDist));

   		    tf3 = tmp->DynamicObject->Init(asNodeName, str1, Skin, str3, Track, (tf1 == -1.0 ? fTrainSetDist : fTrainSetDist - tf1), DriverType, tf3, asTrainName, int2, str2, (tf1 == -1.0));
			if (tf3 != 0.0) //zero oznacza błąd
			{
				fTrainSetDist -= tf3; //przesunięcie dla kolejnego, minus bo idziemy w stronę punktu 1
				tmp->pCenter = tmp->DynamicObject->GetPosition();
				if (TempConnectionType[iTrainSetWehicleNumber]) //jeśli jest sprzęg
					if (tmp->DynamicObject->MoverParameters->Couplers[tf1 == -1.0 ? 0 : 1].AllowedFlag&ctrain_depot) //jesli zablokowany
						TempConnectionType[iTrainSetWehicleNumber] |= ctrain_depot; //będzie blokada
				iTrainSetWehicleNumber++;
			}
			else
			{//LastNode=NULL;
				delete tmp;
				tmp = NULL; //nie może być tu return, bo trzeba pominąć jeszcze enddynamic
			}
		}
		else
		{//gdy tor nie znaleziony
			WriteLogSS("Missed track: dynamic placed on " + tmp->DynamicObject->asTrack, "ERROR" );
			delete tmp;
			tmp = NULL; //nie może być tu return, bo trzeba pominąć jeszcze enddynamic
		}
		parser->getTokens();
		*parser >> token;
		if (token.compare("destination") == 0)
		{//dokąd wagon ma jechać, uwzględniane przy manewrach
			parser->getTokens();
			*parser >> token;
			if (tmp)
				tmp->DynamicObject->asDestination = token.c_str();
			*parser >> token;
		}
		if (token.compare("enddynamic") != 0)
			WriteLogSS("enddynamic statement missing", "ERROR");
		break;
		*/
		//case TP_TERRAIN: //TODO: zrobić jak zwykły, rozróżnienie po nazwie albo czymś innym
	case TP_MODEL:
		if (rmin<0)
		{
			tmp->iType = TP_TERRAIN;
			tmp->fSquareMinRadius = 0; //to w ogóle potrzebne?
		}
		parser->getTokens(3);
		*parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
		parser->getTokens();
		*parser >> tf1;
		//OlO_EU&KAKISH-030103: obracanie punktow zaczepien w modelu
		tmp->pCenter.RotateY(aRotate.y / 180.0*M_PI);
		//McZapkie-260402: model tez ma wspolrzedne wzgledne
		tmp->pCenter += pOrigin;
		//tmp->fAngle+=aRotate.y; // /180*M_PI
		/*
		if (tmp->iType==TP_MODEL)
		{//jeśli standardowy model
		*/
		tmp->Model = new TAnimModel();
		tmp->Model->RaAnglesSet(aRotate.x, tf1 + aRotate.y, aRotate.z); //dostosowanie do pochylania linii
		if (tmp->Model->Load(parser, tmp->iType == TP_TERRAIN)) //wczytanie modelu, tekstury i stanu świateł...
			tmp->iFlags = tmp->Model->Flags() | 0x200; //ustalenie, czy przezroczysty; flaga usuwania
		else
			if (tmp->iType != TP_TERRAIN)
			{//model nie wczytał się - ignorowanie node
				delete tmp;
				tmp = NULL; //nie może być tu return
				break; //nie może być tu return?
			}
		/*
		}
		else if (tmp->iType==TP_TERRAIN)
		{//nie potrzeba nakładki animującej submodele
		*parser >> token;
		tmp->pModel3D=TModelsManager::GetModel(token.c_str(),false);
		do //Ra: z tym to trochę bez sensu jest
		{parser->getTokens();
		*parser >> token;
		str=AnsiString(token.c_str());
		} while (str!="endterrains");
		}
		*/
		if (tmp->iType == TP_TERRAIN)
		{//jeśli model jest terenem, trzeba utworzyć dodatkowe obiekty
			//po wczytaniu model ma już utworzone DL albo VBO
			Global::pTerrainCompact = tmp->Model; //istnieje co najmniej jeden obiekt terenu
			//-tmp->iCount = Global::pTerrainCompact->TerrainCount() + 1; //zliczenie submodeli
			tmp->nNode = new TGroundNode[tmp->iCount]; //sztuczne node dla kwadratów
			tmp->nNode[0].iType = TP_MODEL; //pierwszy zawiera model (dla delete)
			tmp->nNode[0].Model = Global::pTerrainCompact;
			tmp->nNode[0].iFlags = 0x200; //nie wyświetlany, ale usuwany
			for (i = 1; i<tmp->iCount; ++i)
			{//a reszta to submodele
				tmp->nNode[i].iType = TP_SUBMODEL; //
				//-tmp->nNode[i].smTerrain = Global::pTerrainCompact->TerrainSquare(i - 1);
				tmp->nNode[i].iFlags = 0x10; //nieprzezroczyste; nie usuwany
				tmp->nNode[i].bVisible = true;
				tmp->nNode[i].pCenter = tmp->pCenter; //nie przesuwamy w inne miejsce
				//tmp->nNode[i].asName=
			}
		}
		else if (!tmp->asName.empty()) //jest pusta gdy "none"
		{//dodanie do wyszukiwarki
			if (sTracks->Update(TP_MODEL, tmp->asName.c_str(), tmp)) //najpierw sprawdzić, czy już jest
			{//przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność wsteczna)
				WriteLogSS("Duplicated model: " + tmp->asName, "ERROR"); //to zgłaszać duplikat
			}
			else
				sTracks->Add(TP_MODEL, tmp->asName.c_str(), tmp); //nazwa jest unikalna
		}
		//str=Parser->GetNextSymbol().LowerCase();
		break;
		//case TP_GEOMETRY :
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
		parser->getTokens();
		*parser >> token;
		//McZapkie-050702: opcjonalne wczytywanie parametrow materialu (ambient,diffuse,specular)
		if (token.compare("material") == 0)
		{
			parser->getTokens();
			*parser >> token;
			while (token.compare("endmaterial") != 0)
			{
				if (token.compare("ambient:") == 0)
				{
					parser->getTokens(); *parser >> tmp->Ambient[0];
					parser->getTokens(); *parser >> tmp->Ambient[1];
					parser->getTokens(); *parser >> tmp->Ambient[2];
				}
				else if (token.compare("diffuse:") == 0)
				{//Ra: coś jest nie tak, bo w jednej linijce nie działa
					parser->getTokens(); *parser >> tmp->Diffuse[0];
					parser->getTokens(); *parser >> tmp->Diffuse[1];
					parser->getTokens(); *parser >> tmp->Diffuse[2];
				}
				else if (token.compare("specular:") == 0)
				{
					parser->getTokens(); *parser >> tmp->Specular[0];
					parser->getTokens(); *parser >> tmp->Specular[1];
					parser->getTokens(); *parser >> tmp->Specular[2];
				}
				else WriteLogSS("Scene material failure!", "ERROR");
				parser->getTokens();
				*parser >> token;
			}
		}
		if (token.compare("endmaterial") == 0)
		{
			parser->getTokens();
			*parser >> token;
		}
		str = token.c_str();
#ifdef _PROBLEND
		// PROBLEND Q: 13122011 - Szociu: 27012012
		PROBLEND = true;     // domyslnie uruchomione nowe wyświetlanie
		tmp->PROBLEND = true;  // odwolanie do tgroundnode, bo rendering jest w tej klasie
		if (str.find("@") > 0)     // sprawdza, czy w nazwie tekstury jest znak "@"
		{
			PROBLEND = false;     // jeśli jest, wyswietla po staremu
			tmp->PROBLEND = false;
		}
#endif
		tmp->TextureID = TTexturesManager::GetTextureID(stdstrtochar(str), false);
		tmp->iFlags = TTexturesManager::GetAlpha(tmp->TextureID) ? 0x220 : 0x210; //z usuwaniem
		/*
		if (((tmp->iType == GL_TRIANGLES) && (tmp->iFlags & 0x10)) ? Global::pTerrainCompact->TerrainLoaded() : false)
		{//jeśli jest tekstura nieprzezroczysta, a teren załadowany, to pomijamy trójkąty
			do
			{//pomijanie wtrójkątów
				parser->getTokens();
				*parser >> token;
			} while (token.compare("endtri") != 0);
			//delete tmp; //nie ma co tego trzymać
			//tmp=NULL; //to jest błąd
		}
		else
		*/
		{
			i = 0;
			do
			{
				parser->getTokens(3);
				*parser >> TempVerts[i].Point.x >> TempVerts[i].Point.y >> TempVerts[i].Point.z;
				parser->getTokens(3);
				*parser >> TempVerts[i].Normal.x >> TempVerts[i].Normal.y >> TempVerts[i].Normal.z;
				/*
				str=Parser->GetNextSymbol().LowerCase();
				if (str==AnsiString("x"))
				TempVerts[i].tu=(TempVerts[i].Point.x+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
				else
				if (str==AnsiString("y"))
				TempVerts[i].tu=(TempVerts[i].Point.y+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
				else
				if (str==AnsiString("z"))
				TempVerts[i].tu=(TempVerts[i].Point.z+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
				else
				TempVerts[i].tu=str.ToDouble();;

				str=Parser->GetNextSymbol().LowerCase();
				if (str==AnsiString("x"))
				TempVerts[i].tv=(TempVerts[i].Point.x+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
				else
				if (str==AnsiString("y"))
				TempVerts[i].tv=(TempVerts[i].Point.y+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
				else
				if (str==AnsiString("z"))
				TempVerts[i].tv=(TempVerts[i].Point.z+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
				else
				TempVerts[i].tv=str.ToDouble();;
				*/
				parser->getTokens(2);
				*parser >> TempVerts[i].tu >> TempVerts[i].tv;

				//    tf=Parser->GetNextSymbol().ToDouble();
				//          TempVerts[i].tu=tf;
				//        tf=Parser->GetNextSymbol().ToDouble();
				//      TempVerts[i].tv=tf;

				TempVerts[i].Point.RotateZ(aRotate.z / 180 * M_PI);
				TempVerts[i].Point.RotateX(aRotate.x / 180 * M_PI);
				TempVerts[i].Point.RotateY(aRotate.y / 180 * M_PI);
				TempVerts[i].Normal.RotateZ(aRotate.z / 180 * M_PI);
				TempVerts[i].Normal.RotateX(aRotate.x / 180 * M_PI);
				TempVerts[i].Normal.RotateY(aRotate.y / 180 * M_PI);
				TempVerts[i].Point += pOrigin;
				tmp->pCenter += TempVerts[i].Point;
				i++;
				parser->getTokens();
				*parser >> token;

				//   }

			} while (token.compare("endtri") != 0);
			nv = i;
			tmp->Init(nv); //utworzenie tablicy wierzchołków
			tmp->pCenter /= (nv>0 ? nv : 1);

			//   memcpy(tmp->Vertices,TempVerts,nv*sizeof(TGroundVertex));

			r = 0;
			for (int i = 0; i<nv; i++)
			{
				tmp->Vertices[i] = TempVerts[i];
				tf = SquareMagnitude(tmp->Vertices[i].Point - tmp->pCenter);
				if (tf>r) r = tf;
			}

			//   tmp->fSquareRadius=2000*2000+r;
			tmp->fSquareRadius += r;
			RaTriangleDivider(tmp); //Ra: dzielenie trójkątów jest teraz całkiem wydajne
		} //koniec wczytywania trójkątów
		break;
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
		parser->getTokens(3);
		*parser >> tmp->Diffuse[0] >> tmp->Diffuse[1] >> tmp->Diffuse[2];
		//   tmp->Diffuse[0]=Parser->GetNextSymbol().ToDouble()/255;
		//   tmp->Diffuse[1]=Parser->GetNextSymbol().ToDouble()/255;
		//   tmp->Diffuse[2]=Parser->GetNextSymbol().ToDouble()/255;
		parser->getTokens();
		*parser >> tmp->fLineThickness;
		i = 0;
		parser->getTokens();
		*parser >> token;
		do
		{
			str = token.c_str();
			TempVerts[i].Point.x = atof(str.c_str()); // TODO: CZY DOBRZE??
			parser->getTokens(2);
			*parser >> TempVerts[i].Point.y >> TempVerts[i].Point.z;
			TempVerts[i].Point.RotateZ(aRotate.z / 180 * M_PI);
			TempVerts[i].Point.RotateX(aRotate.x / 180 * M_PI);
			TempVerts[i].Point.RotateY(aRotate.y / 180 * M_PI);
			TempVerts[i].Point += pOrigin;
			tmp->pCenter += TempVerts[i].Point;
			i++;
			parser->getTokens();
			*parser >> token;
		} while (token.compare("endline") != 0);
		nv = i;
		//   tmp->Init(nv);
		tmp->Points = new vector3[nv];
		tmp->iNumPts = nv;
		tmp->pCenter /= (nv>0 ? nv : 1);
		for (int i = 0; i<nv; i++)
			tmp->Points[i] = TempVerts[i].Point;
		break;
	}
	return tmp;
}


TSubRect* TGround::FastGetSubRect(int iCol, int iRow)
{
	int br, bc, sr, sc;
	br = iRow / iNumSubRects;
	bc = iCol / iNumSubRects;
	sr = iRow - br*iNumSubRects;
	sc = iCol - bc*iNumSubRects;
	if ((br<0) || (bc<0) || (br >= iNumRects) || (bc >= iNumRects)) return NULL;
	return (Rects[br][bc].FastGetRect(sc, sr));
}

TSubRect* TGround::GetSubRect(int iCol, int iRow)
{//znalezienie małego kwadratu mapy
	int br, bc, sr, sc;
	br = iRow / iNumSubRects; //współrzędne kwadratu kilometrowego
	bc = iCol / iNumSubRects;
	sr = iRow - br*iNumSubRects; //współrzędne wzglęne małego kwadratu
	sc = iCol - bc*iNumSubRects;
	if ((br<0) || (bc<0) || (br >= iNumRects) || (bc >= iNumRects))
		return NULL; //jeśli poza mapą
	return (Rects[br][bc].SafeGetRect(sc, sr)); //pobranie małego kwadratu
}

TEvent* TGround::FindEvent(const std::string &asEventName)
{
	return (TEvent*)sTracks->Find(0, asEventName.c_str()); //wyszukiwanie w drzewie
	/* //powolna wyszukiwarka
	for (TEvent *Current=RootEvent;Current;Current=Current->Next2)
	{
	if (Current->asName==asEventName)
	return Current;
	}
	return NULL;
	*/
}

void TGround::FirstInit()
{//ustalanie zależności na scenerii przed wczytaniem pojazdów
	if (bInitDone) return; //Ra: żeby nie robiło się dwa razy
	bInitDone = true;
	WriteLog("InitNormals");
	int i, j;
	for (i = 0; i<TP_LAST; ++i)
	{
		for (TGroundNode *Current = nRootOfType[i]; Current; Current = Current->Next)
		{
			Current->InitNormals();
			if (Current->iType != TP_DYNAMIC)
			{//pojazdów w ogóle nie dotyczy dodawanie do mapy
				//-if (i == TP_EVLAUNCH ? Current->EvLaunch->IsGlobal() : false)
				//-	srGlobal.NodeAdd(Current); //dodanie do globalnego obiektu
				//-else if (i == TP_TERRAIN)
				//-{//specjalne przetwarzanie terenu wczytanego z pliku E3D
				//-	std::string xxxzzz; //nazwa kwadratu
				//-	TGroundRect *gr;
				//-	for (j = 1; j<Current->iCount; ++j)
				//-	{//od 1 do końca są zestawy trójkątów
				//-		xxxzzz = Current->nNode[j].smTerrain->pName; //pobranie nazwy
				//-		gr = GetRect(1000 * (xxxzzz.SubString(1, 3).ToIntDef(0) - 500), 1000 * (xxxzzz.SubString(4, 3).ToIntDef(0) - 500));
				//-		if (Global::bUseVBO)
				//-			gr->nTerrain = Current->nNode + j; //zapamiętanie
				//-		else
				//-			gr->RaNodeAdd(&Current->nNode[j]);
				//-	}
				//-}
				//    else if ((Current->iType!=GL_TRIANGLES)&&(Current->iType!=GL_TRIANGLE_STRIP)?true //~czy trójkąt?
				//-else 
				if ((Current->iType != GL_TRIANGLES) ? true //~czy trójkąt?
					: (Current->iFlags & 0x20) ? true //~czy teksturę ma nieprzezroczystą?
					: (Current->fSquareMinRadius != 0.0) ? true //~czy widoczny z bliska?
					: (Current->fSquareRadius <= 90000.0)) //~czy widoczny z daleka?
					GetSubRect(Current->pCenter.x, Current->pCenter.z)->NodeAdd(Current);
				else //dodajemy do kwadratu kilometrowego
					GetRect(Current->pCenter.x, Current->pCenter.z)->NodeAdd(Current);
			}
			//if (Current->iType!=TP_DYNAMIC)
			// GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
		}
	}
	for (i = 0; i<iNumRects; ++i)
		for (j = 0; j<iNumRects; ++j)
			Rects[i][j].Optimize(); //optymalizacja obiektów w sektorach
	WriteLog("InitNormals OK");
	WriteLog("InitTracks");
	InitTracks(); //łączenie odcinków ze sobą i przyklejanie eventów
	WriteLog("InitTracks OK");
//-	WriteLog("InitTraction");
//-	InitTraction(); //łączenie drutów ze sobą
//-	WriteLog("InitTraction OK");
//-	WriteLog("InitEvents");
//-	InitEvents();
//-	WriteLog("InitEvents OK");
//-	WriteLog("InitLaunchers");
//-	InitLaunchers();
//-	WriteLog("InitLaunchers OK");
//-	WriteLog("InitGlobalTime");
	//ABu 160205: juz nie TODO :)
//-	GlobalTime = new TMTableTime(hh, mm, srh, srm, ssh, ssm); //McZapkie-300302: inicjacja czasu rozkladowego - TODO: czytac z trasy!
//-	WriteLog("InitGlobalTime OK");
	//jeszcze ustawienie pogody, gdyby nie było w scenerii wpisów
	glClearColor(Global::AtmoColor[0], Global::AtmoColor[1], Global::AtmoColor[2], 0.0);                  // Background Color
	if (Global::fFogEnd>0)
	{
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogfv(GL_FOG_COLOR, Global::FogColor); // set fog color
		glFogf(GL_FOG_START, Global::fFogStart); // fog start depth
		glFogf(GL_FOG_END, Global::fFogEnd); // fog end depth
		glEnable(GL_FOG);
	}
	else
		glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, Global::lightPos);        //daylight position
	glLightfv(GL_LIGHT0, GL_AMBIENT, Global::ambientDayLight);  //kolor wszechobceny
	glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight);  //kolor padający
	glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight); //kolor odbity
	//musi być tutaj, bo wcześniej nie mieliśmy wartości światła
	if (Global::fMoveLight >= 0.0) //albo tak, albo niech ustala minimum ciemności w nocy
	{
		Global::fLuminance = //obliczenie luminacji "światła w ciemności"
			+0.150*Global::ambientDayLight[0]  //R
			+ 0.295*Global::ambientDayLight[1]  //G
			+ 0.055*Global::ambientDayLight[2]; //B
		if (Global::fLuminance>0.1) //jeśli miało by być za jasno
			for (int i = 0; i<3; i++)
				Global::ambientDayLight[i] *= 0.1 / Global::fLuminance; //ograniczenie jasności w nocy
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Global::ambientDayLight);
	}
	else if (Global::bDoubleAmbient) //Ra: wcześniej było ambient dawane na obydwa światła
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Global::ambientDayLight);
	glEnable(GL_LIGHTING);
	WriteLog("FirstInit is done");
};


// ********************************************************************************************************************
// TGround::Init
// ********************************************************************************************************************
bool TGround::Init(std::string asFile, HDC hDC)
{//główne wczytywanie scenerii
	//--if (asFile.LowerCase().SubString(1, 7) == "scenery")
	//--	asFile.Delete(1, 8); //Ra: usunięcie niepotrzebnych znaków - zgodność wstecz z 2003

	WriteLogSS("Loading scenery from " + asFile, "");
	Global::pGround = this;
  //pTrain=NULL;
	pOrigin = aRotate = vector3(0, 0, 0); //zerowanie przesunięcia i obrotu
	std::string str = "";
	std::string subpath = Global::asCurrentSceneryPath.c_str(); //   "scenery/";
	cParser parser(asFile.c_str(), cParser::buffer_FILE, subpath, false); // false = Global::bLoadTraction
	std::string token;

	const int OriginStackMaxDepth = 100; //rozmiar stosu dla zagnieżdżenia origin
	int OriginStackTop = 0;
	vector3 OriginStack[OriginStackMaxDepth]; //stos zagnieżdżenia origin

	double tf;
	int ParamCount, ParamPos;

	//ABu: Jezeli nie ma definicji w scenerii to ustawiane ponizsze wartosci:
	hh = 10;  //godzina startu
	mm = 30;  //minuty startu
	srh = 6;  //godzina wschodu slonca
	srm = 0;  //minuty wschodu slonca
	ssh = 20; //godzina zachodu slonca
	ssm = 0;  //minuty zachodu slonca
	TGroundNode *LastNode = NULL; //do użycia w trainset
	iNumNodes = 0;
	token = "";
	parser.getTokens();
	parser >> token;
	int refresh = 0;

	while (token != "") //(!Parser->EndOfFile)
	{
		if (refresh == 50)
		{//SwapBuffers(hDC); //Ra: bez ogranicznika za bardzo spowalnia :( a u niektórych miga
			refresh = 0;
		}
		else ++refresh;
		str = token.c_str();
		if (str == "node")
		{
			LastNode = AddGroundNode(&parser); //rozpoznanie węzła
			if (LastNode)
			{//jeżeli przetworzony poprawnie
				if (LastNode->iType == GL_TRIANGLES)
				{
					if (!LastNode->Vertices)
						SafeDelete(LastNode); //usuwamy nieprzezroczyste trójkąty terenu
				}
				else if (Global::bLoadTraction ? false : LastNode->iType == TP_TRACTION)
					SafeDelete(LastNode); //usuwamy druty, jeśli wyłączone
				if (LastNode) //dopiero na koniec dopisujemy do tablic
					if (LastNode->iType != TP_DYNAMIC)
					{//jeśli nie jest pojazdem
						LastNode->Next = nRootOfType[LastNode->iType]; //ostatni dodany dołączamy na końcu nowego
						nRootOfType[LastNode->iType] = LastNode; //ustawienie nowego na początku listy
						iNumNodes++;
					}
					else
					{//jeśli jest pojazdem
//-						//if (!bInitDone) FirstInit(); //jeśli nie było w scenerii
//-						if (LastNode->DynamicObject->Mechanik) //ale może być pasażer
//-							if (LastNode->DynamicObject->Mechanik->Primary()) //jeśli jest głównym (pasażer nie jest)
//-								TrainSetDriver = LastNode; //pojazd, któremu zostanie wysłany rozkład
//-						LastNode->Next = nRootDynamic;
//-						nRootDynamic = LastNode; //dopisanie z przodu do listy
//-						//if (bTrainSet && (LastNode?(LastNode->iType==TP_DYNAMIC):false))
//-						if (TrainSetNode) //jeżeli istnieje wcześniejszy TP_DYNAMIC
//-							TrainSetNode->DynamicObject->AttachPrev(LastNode->DynamicObject, TempConnectionType[iTrainSetWehicleNumber - 2]);
//-						TrainSetNode = LastNode; //ostatnio wczytany
					}
			}
			else
			{
				WriteLogSS("Scene parse error near " + token, "ERROR");
				//break;
			}
		}
		else
		if (str == "trainset")
			{
				iTrainSetWehicleNumber = 0;
				TrainSetNode = NULL;
				TrainSetDriver = NULL; //pojazd, któremu zostanie wysłany rozkład
				bTrainSet = true;
				parser.getTokens();
				parser >> token;
				asTrainName = token.c_str();  //McZapkie: rodzaj+nazwa pociagu w SRJP
				parser.getTokens();
				parser >> token;
				asTrainSetTrack = token.c_str(); //ścieżka startowa
				parser.getTokens(2);
				parser >> fTrainSetDist >> fTrainSetVel; //przesunięcie i prędkość
			}
		else
		if (str == "endtrainset")
				{//McZapkie-110103: sygnaly konca pociagu ale tylko dla pociagow rozkladowych
					if (TrainSetNode) //trainset bez dynamic się sypał
					{//powinien też tu wchodzić, gdy pojazd bez trainset
						if (TrainSetDriver) //pojazd, któremu zostanie wysłany rozkład
						{
//-							TrainSetDriver->DynamicObject->Mechanik->DirectionInitial();
//-							TrainSetDriver->DynamicObject->Mechanik->PutCommand("Timetable:" + asTrainName, fTrainSetVel, 0, NULL);
						}
						if (asTrainName != "none")
						{//gdy podana nazwa, włączenie jazdy pociągowej
							/*
							if((TrainSetNode->DynamicObject->EndSignalsLight1Active())
							||(TrainSetNode->DynamicObject->EndSignalsLight1oldActive()))
							TrainSetNode->DynamicObject->MoverParameters->HeadSignal=2+32;
							else
							TrainSetNode->DynamicObject->MoverParameters->EndSignalsFlag=64;
							*/
						}
					}
	//-				if (LastNode) //ostatni wczytany obiekt
	//-					if (LastNode->iType == TP_DYNAMIC) //o ile jest pojazdem (na ogół jest, ale kto wie...)
	//-						if (!TempConnectionType[iTrainSetWehicleNumber - 1]) //jeśli ostatni pojazd ma sprzęg 0
	//-							LastNode->DynamicObject->RaLightsSet(-1, 2 + 32 + 64); //to założymy mu końcówki blaszane (jak AI się odpali, to sobie poprawi)
				
					bTrainSet = false;
					fTrainSetVel = 0;
					//iTrainSetConnection=0;
					TrainSetNode = NULL;
					iTrainSetWehicleNumber = 0;
			}
		/*
		else if (str == "event")
			{
					TEvent *tmp = new TEvent();
					tmp->Load(&parser, &pOrigin);
					if (tmp->Type == tp_Unknown)
						delete tmp;
					else
					{//najpierw sprawdzamy, czy nie ma, a potem dopisujemy
						TEvent *found = FindEvent(tmp->asName);
						if (found)
						{//jeśli znaleziony duplikat
							int i = tmp->asName.Length();
							if (tmp->asName[1] == '#') //zawsze jeden znak co najmniej jest
							{
								delete tmp; tmp = NULL;
							} //utylizacja duplikatu z krzyżykiem
							else if (i>8 ? tmp->asName.SubString(1, 9) == "lineinfo:" : false) //tymczasowo wyjątki
							{
								delete tmp; tmp = NULL;
							} //tymczasowa utylizacja duplikatów W5
							else if (i>8 ? tmp->asName.SubString(i - 7, 8) == "_warning" : false) //tymczasowo wyjątki
							{
								delete tmp; tmp = NULL;
							} //tymczasowa utylizacja duplikatu z trąbieniem
							else if (i>4 ? tmp->asName.SubString(i - 3, 4) == "_shp" : false) //nie podlegają logowaniu
							{
								delete tmp; tmp = NULL;
							} //tymczasowa utylizacja duplikatu SHP
							if (tmp) //jeśli nie został zutylizowany
								if (Global::bJoinEvents)
									found->Append(tmp); //doczepka (taki wirtualny multiple bez warunków)
								else
								{
									ErrorLog("Duplicated event: " + tmp->asName);
									found->Append(tmp); //doczepka (taki wirtualny multiple bez warunków)
									found->Type = tp_Ignored; //dezaktywacja pierwotnego - taka proteza na wsteczną zgodność
									//SafeDelete(tmp); //bezlitośnie usuwamy wszelkie duplikaty, żeby nie zaśmiecać drzewka
								}
						}
						if (tmp)
						{//jeśli nie duplikat
							tmp->Next2 = RootEvent; //lista wszystkich eventów (m.in. do InitEvents)
							RootEvent = tmp;
							if (!found)
							{//jeśli nazwa wystąpiła, to do kolejki i wyszukiwarki dodawany jest tylko pierwszy
								if (RootEvent->Type != tp_Ignored)
									if (RootEvent->asName.Pos("onstart")) //event uruchamiany automatycznie po starcie
										AddToQuery(RootEvent, NULL); //dodanie do kolejki
								sTracks->Add(0, tmp->asName.c_str(), tmp); //dodanie do wyszukiwarki
							}
						}
					}
			}
		//     else
		//     if (str==AnsiString("include"))  //Tolaris to zrobil wewnatrz parsera
		//     {
		//         Include(Parser);
		//     }
		*/
		else if (str == "rotate")
			{
					//parser.getTokens(3);
					//parser >> aRotate.x >> aRotate.y >> aRotate.z; //Ra: to potrafi dawać błędne rezultaty
					parser.getTokens(); parser >> aRotate.x;
					parser.getTokens(); parser >> aRotate.y;
					parser.getTokens(); parser >> aRotate.z;
					//WriteLog("*** rotate "+AnsiString(aRotate.x)+" "+AnsiString(aRotate.y)+" "+AnsiString(aRotate.z));
			}
		else if (str == "origin")
			{

					{
						if (OriginStackTop >= OriginStackMaxDepth - 1)
						{
							MessageBox(0, "Origin stack overflow ", "Error", MB_OK);
							break;
						}
						parser.getTokens(3);
						parser >> OriginStack[OriginStackTop].x >> OriginStack[OriginStackTop].y >> OriginStack[OriginStackTop].z;
						pOrigin += OriginStack[OriginStackTop]; //sumowanie całkowitego przesunięcia
						OriginStackTop++; //zwiększenie wskaźnika stosu
					}
			}
		else if (str == "endorigin")
				{
					//      else
					//    if (str=="end")
					{
						if (OriginStackTop <= 0)
						{
							MessageBox(0, "Origin stack underflow ", "Error", MB_OK);
							break;
						}

						OriginStackTop--; //zmniejszenie wskaźnika stosu
						pOrigin -= OriginStack[OriginStackTop];
					}
				}
		else if (str == "atmo")   //TODO: uporzadkowac gdzie maja byc parametry mgly!
				{//Ra: ustawienie parametrów OpenGL przeniesione do FirstInit
					WriteLog("Scenery atmo definition");
					parser.getTokens(3);
					parser >> Global::AtmoColor[0] >> Global::AtmoColor[1] >> Global::AtmoColor[2];
					parser.getTokens(2);
					parser >> Global::fFogStart >> Global::fFogEnd;
					if (Global::fFogEnd>0.0)
					{//ostatnie 3 parametry są opcjonalne
						parser.getTokens(3);
						parser >> Global::FogColor[0] >> Global::FogColor[1] >> Global::FogColor[2];
					}
					parser.getTokens();
					parser >> token;
					while (token.compare("endatmo") != 0)
					{//a kolejne parametry są pomijane
						parser.getTokens();
						parser >> token;
					}
				}
		else if (str == "time")
				{
					WriteLog("Scenery time definition");
					char temp_in[9];
					char temp_out[9];
					int i, j;
					parser.getTokens();
					parser >> temp_in;
					for (j = 0; j <= 8; j++) temp_out[j] = ' ';
					for (i = 0; temp_in[i] != ':'; i++)
						temp_out[i] = temp_in[i];
					hh = atoi(temp_out);
					for (j = 0; j <= 8; j++) temp_out[j] = ' ';
					for (j = i + 1; j <= 8; j++)
						temp_out[j - (i + 1)] = temp_in[j];
					mm = atoi(temp_out);


					parser.getTokens();
					parser >> temp_in;
					for (j = 0; j <= 8; j++) temp_out[j] = ' ';
					for (i = 0; temp_in[i] != ':'; i++)
						temp_out[i] = temp_in[i];
					srh = atoi(temp_out);
					for (j = 0; j <= 8; j++) temp_out[j] = ' ';
					for (j = i + 1; j <= 8; j++)
						temp_out[j - (i + 1)] = temp_in[j];
					srm = atoi(temp_out);

					parser.getTokens();
					parser >> temp_in;
					for (j = 0; j <= 8; j++) temp_out[j] = ' ';
					for (i = 0; temp_in[i] != ':'; i++)
						temp_out[i] = temp_in[i];
					ssh = atoi(temp_out);
					for (j = 0; j <= 8; j++) temp_out[j] = ' ';
					for (j = i + 1; j <= 8; j++)
						temp_out[j - (i + 1)] = temp_in[j];
					ssm = atoi(temp_out);
					while (token.compare("endtime") != 0)
					{
						parser.getTokens();
						parser >> token;
					}
				}
		else if (str == "light")
				{//Ra: ustawianie światła przeniesione do FirstInit
					WriteLog("Scenery light definition");
					vector3 lp;
					parser.getTokens(); parser >> lp.x;
					parser.getTokens(); parser >> lp.y;
					parser.getTokens(); parser >> lp.z;
					lp = Normalize(lp); //kierunek padania
					Global::lightPos[0] = lp.x; //daylight position
					Global::lightPos[1] = lp.y;
					Global::lightPos[2] = lp.z;
					parser.getTokens(); parser >> Global::ambientDayLight[0]; //kolor wszechobceny
					parser.getTokens(); parser >> Global::ambientDayLight[1];
					parser.getTokens(); parser >> Global::ambientDayLight[2];

					parser.getTokens(); parser >> Global::diffuseDayLight[0]; //kolor padający
					parser.getTokens(); parser >> Global::diffuseDayLight[1];
					parser.getTokens(); parser >> Global::diffuseDayLight[2];

					parser.getTokens(); parser >> Global::specularDayLight[0]; //kolor odbity
					parser.getTokens(); parser >> Global::specularDayLight[1];
					parser.getTokens(); parser >> Global::specularDayLight[2];

					do
					{
						parser.getTokens(); parser >> token;
					} while (token.compare("endlight") != 0);

				}
		else if (str == "camera")
				{
					vector3 xyz, abc;
					xyz = abc = vector3(0, 0, 0); //wartości domyślne, bo nie wszystie muszą być
					int i = -1, into = -1; //do której definicji kamery wstawić
					WriteLog("Scenery camera definition");
					do
					{//opcjonalna siódma liczba określa numer kamery, a kiedyś były tylko 3
						parser.getTokens(); parser >> token;
						switch (++i)
						{//kiedyś camera miało tylko 3 współrzędne
						case 0: xyz.x = atof(token.c_str()); break;
						case 1: xyz.y = atof(token.c_str()); break;
						case 2: xyz.z = atof(token.c_str()); break;
						case 3: abc.x = atof(token.c_str()); break;
						case 4: abc.y = atof(token.c_str()); break;
						case 5: abc.z = atof(token.c_str()); break;
						case 6: into = atoi(token.c_str()); //takie sobie, bo można wpisać -1
						}
					} while (token.compare("endcamera") != 0);
					if (into<0) into = ++Global::iCameraLast;
					if ((into >= 0) && (into<10))
					{//przepisanie do odpowiedniego miejsca w tabelce
						Global::pFreeCameraInit[into] = xyz;
						abc.x = DegToRad(abc.x);
						abc.y = DegToRad(abc.y);
						abc.z = DegToRad(abc.z);
						Global::pFreeCameraInitAngle[into] = abc;
						Global::iCameraLast = into; //numer ostatniej
					}
				}
		else if (str == "sky")
				{//youBy - niebo z pliku
					WriteLog("Scenery sky definition");
					parser.getTokens();
					parser >> token;
					std::string SkyTemp;
					SkyTemp = token;
					if (Global::asSky == "1") Global::asSky = SkyTemp;
					do
					{//pożarcie dodatkowych parametrów
						parser.getTokens(); parser >> token;
					} while (token.compare("endsky") != 0);
					WriteLog(Global::asSky.c_str());
				}
		else if (str == "firstinit")
					FirstInit();
		else if (str == "description")
				{
					do
					{
						parser.getTokens();
						parser >> token;
					} while (token.compare("enddescription") != 0);
				}
		else if (str == "test")
				{//wypisywanie treści po przetworzeniu
					WriteLog("---> Parser test:");
					do
					{
						parser.getTokens();
						parser >> token;
						WriteLog(token.c_str());
					} while (token.compare("endtest") != 0);
					WriteLog("---> End of parser test.");
				}
		else if (str == "config")
				{//możliwość przedefiniowania parametrów w scenerii
			 	 //-	Global::ConfigParse(NULL, &parser); //parsowanie dodatkowych ustawień
				}
		else if (str != "")
				{//pomijanie od nierozpoznanej komendy do jej zakończenia
					if ((token.length()>2) && (atof(token.c_str()) == 0.0))
					{//jeśli nie liczba, to spróbować pominąć komendę
						WriteLogSS("Unrecognized command: " + str, "ERROR");
						str = "end" + str;
						do
						{
							parser.getTokens();
							token = "";
							parser >> token;
						} while ((token != "") && (token.compare(str.c_str()) != 0));
					}
					else //jak liczba to na pewno błąd
						WriteLogSS("Unrecognized command: " + str, "ERROR");
				}
				else
					if (str == "")
						break;

				//LastNode=NULL;

				token = "";
				parser.getTokens();
				parser >> token;
	}

//	delete parser;

	sTracks->Sort(TP_TRACK); //finalne sortowanie drzewa torów
	sTracks->Sort(TP_MEMCELL); //finalne sortowanie drzewa komórek pamięci
	sTracks->Sort(TP_MODEL); //finalne sortowanie drzewa modeli
	sTracks->Sort(0); //finalne sortowanie drzewa eventów
	if (!bInitDone) FirstInit(); //jeśli nie było w scenerii

	//-if (Global::pTerrainCompact)
	//-	TerrainWrite(); //Ra: teraz można zapisać teren w jednym pliku
	return true;
}


bool TGround::InitEvents()
{//łączenie eventów z pozostałymi obiektami
/*
	TGroundNode* tmp;
	char buff[255];
	int i;
	for (TEvent *Current = RootEvent; Current; Current = Current->Next2)
	{
		switch (Current->Type)
		{
		case tp_AddValues: //sumowanie wartości
		case tp_UpdateValues: //zmiana wartości
			tmp = FindGroundNode(Current->asNodeName, TP_MEMCELL); //nazwa komórki powiązanej z eventem
			if (tmp)
			{//McZapkie-100302
				if (Current->iFlags&(conditional_trackoccupied | conditional_trackfree))
				{//jeśli chodzi o zajetosc toru (tor może być inny, niż wpisany w komórce)
					tmp = FindGroundNode(Current->asNodeName, TP_TRACK); //nazwa toru ta sama, co nazwa komórki
					if (tmp) Current->Params[9].asTrack = tmp->pTrack;
					if (!Current->Params[9].asTrack)
						ErrorLog("Bad event: track \"" + AnsiString(Current->asNodeName) + "\" does not exists in \"" + Current->asName + "\"");
				}
				Current->Params[4].asGroundNode = tmp;
				Current->Params[5].asMemCell = tmp->MemCell; //komórka do aktualizacji
				if (Current->iFlags&(conditional_memcompare))
					Current->Params[9].asMemCell = tmp->MemCell; //komórka do badania warunku
				if (tmp->MemCell->asTrackName != "none") //tor powiązany z komórką powiązaną z eventem
				{//tu potrzebujemy wskaźnik do komórki w (tmp)
					tmp = FindGroundNode(tmp->MemCell->asTrackName, TP_TRACK);
					if (tmp)
						Current->Params[6].asTrack = tmp->pTrack;
					else
						ErrorLog("Bad memcell: track \"" + tmp->MemCell->asTrackName + "\" not exists in \"" + tmp->MemCell->asTrackName + "\"");
				}
				else
					Current->Params[6].asTrack = NULL;
			}
			else
				ErrorLog("Bad event: \"" + Current->asName + "\" cannot find memcell \"" + Current->asNodeName + "\"");
			break;
		case tp_LogValues: //skojarzenie z memcell
			if (Current->asNodeName.IsEmpty())
			{//brak skojarzenia daje logowanie wszystkich
				Current->Params[9].asMemCell = NULL;
				break;
			}
		case tp_GetValues:
		case tp_WhoIs:
			tmp = FindGroundNode(Current->asNodeName, TP_MEMCELL);
			if (tmp)
			{
				Current->Params[8].asGroundNode = tmp;
				Current->Params[9].asMemCell = tmp->MemCell;
				if (Current->Type == tp_GetValues) //jeśli odczyt komórki
					if (tmp->MemCell->IsVelocity()) //a komórka zawiera komendę SetVelocity albo ShuntVelocity
						Current->bEnabled = false; //to event nie będzie dodawany do kolejki
			}
			else
				ErrorLog("Bad event: \"" + Current->asName + "\" cannot find memcell \"" + Current->asNodeName + "\"");
			break;
		case tp_CopyValues: //skopiowanie komórki do innej
			tmp = FindGroundNode(Current->asNodeName, TP_MEMCELL); //komórka docelowa
			if (tmp)
			{
				Current->Params[4].asGroundNode = tmp;
				Current->Params[5].asMemCell = tmp->MemCell; //komórka docelowa
			}
			else
				ErrorLog("Bad copyvalues: event \"" + Current->asName + "\" cannot find memcell \"" + Current->asNodeName + "\"");
			strcpy(buff, Current->Params[9].asText); //skopiowanie nazwy drugiej komórki do bufora roboczego
			SafeDeleteArray(Current->Params[9].asText); //usunięcie nazwy komórki
			tmp = FindGroundNode(buff, TP_MEMCELL); //komórka źódłowa
			if (tmp)
			{
				Current->Params[8].asGroundNode = tmp;
				Current->Params[9].asMemCell = tmp->MemCell; //komórka źródłowa
			}
			else
				ErrorLog("Bad copyvalues: event \"" + Current->asName + "\" cannot find memcell \"" + AnsiString(buff) + "\"");
			break;
		case tp_Animation: //animacja modelu
			tmp = FindGroundNode(Current->asNodeName, TP_MODEL); //egzemplarza modelu do animowania
			if (tmp)
			{
				strcpy(buff, Current->Params[9].asText); //skopiowanie nazwy submodelu do bufora roboczego
				SafeDeleteArray(Current->Params[9].asText); //usunięcie nazwy submodelu
				if (Current->Params[0].asInt == 4)
					Current->Params[9].asModel = tmp->Model; //model dla całomodelowych animacji
				else
				{//standardowo przypisanie submodelu
					Current->Params[9].asAnimContainer = tmp->Model->GetContainer(buff); //submodel
					if (Current->Params[9].asAnimContainer)
						Current->Params[9].asAnimContainer->WillBeAnimated(); //oflagowanie animacji
				}
			}
			else
				ErrorLog("Bad animation: event \"" + Current->asName + "\" cannot find model \"" + Current->asNodeName + "\"");
			Current->asNodeName = "";
			break;
		case tp_Lights: //zmiana świeteł modelu
			tmp = FindGroundNode(Current->asNodeName, TP_MODEL);
			if (tmp)
				Current->Params[9].asModel = tmp->Model;
			else
				ErrorLog("Bad lights: event \"" + Current->asName + "\" cannot find model \"" + Current->asNodeName + "\"");
			Current->asNodeName = "";
			break;
		case tp_Visible: //ukrycie albo przywrócenie obiektu
			tmp = FindGroundNode(Current->asNodeName, TP_MODEL); //najpierw model
			if (!tmp) tmp = FindGroundNode(Current->asNodeName, TP_TRACTION); //może druty?
			if (!tmp) tmp = FindGroundNode(Current->asNodeName, TP_TRACK); //albo tory?
			if (tmp)
				Current->Params[9].asGroundNode = tmp;
			else
				ErrorLog("Bad visibility: event \"" + Current->asName + "\" cannot find model \"" + Current->asNodeName + "\"");
			Current->asNodeName = "";
			break;
		case tp_Switch: //pezełożenie zwrotnicy albo zmiana stanu obrotnicy
			tmp = FindGroundNode(Current->asNodeName, TP_TRACK);
			if (tmp)
				Current->Params[9].asTrack = tmp->pTrack;
			else
				ErrorLog("Bad switch: event \"" + Current->asName + "\" cannot find track \"" + Current->asNodeName + "\"");
			Current->asNodeName = "";
			break;
		case tp_Sound: //odtworzenie dźwięku
			tmp = FindGroundNode(Current->asNodeName, TP_SOUND);
			if (tmp)
				Current->Params[9].asRealSound = tmp->pStaticSound;
			else
				ErrorLog("Bad sound: event \"" + Current->asName + "\" cannot find static sound \"" + Current->asNodeName + "\"");
			Current->asNodeName = "";
			break;
		case tp_TrackVel: //ustawienie prędkości na torze
			if (!Current->asNodeName.IsEmpty())
			{
				tmp = FindGroundNode(Current->asNodeName, TP_TRACK);
				if (tmp)
					Current->Params[9].asTrack = tmp->pTrack;
				else
					ErrorLog("Bad velocity: event \"" + Current->asName + "\" cannot find track \"" + Current->asNodeName + "\"");
			}
			Current->asNodeName = "";
			break;
		case tp_DynVel: //komunikacja z pojazdem o konkretnej nazwie
			if (Current->asNodeName == "activator")
				Current->Params[9].asDynamic = NULL;
			else
			{
				tmp = FindGroundNode(Current->asNodeName, TP_DYNAMIC);
				if (tmp)
					Current->Params[9].asDynamic = tmp->DynamicObject;
				else
					Error("Event \"" + Current->asName + "\" cannot find dynamic \"" + Current->asNodeName + "\"");
			}
			Current->asNodeName = "";
			break;
		case tp_Multiple:
			if (Current->Params[9].asText != NULL)
			{//przepisanie nazwy do bufora
				strcpy(buff, Current->Params[9].asText);
				SafeDeleteArray(Current->Params[9].asText);
			}
			else buff[0] = '\0';
			if (Current->iFlags&(conditional_trackoccupied | conditional_trackfree))
			{//jeśli chodzi o zajetosc toru
				tmp = FindGroundNode(buff, TP_TRACK);
				if (tmp) Current->Params[9].asTrack = tmp->pTrack;
				if (!Current->Params[9].asTrack)
				{
					ErrorLog(AnsiString("Bad event: Track \"") + AnsiString(buff) + "\" does not exist in \"" + Current->asName + "\"");
					Current->iFlags &= ~(conditional_trackoccupied | conditional_trackfree); //zerowanie flag
				}
			}
			else if (Current->iFlags&(conditional_memstring | conditional_memval1 | conditional_memval2))
			{//jeśli chodzi o komorke pamieciową
				tmp = FindGroundNode(buff, TP_MEMCELL);
				if (tmp) Current->Params[9].asMemCell = tmp->MemCell;
				if (!Current->Params[9].asMemCell)
				{
					ErrorLog(AnsiString("Bad event: MemCell \"") + AnsiString(buff) + AnsiString("\" does not exist in \"" + Current->asName + "\""));
					Current->iFlags &= ~(conditional_memstring | conditional_memval1 | conditional_memval2);
				}
			}
			for (i = 0; i<8; i++)
			{
				if (Current->Params[i].asText != NULL)
				{
					strcpy(buff, Current->Params[i].asText);
					SafeDeleteArray(Current->Params[i].asText);
					Current->Params[i].asEvent = FindEvent(buff);
					if (!Current->Params[i].asEvent) //Ra: tylko w logu informacja o braku
						if (AnsiString(Current->Params[i].asText).SubString(1, 5) != "none_")
						{
							WriteLog(AnsiString("Event \"") + AnsiString(buff) + AnsiString("\" does not exist"));
							ErrorLog("Missed event: " + AnsiString(buff) + " in multiple " + Current->asName);
						}
				}
			}
			break;
		}
		if (Current->fDelay<0)
			AddToQuery(Current, NULL);
	}
	for (TGroundNode *Current = nRootOfType[TP_MEMCELL]; Current; Current = Current->Next)
	{//Ra: eventy komórek pamięci, wykonywane po wysłaniu komendy do zatrzymanego pojazdu
		Current->MemCell->AssignEvents(FindEvent(Current->asName + ":sent"));
	}
	*/
	return true;
}

void TGround::InitTracks()
{//łączenie torów ze sobą i z eventami
	TGroundNode *Current, *Model;
	TTrack *tmp; //znaleziony tor
	TTrack *Track;
	int iConnection, state;
	std::string name;
	//tracks=tracksfar=0;
	for (Current = nRootOfType[TP_TRACK]; Current; Current = Current->Next)
	{
		Track = Current->pTrack;
		Track->AssignEvents(
			Track->asEvent0Name.empty() ? NULL : FindEvent(Track->asEvent0Name),
			Track->asEvent1Name.empty() ? NULL : FindEvent(Track->asEvent1Name),
			Track->asEvent2Name.empty() ? NULL : FindEvent(Track->asEvent2Name));
		Track->AssignallEvents(
			Track->asEventall0Name.empty() ? NULL : FindEvent(Track->asEventall0Name),
			Track->asEventall1Name.empty() ? NULL : FindEvent(Track->asEventall1Name),
			Track->asEventall2Name.empty() ? NULL : FindEvent(Track->asEventall2Name)); //MC-280503
		switch (Track->eType)
		{
		case tt_Turn: //obrotnicę też łączymy na starcie z innymi torami
			Model = FindGroundNode(Current->asName, TP_MODEL); //szukamy modelu o tej samej nazwie
			if (tmp) //mamy model, trzeba zapamiętać wskaźnik do jego animacji
			{//jak coś pójdzie źle, to robimy z tego normalny tor
				//Track->ModelAssign(tmp->Model->GetContainer(NULL)); //wiązanie toru z modelem obrotnicy
				Track->RaAssign(Current, Model->Model); //wiązanie toru z modelem obrotnicy
				//break; //jednak połączę z sąsiednim, jak ma się wysypywać null track
			}
		case tt_Normal: //tylko proste są podłączane do rozjazdów, stąd dwa rozjazdy się nie połączą ze sobą
			if (Track->CurrentPrev() == NULL) //tylko jeśli jeszcze nie podłączony
			{
				tmp = FindTrack(Track->CurrentSegment()->FastGetPoint_0(), iConnection, Current);
				switch (iConnection)
				{
				case -1: //Ra: pierwsza koncepcja zawijania samochodów i statków
					//if ((Track->iCategoryFlag&1)==0) //jeśli nie jest torem szynowym
					// Track->ConnectPrevPrev(Track,0); //łączenie końca odcinka do samego siebie
					break;
				case 0:
					Track->ConnectPrevPrev(tmp, 0);
					break;
				case 1:
					Track->ConnectPrevNext(tmp, 1);
					break;
				case 2:
					Track->ConnectPrevPrev(tmp, 0); //do Point1 pierwszego
					tmp->SetConnections(0); //zapamiętanie ustawień w Segmencie
					break;
				case 3:
					Track->ConnectPrevNext(tmp, 1); //do Point2 pierwszego
					tmp->SetConnections(0); //zapamiętanie ustawień w Segmencie
					break;
				case 4:
					tmp->Switch(1);
					Track->ConnectPrevPrev(tmp, 0); //do Point1 drugiego
					tmp->SetConnections(1); //robi też Switch(0)
					tmp->Switch(0);
					break;
				case 5:
					tmp->Switch(1);
					Track->ConnectPrevNext(tmp, 3); //do Point2 drugiego
					tmp->SetConnections(1); //robi też Switch(0)
					tmp->Switch(0);
					break;
				}
			}
			if (Track->CurrentNext() == NULL) //tylko jeśli jeszcze nie podłączony
			{
				tmp = FindTrack(Track->CurrentSegment()->FastGetPoint_1(), iConnection, Current);
				switch (iConnection)
				{
				case -1: //Ra: pierwsza koncepcja zawijania samochodów i statków
					//if ((Track->iCategoryFlag&1)==0) //jeśli nie jest torem szynowym
					// Track->ConnectNextNext(Track,1); //łączenie końca odcinka do samego siebie
					break;
				case 0:
					Track->ConnectNextPrev(tmp, 0);
					break;
				case 1:
					Track->ConnectNextNext(tmp, 1);
					break;
				case 2:
					Track->ConnectNextPrev(tmp, 0);
					tmp->SetConnections(0); //zapamiętanie ustawień w Segmencie
					break;
				case 3:
					Track->ConnectNextNext(tmp, 1);
					tmp->SetConnections(0); //zapamiętanie ustawień w Segmencie
					break;
				case 4:
					tmp->Switch(1);
					Track->ConnectNextPrev(tmp, 0);
					tmp->SetConnections(1); //robi też Switch(0)
					//tmp->Switch(0);
					break;
				case 5:
					tmp->Switch(1);
					Track->ConnectNextNext(tmp, 3);
					tmp->SetConnections(1); //robi też Switch(0)
					//tmp->Switch(0);
					break;
				}
			}
			break;
		case tt_Switch: //dla rozjazdów szukamy eventów sygnalizacji rozprucia
			Track->AssignForcedEvents(
				FindEvent(Current->asName + ":forced+"),
				FindEvent(Current->asName + ":forced-"));
			break;
		}
		name = Track->IsolatedName(); //pobranie nazwy odcinka izolowanego
		if (!name.empty()) //jeśli została zwrócona nazwa
			Track->IsolatedEventsAssign(FindEvent(name + ":busy"), FindEvent(name + ":free"));
		if (Current->asName.substr(1, 1) == "*") //możliwy portal, jeśli nie podłączony od striny 1
			if (!Track->CurrentPrev() && Track->CurrentNext())
				Track->iCategoryFlag |= 0x100; //ustawienie flagi portalu
	}
	//WriteLog("Total "+AnsiString(tracks)+", far "+AnsiString(tracksfar));
}



void TGround::InitTraction()
{//łączenie drutów ze sobą oraz z torami i eventami
	/*
	TGroundNode *Current;
	TTraction *tmp; //znalezione przęsło
	TTraction *Traction;
	int iConnection, state;
	std::string name;
	for (Current = nRootOfType[TP_TRACTION]; Current; Current = Current->Next)
	{
		Traction = Current->Traction;
		if (!Traction->pPrev) //tylko jeśli jeszcze nie podłączony
		{
			tmp = FindTraction(&Traction->pPoint1, iConnection, Current);
			switch (iConnection)
			{
			case 0:
				Traction->Connect(0, tmp, 0);
				break;
			case 1:
				Traction->Connect(0, tmp, 1);
				break;
			}
		}
		if (!Traction->pNext) //tylko jeśli jeszcze nie podłączony
		{
			tmp = FindTraction(&Traction->pPoint2, iConnection, Current);
			switch (iConnection)
			{
			case 0:
				Traction->Connect(1, tmp, 0);
				break;
			case 1:
				Traction->Connect(1, tmp, 1);
				break;
			}
		}
	}
	for (Current = nRootOfType[TP_TRACTION]; Current; Current = Current->Next)
		Current->Traction->WhereIs(); //oznakowanie przedostatnich przęseł
		*/
};

void TGround::TrackJoin(TGroundNode *Current)
{//wyszukiwanie sąsiednich torów do podłączenia (wydzielone na użytek obrotnicy)
	TTrack *Track = Current->pTrack;
	TTrack *tmp;
	int iConnection;
	if (!Track->CurrentPrev())
	{
		tmp = FindTrack(Track->CurrentSegment()->FastGetPoint_0(), iConnection, Current); //Current do pominięcia
		switch (iConnection)
		{
		case 0: Track->ConnectPrevPrev(tmp, 0); break;
		case 1: Track->ConnectPrevNext(tmp, 1); break;
		}
	}
	if (!Track->CurrentNext())
	{
		tmp = FindTrack(Track->CurrentSegment()->FastGetPoint_1(), iConnection, Current);
		switch (iConnection)
		{
		case 0: Track->ConnectNextPrev(tmp, 0); break;
		case 1: Track->ConnectNextNext(tmp, 1); break;
		}
	}
}

//McZapkie-070602: wyzwalacze zdarzen
bool TGround::InitLaunchers()
{
	/*
	TGroundNode *Current, *tmp;
	TEventLauncher *EventLauncher;
	int i;
	for (Current = nRootOfType[TP_EVLAUNCH]; Current; Current = Current->Next)
	{
		EventLauncher = Current->EvLaunch;
		if (EventLauncher->iCheckMask != 0)
			if (EventLauncher->asMemCellName != AnsiString("none"))
			{//jeśli jest powiązana komórka pamięci
				tmp = FindGroundNode(EventLauncher->asMemCellName, TP_MEMCELL);
				if (tmp)
					EventLauncher->MemCell = tmp->MemCell; //jeśli znaleziona, dopisać
				else
					MessageBox(0, "Cannot find Memory Cell for Event Launcher", "Error", MB_OK);
			}
			else
				EventLauncher->MemCell = NULL;
		EventLauncher->Event1 = (EventLauncher->asEvent1Name != AnsiString("none")) ? FindEvent(EventLauncher->asEvent1Name) : NULL;
		EventLauncher->Event2 = (EventLauncher->asEvent2Name != AnsiString("none")) ? FindEvent(EventLauncher->asEvent2Name) : NULL;
	}
	*/
	return true;
}

TTrack* TGround::FindTrack(vector3 Point, int &iConnection, TGroundNode *Exclude)
{//wyszukiwanie innego toru kończącego się w (Point)
	TTrack *Track;
	TGroundNode *Current;
	TTrack *tmp;
	iConnection = -1;
	TSubRect *sr;
	//najpierw szukamy w okolicznych segmentach
	int c = GetColFromX(Point.x);
	int r = GetRowFromZ(Point.z);
	if ((sr = FastGetSubRect(c, r)) != NULL) //75% torów jest w tym samym sektorze
		if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
			return tmp;
	int i, x, y;
	for (i = 1; i<9; ++i) //sektory w kolejności odległości, 4 jest tu wystarczające, 9 na wszelki wypadek
	{//niemal wszystkie podłączone tory znajdują się w sąsiednich 8 sektorach
		x = SectorOrder[i].x;
		y = SectorOrder[i].y;
		if ((sr = FastGetSubRect(c + y, r + x)) != NULL)
			if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
				return tmp;
		if (x)
			if ((sr = FastGetSubRect(c + y, r - x)) != NULL)
				if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
					return tmp;
		if (y)
			if ((sr = FastGetSubRect(c - y, r + x)) != NULL)
				if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
					return tmp;
		if ((sr = FastGetSubRect(c - y, r - x)) != NULL)
			if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
				return tmp;
	}
#if 0
	//wyszukiwanie czołgowe (po wszystkich jak leci) - nie ma chyba sensu
	for (Current = nRootOfType[TP_TRACK]; Current; Current = Current->Next)
	{
		if ((Current->iType == TP_TRACK) && (Current != Exclude))
		{
			iConnection = Current->pTrack->TestPoint(&Point);
			if (iConnection >= 0) return Current->pTrack;
		}
	}
#endif
	return NULL;
}


TTraction* TGround::FindTraction(vector3 *Point, int &iConnection, TGroundNode *Exclude)
{//wyszukiwanie innego przęsła kończącego się w (Point)
	TTraction *Traction;
	TGroundNode *Current;
	TTraction *tmp;
	iConnection = -1;
	TSubRect *sr;
	//najpierw szukamy w okolicznych segmentach
	int c = GetColFromX(Point->x);
	int r = GetRowFromZ(Point->z);
	if ((sr = FastGetSubRect(c, r)) != NULL) //większość będzie w tym samym sektorze
		if ((tmp = sr->FindTraction(Point, iConnection, Exclude->Traction)) != NULL)
			return tmp;
	int i, x, y;
	for (i = 1; i<9; ++i) //sektory w kolejności odległości, 4 jest tu wystarczające, 9 na wszelki wypadek
	{//wszystkie przęsła powinny zostać znajdować się w sąsiednich 8 sektorach
		x = SectorOrder[i].x;
		y = SectorOrder[i].y;
		if ((sr = FastGetSubRect(c + y, r + x)) != NULL)
			if ((tmp = sr->FindTraction(Point, iConnection, Exclude->Traction)) != NULL)
				return tmp;
		if (x&y)
		{
			if ((sr = FastGetSubRect(c + y, r - x)) != NULL)
				if ((tmp = sr->FindTraction(Point, iConnection, Exclude->Traction)) != NULL)
					return tmp;
			if ((sr = FastGetSubRect(c - y, r + x)) != NULL)
				if ((tmp = sr->FindTraction(Point, iConnection, Exclude->Traction)) != NULL)
					return tmp;
		}
		if ((sr = FastGetSubRect(c - y, r - x)) != NULL)
			if ((tmp = sr->FindTraction(Point, iConnection, Exclude->Traction)) != NULL)
				return tmp;
	}
	return NULL;
}

/*
TGroundNode* TGround::CreateGroundNode()
{
TGroundNode *tmp= new TGroundNode();
//    RootNode->Prev= tmp;
tmp->Next= RootNode;
RootNode= tmp;
return tmp;
}

TGroundNode* TGround::GetVisible( AnsiString asName )
{
MessageBox(NULL,"Error","TGround::GetVisible( AnsiString asName ) is obsolete",MB_OK);
return RootNode->Find(asName);
//    return FirstVisible->FindVisible(asName);
}

TGroundNode* TGround::GetNode( AnsiString asName )
{
return RootNode->Find(asName);
}
*/
bool TGround::AddToQuery(TEvent *Event, TDynamicObject *Node)
{
	/*
	if (Event->bEnabled) //jeśli może być dodany do kolejki (nie używany w skanowaniu)
		if (!Event->iQueued) //jeśli nie dodany jeszcze do kolejki
		{//kolejka eventów jest posortowane względem (fStartTime)
			WriteLog("EVENT ADDED TO QUEUE: " + Event->asName + (Node ? AnsiString(" by " + Node->asName) : AnsiString("")));
			Event->Activator = Node;
			Event->fStartTime = fabs(Event->fDelay) + Timer::GetTime(); //czas od uruchomienia scenerii
			++Event->iQueued; //zabezpieczenie przed podwójnym dodaniem do kolejki
			if (QueryRootEvent ? Event->fStartTime >= QueryRootEvent->fStartTime : false)
				QueryRootEvent->AddToQuery(Event); //dodanie gdzieś w środku
			else
			{//dodanie z przodu: albo nic nie ma, albo ma być wykonany szybciej niż pierwszy
				Event->Next = QueryRootEvent;
				QueryRootEvent = Event;
			}
		}
		*/
	return true;
}


bool TGround::EventConditon(TEvent *e)
{//sprawdzenie spelnienia warunków dla eventu
	/*
	if (e->iFlags <= update_only) return true; //bezwarunkowo
	if (e->iFlags&conditional_trackoccupied)
		return (!e->Params[9].asTrack->IsEmpty());
	else if (e->iFlags&conditional_trackfree)
		return (e->Params[9].asTrack->IsEmpty());
	else if (e->iFlags&conditional_propability)
	{
		double rprobability = 1.0*rand() / RAND_MAX;
		WriteLog("Random integer: " + CurrToStr(rprobability) + "/" + CurrToStr(e->Params[10].asdouble));
		return (e->Params[10].asdouble>rprobability);
	}
	else if (e->iFlags&conditional_memcompare)
	{//porównanie wartości
		if (tmpEvent->Params[9].asMemCell->Compare(e->Params[10].asText, e->Params[11].asdouble, e->Params[12].asdouble, e->iFlags))
			return true;
		else if (Global::iWriteLogEnabled && DebugModeFlag)
		{//nie zgadza się więc sprawdzmy, co
			LogComment = e->Params[9].asMemCell->Text() + AnsiString(" ") + FloatToStrF(e->Params[9].asMemCell->Value1(), ffFixed, 8, 2) + " " + FloatToStrF(tmpEvent->Params[9].asMemCell->Value2(), ffFixed, 8, 2) + " != ";
			if (TestFlag(e->iFlags, conditional_memstring))
				LogComment += AnsiString(tmpEvent->Params[10].asText);
			else
				LogComment += "*";
			if (TestFlag(tmpEvent->iFlags, conditional_memval1))
				LogComment += " " + FloatToStrF(tmpEvent->Params[11].asdouble, ffFixed, 8, 2);
			else
				LogComment += " *";
			if (TestFlag(tmpEvent->iFlags, conditional_memval2))
				LogComment += " " + FloatToStrF(tmpEvent->Params[12].asdouble, ffFixed, 8, 2);
			else
				LogComment += " *";
			WriteLog(LogComment.c_str());
		}
	}
	*/
	return false;
};

bool TGround::CheckQuery()
{//sprawdzenie kolejki eventów oraz wykonanie tych, którym czas minął
	/*
	TLocation loc;
	int i;

	while (QueryRootEvent && (QueryRootEvent->fStartTime<Timer::GetTime()))
	{//eventy są posortowane wg czasu wykonania
		tmpEvent = QueryRootEvent; //wyjęcie eventu z kolejki
		if (QueryRootEvent->eJoined) //jeśli jest kolejny o takiej samej nazwie
		{//to teraz on będzie następny do wykonania
			QueryRootEvent = QueryRootEvent->eJoined; //następny będzie ten doczepiony
			QueryRootEvent->Next = tmpEvent->Next; //pamiętając o następnym z kolejki
			QueryRootEvent->fStartTime = tmpEvent->fStartTime; //czas musi być ten sam, bo nie jest aktualizowany
			QueryRootEvent->Activator = tmpEvent->Activator; //pojazd aktywujący
		}
		else //a jak nazwa jest unikalna, to kolejka idzie dalej
			QueryRootEvent = QueryRootEvent->Next; //NULL w skrajnym przypadku
		if (tmpEvent->bEnabled)
		{
			WriteLog("EVENT LAUNCHED: " + tmpEvent->asName + (tmpEvent->Activator ? AnsiString(" by " + tmpEvent->Activator->asName) : AnsiString("")));
			switch (tmpEvent->Type)
			{
			case tp_CopyValues: //skopiowanie wartości z innej komórki
				tmpEvent->Params[5].asMemCell->UpdateValues
					(tmpEvent->Params[9].asMemCell->Text(),
					tmpEvent->Params[9].asMemCell->Value1(),
					tmpEvent->Params[9].asMemCell->Value2(),
					tmpEvent->iFlags
					);
				break;
			case tp_AddValues: //różni się jedną flagą od UpdateValues
			case tp_UpdateValues:
				if (EventConditon(tmpEvent))
				{//teraz mogą być warunki do tych eventów
					tmpEvent->Params[5].asMemCell->UpdateValues(tmpEvent->Params[0].asText, tmpEvent->Params[1].asdouble, tmpEvent->Params[2].asdouble, tmpEvent->iFlags);
					//McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
					if (tmpEvent->Params[6].asTrack)
					{
						//loc.X= -tmpEvent->Params[8].asGroundNode->pCenter.x;
						//loc.Y=  tmpEvent->Params[8].asGroundNode->pCenter.z;
						//loc.Z=  tmpEvent->Params[8].asGroundNode->pCenter.y;
						for (int i = 0; i<tmpEvent->Params[6].asTrack->iNumDynamics; ++i)
						{
							//tmpEvent->Params[9].asMemCell->PutCommand(tmpEvent->Params[10].asTrack->Dynamics[i]->Mechanik,loc);
							tmpEvent->Params[5].asMemCell->PutCommand(tmpEvent->Params[6].asTrack->Dynamics[i]->Mechanik, &tmpEvent->Params[4].asGroundNode->pCenter);
						}
						if (DebugModeFlag)
							WriteLog("Type: UpdateValues & Track command - " + AnsiString(tmpEvent->Params[0].asText) + " " + AnsiString(tmpEvent->Params[1].asdouble) + " " + AnsiString(tmpEvent->Params[2].asdouble));
					}
					else
						if (DebugModeFlag)
							WriteLog("Type: UpdateValues - " + AnsiString(tmpEvent->Params[0].asText) + " " + AnsiString(tmpEvent->Params[1].asdouble) + " " + AnsiString(tmpEvent->Params[2].asdouble));
				}
				break;
			case tp_GetValues:
				if (tmpEvent->Activator)
				{
					//loc.X= -tmpEvent->Params[8].asGroundNode->pCenter.x;
					//loc.Y=  tmpEvent->Params[8].asGroundNode->pCenter.z;
					//loc.Z=  tmpEvent->Params[8].asGroundNode->pCenter.y;
					if (Global::iMultiplayer) //potwierdzenie wykonania dla serwera - najczęściej odczyt semafora
						WyslijEvent(tmpEvent->asName, tmpEvent->Activator->GetName());
					//tmpEvent->Params[9].asMemCell->PutCommand(tmpEvent->Activator->Mechanik,loc);
					tmpEvent->Params[9].asMemCell->PutCommand(tmpEvent->Activator->Mechanik, &tmpEvent->Params[8].asGroundNode->pCenter);
				}
				WriteLog("Type: GetValues");
				break;
			case tp_PutValues:
				if (tmpEvent->Activator)
				{
					loc.X = -tmpEvent->Params[3].asdouble; //zamiana, bo fizyka ma inaczej niż sceneria
					loc.Y = tmpEvent->Params[5].asdouble;
					loc.Z = tmpEvent->Params[4].asdouble;
					if (tmpEvent->Activator->Mechanik) //przekazanie rozkazu do AI
						tmpEvent->Activator->Mechanik->PutCommand(tmpEvent->Params[0].asText, tmpEvent->Params[1].asdouble, tmpEvent->Params[2].asdouble, loc);
					else
					{//przekazanie do pojazdu
						tmpEvent->Activator->MoverParameters->PutCommand(tmpEvent->Params[0].asText, tmpEvent->Params[1].asdouble, tmpEvent->Params[2].asdouble, loc);
					}
				}
				WriteLog("Type: PutValues");
				break;
			case tp_Lights:
				if (tmpEvent->Params[9].asModel)
					for (i = 0; i<iMaxNumLights; i++)
						if (tmpEvent->Params[i].asdouble >= 0) //-1 zostawia bez zmiany
							tmpEvent->Params[9].asModel->LightSet(i, tmpEvent->Params[i].asdouble); //teraz też ułamek
				break;
			case tp_Visible:
				if (tmpEvent->Params[9].asGroundNode)
					tmpEvent->Params[9].asGroundNode->bVisible = (tmpEvent->Params[i].asInt>0);
				break;
			case tp_Velocity:
				Error("Not implemented yet :(");
				break;
			case tp_Exit:
				MessageBox(0, tmpEvent->asNodeName.c_str(), " THE END ", MB_OK);
				Global::iTextMode = -1; //wyłączenie takie samo jak sekwencja F10 -> Y
				return false;
			case tp_Sound:
				if (tmpEvent->Params[0].asInt == 0)
					tmpEvent->Params[9].asRealSound->Stop();
				if (tmpEvent->Params[0].asInt == 1)
					tmpEvent->Params[9].asRealSound->Play(1, 0, true, tmpEvent->Params[9].asRealSound->vSoundPosition);
				if (tmpEvent->Params[0].asInt == -1)
					tmpEvent->Params[9].asRealSound->Play(1, DSBPLAY_LOOPING, true, tmpEvent->Params[9].asRealSound->vSoundPosition);
				break;
			case tp_Disable:
				Error("Not implemented yet :(");
				break;
			case tp_Animation: //Marcin: dorobic translacje - Ra: dorobiłem ;-)
				if (tmpEvent->Params[0].asInt == 1)
					tmpEvent->Params[9].asAnimContainer->SetRotateAnim(
					vector3(tmpEvent->Params[1].asdouble,
					tmpEvent->Params[2].asdouble,
					tmpEvent->Params[3].asdouble),
					tmpEvent->Params[4].asdouble);
				else if (tmpEvent->Params[0].asInt == 2)
					tmpEvent->Params[9].asAnimContainer->SetTranslateAnim(
					vector3(tmpEvent->Params[1].asdouble,
					tmpEvent->Params[2].asdouble,
					tmpEvent->Params[3].asdouble),
					tmpEvent->Params[4].asdouble);
				else if (tmpEvent->Params[0].asInt == 4)
					tmpEvent->Params[9].asModel->AnimationVND(
					tmpEvent->Params[8].asPointer,
					tmpEvent->Params[1].asdouble, //tu mogą być dodatkowe parametry, np. od-do
					tmpEvent->Params[2].asdouble,
					tmpEvent->Params[3].asdouble,
					tmpEvent->Params[4].asdouble);
				break;
			case tp_Switch:
				if (tmpEvent->Params[9].asTrack)
					tmpEvent->Params[9].asTrack->Switch(tmpEvent->Params[0].asInt);
				if (Global::iMultiplayer) //dajemy znać do serwera o przełożeniu
					WyslijEvent(tmpEvent->asName, ""); //wysłanie nazwy eventu przełączajacego
				//Ra: bardziej by się przydała nazwa toru, ale nie ma do niej stąd dostępu
				break;
			case tp_TrackVel:
				if (tmpEvent->Params[9].asTrack)
				{
					WriteLog("type: TrackVel");
					//WriteLog("Vel: ",tmpEvent->Params[0].asdouble);
					tmpEvent->Params[9].asTrack->VelocitySet(tmpEvent->Params[0].asdouble);
					if (DebugModeFlag)
						WriteLog("vel: ", tmpEvent->Params[9].asTrack->VelocityGet());
				}
				break;
			case tp_DynVel:
				Error("Event \"DynVel\" is obsolete");
				break;
			case tp_Multiple:
			{
				bCondition = EventConditon(tmpEvent);
				if (bCondition || (tmpEvent->iFlags&conditional_anyelse)) //warunek spelniony albo było użyte else
				{
					WriteLog("Multiple passed");
					for (i = 0; i<8; ++i)
					{//dodawane do kolejki w kolejności zapisania
						if (tmpEvent->Params[i].asEvent)
							if (bCondition != bool(tmpEvent->iFlags&(conditional_else << i)))
							{
								if (tmpEvent->Params[i].asEvent != tmpEvent)
									AddToQuery(tmpEvent->Params[i].asEvent, tmpEvent->Activator); //normalnie dodać
								else //jeśli ma być rekurencja
									if (tmpEvent->fDelay >= 5.0) //to musi mieć sensowny okres powtarzania
										if (tmpEvent->iQueued<2)
										{//trzeba zrobić wyjątek, aby event mógł się sam dodać do kolejki, raz już jest, ale będzie usunięty
											//pętla eventowa może być uruchomiona wiele razy, ale tylko pierwsze uruchomienie zadziała
											tmpEvent->iQueued = 0; //tymczasowo, aby był ponownie dodany do kolejki
											AddToQuery(tmpEvent, tmpEvent->Activator);
											tmpEvent->iQueued = 2; //kolejny raz już absolutnie nie dodawać
										}
							}
					}
					if (Global::iMultiplayer) //dajemy znać do serwera o wykonaniu
						if ((tmpEvent->iFlags&conditional_anyelse) == 0) //jednoznaczne tylko, gdy nie było else
						{
							if (tmpEvent->Activator)
								WyslijEvent(tmpEvent->asName, tmpEvent->Activator->GetName());
							else
								WyslijEvent(tmpEvent->asName, "");
						}
				}
			}
			break;
			case tp_WhoIs: //pobranie nazwy pociągu do komórki pamięci
				if (tmpEvent->iFlags&update_load)
				{//jeśli pytanie o ładunek
					if (tmpEvent->iFlags&update_memadd) //jeśli typ pojazdu
						tmpEvent->Params[9].asMemCell->UpdateValues(
						tmpEvent->Activator->MoverParameters->TypeName.c_str(), //typ pojazdu
						0, //na razie nic
						0, //na razie nic
						tmpEvent->iFlags&(update_memstring | update_memval1 | update_memval2));
					else //jeśli parametry ładunku
						tmpEvent->Params[9].asMemCell->UpdateValues(
						tmpEvent->Activator->MoverParameters->LoadType != "" ? tmpEvent->Activator->MoverParameters->LoadType.c_str() : "none", //nazwa ładunku
						tmpEvent->Activator->MoverParameters->Load, //aktualna ilość
						tmpEvent->Activator->MoverParameters->MaxLoad, //maksymalna ilość
						tmpEvent->iFlags&(update_memstring | update_memval1 | update_memval2));
				}
				else if (tmpEvent->iFlags&update_memadd)
				{//jeśli miejsce docelowe pojazdu
					tmpEvent->Params[9].asMemCell->UpdateValues(
						tmpEvent->Activator->asDestination.c_str(), //adres docelowy
						tmpEvent->Activator->DirectionGet(), //kierunek pojazdu względem czoła składu (1=zgodny,-1=przeciwny)
						tmpEvent->Activator->MoverParameters->Power, //moc pojazdu silnikowego: 0 dla wagonu
						tmpEvent->iFlags&(update_memstring | update_memval1 | update_memval2));
				}
				else if (tmpEvent->Activator->Mechanik)
					if (tmpEvent->Activator->Mechanik->Primary())
					{//tylko jeśli ktoś tam siedzi - nie powinno dotyczyć pasażera!
						tmpEvent->Params[9].asMemCell->UpdateValues(
							tmpEvent->Activator->Mechanik->TrainName().c_str(),
							tmpEvent->Activator->Mechanik->StationCount() - tmpEvent->Activator->Mechanik->StationIndex(), //ile przystanków do końca
							tmpEvent->Activator->Mechanik->IsStop() ? 1 : 0, //1, gdy ma tu zatrzymanie
							tmpEvent->iFlags);
						WriteLog("Train detected: " + tmpEvent->Activator->Mechanik->TrainName());
					}
				break;
			case tp_LogValues: //zapisanie zawartości komórki pamięci do logu
				if (tmpEvent->Params[9].asMemCell) //jeśli była podana nazwa komórki
					WriteLog("Memcell \"" + tmpEvent->asNodeName + "\": " +
					tmpEvent->Params[9].asMemCell->Text() + " " +
					tmpEvent->Params[9].asMemCell->Value1() + " " +
					tmpEvent->Params[9].asMemCell->Value2());
				else //lista wszystkich
					for (TGroundNode *Current = nRootOfType[TP_MEMCELL]; Current; Current = Current->Next)
						WriteLog("Memcell \"" + Current->asName + "\": " +
						Current->MemCell->Text() + " " +
						Current->MemCell->Value1() + " " +
						Current->MemCell->Value2());
				break;
			} //switch (tmpEvent->Type)
		} //if (tmpEvent->bEnabled)
		--tmpEvent->iQueued; //teraz moze być ponownie dodany do kolejki

	}  //while

	*/
	return true;
}

void TGround::OpenGLUpdate(HDC hDC)
{
	SwapBuffers(hDC); //swap buffers (double buffering)
}


bool TGround::Update(double dt, int iter)
{//dt=krok czasu [s], dt*iter=czas od ostatnich przeliczeń
	if (iter>1) //ABu: ponizsze wykonujemy tylko jesli wiecej niz jedna iteracja
	{//pierwsza iteracja i wyznaczenie stalych:
		for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
		{
//-			Current->DynamicObject->MoverParameters->ComputeConstans();
//-			Current->DynamicObject->CoupleDist();
//-			Current->DynamicObject->UpdateForce(dt, dt, false);
		}
//-		for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
//-			Current->DynamicObject->FastUpdate(dt);
		//pozostale iteracje
		for (int i = 1; i<(iter - 1); ++i) //jeśli iter==5, to wykona się 3 razy
		{
//-			for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
//-				Current->DynamicObject->UpdateForce(dt, dt, false);
//-			for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
//-				Current->DynamicObject->FastUpdate(dt);
		}
		//ABu 200205: a to robimy tylko raz, bo nie potrzeba więcej
		//Winger 180204 - pantografy
		double dt1 = dt*iter; //całkowity czas
		//if (Global::bEnableTraction) //bLoadTraction?
		{//Ra: zmienić warunek na sprawdzanie pantografów w jednej zmiennej: czy pantografy i czy podniesione
			for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
			{
//-				if ((Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType == CurrentCollector)
//-					//ABu: usunalem, bo sie krzaczylo: && (Current->DynamicObject->MoverParameters->PantFrontUp || Current->DynamicObject->MoverParameters->PantRearUp))
//-					//     a za to dodalem to:
//-					&& (Current->DynamicObject->MoverParameters->CabNo != 0))
//-					GetTraction(Current->DynamicObject);
//-				Current->DynamicObject->UpdateForce(dt, dt1, true);//,true);
			}
//-			for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
//-				Current->DynamicObject->Update(dt, dt1);
		}

	}
	else
	{//jezeli jest tylko jedna iteracja
		//if (Global::bEnableTraction) //bLoadTraction?
		{
			for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
			{
			//-	if ((Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType == CurrentCollector)
			//-		&& (Current->DynamicObject->MoverParameters->CabNo != 0)) GetTraction(Current->DynamicObject);
			//-	Current->DynamicObject->MoverParameters->ComputeConstans();
			//-	Current->DynamicObject->CoupleDist();
			//-	Current->DynamicObject->UpdateForce(dt, dt, true);//,true);
			}
			//-for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
			//-	Current->DynamicObject->Update(dt, dt);
		}

	}
	if (bDynamicRemove)
	{//jeśli jest coś do usunięcia z listy, to trzeba na końcu
	//-	for (TGroundNode *Current = nRootDynamic; Current; Current = Current->Next)
	//-		if (!Current->DynamicObject->bEnabled)
	//-		{
	//-			DynamicRemove(Current->DynamicObject); //usunięcie tego i podłączonych
	//-			Current = nRootDynamic; //sprawdzanie listy od początku
	//-		}
		bDynamicRemove = false; //na razie koniec
	}
	return true;
}



bool TGround::RenderDL(vector3 pPosition)
{//renderowanie scenerii z Display List - faza nieprzezroczystych
	++TGroundRect::iFrameNumber; //zwięszenie licznika ramek (do usuwniania nadanimacji)
	CameraDirection.x = sin(Global::pCameraRotation); //wektor kierunkowy
	CameraDirection.z = cos(Global::pCameraRotation);
	int tr, tc;
	TGroundNode *node;
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
	int n = 2 * iNumSubRects; //(2*==2km) promień wyświetlanej mapy w sektorach
	int c = GetColFromX(pPosition.x);
	int r = GetRowFromZ(pPosition.z);
	TSubRect *tmp;
	for (node = srGlobal.nRenderHidden; node; node = node->nNext3)
		node->RenderHidden(); //rednerowanie globalnych (nie za często?)
	int i, j, k;
	//renderowanie czołgowe dla obiektów aktywnych a niewidocznych
	for (j = r - n; j <= r + n; j++)
		for (i = c - n; i <= c + n; i++)
			if ((tmp = FastGetSubRect(i, j)) != NULL)
			{
				tmp->LoadNodes(); //oznaczanie aktywnych sektorów
				for (node = tmp->nRenderHidden; node; node = node->nNext3)
					node->RenderHidden();
				//TODO: jeszcze dźwięki pojazdów by się przydały
			}
	//renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
	iRendered = 0; //ilość renderowanych sektorów
	vector3 direction;
	for (k = 0; k<Global::iSegmentsRendered; ++k) //sektory w kolejności odległości
	{//przerobione na użycie SectorOrder
		i = SectorOrder[k].x; //na starcie oba >=0
		j = SectorOrder[k].y;
		do
		{
			if (j <= 0) i = -i; //pierwszy przebieg: j<=0, i>=0; drugi: j>=0, i<=0; trzeci: j<=0, i<=0 czwarty: j>=0, i>=0;
			j = -j; //i oraz j musi być zmienione wcześniej, żeby continue działało
			direction = vector3(i, 0, j); //wektor od kamery do danego sektora
			if (LengthSquared3(direction)>5) //te blisko są zawsze wyświetlane
			{
				direction = SafeNormalize(direction); //normalizacja
				if (CameraDirection.x*direction.x + CameraDirection.z*direction.z<0.55)
					continue; //pomijanie sektorów poza kątem patrzenia
			}
			Rects[(i + c) / iNumSubRects][(j + r) / iNumSubRects].RenderDL(); //kwadrat kilometrowy nie zawsze, bo szkoda FPS
			if ((tmp = FastGetSubRect(i + c, j + r)) != NULL)
				if (tmp->iNodeCount) //o ile są jakieś obiekty, bo po co puste sektory przelatywać
					pRendered[iRendered++] = tmp; //tworzenie listy sektorów do renderowania
		} while ((i<0) || (j<0)); //są 4 przypadki, oprócz i=j=0
	}
	for (i = 0; i<iRendered; i++)
		pRendered[i]->RenderDL(); //renderowanie nieprzezroczystych
	return true;
}

bool TGround::RenderAlphaDL(vector3 pPosition)
{//renderowanie scenerii z Display List - faza przezroczystych
	TGroundNode *node;
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	TSubRect *tmp;
	//Ra: renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
	int i;
	for (i = iRendered - 1; i >= 0; --i) //od najdalszych
	{//przezroczyste trójkąty w oddzielnym cyklu przed modelami
		tmp = pRendered[i];
		for (node = tmp->nRenderRectAlpha; node; node = node->nNext3)
			node->RenderAlphaDL(); //przezroczyste modele
	}
	for (i = iRendered - 1; i >= 0; --i) //od najdalszych
	{//renderowanie przezroczystych modeli oraz pojazdów
		pRendered[i]->RenderAlphaDL();
	}
	glDisable(GL_LIGHTING); //linie nie powinny świecić
	for (i = iRendered - 1; i >= 0; --i) //od najdalszych
	{//druty na końcu, żeby się nie robiły białe plamy na tle lasu
		tmp = pRendered[i];
		for (node = tmp->nRenderWires; node; node = node->nNext3)
			node->RenderAlphaDL(); //druty
	}
	return true;
}


bool TGround::RenderVBO(vector3 pPosition)
{//renderowanie scenerii z VBO - faza nieprzezroczystych
	++TGroundRect::iFrameNumber; //zwięszenie licznika ramek
	CameraDirection.x = sin(Global::pCameraRotation); //wektor kierunkowy
	CameraDirection.z = cos(Global::pCameraRotation);
	int tr, tc;
	TGroundNode *node;
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
	int n = 2 * iNumSubRects; //(2*==2km) promień wyświetlanej mapy w sektorach
	int c = GetColFromX(pPosition.x);
	int r = GetRowFromZ(pPosition.z);
	TSubRect *tmp;
	for (node = srGlobal.nRenderHidden; node; node = node->nNext3)
		node->RenderHidden(); //rednerowanie globalnych (nie za często?)
	int i, j, k;
	//renderowanie czołgowe dla obiektów aktywnych a niewidocznych
	for (j = r - n; j <= r + n; j++)
		for (i = c - n; i <= c + n; i++)
		{
			if ((tmp = FastGetSubRect(i, j)) != NULL)
				for (node = tmp->nRenderHidden; node; node = node->nNext3)
					node->RenderHidden();
			//TODO: jeszcze dźwięki pojazdów by się przydały
		}
	//renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
	iRendered = 0; //ilość renderowanych sektorów
	vector3 direction;
	for (k = 0; k<Global::iSegmentsRendered; ++k) //sektory w kolejności odległości
	{//przerobione na użycie SectorOrder
		i = SectorOrder[k].x; //na starcie oba >=0
		j = SectorOrder[k].y;
		do
		{
			if (j <= 0) i = -i; //pierwszy przebieg: j<=0, i>=0; drugi: j>=0, i<=0; trzeci: j<=0, i<=0 czwarty: j>=0, i>=0;
			j = -j; //i oraz j musi być zmienione wcześniej, żeby continue działało
			direction = vector3(i, 0, j); //wektor od kamery do danego sektora
			if (LengthSquared3(direction)>5) //te blisko są zawsze wyświetlane
			{
				direction = SafeNormalize(direction); //normalizacja
				if (CameraDirection.x*direction.x + CameraDirection.z*direction.z<0.55)
					continue; //pomijanie sektorów poza kątem patrzenia
			}
			Rects[(i + c) / iNumSubRects][(j + r) / iNumSubRects].RenderVBO(); //kwadrat kilometrowy nie zawsze, bo szkoda FPS
			if ((tmp = FastGetSubRect(i + c, j + r)) != NULL)
				if (tmp->iNodeCount) //jeżeli są jakieś obiekty, bo po co puste sektory przelatywać
					pRendered[iRendered++] = tmp; //tworzenie listy sektorów do renderowania
		} while ((i<0) || (j<0)); //są 4 przypadki, oprócz i=j=0
	}

	//dodać rednerowanie terenu z E3D - jedno VBO jest używane dla całego modelu, chyba że jest ich więcej
	//-if (Global::pTerrainCompact)
	//-	Global::pTerrainCompact->TerrainRenderVBO(TGroundRect::iFrameNumber);

	for (i = 0; i<iRendered; i++)
	{//renderowanie nieprzezroczystych
		pRendered[i]->RenderVBO();
	}
	return true;
}


bool TGround::RenderAlphaVBO(vector3 pPosition)
{//renderowanie scenerii z VBO - faza przezroczystych
	TGroundNode *node;
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	TSubRect *tmp;
	int i;
	for (i = iRendered - 1; i >= 0; --i) //od najdalszych
	{//renderowanie przezroczystych trójkątów sektora
		tmp = pRendered[i];
		tmp->LoadNodes(); //ewentualne tworzenie siatek
		if (tmp->StartVBO())
		{
			for (node = tmp->nRenderRectAlpha; node; node = node->nNext3)
				if (node->iVboPtr >= 0)
					node->RenderAlphaVBO(); //nieprzezroczyste obiekty terenu
			tmp->EndVBO();
		}
	}
	for (i = iRendered - 1; i >= 0; --i) //od najdalszych
		pRendered[i]->RenderAlphaVBO(); //przezroczyste modeli oraz pojazdy
	glDisable(GL_LIGHTING); //linie nie powinny świecić
	for (i = iRendered - 1; i >= 0; --i) //od najdalszych
	{//druty na końcu, żeby się nie robiły białe plamy na tle lasu
		tmp = pRendered[i];
		if (tmp->StartVBO())
		{
			for (node = tmp->nRenderWires; node; node = node->nNext3)
				node->RenderAlphaVBO(); //przezroczyste modele
			tmp->EndVBO();
		}
	}
	return true;
};


//---------------------------------------------------------------------------
void TGround::Navigate(std::string ClassName, UINT Msg, WPARAM wParam, LPARAM lParam)
{//wysłanie komunikatu do sterującego
	HWND h = FindWindow(ClassName.c_str(), 0); //można by to zapamiętać
	SendMessage(h, Msg, wParam, lParam);
};
//--------------------------------
void TGround::WyslijEvent(const std::string &e, const std::string &d)
{//Ra: jeszcze do wyczyszczenia
	/*
	DaneRozkaz r;
	r.iSygn = 'EU07';
	r.iComm = 2; //2 - event
	int i = e.Length(), j = d.Length();
	r.cString[0] = char(i);
	strcpy(r.cString + 1, e.c_str()); //zakończony zerem
	r.cString[i + 2] = char(j); //licznik po zerze kończącym
	strcpy(r.cString + 3 + i, d.c_str()); //zakończony zerem
	COPYDATASTRUCT cData;
	cData.dwData = 'EU07'; //sygnatura
	cData.cbData = 12 + i + j; //8+dwa liczniki i dwa zera kończące
	cData.lpData = &r;
	Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
	*/
};
//---------------------------------------------------------------------------
void TGround::WyslijString(const std::string &t, int n)
{//wysłanie informacji w postaci pojedynczego tekstu
	/*
	DaneRozkaz r;
	r.iSygn = 'EU07';
	r.iComm = n; //numer komunikatu
	int i = t.Length();
	r.cString[0] = char(i);
	strcpy(r.cString + 1, t.c_str()); //z zerem kończącym
	COPYDATASTRUCT cData;
	cData.dwData = 'EU07'; //sygnatura
	cData.cbData = 10 + i; //8+licznik i zero kończące
	cData.lpData = &r;
	Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
	*/
};
//---------------------------------------------------------------------------
void TGround::WyslijWolny(const std::string &t)
{//Ra: jeszcze do wyczyszczenia
	WyslijString(t, 4); //tor wolny
};
//--------------------------------
void TGround::WyslijNamiary(TGroundNode* t)
{//wysłanie informacji o pojeździe - (float), długość ramki będzie zwiększana w miarę potrzeby
	/*
	DaneRozkaz r;
	r.iSygn = 'EU07';
	r.iComm = 7; //7 - dane pojazdu
	int i = 9, j = t->asName.Length();
	r.iPar[0] = i; //ilość danych liczbowych
	r.fPar[1] = Global::fTimeAngleDeg / 360.0; //aktualny czas (1.0=doba)
	r.fPar[2] = t->DynamicObject->MoverParameters->Loc.X; //pozycja X
	r.fPar[3] = t->DynamicObject->MoverParameters->Loc.Y; //pozycja Y
	r.fPar[4] = t->DynamicObject->MoverParameters->Loc.Z; //pozycja Z
	r.fPar[5] = t->DynamicObject->MoverParameters->V; //prędkość ruchu X
	r.fPar[6] = 0; //prędkość ruchu Y
	r.fPar[7] = 0; //prędkość ruchu Z
	r.fPar[8] = t->DynamicObject->MoverParameters->AccV; //przyspieszenie X
	//r.fPar[ 9]=0; //przyspieszenie Y //na razie nie
	//r.fPar[10]=0; //przyspieszenie Z
	i <<= 2; //ilość bajtów
	r.cString[i] = char(j); //na końcu nazwa, żeby jakoś zidentyfikować
	strcpy(r.cString + i + 1, t->asName.c_str()); //zakończony zerem
	COPYDATASTRUCT cData;
	cData.dwData = 'EU07'; //sygnatura
	cData.cbData = 10 + i + j; //8+licznik i zero kończące
	cData.lpData = &r;
	Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
	*/
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TGround::RadioStop(vector3 pPosition)
{//zatrzymanie pociągów w okolicy
	TGroundNode *node;
	TSubRect *tmp;
	int c = GetColFromX(pPosition.x);
	int r = GetRowFromZ(pPosition.z);
	int i, j;
	int n = 2 * iNumSubRects; //przeglądanie czołgowe okolicznych torów w kwadracie 4km×4km
	for (j = r - n; j <= r + n; j++)
		for (i = c - n; i <= c + n; i++)
			if ((tmp = FastGetSubRect(i, j)) != NULL)
				for (node = tmp->nRootNode; node != NULL; node = node->nNext2)
					if (node->iType == TP_TRACK)
						node->pTrack->RadioStop(); //przekazanie do każdego toru w każdym segmencie
};

TDynamicObject* TGround::DynamicNearest(vector3 pPosition, double distance, bool mech)
{//wyszukanie pojazdu najbliższego względem (pPosition)

	TGroundNode *node;
	TSubRect *tmp;
	TDynamicObject *dyn = NULL;
	/*
	int c = GetColFromX(pPosition.x);
	int r = GetRowFromZ(pPosition.z);
	int i, j, k;
	double sqm = distance*distance, sqd; //maksymalny promien poszukiwań do kwadratu
	for (j = r - 1; j <= r + 1; j++) //plus dwa zewnętrzne sektory, łącznie 9
		for (i = c - 1; i <= c + 1; i++)
			if ((tmp = FastGetSubRect(i, j)) != NULL)
				for (node = tmp->nRootNode; node; node = node->nNext2) //następny z sektora
					if (node->iType == TP_TRACK) //Ra: przebudować na użycie tabeli torów?
						for (k = 0; k<node->pTrack->iNumDynamics; k++)
							if ((sqd = SquareMagnitude(node->pTrack->Dynamics[k]->GetPosition() - pPosition))<sqm)
								if (mech ? (node->pTrack->Dynamics[k]->Mechanik != NULL) : true) //czy ma mieć obsadę
								{
									sqm = sqd; //nowa odległość
									dyn = node->pTrack->Dynamics[k]; //nowy lider
								}
	*/
	return dyn;
};

//---------------------------------------------------------------------------
void TGround::DynamicRemove(TDynamicObject* dyn)
{//Ra: usunięcie pojazdów ze scenerii (gdy dojadą na koniec i nie sa potrzebne)
/*
	TDynamicObject* d = dyn->Prev();
	if (d) //jeśli coś jest z przodu
		DynamicRemove(d); //zaczynamy od tego z przodu
	else
	{//jeśli mamy już tego na początku
		TGroundNode **n, *node;
		d = dyn; //od pierwszego
		while (d)
		{
			if (d->MyTrack) d->MyTrack->RemoveDynamicObject(d); //usunięcie z toru o ile nie usunięty
			n = &nRootDynamic; //lista pojazdów od początku
			//node=NULL; //nie znalezione
			while (*n ? (*n)->DynamicObject != d : false)
			{//usuwanie z listy pojazdów
				n = &((*n)->Next); //sprawdzenie kolejnego pojazdu na liście
			}
			if ((*n)->DynamicObject == d)
			{//jeśli znaleziony
				node = (*n); //zapamiętanie węzła, aby go usunąć
				(*n) = node->Next; //pominięcie na liście
			//-	Global::TrainDelete(d);                                                    // TODO:
				d = d->Next(); //przejście do kolejnego pojazdu, póki jeszcze jest
				delete node; //usuwanie fizyczne z pamięci
			}
			else
				d = NULL; //coś nie tak!
		}
	}
	*/
};


void TGround::TrackBusyList()
{//wysłanie informacji o wszystkich zajętych odcinkach
	TGroundNode *Current;
	TTrack *Track;
	std::string name;
	for (Current = nRootOfType[TP_TRACK]; Current; Current = Current->Next)
		if (Current->asName.empty()) //musi być nazwa
			if (Current->pTrack->iNumDynamics) //osi to chyba nie ma jak policzyć
				WyslijString(Current->asName, 8); //zajęty
};
//---------------------------------------------------------------------------
