//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "Texture.h"
#include "MdlMngr.h"
#include "Globals.h"
#include "object3d.h"

//#define SeekFiles AnsiString("*.t3d")

TMdlContainer::TMdlContainer()
{
	//Name = NULL; 
	//Model3D = NULL;
}

TMdlContainer::~TMdlContainer()
{
 //SafeDeleteArray(Name); 
 //SafeDelete(Model); 
}

TObject3d* TMdlContainer::LoadModel(static char *newName, bool dynamic)
{
WriteLog("TModel3d* TMdlContainer::LoadModel(char *newName,bool dynamic)");
WriteLog(newName);

 //SafeDeleteArray(Name);
// SafeDelete(Model);

 WriteLog("--");

//Name= new char[strlen(newName)+1];
// std::string Name;
// Name = chartostdstr(newName);
//strcpy(Name,newName);
 
 WriteLog( "INFO1");


 Model3D=new TObject3d();


 Sleep(100); WriteLog("INFO2");
 Sleep(100); WriteLog("INFO2");
 Sleep(100); WriteLog("INFO2");
 Sleep(100); WriteLog("INFO2");
 Sleep(100);

// if (!Model->LoadFromFile(Name, dynamic)) //np. "models\\pkp/head1-y.t3d"
//  SafeDelete(Model);
 return Model3D;
};

TMdlContainer *TModelsManager::Models;
int TModelsManager::Count;
int MAX_MODELS= 700;

void TModelsManager::Init()
{
	Models = new TMdlContainer[700];
    Count= 0;
}

void TModelsManager::Free()
{
    SafeDeleteArray(Models);
}



double Radius;


TObject3d* TModelsManager::LoadModel(static char *Name, bool dynamic)
{
WriteLog("TObject3d*  TModelsManager::LoadModel(char *Name, bool dynamic)");
 TObject3d *mdl=NULL;

 if (Count==700)
  WriteLogSS("FIXME: Too many models, program will now crash :)", "ERROR");
 else
 {
  mdl=Models[Count].LoadModel(Name,dynamic);
  if (mdl) Count++; //jeœli b³¹d wczytania modelu, to go nie wliczamy
 }

 return mdl;
}

TObject3d* TModelsManager::GetModel(static char *Name, bool dynamic)
{
 WriteLog("TModelsManager::GetModel()");

 char buf[255];
// std::string buftp=Global::asCurrentTexturePath;
 TObject3d* tmpModel; //tymczasowe zmienne

 
 //WriteLogSS("["+chartostdstr(Name)+"]", "INFO");
 if (strchr(Name,'\\')==NULL)
 {
  WriteLog("ADD DEF PATH");
  //WriteLogSS("[" + chartostdstr(Name) + "]", "INFO");
  strcpy(buf,"models\\"); //Ra: by³o by lepiej katalog dodaæ w parserze
  strcat(buf,Name);
  //WriteLogSS("[" + chartostdstr(buf) + "]", "INFO");
  if (strchr(Name,'/')!=NULL)
  {
  //- Global::asCurrentTexturePath= Global::asCurrentTexturePath + chartostdstr(Name);
  //- Global::asCurrentTexturePath.erase(Global::asCurrentTexturePath.Pos("/")+1, Global::asCurrentTexturePath.length());
  }
 }
 else

 {
  strcpy(buf,Name);

  if (dynamic) //na razie tak, bo nie wiadomo, jaki mo¿e mieæ wp³yw na pozosta³e modele
   if (strchr(Name,'/')!=NULL)
   {//pobieranie tekstur z katalogu, w którym jest model
    //Global::asCurrentTexturePath= Global::asCurrentTexturePath+ Name;
    //Global::asCurrentTexturePath.erase(Global::asCurrentTexturePath.find("/")+1,Global::asCurrentTexturePath.length());
   }
 
 }

 
 ToLowerCase(buf);
 WriteLog("CHECK MODELS TAB");
 for (int i=0; i<Count; i++)
 {
  if (strcmp(buf,Models[i].Name)==0)
  {
	  
  // Global::asCurrentTexturePath= buftp;
   return (Models[i].Model3D);
  }
 };
 

 WriteLog("FIRST INSTANCE");
 WriteLog(buf);
 tmpModel = LoadModel(buf, dynamic);
// Global::asCurrentTexturePath=buftp;
 return (tmpModel); //NULL jeœli b³¹d

};


//---------------------------------------------------------------------------
#pragma package(smart_init)
