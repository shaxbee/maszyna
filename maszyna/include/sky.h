//---------------------------------------------------------------------------

#ifndef skyH
#define skyH

#include "../commons.h"
#include "../commons_usr.h"
#include "MdlMngr.h"


class TSky
{
private:
  TModel3d *mdCloud;
public:
  __fastcall TSky();
  __fastcall ~TSky();
  void __fastcall Init();
  void __fastcall Render();
};

//---------------------------------------------------------------------------
#endif