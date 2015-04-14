#ifndef _World_H_
#define _World_H_

#include "commons.h"
#include "commons_usr.h"
#include "cstring.h"
class TWorld
{
public:
    void  InOutKey();
    bool  Init();
	bool  Render(double dt);
	bool  Update(double dt);
	void  OnKeyDown(int cKey, int scancode, int action, int mode, std::string command);
	void  OnKeyUp(int cKey, int scancode, int action, int mode, std::string command);
	void  OnMouseMove(double x, double y);
	GLvoid glPrint(CString fmt);
    TWorld();
   ~TWorld();
  
private:

    bool  RenderX();

public:
	
};

#endif
