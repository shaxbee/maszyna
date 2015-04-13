//---------------------------------------------------------------------------

#ifndef LogsH
#define LogsH

#include "../commons.h"

void WriteConsoleOnly(char *str, double value);
void WriteConsoleOnly(char *str);
void WriteLog(char *str, double value);
void WriteLog(char *str);
char* stdstrtochara(std::string var);
void WriteLogSS(std::string text, std::string token);
void WriteExecute(char *str);
void WriteQ3D(LPSTR text, std::string token, char* fn, bool f);

//---------------------------------------------------------------------------
#endif
