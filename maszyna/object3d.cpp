//---------------------------------------------------------------------------

/*
MaSzyna EU07 locomotive simulator
Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#pragma hdrstop

#include "commons.h"
#include "commons_usr.h"



TSubObject::TSubObject()
{
	WriteLog("TSubObject::TSubObject()");
	ZeroMemory(this, sizeof(TSubModel)); //istotne przy zapisywaniu wersji binarnej
	FirstInit();
};

void TSubObject::FirstInit()
{
	WriteLog("void TSubObject::FirstInit()");
	eType = TP_ROTATOR;
	Vertices = NULL;
	uiDisplayList = 0;
	iNumVerts = -1; //do sprawdzenia
	iVboPtr = -1;
	fLight = -1.0; //œwietcenie wy³¹czone
	v_RotateAxis = float3(0, 0, 0);
	v_TransVector = float3(0, 0, 0);
	f_Angle = 0;
	b_Anim = qat_None;
	b_aAnim = qat_None;
	fVisible = 0.0; //zawsze widoczne
	iVisible = 1;
	fMatrix = NULL; //to samo co iMatrix=0;
	Next = NULL;
	Child = NULL;
	TextureID = 0;
	//TexAlpha=false;
	iFlags = 0x0200; //bit 9=1: submodel zosta³ utworzony a nie ustawiony na wczytany plik
	//TexHash=false;
	//Hits=NULL;
	//CollisionPts=NULL;
	//CollisionPtsCount=0;
	Opacity = 1.0; //przy wczytywaniu modeli by³o dzielone przez 100...
	bWire = false;
	fWireSize = 0;
	fNearAttenStart = 40;
	fNearAttenEnd = 80;
	bUseNearAtten = false;
	iFarAttenDecay = 0;
	fFarDecayRadius = 100;
	fCosFalloffAngle = 0.5; //120°?
	fCosHotspotAngle = 0.3; //145°?
	fCosViewAngle = 0;
	fSquareMaxDist = 10000 * 10000; //10km
	fSquareMinDist = 0;
	iName = -1; //brak nazwy
	iTexture = 0; //brak tekstury
	//asName="";
	//asTexture="";
	pName = pTexture = NULL;
	f4Ambient[0] = f4Ambient[1] = f4Ambient[2] = f4Ambient[3] = 1.0; //{1,1,1,1};
	f4Diffuse[0] = f4Diffuse[1] = f4Diffuse[2] = f4Diffuse[3] = 1.0; //{1,1,1,1};
	f4Specular[0] = f4Specular[1] = f4Specular[2] = 0.0; f4Specular[3] = 1.0; //{0,0,0,1};
	f4Emision[0] = f4Emision[1] = f4Emision[2] = f4Emision[3] = 1.0;
	smLetter = NULL; //u¿ywany tylko roboczo dla TP_TEXT, do przyspieszenia wyœwietlania
};

TSubObject::~TSubObject()
{
	if (uiDisplayList) glDeleteLists(uiDisplayList, 1);
	if (iFlags & 0x0200)
	{//wczytany z pliku tekstowego musi sam posprz¹taæ
		//SafeDeleteArray(Indices);
		SafeDelete(Next);
		SafeDelete(Child);
		delete fMatrix; //w³asny transform trzeba usun¹æ (zawsze jeden)
		delete[] Vertices;
		delete[] pTexture;
		delete[] pName;
	}

	delete[] smLetter; //u¿ywany tylko roboczo dla TP_TEXT, do przyspieszenia wyœwietlania
};


TObject3d::TObject3d()
{
	WriteLog("TObject3d::TObject3d()");
	//Root = NULL;
	iFlags = 0;
	iSubModelsCount = 0;
	//iModel = NULL; //tylko jak wczytany model binarny
	iNumVerts = 0; //nie ma jeszcze wierzcho³ków
	WriteLog("TObject3d::TObject3d() end");
};


TObject3d::~TObject3d()
{
	WriteLog("TObject3d::~TObject3d()");
};


TSubObject* TObject3d::GetFromNameQ(char *sName)
{//wyszukanie submodelu po nazwie
	if (!sName) return Root; //potrzebne do terenu z E3D
	if (iFlags & 0x0200) //wczytany z pliku tekstowego, wyszukiwanie rekurencyjne
		return Root ? Root->GetFromNameQ(sName) : NULL;
	else //wczytano z pliku binarnego, mo¿na wyszukaæ iteracyjnie
	{
		//for (int i=0;i<iSubModelsCount;++i)
		return Root ? Root->GetFromNameQ(sName) : NULL;
	}
};

TSubObject* TSubObject::GetFromNameQ(char* search, bool i)
{
	return GetFromNameQ(search, i);
};

