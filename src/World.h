//---------------------------------------------------------------------------

#ifndef _World_H_
#define _World_H_


//#include "commons.h"

class TWorld
{
public:
    void  InOutKey();
    bool  Init();
	bool  Render(double dt);
	bool  Update(double dt);
	GLvoid glPrint(const char *fmt);
    TWorld();
   ~TWorld();
private:

    bool  RenderX();

public:
};

//---------------------------------------------------------------------------
#endif
