//---------------------------------------------------------------------------
#ifndef MdlMngrH
#define MdlMngrH


#include "Model3d.h"
#include "Object3d.h"
#include "usefull.h"

class TMdlContainer
{
public:
	TObject3d *Model3D;
    //friend class TModelsManager;
    TMdlContainer();
	~TMdlContainer();
	TObject3d* LoadModel(static char *newName, bool dynamic);
    
    char *Name;
};

class TModelsManager
{
	
public:
	static int Count;
	static TObject3d* LoadModel(static char *Name, bool dynamic);
	static TMdlContainer* Models;
    static void Init();
    static void Free();
//McZapkie: dodalem sciezke, notabene Path!=Patch :)
	static int LoadModels(std::string asModelsPathC);
	static TObject3d* GetModel(static char *Name, bool dynamic = false);
};
//---------------------------------------------------------------------------
#endif
