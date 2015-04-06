//---------------------------------------------------------------------------

#ifndef AirCouplerH
#define AirCouplerH

#include "Model3d.h"
#include "parser.h"

class TAirCoupler
{
  private:
    //    TButtonType eType;
    TSubModel *pModelOn, *pModelOff, *pModelxOn;
    bool bOn;
    bool bxOn;
    void Update();

  public:
    TAirCoupler();
    ~TAirCoupler();
    void Clear();
    inline void TurnOn()
    {
        bOn = true;
        bxOn = false;
        Update();
    };
    inline void TurnOff()
    {
        bOn = false;
        bxOn = false;
        Update();
    };
    inline void TurnxOn()
    {
        bOn = false;
        bxOn = true;
        Update();
    };
    //  inline bool Active() { if ((pModelOn)||(pModelOff)) return true; return false;};
    int GetStatus();
    void Init(const std::string asName, TModel3d *pModel);
    void Load(cParser &parser, TModel3d *pModel);
    //  bool Render();
};

//---------------------------------------------------------------------------
#endif
