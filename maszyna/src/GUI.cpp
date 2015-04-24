//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others
*/
// 2015.04.13 has included other soundsystem files
// 2015.04.13 starts changing char pointer type into CString (more flexible)
// 2015.04.14 removed many compiler warnings

#include "commons.h"
#include "commons_usr.h"
#pragma hdrstop


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// RenderLoader() - SCREEN WCZYTYWANIA SCENERII ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

bool TWorld::RenderLoader(HDC hDC, int node, std::string text)
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, Global::iWindowWidth, Global::iWindowHeight, 0, -100, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0.8f, 0.8f, 0.8f, 0.9f);     // 09 07 04 07

	//glDisable(GL_DEPTH_TEST);			// Disables depth testing
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);

	//if (!floaded) BFONT = new Font();
	//if (!floaded) BFONT->loadf("none");
	//floaded = true;

	glEnable(GL_TEXTURE_2D);

	//    if (node != 77) nn = Global::iPARSERBYTESPASSED;

	//    Global::postep = (Global::iPARSERBYTESPASSED * 100 / Global::iNODES ); // PROCENT
	//    currloading = AnsiString(Global::postep) + "%";
	//    currloading_bytes = IntToStr(Global::iPARSERBYTESPASSED);

	// else { currloading = IntToStr(nn) + "N, PIERWSZE WCZYTYWANIE"; }
	//currloading_b = text;

	//glColor4f(1.0,1.0,1.0,1);
	glColor4f(0.9f, 0.7f, 0.7f, 1.0f);
	int margin = 1;

	//glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	//glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	//glColor4f(0.5,0.45,0.4, 0.5);


	Global::aspectratio = 169;
	// OBRAZEK LOADERA ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	if (Global::aspectratio == 43)
	{
		glBindTexture(GL_TEXTURE_2D, Global::loaderbackg);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3i(margin - 174, margin, 0);   // GORNY LEWY
		glTexCoord2f(0, 0); glVertex3i(margin - 174, Global::iWindowHeight - margin, 0); // DOLY LEWY
		glTexCoord2f(1, 0); glVertex3i(Global::iWindowWidth - margin + 174, Global::iWindowHeight - margin, 0); // DOLNY PRAWY
		glTexCoord2f(1, 1); glVertex3i(Global::iWindowWidth - margin + 174, margin, 0);   // GORNY PRAWY
		glEnd();
	}

	if (Global::aspectratio == 169)
	{
		int pm = 0;
		glBindTexture(GL_TEXTURE_2D, Global::loaderbackg);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3i(margin - pm, margin, 0);   // GORNY LEWY
		glTexCoord2f(0, 0); glVertex3i(margin - pm, Global::iWindowHeight - margin, 0); // DOLY LEWY
		glTexCoord2f(1, 0); glVertex3i(Global::iWindowWidth - margin + pm, Global::iWindowHeight - margin, 0); // DOLNY PRAWY
		glTexCoord2f(1, 1); glVertex3i(Global::iWindowWidth - margin + pm, margin, 0);   // GORNY PRAWY
		glEnd();
	}


	// LOGO PROGRAMU ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	/*
	if (LDR_LOGOVIS !=0)
	{
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glColor4f(0.8,0.8,0.8,LDR_MLOGO_A);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, Global::loaderlogo);
	glBegin( GL_QUADS );
	glTexCoord2f(0, 1); glVertex3i( LDR_MLOGO_X,   LDR_MLOGO_Y,0);   // GORNY LEWY
	glTexCoord2f(0, 0); glVertex3i( LDR_MLOGO_X,   LDR_MLOGO_Y+100,0); // DOLY LEWY
	glTexCoord2f(1, 0); glVertex3i( LDR_MLOGO_X+300, LDR_MLOGO_Y+100,0); // DOLNY PRAWY
	glTexCoord2f(1, 1); glVertex3i( LDR_MLOGO_X+300, LDR_MLOGO_Y,0);   // GORNY PRAWY
	glEnd( );
	}
	*/

	// BRIEFING - OPIS MISJI ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	/*
	if (LDR_DESCVIS != 0 && Global::bloaderbriefing)
	{
	glDisable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_COLOR);  // PRAWIE OK

	glColor4f(1.9, 1.9, 1.9, 1.9);
	//glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, loaderbrief);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1); glVertex3i(LDR_BRIEF_X, LDR_BRIEF_Y, 0);   // GORNY LEWY
	glTexCoord2f(0, 0); glVertex3i(LDR_BRIEF_X, LDR_BRIEF_Y + 1000, 0); // DOLY LEWY
	glTexCoord2f(1, 0); glVertex3i(LDR_BRIEF_X + 500, LDR_BRIEF_Y + 1000, 0); // DOLNY PRAWY
	glTexCoord2f(1, 1); glVertex3i(LDR_BRIEF_X + 500, LDR_BRIEF_Y, 0);   // GORNY PRAWY
	glEnd();
	}
	*/
	glDisable(GL_BLEND);


	//PROGRESSBAR ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	float PBARY, PBY;
	PBY = Global::iWindowHeight - (200.0f - 8.0f);
	PBARY = PBY + 68.0f;
	float PBARLEN = Global::iWindowWidth / 100.0f; //LDR_PBARLEN;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(20.0f, PBARY - 2.0f);                                   // gorny lewy
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(20.0f, PBARY - 1.0f);                                   // dolny lewy
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(100.0f * PBARLEN, PBARY - 1.0f);                          // dolny prawy
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(100.0f * PBARLEN, PBARY - 2.0f);                          // gorny prawy
	glEnd();

	if (Global::bfirstloadingscn) Global::postep = 0;
	if (Global::bfirstloadingscn) PBARLEN = 3;

	if (!Global::bfirstloadingscn) Global::postep = (Global::iNODES * 100 / Global::iPARSERBYTESPASSED);

	if (Global::bfirstloadingscn)   // PRZY PIERWSZYM WCZYTYWANIU PASEK POSTEPU JEST USTAWIONY NA 100%
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		//glEnable(GL_BLEND);
		glColor4f(0.9f, 0.7f, 0.1f, 0.5f);
		glBegin(GL_QUADS);
		glVertex2f(20, PBARY + 2);    // gorny lewy
		glVertex2f(20, PBARY + 10);   // dolny lewy
		glVertex2f(100 * PBARLEN, PBARY + 10);   // dolny prawy
		glVertex2f(100 * PBARLEN, PBARY + 2);   // gorny prawy
		glEnd();
	}

	else
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		//glEnable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.9f, 0.7f, 0.1f, 0.5f);
		glBegin(GL_QUADS);
		glVertex2f(20.0, PBARY + 2);    // gorny lewy
		glVertex2f(20.0, PBARY + 10);   // dolny lewy
		glVertex2f(Global::postep*PBARLEN, PBARY + 10);   // dolny prawy
		glVertex2f(Global::postep*PBARLEN, PBARY + 2);   // gorny prawy
		glEnd();
	}

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	//-- RenderInformation(99);

	// if (!Global::bSCNLOADED) 
	// glfwSwapBuffers(Global::window); //SwapBuffers(hDC);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	return true;
}
