//---------------------------------------------------------------------------

#pragma hdrstop
#include "commons.h"
#include "commons_usr.h"

bool first= true;
char endstring[10]= "\n";
char LOGFILENAME[200];

void WriteConsoleOnly(char *str, double value)
{
    char buf[255];
    sprintf_s(buf,"%s %f \n",str,value);

//    stdout=  GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD wr= 0;
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, qstrlen(buf), &wr, NULL);  //strlen = sizeof
    //WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void WriteConsoleOnly(char *str)
{
//    printf("%n ffafaf /n",str);
    DWORD wr= 0;
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), str, strlen(str), &wr, NULL);  //strlen = sizeof
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), endstring, strlen(endstring), &wr, NULL); //strlen = sizeof
}

void WriteLog(char *str, double value)
{
 if (Global::bWriteLogEnabled)
  {
    if (str)
    {
        char buf[255];
        sprintf_s(buf,"%s %f",str,value);
        WriteLog(buf);
    }
  };
}

void WriteLog(char *str)
{   
   FILE *qstream = NULL;
   errno_t err;
   std::string cwd;
   cwd = Global::asCWD;
	
   if (str)
   {
    if (Global::bWriteLogEnabled)
      {
        
        if (first)
        {
			sprintf_s(LOGFILENAME, "%s", Global::logfilenm1.c_str());
            //stream = fopen(LOGFILENAME, "w"); // Global::logfilenm1.c_str()
			err = fopen_s(&qstream, LOGFILENAME, "w");
            first= false;
        }
        else
			err = fopen_s(&qstream, LOGFILENAME, "a+");
            //stream = fopen(LOGFILENAME, "a+");
        fprintf(qstream, str);
        fprintf(qstream, "\n");
        fclose(qstream);
      }
    WriteConsoleOnly(str);
  };
}


void WriteExecute(char *str)
{
char FN [80];
std::time_t rawtime;
std::tm* timeinfo;

char am_pm[] = "AM";

//char timebuf[26];

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
  //char* ReturnString = new char[100];
	//char ReturnString[100];
	//char szBuffer[100];
	//LPCSTR lpMyString = var.c_str();

    //sprintf_s(szBuffer,"%s",lpMyString);

	//strcpy_s( ReturnString, szBuffer );

	char *cstr = new char[var.length() + 1];
	strcpy(cstr, var.c_str());

	return cstr; // ReturnString;
	return cstr;
}

void WriteLogSS(std::string text, std::string token)
{
			char tolog[100];
			sprintf_s(tolog,"%s %s", text.c_str(), token.c_str());
            WriteLog(stdstrtochara(tolog));
}

void WriteQ3D(LPSTR text, std::string token, char* fn, bool f)
{
	char ADDTOFILE[200];
	//char Q3DFILENAME[200];
	//char tolog[100];
    FILE *qstream = NULL;
	errno_t err;


	std::string cwd;
	cwd = Global::asCWD;

	sprintf_s(ADDTOFILE, "%s%s", text, token.c_str());

	// Open for read (will fail if file "crt_fopen_s.c" does not exist)]

	err = fopen_s(&qstream, "logx.txt", "r");

        if (err==0)
        {
			//sprintf(Q3DFILENAME, "%s%s", cwd.c_str() , fn);
			//qstream = fopen(fn, "w"); // Global::logfilenm1.c_str()
            fprintf(qstream, ADDTOFILE);
            fprintf(qstream, "\n");
			fclose(qstream);
        }
        else
		{
			
       // qstream = fopen_s(fn, "a+");
        fprintf(qstream, ADDTOFILE);
        fprintf(qstream, "\n");
        fclose(qstream);
		}
}
//---------------------------------------------------------------------------

//#pragma package(smart_init)
