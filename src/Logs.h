//---------------------------------------------------------------------------

#ifndef LogsH
#define LogsH

#include <windows.h>        // Header File For Windows
#include <wtypes.h>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>
#include <ctime>

void __fastcall WriteConsoleOnly(char *str, double value);
void __fastcall WriteConsoleOnly(char *str);
void __fastcall WriteLog(char *str, double value);
void __fastcall WriteLog(char *str);
char* stdstrtochara(std::string var);
void __fastcall WriteLogSS(std::string text, std::string token);
void __fastcall WriteExecute(char *str);
void __fastcall WriteQ3D(LPSTR text, std::string token, char* fn, bool f);

//---------------------------------------------------------------------------
#endif
