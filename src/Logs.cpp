//---------------------------------------------------------------------------


#pragma hdrstop
#include <stdio.h>
#include "Logs.h"
#include "Globals.h"


bool first= true;
char endstring[10]= "\n";
char LOGFILENAME[200];

void __fastcall WriteConsoleOnly(char *str, double value)
{
    char buf[255];
    sprintf(buf,"%s %f \n",str,value);

//    stdout=  GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD wr= 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),buf,strlen(buf),&wr,NULL);
    //WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void __fastcall WriteConsoleOnly(char *str)
{
//    printf("%n ffafaf /n",str);
    DWORD wr= 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),str,strlen(str),&wr,NULL);
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void __fastcall WriteLog(char *str, double value)
{
 if (Global::bWriteLogEnabled)
  {
    if (str)
    {
        char buf[255];
        sprintf(buf,"%s %f",str,value);
        WriteLog(buf);
    }
  };
}
void __fastcall WriteLog(char *str)
{
	std::string cwd;
	cwd = Global::asCWD;
	

	
   if (str)
   {
    if (Global::bWriteLogEnabled)
      {
        FILE *stream=NULL;
        if (first)
        {
			sprintf(LOGFILENAME, "%s%s", cwd.c_str() , Global::logfilenm1.c_str());
            stream = fopen(LOGFILENAME, "w"); // Global::logfilenm1.c_str()
            first= false;
        }
        else
            stream = fopen(LOGFILENAME, "a+");
        fprintf(stream, str);
        fprintf(stream, "\n");
        fclose(stream);
      }
    WriteConsoleOnly(str);
  };
}


void __fastcall WriteExecute(char *str)
{
char FN [80];
std::time_t rawtime;
std::tm* timeinfo;
  
std::time(&rawtime);
timeinfo = std::localtime(&rawtime);

std::strftime(FN, 80, "[%Y%m%d %H%M%S]", timeinfo);
str = FN;

   if (str)
   {
        FILE *stream=NULL;
        if (first)
        {
            stream = fopen("log\\executes.log", "w");
            first= false;
        }
        else
            stream = fopen("log\\executes.log", "a+");
        fprintf(stream, str);
        fprintf(stream, "\n");
        fclose(stream);
  };
}

// ******************************************************************************************
// stdstrtochara()
// ******************************************************************************************

char* stdstrtochara(std::string var)
{
	char* ReturnString = new char[100];
	char szBuffer[100];
	LPCSTR lpMyString = var.c_str();

    sprintf(szBuffer,"%s",lpMyString);

	strcpy( ReturnString, szBuffer );

	return ReturnString;
}

void __fastcall WriteLogSS(std::string text, std::string token)
{
			char tolog[100];
			sprintf(tolog,"%s %s", text.c_str(), token.c_str());
            WriteLog(stdstrtochara(tolog));
}

void __fastcall WriteQ3D(LPSTR text, std::string token, char* fn, bool f)
{
	char ADDTOFILE[200];
	char Q3DFILENAME[200];
	char tolog[100];
    FILE *qstream = NULL;

	std::string cwd;
	cwd = Global::asCWD;

	sprintf(ADDTOFILE, "%s%s", text, token.c_str());

       
        if (f)
        {
			//sprintf(Q3DFILENAME, "%s%s", cwd.c_str() , fn);
			qstream = fopen(fn, "w"); // Global::logfilenm1.c_str()
            fprintf(qstream, ADDTOFILE);
            fprintf(qstream, "\n");
			fclose(qstream);
        }
        else
		{
        qstream = fopen(fn, "a+");
        fprintf(qstream, ADDTOFILE);
        fprintf(qstream, "\n");
        fclose(qstream);
		}
}
//---------------------------------------------------------------------------

#pragma package(smart_init)
