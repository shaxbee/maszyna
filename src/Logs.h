#ifndef LogsH
#define LogsH

#include <string>

void WriteConsoleOnly(const char *str, double value);
void WriteConsoleOnly(const char *str);
void WriteLog(const char *str, double value);
void WriteLog(const char *str);
void Error(const std::string asMessage, bool box = true);
void ErrorLog(const std::string asMessage);
void WriteLog(const std::string str);

#endif
