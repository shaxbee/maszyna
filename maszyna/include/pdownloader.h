//---------------------------------------------------------------------------

#ifndef pdownloaderH
#define pdownloaderH

#include "../commons.h"
#include "../commons_usr.h"
#include "globals.h"

bool CHECKUPDATES(bool check);
bool downloadfile(std::string url, std::string loc, int fsize, bool chksize);

#endif
