#ifndef qstrutilsH
#define qstrutilsH

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "../commons.h"
#include "../commons_usr.h"

// ***********************************************************************************************************************
// SPRAWDZA CZY CIaG ZNAKOW JEST W LINII
// ***********************************************************************************************************************


// **************************************************************************************************************************
// ReplaceCharInString() - REPLACES ALL CHOSED CHAR TO OTHER GIVEN
// **************************************************************************************************************************
/*
std::string ReplaceCharInString( std::string source, char ToReplace, const std::string newString) 
{ 
    std::string result; 
 
    // For each character in source string: 
    const char * pch = source.c_str(); 
    while ( *pch != '\0' ) 
    { 
        // Found character to be replaced? 
        if ( *pch == ToReplace )  result += newString; 
        else  result += (*pch); // Just copy original character 

        ++pch; // Move to next character 
    } 
 
    return result; 
} 
*/


inline std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

inline char* arraytoptr(char aPointer[100])
{
                    int i;
    //char aPointer[] = "I love earth and all its creatures.";

   // strcpy( aPointer, xxx.c_str());
    char *pCarrier[sizeof(aPointer)];
    for (i = 0; i < sizeof(aPointer); i++)
        pCarrier[i] = &aPointer[i];

       return  *pCarrier;
}

// **************************************************************************************
// GETCWD()
// **************************************************************************************

inline std::string __fastcall GETCWD()
 {
   char* cwdbuffer;
   char szBuffer[200];

   // Get the current working directory:
   if( (cwdbuffer = _getcwd( NULL, 0 )) == NULL )  perror( "_getcwd error" );
   else
   {
	  Global::asCWD = cwdbuffer;
	  sprintf_s(szBuffer,"CWD: [%s]", cwdbuffer);
	  WriteLog(szBuffer);
      free(cwdbuffer);
   }
   return cwdbuffer;
}



// *****************************************************************************
// stdstrtochar()
// *****************************************************************************
inline char* stdstrtochar(std::string var)
{
//	char* ReturnString = new char[100];
	//char ReturnString[100];
	//char szBuffer[100];
	//LPCSTR lpMyString = var.c_str();
    //sprintf_s(szBuffer,"%s",lpMyString);
	//strcpy_s( ReturnString, szBuffer );

	char *cstr = new char[var.length() + 1];
	strcpy(cstr, var.c_str());

	return cstr; // ReturnString;
}




// *****************************************************************************
// chartostdstr()
// *****************************************************************************
inline std::string chartostdstr(char *var)
{
std::string strFromChar;
strFromChar.append(var);

return strFromChar;
}


// *********************************************************************************************
// itos() - KONWERTOWANIE INTEGRA DO CHAR
// *********************************************************************************************
/*
inline char* itos(int val)
{
	//char* ReturnString = new char[100];
	char ReturnString[100];
	char szBuffer[100];

    sprintf_s(szBuffer,"%i",val);

    strcpy_s(ReturnString, szBuffer);

	return ReturnString;
}
*/

// *********************************************************************************************
// ftos() - KONWERTOWANIE FLOAT DO CHAR
// *********************************************************************************************
/*
inline char* ftos(float val)
{
	//char* ReturnString = new char[100];
	char ReturnString[100];
	char szBuffer[100];

    sprintf_s(szBuffer,"%f", val);

	strcpy_s( ReturnString, szBuffer );

	return ReturnString;
}
*/

// *********************************************************************************************
// ftoss() - KONWERTOWANIE FLOAT DO std::string
// *********************************************************************************************

inline std::string ftoss(float val)
{
	std::string ReturnString;
	char szBuffer[100];

    sprintf_s(szBuffer,"%f",val);

	ReturnString = chartostdstr(szBuffer);
	//strcpy( ReturnString, szBuffer );

	return ReturnString;
}

// *********************************************************************************************
// itoss() - KONWERTOWANIE int DO std::string
// *********************************************************************************************
/*
inline std::string itoss(int val)
{
	std::string ReturnString;
	char szBuffer[100];

    sprintf_s(szBuffer,"%i",val);

	ReturnString = chartostdstr(szBuffer);
	//strcpy( ReturnString, szBuffer );

	return ReturnString;
}
*/
inline std::string itoss(int val)
{
	std::string str = std::to_string(val);
	return str;
}


// *********************************************************************************************
// atoss() -
// *********************************************************************************************

inline std::string atoss(char aPointer[100])
{
	std::string ReturnString;
	char szBuffer[100];

    sprintf_s(szBuffer,"%s",aPointer);

	ReturnString = chartostdstr(szBuffer);
	//strcpy( ReturnString, szBuffer );

	return ReturnString;
}

// *****************************************************************************
//
// *****************************************************************************
inline std::string ToLowerCase(std::string text)
{

   for(int i = 0; i<text.length(); i++){
      char c = text[i]; 
      if((c>=65)&&(c<=90)){
	 text[i]|=0x20; 
      }
   }
   return text; 
}


inline int qstrlen(char *inp)
{
	char *p = inp;

	while (*p)
		p++;

	return p - inp;
}

//---------------------------------------------------------------------------
#endif
