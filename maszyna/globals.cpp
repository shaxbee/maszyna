//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#pragma hdrstop

#include "commons.h"
#include "Globals.h"
#include "usefull.h"
int Global::Keys[MaxKeys];
vector3 Global::pCameraPosition;
double Global::pCameraRotation;
double Global::pCameraRotationDeg;
vector3 Global::pFreeCameraInit[10];
vector3 Global::pFreeCameraInitAngle[10];
int Global::iWindowWidth = 800;
int Global::iWindowHeight = 600;

GLuint Global::fonttexturex;
GLfloat  Global::AtmoColor[] = { 0.6f, 0.7f, 0.8f };
GLfloat  Global::FogColor[] = { 0.6f, 0.7f, 0.8f };
GLfloat  Global::ambientDayLight[] = { 0.40f, 0.40f, 0.45f, 1.0f };
GLfloat  Global::diffuseDayLight[] = { 0.55f, 0.54f, 0.50f, 1.0f };
GLfloat  Global::specularDayLight[] = { 0.95f, 0.94f, 0.90f, 1.0f };
GLfloat  Global::whiteLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat  Global::noLight[] = { 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat  Global::lightPos[4];

bool Global::bFreeFly = false;
bool Global::bFreeFlyModeFlag = false;

void __fastcall Global::SetCameraPosition(vector3 pNewCameraPosition)
{
	pCameraPosition = pNewCameraPosition;
}

void __fastcall Global::SetCameraRotation(double Yaw)
{//ustawienie bezwzglêdnego kierunku kamery z korekcj¹ do przedzia³u <-M_PI,M_PI>
	pCameraRotation = Yaw;
	while (pCameraRotation<-M_PI) pCameraRotation += 2 * M_PI;
	while (pCameraRotation> M_PI) pCameraRotation -= 2 * M_PI;
	pCameraRotationDeg = pCameraRotation*180.0 / M_PI;
}


#pragma package(smart_init)
