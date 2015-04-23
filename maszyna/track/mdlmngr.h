//---------------------------------------------------------------------------
#ifndef MdlMngrH
#define MdlMngrH

#include "../commons.h"
#include "../commons_usr.h"


class TMdlContainer
{
    friend class TModelsManager;
    __fastcall TMdlContainer() { Name= NULL; Model= NULL; };
    __fastcall ~TMdlContainer() { SafeDeleteArray(Name); SafeDelete(Model); };
    TModel3d* __fastcall LoadModel(char *newName, bool dynamic);
    TModel3d *Model;
    char *Name;
};

class TModelsManager
{
private:
//    CD3DFile** Models;
    static TMdlContainer *Models;
    static int Count;
    static TModel3d* __fastcall LoadModel(char *Name, bool dynamic);
public:
//    __fastcall TModelsManager();
//    __fastcall ~TModelsManager();
   int static __fastcall Init();
   int static __fastcall Free();
//McZapkie: dodalem sciezke, notabene Path!=Patch :)
    static int __fastcall LoadModels(char *asModelsPathc);
    static TModel3d* __fastcall GetModel(char *Name, bool dynamic=false); 
};
//---------------------------------------------------------------------------
#endif
