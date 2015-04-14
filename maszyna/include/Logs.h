//---------------------------------------------------------------------------

#ifndef LogsH
#define LogsH

#include "../commons.h"
#include "cstring.h"

void WriteConsoleOnly(CString str, double value);
void WriteConsoleOnly(CString str);
void WriteLog(CString str, double value);
void WriteLog(CString str);
char* stdstrtochara(std::string var);
void WriteLogSS(std::string text, std::string token);
void WriteExecute(char *str);
void WriteQ3D(LPSTR text, std::string token, char* fn, bool f);

//---------------------------------------------------------------------------
#endif
